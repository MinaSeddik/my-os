
#include <stdio.h>
#include <inet.h>
#include <pbuf.h>
#include <pbuf.h>
#include <ip_addr.h>
#include <debug.h>




/*-----------------------------------------------------------------------------------*/
/* chksum:
 *
 * Sums up all 16 bit words in a memory portion. Also includes any odd byte.
 * This function is used by the other checksum functions.
 *
 * For now, this is not optimized. Must be optimized for the particular processor
 * arcitecture on which it is to run. Preferebly coded in assembler.
 */
/*-----------------------------------------------------------------------------------*/
static u32_t chksum(void *dataptr, int len)
{
	u32_t acc;
    	u16_t* data = (u16_t*) dataptr;

  	for(acc = 0; len > 1; len -= 2) 
	{
    		acc += *data++;
  	}

  	/* add up any odd byte */
  	if(len == 1) 
	{
    		acc += htons((u16_t)((*(u8_t *)dataptr) & 0xff) << 8);
    		printf("inet: chksum: odd byte %d\n", *(u8_t *)dataptr);
  	}

  	return acc;
}




/*-----------------------------------------------------------------------------------*/
/* inet_chksum:
 *
 * Calculates the Internet checksum over a portion of memory. Used primarely for IP
 * and ICMP.
 */
/*-----------------------------------------------------------------------------------*/
u16_t inet_chksum(void *dataptr, u16_t len)
{
	u32_t acc;

  	acc = chksum(dataptr, len);
  	while(acc >> 16) 
	{
    		acc = (acc & 0xffff) + (acc >> 16);
  	}    
  	
	return ~(acc & 0xffff);
}



/*-----------------------------------------------------------------------------------*/
/* inet_chksum_pseudo:
 *
 * Calculates the pseudo Internet checksum used by TCP and UDP for a pbuf chain.
 */
/*-----------------------------------------------------------------------------------*/
u16_t inet_chksum_pseudo(pbuf *p, ip_addr *src, ip_addr *dest, u8_t proto, u16_t proto_len)
{
	u32_t acc;
  	pbuf *q;
  	u8_t swapped;

  	acc = 0;
  	swapped = 0;
  	for(q = p; q != NULL; q = q->next) 
	{    
    		acc += chksum(q->payload, q->len);
    		while(acc >> 16) 
		{
      			acc = (acc & 0xffff) + (acc >> 16);
    		}
	
    		if(q->len % 2 != 0) 
		{
      			swapped = 1 - swapped;
      			acc = ((acc & 0xff) << 8) | ((acc & 0xff00) >> 8);
    		}
  	}

  	if(swapped) 
	{
    		acc = ((acc & 0xff) << 8) | ((acc & 0xff00) >> 8);
  	}
  
	acc += (src->addr & 0xffff);
  	acc += ((src->addr >> 16) & 0xffff);
  	acc += (dest->addr & 0xffff);
  	acc += ((dest->addr >> 16) & 0xffff);
  	acc += (u32_t)htons((u16_t)proto);
  	acc += (u32_t)htons(proto_len);  
  
  	while(acc >> 16) 
	{
    		acc = (acc & 0xffff) + (acc >> 16);
  	}    
  
	return ~(acc & 0xffff);
}



u16_t inet_chksum_pbuf(pbuf *p)
{
	u32_t acc;
  	pbuf *q;
  	u8_t swapped;
  
  	acc = 0;
  	swapped = 0;
	for(q = p; q != NULL; q = q->next) 
	{
    		acc += chksum(q->payload, q->len);
    		while(acc >> 16) 
		{
      			acc = (acc & 0xffff) + (acc >> 16);
    		}    
    
		if(q->len % 2 != 0)	
		{
      			swapped = 1 - swapped;
      			acc = (acc & 0xff << 8) | (acc & 0xff00 >> 8);
    		}
  	}
 
  	if(swapped) 
	{
    		acc = ((acc & 0xff) << 8) | ((acc & 0xff00) >> 8);
  	}
  
	return ~(acc & 0xffff);
}









