/**
 * Memory allocation routine that decides automatically which pool to use (according to free space)
 * 
 * Implementation based on memcheck
 */

#ifndef _mem_h_
#define _mem_h_

/* this file needs to get included in dryos.h to replace old malloc / AllocateMemory calls */

extern void * __mem_malloc( size_t len, unsigned int flags, const char *file, unsigned int line);
extern void __mem_free( void * buf);

/* flags */
#define MEM_DMA       1 /* require uncacheable (DMA) memory (e.g. for file I/O) */
#define MEM_TEMPORARY 2 /* this memory will be freed quickly (e.g. to use shoot_malloc, that must be free when changing some Canon settings) */

/* this may be reused by other code */
/* warning: not thread safe (but it's OK to use it in menu) */
const char * format_memory_size( unsigned size); /* e.g. 2.0GB, 32MB, 2.4kB... */

/* TODO: at some point we will probably want to replace all these calls
 * with plain malloc/free, fio_malloc/fio_free and tmp_malloc/tmp_free
 */
#ifndef NO_MALLOC_REDIRECT

#define malloc(len)         __mem_malloc(len, 0, __FILE__, __LINE__)
#define free(buf)           __mem_free(buf)

#define AllocateMemory      malloc
#define FreeMemory          free

#define SmallAlloc          malloc
#define SmallFree           free

#define shoot_malloc(len)   __mem_malloc(len, MEM_TEMPORARY | MEM_DMA, __FILE__, __LINE__)
#define shoot_free          free

#define alloc_dma_memory(len) __mem_malloc(len, MEM_DMA, __FILE__,__LINE__)
#define free_dma_memory     free

/* allocate temporary memory that will be freed shortly after using it */
#define tmp_malloc(len)       __mem_malloc(len, MEM_TEMPORARY, __FILE__, __LINE__)
#define tmp_free            free

/* allocate temporary memory for reading files */
#define fio_malloc(len)     __mem_malloc(len, MEM_TEMPORARY | MEM_DMA, __FILE__, __LINE__)
#define fio_free            free

#endif

#endif
