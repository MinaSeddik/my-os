
#include <scheduler.h>
#include <system.h>


extern u32_t kernal_top_stack;
extern u32_t kernal_stack;

proc*	current_proc;		// the current running process

u32_t proc_count;
u32_t ready_count;
u32_t blocked_count;

extern int kernel_loaded;

int trace = 0;
int sdebug = 0;

void re_schedule()		// invoke scheduler
{
	//printf("*** i'll send software INT 32\n");
	__asm__ __volatile__ ("INT $32");
}

u32_t schedule(u32_t context)
{
	int p = 0;



	if ( ! kernel_loaded )
	{
	//	printf("still inside kernel, context = 0x%x\n", context);
	//	struct regs* x= (struct regs *) (context+4);
	//	printf (" [%d] *** inside kernel ***> CS:EIP  0x%x:%d = %x \n", x->int_no, x->cs, x->eip, x->eip);
		//while(1);
		return context;
	}


//printf(".");

/*
if ( ready_count > 1 )
{
	printf("[%d] ready_count = %d blocked_count = %d   current = %d\n", current_proc->proc_id, ready_count, blocked_count, current_proc );
	printf("stack pointer = %d\n",context);

trace++;
if ( trace == 1 )	
{
	//debug_register(current_proc->stack_pointer + 4);
	//while(1);

	struct regs* test = (struct regs *) (current_proc->stack_pointer+4);
	printf (" [%d] ***> CS:EIP  0x%x:%d = %x \n", test->int_no, test->cs, test->eip, test->eip);
	debug_register(test);
	while(1);
}


}
*/


//printf("b-still [%d] block count = %d --- ready count = %d  \n", current_proc->proc_id, blocked_count, ready_count);

	if ( ready_count > 1 )
	{

		// first of all, save the process context ESP
		if(current_proc != NULL )	// if NULL ,ignore that context 
			current_proc->stack_pointer = context;	
		else
			swich_to_next_process();

		switch (current_proc->status)
		{
			case PROCESS_STATUS_NORMAL:
				if( current_proc->ticks_assgined == 0 )
				{

					// switch to the next process
					swich_to_next_process();
				}
				break;

			case PROCESS_BLOCKED_BY_MUTEX:
				if( current_proc->locked_mutex != NULL && current_proc->locked_mutex->entries > current_proc->locked_mutex->exits && current_proc == ready[ready_count-1] )
				{
					// 1) remove it from the ready queue
					ready[ready_count-1] = NULL;
					--ready_count;

					// 2) add it into the blocked queue
					blocked[blocked_count++] = current_proc;
					
					// 3) switch to the next process
					swich_to_next_process();

				}
				else
				{
					current_proc->status = PROCESS_STATUS_NORMAL;
					current_proc->locked_mutex = NULL;
				}	
				break;


			case PROCESS_BLOCKED_BY_WAIT_TIME:
				if( current_proc->wait_ticks > 0 && current_proc == ready[ready_count-1] )
				{

					// 1) remove it from the ready queue
					ready[ready_count-1] = NULL;
					--ready_count;

					// 2) add it into the blocked queue
					blocked[blocked_count++] = current_proc;
//printf("still %d---  count = %d ---", current_proc->wait_ticks, blocked_count);					
					// 3) switch to the next process
					swich_to_next_process();
//printf(" --- next pid = %d ---", current_proc->proc_id);	
				}
				else
				{
//printf("here");
					current_proc->status = PROCESS_STATUS_NORMAL;
					current_proc->wait_ticks = 0;
				}	
				break;

			case PROCESS_BLOCKED_BY_WAIT_PROC_TO_FINISH:
				if( current_proc == ready[ready_count-1] && current_proc->wait_proc_id != -1 )
				{

					// 1) remove it from the ready queue
					ready[ready_count-1] = NULL;
					--ready_count;

					// 2) add it into the blocked queue
					blocked[blocked_count++] = current_proc;
//printf("b-still [%d] %d---  count = %d --- ready count = %d	***********\n", current_proc->proc_id, current_proc->wait_proc_id, blocked_count, ready_count);					
					// 3) switch to the next process
					swich_to_next_process();
//printf("a-still [%d] %d---  count = %d --- ready count = %d status = %d \n", current_proc->proc_id, current_proc->wait_proc_id, blocked_count, ready_count, current_proc->status);

//debug = 1;	
//printf(" --- next pid = %d ---", current_proc->proc_id);	
				}
				else
				{
//printf("here");
					current_proc->status = PROCESS_STATUS_NORMAL;
				}
				break;



			case PROCESS_BLOCKED_BY_WAIT_KB_INPUT:
				if( current_proc == ready[ready_count-1] )
				{

					// 1) remove it from the ready queue
					ready[ready_count-1] = NULL;
					--ready_count;

					// 2) add it into the blocked queue
					blocked[blocked_count++] = current_proc;
//printf("still %d---  count = %d ---", current_proc->wait_ticks, blocked_count);					
					// 3) switch to the next process
					swich_to_next_process();
//printf(" --- next pid = %d ---", current_proc->proc_id);	
				}
				else
				{
//printf("here");
					current_proc->status = PROCESS_STATUS_NORMAL;
				}	
				break;



			case PROCESS_NOTIFY_BY_MUTEX:
				if( current_proc->locked_mutex != NULL && blocked_count > 0 )
				{
					while ( (p = process_blocked_by_mutux(current_proc->locked_mutex) ) != -1 )
						blocked_to_ready(p);
	
					current_proc->status = PROCESS_STATUS_NORMAL;
					current_proc->locked_mutex = NULL;					
					swich_to_next_process();
				}
				else
				{
					current_proc->status = PROCESS_STATUS_NORMAL;
					current_proc->locked_mutex = NULL;
				}
				break;


		}	// end switch statment


		
	}
	else
	{
		if (current_proc == NULL )	// it is a virgin process, doesn't run before
			current_proc = ready[0];
		else
			current_proc->stack_pointer = context;
	}

	

/*
if ( ready_count > 1 )
{
	//printf("[%d] ready_count = %d blocked_count = %d\n", current_proc->proc_id, ready_count, blocked_count);
	//printf("stack pointer = %d\n",current_proc->stack_pointer);

trace++;
if ( trace > 0 )	
{
	//debug_register(current_proc->stack_pointer + 4);
	//while(1);

	printf (" stack pointer = %d val = %d \n", current_proc->stack_pointer, *(current_proc->stack_pointer));
	u32_t temp =  ((u32_t)current_proc->stack_pointer)+4;
	struct regs* test = (struct regs *) temp;
	printf (" [%d] ***> CS:EIP  0x%x:%d = %x \n", test->int_no, test->cs, test->eip, test->eip);
	//debug_register(test);
	
	//if (trace == 86) while(1);
}


}
*/


	return current_proc->stack_pointer;
		
}


void swich_to_next_process()
{
	proc* tmp;

//printf("swich_to_next_process  ready count = %d\n",ready_count);

	if ( ready_count > 1 )
	{
		current_proc = ready[1];
		current_proc->ticks_assgined = MAX_TICK_PREIOD;
			
		tmp = ready[1];
		memmove(ready+1, ready+2, sizeof(proc*) * (ready_count - 2) );
		ready[ready_count-1] = tmp;
	}
	else
		current_proc = ready[0];
	
 	
}

int process_blocked_by_mutux(mutex_t* mutex)
{
	int i;

	for ( i=0;i<blocked_count;i++)
		if ( blocked[i]->status == PROCESS_BLOCKED_BY_MUTEX && blocked[i]->locked_mutex == mutex )
			return i;	// index in the block queue

	return -1;	// not found
			
}

int process_blocked_by_timeout()
{
	int i;
//printf("process_blocked_by_timeout : count =  %d \n", blocked_count);

	for ( i=0;i<blocked_count;i++)
	{
//printf("[%d] : status = %d,  wait_tick = %d \n", i, PROCESS_BLOCKED_BY_WAIT_TIME , blocked[i]->wait_ticks);
		if ( blocked[i]->status == PROCESS_BLOCKED_BY_WAIT_TIME && blocked[i]->wait_ticks == 0 )
			return i;	// index in the block queue
	}

	return -1;	// not found		
}


int process_blocked_by_kb_input()
{
	int i;
//printf("process_blocked_by_timeout : count =  %d \n", blocked_count);

	for ( i=0;i<blocked_count;i++)
	{
//printf("[%d] : status = %d,  wait_tick = %d \n", i, PROCESS_BLOCKED_BY_WAIT_TIME , blocked[i]->wait_ticks);
		if ( blocked[i]->status == PROCESS_BLOCKED_BY_WAIT_KB_INPUT )
			return i;	// index in the block queue
	}

	return -1;	// not found		
}

int process_blocked_process_to_finish(int proc_id)
{
	int i;
//printf("process_blocked_by_timeout : count =  %d \n", blocked_count);

	for ( i=0;i<blocked_count;i++)
	{
//printf("[%d] : status = %d,  wait_tick = %d \n", i, PROCESS_BLOCKED_BY_WAIT_TIME , blocked[i]->wait_ticks);
		if ( blocked[i]->status == PROCESS_BLOCKED_BY_WAIT_PROC_TO_FINISH && proc_id == blocked[i]->wait_proc_id)
			return i;	// index in the block queue
	}

	return -1;	// not found		
}


void blocked_to_ready(int index)
{
	proc* tmp = blocked[index];

	memmove(ready+2, ready+1, sizeof(proc*) * (ready_count - 1) );
	ready[1] = tmp;
	
	tmp->status = PROCESS_STATUS_NORMAL;
	tmp->locked_mutex = NULL;
	tmp->wait_ticks = 0;
	
	if ( index == blocked_count-1)
		blocked[index] = NULL;
	else
		memmove(blocked+index, blocked+index+1, sizeof(proc*) * (blocked_count-index-1) );

	++ready_count;
	--blocked_count;

}


void scheduler_add_thread(THandler th_handler)
{

	int i;	

	__asm__ __volatile__ ("cli");			// disable interrupt

	// starting from 1 , 0 is idle process
	for (i=1;i<MAX_PROCESS;++i)
		if( process[i].bsy_slot == 0 )		break;
	
	if (i == MAX_PROCESS)
	{
		__asm__ __volatile__ ("sti");		// enable interrupt
		return;	// return error code		
	}
	
	th_handler->pid = i;
	
	process[i].proc_id 		= th_handler->pid;
	process[i].wait_proc_id		= -1;
	process[i].total_ticks_count 	= 0;
	process[i].ticks_assgined 	= MAX_TICK_PREIOD;
	process[i].wait_ticks 		= 0;
	process[i].base_stack 		= th_handler->stack_base;
	process[i].stack_pointer	= th_handler->stack_pointer;
	process[i].stack_size 		= th_handler->stack_size;
	process[i].status 		= 0;		
	process[i].locked_mutex		= NULL;
	process[i].bsy_slot		= 1;
	process[i].thread_handler	= th_handler;
	process[i].threadfunc		= th_handler->threadfunc;
	process[i].threadargs		= th_handler->threadargs;
	process[i].status		= PROCESS_STATUS_NORMAL;

//printf("init status = %d\n", process[i].status);

	//process[proc_count].name 		= NULL;

	// insert it in the front of the ready queue
	memmove(ready+2, ready+1, sizeof(proc*) * (ready_count - 1) );
	ready[1] = &process[i];

	++ready_count;
	++proc_count;

	kernel_loaded = 1;	
/*
	u32_t* tmp =  process[i].stack_pointer;
	printf("\n-----stack ptr %d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);
++tmp;
printf("%d   first val = %d \n", tmp, *tmp);

printf("stack ptr %d   first val = %d \n", process[i].stack_pointer, *(process[i].stack_pointer));
*/
//while(1);
	__asm__ __volatile__ ("sti");		// enable interrupt
	
}

void idle_process()
{
	while(1)
{
//printf(".");	
		__asm__ __volatile__ ("hlt");	// hlt processor untill next interrupt
}	
}

void init_system_idle_process()
{

	process[0].proc_id 		= 0;
	process[0].total_ticks_count 	= 0;
	process[0].ticks_assgined 	= 0;
	process[0].wait_ticks 		= 0;
	process[0].base_stack 		= (u32_t*) &kernal_top_stack;	// use kernel stack
	process[0].stack_pointer	= (u32_t*) &kernal_stack;
	process[0].stack_size 		= 0x4000;	
	process[0].status 		= 0;	
	process[0].locked_mutex		= NULL;
	process[0].bsy_slot		= 1;
	process[0].threadfunc		= &idle_process;
	process[0].threadargs		= NULL;
	strcpy(process[0].name, "SystemIdleProcess");

	ready[0] = &process[0];

	// set up stack, override kernel stack 
	*(--(ready[0]->stack_pointer)) = 0;		// SS	exactly is DS
	*(--(ready[0]->stack_pointer)) = ready[0]->stack_pointer+1; // ESP
	*(--(ready[0]->stack_pointer)) = EFLAGS_IF | EFLAGS_RESERVED;	// EFLAGS
	*(--(ready[0]->stack_pointer)) = GDT_KERNEL_CODE_SEL<<3;	// CS
	*(--(ready[0]->stack_pointer)) = (u32_t) &idle_process;	// EIP

	*(--(ready[0]->stack_pointer)) = 0;	// error code
	*(--(ready[0]->stack_pointer)) = 0;	// interrupt number

	*(--(ready[0]->stack_pointer)) = 0;	// EAX
	*(--(ready[0]->stack_pointer)) = 0;	// ECX
	*(--(ready[0]->stack_pointer)) = 0;	// EDX
	*(--(ready[0]->stack_pointer)) = 0;	// EBX
	*(--(ready[0]->stack_pointer)) = 0;	// ESP
	*(--(ready[0]->stack_pointer)) = 0;//(u32_t) current_proc->base_stack;	// EBP
	*(--(ready[0]->stack_pointer)) = 0;	// ESI
	*(--(ready[0]->stack_pointer)) = 0;	// EDI

	*(--(ready[0]->stack_pointer)) = GDT_KERNEL_DATA_SEL<<3;	// DS
	*(--(ready[0]->stack_pointer)) = GDT_KERNEL_DATA_SEL<<3;	// ES
	*(--(ready[0]->stack_pointer)) = GDT_KERNEL_DATA_SEL<<3;	// FS
	*(--(ready[0]->stack_pointer)) = GDT_KERNEL_DATA_SEL<<3;	// GS

	*(--(ready[0]->stack_pointer)) = 0;	// dummy EAX


	++ready_count;
	++proc_count;	
	
}


void scheduler_remove_thread(int proc_id)
{

	int i;

	__asm__ __volatile__ ("cli");			// disable interrupt	

//printf("I1'm here 4 removing id = %d\n",proc_id);
	if( process[proc_id].bsy_slot == 0 )
	{
//printf("I2'm here 4 removing id = %d\n",proc_id);
		__asm__ __volatile__ ("sti");		// enable interrupt
		re_schedule();
		return;	// return error code			
	}

//printf("I3'm here 4 ready_count = %d    before\n",ready_count);
	// remove it from the ready queue
	for (i=1;i<ready_count;++i)
		if( ready[i] == &process[proc_id] )
		{
//printf("matched .......\n");
			if ( i == ready_count-1)
				ready[i] = NULL;
			else
				memmove(ready+i, ready+i+1, sizeof(proc*) * (ready_count - i - 1) );
		}
//printf("I3'm here 4 ready_count = %d    after\n",ready_count);
	// free its slot
	memset(&process[proc_id], 0, sizeof(proc));

	--ready_count;
	--proc_count;

	if ( current_proc == &process[proc_id] )
		current_proc = NULL;	// NULL means don't save context in the next switch

//printf("9999999999999999999999999999999999999999999\n");
	
	__asm__ __volatile__ ("sti");		// enable interrupt


//printf("00000000000000000000000000000000\n");
	re_schedule();

	
}


void init_sched()
{
	memset(process, 0, sizeof(proc) * MAX_PROCESS);
	memset(ready, 0, sizeof(proc*) * MAX_PROCESS);
	memset(blocked, 0, sizeof(proc*) * MAX_PROCESS);
	
	init_system_idle_process();
	
	blocked_count= 0;
	
}

void block_current_process_mutex(mutex_t* mutex)
{
	current_proc->status = PROCESS_BLOCKED_BY_MUTEX;

	current_proc->locked_mutex = mutex;

	re_schedule();
	
}

void notify_waiting_processess_mutex(mutex_t* mutex)
{

	current_proc->status = PROCESS_NOTIFY_BY_MUTEX;

	current_proc->locked_mutex = mutex;
//printf("inside here notify_waiting_processess_mutex \n");

	re_schedule();

}



void block_current_process_sleep(u32_t num_of_ticks)
{

	current_proc->status = PROCESS_BLOCKED_BY_WAIT_TIME;

	current_proc->wait_ticks = num_of_ticks;

	re_schedule();
	
}



void block_current_process_keyboard_input()
{

	current_proc->status = PROCESS_BLOCKED_BY_WAIT_KB_INPUT;

	re_schedule();
	
}


void notify_waiting_processess_keyboard_input()
{
	int p;

	while ( (p = process_blocked_by_kb_input() ) != -1 )
		blocked_to_ready(p);
	
}


void block_current_process_waiting_process_to_finish(int proc_id)
{

//printf("blocking current process id = %d waiting for id = %d\n", current_proc->proc_id, proc_id);
	current_proc->status = PROCESS_BLOCKED_BY_WAIT_PROC_TO_FINISH;

	current_proc->wait_proc_id = proc_id;

	re_schedule();
	
}


void notify_waiting_process_for_process_finish(int proc_id)
{
	int p;

	while ( (p = process_blocked_process_to_finish(proc_id) ) != -1 )
		blocked_to_ready(p);
	
}

/*
void sched_trace()
{
printf("b-still [%d] block count = %d --- ready count = %d  \n", current_proc->proc_id, blocked_count, ready_count);
}
*/
