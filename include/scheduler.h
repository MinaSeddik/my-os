#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <types.h>
#include <thread.h>
#include <mutex.h>

#define PROCESS_STATUS_NORMAL			0x00

#define PROCESS_BLOCKED_BY_MUTEX		0x01
#define PROCESS_BLOCKED_BY_WAIT_TIME		0x02
#define PROCESS_BLOCKED_BY_WAIT_KB_INPUT	0x03
#define PROCESS_BLOCKED_BY_WAIT_PROC_TO_FINISH	0x04

#define PROCESS_NOTIFY_BY_MUTEX			0x10
#define PROCESS_NOTIFY_BY_WAIT_TIME		0x11




void init_sched();

void block_current_process_mutex();
void notify_waiting_processess_mutex();
void block_current_process_sleep(u32_t num_of_ticks);

void scheduler_add_thread (THandler th_handler);
void init_system_idle_process();

u32_t schedule(u32_t context);
void re_schedule();
void swich_to_next_process();
void blocked_to_ready(int index);

int process_blocked_by_mutux(mutex_t* mutex);
void notify_waiting_processess_mutex(mutex_t* mutex);

int process_blocked_by_timeout();

void block_current_process_keyboard_input();
void notify_waiting_processess_keyboard_input();

void block_current_process_waiting_process_to_finish(int proc_id);
void notify_waiting_process_for_process_finish(int proc_id);
#endif
