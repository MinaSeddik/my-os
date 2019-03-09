
#include <stdio.h>
#include <icmp.h>
#include <pbuf.h>
#include <ip.h>
#include <ip_addr.h>
#include <stats.h>


void icmp_input(pbuf *p, netif *inp)
{
  	unsigned char type;
  	icmp_echo_hdr *iecho;
  	ip_hdr *iphdr;
  	ip_addr tmpaddr;
  	u16_t hlen;
  

#ifdef ICMP_STATS
  ++stats.icmp.recv;
#endif /* ICMP_STATS */

  
  	iphdr = p->payload;
  	hlen = IPH_HL(iphdr) * 4/sizeof(u8_t);
  	pbuf_header(p, -hlen);

  	type = *((u8_t *)p->payload);

  	switch(type) 
	{
  		case ICMP_ECHO:
    		if(ip_addr_isbroadcast(&iphdr->dest, &inp->netmask) ||
       	ip_addr_ismulticast(&iphdr->dest)) 
		{
      			printf("Smurf.\n");
#ifdef ICMP_STATS
      ++stats.icmp.err;
#endif /* ICMP_STATS */

		      	pbuf_free(p);
      			return;
    		}
    		printf("icmp_input: ping\n");
		printf("Pong!\n");
    		if(p->tot_len < sizeof(icmp_echo_hdr)) 
		{
      			printf("icmp_input: bad ICMP echo received\n");
      			pbuf_free(p);
#ifdef ICMP_STATS
      ++stats.icmp.lenerr;
#endif /* ICMP_STATS */

      			return;      
    		}
    		iecho = p->payload;    
    		if(inet_chksum_pbuf(p) != 0) 
		{
      			printf("icmp_input: checksum failed for received ICMP echo\n");
      			pbuf_free(p);
#ifdef ICMP_STATS
      ++stats.icmp.chkerr;
#endif /* ICMP_STATS */

      			return;
    		}
    
		tmpaddr.addr = iphdr->src.addr;
    		iphdr->src.addr = iphdr->dest.addr;
    		iphdr->dest.addr = tmpaddr.addr;
    		ICMPH_TYPE_SET(iecho, ICMP_ER);
    
		/* adjust the checksum */
    		if(iecho->chksum >= htons(0xffff - (ICMP_ECHO << 8))) 
		{
      			iecho->chksum += htons(ICMP_ECHO << 8) + 1;
    		} 
		else 
		{
      			iecho->chksum += htons(ICMP_ECHO << 8);
    		}

#ifdef ICMP_STATS
    ++stats.icmp.xmit;
#endif /* ICMP_STATS */

    		pbuf_header(p, hlen);
    		ip_output_if(p, &(iphdr->src), IP_HDRINCL, IPH_TTL(iphdr), IP_PROTO_ICMP, inp);
    		break; 
  
		default:
    			printf("icmp_input: ICMP type not supported.\n");
#ifdef ICMP_STATS
    ++stats.icmp.proterr;
    ++stats.icmp.drop;
#endif /* ICMP_STATS */
  	
	}  // end switch

  	pbuf_free(p);

}

/*-----------------------------------------------------------------------------------*/
void icmp_dest_unreach(pbuf *p, enum icmp_dur_type t)
{
	pbuf *q;
	ip_hdr *iphdr;
	icmp_dur_hdr *idur;
  
  	q = pbuf_alloc(PBUF_TRANSPORT, 8 + IP_HLEN + 8, PBUF_RW);
  	/* ICMP header + IP header + 8 bytes of data */

  	iphdr = p->payload;
  
  	idur = q->payload;
  	ICMPH_TYPE_SET(idur, ICMP_DUR);
  	ICMPH_CODE_SET(idur, t);

  	memcpy(p->payload, (char *)q->payload + 8, IP_HLEN + 8);
  
  	/* calculate checksum */
  	idur->chksum = 0;
  	idur->chksum = inet_chksum(idur, q->len);

#ifdef ICMP_STATS
  ++stats.icmp.xmit;
#endif /* ICMP_STATS */

  	ip_output(q, NULL, &(iphdr->src), ICMP_TTL, IP_PROTO_ICMP);
  	pbuf_free(q);
}


/*-----------------------------------------------------------------------------------*/
void icmp_time_exceeded(pbuf *p, enum icmp_te_type t)
{
	pbuf *q;
  	ip_hdr *iphdr;
  	icmp_te_hdr *tehdr;

  	q = pbuf_alloc(PBUF_TRANSPORT, 8 + IP_HLEN + 8, PBUF_RW);

  	iphdr = p->payload;

#if ICMP_DEBUG
  DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded from "));
  ip_addr_debug_print(&(iphdr->src));
  DEBUGF(ICMP_DEBUG, (" to "));
  ip_addr_debug_print(&(iphdr->dest));
  DEBUGF(ICMP_DEBUG, ("\n"));
#endif /* ICMP_DEBNUG */

  	tehdr = q->payload;
  	ICMPH_TYPE_SET(tehdr, ICMP_TE);
  	ICMPH_CODE_SET(tehdr, t);

  	/* copy fields from original packet */
  	memcpy((char *)p->payload, (char *)q->payload + 8, IP_HLEN + 8);
  
  	/* calculate checksum */
  	tehdr->chksum = 0;
  	tehdr->chksum = inet_chksum(tehdr, q->len);

#ifdef ICMP_STATS
  ++stats.icmp.xmit;
#endif /* ICMP_STATS */

  	ip_output(q, NULL, &(iphdr->src), ICMP_TTL, IP_PROTO_ICMP);
  	pbuf_free(q);
}



