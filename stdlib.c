
#include <stdlib.h>

static unsigned long int next = 1;


double atof ( const char * str )
{
	char *p = (char *) str;
	double value = 0;
	int decimal_places = 1;
	bool is_signed = FALSE;
	bool inside_decimal = FALSE;

	// eat spaces
	while( *p && *p == 0x20 )	++p;

	if ( *p == '-' )		// negative number
	{
		is_signed = TRUE;
		p++;

		// eat spaces
		while( *p && *p == 0x20 )	++p;
	}

	while( *p && ( is_digit(*p) || *p == '.') )
	{
		if( *p == '.' )
		{
			p++;
			inside_decimal = TRUE;
			continue;
		}
	
		value = (value * 10) + ( *p - '0');

		if ( inside_decimal ) 
			decimal_places*= 10;
		p++;
	}

	if ( decimal_places > 0 )
		value/=decimal_places;

	
	if ( is_signed )
		value*=-1;

	return value;
		
}

int atoi ( const char * str )
{
	char *p = (char *) str;
	int value = 0;
	bool is_signed = FALSE;

	// eat spaces
	while( *p && *p == 0x20 )	++p;

	if ( *p == '-' )		// negative number
	{
		is_signed = TRUE;
		p++;

		// eat spaces
		while( *p && *p == 0x20 )	++p;
	}

	while( *p && is_digit(*p) )
	{
		value = (value * 10) + ( *p - '0');
		p++;
	}
	
	if ( is_signed )
		value*=-1;

	return value;	
}

int is_digit(char ch)
{
	return ch >= '0' && ch <= '9' ? 1:0;
}


int abs ( int n )
{
	return n > 0 ? n : -n; 	
}


int rand(void) 			
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % RAND_MAX;
}

void srand(unsigned int seed)
{
    next = seed;
}


char* itoa (int num,char* buffer,int radix)
{
	char buff[33] = {0};
	char *p = buff;
	char *pbuffer = buffer;
	char tmp;
	int value;
	bool is_signed = FALSE;

	if ( num == 0 )
	{
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}


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

char* ftoa (double num,char* buffer,int radix)
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

	if ( num == 0.0 )
	{
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}	

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



char* utoa (unsigned int num,char* buffer,int radix)
{
	char buff[33] = {0};
	char *p = buff;
	char *pbuffer = buffer;
	char tmp;
	unsigned int value = num;

	if ( num == 0 )
	{
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}

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



char tolower(char ch)
{
	return ( ch >= 'A' && ch <= 'Z' ) ? ( ch + 0x20 ) : ch;
}

char toupper(char ch)
{
	return ( ch >= 'a' && ch <= 'z' ) ? ( ch - 0x20 ) : ch;
}










