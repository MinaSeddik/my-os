
#ifndef _NET_SYS_H
#define _NET_SYS_H


#include <types.h>
#include <mutex.h>


#define SYS_MBOX_NULL 	NULL
#define SYS_MBOX_SIZE 100


struct sys_mbox_msg 
{
  	struct sys_mbox_msg *next;
  	void *msg;
};



struct sys_mbox 
{
  	u16_t first, last;
  	void *msgs[SYS_MBOX_SIZE];
  	mutex_t *mail;
  	mutex_t *mutex;
};

//typedef sys_mbox sys_mbox_t;

// Mailbox functions. 
struct sys_mbox* sys_mbox_new(void);
void sys_mbox_post(struct sys_mbox *mbox, void*msg);
u16_t sys_arch_mbox_fetch(struct sys_mbox *mbox, void **msg, u16_t timeout);
void sys_mbox_free(struct sys_mbox *mbox);

void sys_mbox_fetch(struct sys_mbox mbox, void **msg);

#endif



