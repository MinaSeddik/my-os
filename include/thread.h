#ifndef _THREAD_H
#define _THREAD_H

#include <types.h>

#define THandler		thread_t*

#define THREAD_STACK_SIZE	0x4000		// 16 KB

typedef struct _thread
{
	u32_t  pid; 				/* 	PID of the task */

  	u32_t*  stack_pointer;                	/* 	Pointer to the head of the stack */
  	u32_t*  stack_base;           		/* 	Pointer to the base of the stack */
  	u32_t  stack_size;            		/* 	Size of the stack */

  	void   (*threadfunc)(void*); 		/* 	Thread's main function */
  	void*  threadargs;           		/* 	Argument of the function */

} thread_t;


typedef struct _Th_Param
{
	int argc;
	char** argv;
}Th_Param;



thread_t* create_thread( void (*threadfunc)(void*) , void* threadargs);

void destroy_thread(THandler thread );

void run_thread();




#endif
