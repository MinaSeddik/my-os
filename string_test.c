#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <string.h>

#define MSG_MAX_LENGTH		1000

#define FALSE	0
#define TRUE	1

typedef unsigned char 	u8_t;
typedef unsigned short 	u16_t;
typedef unsigned long 	u32_t;

struct gdt_entry
{
    u16_t limit_low;
    u16_t base_low;
    u8_t  base_middle;
    u8_t access;
    u8_t granularity;		// limit and flags ( bytes or pages )
    u8_t base_high;
} __attribute__((packed));

typedef int bool;

char* ftoa1 (double num,char* buffer,int radix);
char* itoa1 (int num,char* buffer,int radix);
char* utoa1 (unsigned int num,char* buffer,int radix);

void xxxprintf (const char *format, ... );

int main ()
{

int x= 30;
double d = 465.13;
char msg[] = "hello monmon !";
char *p = (char*) &x;

printf("size = %d\n" , sizeof(struct gdt_entry));

printf("int size = %d\n" , sizeof(int));

xxxprintf("x = %d, d = %f, msg = %s, p=0x%x ", x, d, msg, p);

}



void xxxprintf (const char *format, ... )
{
	char msg[MSG_MAX_LENGTH];
	char *p = (char *) format, *pmsg = msg;
	char *stack_pointer = (char *) &format;
	bool inside_format = FALSE;

	char *str;
	char tmp_str[50];
	int int_val;
	double double_val;

	stack_pointer+=sizeof(char*);	// skip format string

	memset( (void *) msg, 0, MSG_MAX_LENGTH);

	while ( *p )
	{
		switch(*p)
		{
			case '%':
			{
				p++;
				switch(*p)
				{
					case 's':	// handle string
					str = (char *) *( (int *)stack_pointer);
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int*);	// update stack frame pointer
					break;

					case 'd':	// handle integer
					int_val = (int) *( (int *) stack_pointer);
					itoa1(int_val, tmp_str, 10);
					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int);	// update stack frame pointer
					break;

					case 'x':	// handle hexa
					int_val = (int) *( (int *) stack_pointer);
					utoa1(int_val, tmp_str, 16);	
					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int*);	// update stack frame pointer
					break;

					case 'f':	// handle float	
					double_val = (double) *((double*) stack_pointer);
					ftoa1(double_val, tmp_str, 10);
					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(double);	// update stack frame pointer
					break;

				}	// end inner switch
				break;	
			}
			default :
				*pmsg++ = *p;

				
		}	// end outter switch

			++p;

	}	// end while loop

	*pmsg = 0;

}



char* itoa1 (int num,char* buffer,int radix)
{
	char buff[33] = {0};
	char *p = buff;
	char *pbuffer = buffer;
	char tmp;
	int value;
	bool is_signed = FALSE;


	if( num < 0 )
		is_signed = TRUE;

	value = abs(num);

	while ( value != 0 )
	{
		tmp= value % radix;
		value/= radix;
		
		*p = tmp >= 0 && tmp <= 9 ? tmp + '0' : tmp + 87;
		
		p++;
	}
	
	if ( is_signed )
		*pbuffer++ = '-';


	while ( p > buff )
		*pbuffer++ = *--p;

	*pbuffer = 0;

	return buffer;
}


char* ftoa1 (double num,char* buffer,int radix)
{
	char buff[33] = {0};
	char *p = buff;
	char *pbuffer = buffer;
	char tmp;
	double value = num;
	bool is_signed = FALSE;

	int int_part;
	double fract_part;
	double tmp_fract;
	int max_decimal_places = 10;

	

	if( num < 0 )
	{
		is_signed = TRUE;
		value = -value;
	}
	
	// for percision problem
	value+= 0.0000000001;


	int_part = value;
	fract_part = value - int_part;


	while ( int_part != 0 )
	{
		tmp = int_part % radix;
		int_part/= radix;
		
		*p = tmp >= 0 && tmp <= 9 ? tmp + '0' : tmp + 87;
		
		p++;
	}
	
	if ( is_signed )
		*pbuffer++ = '-';


	while ( p > buff )
		*pbuffer++ = *--p;

	if ( fract_part != 0.0 )
	{
		*pbuffer++ = '.';
	
		while ( fract_part != 0.0 && max_decimal_places-- > 0)
		{
			tmp_fract = fract_part * radix;
			tmp = (int) tmp_fract;
			tmp_fract-= tmp;
			fract_part = tmp_fract;
			
			*pbuffer++ = tmp >= 0 && tmp <= 9 ? tmp + '0' : tmp + 87;
		}
		 
	}

	*pbuffer = 0;

	return buffer;
}



char* utoa1 (unsigned int num,char* buffer,int radix)
{
	char buff[33] = {0};
	char *p = buff;
	char *pbuffer = buffer;
	char tmp;
	int value;

	while ( value != 0 )
	{
		tmp= value % radix;
		value/= radix;
		
		*p = tmp >= 0 && tmp <= 9 ? tmp + '0' : tmp + 87;
		
		p++;
	}

	while ( p > buff )
		*pbuffer++ = *--p;

	*pbuffer = 0;

	return buffer;
}

