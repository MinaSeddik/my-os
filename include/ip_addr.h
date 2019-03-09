#ifndef _IP_ADDR_H
#define _IP_ADDR_H

#include <types.h>

#define IP_ADDR_ANY 	0

typedef struct _ip_addr 
{
	u32_t addr;
} __attribute__((packed)) ip_addr;


#define IP_ADDR_BROADCAST (&ip_addr_broadcast)

extern ip_addr ip_addr_broadcast;


#define IP4_ADDR(ipaddr, a,b,c,d) (ipaddr)->addr = htonl(((u32_t)(a & 0xFF) << 24) | ((u32_t)(b & 0xFF) << 16) | \
                                                         ((u32_t)(c & 0xFF) << 8) | (u32_t)(d & 0xFF))


#define ip_addr_set(dest, src) (dest)->addr = \
                               ((src) == IP_ADDR_ANY? IP_ADDR_ANY:\
				((ip_addr *)src)->addr)


#define ip_addr_maskcmp(addr1, addr2, mask) (((addr1)->addr & \
                                              (mask)->addr) == \
                                             ((addr2)->addr & \
                                              (mask)->addr))


#define ip_addr_isany(addr1) ((addr1) == NULL || (addr1)->addr == 0)

#define ip_addr_cmp(addr1, addr2) ((addr1)->addr == (addr2)->addr)

#define ip_addr_isbroadcast(addr1, mask) (((((addr1)->addr) & ~((mask)->addr)) == \
					 (0xffffffff & ~((mask)->addr))) || \
                                         ((addr1)->addr == 0xffffffff) || \
                                         ((addr1)->addr == 0x00000000))

#define ip_addr_ismulticast(addr1) (((addr1)->addr & ntohl(0xf0000000)) == ntohl(0xe0000000))

#define ip4_addr1(ipaddr) ((unsigned char)(ntohl((ipaddr)->addr) >> 24) & 0xFF)
#define ip4_addr2(ipaddr) ((unsigned char)(ntohl((ipaddr)->addr) >> 16) & 0xFF)
#define ip4_addr3(ipaddr) ((unsigned char)(ntohl((ipaddr)->addr) >> 8) & 0xFF)
#define ip4_addr4(ipaddr) ((unsigned char)(ntohl((ipaddr)->addr)) & 0xFF)

#endif


