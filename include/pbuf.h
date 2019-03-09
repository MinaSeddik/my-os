
#ifndef PBUF_H
#define PBUF_H


#include <types.h>


#define MEM_ALIGNMENT           4
#define PBUF_POOL_SIZE          128
#define PBUF_POOL_BUFSIZE       128


#define PBUF_TRANSPORT_HLEN 	20
#define PBUF_IP_HLEN        	20
#define PBUF_LINK_HLEN      	14

#define PBUF_TRANSPORT      	0
#define PBUF_IP             	1
#define PBUF_LINK           	2
#define PBUF_RAW            	3

#define PBUF_RW             	0
#define PBUF_RO             	1
#define PBUF_POOL           	2

// Definitions for the pbuf flag field (these are not the flags that
// are passed to pbuf_alloc()).

#define PBUF_FLAG_RW    	0x00    // Flags that pbuf data is read/write.
#define PBUF_FLAG_RO    	0x01    // Flags that pbuf data is read-only.
#define PBUF_FLAG_POOL  	0x02    // Flags that the pbuf comes from the pbuf pool.

typedef struct pbuf 
{
	struct pbuf *next;
  
  	u16_t flags;
  	u16_t ref;
  	void *payload;
  
  	int tot_len;                // Total length of buffer + additionally chained buffers.
  	int len;                    // Length of this buffer.
  	int size;                   // Allocated size of buffer
} __attribute__((packed)) pbuf;

void pbuf_init();

pbuf* pbuf_alloc(int layer, int size, int type);
void pbuf_realloc(pbuf *p, int size); 
int pbuf_header(pbuf *p, int header_size);
int pbuf_clen(pbuf *p);
int pbuf_spare(pbuf *p);
void pbuf_ref(pbuf *p);
int pbuf_free(pbuf *p);
void pbuf_chain(pbuf *h, pbuf *t);
pbuf* pbuf_dechain(pbuf *p);
pbuf* pbuf_dup(int layer, pbuf *p);
pbuf* pbuf_linearize(int layer, pbuf *p);
pbuf* pbuf_cow(int layer, pbuf *p);

#endif
