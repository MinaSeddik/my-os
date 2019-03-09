
#include <stdio.h>
#include <net_sys.h>
#include <mutex.h>
#include <err.h>
#include <stats.h>



struct sys_mbox* sys_mbox_new()
{
  	struct sys_mbox *mbox;

  	mbox = malloc(sizeof(struct sys_mbox));
  	mbox->first = mbox->last = 0;
  	mbox->mail = (mutex_t* ) malloc(sizeof(mutex_t));
  	mbox->mutex = (mutex_t* ) malloc(sizeof(mutex_t));

	memset(mbox->mail, 0, sizeof(mutex_t));
	memset(mbox->mutex, 0, sizeof(mutex_t));


	mutex_init(mbox->mail);
	mutex_init(mbox->mutex);
  
#ifdef SYS_STATS
  stats.sys.mbox.used++;
  if(stats.sys.mbox.used > stats.sys.mbox.max) {
    stats.sys.mbox.max = stats.sys.mbox.used;
  }
#endif 
  
  	return mbox;
}


/*-----------------------------------------------------------------------------------*/
void sys_mbox_free(struct sys_mbox *mbox)
{
  	if(mbox != SYS_MBOX_NULL) 
	{

#ifdef SYS_STATS
    stats.sys.mbox.used--;
#endif // SYS_STATS 

    		//sys_sem_wait(mbox->mutex);		//???
    
    		free(mbox->mail);
    		free(mbox->mutex);
    
		mbox->mail = mbox->mutex = NULL;
    
		//  DEBUGF("sys_mbox_free: mbox 0x%lx\n", mbox);
    		free(mbox);
  	}
}
/*-----------------------------------------------------------------------------------*/
void sys_mbox_post(struct sys_mbox *mbox, void *msg)
{
  	u8_t first;
  
  	lock(mbox->mutex);
  
  	printf("sys_mbox_post: mbox %p msg %p\n", mbox, msg);

  	mbox->msgs[mbox->last] = msg;

  	if(mbox->last == mbox->first) 
	{
    		first = 1;
  	} 
	else 
	{
    		first = 0;
  	}
  
  	mbox->last++;
  
	if(mbox->last == SYS_MBOX_SIZE) 
	{
    		mbox->last = 0;
  	}

/*
  	if(first) 
	{
    		sys_sem_signal(mbox->mail);
  	}
*/  
  	unlock(mbox->mutex);

}
/*-----------------------------------------------------------------------------------*/
u16_t sys_arch_mbox_fetch(struct sys_mbox *mbox, void **msg, u16_t timeout)
{
  	u16_t time = 1;
  
  	// The mutex lock is quick so we don't bother with the timeout stuff here. 
  	//sys_arch_sem_wait(mbox->mutex, 0);

	lock(mbox->mutex);

	if(mbox->first == mbox->last)
	{
		*msg = NULL;
		time = 0;
	}
	else
	{
		*msg = mbox->msgs[mbox->first];
		
		mbox->first++;
  		if(mbox->first == SYS_MBOX_SIZE) 
		{
    			mbox->first = 0;
  		}	 
	}


	unlock(mbox->mutex);

  	return time;
}

/*-----------------------------------------------------------------------------------*/
u16_t sys_mbox_fetch(struct sys_mbox mbox, void **msg)
{
  	u16_t time = 1;
  
  	// The mutex lock is quick so we don't bother with the timeout stuff here. 
  	//sys_arch_sem_wait(mbox->mutex, 0);

	lock(mbox->mutex);

	if(mbox->first == mbox->last)
	{
		*msg = NULL;
		time = 0;
	}
	else
	{
		*msg = mbox->msgs[mbox->first];
		
		mbox->first++;
  		if(mbox->first == SYS_MBOX_SIZE) 
		{
    			mbox->first = 0;
  		}	 
	}


	unlock(mbox->mutex);

  	return time;
}
