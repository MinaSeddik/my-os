#ifndef _NET_H
#define _NET_H

#include <types.h>
#include <ip_addr.h>
#include <pbuf.h>


typedef int err_t;


typedef struct _netif 
{
	struct _netif *next;
  	u8_t num;
  	
	ip_addr ip_addr;
  	ip_addr netmask;  /* netmask in network byte order */
  	ip_addr gw;
  	
	char hwaddr[6];

  	/* This function is called by the network device driver
     	when it wants to pass a packet to the TCP/IP stack. */
  	err_t (* input)(struct _netif *net_if, pbuf *p);


  	/* The following two fields should be filled in by the
     	initialization function for the device driver. */

  	char name[5];
  	/* This function is called by the IP module when it wants
     	to send a packet on the interface. */
  	err_t (* output)(struct _netif *net_if, pbuf *p);
  	//err_t (* linkoutput)(struct netif *netif, struct pbuf *p);

  	/* This field can be set by the device driver and could point
     	to state information for the device. */
  	void *state;
}netif;

/* The list of network interfaces. */
extern netif *netif_list;
extern netif *netif_default;


int netif_init();

int netif_add(char* name, ip_addr *ipaddr, ip_addr *netmask, ip_addr *gw);

/* Returns a network interface given its name. The name is of the form
   "et0", where the first three letters are the "name" field in the
   netif structure, and the digit is in the num field in the same
   structure. */
/*
struct netif *netif_find(char *name);

void netif_set_default(struct netif *netif);

void netif_set_ipaddr(struct netif *netif, struct ip_addr *ipaddr);
void netif_set_netmask(struct netif *netif, struct ip_addr *netmast);
void netif_set_gw(struct netif *netif, struct ip_addr *gw);
*/

#endif

