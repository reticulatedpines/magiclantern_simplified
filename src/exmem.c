#define NO_MALLOC_REDIRECT

#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"
#include "raw.h"

extern void* _malloc(size_t size);
extern void _free(void* ptr);

/* semaphore from mem.c, used for these routines as well */
extern struct semaphore *mem_sem;

/* the CBRs are called from Canon's RscMgr task */
THREAD_ROLE(RscMgr);

/* semaphore routines used more like event flags or message queues
 * (not checked for thread safety)
 */
static inline NO_THREAD_SAFETY_ANALYSIS
int take_semaphore_nc(struct semaphore *sem, int timeout)
{
    return take_semaphore(sem, timeout);
}

static inline NO_THREAD_SAFETY_ANALYSIS
int give_semaphore_nc(struct semaphore *sem)
{
    return give_semaphore(sem);
}

// experimental memory allocation from shooting buffer (~160MB on 5D2)

typedef struct
{
    struct memSuite *ret;
    uint32_t size;
    uint32_t timed_out;
    struct semaphore *sem;
} alloc_msg_t;

static struct semaphore *alloc_sem = 0;
static struct semaphore *free_sem = 0;

int GetNumberOfChunks(struct memSuite *suite)
{
    CHECK_SUITE_SIGNATURE(suite);
    return suite->num_chunks;
}

int GetSizeOfMemorySuite(struct memSuite *suite)
{
    CHECK_SUITE_SIGNATURE(suite);
    return suite->size;
}

int GetSizeOfMemoryChunk(struct memChunk *chunk)
{
    CHECK_CHUNK_SIGNATURE(chunk);
    return chunk->size;
}

static void freeCBR(unsigned int a)
{
    give_semaphore_nc(free_sem);
}

static void freeCBR_nowait(unsigned int a)
{
}

REQUIRES(mem_sem)
void _shoot_free_suite(struct memSuite *hSuite)
{
    if (hSuite != NULL)
    {
        // FreeMemoryResource is not null pointer safe on D678, crashes
        FreeMemoryResource(hSuite, freeCBR, 0);
    }
    take_semaphore_nc(free_sem, 0);
}

static void allocCBR(unsigned int priv, struct memSuite *hSuite)
{
    alloc_msg_t *suite_info = (alloc_msg_t *)priv;
    
    /* in case we timed out last time, immediately free the newly allocated suite (its the one that timed out) */
    if(suite_info->timed_out)
    {
        if (hSuite != NULL)
        {
            // FreeMemoryResource is not null pointer safe on D678, crashes
            FreeMemoryResource(hSuite, freeCBR_nowait, 0);
        }
        _free(suite_info);
        return;
    }
    
    suite_info->ret = hSuite;
    give_semaphore_nc(suite_info->sem);
}

unsigned int exmem_save_buffer(struct memSuite *hSuite, char *file)
{
    unsigned int written = 0;
    
    FILE *f = FIO_CreateFile(file);
    if (f)
    {
        struct memChunk *currentChunk;
        unsigned char *chunkAddress;
        unsigned int chunkAvail;
        
        currentChunk = GetFirstChunkFromSuite(hSuite);
        
        while(currentChunk)
        {
            chunkAvail = GetSizeOfMemoryChunk(currentChunk);
            chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(currentChunk);
            
            FIO_WriteFile(f, chunkAddress, chunkAvail);
            written += chunkAvail;
            currentChunk = GetNextMemoryChunk(hSuite, currentChunk);
        }
        FIO_CloseFile(f);
    }
    
    return written;
}

unsigned int exmem_clear(struct memSuite *hSuite, char fill)
{
    unsigned int written = 0;
    
    struct memChunk *currentChunk;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    
    currentChunk = GetFirstChunkFromSuite(hSuite);
    
    while(currentChunk)
    {
        chunkAvail = GetSizeOfMemoryChunk(currentChunk);
        chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(currentChunk);
        
        memset(CACHEABLE(chunkAddress), fill, chunkAvail);
        written += chunkAvail;
        currentChunk = GetNextMemoryChunk(hSuite, currentChunk);
    }
    return written;
}


/* when size is set to zero, it will try to allocate the maximum possible block */
static struct memSuite *shoot_malloc_suite_int(size_t size)
{
    alloc_msg_t *suite_info = _malloc(sizeof(alloc_msg_t));
    
    suite_info->ret = NULL;
    suite_info->timed_out = 0;
    suite_info->size = size;
    suite_info->sem = alloc_sem;
    
    AllocateMemoryResource(size, allocCBR, (unsigned int)suite_info, 0x50);
    
    int r = take_semaphore_nc(suite_info->sem, 100);
    if (r)
    {
        // signal to allocCBR that it needs to free suite_info
        suite_info->timed_out = 1;
        return NULL;
    }
    
    struct memSuite *hSuite = suite_info->ret;
    _free(suite_info);
    
    ASSERT((int)size <= hSuite->size);
    
    return hSuite;
}

static size_t largest_chunk_size(struct memSuite *hSuite)
{
    int max_size = 0;
    struct memChunk *hChunk = GetFirstChunkFromSuite(hSuite);
    while(hChunk)
    {
        int size = GetSizeOfMemoryChunk(hChunk);
        max_size = MAX(max_size, size);
        hChunk = GetNextMemoryChunk(hSuite, hChunk);
    }
    return max_size;
}

static size_t shoot_malloc_autodetect()
{
    /* allocate some backup that will service the queued allocation request that fails during the loop */
    size_t backup_size = 4 * 1024 * 1024;
    size_t max_size = 0;
    struct memSuite *backup = shoot_malloc_suite_int(backup_size);

    if (backup == NULL)
        return max_size;

    for (int size_mb = 4; size_mb < 1024; size_mb += 4)
    {
        int tested_size = size_mb * 1024 * 1024;
        //qprintf("[shoot_malloc] trying %s\n", format_memory_size(tested_size));
        struct memSuite *testSuite = shoot_malloc_suite_int(tested_size);
        if (testSuite)
        {
            /* leave 1MB unallocated, just in case */
            max_size = tested_size + backup_size - 1024 * 1024;
            _shoot_free_suite(testSuite);
        }
        else
        {
            //qprintf("[shoot_malloc] memory full\n");
            break;
        }
    }
    /* now free the backup suite. this causes the queued allocation before to get finished. as we timed out, it will get freed immediately in exmem.c:allocCBR */
    _shoot_free_suite(backup);
    
    //qprintf("[shoot_malloc] autodetected size: %s\n", format_memory_size(max_size));
    return max_size;
}

static size_t shoot_malloc_autodetect_contig(uint32_t requested_size)
{
    /* allocate some backup that will service the queued allocation request that fails during the loop */
    size_t backup_size = 1024 * 1024;
    size_t max_contig_size = 0;
    struct memSuite *backup = shoot_malloc_suite_int(backup_size);

    if (backup == NULL)
        return max_contig_size;

    for (int size_mb = 1; size_mb < 1024; size_mb++)
    {
        int tested_size = size_mb * 1024 * 1024;
        //qprintf("[shoot_contig] trying %s\n", format_memory_size(tested_size));
        struct memSuite *testSuite = shoot_malloc_suite_int(tested_size);
        if (testSuite)
        {
            /* find largest chunk */
            max_contig_size = largest_chunk_size(testSuite);
 
            _shoot_free_suite(testSuite);

            if (requested_size && requested_size <= max_contig_size)
            {
                /* have we already got a contiguous block large enough? */
                break;
            }
        }
        else
        {
            //qprintf("[shoot_contig] memory full\n");
            break;
        }
    }
    /* now free the backup suite. this causes the queued allocation before to get finished. as we timed out, it will get freed immediately in exmem.c:allocCBR */
    _shoot_free_suite(backup);
    
    //qprintf("[shoot_contig] autodetected size: %s (requested %x)\n", format_memory_size(max_contig_size), requested_size);

    return max_contig_size;
}

REQUIRES(mem_sem)
struct memSuite *_shoot_malloc_suite(size_t size)
{
    if(size)
    {
        /* allocate exact memory size */
        return shoot_malloc_suite_int(size, 0);
    }
    else
    {
        /* allocate some backup that will service the queued allocation request that fails during the loop */
        int backup_size = 8 * 1024 * 1024;
        int max_size = 0;
        struct memSuite *backup = shoot_malloc_suite_int(backup_size, 0);

        for(int size = 4; size < 1024; size += 4)
        {
            struct memSuite *testSuite = shoot_malloc_suite_int(size * 1024 * 1024, 1);
            if(testSuite)
            {
                _shoot_free_suite(testSuite);
                max_size = size * 1024 * 1024;
            }
            else
            {
                break;
            }
        }
        /* now free the backup suite. this causes the queued allocation before to get finished. as we timed out, it will get freed immediately in exmem.c:allocCBR */
        _shoot_free_suite(backup);
        
        /* allocating max_size + backup_size was reported to fail sometimes */
        struct memSuite * hSuite = shoot_malloc_suite_int(max_size + backup_size - 1024 * 1024, 1);
        
        return hSuite;
    }
}

REQUIRES(mem_sem)
struct memSuite * _shoot_malloc_suite_contig(size_t size)
{
    //qprintf("_shoot_malloc_suite_contig(%x)\n", size);

    if (size == 0 || size > 1024*1024)
    {
        /* check whether we can allocate a block with the requested size */
        size_t max_size = shoot_malloc_autodetect_contig(size);

        if (size && size > max_size)
        {
            /* requested more than we can handle? give up */
            return 0;
        }
        if (size == 0)
        {
            /* allocate as much as we can */
            size = max_size;
        }
    }

    /* we now know for sure how much to allocate */
    //qprintf("_shoot_malloc_suite_contig(trying %x)\n", size);

    alloc_msg_t *suite_info = _malloc(sizeof(alloc_msg_t));
    
    suite_info->ret = NULL;
    suite_info->timed_out = 0;
    suite_info->size = size;
    suite_info->sem = alloc_sem;
    
    AllocateContinuousMemoryResource(size, allocCBR, (unsigned int)suite_info, 0x50);

    int r = take_semaphore(suite_info->sem, 100);
    if (r)
    {
        suite_info->timed_out = 1;
        return NULL;
    }
    
    struct memSuite * hSuite = suite_info->ret;
    _free(suite_info);
    
    ASSERT((int)size <= hSuite->size);
    return hSuite;
}

REQUIRES(mem_sem)
void* _shoot_malloc(size_t size)
{
    struct memSuite *theSuite = _shoot_malloc_suite_contig(size + 4);
    if (!theSuite)
        return 0;
    
    /* now we only have to tweak some things so it behaves like plain malloc */
    void* hChunk = (void*)GetFirstChunkFromSuite(theSuite);
    void* ptr = (void*)GetMemoryAddressOfMemoryChunk(hChunk);
    *(struct memSuite **)ptr = theSuite;
    //~ printf("shoot_malloc(%s) => %x hSuite=%x\n", format_memory_size(size), ptr+4, theSuite);
    return ptr + 4;
}

REQUIRES(mem_sem)
void _shoot_free(void *ptr)
{
    if (!ptr)
        return;
    if ((intptr_t)ptr & 3)
        return;
    struct memSuite *hSuite = *(struct memSuite **)(ptr - 4);
    //~ printf("shoot_free(%x) hSuite=%x\n", ptr, hSuite);
    if (hSuite != NULL)
        FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore_nc(free_sem, 0);
}

/* just a dummy heuristic for now */
REQUIRES(mem_sem)
int _shoot_get_free_space()
{
    if (!alloc_sem)
    {
        return 0;
    }

    /* fixme: should fail quickly when shoot memory is full */
    /* performing test allocations is usually very slow */
    return (int)(31.5 * 1024 * 1024);
}

#if 0
void exmem_test()
{
    struct memSuite *hSuite = 0;
    struct memChunk *hChunk = 0;
    
    msleep(2000);
    AllocateMemoryResource(1024*1024*32, allocCBR, (unsigned int)&hSuite, 0x50);
    int r = take_semaphore_nc(alloc_sem, 100);
    if (r)
        return;
    
    if(!hSuite)
    {
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 0, 0, "Alloc Fail");
        return;
    }
    hChunk = GetFirstChunkFromSuite(hSuite);
    int num = 0;
    
    bmp_printf(FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 0, 0, "C:%d S:0x%08X", GetNumberOfChunks(hSuite), GetSizeOfMemorySuite(hSuite) );
    while(hChunk)
    {
        if(num > 13)
        {
            num = 13;
        }
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 0, 30 + num * 20, 
            "[%d] A:0x%08X S:0x%08X R:0x%08X", num, GetMemoryAddressOfMemoryChunk(hChunk), GetSizeOfMemoryChunk(hChunk), GetRemainOfMemoryChunk(hChunk));
        hChunk = GetNextMemoryChunk(hSuite, hChunk);
        num++;
    } 
    bmp_printf(FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 0, 30 + num++ * 20, "Done");

    FreeMemoryResource(hSuite, freeCBR, 0);
}
#endif

/* SRM job memory */

/* These buffers must be freed in the same order as allocated. */
/* To keep things simple, let's allocate the entire SRM memory on first call (all or nothing) */
static GUARDED_BY(mem_sem) int srm_allocated = 0;

/* There are a few fixed-size buffers; the exact size is a camera-specific constant
 * SRM_BUFFER_SIZE = RAW buffer size, usually around 30-40 MB
 * it's hardcoded in consts.h for speed, since autodetection is slow
 */
static GUARDED_BY(mem_sem) struct
{
    void *buffer;
    int used;
} srm_buffers[16] = {{0}};

/* used to know when allocation was done */
static struct semaphore *srm_alloc_sem = 0;

/* providers of dummy stubs for builds with disabled SRM */
#ifdef CONFIG_MEMORY_SRM_NOT_WORKING
void SRM_AllocateMemoryResourceFor1stJob(
    void (*callback)(void** dst_ptr, void* raw_buffer, uint32_t raw_buffer_size),
    void** dst_ptr)
{
    DryosDebugMsg(0, 15, "SRM_AllocateMemoryResourceFor1stJob disabled");
}
void SRM_FreeMemoryResourceFor1stJob(
    void* raw_buffer,
    int unk1_zero,
    int unk2_zero)
{
    DryosDebugMsg(0, 15, "SRM_FreeMemoryResourceFor1stJob disabled");
}
#endif

/* called from RscMgr task */
static REQUIRES(RscMgr)
void srm_malloc_cbr(void** dst_ptr, void* raw_buffer, uint32_t raw_buffer_size)
{
    //printf("srm_malloc_cbr(%x, %x, %x)\n", dst_ptr, raw_buffer, raw_buffer_size);

    /* we can't tell how much to allocate; the allocator tells us */
    /* the value is hardcoded in consts.h, for speed (probing is very slow) */
    ASSERT(SRM_BUFFER_SIZE == raw_buffer_size);
    
    /* return the newly allocated buffer in the output variable */
    *dst_ptr = raw_buffer;
    
    /* announce it's done */
    give_semaphore_nc(srm_alloc_sem);
}

/* after allocating this buffer, you can no longer take pictures (ERR70); will lock the shutter to prevent it */
static void srm_shutter_lock()
{
    /* block the shutter button to avoid ERR70 */
    /* (only touch the shutter-related bits, just in case) */
    gui_uilock(icu_uilock | UILOCK_SHUTTER);
}

static void srm_shutter_unlock()
{
    /* unlock the shutter button */
    /* (only touch the shutter-related bits, just in case) */
    gui_uilock(icu_uilock & ~UILOCK_SHUTTER);
}

static REQUIRES(mem_sem)
void srm_alloc_internal()
{
    printf("[SRM] alloc all buffers\n");

    if (srm_allocated || lens_info.job_state)
    {
        /* already allocated, or picture taking in progress */
        return;
    }

    srm_shutter_lock();
    msleep(50);

    if (srm_allocated || lens_info.job_state)
    {
        /* did you manage to press the shutter meanwhile? */
        /* you should go to a race contest :) */
        return;
    }

    /* clear previous state */
    memset(srm_buffers, 0, sizeof(srm_buffers));

    /* try to allocate as many buffers as we can */
    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        /* allocate a large contiguous buffer, normally used for RAW photo capture */
        SRM_AllocateMemoryResourceFor1stJob(srm_malloc_cbr, &srm_buffers[i].buffer);
        int err = take_semaphore_nc(srm_alloc_sem, 100);

        if (err)
        {
            /* the call will time out when there's no more RAM, and the request will be dropped */
            /* (unlike shoot_malloc, here it won't trigger the CBR after freeing something) */

            /* all SRM memory allocated => Canon code already locked the shutter for us */
            /* (we still need the lock active while allocating, to pass the race condition test) */
            srm_shutter_unlock();

            break;
        }

        printf("[SRM] buffer %x\n", srm_buffers[i].buffer);
    }

    srm_allocated = 1;
}

static REQUIRES(mem_sem)
void srm_free_internal()
{
    /* any buffers still used? */
    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        if (srm_buffers[i].used)
        {
            return;
        }
    }

    printf("[SRM] free all buffers\n");

    /* free all SRM buffers */
    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        if (srm_buffers[i].buffer)
        {
            ASSERT(!srm_buffers[i].used);
            SRM_FreeMemoryResourceFor1stJob(srm_buffers[i].buffer, 0, 0);
            srm_buffers[i].buffer = 0;
        }
    }

    srm_shutter_unlock();

    srm_allocated = 0;
}

REQUIRES(mem_sem)
struct memSuite * _srm_malloc_suite(int num_requested_buffers)
{
    printf("srm_malloc_suite(%d)...\n", num_requested_buffers);

    /* alloc from operating system, if needed */
    srm_alloc_internal();

    if (num_requested_buffers == 0)
    {
        /* if you request 0, this means allocate as much as you can */
        num_requested_buffers = COUNT(srm_buffers);
    }

    /* pack the buffers into a memory suite, so they can be used in the same way as with shoot_malloc_suite */
    struct memSuite *suite = 0;

    for (int i = 0; i < COUNT(srm_buffers) && num_requested_buffers; i++)
    {
        if (srm_buffers[i].buffer && !srm_buffers[i].used)
        {
            if (!suite)
            {
                /* first buffer */
                suite = CreateMemorySuite(srm_buffers[i].buffer, SRM_BUFFER_SIZE, 0);
            }
            else
            {
                /* subsequent buffers */
                struct memChunk *chunk = CreateMemoryChunk(srm_buffers[i].buffer, SRM_BUFFER_SIZE, 0);
                ASSERT(chunk);
                AddMemoryChunk(suite, chunk);
            }

            /* mark as used */
            srm_buffers[i].used = 1;

            /* next */
            num_requested_buffers--;
        }
    }
    
    printf("srm_malloc_suite => %x\n", suite);
    return suite;
}

REQUIRES(mem_sem)
void _srm_free_suite(struct memSuite *suite)
{
    if (suite == NULL)
    {
        DryosDebugMsg(0, 15, "WARNING: suite was NULL");
        // SJE should we do srm_shutter_unlock() and clear srm_allocated,
        // as below?  Probably not, but I don't know how to handle this
        // error case.
        return;
    }
        
    printf("srm_free_suite(%x)\n", suite);

    struct memChunk *chunk = GetFirstChunkFromSuite(suite);

    while(chunk)
    {
        /* make sure we have a suite returned by srm_malloc_suite */
        uint32_t size = GetSizeOfMemoryChunk(chunk);
        ASSERT(size == SRM_BUFFER_SIZE);

        /* mark each chunk as free in our internal SRM buffer list */
        void* buf = GetMemoryAddressOfMemoryChunk(chunk);

        for (int i = 0; i < COUNT(srm_buffers); i++)
        {
            if (srm_buffers[i].buffer == buf)
            {
                ASSERT(srm_buffers[i].used);
                srm_buffers[i].used = 0;
            }
        }
        
        chunk = GetNextMemoryChunk(suite, chunk);
    }
    
    /* after freeing the big buffers, this will free the memory suite and the chunks (the data structures) */
    DeleteMemorySuite(suite);

    /* free to operating system, if needed */
    srm_free_internal();
}

/* malloc-like wrapper for the SRM buffers */

/* similar to shoot_malloc, but limited to a single large buffer for now */
REQUIRES(mem_sem)
void* _srm_malloc(size_t size)
{
    /* alloc from operating system, if needed */
    srm_alloc_internal();

    /* find the first unused buffer */
    void *buffer = 0;
    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        if (srm_buffers[i].buffer && !srm_buffers[i].used)
        {
            buffer = srm_buffers[i].buffer;
            srm_buffers[i].used = 1;
            break;
        }
    }
    
    /* here we can't request a certain size; we can just check whether we got enough, or not */
    /* note: size checking is done after allocating, in order to simplify the deallocation code */
    if (SRM_BUFFER_SIZE < size + 4)
    {
        /* not enough */
        _srm_free(buffer);
        return 0;
    }
    
    return buffer;
}

REQUIRES(mem_sem)
void _srm_free(void* ptr)
{
    if (!ptr)
        return;

    /* identify the buffer from its pointer, and mark it as unused */
    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        if (srm_buffers[i].used && srm_buffers[i].buffer == ptr)
        {
            srm_buffers[i].used = 0;
        }
    }

    /* deallocate the suite from the system, if needed */
    srm_free_internal();
}

REQUIRES(mem_sem)
int _srm_get_max_region()
{
    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        if (srm_buffers[i].buffer && !srm_buffers[i].used)
        {
            return SRM_BUFFER_SIZE;
        }
    }

    return 0;
}

REQUIRES(mem_sem)
int _srm_get_free_space()
{
    int free_space = 0;

    for (int i = 0; i < COUNT(srm_buffers); i++)
    {
        if (srm_buffers[i].buffer && !srm_buffers[i].used)
        {
            free_space += SRM_BUFFER_SIZE;
        }
    }

    /* assume we have at least one buffer */
    return free_space;
}

static void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
    srm_alloc_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
