

#include <stdio.h>
#include <tcpip.h>
#include <mutex.h>
#include <ip.h>
#include <err.h>




static void (* tcpip_init_done)(void *arg) = NULL;
static void *tcpip_init_done_arg;
static sys_mbox mbox;

/*-----------------------------------------------------------------------------------*/
static void tcpip_tcp_timer(void *arg)
{
  	tcp_tmr();
  	sys_timeout(TCP_TMR_INTERVAL, (sys_timeout_handler)tcpip_tcp_timer, NULL);
}
/*-----------------------------------------------------------------------------------*/

static void tcpip_thread(void *arg)
{
  	struct tcpip_msg *msg;

  	ip_init();
  	udp_init();
  	tcp_init();

  	sys_timeout(TCP_TMR_INTERVAL, (sys_timeout_handler)tcpip_tcp_timer, NULL);
  
  	if(tcpip_init_done != NULL) 
	{
    		tcpip_init_done(tcpip_init_done_arg);
  	}

  	while(1)	                   /* MAIN Loop */ 
	{       
    		sys_mbox_fetch(mbox, (void *)&msg);
    		switch(msg->type) 
		{
    			case TCPIP_MSG_API:
      				printf("tcpip_thread: API message %p\n", msg);
      				api_msg_input(msg->msg.apimsg);
      				break;
    			case TCPIP_MSG_INPUT:
      				printf("tcpip_thread: IP packet %p\n", msg);
      				ip_input(msg->msg.inp.netif, msg->msg.inp.p);
      				break;
    			default:
      				break;
    		}
    		free(msg);
  	}
}
/*-----------------------------------------------------------------------------------*/
err_t tcpip_input(pbuf *p, netif *inp)
{
  	struct tcpip_msg *msg;
  
  	msg = memp_mallocp(MEMP_TCPIP_MSG);
  	if(msg == NULL) 
	{
    		pbuf_free(p);    
    		return ERR_MEM;  
  	}
  
  	msg->type = TCPIP_MSG_INPUT;
  	msg->msg.inp.p = p;
  	msg->msg.inp.netif = inp;
  
	sys_mbox_post(mbox, msg);
  
	return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
void tcpip_apimsg(struct api_msg *apimsg)
{
  	struct tcpip_msg *msg;
  	msg = malloc(sizeof(struct tcpip_msg));
  
	if(msg == NULL) 
	{
    		free(apimsg);
    		return;
  	}
  
	msg->type = TCPIP_MSG_API;
  	msg->msg.apimsg = apimsg;
  	sys_mbox_post(mbox, msg);
}
/*-----------------------------------------------------------------------------------*/
void tcpip_init(void (* initfunc)(void *), void *arg)
{
  	tcpip_init_done = initfunc;
  	tcpip_init_done_arg = arg;
  	mbox = sys_mbox_new();
  	sys_thread_new((void *)tcpip_thread, NULL);
}
/*-----------------------------------------------------------------------------------*/



