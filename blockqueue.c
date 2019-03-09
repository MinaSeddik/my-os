
#include <stdio.h>
#include <system.h>
#include <blockqueue.h>


static net_frame queue[CIRCULAR_QUEUE_LEN];
static int front, rear, is_full;

extern int nic_irq;

int queue_init()
{
	memset(queue, 0, sizeof(pbuf*) * CIRCULAR_QUEUE_LEN);
	front = rear = is_full = 0;
}

void enqueue(netif* net_if, pbuf* p)
{
	queue[rear].net_if = net_if;
	queue[rear++].p = p;
	rear%= CIRCULAR_QUEUE_LEN;
	
	if ( rear == front ) is_full = 1;

//printf("En-Queue rear = %d, front = %d\n",rear, front);
}




int dequeue(netif** net_if, pbuf** p)
{
	net_frame frame ;

	// there is no packet in the queue ( queue is empty )
	while ( rear == front && !is_full )	sleep(2 /* 2 milli-second */ );

	// don't receive any packet during packet queue processing
	disable_irq(nic_irq);
	
	*p = queue[front].p;
	*net_if = queue[front++].net_if;

	front%= CIRCULAR_QUEUE_LEN;
	is_full = 0;

	// enable packet receiving again
	enable_irq(nic_irq);

//printf("De-Queue rear = %d, front = %d\n",rear, front);

	return 0;
	
}


