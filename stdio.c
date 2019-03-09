
#include <system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#include <vga.h>

#define MSG_MAX_LENGTH		1000

void printf (const char *format, ... )
{
	char msg[MSG_MAX_LENGTH];
	char *p = (char *) format, *pmsg = msg;
	char *stack_pointer = (char *) &format;

	char *str;
	char tmp_str[50];
	int int_val;
	double double_val;
	int padding = 0, i=0;

	stack_pointer+=sizeof(char*);	// skip format string

	memset( (void *) msg, 0, MSG_MAX_LENGTH);

	while ( *p )
	{

		switch(*p)
		{
			case '%':
			{
				p++;

				// padding number
				padding = 0;
				i = 0;
				memset(tmp_str, 0, 50);
				while ( p && *p >= '0' && *p <= '9' ) 
					tmp_str[i++] = *p++;
				padding = atoi(tmp_str);
					
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
					itoa(int_val, tmp_str, 10);

					if ( padding )
					{
						i = strlen(tmp_str);
						padding-=i;
						while( padding-- > 0 )	*pmsg++ = '0';
					}

					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int);	// update stack frame pointer
					break;

					case 'x':	// handle hexa
					int_val = (int) *( (int *) stack_pointer);
					utoa(int_val, tmp_str, 16);	

					if ( padding )
					{
						i = strlen(tmp_str);
						padding-=i;
						while( padding-- > 0 )	*pmsg++ = '0';
					}

					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int*);	// update stack frame pointer
					break;

					case 'c':	// handle char
					int_val = (int) *( (int *) stack_pointer);	
					*pmsg++ = (char)int_val;
					stack_pointer+=sizeof(int*);	// update stack frame pointer
					break;

					case 'f':	// handle float	
					double_val = (double) *((double*) stack_pointer);
					ftoa(double_val, tmp_str, 10);
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

	kprint(msg);

}



void sprintf (char *msg, const char *format, ... )
{
	char *p = (char *) format, *pmsg = msg;
	char *stack_pointer = (char *) &format;

	char *str;
	char tmp_str[50];
	int int_val;
	double double_val;
	int padding = 0, i=0;

	stack_pointer+=sizeof(char*);	// skip format string

	memset( (void *) msg, 0, MSG_MAX_LENGTH);

	while ( *p )
	{

		switch(*p)
		{
			case '%':
			{
				p++;

				// padding number
				padding = 0;
				i = 0;
				memset(tmp_str, 0, 50);
				while ( p && *p >= '0' && *p <= '9' ) 
					tmp_str[i++] = *p++;
				padding = atoi(tmp_str);
					
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
					itoa(int_val, tmp_str, 10);

					if ( padding )
					{
						i = strlen(tmp_str);
						padding-=i;
						while( padding-- > 0 )	*pmsg++ = '0';
					}

					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int);	// update stack frame pointer
					break;

					case 'x':	// handle hexa
					int_val = (int) *( (int *) stack_pointer);
					utoa(int_val, tmp_str, 16);	

					if ( padding )
					{
						i = strlen(tmp_str);
						padding-=i;
						while( padding-- > 0 )	*pmsg++ = '0';
					}

					str = tmp_str;
					while ( *str )
						*pmsg++ = *str++;
					stack_pointer+=sizeof(int*);	// update stack frame pointer
					break;

					case 'c':	// handle char
					int_val = (int) *( (int *) stack_pointer);	
					*pmsg++ = (char)int_val;
					stack_pointer+=sizeof(int*);	// update stack frame pointer
					break;

					case 'f':	// handle float	
					double_val = (double) *((double*) stack_pointer);
					ftoa(double_val, tmp_str, 10);
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

int getline(char* line,int max)
{
	char ch;
	int char_count = 0;

//printf("getline 1\n");
	while ( (ch = get_char()) != '\n' && char_count < max )	
	{
//printf("getline 2\n");
		if ( ch == '\b' ) 	// back space
		{
//printf("getline 3\n");
			if ( char_count > 0 )
			{ 
				char_count--;
				
				// echo char
				putch(ch);
			}
		}
		else
		{
//printf("getline 4\n");
			line[char_count++] = ch ;

			// echo char
			putch(ch);
		}
		

	}

	if ( ch == '\n' )	putch(ch);

	line[char_count] = 0 ;
//printf("getline 5\n");
	return char_count;
}


