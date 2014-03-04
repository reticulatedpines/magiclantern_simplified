/**
 * Memory allocation routine that decides automatically which pool to use (according to free space)
 * 
 * Implementation based on memcheck
 */

#ifndef _mem_h_
#define _mem_h_

/* this file needs to get included in dryos.h to replace old malloc / AllocateMemory calls */

/* not used; will be replaced by macros in order to get calling context (file and line) */
extern void * malloc( size_t len );
extern void free( void * buf );

/* in posix.c */
extern void * realloc( void * buf, size_t newlen );

/* not to be called directly (only via macros) */
extern void * __mem_malloc( size_t len, unsigned int flags, const char *file, unsigned int line);
extern void __mem_free( void * buf);

/* flags */
#define MEM_DMA       1 /* require uncacheable (DMA) memory (e.g. for file I/O) */
#define MEM_TEMPORARY 2 /* this memory will be freed quickly (e.g. to use shoot_malloc, that must be free when changing some Canon settings) */

/* this may be reused by other code */
/* warning: not thread safe (but it's OK to use it in menu) */
const char * format_memory_size( unsigned size); /* e.g. 2.0GB, 32MB, 2.4kB... */

#ifndef NO_MALLOC_REDIRECT

#define malloc(len)         __mem_malloc(len, 0, __FILE__, __LINE__)
#define free(buf)           __mem_free(buf)

#define AllocateMemory      please use malloc
#define FreeMemory          please use free

#define SmallAlloc          please use malloc
#define SmallFree           please use free

#define shoot_malloc(len)   please use malloc, fio_malloc or tmp_malloc
#define shoot_free          please use free, fio_free or tmp_free

#define alloc_dma_memory(len) please use fio_malloc
#define free_dma_memory     please use fio_free

/* allocate temporary memory that will be freed shortly after using it */
#define tmp_malloc(len)       __mem_malloc(len, MEM_TEMPORARY, __FILE__, __LINE__)
#define tmp_free            free

/* allocate temporary memory for reading files */
#define fio_malloc(len)     __mem_malloc(len, MEM_TEMPORARY | MEM_DMA, __FILE__, __LINE__)
#define fio_free            free

#endif

#endif
