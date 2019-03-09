
#include <stdio.h>
#include <memory.h>
#include <udp.h>
#include <icmp.h>
#include <netif.h>
#include <pbuf.h>
#include <ip.h>
#include <ip_addr.h>
#include <stats.h>


/* The list of UDP PCBs. */
static udp_pcb *udp_pcbs = NULL;

static udp_pcb *pcb_cache = NULL;


void udp_init(void)
{
}

#ifdef LWIP_DEBUG
u8_t udp_lookup(ip_hdr *iphdr, netif *inp)
{
	udp_pcb *pcb;
  	udp_hdr *udphdr;
  	u16_t src, dest;
  
  	//PERF_START;
  
  	udphdr = (udp_hdr *)(u8_t *)iphdr + IPH_HL(iphdr) * 4/sizeof(u8_t);

  	src = NTOHS(udphdr->src);
  	dest = NTOHS(udphdr->dest);
  
  	pcb = pcb_cache;
  	if(pcb != NULL && pcb->remote_port == src && pcb->local_port == dest &&
     		(ip_addr_isany(&pcb->remote_ip) || ip_addr_cmp(&(pcb->remote_ip), &(iphdr->src))) &&
     		(ip_addr_isany(&pcb->local_ip) || ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) 
	{
    		return 1;
  	} 
	else 
	{  
    		for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) 
		{
      			if(pcb->remote_port == src && pcb->local_port == dest &&
	 			(ip_addr_isany(&pcb->remote_ip) || ip_addr_cmp(&(pcb->remote_ip), &(iphdr->src))) &&
	 			(ip_addr_isany(&pcb->local_ip) || ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) 
			{
				pcb_cache = pcb;
				break;
      			}
    		}

    		if(pcb == NULL) 	
		{
      			for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) 
			{
				if(pcb->local_port == dest && (ip_addr_isany(&pcb->remote_ip) || ip_addr_cmp(&(pcb->remote_ip), &(iphdr->src))) && (ip_addr_isany(&pcb->local_ip) || ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) 
				{
	  				break;
				}
      			}
    		}
  	}

  	//PERF_STOP("udp_lookup");
  
  	if(pcb != NULL) 
	{
    		return 1;
  	} 
	else 
	{  
    		return 1;
  	}
}
#endif /* LWIP_DEBUG */

/*-----------------------------------------------------------------------------------*/
void udp_input(pbuf *p, netif *inp)
{
	udp_hdr *udphdr;  
  	udp_pcb *pcb;
  	ip_hdr *iphdr;
  	u16_t src, dest;	// source and dest ports
  
  	//PERF_START;
  
#ifdef UDP_STATS
  ++stats.udp.recv;
#endif /* UDP_STATS */

  	iphdr = p->payload;

  	pbuf_header(p, -(UDP_HLEN + IPH_HL(iphdr) * 4));

  	udphdr = (udp_hdr *)((u8_t *)p->payload - UDP_HLEN);
  
  	printf("udp_input: received datagram of length %d\n", p->tot_len);
	
  	src = NTOHS(udphdr->src);
  	dest = NTOHS(udphdr->dest);

#if UDP_DEBUG
  udp_debug_print(udphdr);
#endif /* UDP_DEBUG */
  
  	/* Demultiplex packet. First, go for a perfect match. */
  	for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) 
	{
    		printf("udp_input: pcb local port %d (dgram %d)\n", pcb->local_port, ntohs(udphdr->dest));
    		if(pcb->remote_port == src &&
       			pcb->local_port == dest &&
       				(ip_addr_isany(&pcb->remote_ip) || ip_addr_cmp(&(pcb->remote_ip), &(iphdr->src))) &&
       				(ip_addr_isany(&pcb->local_ip) || ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) 
		{
      			break;
    		}
  	}


  	if(pcb == NULL) 
	{
    		for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) 
		{
      			printf("udp_input: pcb local port %d (dgram %d)\n", pcb->local_port, dest);
      			if(pcb->local_port == dest &&
	 			(ip_addr_isany(&pcb->remote_ip) || ip_addr_cmp(&(pcb->remote_ip), &(iphdr->src))) &&
	 			(ip_addr_isany(&pcb->local_ip) || ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)))) 
			{
				break;
      			}      
    		}
  	}


  	/* Check checksum if this is a match or if it was directed at us. */
  	/*  if(pcb != NULL ||
      	ip_addr_cmp(&inp->ip_addr, &iphdr->dest)) {*/
  
	if(pcb != NULL) 
	{
    		printf("udp_input: calculating checksum\n");
    		pbuf_header(p, UDP_HLEN);    
#ifdef IPv6
    if(iphdr->nexthdr == IP_PROTO_UDPLITE) {    
#else
    		if(IPH_PROTO(iphdr) == IP_PROTO_UDPLITE) 
		{    
#endif /* IPv4 */
      
			/* Do the UDP Lite checksum */
      			if(inet_chksum_pseudo(p, (ip_addr *)&(iphdr->src),
			    (ip_addr *)&(iphdr->dest), IP_PROTO_UDPLITE, ntohs(udphdr->len)) != 0) 
			{
				printf("udp_input: UDP Lite datagram discarded due to failing checksum\n");
#ifdef UDP_STATS
	++stats.udp.chkerr;
	++stats.udp.drop;
#endif /* UDP_STATS */

				pbuf_free(p);
				goto end;
      			}
    		} 
		else 
		{
      			if(udphdr->chksum != 0) 
			{
				if(inet_chksum_pseudo(p, (ip_addr *)&(iphdr->src), (ip_addr *)&(iphdr->dest),
			      		IP_PROTO_UDP, p->tot_len) != 0) 
				{
	  				printf("udp_input: UDP datagram discarded due to failing checksum\n");
	  
#ifdef UDP_STATS
	  ++stats.udp.chkerr;
	  ++stats.udp.drop;
#endif /* UDP_STATS */

	  				pbuf_free(p);
	  				goto end;
				}
      			}
    		}

    		pbuf_header(p, -UDP_HLEN);    
    		if(pcb != NULL) 
		{
      			pcb->recv(pcb->recv_arg, pcb, p, &(iphdr->src), src);
    		} 
		else 
		{
      			printf("udp_input: not for us.\n");
      
      			/* No match was found, send ICMP destination port unreachable unless
	 		destination address was broadcast/multicast. */
      
      			if(!ip_addr_isbroadcast(&iphdr->dest, &inp->netmask) &&
	 			!ip_addr_ismulticast(&iphdr->dest)) 
			{
	
				/* deconvert from host to network byte order */
				udphdr->src = htons(udphdr->src);
				udphdr->dest = htons(udphdr->dest); 
	
				/* adjust pbuf pointer */
				p->payload = iphdr;
				icmp_dest_unreach(p, ICMP_DUR_PORT);
      			}
#ifdef UDP_STATS
      ++stats.udp.proterr;
      ++stats.udp.drop;
#endif /* UDP_STATS */

      			pbuf_free(p);
    		}
  
	} 
	else 
	{
    		pbuf_free(p);
  	}
  
end:
    
  //PERF_STOP("udp_input");
	return;
}

/*-----------------------------------------------------------------------------------*/
udp_pcb* udp_new(void) 
{
	udp_pcb *pcb;
  	//pcb = malloc(MEMP_UDP_PCB);
	pcb = malloc(sizeof(udp_pcb));
  	if(pcb != NULL) 
	{
    		memset(pcb, 0, sizeof(udp_pcb));
    		return pcb;
  	}
  	return NULL;
}
/*-----------------------------------------------------------------------------------*/
void udp_remove(udp_pcb *pcb)
{
	udp_pcb *pcb2;
  
  	if(udp_pcbs == pcb) 
	{
    		udp_pcbs = udp_pcbs->next;
  	} 
	else 
		for(pcb2 = udp_pcbs; pcb2 != NULL; pcb2 = pcb2->next) 
		{
    			if(pcb2->next != NULL && pcb2->next == pcb) 
			{
      				pcb2->next = pcb->next;
				// break;
    			}
  		}

  	free(pcb);  
}
/*-----------------------------------------------------------------------------------*/
int udp_bind(udp_pcb *pcb, ip_addr *ipaddr, u16_t port)
{
  	udp_pcb *ipcb;
  	ip_addr_set(&pcb->local_ip, ipaddr);
  	pcb->local_port = port;

  	/* Insert UDP PCB into the list of active UDP PCBs. */
  	for(ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) 
	{
    		if(pcb == ipcb) 
		{
      			/* Already on the list, just return. */
      			return 0;
    		}
  	}
  
	/* We need to place the PCB on the list. */
  	pcb->next = udp_pcbs;
  	udp_pcbs = pcb;

  	printf("udp_bind: bound to port %d\n", port);
  	return 0;
}
/*-----------------------------------------------------------------------------------*/
int udp_connect(udp_pcb *pcb, ip_addr *ipaddr, u16_t port)
{
  	udp_pcb *ipcb;
  	ip_addr_set(&pcb->remote_ip, ipaddr);
 	pcb->remote_port = port;

  	/* Insert UDP PCB into the list of active UDP PCBs. */
  	for(ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) 
	{
    		if(pcb == ipcb) 
		{
      			/* Already on the list, just return. */
      			return 0;
    		}
  	}
  
	/* We need to place the PCB on the list. */
  	pcb->next = udp_pcbs;
  	udp_pcbs = pcb;
  	return 0;
}
/*-----------------------------------------------------------------------------------*/
void udp_recv(udp_pcb *pcb, void (* recv)(void *arg, udp_pcb *upcb, pbuf *p,
		        ip_addr *addr, u16_t port), void *recv_arg)
{
  	pcb->recv = recv;
  	pcb->recv_arg = recv_arg;
}
/*-----------------------------------------------------------------------------------*/
int udp_send(udp_pcb *pcb, pbuf *p)
{
	udp_hdr *udphdr;
  	netif *net_if;
  	ip_addr *src_ip;
  	int err;
  	pbuf *q;
  
  	if(pbuf_header(p, UDP_HLEN)) 
	{
    		q = pbuf_alloc(PBUF_IP, UDP_HLEN, PBUF_RW);
    		if(q == NULL) 
		{
      			return -1;
    		}
    		pbuf_chain(q, p);
    		p = q;
  	}

  	udphdr = p->payload;
  	udphdr->src = htons(pcb->local_port);
  	udphdr->dest = htons(pcb->remote_port);
  	udphdr->chksum = 0x0000;

  	if((net_if = ip_route(&(pcb->remote_ip))) == NULL) 
	{
    		printf("udp_send: No route to 0x%lx\n", pcb->remote_ip.addr);
#ifdef UDP_STATS
    ++stats.udp.rterr;
#endif /* UDP_STATS */
	
    		return -1;
  	}

  	if(ip_addr_isany(&pcb->local_ip)) 
	{
    		src_ip = &(net_if->ip_addr);
  	} 
	else 
	{
    		src_ip = &(pcb->local_ip);
  	}
  
 	printf("udp_send: sending datagram of length %d\n", p->tot_len);
  
  	if(pcb->flags & UDP_FLAGS_UDPLITE) 
	{
    		udphdr->len = htons(pcb->chksum_len);
    		
		/* calculate checksum */
    		udphdr->chksum = inet_chksum_pseudo(p, src_ip, &(pcb->remote_ip), IP_PROTO_UDP, pcb->chksum_len);
    		
		if(udphdr->chksum == 0x0000) 
		{
      			udphdr->chksum = 0xffff;
    		}
    		err = ip_output_if(p, src_ip, &pcb->remote_ip, UDP_TTL, IP_PROTO_UDPLITE, net_if);    
  	} 
	else 
	{
    		udphdr->len = htons(p->tot_len);
    
		/* calculate checksum */
    		if((pcb->flags & UDP_FLAGS_NOCHKSUM) == 0) 
		{
      			udphdr->chksum = inet_chksum_pseudo(p, src_ip, &pcb->remote_ip, IP_PROTO_UDP, p->tot_len);
      	
			if(udphdr->chksum == 0x0000) 
			{
				udphdr->chksum = 0xffff;
      			}
    		}
    
		err = ip_output_if(p, src_ip, &pcb->remote_ip, UDP_TTL, IP_PROTO_UDP, net_if);    
  	}
  
#ifdef UDP_STATS
  ++stats.udp.xmit;
#endif /* UDP_STATS */
 
	return err;
}
