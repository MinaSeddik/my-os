
#ifndef _UDP_H
#define _UDP_H

#include <netif.h>
#include <pbuf.h>
#include <ip.h>

#define UDP_HLEN 	8

typedef struct _udp_hdr 
{
	u16_t src;
	u16_t dest;  /* src/dest UDP ports */
  	u16_t len;
  	u16_t chksum;
} __attribute__((packed)) udp_hdr;

#define UDP_FLAGS_NOCHKSUM 	0x01
#define UDP_FLAGS_UDPLITE  	0x02


#define UDP_TTL                 255


typedef struct _udp_pcb 
{
	struct udp_pcb *next;

  	ip_addr local_ip, remote_ip;
  	u16_t local_port, remote_port;
  
  	u8_t flags;
  	u16_t chksum_len;
  
  	void (* recv)(void *arg, struct udp_pcb *pcb, pbuf *p, ip_addr *addr, u16_t port);
  	void *recv_arg;  
} udp_pcb ;

/* The following functions is the application layer interface to the UDP code. */
udp_pcb* udp_new(void);
void udp_remove(udp_pcb *pcb);
int udp_bind(udp_pcb *pcb, ip_addr *ipaddr,
				 u16_t port);
int udp_connect(udp_pcb *pcb, ip_addr *ipaddr, u16_t port);
void udp_recv(udp_pcb *pcb, void (* recv)(void *arg, udp_pcb *upcb, pbuf *p, ip_addr *addr,u16_t port), 
					void *recv_arg);
int udp_send(udp_pcb *pcb, pbuf *p);

#define          udp_flags(pcb)  ((pcb)->flags)
#define          udp_setflags(pcb, f)  ((pcb)->flags = (f))


/* The following functions is the lower layer interface to UDP. */
u8_t udp_lookup(ip_hdr *iphdr, netif *inp);
void udp_input(pbuf *p, netif *inp);
void udp_init(void);


#endif


