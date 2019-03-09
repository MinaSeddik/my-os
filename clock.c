#include <system.h>
#include <scheduler.h>
#include <stdio.h>

u32_t sys_seconds = 0;	// number of seconds till the system up

u32_t tick_count = 0;

extern proc*	current_proc;
extern u32_t 	blocked_count;

u32_t timer_handler(u32_t context)
{
	int p;
	tick_count++;
	sys_seconds = ( tick_count % HZ ) ? sys_seconds : sys_seconds+1;

	for (p=0;p<blocked_count;++p)
		if ( blocked[p]->status == PROCESS_BLOCKED_BY_WAIT_TIME ) 
			--blocked[p]->wait_ticks;


	while ( (p = process_blocked_by_timeout() ) != -1 )
		blocked_to_ready(p);

	if( current_proc != NULL && current_proc->proc_id != 0 && current_proc->status == PROCESS_STATUS_NORMAL)
	{
		--current_proc->ticks_assgined;
		++current_proc->total_ticks_count;
	}

	return schedule(context);	

}


void timer_install()
{
	    
	timer_init();

    	irq_install_handler(32, timer_handler);

	init_sched();
}




void timer_init()
{
	// divisor is the initial count, the clock will count down till it reachs 0, then send interrupt
	int divisor = 1193182 / HZ ;       	// calculate our divisor 
    	outportb(0x43, 0x36);             	// set command byte 0x36 
	outportb(0x40, divisor & 0xFF);   	// Set low byte of divisor 
   	outportb(0x40, divisor >> 8);     	// Set high byte of divisor 	
}

void ksleep(u32_t milli)
{
	u32_t fire_tick, num_of_ticks = milli / 1000 * HZ;

	fire_tick = tick_count + num_of_ticks;

	while(fire_tick > tick_count);	// bussy wait for kernel sleep
}

void sleep(u32_t milli)
{

	u32_t num_of_ticks = milli / 1000 * HZ;
	
	if ( num_of_ticks == 0 )	return;

	block_current_process_sleep(num_of_ticks);

}

u32_t gettickcount()
{
	// get tickets count in milli-second
	return tick_count / HZ * 1000;
}
