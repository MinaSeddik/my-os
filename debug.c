

#include <debug.h>
#include <stdio.h>
#include <stdlib.h>


void dump_hexa(void* ptr, u32_t len)
{
	u8_t* data = (u8_t*) ptr;

	int i, index = 0;
	while ( index < len )
	{
		printf("%8x  ", index); 
		for (i=0;i<16;i++)
			printf("%2x ", data[index+i]);

		printf(" | ");

		for (i=0;i<16;i++)
			printf("%c", data[index+i] < 32 || data[index+i] > 126 ? '.' : data[index+i]);

		printf("\n");
		index+=16;
	}

	return ;
	
}

