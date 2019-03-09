
#include <stdio.h>
#include <memory.h>
#include <tcp.h>
#include <icmp.h>
#include <netif.h>
#include <pbuf.h>
#include <ip.h>
#include <ip_addr.h>
#include <err.h>
#include <stats.h>


int tcp_enqueue(tcp_pcb *pcb, void *arg, u16_t len,u8_t flags, u8_t copy, u8_t *optdata, u8_t optlen);

/* Forward declarations.*/
static void tcp_output_segment(tcp_seg *seg, tcp_pcb *pcb);

extern u32_t tcp_ticks;

/*-----------------------------------------------------------------------------------*/
int tcp_send_ctrl(tcp_pcb *pcb, u8_t flags)
{
  	return tcp_enqueue(pcb, NULL, 0, flags, 1, NULL, 0);

}
/*-----------------------------------------------------------------------------------*/
int tcp_write(tcp_pcb *pcb, const void *arg, u16_t len, u8_t copy)
{
  	if(pcb->state == SYN_SENT ||
     		pcb->state == SYN_RCVD ||
     		pcb->state == ESTABLISHED ||
     		pcb->state == CLOSE_WAIT) 
	{
    		if(len > 0) 
		{
      			return tcp_enqueue(pcb, (void *)arg, len, 0, copy, NULL, 0);
    		}
    		return ERR_OK;
  	} 
	else 
	{
    		return ERR_CONN;
  	}
}
/*-----------------------------------------------------------------------------------*/
int tcp_enqueue(tcp_pcb *pcb, void *arg, u16_t len,u8_t flags, u8_t copy, u8_t *optdata, u8_t optlen)
{
  	pbuf *p;
  	tcp_seg *seg, *useg, *queue;
  	u32_t left, seqno;
  	u16_t seglen;
  	void *ptr;
  	u8_t queuelen;

  	left = len;
  	ptr = arg;
  
  	if(len > pcb->snd_buf) 
	{
    		printf("tcp_enqueue: too much data %d\n", len);
    		return ERR_MEM;
  	}
  
  	seqno = pcb->snd_lbb;
  
  	queue = NULL;
  	printf("tcp_enqueue: %d\n", pcb->snd_queuelen);
  	queuelen = pcb->snd_queuelen;
  	if(queuelen >= TCP_SND_QUEUELEN) 
	{
    		printf("tcp_enqueue: too long queue %d (max %d)\n", queuelen, TCP_SND_QUEUELEN);
    		goto memerr;
  	}   
  
  
  	seg = NULL;
  	seglen = 0;
  
  	while(queue == NULL || left > 0) 
	{
    
    		seglen = left > pcb->mss? pcb->mss: left;
    
    		/* allocate memory for tcp_seg, and fill in fields */
    		seg = malloc(sizeof(tcp_seg));
    		if(seg == NULL) 
		{
      			printf("tcp_enqueue: could not allocate memory for tcp_seg\n");
      			goto memerr;
    		}
    		seg->next = NULL;
    		seg->p = NULL;
    
    
    		if(queue == NULL) 
		{
      			queue = seg;
    		} 
		else 
		{
      			for(useg = queue; useg->next != NULL; useg = useg->next);
      			useg->next = seg;
    		}
      
    		/* If copy is set, memory should be allocated
       		and data copied into pbuf, otherwise data comes from
       		ROM or other static memory, and need not be copied. If
       		optdata is != NULL, we have options instead of data. */
    		if(optdata != NULL) 
		{
      			if((seg->p = pbuf_alloc(PBUF_TRANSPORT, optlen, PBUF_RW)) == NULL) 
			{
				goto memerr;
      			}
      			++queuelen;
      			seg->dataptr = seg->p->payload;
    		} 
		else if(copy) 
		{
      			if((seg->p = pbuf_alloc(PBUF_TRANSPORT, seglen, PBUF_RW)) == NULL) 
			{
				printf("tcp_enqueue: could not allocate memory for pbuf copy\n");	  
				goto memerr;
      			}
      			++queuelen;
      			if(arg != NULL) 
			{
				memcpy(seg->p->payload, ptr, seglen);
      			}
      			seg->dataptr = seg->p->payload;
    		} 
		else 
		{
      			/* Do not copy the data. */
      			if((p = pbuf_alloc(PBUF_TRANSPORT, seglen, PBUF_RO)) == NULL) 
			{
				printf("tcp_enqueue: could not allocate memory for pbuf non-copy\n");	  	  
				goto memerr;
      			}
      			++queuelen;
      			p->payload = ptr;
      			seg->dataptr = ptr;
      			if((seg->p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RW)) == NULL) 
			{
				pbuf_free(p);
				printf("tcp_enqueue: could not allocate memory for header pbuf\n");		  
				goto memerr;
      			}
      			++queuelen;
      			pbuf_chain(seg->p, p);
    		}
    	
		if(queuelen > TCP_SND_QUEUELEN) 
		{
      			printf("tcp_enqueue: queue too long %d (%d)\n", queuelen, TCP_SND_QUEUELEN); 	
      			goto memerr;
    		}
      
    		seg->len = seglen;
    		/*    if((flags & TCP_SYN) || (flags & TCP_FIN)) { 
      			++seg->len;
      		}*/
      
    		/* build TCP header */
    		if(pbuf_header(seg->p, TCP_HLEN)) 
		{
	
      			printf("tcp_enqueue: no room for TCP header in pbuf.\n");
	
#ifdef TCP_STATS
      ++stats.tcp.err;
#endif /* TCP_STATS */
      			goto memerr;
    		}
    
		seg->tcphdr = seg->p->payload;
    		seg->tcphdr->src = htons(pcb->local_port);
    		seg->tcphdr->dest = htons(pcb->remote_port);
    		seg->tcphdr->seqno = htonl(seqno);
    		seg->tcphdr->urgp = 0;
    		TCPH_FLAGS_SET(seg->tcphdr, flags);
    		/* don't fill in tcphdr->ackno and tcphdr->wnd until later */
      
    		if(optdata == NULL) 
		{
      			TCPH_OFFSET_SET(seg->tcphdr, 5 << 4);
    		} 
		else 
		{
      			TCPH_OFFSET_SET(seg->tcphdr, (5 + optlen / 4) << 4);
      			/* Copy options into data portion of segment.
	 		Options can thus only be sent in non data carrying
	 		segments such as SYN|ACK. */
      			memcpy(seg->dataptr, optdata, optlen);
    		}
    
		printf("tcp_enqueue: queueing %lu:%lu (0x%x)\n",
			      ntohl(seg->tcphdr->seqno), ntohl(seg->tcphdr->seqno) + TCP_TCPLEN(seg), flags);

    		left -= seglen;
    		seqno += seglen;
    		ptr = (void *)((char *)ptr + seglen);
  	

	}    // end while loop

    
  	/* Go to the last segment on the ->unsent queue. */    
  	if(pcb->unsent == NULL) 
	{
    		useg = NULL;
  	} 
	else 
	{
    		for(useg = pcb->unsent; useg->next != NULL; useg = useg->next);
  	}
    
  	/* If there is room in the last pbuf on the unsent queue,
     	chain the first pbuf on the queue together with that. */
  	if(useg != NULL &&
     		TCP_TCPLEN(useg) != 0 &&
     		!(TCPH_FLAGS(useg->tcphdr) & (TCP_SYN | TCP_FIN)) &&
     		!(flags & (TCP_SYN | TCP_FIN)) &&
     		useg->len + queue->len <= pcb->mss) 
	{
    		/* Remove TCP header from first segment. */
    		pbuf_header(queue->p, -TCP_HLEN);
    		pbuf_chain(useg->p, queue->p);
    		useg->len += queue->len;
    		useg->next = queue->next;
      
    		printf("tcp_output: chaining, new len %u\n", useg->len);
    		if(seg == queue) 
		{
      			seg = NULL;
    		}
    		free(queue);
  	} 
	else 
	{      
    		if(useg == NULL) 
		{
      			pcb->unsent = queue;
    		} 
		else 
		{
      			useg->next = queue;
    		}
  	}
  
	if((flags & TCP_SYN) || (flags & TCP_FIN)) 
	{
    		++len;
  	}
  
	pcb->snd_lbb += len;
  	pcb->snd_buf -= len;
  	pcb->snd_queuelen = queuelen;
 	printf("tcp_enqueue: %d (after enqueued)\n", pcb->snd_queuelen);

    
  	/* Set the PSH flag in the last segment that we enqueued, but only
     	if the segment has data (indicated by seglen > 0). */
  	if(seg != NULL && seglen > 0 && seg->tcphdr != NULL) 
	{
    		TCPH_FLAGS_SET(seg->tcphdr, TCPH_FLAGS(seg->tcphdr) | TCP_PSH);
  	}
  
  	return ERR_OK;
 
	memerr:

#ifdef TCP_STATS
  ++stats.tcp.memerr;
#endif /* TCP_STATS */

  		if(queue != NULL) 
		{
    			tcp_segs_free(queue);
  		}

#ifdef LWIP_DEBUG
    if(pcb->snd_queuelen != 0) {
      ASSERT("tcp_enqueue: valid queue length", pcb->unacked != NULL ||
	     pcb->unsent != NULL);
      
    }
#endif /* LWIP_DEBUG */

    		printf("tcp_enqueue: %d (with mem err)\n", pcb->snd_queuelen);
  
	return ERR_MEM;
}
/*-----------------------------------------------------------------------------------*/
/* find out what we can send and send it */
int tcp_output(tcp_pcb *pcb)
{
  	pbuf *p;
  	tcp_hdr *tcphdr;
  	tcp_seg *seg, *useg;
  	u32_t wnd;

#if TCP_CWND_DEBUG
  int i = 0;
#endif /* TCP_CWND_DEBUG */
  
  	wnd = MIN(pcb->snd_wnd, pcb->cwnd);

  
  	seg = pcb->unsent;

  	if(pcb->flags & TF_ACK_NOW) 
	{
    		/* If no segments are enqueued but we should send an ACK, we
       		construct the ACK and send it. */
    		pcb->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
    		p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RW);
    		if(p == NULL) 
		{
      			printf("tcp_enqueue: (ACK) could not allocate pbuf\n");
      			return ERR_BUF;
    		}
    		printf("tcp_enqueue: sending ACK for %lu\n", pcb->rcv_nxt);    
    		
		if(pbuf_header(p, TCP_HLEN)) 
		{
      			printf("tcp_enqueue: (ACK) no room for TCP header in pbuf.\n");
      
#ifdef TCP_STATS
      ++stats.tcp.err;
#endif /* TCP_STATS */
      			pbuf_free(p);
      			return ERR_BUF;
    		}
    
    		tcphdr = p->payload;
    		tcphdr->src = htons(pcb->local_port);
    		tcphdr->dest = htons(pcb->remote_port);
    		tcphdr->seqno = htonl(pcb->snd_nxt);
    		tcphdr->ackno = htonl(pcb->rcv_nxt);
    		TCPH_FLAGS_SET(tcphdr, TCP_ACK);
    		tcphdr->wnd = htons(pcb->rcv_wnd);
    		tcphdr->urgp = 0;
    		TCPH_OFFSET_SET(tcphdr, 5 << 4);
    
    		tcphdr->chksum = 0;
    		tcphdr->chksum = inet_chksum_pseudo(p, &(pcb->local_ip), &(pcb->remote_ip), IP_PROTO_TCP, p->tot_len);

    		ip_output(p, &(pcb->local_ip), &(pcb->remote_ip), TCP_TTL, IP_PROTO_TCP);
    		pbuf_free(p);

    		return ERR_OK;
  	} 
  
#if TCP_OUTPUT_DEBUG
  if(seg == NULL) {
    DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output: nothing to send\n"));
  }
#endif /* TCP_OUTPUT_DEBUG */
#if TCP_CWND_DEBUG
  if(seg == NULL) {
    DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, seg == NULL, ack %lu\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            pcb->lastack));
  } else {
    DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, effwnd %lu, seq %lu, ack %lu\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len,
                            ntohl(seg->tcphdr->seqno), pcb->lastack));
  }
#endif /* TCP_CWND_DEBUG */



  
  	while(seg != NULL && ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len <= wnd) 
	{
    		pcb->rtime = 0;
#if TCP_CWND_DEBUG
    DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, effwnd %lu, seq %lu, ack %lu, i%d\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            ntohl(seg->tcphdr->seqno) + seg->len -
                            pcb->lastack,
                            ntohl(seg->tcphdr->seqno), pcb->lastack, i));
    ++i;
#endif /* TCP_CWND_DEBUG */

    		pcb->unsent = seg->next;
    
    
    		if(pcb->state != SYN_SENT) 
		{
      			TCPH_FLAGS_SET(seg->tcphdr, TCPH_FLAGS(seg->tcphdr) | TCP_ACK);
      			pcb->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
    		}
    
    		tcp_output_segment(seg, pcb);
    		pcb->snd_nxt = ntohl(seg->tcphdr->seqno) + TCP_TCPLEN(seg);
    	
		if(TCP_SEQ_LT(pcb->snd_max, pcb->snd_nxt)) 
		{
      			pcb->snd_max = pcb->snd_nxt;
    		}
    
		/* put segment on unacknowledged list if length > 0 */
    		if(TCP_TCPLEN(seg) > 0) 
		{
      			seg->next = NULL;
      			if(pcb->unacked == NULL) 
			{
        			pcb->unacked = seg;
      			} 
			else 
			{
        			for(useg = pcb->unacked; useg->next != NULL; useg = useg->next);
        			useg->next = seg;
      			}
      			/*      seg->rtime = 0;*/      
    		} 
		else 
		{
      			tcp_seg_free(seg);
    		}
    		seg = pcb->unsent;
  	}  
  
	return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
static void tcp_output_segment(tcp_seg *seg, tcp_pcb *pcb)
{
  	u16_t len, tot_len;
	netif *net_if;

  	/* The TCP header has already been constructed, but the ackno and
   	wnd fields remain. */
  	seg->tcphdr->ackno = htonl(pcb->rcv_nxt);

  	/* silly window avoidance */
  	if(pcb->rcv_wnd < pcb->mss) 
	{
    		seg->tcphdr->wnd = 0;
  	} 
	else 
	{
    		seg->tcphdr->wnd = htons(pcb->rcv_wnd);
  	}

  	/* If we don't have a local IP address, we get one by
     	calling ip_route(). */
  	if(ip_addr_isany(&(pcb->local_ip))) 
	{
    		net_if = ip_route(&(pcb->remote_ip));
    		if(net_if == NULL) 
		{
      			return;
    		}
    		ip_addr_set(&(pcb->local_ip), &(net_if->ip_addr));
  	}

  	pcb->rtime = 0;
  
  	if(pcb->rttest == 0) 
	{
    		pcb->rttest = tcp_ticks;
    		pcb->rtseq = ntohl(seg->tcphdr->seqno);

    		printf("tcp_output_segment: rtseq %lu\n", pcb->rtseq);
  	}
  
	printf("tcp_output_segment: %lu:%lu\n",
			    htonl(seg->tcphdr->seqno), htonl(seg->tcphdr->seqno) +
			    seg->len);

  	seg->tcphdr->chksum = 0;
  	seg->tcphdr->chksum = inet_chksum_pseudo(seg->p,
					   &(pcb->local_ip),
					   &(pcb->remote_ip),
					   IP_PROTO_TCP, seg->p->tot_len);
#ifdef TCP_STATS
  ++stats.tcp.xmit;
#endif /* TCP_STATS */

  	len = seg->p->len;
  	tot_len = seg->p->tot_len;
  	ip_output(seg->p, &(pcb->local_ip), &(pcb->remote_ip), TCP_TTL, IP_PROTO_TCP);
  	seg->p->len = len;
  	seg->p->tot_len = tot_len;
  	seg->p->payload = seg->tcphdr;

}
/*-----------------------------------------------------------------------------------*/
void tcp_rexmit_seg(tcp_pcb *pcb, tcp_seg *seg)
{
  	u32_t wnd;
  	u16_t len, tot_len;
  	netif *net_if;
  
  	printf("tcp_rexmit_seg: skickar %ld:%ld\n",
		    ntohl(seg->tcphdr->seqno),
		    ntohl(seg->tcphdr->seqno) + TCP_TCPLEN(seg));

  	wnd = MIN(pcb->snd_wnd, pcb->cwnd);
  
  	if(ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len <= wnd) 
	{

    		/* Count the number of retranmissions. */
    		++pcb->nrtx;
    
    		if((net_if = ip_route((ip_addr *)&(pcb->remote_ip))) == NULL) 
		{
      			printf("tcp_rexmit_segment: No route to 0x%lx\n", pcb->remote_ip.addr);
#ifdef TCP_STATS
      ++stats.tcp.rterr;
#endif /* TCP_STATS */
      			return;
    		}

    		seg->tcphdr->ackno = htonl(pcb->rcv_nxt);
    		seg->tcphdr->wnd = htons(pcb->rcv_wnd);

    		/* Recalculate checksum. */
    		seg->tcphdr->chksum = 0;
    		seg->tcphdr->chksum = inet_chksum_pseudo(seg->p,
                                             &(pcb->local_ip),
                                             &(pcb->remote_ip),
                                             IP_PROTO_TCP, seg->p->tot_len);
    
    		len = seg->p->len;
    		tot_len = seg->p->tot_len;
    		pbuf_header(seg->p, IP_HLEN);
    		ip_output_if(seg->p, NULL, IP_HDRINCL, TCP_TTL, IP_PROTO_TCP, net_if);
    		seg->p->len = len;
    		seg->p->tot_len = tot_len;
    		seg->p->payload = seg->tcphdr;

#ifdef TCP_STATS
    ++stats.tcp.xmit;
    ++stats.tcp.rexmit;
#endif /* TCP_STATS */

    		pcb->rtime = 0;
    
    		/* Don't take any rtt measurements after retransmitting. */    
    		pcb->rttest = 0;
  	} 
	else 
	{
    		printf("tcp_rexmit_seg: no room in window %lu to send %lu (ack %lu)\n",
                              wnd, ntohl(seg->tcphdr->seqno), pcb->lastack);
  	}
}
/*-----------------------------------------------------------------------------------*/
void tcp_rst(u32_t seqno, u32_t ackno, ip_addr *local_ip, ip_addr *remote_ip, u16_t local_port, u16_t remote_port)
{
  	pbuf *p;
  	tcp_hdr *tcphdr;
  	p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RW);
  
	if(p == NULL) 
	{


#if MEM_RECLAIM
    mem_reclaim(sizeof(pbuf));
    p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RAM);
#endif /* MEM_RECLAIM */    
   
 
		if(p == NULL) 
		{
      			printf("tcp_rst: could not allocate memory for pbuf\n");
      			return;
    		}
  	}

  	if(pbuf_header(p, TCP_HLEN)) 
	{
    		printf("tcp_send_data: no room for TCP header in pbuf.\n");

#ifdef TCP_STATS
    ++stats.tcp.err;
#endif /* TCP_STATS */
    		return;
  	}

  	tcphdr = p->payload;
  	tcphdr->src = htons(local_port);
  	tcphdr->dest = htons(remote_port);
  	tcphdr->seqno = htonl(seqno);
  	tcphdr->ackno = htonl(ackno);
  	TCPH_FLAGS_SET(tcphdr, TCP_RST | TCP_ACK);
  	tcphdr->wnd = 0;
  	tcphdr->urgp = 0;
  	TCPH_OFFSET_SET(tcphdr, 5 << 4);
  
  	tcphdr->chksum = 0;
  	tcphdr->chksum = inet_chksum_pseudo(p, local_ip, remote_ip, IP_PROTO_TCP, p->tot_len);

#ifdef TCP_STATS
  ++stats.tcp.xmit;
#endif /* TCP_STATS */

  	ip_output(p, local_ip, remote_ip, TCP_TTL, IP_PROTO_TCP);
  	pbuf_free(p);
  	printf("tcp_rst: seqno %lu ackno %lu.\n", seqno, ackno);

}
/*-----------------------------------------------------------------------------------*/
