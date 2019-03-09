

#include <stdio.h>
#include <ether.h>
#include <ip.h>
#include <arp.h>
#include <stats.h>
#include <debug.h>


extern netstats stats;
static const eth_addr ethbroadcast = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

err_t ether_input(netif *net_if, pbuf *p) 
{
	eth_hdr *ethhdr;

  	//if ((net_if->flags & NETIF_UP) == 0) return -ENETDOWN;

  	if (p->len < ETHER_HLEN) 
	{
    		//printf("ether: Packet dropped due to too short packet %d %s\n", p->len, net_if->name);
    		stats.link.lenerr++;
    		stats.link.drop++;
		pbuf_free(p);
    		return -1;
  	}

  	if (p != NULL) 
	{
      		ethhdr = p->payload;

      		//if (!eth_addr_isbroadcast(&ethhdr->dest)) printf("ether: recv src=%la dst=%la type=%04X len=%d\n", &ethhdr->src, &ethhdr->dest, htons(ethhdr->type), p->len);
     
      		switch (htons(ethhdr->type)) 
		{
        		case ETHTYPE_IP:
				//printf("ether: IP packet\n");


          			arp_ip_input(net_if, p);

          			pbuf_header(p, -ETHER_HLEN);
				//p->payload = ((char*) p->payload) + ETHER_HLEN;
				
				// call ip layer
          			net_if->input(net_if, p);

				pbuf_free(p);

          			break;

		        case ETHTYPE_ARP:

				//printf("ether: ARP packet\n");
		          	p = arp_arp_input(net_if, &net_if->hwaddr, p);
          			if (p != NULL) 
				{
					// transmit the packet, send arp response
            				net_if->output(net_if, p);
					pbuf_free(p);
          			}

				//pbuf_free(p);
          			break;

        		default:	// un-supported packet type
          			pbuf_free(p);
          			break;

      		}	// end switch

    	}	// end if


  	return 0;
}




//
// ether_output
//
// This function is called by the TCP/IP stack when an IP packet
// should be sent.
//

int ether_output(netif *net_if, pbuf *p, ip_addr *ipaddr) 
{
	pbuf *q;
  	eth_hdr *ethhdr;
   	eth_addr *dest, mcastaddr;
  	ip_addr *queryaddr;
  	int err;
  	int i;
  	int loopback = 0;

  	printf("ether: xmit %d bytes, %d bufs\n", p->tot_len, pbuf_clen(p));

  	//if ((net_if->flags & NETIF_UP) == 0) return -ENETDOWN;

  	if (pbuf_header(p, ETHER_HLEN)) 	
	{
    		printf("ether_output: not enough room for Ethernet header in pbuf\n");
    		stats.link.err++;
    		return -1;
  	}

  	// Construct Ethernet header. Start with looking up deciding which
  	// MAC address to use as a destination address. Broadcasts and
  	// multicasts are special, all other addresses are looked up in the
  	// ARP table.

  	queryaddr = ipaddr;
  	if (ip_addr_isany(ipaddr) || ip_addr_isbroadcast(ipaddr, &net_if->netmask)) 
	{
    		dest = (eth_addr *) &ethbroadcast;
  	}
	else if (ip_addr_ismulticast(ipaddr)) 
	{
    		// Hash IP multicast address to MAC address.
    		mcastaddr.addr[0] = 0x01;
    		mcastaddr.addr[1] = 0x0;
    		mcastaddr.addr[2] = 0x5e;
    		mcastaddr.addr[3] = ip4_addr2(ipaddr) & 0x7f;
    		mcastaddr.addr[4] = ip4_addr3(ipaddr);
    		mcastaddr.addr[5] = ip4_addr4(ipaddr);
    		dest = &mcastaddr;
  	} 
	else if (ip_addr_cmp(ipaddr, &net_if->ip_addr)) 
	{
    		dest = &net_if->hwaddr;
    		loopback = 1;
  	}
	else 
	{
    		if (ip_addr_maskcmp(ipaddr, &net_if->ip_addr, &net_if->netmask)) 
		{
      			// Use destination IP address if the destination is on the same subnet as we are.
      			queryaddr = ipaddr;
    		} 
		else 
		{
      			// Otherwise we use the default router as the address to send the Ethernet frame to.
      			queryaddr = &net_if->gw;
    		}

    		dest = arp_lookup(queryaddr);
  	}

  	// If the arp_lookup() didn't find an address, we send out an ARP query for the IP address.
  	if (dest == NULL) 
	{
    		q = arp_query(net_if, &net_if->hwaddr, queryaddr);
    		if (q != NULL) 
		{
      			err = net_if->output(net_if, p);
      			if (err < 0) 
			{
        			printf("ether: error %d sending arp packet\n", err);
        			pbuf_free(q);
        			stats.link.drop++;
        			return err;
      			}
    		}

    		// Queue packet for transmission, when the ARP reply returns
    		err = arp_queue(net_if, p, queryaddr);
    		if (err < 0) 
		{
      			printf("ether: error %d queueing packet\n", err);
      			stats.link.drop++;
      			stats.link.memerr++;
      			return err;
    		}

    		return 0;
  	}

  	ethhdr = p->payload;

  	for (i = 0; i < 6; i++) 
	{
    		ethhdr->dest.addr[i] = dest->addr[i];
    		ethhdr->src.addr[i] = net_if->hwaddr[i];
  	}
  	ethhdr->type = htons(ETHTYPE_IP);
  
  	if (loopback) 
	{
    		pbuf *q;

    		q = pbuf_dup(PBUF_RW, p);
    		if (!q) return -1;

    		err = ether_input(net_if, q);
    		if (err < 0) 
		{
      			pbuf_free(q);
      			return err;
    		}
  	} 
	else 
	{
    		err = net_if->output(net_if, p);
    		if (err < 0) 
		{
      			printf("ether: error %d sending packet\n", err);
      			return err;
    		}
  	}

  	return 0;
}
