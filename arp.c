
#include <stdio.h>
#include <arp.h>
#include <ip_addr.h>
#include <thread.h>
#include <stats.h>

#define time_after(a, b)     ((long) (b) - (long) (a) < 0)
#define time_before(a, b)    time_after(b, a)

#define time_after_eq(a, b)  ((long) (a) - (long) (b) >= 0)
#define time_before_eq(a ,b) time_after_eq(b, a)


extern netstats stats;

static arp_entry arp_table[ARP_TABLE_SIZE];
static xmit_queue_entry xmit_queue_table[ARP_XMIT_QUEUE_SIZE];


static void add_arp_entry(ip_addr *ipaddr, eth_addr *ethaddr);
void arp_timer_handler();

void arp_init() 
{
	int i;
  
  	for (i = 0; i < ARP_TABLE_SIZE; ++i) 
		ip_addr_set(&arp_table[i].ipaddr, IP_ADDR_ANY);
  
	// init timer
//printf("++ before thread\n");
	THandler handler = create_thread(&arp_timer_handler, NULL);
//printf("++ after thread\n");	
	
}


void arp_timer_handler()
{

	int interval = ARP_TIMER_INTERVAL * 1000;	// to milli-second
	int i, arp_ctime;
//printf("$$$$$$$$$$$$$$$$$$$$$$$\n");
	while(1)
	{
	//printf("$$$$$$$$$$$$$44$$$$$$$$$$\n");
		arp_ctime = gettickcount();
  		for (i = 0; i < ARP_TABLE_SIZE; ++i) 
		{
    			if (!ip_addr_isany(&arp_table[i].ipaddr) && (arp_ctime - arp_table[i].ctime) / 1000 >= ARP_MAX_AGE) 
			{
      				//printf("arp: expired entry %d\n", i);
      				ip_addr_set(&arp_table[i].ipaddr, IP_ADDR_ANY);
    			}
  		}
 		//printf("$$$$$$$$$$$$$44$$$$$$$$$$\n");
		sleep(interval);	
	}
}

eth_addr* arp_lookup(ip_addr *ipaddr) 
{
  	int i;
  
  	for (i = 0; i < ARP_TABLE_SIZE; ++i) 
	{
    		if (ip_addr_cmp(ipaddr, &arp_table[i].ipaddr)) 
			return &arp_table[i].ethaddr;
  	}
 
	return NULL;  
}

static void add_arp_entry(ip_addr *ipaddr, eth_addr *ethaddr) 
{
	int i, j, k;
  	int maxtime, arp_ctime;
  	int err;
  


  	// Walk through the ARP mapping table and try to find an entry to
  	// update. If none is found, the IP -> MAC address mapping is
  	// inserted in the ARP table.
  	for (i = 0; i < ARP_TABLE_SIZE; i++) 
	{
    		// Only check those entries that are actually in use.
    		if (!ip_addr_isany(&arp_table[i].ipaddr)) 
		{
      			// Check if the source IP address of the incoming packet matches
      			// the IP address in this ARP table entry.
      			if (ip_addr_cmp(ipaddr, &arp_table[i].ipaddr)) 
			{
        			// An old entry found, update this and return.
        			for (k = 0; k < 6; ++k) 
					arp_table[i].ethaddr.addr[k] = ethaddr->addr[k];
        			
				arp_table[i].ctime = gettickcount();	// in milli-second
        			return;
      			}
    		}

  	} // end for loop

  	// If we get here, no existing ARP table entry was found, so we create one.
  	// First, we try to find an unused entry in the ARP table.
  	for (i = 0; i < ARP_TABLE_SIZE; i++) 
	{
    		if (ip_addr_isany(&arp_table[i].ipaddr)) break;
  	}

  	// If no unused entry is found, we try to find the oldest entry and throw it away
  	if (i == ARP_TABLE_SIZE) 
	{
    		j = 0;
		maxtime = arp_table[j].ctime;
		arp_ctime = gettickcount();
    		for (i = 0; i < ARP_TABLE_SIZE; ++i) 
		{
      			if ( arp_table[i].ctime > maxtime) 
			{
        			maxtime = arp_table[i].ctime;
        			j = i;
      			}
    		}
    		i = j;
  	}

  		
	// Now, i is the ARP table entry which we will fill with the new information.
  	ip_addr_set(&arp_table[i].ipaddr, ipaddr);
  	for (k = 0; k < 6; k++) 
		arp_table[i].ethaddr.addr[k] = ethaddr->addr[k];
  	
	arp_table[i].ctime = gettickcount();

	 
//printf("-------%d.%d.%d.%d\t %2x:%2x:%2x:%2x:%2x:%2x \n",(ipaddr->addr & 0x000000FF ),((ipaddr->addr & 0x0000FF00 ) >> 8 ),((ipaddr->addr & 0x00FF0000 ) >> 16 ),((ipaddr->addr & 0xFF000000 ) >> 24 ) ,ethaddr->addr[0], ethaddr->addr[1],ethaddr->addr[2],ethaddr->addr[3], ethaddr->addr[4],ethaddr->addr[5]);


  	// Check for delayed transmissions
  	for (i = 0; i < ARP_XMIT_QUEUE_SIZE; i++) 
	{
    		xmit_queue_entry *entry = xmit_queue_table + i;

    		if (entry->p && ip_addr_cmp(&entry->ipaddr, ipaddr)) 
		{
      			pbuf *p = entry->p;
      			eth_hdr *ethhdr = p->payload;

      			entry->p = NULL;

      			for (i = 0; i < 6; i++) 
			{
        			ethhdr->dest.addr[i] = ethaddr->addr[i];
        			ethhdr->src.addr[i] = entry->net_if->hwaddr[i];
      			}
      			
			ethhdr->type = htons(ETHTYPE_IP);

			err = entry->net_if->output(entry->net_if, p);
      			
			if (err < 0) 
			{
        			printf("arp: error %d in delayed transmit\n", err);
        			pbuf_free(p);
      			}
    		}
  	} // end for loop

}


void arp_ip_input(netif* net_if, pbuf *p) 
{
	ethip_hdr *hdr;
  
  	hdr = p->payload;
  
  	// Only insert/update an entry if the source IP address of the
  	// incoming IP packet comes from a host on the local network.
  	if (!ip_addr_maskcmp(&hdr->ip.src, &net_if->ip_addr, &net_if->netmask)) return;

  	add_arp_entry(&hdr->ip.src, &hdr->eth.src);
}



pbuf *arp_arp_input(netif *net_if, eth_addr *ethaddr, pbuf *p) 
{
	arp_hdr *hdr;
  	int i;
  
  	if ( p->tot_len < sizeof(arp_hdr)) 
	{
    		pbuf_free(p);
    		return NULL;
  	}

  	hdr = p->payload;
  

//printf("-------%d %d\n", hdr->opcode, ntohs(hdr->opcode));
  	switch(ntohs(hdr->opcode)) 
	{
    		case ARP_REQUEST:
//			printf("ether: ARP packet request\n");
      			// ARP request. If it asked for our address, we send out a reply
      			if (ip_addr_cmp(&hdr->dipaddr, &net_if->ip_addr)) 
			{
        			hdr->opcode = htons(ARP_REPLY);
//printf("arp here 1\n");
        			ip_addr_set(&hdr->dipaddr, &hdr->sipaddr);
        			ip_addr_set(&hdr->sipaddr, &net_if->ip_addr);
//printf("arp here 2\n");
        			for (i = 0; i < 6; i++) 
				{
          				hdr->dhwaddr.addr[i] = hdr->shwaddr.addr[i];
          				hdr->shwaddr.addr[i] = ethaddr->addr[i];
          				hdr->ethhdr.dest.addr[i] = hdr->dhwaddr.addr[i];
          				hdr->ethhdr.src.addr[i] = ethaddr->addr[i];
        			}
//printf("arp here 3\n");
        			hdr->hwtype = htons(HWTYPE_ETHERNET);
        			ARPH_HWLEN_SET(hdr, 6);
      
        			hdr->proto = htons(ETHTYPE_IP);
        			ARPH_PROTOLEN_SET(hdr, sizeof(ip_addr));      
//printf("arp here 4\n");      
        			hdr->ethhdr.type = htons(ETHTYPE_ARP);      
        			return p;
      			}
      			break;

    		case ARP_REPLY:
			//printf("ether: ARP packet reply\n");
      			// ARP reply. We insert or update the ARP table.
      			if (ip_addr_cmp(&hdr->dipaddr, &net_if->ip_addr)) 
			{
        			add_arp_entry(&hdr->sipaddr, &hdr->shwaddr);
        			//dhcp_arp_reply(&hdr->sipaddr);
      			}
      			break;

    		default:
      			break;
  	}
//printf("arp here 5\n");
  	pbuf_free(p);
//printf("arp here 6\n");  
	return NULL;
}


pbuf* arp_query(netif *net_if, eth_addr *ethaddr, ip_addr *ipaddr) 
{
	arp_hdr *hdr;
   	pbuf *p;
  	int i;

  	p = pbuf_alloc(PBUF_LINK, sizeof(arp_hdr), PBUF_RW);
  	if (p == NULL) return NULL;

  	hdr = p->payload;
  	hdr->opcode = htons(ARP_REQUEST);

  	for (i = 0; i < 6; ++i) 
	{
    		hdr->dhwaddr.addr[i] = 0x00;
    		hdr->shwaddr.addr[i] = ethaddr->addr[i];
  	}
  
  	ip_addr_set(&hdr->dipaddr, ipaddr);
  	ip_addr_set(&hdr->sipaddr, &net_if->ip_addr);

  	hdr->hwtype = htons(HWTYPE_ETHERNET);
  	ARPH_HWLEN_SET(hdr, 6);

  	hdr->proto = htons(ETHTYPE_IP);
  	ARPH_PROTOLEN_SET(hdr, sizeof(ip_addr));

  	for (i = 0; i < 6; ++i) 
	{
    		hdr->ethhdr.dest.addr[i] = 0xFF;
    		hdr->ethhdr.src.addr[i] = ethaddr->addr[i];
  	}
  
  	hdr->ethhdr.type = htons(ETHTYPE_ARP);      
  	
	return p;
}

int arp_queue(netif *net_if, pbuf *p, ip_addr *ipaddr) 
{
  	int i;
  	xmit_queue_entry *entry = NULL;

  	// Find empty entry
  	for (i = 0; i < ARP_XMIT_QUEUE_SIZE; i++) 
	{
    		if (xmit_queue_table[i].p == NULL) 
		{
      			entry = &xmit_queue_table[i];
      			break;
    		}
  	}

/*
  	// If no entry entry found, try to find an expired entry
  	if (entry == NULL) 
	{
    		for (i = 0; i < ARP_XMIT_QUEUE_SIZE; i++) 
		{
      			if (time_before(xmit_queue_table[i].expires, ticks)) 
			{
        			entry = &xmit_queue_table[i];
        			break;
      			}
    		}
  	}
*/
  	// If there are no room in the xmit queue, we have to drop the packet
  	if (!entry) 
	{
   	 	stats.link.drop++;
    		return -1;
  	}

  	// Expire entry if it is not empty
  	if (entry->p) 
	{
    		pbuf_free(entry->p);
    		stats.link.drop++;
  	}

  	// Fill xmit queue entry
  	entry->net_if = net_if;
  	entry->p = p;
  	ip_addr_set(&entry->ipaddr, ipaddr);
  	//entry->expires = ticks + MAX_XMIT_DELAY / MSECS_PER_TICK;
  
  	return 0;
}


