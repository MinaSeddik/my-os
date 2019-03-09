
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <netif.h>
#include <inet.h>
#include <pci.h>
#include <ip.h>
#include <pci.h>
#include <rtl8139.h>
#include <ne2000.h>
#include <blockqueue.h>
#include <thread.h>

netif* netif_list = NULL;
netif* netif_default = NULL;

int nic_irq;


extern err_t ip_input(netif *inp, pbuf *p);

void process_packets();
netif* netif_create();

int netif_init()
{
	int network_supported = 0;
	
	ip_addr ipaddr, netmask, gw;
	IP4_ADDR(&gw, 192,168,40,1);
  	IP4_ADDR(&ipaddr, 192,168,40,138);
  	IP4_ADDR(&netmask, 255,255,255,0);

	printf("\nRealTek-8139 Ethernet Network Interface ...	");
	PCI_Device* rtl_dev = pci_find(0x10ec, 0x8139);
	if ( rtl_dev )
	{
		printf("[ OK ] \n");

		init_network_server();

		netif* net_if = netif_create();
		netif_add2(net_if, &ipaddr, &netmask, &gw);

		net_if->input = ip_input;	// function to be called on packet receiving
		init_rtl8139(rtl_dev, net_if);
		nic_irq = rtl_dev->irq;
		net_if->next = netif_list;
  		netif_list = net_if;
		network_supported = 1;
	}
	else
	{
		printf("[ NO ] \n");		
	}

	printf("NE2000 Ethernet Network Interface ... 	        ");
	PCI_Device* ne_dev = pci_find(0x10ec, 0x8029);
	if ( ne_dev )
	{
		printf("[ OK ] \n");

		//nic_irq = ne_dev->irq;
		if ( !network_supported )    init_network_server();

		netif* net_if = netif_create();
		netif_add2(net_if, &ipaddr, &netmask, &gw);

		net_if->input = ip_input;	// function to be called on packet receiving
		init_ne2000(ne_dev, net_if);
		nic_irq = ne_dev->irq;
		net_if->next = netif_list;
  		netif_list = net_if;
		

	}
	else
	{
		printf("[ NO ] \n");
	}

	if ( !rtl_dev && !ne_dev ) 
		printf("Network NOT Supported!\n\n");

	netif_default = netif_list;

	
}

void init_network_server()
{
	pbuf_init();
	arp_init();
	queue_init();
//printf("will disable ... %d\n", nic_irq);	
//disable_irq(nic_irq);
	THandler handler = create_thread(&process_packets, NULL);
}


void process_packets()
{

	netif* net_if = NULL;
	pbuf* p = NULL;
	int rc = 0;
//printf("#####################\n");
	while(1)
	{
		dequeue(&net_if, &p);
		//printf("#####################\n");
		if (net_if && p)	rc = ether_input(net_if, p);
	}


}


netif* netif_create()
{
	netif* net_if;
  	static int netifnum = 0;
  
  	net_if = (netif*) malloc(sizeof(netif));
  	if(net_if == NULL)	return NULL;
	memset(net_if, 0, sizeof(netif));
  
  	net_if->num = netifnum++;

	strcpy(net_if->name, "eth");
	net_if->name[3] = net_if->num + '0';

	return net_if;
}

int netif_add(char* name, ip_addr *ipaddr, ip_addr *netmask, ip_addr *gw)
{
	
	netif* target = NULL, *tmp = netif_list;

	while( tmp )
	{
		if ( !strcmp(name, tmp->name) )
		{
			target = tmp;
			break;
		}
		tmp = tmp->next;
	}

	if ( !target )	return -1;

  	ip_addr_set(&(target->ip_addr), ipaddr);
  	ip_addr_set(&(target->netmask), netmask);
  	ip_addr_set(&(target->gw), gw);

  	return 0;

}

int netif_add2(netif* target, ip_addr *ipaddr, ip_addr *netmask, ip_addr *gw)
{
	if ( !target )	return -1;

  	ip_addr_set(&(target->ip_addr), ipaddr);
  	ip_addr_set(&(target->netmask), netmask);
  	ip_addr_set(&(target->gw), gw);

  	return 0;

}

netif* netif_find(char *name)
{
	netif *net_if;
	u8_t num;
  
  	if(name == NULL)    return NULL;
 
  	for(net_if = netif_list; net_if != NULL; net_if = net_if->next) 
    		if( !strcmp(name, net_if->name) )	return net_if;

  	return NULL;
}

void netif_set_ipaddr(netif *net_if, ip_addr *ipaddr)
{
  	ip_addr_set(&(net_if->ip_addr), ipaddr);
  	printf("netif: setting IP address of interface %s to %d.%d.%d.%d\n",
		       net_if->name,
		       (u8_t)(ntohl(ipaddr->addr) >> 24 & 0xFF),
		       (u8_t)(ntohl(ipaddr->addr) >> 16 & 0xFF),
		       (u8_t)(ntohl(ipaddr->addr) >> 8 & 0xFF),
		       (u8_t)(ntohl(ipaddr->addr) & 0xFF));
}

void netif_set_gw(netif *net_if, ip_addr *gw)
{
  	ip_addr_set(&(net_if->gw), gw);
}

void netif_set_netmask(netif *net_if, ip_addr *netmask)
{
  	ip_addr_set(&(net_if->netmask), netmask);
}

void netif_set_default(netif *net_if)
{
  	netif_default = net_if;
  	printf("netif: setting default interface %s\n", net_if->name);
}

