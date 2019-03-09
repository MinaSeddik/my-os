

#include <pbuf.h>
#include <memory.h>
#include <mutex.h>
#include <stats.h>
#include <mem.h>

extern netstats stats;

static pbuf *pbuf_pool = NULL;
static pbuf *pbuf_pool_alloc_cache = NULL;
static pbuf *pbuf_pool_free_cache = NULL;

static int pbuf_pool_free_lock, pbuf_pool_alloc_lock;

//static mutex_t pbuf_mutex;

void pbuf_init() 
{
	pbuf *p, *q;
	int i;

  	// Allocate buffer pool
  	pbuf_pool = (pbuf *) malloc(PBUF_POOL_SIZE * (PBUF_POOL_BUFSIZE + sizeof(pbuf)));
  	stats.pbuf.avail = PBUF_POOL_SIZE;
  
  	// Set up next pointers to link the pbufs of the pool together
  	p = pbuf_pool;
  
  	for (i = 0; i < PBUF_POOL_SIZE; i++) 
	{
    		p->next = (pbuf *) ((char *) p + PBUF_POOL_BUFSIZE + sizeof(pbuf));
    		p->len = p->tot_len = p->size = PBUF_POOL_BUFSIZE;
    		p->payload = (void *) ((char *) p + sizeof(pbuf));
    		q = p;
    		p = p->next;
  	}
  
  	// The next pointer of last pbuf is NULL to indicate that there are no more pbufs in the pool
  	q->next = NULL;

	mem_init();
//printf("here\n"); while(1);
//	mutex_init(&pbuf_mutex);
	
}


static pbuf *pbuf_pool_alloc() 
{
  	pbuf *p = NULL;

	// First, see if there are pbufs in the cache
  	if ( pbuf_pool_alloc_cache ) 
	{
    		p = pbuf_pool_alloc_cache;
    		if (p) pbuf_pool_alloc_cache = p->next; 
  	} 
	else 
	{
    		// Next, check the actual pbuf pool, but if the pool is locked, we
    		// pretend to be out of buffers and return NULL
    		if (pbuf_pool_free_lock) 
		{
      			stats.pbuf.alloc_locked++;
      			return NULL;
    		}
    
//		lock(&pbuf_mutex);
    		pbuf_pool_alloc_lock++;

    		if (!pbuf_pool_free_lock) 
		{
      			p = pbuf_pool;
      			if(p) pbuf_pool = p->next; 
    		} 
		else 
		{
      			stats.pbuf.alloc_locked++;
    		}

    		pbuf_pool_alloc_lock--;
//		unlock(&pbuf_mutex);

  	}

  
	if (p != NULL) 
	{
    		stats.pbuf.used++;
    		if (stats.pbuf.used > stats.pbuf.max) stats.pbuf.max = stats.pbuf.used;
  	}

  	return p;   
}


void pbuf_pool_free(pbuf *p) 
{
	pbuf *q;

  	for (q = p; q != NULL; q = q->next) stats.pbuf.used--;
  
  	if (pbuf_pool_alloc_cache == NULL) 
	{
    		pbuf_pool_alloc_cache = p;
  	} 
	else 
	{
    		for (q = pbuf_pool_alloc_cache; q->next != NULL; q = q->next);
    		q->next = p;    
  	}
}


pbuf* pbuf_alloc(int layer, int size, int flag) 
{
  	pbuf *p, *q, *r;
  	int offset;
  	int rsize;

  	//printf("pbuf_alloc: alloc %d bytes layer=%d flags=%d\n", size, layer, flag);

  	offset = 0;
  	switch (layer) 
	{
    		case PBUF_TRANSPORT:
			offset += PBUF_TRANSPORT_HLEN;
      			// FALLTHROUGH

    		case PBUF_IP:
      			offset += PBUF_IP_HLEN;
      			// FALLTHROUGH

    		case PBUF_LINK:
      			offset += PBUF_LINK_HLEN;
      			// FALLTHROUGH

    		case PBUF_RAW:
      			break;

    		default:
      			printf("ERROR : pbuf_alloc -> bad pbuf layer");

  	}	// end switch



  	switch (flag) 
	{
    		case PBUF_POOL:
      			// Allocate head of pbuf chain into p
      			p = pbuf_pool_alloc();
      			if (p == NULL)  
			{
        			stats.pbuf.err++;
        			return NULL;
      			}
      			p->next = NULL;
    
      			// Set the payload pointer so that it points offset bytes into pbuf data memory
      			p->payload = (void *) ((char *) p + sizeof(pbuf) + offset);

      			// The total length of the pbuf is the requested size
      			p->tot_len = size;

      			// Set the length of the first pbuf in the chain
      			p->len = size > PBUF_POOL_BUFSIZE - offset ? PBUF_POOL_BUFSIZE - offset: size;

      			p->flags = PBUF_FLAG_POOL;
    
      			// Allocate the tail of the pbuf chain
      			r = p;
      			rsize = size - p->len;
      			while (rsize > 0) 
			{
        			q = pbuf_pool_alloc();
        			if (q == NULL) 
				{
          				stats.pbuf.err++;
          				pbuf_pool_free(p);
          				return NULL;
        			}
        			q->next = NULL;
        			r->next = q;
        			q->len = rsize > PBUF_POOL_BUFSIZE ? PBUF_POOL_BUFSIZE: rsize;
        			q->flags = PBUF_FLAG_POOL;
        			q->payload = (void *) ((char *) q + sizeof(pbuf));
        			r = q;
        			q->ref = 1;
        			q = q->next;
        			rsize -= PBUF_POOL_BUFSIZE;
      			}
      			r->next = NULL;
      			break;

    		case PBUF_RW:
      			// If pbuf is to be allocated in RAM, allocate memory for it
      			p = (pbuf *) mem_malloc(sizeof(pbuf) + size + offset);
      			if (p == NULL) return NULL;

      			// Set up internal structure of the pbuf.
      			p->payload = (void *) ((char *) p + sizeof(pbuf) + offset);
//printf(" p = %d\t p->payload = %d \n", p, p->payload );
//printf(" next = %d\t flags = %d tot_len = %d\n", &(p->next), &p->flags, &p->tot_len );
//while(1);
      			p->len = p->tot_len = p->size = size;
      			p->next = NULL;
      			p->flags = PBUF_FLAG_RW;


      			stats.pbuf.rwbufs++;
//printf("ne2000: allocating PBUF_RW = %d\n", sizeof(pbuf) + size + offset);
      			break;

    		case PBUF_RO:
      			// If the pbuf should point to ROM, we only need to allocate memory for the pbuf structure
      			p = (pbuf *) mem_malloc(sizeof(pbuf));
      			if (p == NULL) return NULL;

      			p->payload = NULL;
      			p->len = p->tot_len = p->size = size;
      			p->next = NULL;
      			p->flags = PBUF_FLAG_RO;
     			break;

    		default:
      			printf("ERROR : pbuf_alloc -> erroneous flag");
  	}

  	//printf("pbuf: %d bufs\n", stats.pbuf.rwbufs);
  	p->ref = 1;
  	return p;
}


// Moves free buffers from the pbuf_pool_free_cache to the pbuf_pool
void pbuf_refresh() 
{
	pbuf *p;

  	if (pbuf_pool_free_cache != NULL) 
	{
    		pbuf_pool_free_lock++;

    		if (!pbuf_pool_alloc_lock) 
		{
      			if (pbuf_pool == NULL) 
			{
       				pbuf_pool = pbuf_pool_free_cache;       
      			} 
			else 
			{  
        			for (p = pbuf_pool; p->next != NULL; p = p->next);
        			p->next = pbuf_pool_free_cache;
      			}

      			pbuf_pool_free_cache = NULL;
    		} 
		else 
		{
      			stats.pbuf.refresh_locked++;
    		}
    
    		pbuf_pool_free_lock--;
  	}
}


void pbuf_realloc(pbuf *p, int size) 
{

	pbuf *q, *r;
  	int rsize;



  	if (p->tot_len <= size) return;

//printf("re-alloc size = %d  p->tot_len = %d\n", size, p->tot_len); 
//while(1);


  	switch (p->flags) 
	{
		case PBUF_FLAG_POOL:
      			// First, step over any pbufs that should still be in the chain
      			rsize = size;
      			q = p;  
      			while (rsize > q->len) 
			{
        			rsize -= q->len;
        			q = q->next;
      			}
      
      			// Adjust the length of the pbuf that will be halved
      			q->len = rsize;

      			// And deallocate any left over pbufs
      			r = q->next;
      			q->next = NULL;
      			q = r;
      			while (q != NULL) 
			{
        			r = q->next;

        			q->next = pbuf_pool_free_cache;
        			pbuf_pool_free_cache = q;

        			stats.pbuf.used--;
        			q = r;
      			}
      			break;

    		case PBUF_FLAG_RO:
      			p->len = size;
      			break;

    		case PBUF_FLAG_RW:
      			// First, step over the pbufs that should still be in the chain.
      			rsize = size;
      			q = p;
      			while (rsize > q->len) 
			{
        			rsize -= q->len;
        			q = q->next;
      			}

      			if (q->flags == PBUF_FLAG_RW) 
			{
        			// Reallocate and adjust the length of the pbuf that will be halved
        
				// TODO: we cannot reallocate the buffer without relinking it, we just leave it for now
        			mem_realloc(q, (u8_t *)q->payload - (u8_t *)q + rsize/sizeof(u8_t));
      			}
    
      			q->len = rsize;
    
      			// And deallocate any left over pbufs
     			r = q->next;
      			q->next = NULL;
      			pbuf_free(r);
      			break;
  	}	// end switch stmt

  	p->tot_len = size;
  	pbuf_refresh();
}


int pbuf_header(pbuf *p, int header_size) 
{
	void *payload;

  	payload = p->payload;
  	p->payload = (char *) (p->payload) - header_size;
  
	if ((char *) p->payload < (char *) p + sizeof(pbuf)) 
	{
    		p->payload = payload;
    		return -1;
  	}
  	p->len += header_size;
  	p->tot_len += header_size;
  
  	return 0;
}

int pbuf_free(pbuf *p) 
{
	pbuf *q;
  	int count = 0;
    
  	if (p == NULL) return 0;

  	// Decrement reference count
  	p->ref--;
//printf("------ne2000: free PBUF_RW = %d\n", p->flags);
  	// If reference count is zero, actually deallocate pbuf
  	if (p->ref == 0) 
	{

    		q = NULL;
    		while (p != NULL) 
		{
//printf("ne2000: inside while ....\n");
      			// Check if this is a pbuf from the pool
      			if (p->flags == PBUF_FLAG_POOL) 
			{
        			p->len = p->tot_len = PBUF_POOL_BUFSIZE;
        			p->payload = (void *)((char *) p + sizeof(pbuf));
        			q = p->next;

        			p->next = pbuf_pool_free_cache;
        			pbuf_pool_free_cache = p;

        			stats.pbuf.used--;
      			} 
			else if (p->flags == PBUF_FLAG_RO) 
			{
        			q = p->next;
        			mem_free(p);
      			} 
			else 
			{
//printf("ne2000: free before = %d\n", p->flags);
        			q = p->next;
        			stats.pbuf.rwbufs--;
        			mem_free(p);
//printf("ne2000: free after = %d\n", p->flags);
      			}

      			p = q;
      			count++;
    		}
  	}

	//printf("pbuf: %d bufs\n", stats.pbuf.rwbufs);
  	pbuf_refresh();
  	return count;
}

// chain length, number of nodes in the chain
int pbuf_clen(pbuf *p) 
{
	int len;

  	if (!p) return 0;
  
  	for (len = 0; p != NULL; p = p->next) ++len;

  	return len;
}

int pbuf_spare(struct pbuf *p) 
{
	return ((char *) (p + 1) + p->size) - ((char *) p->payload + p->len);
}

void pbuf_ref(struct pbuf *p) 
{
	if (p == NULL) return;
  	p->ref++;
}

void pbuf_chain(pbuf *h, pbuf *t) 
{
	pbuf *p;

  	if (t == NULL) return;
  
	for (p = h; p->next != NULL; p = p->next);
  	p->next = t;
  	h->tot_len += t->tot_len;
}

pbuf* pbuf_dechain(pbuf *p) 
{
	pbuf *q;
  
  	q = p->next;
  	if (q) q->tot_len = p->tot_len - p->len;
  	p->tot_len = p->len;
  	p->next = NULL;

  	return q;
}

pbuf* pbuf_dup(int layer, pbuf *p) 
{
	pbuf *q;
  	char *ptr;
  	int size;

 	// Allocate new pbuf
  	size = p->tot_len;
  	q = pbuf_alloc(layer, size, PBUF_RW);
  	if (q == NULL) return NULL;

  	// Copy buffer contents
  	ptr = q->payload;
  	while (p) 
	{
    		memcpy(ptr, p->payload, p->len);
	    	ptr += p->len;
    		p = p->next;
  	}

  	return q;
}

pbuf* pbuf_linearize(int layer, pbuf *p) 
{
	pbuf *q;

  	if (!p->next) return p;

  	q = pbuf_dup(layer, p);
  	if (!q) return NULL;
  	pbuf_free(p);
  
  	return q;
}

pbuf* pbuf_cow(int layer, pbuf *p) 
{
	pbuf *q;

  	if (p->ref == 1) return p;
  	q = pbuf_dup(layer, p);
  	if (!q) return NULL;
  	pbuf_free(p);
  
  	return q;
}


