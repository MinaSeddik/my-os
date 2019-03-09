
#include <system.h>
#include <string.h>
#include <vga.h>


/* We will use this later on for reading from the I/O ports to get data
*  from devices such as the keyboard. We are using what is called
*  'inline assembly' in these routines to actually do the work */
u8_t inportb (u16_t _port)
{
    	u8_t rv;
    	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    	return rv;
}

/* We will use this to write to I/O ports to send bytes to devices. This
*  will be used in the next tutorial for changing the textmode cursor
*  position. Again, we use some inline assembly for the stuff that simply
*  cannot be done in C */
void outportb (u16_t _port, u8_t _data)
{
    	__asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

/* Output one byte to an I/O port. Privileged operation. */
void outb(int port, unsigned char value)
{
	asm volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

/* Read a byte from an I/O port. Privileged operation. */
unsigned char inb(int port)
{
	unsigned char value;
	asm volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port));
	return value;
}

unsigned short inw(int port)
{                                                  
	unsigned short value;
	asm volatile("inw %w1, %w0" : "=a"(value) : "Nd"(port));
	return value;
}

void outw(int port, unsigned short value)
{
	asm volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}


/*
//------------------------------------------------------------------------
void insl(u16_t _port, u32_t buffer, u32_t quads)
{
	__asm__ __volatile__ ("cld ; rep ; insl"
			 : "=D" (buffer), "=c" (quads) 
			 : "d" (_port),"0" (buffer),"1" (quads));
}
//------------------------------------------------------------------------
*/
/*
#define insl(port, buffer, count) \
	         asm volatile("cld; rep; insl" :: "D" (buffer), "d" (port), "c" (count))

*/
/*
 *  outl and inl: Output or input a 32-bit word
 */
void outl(u32_t _port,u32_t val)
{
  	__asm__ volatile("out%L0 (%%dx)" : :"a" (val), "d" (_port));
}

u32_t inl(u32_t _port)
{
	u32_t ret;

  	__asm__ volatile("in%L0 (%%dx)" : "=a" (ret) : "d" (_port));
  
	return ret;
}


/*
insl(u16_t port , void * address , int count)
{
        asm volatile("cld; rep insl" :
                "=D" (address), "=c" (count) :
                "d" (port), "0" (address), "1" (count) :
                "memory", "cc");
}

*/

void kprint(char* str)
{
	int i = 0;

	for ( i=0;i<strlen(str);i++)
		putch(str[i]);
}








