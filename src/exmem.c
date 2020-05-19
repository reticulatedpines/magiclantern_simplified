#define NO_MALLOC_REDIRECT

#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"

extern void* _malloc(size_t size);
extern void _free(void* ptr);

// experimental memory allocation from shooting buffer (~160MB on 5D2)

typedef struct
{
    struct memSuite *ret;
    uint32_t size;
    uint32_t timed_out;
    struct semaphore * sem;
} alloc_msg_t;

static struct semaphore * alloc_sem = 0;
static struct semaphore * free_sem = 0;

int GetNumberOfChunks(struct memSuite * suite)
{
    CHECK_SUITE_SIGNATURE(suite);
    return suite->num_chunks;
}

int GetSizeOfMemorySuite(struct memSuite * suite)
{
    CHECK_SUITE_SIGNATURE(suite);
    return suite->size;
}

int GetSizeOfMemoryChunk(struct memChunk * chunk)
{
    CHECK_CHUNK_SIGNATURE(chunk);
    return chunk->size;
}

static void freeCBR(unsigned int a)
{
    give_semaphore(free_sem);
}

static void freeCBR_nowait(unsigned int a)
{
}

void _shoot_free_suite(struct memSuite * hSuite)
{
    FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore(free_sem, 0);
}

static void allocCBR(unsigned int priv, struct memSuite * hSuite)
{
    alloc_msg_t *suite_info = (alloc_msg_t *)priv;
    
    /* in case we timed out last time, immediately free the newly allocated suite (its the one that timed out) */
    if(suite_info->timed_out)
    {
        FreeMemoryResource(hSuite, freeCBR_nowait, 0);
        _free(suite_info);
        return;
    }
    
    suite_info->ret = hSuite;
    give_semaphore(suite_info->sem);
}

unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file)
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

unsigned int exmem_clear(struct memSuite * hSuite, char fill)
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

static size_t largest_chunk_size(struct memSuite * hSuite)
{
    int max_size = 0;
    struct memChunk * hChunk = GetFirstChunkFromSuite(hSuite);
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
    struct memSuite * backup = shoot_malloc_suite_int(backup_size);

    for (int size_mb = 4; size_mb < 1024; size_mb += 4)
    {
        int tested_size = size_mb * 1024 * 1024;
        //qprintf("[shoot_malloc] trying %s\n", format_memory_size(tested_size));
        struct memSuite * testSuite = shoot_malloc_suite_int(tested_size);
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
    struct memSuite * backup = shoot_malloc_suite_int(backup_size);

    for (int size_mb = 1; size_mb < 1024; size_mb++)
    {
        int tested_size = size_mb * 1024 * 1024;
        //qprintf("[shoot_contig] trying %s\n", format_memory_size(tested_size));
        struct memSuite * testSuite = shoot_malloc_suite_int(tested_size);
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

struct memSuite *_shoot_malloc_suite(size_t size)
{
    //qprintf("_shoot_malloc_suite(%x)\n", size);

    if(size)
    {
        /* allocate exact memory size */
        return shoot_malloc_suite_int(size);
    }
    else
    {
        /* allocate as much as we can */
        size_t max_size = shoot_malloc_autodetect();
        return shoot_malloc_suite_int(max_size);
    }
}

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

void* _shoot_malloc(size_t size)
{
    struct memSuite * theSuite = _shoot_malloc_suite_contig(size + 4);
    if (!theSuite) return 0;
    
    /* now we only have to tweak some things so it behaves like plain malloc */
    void* hChunk = (void*) GetFirstChunkFromSuite(theSuite);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    *(struct memSuite **)ptr = theSuite;
    //~ printf("shoot_malloc(%s) => %x hSuite=%x\n", format_memory_size(size), ptr+4, theSuite);
    return ptr + 4;
}

void _shoot_free(void* ptr)
{
    if (!ptr) return;
    if ((intptr_t)ptr & 3) return;
    struct memSuite * hSuite = *(struct memSuite **)(ptr - 4);
    //~ printf("shoot_free(%x) hSuite=%x\n", ptr, hSuite);
    FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore(free_sem, 0);
}

/* just a dummy heuristic for now */
int _shoot_get_free_space()
{
    if (!alloc_sem)
    {
        return 0;
    }

    /* fixme: should fail quickly when shoot memory is full */
    /* performing test allocations is usually very slow */
    return (int)(31.5*1024*1024);
}

#if 0
void exmem_test()
{
    struct memSuite * hSuite = 0;
    struct memChunk * hChunk = 0;
    
    msleep(2000);
    AllocateMemoryResource(1024*1024*32, allocCBR, (unsigned int)&hSuite, 0x50);
    int r = take_semaphore(alloc_sem, 100);
    if (r) return;
    
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
/* To keep things simple, let's allow a single allocation call at a time */
/* (no other tasks will be able to use it until the original task frees what it got) */
static int srm_allocated = 0;

/* There are a few fixed-size buffers; the exact size is camera-specific (RAW buffer size, 30-40 MB) */
/* and will be detected upon first allocation */
static uint32_t srm_buffer_size = 0;

/* used to know when allocation was done */
static struct semaphore * srm_alloc_sem = 0;

static void srm_malloc_cbr(void** dst_ptr, void* raw_buffer, uint32_t raw_buffer_size)
{
    if (!srm_buffer_size)
    {
        /* we can't tell how much to allocate; the allocator tells us */
        srm_buffer_size = raw_buffer_size;
    }
    else
    {
        /* it should tell us the same thing every time */
        ASSERT(srm_buffer_size == raw_buffer_size);
    }
    
    /* return the newly allocated buffer in the output variable */
    *dst_ptr = raw_buffer;
    
    /* announce it's done */
    give_semaphore(srm_alloc_sem);
}

/* after allocating this buffer, you can no longer take pictures (ERR70); will lock the shutter to prevent it */
static uint32_t old_uilock_shutter;

static void srm_shutter_lock()
{
    /* block the shutter button to avoid ERR70 */
    /* (only touch the shutter-related bits, just in case) */
    old_uilock_shutter = icu_uilock & UILOCK_SHUTTER;
    gui_uilock(icu_uilock | UILOCK_SHUTTER);
}

static void srm_shutter_unlock()
{
    /* unlock the shutter button */
    /* (only touch the shutter-related bits, just in case) */
    int unlocked_shutter = (icu_uilock & ~UILOCK_SHUTTER) | old_uilock_shutter;
    gui_uilock(unlocked_shutter);
}

struct memSuite * _srm_malloc_suite(int num_requested_buffers)
{
    printf("srm_malloc_suite(%d)...\n", num_requested_buffers);

    if (srm_allocated)
    {
        /* only one task can alloc it at a time */
        return 0;
    }
    
    if (lens_info.job_state)
    {
        /* this can't work in parallel with taking pictures */
        return 0;
    }
    
    srm_shutter_lock();
    msleep(50);

    if (lens_info.job_state)
    {
        /* did you manage to press the shutter meanwhile? */
        /* you should go to a race contest :) */
        srm_shutter_unlock();
        return 0;
    }

    void* buffers[16];
    
    if (num_requested_buffers <= 0 || num_requested_buffers > COUNT(buffers))
    {
        /* if you request 0, this means allocate as much as you can */
        num_requested_buffers = COUNT(buffers);
    }
    
    int num_buffers = 0;
    
    /* try to allocate the number of requested buffers (or less, if not possible) */
    for (num_buffers = 0; num_buffers < num_requested_buffers; num_buffers++)
    {
        /* allocate a large contiguous buffer, normally used for RAW photo capture */
        buffers[num_buffers] = 0;
        SRM_AllocateMemoryResourceFor1stJob(srm_malloc_cbr, &buffers[num_buffers]);
        int err = take_semaphore(srm_alloc_sem, 100);

        if (err)
        {
            /* the call will time out when there's no more RAM, and the request will be dropped */
            /* (unlike shoot_malloc, here it won't trigger the CBR after freeing something) */
            break;
        }
    }
    
    if (num_buffers == 0)
    {
        srm_shutter_unlock();
        return 0;
    }
    
    if (num_requested_buffers == COUNT(buffers))
    {
        /* all SRM memory allocated => Canon code already locked the shutter for us */
        /* (we still need the lock active while allocating, to pass the race condition test) */
        srm_shutter_unlock();
    }
    
    /* pack the buffers into a memory suite, so they can be used in the same way as with shoot_malloc_suite */
    struct memSuite * suite = CreateMemorySuite(buffers[0], srm_buffer_size, 0);
    ASSERT(suite);
    
    for (int i = 1; i < num_buffers; i++)
    {
        struct memChunk * chunk = CreateMemoryChunk(buffers[i], srm_buffer_size, 0);
        ASSERT(chunk);
        AddMemoryChunk(suite, chunk);
    }
    
    printf("srm_malloc_suite => %x\n", suite);
    srm_allocated = 1;
    return suite;
}

void _srm_free_suite(struct memSuite * suite)
{
    printf("srm_free_suite(%x)\n", suite);
    
    struct memChunk * chunk = GetFirstChunkFromSuite(suite);

    while(chunk)
    {
        /* make sure we have a suite returned by srm_malloc_suite */
        uint32_t size = GetSizeOfMemoryChunk(chunk);
        ASSERT(size == srm_buffer_size);
        
        /* we need to delete each chunk in exactly the same order as we have allocated them */
        void* buf = GetMemoryAddressOfMemoryChunk(chunk);
        SRM_FreeMemoryResourceFor1stJob(buf, 0, 0);
        
        chunk = GetNextMemoryChunk(suite, chunk);
    }
    
    /* after freeing the big buffers, this will free the memory suite and the chunks (the data structures) */
    DeleteMemorySuite(suite);
    
    srm_shutter_unlock();
    srm_allocated = 0;
}

/* malloc-like wrapper for the SRM buffers */
struct srm_malloc_buf
{
    void* buffer;
    int used;
};

static struct memSuite * srm_malloc_hSuite = 0;
static struct srm_malloc_buf srm_malloc_buffers[10] = {{0}};

/* similar to shoot_malloc, but limited to a single large buffer for now */
void* _srm_malloc(size_t size)
{
    if (!srm_malloc_hSuite)
    {
        /*
         * allocate everything on first call
         * (since other tasks can't allocate from this buffer anymore anyway,
         * there's nothing to lose - other than 100ms for autodetection
         */
        srm_malloc_hSuite = _srm_malloc_suite(0);
        
        if (!srm_malloc_hSuite)
        {
            /* what the duck? */
            return 0;
        }
        
        /* let's see what we've got here */
        struct memChunk * chunk = GetFirstChunkFromSuite(srm_malloc_hSuite);
        int i = 0;

        while(chunk)
        {
            /* make sure we have a suite returned by srm_malloc_suite */
            uint32_t size = GetSizeOfMemoryChunk(chunk);
            ASSERT(size == srm_buffer_size);
            
            /* populate the buffers available for this large malloc, and mark them as unused */
            void* buffer = GetMemoryAddressOfMemoryChunk(chunk);
            ASSERT(buffer);
            srm_malloc_buffers[i].buffer = buffer;
            srm_malloc_buffers[i].used = 0;
            
            chunk = GetNextMemoryChunk(srm_malloc_hSuite, chunk);
            i++;
        }
    }
    
    /* find the first unused buffer */
    void* buffer = 0;
    for (int i = 0; i < srm_malloc_hSuite->num_chunks; i++)
    {
        if (!srm_malloc_buffers[i].used)
        {
            buffer = srm_malloc_buffers[i].buffer;
            srm_malloc_buffers[i].used = 1;
            break;
        }
    }
    
    /* here we can't request a certain size; we can just check whether we got enough, or not */
    /* note: size checking is done after allocating, in order to simplify the deallocation code */
    if (srm_buffer_size < size + 4)
    {
        /* not enough */
        _srm_free(buffer);
        return 0;
    }
    
    return buffer;
}

void _srm_free(void* ptr)
{
    if (!ptr) return;

    /* identify the buffer from its pointer, and mark it as unused */
    /* also count how many used buffers are left */
    int buffers_used = 0;
    for (int i = 0; i < srm_malloc_hSuite->num_chunks; i++)
    {
        if (srm_malloc_buffers[i].used && srm_malloc_buffers[i].buffer == ptr)
        {
            srm_malloc_buffers[i].used = 0;
        }
        
        if (srm_malloc_buffers[i].used)
        {
            buffers_used++;
        }
    }

    if (buffers_used == 0)
    {
        /* no more buffers used? deallocate the suite from the system */
        _srm_free_suite(srm_malloc_hSuite);
        srm_malloc_hSuite = 0;
    }
}

int _srm_get_max_region()
{
    if (!srm_buffer_size)
    {
        /* do a quick test malloc just to check the size */
        void* test_suite = _srm_malloc_suite(1);
        if (test_suite) _srm_free_suite(test_suite);
    }
    
    return srm_buffer_size;
}

int _srm_get_free_space()
{
    if (!srm_malloc_hSuite)
    {
        if (srm_allocated)
        {
            /* somebody else already allocated everything */
            return 0;
        }
    }
    
    if (!srm_buffer_size)
    {
        /* do a quick test malloc just to check the size */
        void* test_suite = _srm_malloc_suite(1);
        if (test_suite) _srm_free_suite(test_suite);
    }
    
    if (srm_malloc_hSuite)
    {
        /* we already have some malloc's from this buffer; let's see if there's anything left */
        int buffers_used = 0;
        for (int i = 0; i < srm_malloc_hSuite->num_chunks; i++)
        {
            if (srm_malloc_buffers[i].used)
            {
                buffers_used++;
            }
        }
        
        int buffers_free = srm_malloc_hSuite->num_chunks - buffers_used;
        return srm_buffer_size * buffers_free;
    }
    
    /* assume we have at least one buffer */
    return srm_buffer_size;
}

static void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
    srm_alloc_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
