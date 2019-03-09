
#ifndef _TCP_H
#define _TCP_H

#include <netif.h>
#include <pbuf.h>
#include <ip.h>

typedef signed     long    s32_t;

#define TCP_TTL                 255



#define TCP_SND_BUF             256
#define TCP_WND                 1024
#define TCP_MSS                 128

#define TCP_SND_QUEUELEN        4 * TCP_SND_BUF/TCP_MSS

#define TCP_SEQ_LT(a,b)     ((s32_t)((a)-(b)) < 0)
#define TCP_SEQ_LEQ(a,b)    ((s32_t)((a)-(b)) <= 0)
#define TCP_SEQ_GT(a,b)     ((s32_t)((a)-(b)) > 0)
#define TCP_SEQ_GEQ(a,b)    ((s32_t)((a)-(b)) >= 0)

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

/* Length of the TCP header, excluding options. */
#define TCP_HLEN 20

#define TCP_TMR_INTERVAL       	100  /* The TCP timer interval in milliseconds. */

#define TCP_FAST_INTERVAL      	200  /* the fine grained timeout in milliseconds */

#define TCP_SLOW_INTERVAL      	500  /* the coarse grained timeout in milliseconds */

#define TCP_FIN_WAIT_TIMEOUT 	20000 /* milliseconds */
#define TCP_SYN_RCVD_TIMEOUT 	20000 /* milliseconds */

#define TCP_OOSEQ_TIMEOUT       6 /* x RTO */

#define TCP_MSL 		60000  /* The maximum segment lifetime in microseconds */


#define TCP_SYNMAXRTX           6
#define TCP_MAXRTX              12



typedef struct _tcp_hdr 
{
	u16_t src;
  	u16_t dest;
  	u32_t seqno;
  	u32_t ackno;
  	u16_t _offset_flags;
  	u16_t wnd;
  	u16_t chksum;
  	u16_t urgp;
} __attribute__((packed)) tcp_hdr ;

#define TCPH_OFFSET(hdr) (NTOHS((hdr)->_offset_flags) >> 8)
#define TCPH_FLAGS(hdr) (NTOHS((hdr)->_offset_flags) & 0xff)

#define TCPH_OFFSET_SET(hdr, offset) (hdr)->_offset_flags = HTONS(((offset) << 8) | TCPH_FLAGS(hdr))
#define TCPH_FLAGS_SET(hdr, flags) (hdr)->_offset_flags = HTONS((TCPH_OFFSET(hdr) << 8) | (flags))

#define TCP_TCPLEN(seg) ((seg)->len + ((TCPH_FLAGS((seg)->tcphdr) & TCP_FIN || \
					TCPH_FLAGS((seg)->tcphdr) & TCP_SYN)? 1: 0))

#define UMAX(a, b)      ((a) > (b) ? (a) : (b))

#define MIN(x,y) (x) < (y)? (x): (y)


enum tcp_state 
{
  	CLOSED      = 0,
  	LISTEN      = 1,
  	SYN_SENT    = 2,
  	SYN_RCVD    = 3,
  	ESTABLISHED = 4,
  	FIN_WAIT_1  = 5,
  	FIN_WAIT_2  = 6,
  	CLOSE_WAIT  = 7,
  	CLOSING     = 8,
  	LAST_ACK    = 9,
  	TIME_WAIT   = 10
};


/* This structure is used to repressent TCP segments. */
typedef struct _tcp_seg 
{
	struct _tcp_seg *next;    /* used when putting segements on a queue */
  	pbuf *p;          /* buffer containing data + TCP header */
  	void *dataptr;           /* pointer to the TCP data in the pbuf */
  	u16_t len;               /* the TCP length of this segment */
   	tcp_hdr *tcphdr;  /* the TCP header */
} tcp_seg ;


/* the TCP protocol control block */
typedef struct _tcp_pcb 
{
  	struct _tcp_pcb *next;   /* for the linked list */

  	enum tcp_state state;   /* TCP state */

  	void *callback_arg;
  
  	/* Function to call when a listener has been connected. */
  	err_t (* accept)(void *arg, struct _tcp_pcb *newpcb, err_t err);

  	ip_addr local_ip;
  	u16_t local_port;
  
  	ip_addr remote_ip;
  	u16_t remote_port;
  
  	/* receiver varables */
  	u32_t rcv_nxt;   /* next seqno expected */
  	u16_t rcv_wnd;   /* receiver window */

  	/* Timers */
  	u16_t tmr;

  	/* Retransmission timer. */
  	u8_t rtime;
  
  	u16_t mss;   /* maximum segment size */

  	u8_t flags;

#define TF_ACK_DELAY 0x01   /* Delayed ACK. */
#define TF_ACK_NOW   0x02   /* Immediate ACK. */
#define TF_INFR      0x04   /* In fast recovery. */
#define TF_RESET     0x08   /* Connection was reset. */
#define TF_CLOSED    0x10   /* Connection was sucessfully closed. */
#define TF_GOT_FIN   0x20   /* Connection was closed by the remote end. */
  
  	/* RTT estimation variables. */
  	u16_t rttest; /* RTT estimate in 500ms ticks */
  	u32_t rtseq;  /* sequence number being timed */
  	s32_t sa, sv;

  	u16_t rto;    /* retransmission time-out */
  	u8_t nrtx;    /* number of retransmissions */

  	/* fast retransmit/recovery */
  	u32_t lastack; /* Highest acknowledged seqno. */
  	u8_t dupacks;
  
  	/* congestion avoidance/control variables */
  	u16_t cwnd;  
  	u16_t ssthresh;

  	/* sender variables */
  	u32_t snd_nxt,       /* next seqno to be sent */
    	snd_max,       /* Highest seqno sent. */
    	snd_wnd,       /* sender window */
   	snd_wl1, snd_wl2,
    	snd_lbb;      

  	u16_t snd_buf;   /* Avaliable buffer space for sending. */
  	u8_t snd_queuelen;

  	/* Function to be called when more send buffer space is avaliable. */
  	int (* sent)(void *arg, struct _tcp_pcb *pcb, u16_t space);
  	u16_t acked;
  
  	/* Function to be called when (in-sequence) data has arrived. */
  	int (* recv)(void *arg, struct _tcp_pcb *pcb, pbuf *p, int err);
  	pbuf *recv_data;

  	/* Function to be called when a connection has been set up. */
  	int (* connected)(void *arg, struct _tcp_pcb *pcb, int err);

  	/* Function which is called periodically. */
  	int (* poll)(void *arg, struct _tcp_pcb *pcb);

  	/* Function to be called whenever a fatal error occurs. */
  	void (* errf)(void *arg, int err);
  
  	u8_t polltmr, pollinterval;
  
  	/* These are ordered by sequence number: */
  	tcp_seg *unsent;   /* Unsent (queued) segments. */
  	tcp_seg *unacked;  /* Sent but unacknowledged segments. */

#if TCP_QUEUE_OOSEQ  
  	tcp_seg *ooseq;    /* Received out of sequence segments. */
#endif /* TCP_QUEUE_OOSEQ */

} tcp_pcb ;

typedef struct _tcp_pcb_listen 
{ 
	struct _tcp_pcb_listen *next;   /* for the linked list */
  
  	enum tcp_state state;   /* TCP state */

  	void *callback_arg;
  
  	/* Function to call when a listener has been connected. */
  	void (* accept)(void *arg, struct _tcp_pcb *newpcb);

	ip_addr local_ip;
  	u16_t local_port;
} tcp_pcb_listen ;


/* Functions for interfacing with TCP: */

/* Lower layer interface to TCP: */
void tcp_init(void);  /* Must be called first to initialize TCP. */
void tcp_tmr(void);  /* Must be called every TCP_TMR_INTERVAL ms. (Typically 100 ms). */

/* Application program's interface: */
tcp_pcb* tcp_new(void);

void tcp_arg(tcp_pcb *pcb, void *arg);
void tcp_accept(tcp_pcb *pcb,int (* accept)(void *arg, tcp_pcb *newpcb, err_t err));
void tcp_recv(tcp_pcb *pcb, int (* recv)(void *arg, tcp_pcb *tpcb, pbuf *p, int err));
void tcp_sent(tcp_pcb *pcb, int (* sent)(void *arg, tcp_pcb *tpcb, u16_t len));
void tcp_poll(tcp_pcb *pcb, int (* poll)(void *arg, tcp_pcb *tpcb), u8_t interval);
void tcp_err(tcp_pcb *pcb, void (* err)(void *arg, int err));

#define tcp_sndbuf(pcb)   ((pcb)->snd_buf)

void tcp_recved(tcp_pcb *pcb, u16_t len);
int tcp_bind(tcp_pcb *pcb, ip_addr *ipaddr, u16_t port);
int tcp_connect(tcp_pcb *pcb, ip_addr *ipaddr, u16_t port, err_t (* connected)(void *arg, tcp_pcb *tpcb, int err));
tcp_pcb * tcp_listen(tcp_pcb *pcb);
void tcp_abort(tcp_pcb *pcb);
int tcp_close(tcp_pcb *pcb);
int tcp_write(tcp_pcb *pcb, const void *dataptr, u16_t len, u8_t copy);

/* It is also possible to call these two functions at the right
   intervals (instead of calling tcp_tmr()). */
void tcp_slowtmr(void);
void tcp_fasttmr(void);


/* Only used by IP to pass a TCP segment to TCP: */
void tcp_input(pbuf *p, netif *inp);
/* Used within the TCP code only: */
int tcp_output(tcp_pcb *pcb);



#define tcp_ack(pcb)     if((pcb)->flags & TF_ACK_DELAY) { \
                            (pcb)->flags |= TF_ACK_NOW; \
                            tcp_output(pcb); \
                         } else { \
                            (pcb)->flags |= TF_ACK_DELAY; \
                         }

#define tcp_ack_now(pcb) (pcb)->flags |= TF_ACK_NOW; \
                         tcp_output(pcb)



/* Axoims about the above lists:   
   1) Every TCP PCB that is not CLOSED is in one of the lists.
   2) A PCB is only in one of the lists.
   3) All PCBs in the tcp_listen_pcbs list is in LISTEN state.
   4) All PCBs in the tcp_tw_pcbs list is in TIME-WAIT state.
*/

/* Define two macros, TCP_REG and TCP_RMV that registers a TCP PCB
   with a PCB list or removes a PCB from a list, respectively. */
#ifdef LWIP_DEBUG
#define TCP_REG(pcbs, npcb) do {\
                            DEBUGF(TCP_DEBUG, ("TCP_REG %p local port %d\n", npcb, npcb->local_port)); \
                            for(tcp_tmp_pcb = *pcbs; \
				  tcp_tmp_pcb != NULL; \
				tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                                ASSERT("TCP_REG: already registered\n", tcp_tmp_pcb != npcb); \
                            } \
                            ASSERT("TCP_REG: pcb->state != CLOSED", npcb->state != CLOSED); \
                            npcb->next = *pcbs; \
                            ASSERT("TCP_REG: npcb->next != npcb", npcb->next != npcb); \
                            *pcbs = npcb; \
                            ASSERT("TCP_RMV: tcp_pcbs sane", tcp_pcbs_sane()); \
                            } while(0)
#define TCP_RMV(pcbs, npcb) do { \
                            ASSERT("TCP_RMV: pcbs != NULL", *pcbs != NULL); \
                            printf("TCP_RMV: removing %p from %p\n", npcb, *pcbs); \
                            if(*pcbs == npcb) { \
                               *pcbs = (*pcbs)->next; \
                            } else for(tcp_tmp_pcb = *pcbs; tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                               if(tcp_tmp_pcb->next != NULL && tcp_tmp_pcb->next == npcb) { \
                                  tcp_tmp_pcb->next = npcb->next; \
                                  break; \
                               } \
                            } \
                            npcb->next = NULL; \
                            ASSERT("TCP_RMV: tcp_pcbs sane", tcp_pcbs_sane()); \
                            DEBUGF(TCP_DEBUG, ("TCP_RMV: removed %p from %p\n", npcb, *pcbs)); \
                            } while(0)

#else /* LWIP_DEBUG */
#define TCP_REG(pcbs, npcb) do { \
                            npcb->next = *pcbs; \
                            *pcbs = npcb; \
                            } while(0)
#define TCP_RMV(pcbs, npcb) do { \
                            if(*pcbs == npcb) { \
                               *pcbs = (*pcbs)->next; \
                            } else for(tcp_tmp_pcb = *pcbs; tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                               if(tcp_tmp_pcb->next != NULL && tcp_tmp_pcb->next == npcb) { \
                                  tcp_tmp_pcb->next = npcb->next; \
                                  break; \
                               } \
                            } \
                            npcb->next = NULL; \
                            } while(0)


#endif

















#endif
