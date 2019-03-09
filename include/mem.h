#ifndef MEM_H
#define MEM_H

#include <types.h>


#define MEM_SIZE 	20 * 1024 * 1024		// 2 MB

#if MEM_SIZE > 64000l
typedef u32_t mem_size_t;
#else
typedef u16_t mem_size_t;
#endif /* MEM_SIZE > 64000 */

//#define MEM_ALIGN_SIZE(size) (size + \
                             ((((size) % MEM_ALIGNMENT) == 0)? 0 : \
                             (MEM_ALIGNMENT - ((size) % MEM_ALIGNMENT))))

//#define MEM_ALIGN(addr) (void *)MEM_ALIGN_SIZE((u32_t)addr)


typedef struct _mem 
{
	mem_size_t next, prev;
  	u16_t used;
} __attribute__((packed)) mem;

#define SIZEOF_STRUCT_MEM 	sizeof(mem)
#define MIN_SIZE 12
//#define SIZEOF_STRUCT_MEM 	MEM_ALIGN_SIZE(sizeof(struct mem))
/*#define SIZEOF_STRUCT_MEM (sizeof(struct mem) + \
                          (((sizeof(struct mem) % MEM_ALIGNMENT) == 0)? 0 : \
                          (4 - (sizeof(struct mem) % MEM_ALIGNMENT))))*/






void mem_init(void);

void *mem_malloc(mem_size_t size);
void mem_free(void *mem);
void *mem_realloc(void *mem, mem_size_t size);

#endif 

