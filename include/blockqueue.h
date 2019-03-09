#ifndef _BLOCK_QUEUE_H
#define _BLOCK_QUEUE_H


#include <types.h>
#include <pbuf.h>
#include <netif.h>


#define CIRCULAR_QUEUE_LEN 	1024

typedef struct _net_frame
{
	netif* net_if;
	pbuf* p;
} net_frame;

void enqueue(netif* net_if, pbuf* p);
int dequeue(netif** net_if, pbuf** p);





#endif

