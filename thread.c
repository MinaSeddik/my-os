
#include <thread.h>
#include <system.h>
#include <memory.h>

extern proc*	current_proc;


thread_t* create_thread( void (*threadfunc)(void*) , void* threadargs)
{
	thread_t* handler = NULL;

//printf("create_thread,  before handler = 0x%x\n", handler);

	handler = ( thread_t* ) malloc( sizeof(thread_t) );

//printf("create_thread,  after handler = 0x%x\n", handler);

	handler->stack_base = (u32_t*) malloc(THREAD_STACK_SIZE);
	handler->stack_size = THREAD_STACK_SIZE;//3999999;//THREAD_STACK_SIZE;

	handler->stack_pointer = (u32_t*) (( (u32_t) handler->stack_base ) + handler->stack_size);	
			
	handler->threadfunc = threadfunc;	
	handler->threadargs = threadargs;

	u32_t* tmp = handler->stack_pointer;

//printf("stack base = 0x%x = %d\n", handler->stack_base, handler->stack_base);
//printf("stack pointer = 0x%x = %d \n", handler->stack_pointer, handler->stack_pointer);
//printf("first val = %d \n", (handler->stack_pointer));


	// prepare thread_stack	
	*(--(handler->stack_pointer)) = (u32_t) GDT_KERNEL_DATA_SEL<<3;	// SS	exactly is DS
//printf("first val = %d \n", (handler->stack_pointer));
	*(--(handler->stack_pointer)) = (u32_t) handler->stack_pointer; // ESP
	*(--(handler->stack_pointer)) = (u32_t) EFLAGS_IF | EFLAGS_RESERVED;	// EFLAGS
	*(--(handler->stack_pointer)) = (u32_t) GDT_KERNEL_CODE_SEL<<3;	// CS
	*(--(handler->stack_pointer)) = (u32_t) &run_thread;	// EIP

	*(--(handler->stack_pointer)) = (u32_t) 0;	// error code
	*(--(handler->stack_pointer)) = (u32_t) 0;	// interrupt number

	*(--(handler->stack_pointer)) = (u32_t) 0;	// EAX
	*(--(handler->stack_pointer)) = (u32_t) 0;	// ECX
	*(--(handler->stack_pointer)) = (u32_t) 0;	// EDX
	*(--(handler->stack_pointer)) = (u32_t) 0;	// EBX
	*(--(handler->stack_pointer)) = (u32_t) 0;	// ESP
	*(--(handler->stack_pointer)) = (u32_t) handler->stack_base;	// EBP
//	*(--(handler->stack_pointer)) = (u32_t) 0;	// EBP
	*(--(handler->stack_pointer)) = (u32_t) 0;	// ESI
	*(--(handler->stack_pointer)) = (u32_t) 0;	// EDI

	*(--(handler->stack_pointer)) = (u32_t) GDT_KERNEL_DATA_SEL<<3;	// DS
	*(--(handler->stack_pointer)) = (u32_t) GDT_KERNEL_DATA_SEL<<3;	// ES
	*(--(handler->stack_pointer)) = (u32_t) GDT_KERNEL_DATA_SEL<<3;	// FS
	*(--(handler->stack_pointer)) = (u32_t) GDT_KERNEL_DATA_SEL<<3;	// GS

	*(--(handler->stack_pointer)) = (u32_t) 0;	// dummy EAX
/*
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);
--tmp;
printf("%d   first val = %d \n", tmp, *tmp);

printf("stack ptr %d   first val = %d \n", handler->stack_pointer, *(handler->stack_pointer));
*/
//printf("I'm here create_thread b\n");
  	/* Add to the schedulers queue */
  	scheduler_add_thread(handler);

//printf("I'm here create_thread a\n");

	return handler;
}


void destroy_thread(THandler thread_handler)
{

//printf("destroy_thread thread_handler = %x  stack = %x \n", thread_handler , thread_handler->stack_base );


//printf("destroy_thread 2\n");
	if ( thread_handler != NULL && thread_handler->stack_base != NULL )
		free(thread_handler->stack_base);

//printf("destroy_thread**** thread_handler = %x  stack = %x \n", thread_handler , thread_handler->stack_base );

//printf("destroy_thread 3\n");
	if ( thread_handler != NULL )
		free(thread_handler);
}

void run_thread()
{

//printf("run_thread()\n");



  	/* Run the thread */
  	(*current_proc->threadfunc) (current_proc->threadargs);

//printf("I'm here 5555555555555555555555555555\n");

//printf("I'm here 11111111111111111111111111\n");
	/* destroy thread */
	destroy_thread(current_proc->thread_handler);

	/* notify any process waiting this process to finish */
	notify_waiting_process_for_process_finish(current_proc->proc_id);

//printf("I'm here 888888888888888888888888888888\n");
//printf("I'm here 4 removing id = %d\n",current_proc->proc_id );
	/* Tell the scheduler to remove the thread */
	scheduler_remove_thread(current_proc->proc_id);

//printf("I'm here 7777777777777777777777777777777777\n");
//printf("I'm here 5\n");
}

