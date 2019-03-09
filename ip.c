

#include <stdio.h>
#include <string.h>
#include <ip.h>
#include <stats.h>
#include <debug.h>
#include <pbuf.h>
#include <icmp.h>

extern netstats stats;

extern netif* netif_list;

ip_addr ip_addr_broadcast = {0xffffffff};



#define PART1(ip)	(ip>>24)
#define PART2(ip)	((ip & 0x00FF0000) >>16 )
#define PART3(ip)	((ip & 0x0000FF00) >>8 )
#define PART4(ip)	(ip & 0x000000FF)

err_t ip_input(netif *inp, pbuf *p) 
{

	//printf("Inside ip_input ...\n");

	static ip_hdr *iphdr;
  	static netif *net_if;
  	static u8_t hl;

	++stats.ip.recv;

  	/* identify the IP header */
  	iphdr = p->payload;
//dump_hexa(p->payload, 34);
//while(1);
  	if(IPH_V(iphdr) != 4) 
	{
    		printf("IP packet dropped due to bad version number %d\n", IPH_V(iphdr));
    		pbuf_free(p);

    		++stats.ip.err;
    		++stats.ip.drop;

    		return -1;
  	}
  
  	hl = IPH_HL(iphdr);
  	if(hl * 4 > p->len) 
	{
    		printf("IP packet dropped due to too short packet length %d\n", p->len);
		pbuf_free(p);

    		++stats.ip.lenerr;
    		++stats.ip.drop;

		return -1;
  	}


  	/* verify checksum */
  	if(inet_chksum(iphdr, hl * 4) != 0) 
	{

    		printf("IP packet dropped due to failing checksum 0x%x\n", inet_chksum(iphdr, hl * 4));
		pbuf_free(p);

    		++stats.ip.chkerr;
    		++stats.ip.drop;

    		return -1;
  	}

	/* Trim pbuf. This should have been done at the netif layer,
     	but we'll do it anyway just to be sure that its done. */
  	pbuf_realloc(p, ntohs(IPH_LEN(iphdr)));


  	/* is this packet for us? */
  	for(net_if = netif_list; net_if != NULL; net_if = net_if->next) 
	{

		//printf("local IP : %d.%d.%d.%d \n", PART4(net_if->ip_addr.addr), PART3(net_if->ip_addr.addr), PART2(net_if->ip_addr.addr), PART1(net_if->ip_addr.addr) );

		printf("%d.%d.%d.%d \t", PART4(iphdr->src.addr), PART3(iphdr->src.addr), PART2(iphdr->src.addr), PART1(iphdr->src.addr) );

		printf("%d.%d.%d.%d \n", PART4(iphdr->dest.addr), PART3(iphdr->dest.addr), PART2(iphdr->dest.addr), PART1(iphdr->dest.addr) );

    		if(ip_addr_isany(&(net_if->ip_addr)) ||
       			ip_addr_cmp(&(iphdr->dest), &(net_if->ip_addr)) ||
       			(ip_addr_isbroadcast(&(iphdr->dest), &(net_if->netmask)) &&
			ip_addr_maskcmp(&(iphdr->dest), &(net_if->ip_addr), &(net_if->netmask))) ||
       			ip_addr_cmp(&(iphdr->dest), IP_ADDR_BROADCAST)) 
		{
      			break;
    		}

  	}


#if LWIP_DHCP
  	/* If a DHCP packet has arrived on the interface, we pass it up the
     	stack regardless of destination IP address. The reason is that
     	DHCP replies are sent to the IP adress that will be given to this
     	node (as recommended by RFC 1542 section 3.1.1, referred by RFC
     	2131). */
  	if(IPH_PROTO(iphdr) == IP_PROTO_UDP && ((struct udp_hdr *)((u8_t *)iphdr + IPH_HL(iphdr) * 4/sizeof(u8_t)))->src == DHCP_SERVER_PORT) 
	{
    		net_if = inp;
  	}  
#endif /* LWIP_DHCP */


  	if(net_if == NULL) 
	{
    		/* packet not for us, route or discard */
    		printf("ip_input: packet not for us.\n");
		
#if IP_FORWARD
    		if(!ip_addr_isbroadcast(&(iphdr->dest), &(inp->netmask))) 
		{
      			ip_forward(p, iphdr, inp);
    		}
#endif /* IP_FORWARD */
    	
		pbuf_free(p);
    		return -1;
  	}



#if IP_REASSEMBLY
  	if((IPH_OFFSET(iphdr) & htons(IP_OFFMASK | IP_MF)) != 0) 
	{
    		p = ip_reass(p);
    		if(p == NULL) 
		{
      			return -1;
    		}
    		iphdr = p->payload;
  	}
#else /* IP_REASSEMBLY */
  	if((IPH_OFFSET(iphdr) & htons(IP_OFFMASK | IP_MF)) != 0) 
	{
    		pbuf_free(p);
    		printf("IP packet dropped since it was fragmented (0x%x).\n",ntohs(IPH_OFFSET(iphdr)));

#ifdef IP_STATS
    ++stats.ip.opterr;
    ++stats.ip.drop;
#endif /* IP_STATS */
    
		return -1;
  	}
#endif /* IP_REASSEMBLY */


#if IP_OPTIONS == 0
  	if(hl * 4 > IP_HLEN) 
	{
    		printf("IP packet dropped since there were IP options.\n");

    		pbuf_free(p);    

#ifdef IP_STATS
    ++stats.ip.opterr;
    ++stats.ip.drop;
#endif /* IP_STATS */

    		return -1;
  }  
#endif /* IP_OPTIONS == 0 */


  /* send to upper layers */
#if IP_DEBUG
  DEBUGF(IP_DEBUG, ("ip_input: \n"));
  ip_debug_print(p);
  DEBUGF(IP_DEBUG, ("ip_input: p->len %d p->tot_len %d\n", p->len, p->tot_len));
#endif /* IP_DEBUG */  



  	switch(IPH_PROTO(iphdr)) 
	{
//#if LWIP_UDP > 0    
  		case IP_PROTO_UDP:
		case IP_PROTO_UDPLITE:
			printf("receive UDP ...\n");
    			udp_input(p, inp);
    		break;
//#endif /* LWIP_UDP */

//#if LWIP_TCP > 0    
  		case IP_PROTO_TCP:
			printf("receive TCP ...\n");
    			//tcp_input(p, inp);
    		break;
//#endif /* LWIP_TCP */

  		case IP_PROTO_ICMP:
			printf("receive ICMP ...\n");
    			icmp_input(p, inp);
    		break;
  	
		default:
    		/* send ICMP destination protocol unreachable unless is was a broadcast */
    		if(!ip_addr_isbroadcast(&(iphdr->dest), &(inp->netmask)) &&
       	!ip_addr_ismulticast(&(iphdr->dest))) 
		{
      			p->payload = iphdr;
      			icmp_dest_unreach(p, ICMP_DUR_PROTO);
    		}
    		pbuf_free(p);

    		printf("Unsupported transportation protocol %d\n", IPH_PROTO(iphdr));

#ifdef IP_STATS
    ++stats.ip.proterr;
    ++stats.ip.drop;
#endif /* IP_STATS */

  	}	// end switch stmt




	static int num = 0;
	static total = 0;
	total+= p->tot_len;
	num++;
	//printf("valid IP packet received version %d count = %d header len = %d\n", IPH_V(iphdr), num, hl * 4);

	return 0;
}


#if IP_FORWARD
/*-----------------------------------------------------------------------------------*/
/* ip_forward:
 *
 * Forwards an IP packet. It finds an appropriate route for the
 * packet, decrements the TTL value of the packet, adjusts the
 * checksum and outputs the packet on the appropriate interface.
 */
/*-----------------------------------------------------------------------------------*/
static void ip_forward(struct pbuf *p, struct ip_hdr *iphdr, struct netif *inp)
{
	static netif *net_if;
  
  	//PERF_START;
  	if((net_if = ip_route((ip_addr *)&(iphdr->dest))) == NULL) 
	{

    		//printf("ip_forward: no forwarding route for 0x%lx found\n", iphdr->dest.addr));

    		return;
  	}

  	/* Don't forward packets onto the same network interface on which
     	they arrived. */
  	if(net_if == inp) 
	{
    		printf("ip_forward: not forward packets back on incoming interface.\n");

    		return;
  	}
  
  	/* Decrement TTL and send ICMP if ttl == 0. */
  	IPH_TTL_SET(iphdr, IPH_TTL(iphdr) - 1);
  	if(IPH_TTL(iphdr) == 0) 
	{
    		/* Don't send ICMP messages in response to ICMP messages */
    		if(IPH_PROTO(iphdr) != IP_PROTO_ICMP) 
		{
      			icmp_time_exceeded(p, ICMP_TE_TTL);
    		}
    		return;       
  	}
  
  	/* Incremental update of the IP checksum. */
  	if(IPH_CHKSUM(iphdr) >= htons(0xffff - 0x100)) 
	{
    		IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + htons(0x100) + 1);
  	} 
	else 
	{
    		IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + htons(0x100));
  	}

  	//printf("ip_forward: forwarding packet to 0x%lx\n", iphdr->dest.addr);

#ifdef IP_STATS
  ++stats.ip.fw;
  ++stats.ip.xmit;
#endif /* IP_STATS */

  	//PERF_STOP("ip_forward");
  
  	net_if->output(netif, p, (ip_addr *)&(iphdr->dest));
}
#endif /* IP_FORWARD */

/*-----------------------------------------------------------------------------------*/
/* ip_route:
 *
 * Finds the appropriate network interface for a given IP address. It
 * searches the list of network interfaces linearly. A match is found
 * if the masked IP address of the network interface equals the masked
 * IP address given to the function.
 */
/*-----------------------------------------------------------------------------------*/
netif *ip_route(ip_addr *dest)
{
	netif *net_if;
  
  	for(net_if = netif_list; net_if != NULL; net_if = net_if->next) 
	{
    		if(ip_addr_maskcmp(dest, &(net_if->ip_addr), &(net_if->netmask))) 
		{
      			return net_if;
    		}
  	}

  	return netif_default;
}

/*-----------------------------------------------------------------------------------*/
/* ip_reass:
 *
 * Tries to reassemble a fragmented IP packet.
 */
/*-----------------------------------------------------------------------------------*/
#define IP_REASSEMBLY 		0
#define IP_REASS_BUFSIZE 	5760
#define IP_REASS_MAXAGE 	10

#if IP_REASSEMBLY
static u8_t ip_reassbuf[IP_HLEN + IP_REASS_BUFSIZE];
static u8_t ip_reassbitmap[IP_REASS_BUFSIZE / (8 * 8)];
static const u8_t bitmap_bits[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
static u16_t ip_reasslen;
static u8_t ip_reassflags;
#define IP_REASS_FLAG_LASTFRAG 	0x01
static u8_t ip_reasstmr;

static pbuf * ip_reass(pbuf *p)
{
  	pbuf *q;
  	ip_hdr *fraghdr, *iphdr;
  	u16_t offset, len;
  	u16_t i;
  
  	iphdr = (ip_hdr *)ip_reassbuf;
  	fraghdr = (ip_hdr *)p->payload;

  	/* If ip_reasstmr is zero, no packet is present in the buffer, so we
     	write the IP header of the fragment into the reassembly
     	buffer. The timer is updated with the maximum age. */
  	if(ip_reasstmr == 0) 
	{
    		DEBUGF(IP_REASS_DEBUG, ("ip_reass: new packet\n"));
    		bcopy(fraghdr, iphdr, IP_HLEN);
    		ip_reasstmr = IP_REASS_MAXAGE;
    		ip_reassflags = 0;
    	
		/* Clear the bitmap. */
    		bzero(ip_reassbitmap, sizeof(ip_reassbitmap));
  	}

  	/* Check if the incoming fragment matches the one currently present
     	in the reasembly buffer. If so, we proceed with copying the
     	fragment into the buffer. */
  	if(ip_addr_cmp(&iphdr->src, &fraghdr->src) &&
     		ip_addr_cmp(&iphdr->dest, &fraghdr->dest) &&
     		IPH_ID(iphdr) == IPH_ID(fraghdr)) 
	{
    		DEBUGF(IP_REASS_DEBUG, ("ip_reass: matching old packet\n"));
    		
		/* Find out the offset in the reassembly buffer where we should
       		copy the fragment. */
    		len = ntohs(IPH_LEN(fraghdr)) - IPH_HL(fraghdr) * 4;
    		offset = (ntohs(IPH_OFFSET(fraghdr)) & IP_OFFMASK) * 8;

    		/* If the offset or the offset + fragment length overflows the
       		reassembly buffer, we discard the entire packet. */
    		if(offset > IP_REASS_BUFSIZE ||
       			offset + len > IP_REASS_BUFSIZE) 
		{
      			DEBUGF(IP_REASS_DEBUG, ("ip_reass: fragment outside of buffer (%d:%d/%d).\n",
			      offset, offset + len, IP_REASS_BUFSIZE));
      			ip_reasstmr = 0;
      			goto nullreturn;
    		}

    		/* Copy the fragment into the reassembly buffer, at the right
       		offset. */
    		DEBUGF(IP_REASS_DEBUG, ("ip_reass: copying with offset %d into %d:%d\n",
			    offset, IP_HLEN + offset, IP_HLEN + offset + len));
    		bcopy((u8_t *)fraghdr + IPH_HL(fraghdr) * 4,
	  		&ip_reassbuf[IP_HLEN + offset], len);

    		/* Update the bitmap. */
    		if(offset / (8 * 8) == (offset + len) / (8 * 8)) 
		{
      			DEBUGF(IP_REASS_DEBUG, ("ip_reass: updating single byte in bitmap.\n"));
      			/* If the two endpoints are in the same byte, we only update
	 		that byte. */
      			ip_reassbitmap[offset / (8 * 8)] |=
			bitmap_bits[(offset / 8 ) & 7] &
			~bitmap_bits[((offset + len) / 8 ) & 7];
    		} 
		else 
		{
      			/* If the two endpoints are in different bytes, we update the
	 		bytes in the endpoints and fill the stuff inbetween with 0xff. */
      			ip_reassbitmap[offset / (8 * 8)] |= bitmap_bits[(offset / 8 ) & 7];
      			DEBUGF(IP_REASS_DEBUG, ("ip_reass: updating many bytes in bitmap (%d:%d).\n",
			      1 + offset / (8 * 8), (offset + len) / (8 * 8)));
      
			for(i = 1 + offset / (8 * 8); i < (offset + len) / (8 * 8); ++i) 
			{
				ip_reassbitmap[i] = 0xff;
      			}      
      			ip_reassbitmap[(offset + len) / (8 * 8)] |= ~bitmap_bits[((offset + len) / 8 ) & 7];
    		}
    
    		/* If this fragment has the More Fragments flag set to zero, we
       		know that this is the last fragment, so we can calculate the
       		size of the entire packet. We also set the
       		IP_REASS_FLAG_LASTFRAG flag to indicate that we have received
       		the final fragment. */
    		if((ntohs(IPH_OFFSET(fraghdr)) & IP_MF) == 0) 
		{
      			ip_reassflags |= IP_REASS_FLAG_LASTFRAG;
      			ip_reasslen = offset + len;
      			DEBUGF(IP_REASS_DEBUG, ("ip_reass: last fragment seen, total len %d\n", ip_reasslen));
    		}
    
    		/* Finally, we check if we have a full packet in the buffer. We do
       		this by checking if we have the last fragment and if all bits
       		in the bitmap are set. */
    		if(ip_reassflags & IP_REASS_FLAG_LASTFRAG) 
		{
      			/* Check all bytes up to and including all but the last byte in
	 		the bitmap. */
      			for(i = 0; i < ip_reasslen / (8 * 8) - 1; ++i) 
			{
				if(ip_reassbitmap[i] != 0xff) 
				{
	  				DEBUGF(IP_REASS_DEBUG, ("ip_reass: last fragment seen, bitmap %d/%d failed (%x)\n", i, ip_reasslen / (8 * 8) - 1, ip_reassbitmap[i]));
	  			
				goto nullreturn;
			}
      		}

      		/* Check the last byte in the bitmap. It should contain just the
	 	right amount of bits. */
      		if(ip_reassbitmap[ip_reasslen / (8 * 8)] !=
	 	(u8_t)~bitmap_bits[ip_reasslen / 8 & 7]) {
		DEBUGF(IP_REASS_DEBUG, ("ip_reass: last fragment seen, bitmap %d didn't contain %x (%x)\n",
				ip_reasslen / (8 * 8), ~bitmap_bits[ip_reasslen / 8 & 7],
				ip_reassbitmap[ip_reasslen / (8 * 8)]));
		goto nullreturn;
      	}

      	/* Pretend to be a "normal" (i.e., not fragmented) IP packet
	 from now on. */
      	IPH_OFFSET_SET(iphdr, 0);
      	IPH_CHKSUM_SET(iphdr, 0);
      	IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));
      
      	/* If we have come this far, we have a full packet in the
	 buffer, so we allocate a pbuf and copy the packet into it. We
	 also reset the timer. */
      	ip_reasstmr = 0;
      	pbuf_free(p);
      	p = pbuf_alloc(PBUF_LINK, ip_reasslen, PBUF_POOL);
      	if(p != NULL) 
	{
		i = 0;
		for(q = p; q != NULL; q = q->next) 
		{
	  		/* Copy enough bytes to fill this pbuf in the chain. The
	     		avaliable data in the pbuf is given by the q->len
	     		variable. */
	  		DEBUGF(IP_REASS_DEBUG, ("ip_reass: bcopy from %p (%d) to %p, %d bytes\n",
				  &ip_reassbuf[i], i, q->payload, q->len > ip_reasslen - i? ip_reasslen - i: q->len));
	  		bcopy(&ip_reassbuf[i], q->payload,
				q->len > ip_reasslen - i? ip_reasslen - i: q->len);
	  		i += q->len;
		}
      }
      DEBUGF(IP_REASS_DEBUG, ("ip_reass: p %p\n", p));
      return p;
    }
  }

 	nullreturn:
  	pbuf_free(p);
  	return NULL;
}
#endif /* IP_REASSEMBLY */





/*-----------------------------------------------------------------------------------*/
/* ip_output_if:
 *
 * Sends an IP packet on a network interface. This function constructs
 * the IP header and calculates the IP header checksum. If the source
 * IP address is NULL, the IP address of the outgoing network
 * interface is filled in as source address.
 */
/*-----------------------------------------------------------------------------------*/
int ip_output_if(pbuf *p, ip_addr *src, ip_addr *dest, u8_t ttl, u8_t proto, netif *net_if)
{
	static ip_hdr *iphdr;
  	static u16_t ip_id = 0;

  
  
  	if(dest != IP_HDRINCL) 
	{
    		if(pbuf_header(p, IP_HLEN)) 
		{
      			printf("ip_output: not enough room for IP header in pbuf\n");
      
#ifdef IP_STATS
      ++stats.ip.err;
#endif /* IP_STATS */
      			pbuf_free(p);
      			return -1;
    		}
    
    		iphdr = p->payload;
    
    		IPH_TTL_SET(iphdr, ttl);
    		IPH_PROTO_SET(iphdr, proto);
    
    		ip_addr_set(&(iphdr->dest), dest);

    		IPH_VHLTOS_SET(iphdr, 4, IP_HLEN / 4, 0);
    		IPH_LEN_SET(iphdr, htons(p->tot_len));
    		IPH_OFFSET_SET(iphdr, htons(IP_DF));
    		IPH_ID_SET(iphdr, htons(++ip_id));

    		if(ip_addr_isany(src)) 
		{
      			ip_addr_set(&(iphdr->src), &(net_if->ip_addr));
    		} 
		else 
		{
      			ip_addr_set(&(iphdr->src), src);
    		}

    		IPH_CHKSUM_SET(iphdr, 0);
    		IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));
  	} 
	else 
	{
    		iphdr = p->payload;
    		dest = &(iphdr->dest);
  	}

#ifdef IP_STATS
  stats.ip.xmit++;
#endif /* IP_STATS */

  	printf("ip_output_if: %s ", net_if->name);

#if IP_DEBUG
  ip_debug_print(p);
#endif /* IP_DEBUG */

  	//return net_if->output(net_if, p);  
	return ether_output(net_if, p, dest);
}


/*-----------------------------------------------------------------------------------*/
/* ip_output:
 *
 * Simple interface to ip_output_if. It finds the outgoing network
 * interface and calls upon ip_output_if to do the actual work.
 */
/*-----------------------------------------------------------------------------------*/
int ip_output(pbuf *p, ip_addr *src, ip_addr *dest, u8_t ttl, u8_t proto)
{
	static netif *net_if;

  
  	if((net_if = ip_route(dest)) == NULL) 
	{
//    		printf("ip_output: No route to 0x%lx\n", dest->addr);

#ifdef IP_STATS
    ++stats.ip.rterr;
#endif /* IP_STATS */
    
		pbuf_free(p);
    		return -1;
  	}

  	return ip_output_if(p, src, dest, ttl, proto, net_if);
}

