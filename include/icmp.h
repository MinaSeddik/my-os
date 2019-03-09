#ifndef _ICMP_H
#define _ICMP_H

#include <netif.h>
#include <pbuf.h>


#define ICMP_TTL                255


#define ICMP_ER 	0      	/* echo reply */
#define ICMP_DUR 	3     	/* destination unreachable */
#define ICMP_SQ 	4      	/* source quench */
#define ICMP_RD 	5      	/* redirect */
#define ICMP_ECHO 	8    	/* echo */
#define ICMP_TE 	11     	/* time exceeded */
#define ICMP_PP 	12     	/* parameter problem */
#define ICMP_TS 	13     	/* timestamp */
#define ICMP_TSR 	14    	/* timestamp reply */
#define ICMP_IRQ 	15    	/* information request */
#define ICMP_IR 	16     	/* information reply */

enum icmp_dur_type 
{
  	ICMP_DUR_NET = 0,    /* net unreachable */
  	ICMP_DUR_HOST = 1,   /* host unreachable */
  	ICMP_DUR_PROTO = 2,  /* protocol unreachable */
  	ICMP_DUR_PORT = 3,   /* port unreachable */
  	ICMP_DUR_FRAG = 4,   /* fragmentation needed and DF set */
  	ICMP_DUR_SR = 5      /* source route failed */
};

enum icmp_te_type 
{
  	ICMP_TE_TTL = 0,     /* time to live exceeded in transit */
  	ICMP_TE_FRAG = 1     /* fragment reassembly time exceeded */
};

void icmp_input(pbuf *p, netif *inp);

void icmp_dest_unreach(pbuf *p, enum icmp_dur_type t);
void icmp_time_exceeded(pbuf *p, enum icmp_te_type t);

typedef struct _icmp_echo_hdr 
{
	u16_t _type_code;
	u16_t chksum;
	u16_t id;
  	u16_t seqno;
} __attribute__((packed)) icmp_echo_hdr;


typedef struct _icmp_dur_hdr 
{
	u16_t _type_code;
	u16_t chksum;
	u32_t unused;
} __attribute__((packed)) icmp_dur_hdr;

typedef struct icmp_te_hdr 
{
  	u16_t _type_code;
  	u16_t chksum;
  	u32_t unused;
} __attribute__((packed)) icmp_te_hdr;

#define ICMPH_TYPE(hdr) (NTOHS((hdr)->_type_code) >> 8)
#define ICMPH_CODE(hdr) (NTOHS((hdr)->_type_code) & 0xff)

#define ICMPH_TYPE_SET(hdr, type) ((hdr)->_type_code = HTONS(ICMPH_CODE(hdr) | ((type) << 8)))
#define ICMPH_CODE_SET(hdr, code) ((hdr)->_type_code = HTONS((code) | (ICMPH_TYPE(hdr) << 8)))

#endif
	  
