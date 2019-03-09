#ifndef _INET_H
#define _INET_H


#include <types.h>
#include <pbuf.h>
#include <ip_addr.h>



#define htons(n)	HTONS(n)
#define HTONS(n) 	(((((u16_t)(n) & 0xff)) << 8) | (((u16_t)(n) & 0xff00) >> 8))


#define htonl(n) 	HTONL(n)
#define HTONL(n) 	(((((u32_t)(n) & 0xff)) << 24) | \
                        ((((u32_t)(n) & 0xff00)) << 8) | \
                        ((((u32_t)(n) & 0xff0000)) >> 8) | \
                        ((((u32_t)(n) & 0xff000000)) >> 24))


#define NTOHS 	HTONS
#define ntohs	htons

#define NTOHL	HTONL
#define ntohl 	htonl


u16_t inet_chksum(void *dataptr, u16_t len);
u16_t inet_chksum_pbuf(pbuf *p);
u16_t inet_chksum_pseudo(pbuf *p, ip_addr *src, ip_addr *dest, u8_t proto, u16_t proto_len);





#endif
