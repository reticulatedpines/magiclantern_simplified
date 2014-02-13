/**
 * Memory management routines that can wrap all malloc-like memory backends in a transparent way.
 * 
 * Instead of a few small allocation functions, from user code you will only see a huge memory pool, ready to use.
 * 
 * Temporary hack: in order to work with existing code, 
 * it intercepts calls to malloc/free, AllocateMemory, alloc_dma_memory, shoot_malloc and SmallAlloc.
 * 
 * Implementation based on memcheck by g3gg0.
 */
#define NO_MALLOC_REDIRECT

#undef MEM_DEBUG        /* define this one to print a log about who is allocating what */

#include "compiler.h"
#include "tasks.h"
#include "limits.h"
#include "bmp.h"
#include "menu.h"

#ifdef MEM_DEBUG
#define dbg_printf(fmt,...) { console_printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

#define MEM_SEC_ZONE 32
#define MEMCHECK_ENTRIES 256
#define HISTORY_ENTRIES 256
#define TASK_NAME_SIZE 12

#define JUST_FREED 0xF12EEEED   /* FREEED */
#define UNTRACKED 0xFFFFFFFF

/* used for faking the cacheable flag (internally we must use the same flag as returned by allocator) */
#define UNCACHEABLE_FLAG 0x8000

typedef void* (*mem_init_func)();
typedef void* (*mem_alloc_func)(size_t size);
typedef void (*mem_free_func)(void* ptr);
typedef int (*mem_get_free_space_func)();
typedef int (*mem_get_max_region_func)();

/* use underscore for allocator functions to prevent other code from calling them directly */
extern void* _malloc(size_t size);
extern void _free(void* ptr);
extern void* _AllocateMemory(size_t size);
extern void _FreeMemory(void* ptr);
extern void* _alloc_dma_memory(size_t size);
extern void _free_dma_memory(void* ptr);
extern int _shoot_get_free_space();

static struct semaphore * mem_sem = 0;

struct mem_allocator
{
    char name[16];                          /* malloc, AllocateMemory, shoot_malloc, task_mem... */
    mem_init_func init;                     /* can be null; called at startup, before all other INIT_FUNCs */
    mem_alloc_func malloc;
    mem_free_func free;
    mem_alloc_func malloc_dma;              /* optional, if you can allocate uncacheable (DMA) memory from here */
    mem_free_func free_dma;
    mem_get_free_space_func get_free_space; /* can be null; will never try to malloc more than MAX(returned value / 2, get_max_region()) */
    mem_get_max_region_func get_max_region; /* can be null; will never try to malloc more than returned value */
    
    int is_preferred_for_temporary_space;   /* prefer using this for memory that will be freed shortly after used */
    
    int preferred_min_alloc_size;           /* if size is outside this range, it will try from other allocators */
    int preferred_max_alloc_size;           /* (but if it can't find any, it may still use this buffer) */
    int preferred_free_space;               /* if free space would drop under this, will try from other allocators first */
    int minimum_free_space;                 /* will never allocate if free space would drop under this */
    int minimum_alloc_size;                 /* will never allocate a buffer smaller than this */
    
    /* private stuff */
    int mem_used;
    int num_blocks;
};

int GetFreeMemForAllocateMemory()
{
    int a,b;
    GetMemoryInformation(&a,&b);
    return b;
}

static int GetMaxRegionForAllocateMemory()
{
    int a;
    int err = GetSizeOfMaxRegion(&a);
    if (err) return 0;
    return a;
}

static int GetFreeMemForMalloc()
{
    return MALLOC_FREE_MEMORY;
}

static struct mem_allocator allocators[] = {
    {
        .name = "malloc",
        .malloc = _malloc,
        .free = _free,
        .get_free_space = GetFreeMemForMalloc,
        .preferred_min_alloc_size = 0,
        .preferred_max_alloc_size = 512 * 1024,
        .preferred_free_space = 128 * 1024,
        .minimum_free_space = 64 * 1024,
    },

    {
        .name = "AllocateMemory",
        .malloc = _AllocateMemory,
        .free = _FreeMemory,
        .malloc_dma = _alloc_dma_memory,
        .free_dma = _free_dma_memory,
        .get_free_space = GetFreeMemForAllocateMemory,
        .get_max_region = GetMaxRegionForAllocateMemory,
        .preferred_min_alloc_size = 0,
        .preferred_max_alloc_size = 512 * 1024,
        .preferred_free_space = 1024 * 1024 * 3/2,  /* at 1MB free, "dispcheck" may stop working */
        #ifdef CONFIG_1100D
        .minimum_free_space = 384 * 1024,
        #else
        .minimum_free_space = 512 * 1024,           /* Canon code also allocates from here, so keep it free */
        #endif
    },

#if 0 /* not implemented yet */
    {
        .name = "RscMgr",
        .malloc = _rscmgr_malloc,
        .free = _rscmgr_free,
        .get_free_space = rscmgr_get_free_space,
        .preferred_min_alloc_size = 16 * 1024,
        .preferred_max_alloc_size = 2 * 1024 * 1024,
    },
#endif

#if 0 /* not implemented yet */
    {
        .name = "task_mem",
        .malloc = _task_malloc,
        .free = _task_free,
        .get_free_space = task_get_free_space,
        .preferred_min_alloc_size = 64 * 1024,
        .preferred_max_alloc_size = 512 * 1024,
    },
#endif

#if 1
    /* must be completely free when navigating Canon menus, so only use it as a last resort */
    {
        .name = "shoot_malloc",
        .malloc = _shoot_malloc,
        .free = _shoot_free,
        .malloc_dma = _shoot_malloc,       /* can be used for both cacheable and uncacheable memory */
        .free_dma = _shoot_free,
        .get_free_space = _shoot_get_free_space,
        .get_max_region = _shoot_get_free_space,    /* we usually have a bunch of large contiguous chunks */
        
        .is_preferred_for_temporary_space = 1,  /* if we know we'll free this memory quickly, prefer this one */
        
        /* no free space check yet; just assume it's BIG */
        .preferred_min_alloc_size = 512 * 1024,
        .preferred_max_alloc_size = 32 * 1024 * 1024,
        .minimum_alloc_size = 32 * 1024,
    },
#endif
};

/* total memory allocated (for printing it) */
static volatile int alloc_total = 0;

/* total memory allocated, including memcheck overhead */
static volatile int alloc_total_with_memcheck = 0;

/* peak memory allocated (max value since power on), including memcheck overhead */
static volatile int alloc_total_peak_with_memcheck = 0;

/* to show a graph with memory usage */
struct mem_history_node
{
    int timestamp;
    int alloc_total;
};

static struct mem_history_node history[HISTORY_ENTRIES];
static int history_index = 0;

struct memcheck_hdr
{
    unsigned int id;
    unsigned int length;
    uint16_t allocator;
    uint16_t flags;
};

struct memcheck_entry
{
    unsigned int ptr;
    char * file;
    uint16_t failed;
    uint16_t line;
    char task_name[TASK_NAME_SIZE];
};

static struct memcheck_entry memcheck_mallocbuf[MEMCHECK_ENTRIES];
static unsigned int memcheck_bufpos = 0;

static volatile int last_error = 0;
static char last_error_msg_short[20] = "";
static char last_error_msg[100] = "";

static char* file_name_without_path(const char* file)
{
    /* only show the file name, not full path */
    char* fn = (char*)file + strlen(file) - 1;
    while (fn > file && *(fn-1) != '/') fn--;
    return fn;
}

/* warning: can't call this twice in the same printf */
const char * format_memory_size( unsigned size)
{
    static char str[16];
    
    if ( size >= 1024*1024*1024 )
    {
        int size_gb = (size/1024 * 10 + 5)  / 1024 / 1024;
        snprintf( str, sizeof(str), "%d.%dGB", size_gb/10, size_gb%10);
    }
    else if ( size >= 10*1024*1024 )
    {
        int size_mb = size / 1024 / 1024;
        snprintf( str, sizeof(str), "%dMB", size_mb);
    }
    else if ( size >= 1024*1024 )
    {
        int size_mb = (size * 10 + 5) / 1024 / 1024;
        snprintf( str, sizeof(str), "%d.%dMB", size_mb/10, size_mb%10);
    }
    else if ( size >= 10*1024 )
    {
        int size_kb = (size * 10 + 5) / 1024;
        snprintf( str, sizeof(str), "%dkB", size_kb/10);
    }
    else if ( size >= 1024 )
    {
        int size_kb = (size * 10 + 5) / 1024;
        snprintf( str, sizeof(str), "%d.%dkB", size_kb/10, size_kb%10);
    }
    else if (size > 0)
    {
        snprintf( str, sizeof(str), "%d B", size);
    }
    else
    {
        snprintf( str, sizeof(str), "0");
    }

    return str;
}

static const char * format_memory_size_and_flags( unsigned size, unsigned flags)
{
    static char str[32];
    snprintf(str, sizeof(str), format_memory_size(size));
    if (flags & MEM_TEMPORARY) STR_APPEND(str, "|TMP");
    if (flags & MEM_DMA) STR_APPEND(str, "|DMA");
    return str;
}

/* second arg is optional, -1 if not available */
static unsigned int memcheck_check(unsigned int ptr, unsigned int entry)
{
    unsigned int failed = 0;
    unsigned int failed_pos = 0;
    
    for(int pos = sizeof(struct memcheck_hdr); pos < MEM_SEC_ZONE; pos++)
    {
        unsigned char value = ((unsigned char *)ptr)[pos];
        // dbg_printf("free check %d %x\n ", pos, value);
        if (value != 0xA5)
        {
            failed |= 2;
            failed_pos = pos;
        }
    }
    for(int pos = 0; pos < MEM_SEC_ZONE; pos++)
    {
        int pos2 = MEM_SEC_ZONE + ((struct memcheck_hdr *)ptr)->length + pos;
        unsigned char value = ((unsigned char *)ptr)[pos2];
        // dbg_printf("free check %d %x\n ", pos2, value);
        if (value != 0xA5)
        {
            failed |= 4;
            failed_pos = pos2;
        }
    }

    unsigned int id = ((struct memcheck_hdr *)ptr)->id;
    int id_ok = 0;

    if (id == UNTRACKED)
    {
        /* we no longer keep track of this block => nothing to check */
    }
    else if (id == JUST_FREED) /* already freed? */
    {
        failed |= 16;
    }
    else if (id != entry && entry != 0xFFFFFFFF) /* wrong ID? */
    {
        failed |= 8;
    }
    else if (id >= MEMCHECK_ENTRIES) /* out of range? */
    {
        failed |= 8;
    }
    else /* ID looks alright */
    {
        id_ok = 1;
    }

    if (failed && !last_error)
    {
        last_error = failed;

        int flags = ((struct memcheck_hdr *)ptr)->flags;
        int size = ((struct memcheck_hdr *)ptr)->length;
        int allocator = ((struct memcheck_hdr *)ptr)->allocator;

        char* file = "unk";
        int line = 0;
        char* task_name = "unk";
        char* allocator_name = "unk";
        if (id_ok)
        {
            file = (char*) memcheck_mallocbuf[id].file;
            line = memcheck_mallocbuf[id].line;
            task_name = memcheck_mallocbuf[id].task_name;
        }
        else
        {
            task_name = get_task_name_from_id((int)get_current_task());
        }
        
        if (allocator >= 0 && allocator < COUNT(allocators))
        {
            allocator_name = allocators[allocator].name;
        }

        char err_flags[20] = "";
        if (failed & 2) STR_APPEND(err_flags, "underflow,");
        if (failed & 4) STR_APPEND(err_flags, "overflow,");
        if (failed & 8) STR_APPEND(err_flags, "ID error,");
        if (failed & 16) STR_APPEND(err_flags, "double free,");
        if (failed & ~(2|4|8|16)) STR_APPEND(err_flags, "unknown error,");
        err_flags[strlen(err_flags)-1] = 0;
        int index_err = (failed & 6);
        snprintf(last_error_msg_short, sizeof(last_error_msg_short), err_flags);
        snprintf(last_error_msg, sizeof(last_error_msg),
            "%s[%d/%d] %s(%s) at %s:%d, task %s.",
            err_flags, index_err ? failed_pos - MEM_SEC_ZONE : 0, index_err ? size : 0,
            allocator_name, format_memory_size_and_flags(size, flags),
            file, line, task_name
        );
    }
    
    memcheck_mallocbuf[entry].failed |= failed;
    
    return failed;
}

static unsigned int memcheck_get_failed()
{
    unsigned int buf_pos = 0;
    
    for(buf_pos = 0; buf_pos < MEMCHECK_ENTRIES; buf_pos++)
    {
        if(memcheck_mallocbuf[buf_pos].ptr)
        {
            memcheck_check(memcheck_mallocbuf[buf_pos].ptr, buf_pos);
            
            /* marked as failed? */
            if(memcheck_mallocbuf[buf_pos].failed)
            {
                return memcheck_mallocbuf[buf_pos].failed;
            }
        }
    }
    return 0;
}

static void memcheck_add(unsigned int ptr, const char *file, unsigned int line)
{
    int tries = MEMCHECK_ENTRIES;
    
    unsigned int state = cli();
    while(memcheck_mallocbuf[memcheck_bufpos].ptr != 0)
    {
        memcheck_bufpos++;
        memcheck_bufpos %= MEMCHECK_ENTRIES;
        
        if(--tries <= 0)
        {
            ((struct memcheck_hdr *)ptr)->id = UNTRACKED;
            sei(state);
            return;
        }
    }

    memcheck_mallocbuf[memcheck_bufpos].ptr = ptr;
    memcheck_mallocbuf[memcheck_bufpos].failed = 0;
    memcheck_mallocbuf[memcheck_bufpos].file = file_name_without_path(file);
    memcheck_mallocbuf[memcheck_bufpos].line = line;
    snprintf((char*)memcheck_mallocbuf[memcheck_bufpos].task_name, TASK_NAME_SIZE, "%s", get_task_name_from_id((int)get_current_task()));
    
    ((struct memcheck_hdr *)ptr)->id = memcheck_bufpos;
    
    sei(state);
}

static void memcheck_remove(unsigned int ptr, unsigned int failed)
{
    unsigned int buf_pos = ((struct memcheck_hdr *)ptr)->id;
    ((struct memcheck_hdr *)ptr)->id = JUST_FREED;
    
    if(buf_pos != UNTRACKED && (failed || memcheck_mallocbuf[buf_pos].ptr != ptr))
    {
        for(buf_pos = 0;buf_pos < MEMCHECK_ENTRIES;buf_pos++)
        {
            if(memcheck_mallocbuf[buf_pos].ptr == ptr)
            {
                memcheck_mallocbuf[buf_pos].ptr = (intptr_t) INVALID_PTR;
                memcheck_mallocbuf[buf_pos].failed |= (0x00000001 | failed);
            }            
        }
    }
    else
    {
        memcheck_mallocbuf[buf_pos].failed = 0;
        memcheck_mallocbuf[buf_pos].file = 0;
        
        unsigned int state = cli();
        memcheck_mallocbuf[buf_pos].ptr = 0;
        memcheck_bufpos = buf_pos;
        sei(state);
    }
}

static void *memcheck_malloc( unsigned int len, const char *file, unsigned int line, int allocator_index, unsigned int flags)
{
    unsigned int ptr;
    
    //~ dbg_printf("alloc %d %s:%d\n ", len, file, line);
    //~ int t0 = get_ms_clock_value();

    int requires_dma = flags & MEM_DMA;
    if (requires_dma)
    {
        ptr = (unsigned int) allocators[allocator_index].malloc_dma(len + 2 * MEM_SEC_ZONE);
    }
    else
    {
        ptr = (unsigned int) allocators[allocator_index].malloc(len + 2 * MEM_SEC_ZONE);
    }

    //~ int t1 = get_ms_clock_value();
    //~ dbg_printf("alloc returned %x, took %s%d.%03d s\n", ptr, FMT_FIXEDPOINT3(t1-t0));
    
    /* some allocators may return invalid ptr; discard it and return 0, as C malloc does */
    if ((intptr_t)ptr & 1) return 0;
    if (!ptr) return 0;
    
    /* fill MEM_SEC_ZONE with 0xA5 */
    for(unsigned pos = 0; pos < MEM_SEC_ZONE; pos++)
    {
        ((unsigned char *)ptr)[pos] = 0xA5;
    }

    for(unsigned pos = len + MEM_SEC_ZONE; pos < len + 2 * MEM_SEC_ZONE; pos++)
    {
        ((unsigned char *)ptr)[pos] = 0xA5;
    }
    
    /* did our allocator return a cacheable or uncacheable pointer? */
    unsigned int uncacheable_flag = (ptr == (unsigned int) UNCACHEABLE(ptr)) ? UNCACHEABLE_FLAG : 0;
    
    ((struct memcheck_hdr *)ptr)->length = len;
    ((struct memcheck_hdr *)ptr)->allocator = allocator_index;
    ((struct memcheck_hdr *)ptr)->flags = flags | uncacheable_flag;

    memcheck_add(ptr, file, line);
    
    /* keep track of allocated memory and update history */
    allocators[allocator_index].num_blocks++;
    allocators[allocator_index].mem_used += len + 2 * MEM_SEC_ZONE;
    alloc_total += len;
    alloc_total_with_memcheck += len + 2 * MEM_SEC_ZONE;
    alloc_total_peak_with_memcheck = MAX(alloc_total_peak_with_memcheck, alloc_total_with_memcheck);
    history[history_index].timestamp = get_ms_clock_value();
    history[history_index].alloc_total = alloc_total_with_memcheck;
    history_index = mod(history_index + 1, HISTORY_ENTRIES);
    
    return (void*)(ptr + MEM_SEC_ZONE);
}

static void memcheck_free( void * buf, int allocator_index, unsigned int flags)
{
    unsigned int ptr = ((unsigned int)buf - MEM_SEC_ZONE);

    int failed = memcheck_check(ptr, 0xFFFFFFFF);
    
    memcheck_remove(ptr, failed);
    
    /* if there are errors, do not free this block */
    if (failed)
    {
        return;
    }

    /* keep track of allocated memory and update history */
    int len = ((struct memcheck_hdr *)ptr)->length;
    allocators[allocator_index].num_blocks--;
    allocators[allocator_index].mem_used -= (len + 2 * MEM_SEC_ZONE);
    alloc_total -= len;
    alloc_total_with_memcheck -= (len + 2 * MEM_SEC_ZONE);
    history[history_index].timestamp = get_ms_clock_value();
    history[history_index].alloc_total = alloc_total_with_memcheck;
    history_index = mod(history_index + 1, HISTORY_ENTRIES);

    /* tell the backend to free this block */
    int requires_dma = flags & MEM_DMA;
    if (requires_dma)
    {
        allocators[allocator_index].free_dma((void*)ptr);
    }
    else
    {
        allocators[allocator_index].free((void*)ptr);
    }
}

static int search_for_allocator(int size, int require_preferred_size, int require_preferred_free_space, int require_tmp, int require_dma)
{
    for (int a = 0; a < COUNT(allocators); a++)
    {
        int has_non_dma = allocators[a].malloc ? 1 : 0;
        int has_dma = allocators[a].malloc_dma ? 1 : 0;
        int preferred_for_tmp = allocators[a].is_preferred_for_temporary_space ? 1 : -1;

        /* TODO: get rid of cascaded if's (use negative logic and "continue") */
        
        /* do we need DMA? */
        if (
                (require_dma && has_dma) ||
                (!require_dma && has_non_dma)
           )
        {
            /* is this pool preferred for temporary allocations? */
            if (
                    !require_tmp ||
                    (require_tmp == preferred_for_tmp)
               )
            {
                /* matches preferred size criteria? */
                if 
                    (
                        (
                            !require_preferred_size ||
                            (size >= allocators[a].preferred_min_alloc_size && size <= allocators[a].preferred_min_alloc_size)
                        )
                        && 
                        (
                            /* minimum_alloc_size is mandatory (e.g. don't allocate 5-byte blocks from shoot_malloc) */
                            size >= allocators[a].minimum_alloc_size
                        )
                   )
                {
                    /* do we have enough free space without exceeding the preferred limit? */
                    int free_space = allocators[a].get_free_space ? allocators[a].get_free_space() : 30*1024*1024;
                    //~ dbg_printf("%s: free space %s\n", allocators[a].name, format_memory_size(free_space));
                    if (
                            (
                                /* preferred free space is... well... optional */
                                !require_preferred_free_space ||
                                (free_space - size - 1024 > allocators[a].preferred_free_space)
                            )
                            &&
                            (
                                /* minimum_free_space is mandatory */
                                free_space - size - 1024 > allocators[a].minimum_free_space
                            )    
                       )
                    {
                        /* do we have a large enough contiguous chunk? */
                        /* use a heuristic if we don't know, use a safety margin even if we know */
                        int max_region = allocators[a].get_max_region ? allocators[a].get_max_region() - 16384 : free_space / 4;
                        //~ dbg_printf("%s: max rgn %s\n", allocators[a].name, format_memory_size(max_region));
                        if (size < max_region)
                        {
                            /* yes, we do! */
                            return a;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

static int choose_allocator(int size, unsigned int flags)
{
    /* note: free space routines may be queried more than once (this can be optimized) */
    
    int needs_dma = (flags & MEM_DMA) ? 1 : 0;
    int prefers_tmp = (flags & MEM_TEMPORARY) ? 1 : -1;
    
    int a;
    
    /* first try to find an allocator that meets all the conditions (preferred size, free space, temporary preference and DMA); */
    a = search_for_allocator(size, 1, 1, prefers_tmp, needs_dma);
    if (a >= 0) return a;

    /* next, try something that doesn't meet the preferred buffer size */
    a = search_for_allocator(size, 0, 1, prefers_tmp, needs_dma);
    if (a >= 0) return a;

    /* next, try something that doesn't meet the preferred free space */
    a = search_for_allocator(size, 0, 0, prefers_tmp, needs_dma);
    if (a >= 0) return a;

    /* next, try something that doesn't meet the temporary preference */
    if (prefers_tmp)
    {
        a = search_for_allocator(size, 0, 0, 0, needs_dma);
        if (a >= 0) return a;
    }
    
    /* DMA is mandatory, don't relax it */
    
    /* if we arrive here, you should probably solder some memory chips on the mainboard */
    return -1;
}

/* these two will replace all malloc calls */

/* returns 0 if it couldn't allocate */
void* __mem_malloc(size_t size, unsigned int flags, const char* file, unsigned int line)
{
    take_semaphore(mem_sem, 0);

    dbg_printf("alloc(%s) from %s:%d task %s\n", format_memory_size_and_flags(size, flags), file, line, get_task_name_from_id((int)get_current_task()));
    
    /* show files without full path in error messages (they are too big) */
    file = file_name_without_path(file);

    /* choose an allocator (a preferred memory pool to allocate memory from it) */
    int allocator_index = choose_allocator(size, flags);
    
    /* did we find one? */
    if (allocator_index >= 0 && allocator_index < COUNT(allocators))
    {
        /* yes, let's allocate */

        dbg_printf("using %s (%d blocks)\n", allocators[allocator_index].name, allocators[allocator_index].num_blocks);
        
        #ifdef MEM_DEBUG
        int t0 = get_ms_clock_value();
        #endif
        
        void* ptr = memcheck_malloc(size, file, line, allocator_index, flags);
        
        #ifdef MEM_DEBUG
        int t1 = get_ms_clock_value();
        #endif
        
        if (!ptr)
        {
            /* didn't work? */
            snprintf(last_error_msg_short, sizeof(last_error_msg_short), "%s(%s,%x)", allocators[allocator_index].name, format_memory_size_and_flags(size, flags));
            snprintf(last_error_msg, sizeof(last_error_msg), "%s(%s) failed at %s:%d, %s.", allocators[allocator_index].name, format_memory_size_and_flags(size, flags), file, line, get_task_name_from_id((int)get_current_task()));
            dbg_printf("alloc fail, took %s%d.%03d s\n", FMT_FIXEDPOINT3(t1-t0));
        }
        else
        {
            /* force the cacheable pointer to be the way user requested it */
            /* note: internally, this library must use the vanilla pointer (non-mangled) */
            ptr = (flags & MEM_DMA) ? UNCACHEABLE(ptr) : CACHEABLE(ptr);

            dbg_printf("alloc ok, took %s%d.%03d s\n", FMT_FIXEDPOINT3(t1-t0));
        }
        
        give_semaphore(mem_sem);
        return ptr;
    }
    
    /* could not find an allocator (maybe out of memory?) */
    snprintf(last_error_msg_short, sizeof(last_error_msg_short), "alloc(%s)", format_memory_size_and_flags(size, flags));
    snprintf(last_error_msg, sizeof(last_error_msg), "No allocator for %s at %s:%d, %s.", format_memory_size_and_flags(size, flags), file, line, get_task_name_from_id((int)get_current_task()));
    dbg_printf("alloc not found\n");
    give_semaphore(mem_sem);
    return 0;
}

void __mem_free(void* buf)
{
    take_semaphore(mem_sem, 0);

    unsigned int ptr = (unsigned int)buf - MEM_SEC_ZONE;

    int allocator_index = ((struct memcheck_hdr *)ptr)->allocator;
    unsigned int flags = ((struct memcheck_hdr *)ptr)->flags;

    /* make sure the caching flag is the same as returned by the allocator */
    buf = (flags & UNCACHEABLE_FLAG) ? UNCACHEABLE(buf) : CACHEABLE(buf);

    dbg_printf("free(%s) from task %s\n", format_memory_size_and_flags(((struct memcheck_hdr *)ptr)->length, flags), get_task_name_from_id((int)get_current_task()));
    
    if (allocator_index >= 0 && allocator_index < COUNT(allocators))
    {
        memcheck_free(buf, allocator_index, flags);
        dbg_printf("free ok\n");
    }
    else
    {
        dbg_printf("free fail\n");
    }
    
    give_semaphore(mem_sem);
}

/* thread-safe wrappers for exmem routines */
struct memSuite *shoot_malloc_suite(size_t size)
{
    take_semaphore(mem_sem, 0);
    void* ans = _shoot_malloc_suite(size);
    give_semaphore(mem_sem);
    return ans;
}

void shoot_free_suite(struct memSuite * hSuite)
{
    take_semaphore(mem_sem, 0);
    _shoot_free_suite(hSuite);
    give_semaphore(mem_sem);
}

struct memSuite * shoot_malloc_suite_contig(size_t size)
{
    take_semaphore(mem_sem, 0);
    void* ans = _shoot_malloc_suite_contig(size);
    give_semaphore(mem_sem);
    return ans;
}


/* initialize memory pools, if any of them needs that */
/* (called as the first init func => mem.o should be first in the Makefile.src (well, after boot-hack) */
static void mem_init()
{
    mem_sem = create_named_semaphore("mem_sem", 1);

    for (int a = 0; a < COUNT(allocators); a++)
    {
        if (allocators[a].init)
        {
            allocators[a].init();
        }
    }
}

INIT_FUNC(__FILE__, mem_init);


/* GUI stuff */

#ifdef FEATURE_SHOW_FREE_MEMORY

static volatile int max_stack_ack = 0;

static void max_stack_try(void* size) { max_stack_ack = (int) size; }

static int stack_size_crit(int x)
{
    int size = x * 1024;
    task_create("stack_try", 0x1e, size, max_stack_try, (void*) size);
    msleep(50);
    if (max_stack_ack == size) return 1;
    return -1;
}

static int max_shoot_malloc_mem = 0;
static int max_shoot_malloc_frag_mem = 0;
static char shoot_malloc_frag_desc[70] = "";
static char memory_map[720];

#define MEMORY_MAP_ADDRESS_TO_INDEX(p) ((int)CACHEABLE(p)/1024 * 720 / 512/1024)
#define MEMORY_MAP_INDEX_TO_ADDRESS(i) ALIGN32((i) * 512 * 1024 / 720 * 1024)

/* fixme: find a way to read the free stack memory from DryOS */
/* current workaround: compute it by trial and error when you press SET on Free Memory menu item */
static volatile int guess_mem_running = 0;
static void guess_free_mem_task(void* priv, int delta)
{
    /* reset values */
    max_stack_ack = 0;
    max_shoot_malloc_mem = 0;
    max_shoot_malloc_frag_mem = 0;

    bin_search(1, 1024, stack_size_crit);

    /* we won't keep these things allocated much, so we can pause malloc activity while running this (just so nothing will fail) */
    /* note: we use the _underlined routines here, but please don't do that in user code */
    take_semaphore(mem_sem, 0);

    {
        struct memSuite * hSuite = _shoot_malloc_suite_contig(0);
        if (!hSuite)
        {
            beep();
            guess_mem_running = 0;
            give_semaphore(mem_sem);
            return;
        }
        ASSERT(hSuite->num_chunks == 1);
        max_shoot_malloc_mem = hSuite->size;
        _shoot_free_suite(hSuite);
    }

    struct memSuite * hSuite = _shoot_malloc_suite(0);
    if (!hSuite)
    {
        beep();
        guess_mem_running = 0;
        return;
    }
    max_shoot_malloc_frag_mem = hSuite->size;

    struct memChunk *currentChunk;
    int chunkAvail;
    void* chunkAddress;
    int total = 0;

    currentChunk = GetFirstChunkFromSuite(hSuite);

    snprintf(shoot_malloc_frag_desc, sizeof(shoot_malloc_frag_desc), "");
    memset(memory_map, 0, sizeof(memory_map));

    while(currentChunk)
    {
        chunkAvail = GetSizeOfMemoryChunk(currentChunk);
        chunkAddress = (void*)GetMemoryAddressOfMemoryChunk(currentChunk);

        int mb = 10*chunkAvail/1024/1024;
        STR_APPEND(shoot_malloc_frag_desc, mb%10 ? "%s%d.%d" : "%s%d", total ? "+" : "", mb/10, mb%10);
        total += chunkAvail;

        int start = MEMORY_MAP_ADDRESS_TO_INDEX(chunkAddress);
        int width = MEMORY_MAP_ADDRESS_TO_INDEX(chunkAvail);
        memset(memory_map + start, COLOR_GREEN1, width);

        currentChunk = GetNextMemoryChunk(hSuite, currentChunk);
    }
    STR_APPEND(shoot_malloc_frag_desc, " MB.");
    ASSERT(max_shoot_malloc_frag_mem == total);

    exmem_clear(hSuite, 0);

    _shoot_free_suite(hSuite);

    /* mallocs can resume now */
    give_semaphore(mem_sem);

    /* memory analysis: how much appears unused? */
    for (uint32_t i = 0; i < 720; i++)
    {
        if (memory_map[i])
            continue;

        uint32_t empty = 1;
        uint32_t start = MEMORY_MAP_INDEX_TO_ADDRESS(i);
        uint32_t end = MEMORY_MAP_INDEX_TO_ADDRESS(i+1);

        for (uint32_t p = start; p < end; p += 4)
        {
            uint32_t v = MEM(p);
            #ifdef CONFIG_MARK_UNUSED_MEMORY_AT_STARTUP
            if (v != 0x124B1DE0 /* RA(W)VIDEO*/)
            #else
            if (v != 0 && v != 0xFFFFFFFF)
            #endif
            {
                empty = 0;
                break;
            }
        }

        memory_map[i] = empty ? COLOR_BLUE : COLOR_RED;
    }

    menu_redraw();
    guess_mem_running = 0;
}

static void guess_free_mem()
{
    task_create("guess_mem", 0x1e, 0x4000, guess_free_mem_task, 0);
}

static MENU_UPDATE_FUNC(mem_error_display);

static struct { uint32_t addr; char* name; } common_addresses[] = {
    { RESTARTSTART,         "RST"},
    { YUV422_HD_BUFFER_1,   "HD1"},
    { YUV422_HD_BUFFER_1,   "HD2"},
    { YUV422_LV_BUFFER_1,   "LV1"},
    { YUV422_LV_BUFFER_2,   "LV2"},
    { YUV422_LV_BUFFER_3,   "LV3"},
};

static MENU_UPDATE_FUNC(meminfo_display)
{
    int M = GetFreeMemForAllocateMemory();
    int m = MALLOC_FREE_MEMORY;

#ifdef CONFIG_VXWORKS
    MENU_SET_VALUE(
        "%dK",
        M/1024
    );
    if (M < 1024*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Not enough free memory.");
#else

    int guess_needed = 0;
    int info_type = (int) entry->priv;
    switch (info_type)
    {
        case 0: // main entry
            MENU_SET_VALUE(
                "%dK + %dK",
                m/1024, M/1024
            );
            //if (M < 1024*1024 || m < 128*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Not enough free memory.");
            MENU_SET_ENABLED(1);
            MENU_SET_ICON(MNI_DICE, 0);
            mem_error_display(entry, info);
            break;
#if 0
        case 1: // malloc
            MENU_SET_VALUE("%s", format_memory_size(m));
            if (m < 128*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Would be nice to have at least 128K free here.");
            break;

        case 2: // AllocateMemory
            MENU_SET_VALUE("%s", format_memory_size(M));
            if (M < 1024*1024) MENU_SET_WARNING(MENU_WARN_ADVICE, "Canon code requires around 1 MB from here.");
            break;
#endif

        case 3: // task stack
            MENU_SET_VALUE("%s", format_memory_size(max_stack_ack));
            guess_needed = 1;
            break;

        case 4: // shoot_malloc contig
            MENU_SET_VALUE("%s", format_memory_size(max_shoot_malloc_mem));
            guess_needed = 1;
            break;

        case 5: // shoot_malloc fragmented
            MENU_SET_VALUE("%s", format_memory_size(max_shoot_malloc_frag_mem));
            MENU_SET_WARNING(MENU_WARN_INFO, shoot_malloc_frag_desc);
            guess_needed = 1;
            
            /* paint memory map */
            for (int i = 0; i < 720; i++)
                if (memory_map[i])
                    draw_line(i, 400, i, 410, memory_map[i]);
            
            /* show some common addresses on the memory map */
            for (int i = 0; i < COUNT(common_addresses); i++)
            {
                int c = MEMORY_MAP_ADDRESS_TO_INDEX(common_addresses[i].addr);
                draw_line(c, 390, c, 400, COLOR_YELLOW);
                bmp_printf(FONT_SMALL, c, 385, common_addresses[i].name);
            }

            /* show EDMAC addresses on the memory map */
            for (int i = 0; i < 32; i++)
            {
                int a = edmac_get_address(i);
                if (a)
                {
                    int c = MEMORY_MAP_ADDRESS_TO_INDEX(a);
                    draw_line(c, 410, c, 420, COLOR_YELLOW);
                    int msg = i < 10 ? '0'+i : 'a'+i;  /* extended hex to fit in the single character */
                    bmp_printf(FONT_SMALL | FONT_ALIGN_CENTER, c, 415, "%s", (char*) &msg);
                }
            }
            break;

        #if defined(CONFIG_MEMPATCH_CHECK)
        case 6: // autoexec size
        {
            extern uint32_t ml_reserved_mem;
            extern uint32_t ml_used_mem;

            if (ABS(ml_used_mem - ml_reserved_mem) < 1024) MENU_SET_VALUE(
                "%s",
                format_memory_size(ml_used_mem)
            );
            else MENU_SET_VALUE(
                "%s of %s",
                format_memory_size(ml_used_mem), format_memory_size(ml_reserved_mem)
            );
            if (ml_reserved_mem < ml_used_mem)
                MENU_SET_WARNING(MENU_WARN_ADVICE, "ML uses too much memory!!");

            break;
        }
        #endif
    }

    if (guess_needed && !guess_mem_running)
    {
        /* check this once every 20 seconds (not more often) */
        static int aux = INT_MIN;
        if (should_run_polling_action(20000, &aux))
        {
            guess_mem_running = 1;
            guess_free_mem();
        }
    }

    if (guess_mem_running)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Trying to guess how much RAM we have...");
    else
        MENU_SET_HELP("GREEN=free shoot, BLUE=00/FF maybe free, RED=maybe used");
#endif
}

static MENU_UPDATE_FUNC(mem_pool_display)
{
    int index = (int) entry->priv;
    if (index < 0 || index >= COUNT(allocators))
    {
        /* invalid, do not display */
        entry->shidden = 1;
        return;
    }
    
    MENU_SET_NAME(allocators[index].name);

    int used = allocators[index].mem_used;
    int free_space = allocators[index].get_free_space ? allocators[index].get_free_space() : -1;

    if (free_space > 0)
    {
        MENU_SET_VALUE("%s", format_memory_size(free_space));
        MENU_APPEND_VALUE(", %s used", format_memory_size(used));
        MENU_SET_HELP("Free & used memory from %s. %d blocks allocated.", allocators[index].name, allocators[index].num_blocks);
    }
    else
    {
        MENU_SET_VALUE("%s used", format_memory_size(used));
        MENU_SET_HELP("Memory used from %s. %d blocks allocated.", allocators[index].name, allocators[index].num_blocks);
    }
    
    if (allocators[index].get_max_region)
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "Max region: %s.", format_memory_size(allocators[index].get_max_region()));
    }
    else
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "This allocator does not implement get_max_region.");
    }
    
    if (free_space > 0 && free_space < allocators[index].preferred_free_space)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Would be nice to have at least %s free here.", format_memory_size(allocators[index].preferred_free_space));
    }
}

static MENU_UPDATE_FUNC(mem_error_display)
{
    if (strlen(last_error_msg) == 0)
    {
        /* no error caught yet? do a quick check of all allocated stuff */
        /* this will fill last_error strings if there's any error */
        memcheck_get_failed();
    }
    
    if (strlen(last_error_msg))
    {
        MENU_SET_NAME("Memory Error");
        MENU_SET_VALUE(last_error_msg_short);
        MENU_SET_WARNING(MENU_WARN_ADVICE, last_error_msg);
        MENU_SET_ICON(MNI_RECORD, 0); /* red dot */
        return;
    }
}

static int total_ram_detailed = 0;

static MENU_UPDATE_FUNC(mem_total_display)
{
    if (total_ram_detailed && info->can_custom_draw && entry->selected)
    {
        info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
        bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

        bmp_printf(FONT_LARGE, 10, 10, "Allocated memory");
        int x = 10;
        int y = 50;

        int small_blocks = 0;
        int small_blocks_size = 0;
        for(int buf_pos = 0; buf_pos < MEMCHECK_ENTRIES; buf_pos++)
        {
            void* ptr = (void*) memcheck_mallocbuf[buf_pos].ptr;
            if (!ptr) continue;
            
            int size = ((struct memcheck_hdr *)ptr)->length;
            int flags = ((struct memcheck_hdr *)ptr)->flags;
            int allocator = ((struct memcheck_hdr *)ptr)->allocator;
            
            if (size < 32768 || y > 300)
            {
                small_blocks++;
                small_blocks_size += size;
                continue;
            }

            char* file = (char*)memcheck_mallocbuf[buf_pos].file;
            int line = memcheck_mallocbuf[buf_pos].line;
            char* task_name = (char*) memcheck_mallocbuf[buf_pos].task_name;
            char* allocator_name = allocators[allocator].name;
            bmp_printf(FONT_MED, x, y, "%s%s", memcheck_mallocbuf[buf_pos].failed ? "[FAIL] " : "", format_memory_size_and_flags(size, flags));
            bmp_printf(FONT_MED, 180, y, "%s:%d task %s", file, line, task_name);
            bmp_printf(FONT_MED | FONT_ALIGN_RIGHT, 710, y, allocator_name);
            y += font_med.height;
        }
        
        if (small_blocks)
        {
            char msg[100];
            snprintf(msg, sizeof(msg), "%d small blocks, %s, ", small_blocks, format_memory_size(small_blocks_size));
            STR_APPEND(msg, "overhead %s", format_memory_size(small_blocks * 2 * MEM_SEC_ZONE));
            bmp_printf(FONT_MED, x, y, msg);
            y += font_med.height;
        }
        
        /* show history */
        
        int first_index = history_index + 1;
        while (history[first_index].timestamp == 0)
            first_index = mod(first_index + 1, HISTORY_ENTRIES);
        
        int t0 = history[first_index].timestamp;
        int t_end = get_ms_clock_value();
        int peak_y = y+10;
        int peak = alloc_total_peak_with_memcheck;
        int total = alloc_total_with_memcheck;
        if (t_end > t0)
        {
            int maxh = 480 - peak_y;
            bmp_fill(COLOR_GRAY(20), 0, 480-maxh, 720, maxh);
            int next_i;
            for (int i = first_index; i != history_index; i = next_i)
            {
                next_i = mod(i+1, HISTORY_ENTRIES);
                int t = history[i].timestamp;
                int t2 = (next_i != history_index) ? history[next_i].timestamp : t_end;
                if (t2 < t) continue;
                if (i == history_index-1) t2 = t_end;
                int x = 720 * (t - t0) / (t_end - t0);
                int x2 = 720 * (t2 - t0) / (t_end - t0);
                int h = MIN((uint64_t)history[i].alloc_total * maxh / peak, maxh);
                y = 480 - h;
                int w = MAX(x2-x, 2);
                bmp_fill(h == maxh ? COLOR_RED : COLOR_BLUE, x, y, w, h);
            }
        }

        bmp_printf(FONT_MED, 10, peak_y, "%s", format_memory_size(peak));
        bmp_printf(FONT_MED, 650, MAX(y-20, peak_y), "%s", format_memory_size(total));
    }
    else
    {
        total_ram_detailed = 0;
        MENU_SET_VALUE("%s", format_memory_size(alloc_total_with_memcheck));
        MENU_APPEND_VALUE(", peak %s", format_memory_size(alloc_total_peak_with_memcheck));
        int ovh = (alloc_total_with_memcheck + sizeof(memcheck_mallocbuf) - alloc_total) * 1000 / alloc_total;
        MENU_SET_WARNING(MENU_WARN_INFO, "Memcheck overhead: %d.%d%%.", ovh/10, ovh%10, 0);
    }
}


static struct menu_entry mem_menus[] = {
#ifdef CONFIG_VXWORKS
    {
        .name = "Free Memory",
        .update = meminfo_display,
        .icon_type = IT_ALWAYS_ON,
        .help = "Free memory, shared between ML and Canon firmware.",
    },
#else // dryos
    {
        .name = "Free Memory",
        .update = meminfo_display,
        .select = menu_open_submenu,
        .help = "Free memory, shared between ML and Canon firmware.",
        .help2 = "Press SET for detailed info.",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Allocated RAM",
                .update = mem_total_display,
                .priv = &total_ram_detailed,
                .max = 1,
                .help = "Total memory allocated by ML. Press SET for detailed info.",
                .icon_type = IT_ALWAYS_ON,
            },
            {
                .name = allocators[0].name,
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)0,
                .update = mem_pool_display,
            },
            {
                .name = allocators[1].name,
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)1,
                .update = mem_pool_display,
            },
            {
                .name = allocators[2].name,
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)2,
                .update = mem_pool_display,
            },
            {
                .name = "stack space",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)3,
                .update = meminfo_display,
                .help = "Free memory available as stack space for user tasks.",
            },
            {
                .name = "shoot contig",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)4,
                .update = meminfo_display,
                .help = "Largest contiguous block from shoot memory.",
            },
            {
                .name = "shoot total",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)5,
                .update = meminfo_display,
                .help = "Largest fragmented block from shoot memory.",
            },
            #if defined(CONFIG_MEMPATCH_CHECK)
            {
                .name = "AUTOEXEC.BIN",
                .icon_type = IT_ALWAYS_ON,
                .priv = (int*)6,
                .update = meminfo_display,
                .help = "Memory reserved statically at startup for ML binary.",
            },
            #endif
            MENU_EOL
        },
    },
};
#endif

void mem_menu_init()
{
    menu_add("Debug", mem_menus, COUNT(mem_menus));
    //~ console_show();
}

#endif // FEATURE_SHOW_FREE_MEMORY
