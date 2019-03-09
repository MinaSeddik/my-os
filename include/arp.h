#ifndef _ARP_H
#define _ARP_H

#include <types.h>
#include <ether.h>
#include <ip.h>
#include <netif.h>
#include <inet.h>
#include <pbuf.h>


#define ARP_TIMER_INTERVAL	3	// 3 seconds
#define ARP_MAX_AGE         	120*10         // 120 seconds * 10


#define ARP_TABLE_SIZE          32
#define ARP_XMIT_QUEUE_SIZE     14


#define ETHTYPE_ARP 		0x0806
#define ETHTYPE_IP  		0x0800


#define ARP_REQUEST        	1
#define ARP_REPLY          	2

#define HWTYPE_ETHERNET    	1

#define ARPH_HWLEN(hdr) (NTOHS((hdr)->_hwlen_protolen) >> 8)
#define ARPH_PROTOLEN(hdr) (NTOHS((hdr)->_hwlen_protolen) & 0xFF)

#define ARPH_HWLEN_SET(hdr, len) (hdr)->_hwlen_protolen = HTONS(ARPH_PROTOLEN(hdr) | ((len) << 8))
#define ARPH_PROTOLEN_SET(hdr, len) (hdr)->_hwlen_protolen = HTONS((len) | (ARPH_HWLEN(hdr) << 8))


typedef struct _ethip_hdr 
{
	eth_hdr eth;
  	ip_hdr ip;
} __attribute__((packed)) ethip_hdr;


typedef struct _arp_entry 
{
	ip_addr ipaddr;
  	eth_addr ethaddr;
  	int ctime;
}arp_entry;

typedef struct _arp_hdr 
{
	eth_hdr ethhdr;           // Ethernet header

  	u16_t hwtype;           // Hardware type
  	u16_t proto;            // Protocol type
  	u16_t _hwlen_protolen;  // Protocol address length
  	u16_t opcode;           // Opcode ( request or response )
  	eth_addr shwaddr;         // Source hardware address
  	ip_addr sipaddr;          // Source protocol address
  	eth_addr dhwaddr;         // Target hardware address
  	ip_addr dipaddr;          // Target protocol address

} __attribute__((packed)) arp_hdr ;


typedef struct _xmit_queue_entry 
{
	netif *net_if;
  	pbuf *p;
  	ip_addr ipaddr;
  	u32_t expires;
} xmit_queue_entry ;


void arp_init();

void arp_ip_input(netif *net_if, pbuf *p);
pbuf* arp_arp_input(netif *net_if, eth_addr *ethaddr, pbuf *p);


eth_addr* arp_lookup(ip_addr *ipaddr);
pbuf* arp_query(netif *net_if, eth_addr *ethaddr, ip_addr *ipaddr);


#endif
