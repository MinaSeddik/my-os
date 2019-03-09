#ifndef _MEMORY_H
#define _MEMORY_H

#include <types.h>

#define KERNAL_START	&start_at
#define KERNAL_END	&end

#define MEMORY_START	(size_t) ( KERNAL_END + 1 )
#define MEMORY_LIMIT	memory_size * 1024

extern u32_t start_at;		// kernel start at
extern u32_t end;		// kernel end at
extern u32_t memory_size;	// memory size in KB



typedef struct memory_block
{
	
  	struct memory_block* prev; 	/* Address of the previous free block 	*/
  	struct memory_block* next; 	/* Address of the next free block 	*/
	size_t size;     		/* Size of the block */
} __attribute__((packed)) memory_block_t;


void memory_install();

void* malloc(size_t size);

void free (void* block);

void* realloc (void* mem_block, size_t size);

void* calloc (size_t num, size_t size);


#endif
