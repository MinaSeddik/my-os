
#ifndef _API_MSG_H
#define _API_MSG_H

#include <err.h>

enum api_msg_type 
{
  	API_MSG_NEWCONN,
  	API_MSG_DELCONN,
  
  	API_MSG_BIND,
  	API_MSG_CONNECT,

  	API_MSG_LISTEN,
  	API_MSG_ACCEPT,

  	API_MSG_SEND,
  	API_MSG_RECV,
  	API_MSG_WRITE,

  	API_MSG_CLOSE,
  
  	API_MSG_MAX
};

struct api_msg_msg 
{
  	struct netconn *conn;
  	enum netconn_type conntype;
  	union 
	{
    		struct pbuf *p;   

    		struct  
		{	
      			struct ip_addr *ipaddr;
      			u16_t port;
    		} bc;
    	
		struct 
		{
      			void *dataptr;
      			u16_t len;
      			unsigned char copy;
    		} w;    
    		sys_mbox mbox;
    		u16_t len;
  	} msg;
};

struct api_msg 
{
  	enum api_msg_type type;
  	struct api_msg_msg msg;
};

void api_msg_input(struct api_msg *msg);
void api_msg_post(struct api_msg *msg);

#endif 

