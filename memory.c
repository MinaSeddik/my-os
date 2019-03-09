
#include <memory.h>
#include <string.h>
#include <mutex.h>
#include <stdio.h>


#define ADDR(x)		(size_t) x


memory_block_t* first_free;
memory_block_t* first_allocated;

//int alloc = 0;
//int freee = 0;

mutex_t mem_mutex;

void dump_memory();
void dump_alloc_memory();


void dump_memory_ex();
void dump_alloc_memory_ex();

void debug_case()
{
	memory_block_t* tmp = first_free;

	while (tmp != NULL )
	{
		if ( tmp->next == tmp->prev && tmp->next != NULL )
		{
			printf("BUG ****** memory management ... \n");
			dump_memory();
			while(1);	
		}
		tmp = tmp->next;
		//++i;
	}
}

void dump_memory_ex()
{
	memory_block_t* tmp = first_free;
	int i = 1;

	//printf("\nFirst Free = 0x%x = %d\n", first_free, first_free);

	printf("Free list : ");
	while (tmp != NULL )
	{
		//printf("\nBlock %d adress : 0x%x = %d, Block size = %d", i, tmp, tmp, tmp->size);
		//printf("\nBlock %d prev adress : 0x%x = %d, next adress : 0x%x = %d", i, tmp->prev, tmp->prev, tmp->next, tmp->next);
		//printf("\n");
		printf(" %d, " , tmp->size);
		tmp = tmp->next;
		//++i;
	}
	printf("\n");
}

void dump_alloc_memory_ex()
{
	memory_block_t* tmp = first_allocated;
	int i = 1;

	//printf("\nFirst Allocated = 0x%x = %d\n", first_allocated, first_allocated);

	printf("Allocated list : ");
	while (tmp != NULL )
	{
		//printf("\nBlock %d adress : 0x%x = %d, Block size = %d", i, tmp, tmp, tmp->size);
		//printf("\nBlock %d prev adress : 0x%x = %d, next adress : 0x%x = %d", i, tmp->prev, tmp->prev, tmp->next, tmp->next);
		//printf("\n");
		printf(" %d, " , tmp->size);
		tmp = tmp->next;
		//++i;
	}	
	printf("\n");
}


void dump_memory()
{
	memory_block_t* tmp = first_free;
	int i = 1;

	printf("\nFirst Free = 0x%x = %d\n", first_free, first_free);

	while (tmp != NULL )
	{
		printf("Block %d adress : 0x%x = %d, Block size = %d", i, tmp, tmp, tmp->size);
		printf("\nBlock %d prev adress : 0x%x = %d, next adress : 0x%x = %d\n", i, tmp->prev, tmp->prev, tmp->next, tmp->next);
		//printf("\n");
		tmp = tmp->next;
		++i;
//if ( i== 3)break;
	}
}

void dump_alloc_memory()
{
	memory_block_t* tmp = first_allocated;
	int i = 1;

	printf("\nFirst Allocated = 0x%x = %d\n", first_allocated, first_allocated);

	while (tmp != NULL )
	{
		printf("Block %d adress : 0x%x = %d, Block size = %d", i, tmp, tmp, tmp->size);
		printf("\nBlock %d prev adress : 0x%x = %d, next adress : 0x%x = %d\n", i, tmp->prev, tmp->prev, tmp->next, tmp->next);
		tmp = tmp->next;
		++i;
	

	}	
}

void memory_install()
{
	
//	printf("\nkernel starts at = 0x%x \n", (size_t) KERNAL_START );
//	printf("kernel ends at = 0x%x \n", (size_t) KERNAL_END );
	
//	printf("Free Memory starts at 0x%x \n", MEMORY_START );
//	printf("Free Memory Size = %d MB\n", (ADDR(MEMORY_LIMIT) - ADDR(MEMORY_START)) / (1024 * 1024) );

	first_free = (memory_block_t* ) MEMORY_START;
	memset(first_free, 0, sizeof(memory_block_t));
	first_allocated = NULL;

	first_free->size = ( ADDR(MEMORY_LIMIT) - ADDR(MEMORY_START) );

	mutex_init(&mem_mutex);	
}


void* malloc(size_t size)
{
	void* block = NULL;
	memory_block_t* tmp = NULL, *next_free = NULL, *next_alloc = NULL;
//printf("malloc here 000 size = %d\n", size);

  	if (size == 0)		// allocate 0-size block
      		return block;


//printf("malloc debug on enter size = %d\n", size);
//debug_case();



//printf("malloc here 000333 \n");
	lock(&mem_mutex);
//printf("malloc here 000444 tmp = ox%x\n", first_free);


/*
if(alloc == 16 )
{
	dump_memory();

//	dump_alloc_memory();
	
	while(1);
}
*/

	tmp = first_free;

//int d1, d2, d3, d4, d5;
//d1 = d2 = d3 = d4 = d5 = 0;
	while ( tmp != NULL )
	{
		if ( tmp->size >= size )
		{

			block = (void *) ( ADDR(tmp) + sizeof(memory_block_t) );

			if ( tmp->size <= size + sizeof(memory_block_t) )		// take the whole block
			{	
//printf("here 1\n");
//d1= 1;
			//	tmp->size = size + sizeof(memory_block_t);	// bug
				
				// remove it from free list			
				if ( tmp->prev == NULL )
					first_free = tmp->next;
				else
					tmp->prev->next = tmp->next;
				if ( tmp->next != NULL )
					tmp->next->prev = tmp->prev;
				
						
			} 	
			else				// take a part of this block
			{

//printf("here 2 tmp = %d\n", tmp->size); 
//printf("here 2 t_prev = %x  t_next = %x\n", tmp->prev, tmp->next); 
				next_free = (memory_block_t *) ( ADDR(tmp) + sizeof(memory_block_t) + size) ;
				next_free->size = tmp->size - ( size + sizeof(memory_block_t) );
//printf("here 2 val = %d\n", next_free->size); 
				next_free->prev = tmp->prev;
				next_free->next = tmp->next;

				tmp->size = size;
//printf("here 2 val = %d\n", next_free->size); 
				// remove it from free list
				if ( next_free->prev == NULL )
					first_free = next_free;
				else
					next_free->prev->next = next_free;
//printf("here 2 val = %d\n", next_free->size); 
//printf("here 2 prev = %x  next = %x\n", next_free->prev, next_free->next); 
				if ( next_free->next != NULL )
					next_free->next->prev = next_free;	
//dump_memory();				
			}
			
			// append to allocated blocks
			if ( first_allocated == NULL )
			{
//printf("here 3\n");
//d3= 1;
				first_allocated = tmp;
				first_allocated->prev = NULL;
				first_allocated->next = NULL;
			}
			else if( first_allocated > tmp ) 
			{
//printf("here 4\n");
//d4= 1;
				tmp->prev = NULL;
				tmp->next = first_allocated;

				first_allocated->prev = tmp;
		 
				first_allocated = tmp;
			}
			else
			{
//printf("here 5\n");
//d5= 1;
				next_alloc = first_allocated;
				while ( next_alloc != NULL )
				{
					if ( next_alloc->next == NULL || next_alloc->next >= tmp )	break;
//printf("malloc here 5 next_alloc = %x\n", next_alloc );
					next_alloc = next_alloc->next;
				}
//printf("*** 1 malloc here 5 next_alloc = %x, size = %d\n", next_alloc, next_alloc->size );
				tmp->prev = next_alloc;
//printf("*** 2 malloc here 5 next_alloc = %x, size = %d\n", next_alloc, next_alloc->size );
				tmp->next = next_alloc->next;
//printf("*** 3 malloc here 5 next_alloc = %x, size = %d\n", next_alloc, next_alloc->size );
				if ( next_alloc->next ) next_alloc->next->prev = tmp;
//printf("*** 4 malloc here 5 next_alloc = %x, size = %d\n", next_alloc, next_alloc->size );
				next_alloc->next = tmp;
//printf("*** 5 malloc here 5 next_alloc = %x, size = %d\n", next_alloc, next_alloc->size );
			}
	
			break;
		}
		else
			tmp = tmp->next;
	}

//printf("before unlock\n");
	unlock(&mem_mutex);
//printf("after unlock\n");
 
	return block;
}


void* realloc(void* mem_block, size_t size)
{
	void* block = NULL;
	memory_block_t *this_block, *tmp = NULL, *next_free = NULL;
	size_t total_size;
	memory_block_t *tmp_next, *tmp_prev;

  	if (mem_block == NULL)		// if there is NO block allocated
      		return malloc (size);
	
  	if (size == 0)		// allocate 0-size block
	{
		free(mem_block);
      		return block;
	}
	
	lock(&mem_mutex);

	this_block = (memory_block_t* ) ( ADDR(mem_block) - sizeof(memory_block_t) );

	if ( this_block->size >= size )		// if the required size less than or equal the size of allocated block
	{
		block = mem_block;		// no need to re-allocate
	}
	else
	{

		tmp = first_free;
		
		// see if there is a contiguas block before or after the given block
		while ( tmp != NULL )
		{
			if ( ADDR(tmp) + tmp->size + sizeof(memory_block_t) == ADDR(this_block) && tmp->size + sizeof(memory_block_t) + this_block->size >= size )
			{
				block = (void *) ( ADDR(tmp) + sizeof(memory_block_t) );

				tmp_prev = tmp->prev;
				tmp_next = tmp->next;

				// put tmp in allocated list
				tmp->prev = this_block->prev;
				tmp->next = this_block->next;
				if ( first_allocated == this_block ) 
					first_allocated = tmp;
				if ( tmp->next != NULL )
					tmp->next->prev = tmp;

				// total size
				total_size = tmp->size + sizeof(memory_block_t) + this_block->size;
			
				if ( size + sizeof(memory_block_t) >= total_size)	// allocate whole the block
				{
					tmp->size = total_size;
				
					// remove it from free list			
					if ( tmp_prev == NULL )
						first_free = tmp_next;
					else
						tmp_prev->next = tmp_next;
					if ( tmp_next != NULL )
						tmp_next->prev = tmp_prev;

					// copy the data of the block
					memmove( (void *) ( ADDR(tmp) + sizeof(memory_block_t) ), (void *) ( ADDR(this_block) + sizeof(memory_block_t) ), this_block->size);
				}
				else				// allocate part of the block
				{
					tmp->size = size;
				
					// copy the data of the block
					memmove( (void *) ( ADDR(tmp) + sizeof(memory_block_t) ), (void *) ( ADDR(this_block) + sizeof(memory_block_t) ), this_block->size);

					next_free = (memory_block_t *) ( ADDR(tmp) + sizeof(memory_block_t) + tmp->size );

					// set size and next, prev of the next_free node
					next_free->size = total_size - ( tmp->size + sizeof(memory_block_t) );
					next_free->prev = tmp_prev;
					next_free->next = tmp_next;

					// remove it from free list
					if ( next_free->prev == NULL )
						first_free = next_free;
					else
						next_free->prev->next = next_free;
					if ( next_free->next != NULL )
						next_free->next->prev = next_free;	
						
				}


				break;
			}
			else if ( ADDR(this_block) + this_block->size + sizeof(memory_block_t) == ADDR(tmp) && this_block->size + sizeof(memory_block_t) + tmp->size >= size )
			{

				block = mem_block;

				tmp_prev = tmp->prev;
				tmp_next = tmp->next;

				// total size
				total_size = this_block->size + sizeof(memory_block_t) + tmp->size;
				
				if ( size + sizeof(memory_block_t) >= total_size)	// allocate whole the block
				{
					this_block->size = total_size;

					// remove it from free list			
					if ( tmp->prev == NULL )
						first_free = tmp->next;
					else
						tmp->prev->next = tmp->next;
					if ( tmp->next != NULL )
						tmp->next->prev = tmp->prev;

				}
				else		// allocate part the block
				{
					this_block->size = size;
					next_free = (memory_block_t *) ( ADDR(this_block) + sizeof(memory_block_t) + this_block->size );	
									
					// set size and next, prev of the next_free node
					next_free->size = total_size - ( this_block->size + sizeof(memory_block_t) );
					next_free->prev = tmp_prev;
					next_free->next = tmp_next;

					// remove it from free list
					if ( next_free->prev == NULL )
						first_free = next_free;
					else
						next_free->prev->next = next_free;
					if ( next_free->next != NULL )
						next_free->next->prev = next_free;

				}

				break;
			}
			tmp = tmp->next;
		}	// end while loop

		// if there is NO contigous block
		if ( block == NULL )
		{
			unlock(&mem_mutex);
			block = malloc(size);
			lock(&mem_mutex);

			if ( block != NULL )
				memcpy(block, mem_block, this_block->size);

			unlock(&mem_mutex);
			free(mem_block);
			lock(&mem_mutex);	
		}

	}

	unlock(&mem_mutex);

	return block;

}


void* calloc(size_t num, size_t size)
{
	void* block = NULL;
  
  	block = malloc (num * size);
  
  	if (block != NULL)
    		memset ((u8_t*)block, 0, num * size);
  
	return block;		
}


void free(void* block)
{
	//if ( 1) return;
	memory_block_t *this_block, *tmp = NULL;

	if ( block == NULL || first_allocated == NULL )	return;
/*
printf("free debug on enter block = ox%x\n", block);
debug_case();

if ( block == 0x11d416 )
{
	dump_memory();
	
}
*/

	lock(&mem_mutex);

	this_block = (memory_block_t *) ( ADDR(block) - sizeof(memory_block_t) );

//printf("free here 111 block %x , this_block %x, size = %d\n",block , this_block ,this_block->size);

//printf("free here 111 this_block->size %x , this_block->next %x\n",this_block->size , this_block->next );

	// remove from allocated list
	if( first_allocated == this_block ) 
	{
//printf("free here 222");
		first_allocated = first_allocated->next;
		first_allocated->prev = NULL;		
	}
	else
	{
//printf("free here 333");
		tmp = first_allocated;
		while ( tmp != NULL )
		{
//printf("free here 333 ***\n");
			if ( tmp->next == NULL || tmp->next >= this_block )	break;
			tmp = tmp->next;
		}
//printf("free tmp->next = %x ,this_block->next = %x tmp = %x this_block = %x\n", tmp->next, this_block->next, tmp, this_block );
//printf("free this_block->prev = %x ,this_block->size = %x\n", this_block->prev, this_block->size );
//printf("free block = %x \n", block );
		tmp->next = this_block->next;
//printf("free here 333 *** ////2\n");
		if ( this_block->next )		this_block->next->prev = tmp;
//printf("free tmp->next = %x ,this_block->next = %x\n", tmp->next, this_block->next );

//printf("free here 333 *** ////3\n");
	}


	if ( first_free == NULL )
	{
//printf("free here 444");
		first_free = this_block;
		this_block->prev = this_block->next = NULL;
	}
	else if( first_free > this_block )
	{
//printf("free here 555");
		this_block->prev = NULL;
		this_block->next = first_free;

		first_free->prev = this_block;
		 
		first_free = this_block;
	}
	else
	{
//printf("free here 666");
		tmp = first_free;
		while ( tmp != NULL )
		{
//printf("--------");
			if ( tmp->next == NULL || tmp->next >= this_block )	break;
			tmp = tmp->next;
		}

		this_block->prev = tmp;
		this_block->next = tmp->next;
		
		if ( tmp->next )	tmp->next->prev = this_block;

		tmp->next = this_block;
	}
/*
if ( block == 0x13b210 )
{
	dump_memory();

printf("free first_free = 0x%x  this_block = 0x%x\n", first_free, this_block);
//	while(1);
}*/

//int test_count = 0;
	// merging
	tmp = first_free;
	while( tmp != NULL )
	{

//printf("******");
		if ( ADDR(tmp) + sizeof(memory_block_t) + tmp->size == ADDR(this_block) )
		{
//printf("free here 777  tmp->next = 0x%x\n", tmp->next);
			tmp->size+= sizeof(memory_block_t) + this_block->size;	
			tmp->next = this_block->next;

			if ( tmp->next )	tmp->next->prev = tmp;

			this_block = tmp;

		}
		else if ( ADDR(this_block) + sizeof(memory_block_t) + this_block->size == ADDR(tmp) )
		{
//printf("free here 888\n");
			this_block->size+= sizeof(memory_block_t) + tmp->size;					
			this_block->next = tmp->next;

			if ( tmp->next )	tmp->next->prev = this_block;

	//		tmp = tmp->next;

		}
		tmp = tmp->next;
	}
/*
if ( block == 0x13b210 )
{
	dump_memory();

printf("free first_free = 0x%x  this_block = 0x%x\n", first_free, this_block);
	while(1);
}
*/
	unlock(&mem_mutex);
/*
if ( block == 0x13b210 )
{

printf("free tmp = 0x%x \n", tmp);
 while(1);
}
*/
//freee++;

//if(freee == 5 )
//{
//printf("dump free...\n");
//	dump_memory();

//	dump_alloc_memory();
	
//	while(1);
//}






//dump_memory_ex();

//dump_alloc_memory_ex();
//printf("free here                         101010\n");
//printf("free debug before exit\n");
//debug_case();
	//printf("free here exit\n");

}
