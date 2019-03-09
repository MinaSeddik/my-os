#ifndef _STDLIB_H
#define _STDLIB_H

#include <types.h>

// RAND_MAX assumed to be 32767 -> 0x7FFF   ||   2147483647 -> 0x7FFFFFFF
#define RAND_MAX	0x7FFFFFFF


double atof ( const char * str );
int atoi ( const char * str );

char* ftoa (double num,char* buffer,int radix);
char* itoa (int num,char* buffer,int radix);
char* utoa (unsigned int num,char* buffer,int radix);


int rand ( void );
void srand ( unsigned int seed );

int abs ( int n );

int is_digit(char ch);


char tolower(char ch);
char toupper(char ch);


#endif
