#include <mutex.h>
#include <scheduler.h>



inline u32_t fetch_and_add( u32_t* addr)
{
	u32_t value = 1; 
        asm volatile( 
                                "lock; xaddl %%eax, %2;"
                                :"=a" (value)                   //Output
                               : "a" (value), "m" (*addr)  //Input
                               :"memory" );
        return value;
 }



void mutex_init(mutex_t* mutex)
{

	mutex->entries = 0;		
	mutex->exits = 0;

}


void lock(mutex_t* mutex)
{
	u32_t ticket = fetch_and_add( &mutex->entries );

	while (ticket != mutex->exits)
		block_current_process_mutex(mutex);
}


void unlock(mutex_t* mutex)
{
	fetch_and_add((u32_t) &mutex->exits);

	notify_waiting_processess_mutex(mutex);

}


