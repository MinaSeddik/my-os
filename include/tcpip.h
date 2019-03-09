
#ifndef _TCPIP_H
#define _TCPIP_H

#include <err.h>
#include <mutex.h

void tcpip_init(void (* tcpip_init_done)(void *), void *arg);
void tcpip_apimsg(struct api_msg *apimsg);
err_t tcpip_input(struct pbuf *p, struct netif *inp);

enum tcpip_msg_type 
{
  	TCPIP_MSG_API,
  	TCPIP_MSG_INPUT
};

struct tcpip_msg 
{
  	enum tcpip_msg_type type;
  	mutex_t *mutex;
  
	union 
	{
    		struct api_msg *apimsg;
    		struct 
		{
      			pbuf *p;
     	 		netif *net_if;
    		} inp;
  	} msg;
};


#endif


