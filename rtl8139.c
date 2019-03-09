
#include <rtl8139.h>
#include <stdio.h>
#include <system.h>
#include <pci.h>


// Refrence : http://wiki.osdev.org/RTL8139
// http://minirighi.sourceforge.net/html/group__RTL8139Driver.html

void rtl8139_set_promisc_mode(u16_t iobase, int mode);
void rtl8139_read_mac_address(u16_t iobase, u8_t *mac);



/*
	The RTL8139 interrupt handler routine. 
	It is invoked every time that an ethernet packet is received or when a packet has been transmitted 
*/               
u32_t rtl8139_handler(u32_t context)
{

	printf("Received INT for network interface ...............\n");
	//while(1);
/*
	u16_t status;
 
	out16(rtl->iobase + IntrMask, 0);
	status = in16(rtl->iobase + IntrStatus);

	// Acknowledge all known interrupts                             //
	out16(rtl->iobase + IntrStatus, status & ~(RxFIFOOver | RxOverflow | RxOK));

	if (status & (RxOK | RxErr))
               rtl8139_handle_rx(rtl);
	else
		if (status & (TxOK | TxErr))
			rtl8139_handle_tx(rtl);
		else
			printf("\n\rRTL8139: unknown interrupt: isr = %#010x\n", status);


	// Re-enable interrupts from the RTL8139 card                   //
	out16(rtl->iobase + IntrStatus, status & (RxFIFOOver | RxOverflow | RxOK));
	out16(rtl->iobase + IntrMask, IntrDefault);

*/
	return context;
}





void init_rtl8139(PCI_Device* dev, netif* net_if)
{

	// install interupt handler
	irq_install_handler(dev->irq + 32, rtl8139_handler);		// IRQ-xx

   	/* enable the RTL8139 chipset and reset it by..
       		- clearing the config 0 word
       		- setting bit 4 (software reset) in the
         	command register
       		- wait for the software reset bit to clear
   	*/
	outb(dev->bar[0] + RTL8139_PCI_CONFIG1, 0x00);
	outb(dev->bar[0] + RTL8139_PCI_CMD, RTL8139_PCI_CR_RESET);
   	while( (inb(dev->bar[0] + RTL8139_PCI_CMD) & RTL8139_PCI_CR_RESET) != 0 );
   
   	/* get the card's MAC address */
	u8_t mac[6];
   	rtl8139_read_mac_address(dev->bar[0], mac);
	printf("NIC MAC Address : %2x:%2x:%2x:%2x:%2x:%2x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	rtl8139_set_promisc_mode(dev->bar[0], 1);


 char rx_buffer[8192+16]; // declare the local buffer space (8k + header)
 outl(dev->bar[0] + 0x30, (unsigned long)rx_buffer); // send dword memory location to RBSTART (0x30)

outw(dev->bar[0] +0x3C, 0x0005); // Sets the TOK and ROK bits high

outl(dev->bar[0] + 0x44, 0xf | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

	outb(dev->bar[0] + 0x37, 0x0C); // Sets the RE and TE bits high


}

void rtl8139_read_mac_address(u16_t iobase, u8_t *mac)
{
   	int count;
   
   	for(count = 0; count < 6; count++)
      		mac[count] = inb(iobase + RTL8139_PCI_MAC0 + count);
}


void rtl8139_set_promisc_mode(u16_t iobase, int mode)
{
	
	// read configuration register
	u8_t rxc = inb(iobase + RTL8139_PCI_RX_CONFIG);

	if ( mode )
		outb(iobase + RTL8139_PCI_RX_CONFIG, rxc | RX_CONFIG_ACCEPT_ALL_PHYS);
	else
		outb(iobase + RTL8139_PCI_RX_CONFIG, rxc & (~RX_CONFIG_ACCEPT_ALL_PHYS));
	
}


