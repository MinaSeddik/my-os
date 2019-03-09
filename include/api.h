
#ifndef _API_H
#define _API_H


#include <err.h>
#include <mutex.h>

#define NETCONN_NOCOPY 0x00
#define NETCONN_COPY   0x01

enum netconn_type 
{
  	NETCONN_TCP,
  	NETCONN_UDP,
  	NETCONN_UDPLITE,
 	NETCONN_UDPNOCHKSUM
};

enum netconn_state 
{
  	NETCONN_NONE,
  	NETCONN_WRITE,
  	NETCONN_ACCEPT,
  	NETCONN_RECV,
  	NETCONN_CONNECT,
  	NETCONN_CLOSE
};

struct netbuf 
{
  	struct pbuf *p, *ptr;
  	struct ip_addr *fromaddr;
  	u16_t fromport;
  	err_t err;
};

struct netconn 
{
  	enum netconn_type type;
  	enum netconn_state state;
  
	union 
	{
    		tcp_pcb *tcp;
    		udp_pcb *udp;
  	} pcb;
  
	err_t err;
  	sys_mbox mbox;
  	sys_mbox recvmbox;
  	sys_mbox acceptmbox;
  	mutex_t mutex;
};

/* Network buffer functions: */
struct netbuf* netbuf_new(void);
void netbuf_delete(struct netbuf *buf);
void* netbuf_alloc(struct netbuf *buf, u16_t size);
void  netbuf_free(struct netbuf *buf);
void  netbuf_ref(struct netbuf *buf, void *dataptr, u16_t size);
void  netbuf_chain(struct netbuf *head, struct netbuf *tail);

u16_t netbuf_len(struct netbuf *buf);
err_t netbuf_data(struct netbuf *buf, void **dataptr, u16_t *len);
s8_t netbuf_next(struct netbuf *buf);
void netbuf_first(struct netbuf *buf);

void netbuf_copy(struct netbuf *buf, void *dataptr, u16_t len);
struct ip_addr* netbuf_fromaddr (struct netbuf *buf);
u16_t netbuf_fromport (struct netbuf *buf);

/* Network connection functions: */
struct netconn* netconn_new(enum netconn_type type);
err_t netconn_delete(struct netconn *conn);
enum netconn_type netconn_type(struct netconn *conn);
err_t netconn_peer(struct netconn *conn, struct ip_addr **addr, u16_t *port);
err_t netconn_addr(struct netconn *conn, struct ip_addr **addr, u16_t *port);
err_t netconn_bind(struct netconn *conn, struct ip_addr *addr, u16_t port);
err_t netconn_connect(struct netconn *conn, struct ip_addr *addr, u16_t port);
err_t netconn_listen(struct netconn *conn);
struct netconn* netconn_accept(struct netconn *conn);
struct netbuf* netconn_recv(struct netconn *conn);
err_t netconn_send(struct netconn *conn, struct netbuf *buf);
err_t netconn_write(struct netconn *conn, void *dataptr, u16_t size, u8_t copy);
err_t netconn_close(struct netconn *conn);
err_t netconn_err(struct netconn *conn);

#endif


