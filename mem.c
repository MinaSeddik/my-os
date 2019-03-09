
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <mutex.h>
#include <stats.h>


#define MEM_ALIGNMENT	2

extern netstats stats;
extern int nic_irq;

static mem *lfree;   /* pointer to the lowest free block */

//static mutex_t pbuf_mem_mutex;

static mem *ram_end;
static u8_t ram[MEM_SIZE + sizeof(mem)];

static void plug_holes(mem *memory)
{
	mem* nmem;
  	mem* pmem;

	//ASSERT("plug_holes: mem >= ram", (u8_t *)mem >= ram);
  	//ASSERT("plug_holes: mem < ram_end", (u8_t *)mem < (u8_t *)ram_end);
  	//ASSERT("plug_holes: mem->used == 0", mem->used == 0);
  
  	/* plug hole forward */
  	//ASSERT("plug_holes: mem->next <= MEM_SIZE", mem->next <= MEM_SIZE);

	nmem = (mem *)&ram[memory->next];

  	if(memory != nmem && nmem->used == 0 && (u8_t *)nmem != (u8_t *)ram_end) 
	{
    		if(lfree == nmem) 
		{
      			lfree = memory;
    		}
    		memory->next = nmem->next;
    		((mem *)&ram[nmem->next])->prev = (u8_t *)memory - ram;
  	}
	
  	/* plug hole backward */
  	pmem = (mem *)&ram[memory->prev];
  	if(pmem != memory && pmem->used == 0) 
	{
    		if(lfree == memory) 
		{
      			lfree = pmem;
    		}
    		pmem->next = memory->next;
    		((mem *)&ram[memory->next])->prev = (u8_t *)pmem - ram;

  	}



}

static int fd = 0;

void mem_init(void)
{
	mem *memory;

  	memset(ram,0, MEM_SIZE);
  	memory = (mem *)ram;
  	memory->next = MEM_SIZE;
  	memory->prev = 0;
  	memory->used = 0;
  	ram_end = (mem *)&ram[MEM_SIZE];
  	ram_end->used = 1;
  	ram_end->next = MEM_SIZE;
  	ram_end->prev = MEM_SIZE;

//  	mutex_init(&pbuf_mem_mutex);

  	lfree = (mem *)ram;
//  	stats.mem.avail = MEM_SIZE;



}


void* mem_malloc(mem_size_t size)
{
	mem_size_t ptr, ptr2;
  	mem *mem1, *mem2;

  	if(size == 0) 
	{
    		return NULL;
  	}

  	/* Expand the size of the allocated memory region so that we can
     	adjust for alignment. */
  	/*if((size % MEM_ALIGNMENT) != 0) 
	{
    		size += MEM_ALIGNMENT - ((size + SIZEOF_STRUCT_MEM) % MEM_ALIGNMENT);
  	}*/
  
  	if(size > MEM_SIZE) 
	{
    		return NULL;
  	}

	// adjust alignment
  	if((size % MEM_ALIGNMENT) != 0) 
	{
    		size += MEM_ALIGNMENT - ((size + SIZEOF_STRUCT_MEM) % MEM_ALIGNMENT);
  	}

  	//lock(&pbuf_mem_mutex);
	// don't receive any packet during packet queue processing


  	for(ptr = (u8_t *)lfree - ram; ptr < MEM_SIZE; ptr = ((mem *)&ram[ptr])->next) 
	{
    		mem1 = (mem *)&ram[ptr];

    		if(!mem1->used && mem1->next - (ptr + SIZEOF_STRUCT_MEM) >= size + SIZEOF_STRUCT_MEM) 
		{

      			ptr2 = ptr + SIZEOF_STRUCT_MEM + size;
      			mem2 = (mem *)&ram[ptr2];


      			mem2->prev = ptr;      
      			mem2->next = mem1->next;
      			mem1->next = ptr2; 
     
      			if(mem2->next != MEM_SIZE) 
			{
        			((mem *)&ram[mem2->next])->prev = ptr2;
      			}
      

      			mem2->used = 0;      
      			mem1->used = 1;

      			/*if(stats.mem.max < ptr2) 
			{
        			stats.mem.max = ptr2;
      			}*/

      			if(mem1 == lfree) 
			{
				/* Find next free block after mem */
        			while(lfree->used && lfree != ram_end) 
				{
	  				lfree = (mem *)&ram[lfree->next];
        			}
       
      			}

      			//unlock(&pbuf_mem_mutex);
			// enable packet receiving again
			//enable_irq(nic_irq);
      			//ASSERT("mem_malloc: allocated memory not above ram_end.",(u32_t)mem1 + SIZEOF_STRUCT_MEM + size <= (u32_t)ram_end);
      			//ASSERT("mem_malloc: allocated memory properly aligned.",(u32_t)((u8_t *)mem1 + SIZEOF_STRUCT_MEM) % MEM_ALIGNMENT == 0);


      			return (u8_t *)mem1 + SIZEOF_STRUCT_MEM;
    		}    
  	}

  	//++stats.mem.err;
  
  	//unlock(&pbuf_mem_mutex);
	// enable packet receiving again
	//enable_irq(nic_irq);
  	
	return NULL;
}

void mem_free(void *rmem)
{
	mem *mem1;
	char log[1000];
  	if(rmem == NULL) 
	{
    		return;
  	}
  
  	//lock(&pbuf_mem_mutex);
	// don't receive any packet during packet queue processing
	disable_irq(nic_irq);


  	//ASSERT("mem_free: legal memory", (u8_t *)rmem >= (u8_t *)ram && (u8_t *)rmem < (u8_t *)ram_end);
    

  	if((u8_t *)rmem < (u8_t *)ram || (u8_t *)rmem >= (u8_t *)ram_end) 
	{
		//unlock(&pbuf_mem_mutex);
		// enable packet receiving again
		enable_irq(nic_irq);
    		return;
  	}
  	mem1 = (mem *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);

  	//ASSERT("mem_free: mem->used", mem->used);
  
  	mem1->used = 0;

  	if(mem1 < lfree) 
	{
    		lfree = mem1;
  	}
  
  	plug_holes(mem1);
 
	//unlock(&pbuf_mem_mutex);

	// enable packet receiving again
	enable_irq(nic_irq);
}


void* mem_realloc(void *rmem, mem_size_t newsize)
{
	mem_size_t size;
  	mem_size_t ptr, ptr2;
  	mem *mem1, *mem2;
  
  	//lock(&pbuf_mem_mutex);
	// don't receive any packet during packet queue processing
	disable_irq(nic_irq);
  
  	//ASSERT("mem_realloc: legal memory", (u8_t *)rmem >= (u8_t *)ram && (u8_t *)rmem < (u8_t *)ram_end);
  
  	if((u8_t *)rmem < (u8_t *)ram || (u8_t *)rmem >= (u8_t *)ram_end) 
	{
    		printf("mem_free: illegal memory\n");
		//unlock(&pbuf_mem_mutex);
		// enable packet receiving again
		enable_irq(nic_irq);
    		return rmem;
  	}
  	mem1 = (mem *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);

  	ptr = (u8_t *)mem1 - ram;

  	size = mem1->next - ptr - SIZEOF_STRUCT_MEM;

  	//stats.mem.used -= (size - newsize);

  	if(newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size) 
	{
    		ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
    		mem2 = (mem *)&ram[ptr2];
    		mem2->used = 0;
    		mem2->next = mem1->next;
    		mem2->prev = ptr;
    		mem1->next = ptr2;
    		if(mem2->next != MEM_SIZE) 
		{
      			((mem *)&ram[mem2->next])->prev = ptr2;
    		}

    		plug_holes(mem2);
  	}
  
	//unlock(&pbuf_mem_mutex);  
	// enable packet receiving again
	enable_irq(nic_irq);
  
	return rmem;
}
