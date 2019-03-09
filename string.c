
#include <string.h>
#include <stdio.h>

int trace_me = 0;
void *memcpy ( void * destination, const void * source, size_t len )
{
	unsigned char *dest = destination;
	unsigned char const *src = source;

//printf("memcpy before src = %d   dest = %d\n", src, dest);

	int i = 0;
	while (len > 0)
	{
		//dest[i] = src[i];
		*dest++ = *src++;
		--len;
	}

//printf("memcpy after src = %d   dest = %d\n", source, destination);
  	return destination;

}

void *memmove ( void * destination, const void * source, size_t len )
{
	unsigned char *dest;
	unsigned char const *src;
	
	if ( len == 0 )
		return destination;

		
	if (destination >= source)
	{
		dest = destination + len - 1;
		src = source + len - 1;

		while (len-- > 0)
			*dest-- = *src--;
	}
	else // <=
	{
		dest = destination;
		src = source;
		
		while (len-- > 0)
			*dest++ = *src++;
	}
	
	return dest;
}

char *strcpy ( char * destination, const char * source )
{
	char *dest = destination;
	char const *src = source;

	while (*src)
		*dest++ = *src++;
  
	// copy null terminated
	*dest = *src;
	
	return destination;
}
char *strncpy ( char * destination, const char * source, size_t len )
{
	char *dest = destination;
	char const *src = source;

	while (len-- > 0)
		*dest++ = *src++;
  
	
	return destination;
}
char *strcat ( char * destination, const char * source )
{
	char *dest = destination;
	char const *src = source;
	
	while ( *dest) dest++;
	
	while ( *src)
		*dest++ = *src++;
		
	*dest = *src;	// put null
	
	
	return destination;
	
}

char *strncat ( char * destination, char const * source, size_t len )
{
	char *dest = destination;
	char const *src = source;
	
	while ( *dest) dest++;
	
	while ( len-- > 0)
		*dest++ = *src++;
		
	*dest = 0;
	
	
	return destination;	
}

int memcmp ( const void * ptr1, const void * ptr2, size_t len )
{

    if( !len )
        return 0;

    while(--len && *(unsigned char *)ptr1++ == *(unsigned char *)ptr2++ );

    return(*((unsigned char*)ptr1) - *((unsigned char*)ptr2));
		
}

int strcmp ( const char * str1, const char * str2 )
{
	char *p1 = (char*) str1;
	char *p2 = (char*) str2;
		
	while ( *p1 && *p2 && *p1 == *p2)
	{
		p1++;
		p2++;
	}

    return(*p1 - *p2);
}

int strncmp ( const char * str1, const char * str2, size_t len )
{
	char *p1 = (char*) str1;
	char *p2 = (char*) str2;

    if( !len )
        return 0;

    while(--len && *p1 && *p2 && *p1 == *p2 )
		*p1++ = *p2++;

    return(*p1 - *p2);
	
}

void *memset ( void * dest, int val, size_t len )
{


	unsigned char *ptr = (unsigned char*)dest;
 
	while (len-- > 0)
		*ptr++ = val;

	return dest;
	
}

void *test_memset ( void * dest, int val, size_t len )
{

//printf("memset 1 ...  \n");
	unsigned char *ptr = (unsigned char*)dest;
printf("memset 2 ...  ptr = %d , len = %d\n", dest, len); 
//	int j = 0;
//	unsigned char *x = ptr;
//	int size = len;
int trace_me = 1;
	while (len-- > 0)
	{
		*ptr++ = val;
		//j++;
	}
	
//printf("memset 4 ... size = %d j = %d\n", len, j);
	return dest;
	
}

void *memsetw ( void * dest, int val, size_t len )
{
	unsigned short *ptr = (unsigned short*)dest;
  
	while (len-- > 0)
		*ptr++ = val;
  
	return dest;
	
}

size_t strlen ( const char * str )
{
	char *p = (char *) str;

	while ( *p) ++p;

	return (p - str );
}

void *memchr (void *ptr, int value, size_t len )
{
	unsigned char *p = (unsigned char*)ptr;

	while ( --len )
	{
		if (*p == value)	return (void*) p;
		p++;
	}

	return NULL;

	
}

char *strchr (char * str, int ch)
{
	char *p = str;

	while ( *p )
	{
		if (*p == ch)	return (void*) p;
		p++;
	}

	return NULL;	
}

char *strrchr (char * str, int ch )
{
	char *p = str;

	while ( *p++ );

	while ( p >= str )
	{
		if ( *p == ch )	return (void*) p;
		p--;
	}

	return NULL;
}

char *strstr (char * str1, const char * str2 )
{
	size_t len ,i = 0;
	char *p = str1;

	if( *str2 == 0 )	// if empty string
		return str1;

	len = strlen(str2);

	while ( *p )
	{
		if( *p == *(str2+i) )
		{
			if ( i == len-1 )
				return (p - i); 	
			i++;
		}
		else	i = 0;

		++p;
	}

	
	return NULL;
	
}

char *strtok ( char *str, const char *delim )
{
	static char *p;
	char *tmp; 

	if( *delim == 0 )	// if empty string
		return str;

	if ( str != NULL )	
		p = str;

	if ( *p == 0 )	
		return NULL;

	while( *p && strchr( (char *) delim, *p) ) 
		*p++ = 0;	

	tmp = p;

	while( *p && !strchr( (char *) delim, *p) ) 
		p++;

	if (*p)	*p++ = 0;

	return tmp;

}















