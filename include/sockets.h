

#ifndef _SOCKETS_H
#define _SOCKETS_H


#define LWIP_COMPAT_SOCKETS


struct in_addr 
{
  	u32_t s_addr;
};


struct sockaddr_in 
{
  	u8_t sin_len;
  	u8_t sin_family;
  	u16_t sin_port;
  	struct in_addr sin_addr;
  	char sin_zero[8];
};

struct sockaddr 
{
  	u8_t sa_len;
  	u8_t sa_family;
  	char sa_data[14];
};

#define SOCK_STREAM     1
#define SOCK_DGRAM      2

#define AF_INET         2
#define PF_INET         AF_INET

#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#define INADDR_ANY      0
#define INADDR_BROADCAST 0xFFFFFFFF

int lwip_accept(int s, struct sockaddr *addr, int *addrlen);
int lwip_bind(int s, struct sockaddr *name, int namelen);
int lwip_close(int s);
int lwip_connect(int s, struct sockaddr *name, int namelen);
int lwip_listen(int s, int backlog);
int lwip_recv(int s, void *mem, int len, unsigned int flags);
int lwip_read(int s, void *mem, int len);
int lwip_recvfrom(int s, void *mem, int len, unsigned int flags, struct sockaddr *from, int *fromlen);
int lwip_send(int s, void *dataptr, int size, unsigned int flags);
int lwip_sendto(int s, void *dataptr, int size, unsigned int flags, struct sockaddr *to, int tolen);
int lwip_socket(int domain, int type, int protocol);
int lwip_write(int s, void *dataptr, int size);

#ifdef LWIP_COMPAT_SOCKETS
#define accept(a,b,c)         lwip_accept(a,b,c)
#define bind(a,b,c)           lwip_bind(a,b,c)
#define close(s)              lwip_close(s)
#define connect(a,b,c)        lwip_connect(a,b,c)
#define listen(a,b)           lwip_listen(a,b)
#define recv(a,b,c,d)         lwip_recv(a,b,c,d)
#define read(a,b,c)           lwip_read(a,b,c)
#define recvfrom(a,b,c,d,e,f) lwip_recvfrom(a,b,c,d,e,f)
#define send(a,b,c,d)         lwip_send(a,b,c,d)
#define sendto(a,b,c,d,e,f)   lwip_sendto(a,b,c,d,e,f)
#define socket(a,b,c)         lwip_socket(a,b,c)
#define write(a,b,c)          lwip_write(a,b,c)
#endif /* LWIP_NO_COMPAT_SOCKETS */

#endif 


