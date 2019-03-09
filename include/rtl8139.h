#ifndef _RTL8139_H
#define _RTL8139_H

#include <types.h>
#include <pci.h>
#include <netif.h>


#define RTL8139_PCI_MAC0      			0x00
#define RTL8139_PCI_IOAR0     			0x10
#define RTL8139_PCI_IOAR1     			0x12
#define RTL8139_PCI_CMD       			0x37
#define RTL8139_PCI_TX_CONFIG			0x40
#define RTL8139_PCI_RX_CONFIG			0x44
#define RTL8139_PCI_CFG9346			0x50
#define RTL8139_PCI_CONFIG0   			0x51
#define RTL8139_PCI_CONFIG1   			0x52

#define RTL8139_PCI_CR_RESET  			0X10

#define RECV_BUFFER_PAGES     		3
#define SEND_BUFFER_PAGES     		3

#define RECV_BUFFER_VIRTUAL   		0x2000
#define SEND_BUFFER_VIRTUAL   		0x5000

#define RX_CONFIG_ACCEPT_ALL_PHYS	0x01
#define RX_CONFIG_ACCEPT_MY_PHYS	0x02
#define RX_CONFIG_ACCEPT_MULTICAST	0x04
#define RX_CONFIG_ACCEPT_BROADCAST	0x08
#define RX_CONFIG_ACCEPT_RUNT		0x10
#define RX_CONFIG_ACCEPT_ERR		0x20
#define RX_CONFIG_WRAP			0x80

void init_rtl8139(PCI_Device* dev, netif* net_if);

// interupt handler
u32_t rtl8139_handler(u32_t context);










#endif
