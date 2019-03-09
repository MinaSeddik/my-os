#ifndef _ETHER_H
#define _ETHER_H

#include <types.h>
#include <netif.h>
#include <inet.h>
#include <ip.h>
#include <pbuf.h>


#define ETHER_ADDR_LEN 		6
#define ETHER_HLEN 		14

typedef struct _eth_addr 
{
	u8_t addr[ETHER_ADDR_LEN];
} __attribute__((packed)) eth_addr;
  
typedef struct _eth_hdr 
{
  	eth_addr dest;
  	eth_addr src;
  	u16_t type;
} __attribute__((packed)) eth_hdr;



err_t ether_input(netif *net_if, pbuf *p);















#endif
