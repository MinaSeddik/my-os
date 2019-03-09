#ifndef _IP_H
#define _IP_H

#include <types.h>
#include <netif.h>
#include <inet.h>
#include <pbuf.h>
#include <ip_addr.h>

#define IP_STATS	
#define IP_HDRINCL 	NULL

typedef struct _ip_hdr 
{
  	/* version / header length / type of service */
  	u16_t _v_hl_tos;

  	/* total length */
	u16_t _len;
  
	/* identification */
  	u16_t _id;
  
	/* fragment offset field */
  	u16_t _offset;

#define IP_RF 0x8000        /* reserved fragment flag */
#define IP_DF 0x4000        /* dont fragment flag */
#define IP_MF 0x2000        /* more fragments flag */
#define IP_OFFMASK 0x1fff   /* mask for fragmenting bits */

  	/* time to live / protocol*/
  	u16_t _ttl_proto;
  
	/* checksum */
  	u16_t _chksum;
  
	/* source and destination IP addresses */
  	ip_addr src;
  	ip_addr dest; 

} __attribute__((packed)) ip_hdr;


#define IP_HLEN 		20

#define IP_PROTO_ICMP 		1
#define IP_PROTO_UDP 		17
#define IP_PROTO_UDPLITE 	170
#define IP_PROTO_TCP 		6

#define IPH_V(hdr)  (NTOHS((hdr)->_v_hl_tos) >> 12)
#define IPH_HL(hdr) ((NTOHS((hdr)->_v_hl_tos) >> 8) & 0x0f)
#define IPH_TOS(hdr) HTONS((NTOHS((hdr)->_v_hl_tos) & 0xff))
#define IPH_LEN(hdr) ((hdr)->_len)
#define IPH_ID(hdr) ((hdr)->_id)
#define IPH_OFFSET(hdr) ((hdr)->_offset)
#define IPH_TTL(hdr) (NTOHS((hdr)->_ttl_proto) >> 8)
#define IPH_PROTO(hdr) (NTOHS((hdr)->_ttl_proto) & 0xff)
#define IPH_CHKSUM(hdr) ((hdr)->_chksum)


#define IPH_VHLTOS_SET(hdr, v, hl, tos) (hdr)->_v_hl_tos = HTONS(((v) << 12) | ((hl) << 8) | (tos))
#define IPH_LEN_SET(hdr, len) (hdr)->_len = (len)
#define IPH_ID_SET(hdr, id) (hdr)->_id = (id)
#define IPH_OFFSET_SET(hdr, off) (hdr)->_offset = (off)
#define IPH_TTL_SET(hdr, ttl) (hdr)->_ttl_proto = HTONS(IPH_PROTO(hdr) | ((ttl) << 8))
#define IPH_PROTO_SET(hdr, proto) (hdr)->_ttl_proto = HTONS((proto) | (IPH_TTL(hdr) << 8))
#define IPH_CHKSUM_SET(hdr, chksum) (hdr)->_chksum = (chksum)


// not used
void ip_init(void);

u8_t ip_lookup(void *header, netif *inp);

netif *ip_route(ip_addr *dest);

int ip_input(netif *inp, pbuf *p);

int ip_output(pbuf *p, ip_addr *src, ip_addr *dest, u8_t ttl, u8_t proto);

int ip_output_if(pbuf *p, ip_addr *src, ip_addr *dest, u8_t ttl, u8_t proto, netif *net_if);















#endif


