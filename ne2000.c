
#include <ne2000.h>
#include <stdio.h>
#include <system.h>
#include <mutex.h>
#include <pci.h>
#include <pbuf.h>
#include <stats.h>
#include <ether.h>

//	http://www.jbox.dk/sanos/source/sys/dev/ne2000.c.html#:438

unsigned static volatile char dma_irq_invoked = 0;
unsigned static volatile char trx_irq_invoked = 0;

extern netstats stats;


static u32_t ne_iobase;
static u8_t rx_page_start;
static u8_t rx_page_stop;
static u8_t next_pkt;
static u16_t rx_ring_start;
static u16_t rx_ring_end;
static u8_t irq;

static mutex_t ne_mutex;

static netif* netIf;

_insl(u16_t port , void * address , int count)
{
        asm volatile("cld; rep insl" :
                "=D" (address), "=c" (count) :
                "d" (port), "0" (address), "1" (count) :
                "memory", "cc");
}

_insw(u16_t port , void * address , int count)
{
        asm volatile("cld; rep insw" :
                "=D" (address), "=c" (count) :
                "d" (port), "0" (address), "1" (count) :
                "memory", "cc");
}

u32_t ne2000_handler(u32_t context)
{
	u8_t isr;

	//printf("Received INT for network interface ...............\n");
	//printf("...");
//while(1);

	// Select page 0
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STA);

  	// Loop until there are no pending interrupts
	while ((isr = inb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_ISR)) != 0) 
	{
		// Reset bits for interrupts being acknowledged
		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_ISR, isr);

    		// Packet received
    		if (isr & NE_ISR_PRX) 
		{
static int c = 0;
c++;
      			//printf("ne2000: new packet arrived %d\n", c);
      			ne2000_receive();
    		}

    		// Packet transmitted
    		if (isr & NE_ISR_PTX) 
		{
      			printf("ne2000: packet transmitted\n");
      			trx_irq_invoked = 1;
    		}

    		// Remote DMA complete
    		if (isr & NE_ISR_RDC) 
		{
      			//printf("ne2000: remote DMA complete\t");
      			dma_irq_invoked = 1;
    		}

    		// Select page 0
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STA);
  
	}	// end while loop

//printf("END\n");

	return context;
}

void dma_wait_irq()
{
	while (!dma_irq_invoked);
	dma_irq_invoked = 0;
}

void trx_wait_irq()
{
	while (!trx_irq_invoked);
	trx_irq_invoked = 0;
}

void ne2000_read_mac_address(u16_t iobase, u8_t *mac)
{
	int len = 12;
	u8_t buff[len];
			
	ne2000_readmem(0, buff, len);

   	int count;
   	for(count = 0; count < len/2; count++)
      		mac[count] = buff[count*2];
}


//int ne2000_transmit(netif *net_if, pbuf *p, ip_addr *ipaddr)
int ne2000_transmit(netif *net_if, pbuf *p)  
{
  	u16_t dma_len;
  	u16_t dst;
  	u8_t *data;
  	int len, i;
  	pbuf *q;

  	printf("ne2000_transmit : transmit packet len=%d\n", p->tot_len);

	lock(&ne_mutex);

  	// We need to transfer a whole number of words
  	dma_len = p->tot_len;
  	if (dma_len & 1) dma_len++;
//printf("here1\n");
  	// Set page 0 registers
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STA);

  	// Reset remote DMA complete flag
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_ISR, NE_ISR_RDC);
//printf("here2\n");
  	// Set up DMA byte count
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RBCR0, (u8_t) dma_len);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RBCR1, (u8_t) (dma_len >> 8));

  	// Set up destination address in NIC memory
  	dst = rx_page_stop; // for now we only use one tx buffer
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RSAR0, (dst * NE_PAGE_SIZE));
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RSAR1, (dst * NE_PAGE_SIZE) >> 8);
//printf("here3\n");
  	// Set remote DMA write
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD1 | NE_CR_STA);
//printf("here4\n");

  	for (q = p; q != NULL; q = q->next) 
  	{
    		len = q->len;
    		
		if (len > 0) 
		{
      			data = q->payload;

     			if ( len / 4 )
			{
				for(i=0;i<len/4;++i)	
				{
					outl(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_DATA, data);
					data+= 4;
				}
				len = len % 4;
			}

			if ( len / 2 )
			{
				for(i=0;i<len/2;++i)
				{
					outw(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_DATA, data);
					data+= 2;
				}
				len = len % 2;
			}

			if (len )	outb(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_DATA, data);	

    		}
	}

//printf("before dma wait ... %d\n", dma_irq_invoked);
	dma_wait_irq();
//printf("after dma wait ... \n");

  	// Set TX buffer start page
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_TPSR, (u8_t) dst);

  	// Set TX length (packets smaller than 64 bytes must be padded)
  	if (p->tot_len > 64) 
	{
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_TBCR0, p->tot_len);
		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_TBCR1, p->tot_len >> 8);
	}
	else
	{
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_TBCR0, 64);
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_TBCR1, 0);
  	}

  	// Set page 0 registers, transmit packet, and start
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_TXP | NE_CR_STA);

//printf("before trx wait ... \n");
	trx_wait_irq();
//printf("after trx wait ... \n");

  	printf("NE-2000_transmit: packet transmitted\n");
	//pbuf_free(p);
  
	unlock(&ne_mutex);
	
	return 0;
}



void ne2000_receive()
{
	recv_ring_desc packet_hdr;
  	u16_t packet_ptr;
  	u16_t len;
  	u8_t bndry;
  	pbuf *p, *q;
  	int rc;

	// Set page 1 registers
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_PAGE_1 | NE_CR_RD2 | NE_CR_STA);

  	while ( next_pkt != inb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P1_CURR)) 
	{
    		// Get pointer to buffer header structure
    		packet_ptr = next_pkt * NE_PAGE_SIZE;

    		// Read receive ring descriptor
    		ne2000_readmem(packet_ptr, &packet_hdr, sizeof(recv_ring_desc));

    		// Allocate packet buffer
    		len = packet_hdr.count - sizeof(recv_ring_desc);
		//printf("length = %d\n", len);

static int num = 0;
//printf("ne2000: packet received = %d of len = %d\n", num++ , len);
  
  		p = pbuf_alloc(PBUF_RAW, len, PBUF_RW);

    		// Get packet from nic and send to upper layer
    		if (p != NULL) 
		{

     		 	packet_ptr += sizeof(recv_ring_desc);

      			for (q = p; q != NULL; q = q->next) 
			{

        			ne_get_packet(packet_ptr, q->payload, (u16_t) q->len);
 				packet_ptr += q->len;
      			}

      			//printf("ne2000: received packet, %d bytes\n", len);
			//rc = dev_receive(ne->devno, p); 

			//if ( netIf )	rc = ether_input(netIf, p);
			rc = enqueue(netIf, p);

    		} 
		else 
		{
      			// Drop packet
      			printf("ne2000: packet dropped\n");
while(1);
      			stats.link.memerr++;
      			stats.link.drop++;
    		}

		// Update next packet pointer
    		next_pkt = packet_hdr.next_pkt;

    		// Set page 0 registers
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_PAGE_0 | NE_CR_RD2 | NE_CR_STA);

    		// Update boundry pointer
    		bndry = next_pkt - 1;
    		if (bndry < rx_page_start) bndry = rx_page_stop - 1;
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_BNRY, bndry);

    		//printf("start: %02x stop: %02x next: %02x bndry: %02x\n", rx_page_start, rx_page_stop, next_pkt, bndry);

    		// Set page 1 registers
    		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_PAGE_1 | NE_CR_RD2 | NE_CR_STA);
  	}

}


void ne_get_packet(u16_t src, char *dst, u16_t len) 
{

	if (src + len > rx_ring_end) 
	{
    		u16_t split = rx_ring_end - src;
    		ne2000_readmem(src, dst, split);
    		len -= split;
    		src = rx_ring_start;
    		dst += split;
  	}
  	ne2000_readmem(src, dst, len);
}


void ne2000_readmem(u16_t src, void *dst, u16_t len) 
{

	// Word align length
  	if (len & 1) len++;
	//if (len % 4 ) len = len + ( 4 - len % 4 );

  	// Abort any remote DMA already in progress
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STA);

  	// Setup DMA byte count
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RBCR0, (unsigned char) len);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RBCR1, (unsigned char) (len >> 8));

  	// Setup NIC memory source address
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RSAR0, (unsigned char) src);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RSAR1, (unsigned char) (src >> 8));

  	// Select remote DMA read
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD0 | NE_CR_STA);

  	// Read NIC memory
  	//insw(ne->asic_addr + NE_NOVELL_DATA, dst, len >> 1);
	_insw(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_DATA, dst, len >> 1);
	//_insl(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_DATA, dst, len >> 2 /* len / 4 */);

}



void init_ne2000(PCI_Device* dev, netif* net_if)
{
	int membase =  NE2000_MEM_BASE;
	int memsize =  NE2000_MEM_SIZE;

	ne_iobase = dev->bar[0];

  	// Start of receive ring
	rx_page_start = membase / NE_PAGE_SIZE;

	// End of receive ring
  	rx_page_stop = rx_page_start + (memsize / NE_PAGE_SIZE) - NE_TXBUF_SIZE * NE_TX_BUFERS;
  	
	next_pkt = rx_page_start + 1;

  	// Start address of receive ring
	rx_ring_start = rx_page_start * NE_PAGE_SIZE;

	// End address of receive ring
  	rx_ring_end = rx_page_stop * NE_PAGE_SIZE;	

	// Reset
  	u8_t byte = inb(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_RESET);
  	outb(ne_iobase + NE_NOVELL_ASIC_OFFSET + NE_NOVELL_RESET, byte);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STP);

	sleep(100);

  	// Test for a generic DP8390 NIC
  	byte = inb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR);
  	byte &= NE_CR_RD2 | NE_CR_TXP | NE_CR_STA | NE_CR_STP;
  	if (byte != (NE_CR_RD2 | NE_CR_STP)) 
	{
		printf("ERROR : It isn't a Generic DP8390 NIC!\n");
		return ;
	}
  	byte = inb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_ISR);
  	byte &= NE_ISR_RST;
	if (byte != NE_ISR_RST)	
	{
		printf("ERROR : It isn't a Generic DP8390 NIC!\n");
		return ;
	}

	mutex_init(&ne_mutex);

	// install interupt handler
	irq = dev->irq;
	irq_install_handler(dev->irq + 32, ne2000_handler);		// IRQ-xx

	// Set page 0 registers, abort remote DMA, stop NIC
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STP);

  	// Set FIFO threshold to 8, no auto-init remote DMA, byte order=80x86, word-wide DMA transfers
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_DCR, NE_DCR_FT1 | NE_DCR_WTS | NE_DCR_LS);

	/* get the card's MAC address */
	u8_t mac[6];
   	ne2000_read_mac_address(ne_iobase, mac);
	printf("NIC MAC Address : %2x:%2x:%2x:%2x:%2x:%2x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	// Set page 0 registers, abort remote DMA, stop NIC
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STP);

  	// Clear remote byte count registers
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RBCR0, 0);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RBCR1, 0);

  	// Initialize receiver (ring-buffer) page stop and boundry
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_PSTART, rx_page_start);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_PSTOP, rx_page_stop);
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_BNRY, rx_page_start);

  	// Enable the following interrupts: receive/transmit complete, receive/transmit error, 
  	// receiver overwrite and remote dma complete.
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_IMR, NE_IMR_PRXE | NE_IMR_PTXE | NE_IMR_RXEE | NE_IMR_TXEE | NE_IMR_OVWE | NE_IMR_RDCE);

  	// Set page 1 registers
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_PAGE_1 | NE_CR_RD2 | NE_CR_STP);

  	// Copy out our station address
	int i;
  	for (i = 0; i < 6; i++) 
		outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P1_PAR0 + i, mac[i]);

  	// Set current page pointer 
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P1_CURR, next_pkt);

	// Initialize multicast address hashing registers to not accept multicasts
  	for (i = 0; i < 8; i++) outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P1_MAR0 + i, 0);

  	// Set page 0 registers
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STP);

  	// Accept broadcast packets
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RCR, NE_RCR_AB);

	//-------------------------------------------------------------------------
	// set Promiscuous mode
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_RCR, NE_RCR_PRO);
	//-------------------------------------------------------------------------

  	// Take NIC out of loopback
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_TCR, 0);

  	// Clear any pending interrupts
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_ISR, 0xFF);

  	// Start NIC
  	outb(ne_iobase + NE_NOVELL_NIC_OFFSET + NE_P0_CR, NE_CR_RD2 | NE_CR_STA);


	// fill net_if structure
	netIf = net_if;

	// 1) MAC addr
	for (i = 0; i < 6; i++)
		net_if->hwaddr[i] = mac[i];
	
	// 2) transmit handler
	net_if->output = ne2000_transmit;

	return;

}
