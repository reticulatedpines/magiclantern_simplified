/**
 * Memory allocation routine that decides automatically which pool to use (according to free space)
 * 
 * Implementation based on memcheck
 */

#ifndef _mem_h_
#define _mem_h_

#include <stddef.h>
#include <stdint.h>

// structure used for maintaining a memory pool,
// DryOS kernel version.  malloc_info() calls get_malloc_info(),
// passing in the user-level struct, to copy out data from
// this kernel-level struct.
#if defined(CONFIG_MALLOC_STRUCT_V1)
struct malloc_status
{
    // I'm not sure this is part of the struct,
    // might just be padding to make the way MALLOC_STRUCT_ADDR
    // was chosen work.
    uint32_t unk_01;
    uint32_t unk_02;
    uint32_t unk_03;
    uint32_t unk_04;
    uint32_t unk_05;
    uint32_t unk_06;
    // These parts are really in the struct:
    uint32_t start_addr;
    uint32_t total_size_external; // only one total size compared to V2
    uint32_t used_size;
    uint32_t used_peak;
    uint32_t used_count;
    uint32_t free_size;
    uint32_t free_block_max_size;
    uint32_t free_block_count;
};
SIZE_CHECK_STRUCT(malloc_status, (14 * 4));
#elif defined(CONFIG_MALLOC_STRUCT_V2)
struct malloc_status
{
    uint32_t total_size_internal; // Actual size used by the pool.
    void *next_block; // Linked list start, block struct not determined.
    uint32_t total_size_external; // Probably something like size available to allocate,
                                  // accounting for overhead.  In practice this seems
                                  // to be 8 less than total_size_internal on cams I checked.
    void *next_free_block; // Linked list start, block struct not determined.
    uint32_t unk_01; // Might be block alignment related.
    uint32_t unk_02; // Some kind of flags controlling pool behaviour.
                     // I've seen this set to 0x2, that bit seems to control whether allocations
                     // should adjust the "used" fields or not.  Other bits unknown.
    uint32_t used_peak; // Internally these 3 are called "Allocated" peak, size, count,
    uint32_t used_size; // but used is clearer and shorter.
    uint32_t used_count;//
};
SIZE_CHECK_STRUCT(malloc_status, (9 * 4));
#else
    #ifdef FW_VERSION // slight hack, the intent is to check
                      // if we're building in a cam context, modules
                      // also include this file and the error is not relevant there.
        #error "You must determine which malloc struct version to use, and define it in internals.h"
    #endif
#endif

/* this file needs to get included in dryos.h to replace old malloc / AllocateMemory calls */

/* not used; will be replaced by macros in order to get calling context (file and line) */
extern void *malloc(size_t len);
extern void free(void *buf);

/* not to be called directly (only via macros) */
extern void *__mem_malloc(size_t len, unsigned int flags, const char *file, unsigned int line);
extern void __mem_free(void *buf);

/* flags */
#define MEM_DMA       1 /* require uncacheable (DMA) memory (e.g. for file I/O) */
#define MEM_TEMPORARY 2 /* this memory will be freed quickly (e.g. to use shoot_malloc, that must be free when changing some Canon settings) */
#define MEM_SRM       4 /* prefer the SRM job memory allocator (take care) */

/* this may be reused by other code */
/* warning: not thread safe (but it's OK to use it in menu) */
const char *format_memory_size(uint64_t size); /* e.g. 2.0GB, 32MB, 2.4kB... */

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

/* allocate temporary memory for reading files or for DMA operations */
/* (very large buffers will prefer SRM, smaller ones will use shoot_malloc / alloc_dma_memory) */
#define fio_malloc(len)     __mem_malloc(len, (len > 20*1024*1024 ? MEM_SRM : MEM_TEMPORARY) | MEM_DMA, __FILE__, __LINE__)
#define fio_free            free

/* allocate from SRM job buffer (for very large chunks) */
#define srm_malloc(len)     __mem_malloc(len, MEM_SRM | MEM_DMA, __FILE__, __LINE__)
#define srm_free            free

#endif

/* initialization */
void _mem_init();


/* general-purpose memory-related routines (not routed through the backend) */
/* ======================================================================== */

/* in posix.c */
extern void *realloc(void *buf, size_t newlen);
extern void *calloc(size_t nmemb, size_t size);

#define IS_ML_PTR(val) (((uintptr_t)(val) > (uintptr_t)0x1000) && ((uintptr_t)(val) < (uintptr_t)0x20000000))
#if defined(CONFIG_DIGIC_2345)
    #define IS_ROM_PTR(val) ((uintptr_t)(val) > (uintptr_t)0xF0000000)
#elif defined(CONFIG_DIGIC_78X)
    #define IS_ROM_PTR(val) ((uintptr_t)(val) > (uintptr_t)0xE0000000)
#endif

#define PTR_INVALID             ((void *)0xFFFFFFFF)

/** Check a pointer for error code */
#define IS_ERROR(ptr)   (1 & (uintptr_t) ptr)

/* read a uint32_t from memory */
#define MEM(x) *(volatile uint32_t*)(x)

/* read a ENGIO value from shadow memory */
extern uint32_t shamem_read(uint32_t addr);

/* read a uint32_t from memory or engio shadow memory */
#define MEMX(x) ( \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0xC0000000UL) ? (uint32_t)shamem_read(x) : \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0xE0000000UL) ? (uint32_t)0xDEADBEAF : \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0x70000000UL) ? (uint32_t)0xDEADBEAF : \
        ((((uint32_t)(x)) & 0xF0000000UL) == 0x80000000UL) ? (uint32_t)0xDEADBEAF : \
        *(volatile uint32_t *)(x) \
)

/* Cacheable/uncacheable RAM pointers */
#ifdef CONFIG_VXWORKS
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) |  0x10000000))
#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) & ~0x10000000))
#else
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) | (((uint32_t)(x)) < 0x40000000 ?  0x40000000 : 0)))
#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) & ~0x40000000))
#endif

/* align a pointer at 16, 32 or 64 bits, with floor-like rounding */
#define ALIGN16(x) ((__typeof__(x))(((uint32_t)(x)) & ~1))
#define ALIGN32(x) ((__typeof__(x))(((uint32_t)(x)) & ~3))
#define ALIGN64(x) ((__typeof__(x))(((uint32_t)(x)) & ~7))

/* align a pointer at 16, 32 or 64 bits, with ceil-like rounding */
#define ALIGN16SUP(x) ((__typeof__(x))(((uint32_t)(x) + 1) & ~1))
#define ALIGN32SUP(x) ((__typeof__(x))(((uint32_t)(x) + 3) & ~3))
#define ALIGN64SUP(x) ((__typeof__(x))(((uint32_t)(x) + 7) & ~7))

/* memcpy/memset */
extern void * memset ( void * ptr, int value, size_t num );
extern void*  memcpy( void *, const void *, size_t );
extern int memcmp( const void* s1, const void* s2,size_t n );
extern void * memchr( const void *s, int c, size_t n );
extern void* memset64(void* dest, int val, size_t n);
extern void* memcpy64(void* dest, void* srce, size_t n);
extern void* dma_memcpy(void* dest, void* srce, size_t n);
extern void* edmac_memcpy(void* dest, void* srce, size_t n);

/* free memory info */
int GetFreeMemForAllocateMemory();
int GetFreeMemForMalloc();

// aligned malloc, for when the start address must be aligned to some boundary,
// e.g. MMU routines
void *malloc_aligned(size_t len, uint32_t alignment);

// aligned free, takes an aligned pointer, finds the block it's within,
// and frees it.
void free_aligned(void *ptr);

#endif
