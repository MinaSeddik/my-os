#ifndef _MUTEX_H
#define _MUTEX_H

#include <types.h>

typedef struct _mutex
{
  u32_t entries; 	/* Number of processes that have held/waited for the mutex */
  u32_t exits;   	/* Number of processes that have released the mutex */
} mutex_t;


void mutex_init(mutex_t* mutex);


void lock(mutex_t* mutex);
void unlock(mutex_t* mutex);



#endif
