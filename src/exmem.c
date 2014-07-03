#define NO_MALLOC_REDIRECT

#include "dryos.h"
#include "bmp.h"
#include "property.h"

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

static volatile void* entire_memory_allocated = 0;

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
    if (hSuite == entire_memory_allocated) entire_memory_allocated = 0;
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
    if (f != (void*) -1)
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
static struct memSuite *shoot_malloc_suite_int(size_t size, int relaxed)
{
    alloc_msg_t *suite_info = _malloc(sizeof(alloc_msg_t));
    
    suite_info->ret = NULL;
    suite_info->timed_out = 0;
    suite_info->size = size;
    suite_info->sem = alloc_sem;
    
    AllocateMemoryResource(size, allocCBR, (unsigned int)suite_info, 0x50);
    
    int r = take_semaphore(suite_info->sem, 1000);
    if (r)
    {
        suite_info->timed_out = 1;
        return NULL;
    }
    
    struct memSuite * hSuite = suite_info->ret;
    _free(suite_info);
    
    if(!relaxed)
    {
        ASSERT((int)size <= hSuite->size);
    }
    
    return hSuite;
}

struct memSuite *_shoot_malloc_suite(size_t size)
{
    if(entire_memory_allocated)
    {
        /* you may need to solder some more RAM chips */
        return 0;
    }

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
        entire_memory_allocated = hSuite;   /* we need to know which memory suite ate all the RAM; when this is freed, we can shoot_malloc again */
        
        return hSuite;
    }
}

struct memSuite * _shoot_malloc_suite_contig(size_t size)
{
    if (entire_memory_allocated)
    {
        /* you may need to solder some more RAM chips */
        return 0;
    }

    if(size > 0)
    {
        alloc_msg_t *suite_info = _malloc(sizeof(alloc_msg_t));
        
        suite_info->ret = NULL;
        suite_info->timed_out = 0;
        suite_info->size = size;
        suite_info->sem = alloc_sem;
        
        AllocateContinuousMemoryResource(size, allocCBR, (unsigned int)suite_info, 0x50);

        int r = take_semaphore(suite_info->sem, 1000);
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
    else
    {
        //~ entire_memory_allocated = (void*) -1;   /* temporary, just mark as busy */

        /* find the largest chunk and try to allocate it */
        struct memSuite * hSuite = _shoot_malloc_suite(0);
        if (!hSuite) return 0;
        
        int max_size = 0;
        struct memChunk * hChunk = GetFirstChunkFromSuite(hSuite);
        while(hChunk)
        {
            int size = GetSizeOfMemoryChunk(hChunk);
            max_size = MAX(max_size, size);
            hChunk = GetNextMemoryChunk(hSuite, hChunk);
        }
        
        _shoot_free_suite(hSuite);
        
        hSuite = _shoot_malloc_suite_contig(max_size);
        entire_memory_allocated = hSuite;   /* we need to know which memory suite ate all the RAM; when this is freed, we can shoot_malloc again */
        return hSuite;
    }
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
    if (hSuite == entire_memory_allocated) entire_memory_allocated = 0;
}

/* just a dummy heuristic for now */
int _shoot_get_free_space()
{
    if (!alloc_sem)
    {
        return 0;
    }
    
    if (entire_memory_allocated)
    {
        return 0;
    }
    else
    {
        return (int)(31.5*1024*1024);
    }
}

#if 0
void exmem_test()
{
    struct memSuite * hSuite = 0;
    struct memChunk * hChunk = 0;
    
    msleep(2000);
    AllocateMemoryResource(1024*1024*32, allocCBR, (unsigned int)&hSuite, 0x50);
    int r = take_semaphore(alloc_sem, 1000);
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

/* after allocating this buffer, you can no longer take pictures; will lock the shutter to prevent it */
PROP_INT(PROP_ICU_UILOCK, uilock);
static uint32_t old_uilock_shutter;

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

struct memSuite * _srm_malloc_suite(int num_requested_buffers)
{
    if (srm_allocated)
    {
        /* only one task can alloc it at a time */
        return 0;
    }
    
    void* buffers[10];
    
    if (num_requested_buffers <= 0)
    {
        /* if you request 0, this means allocate as much as you can */
        num_requested_buffers = COUNT(buffers);
    }
    
    int num_buffers = 0;
    
    /* try to allocate the number of requested buffers (or less, if not possible) */
    for (num_buffers = 0; num_buffers < MIN(num_requested_buffers, COUNT(buffers)); num_buffers++)
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
        return 0;
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
    
    /* after allocating this, you can no longer take pictures (ERR70) */
    /* block the shutter button to avoid it (only touch the shutter-related bits, just in case) */
    old_uilock_shutter = uilock & UILOCK_SHUTTER;
    gui_uilock(uilock | UILOCK_SHUTTER);
    
    srm_allocated = 1;
    return suite;
}

void _srm_free_suite(struct memSuite * suite)
{
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

    /* unlock the shutter button (only touch the shutter-related bits, just in case) */
    int unlocked_shutter = (uilock & ~UILOCK_SHUTTER) | old_uilock_shutter;
    gui_uilock(unlocked_shutter);
    
    srm_allocated = 0;
}

/* similar to shoot_malloc, but limited to a single large buffer for now */
void* _srm_malloc(size_t size)
{
    struct memSuite * theSuite = _srm_malloc_suite(1);
    if (!theSuite) return 0;
    
    /* now we only have to tweak some things so it behaves like plain malloc */
    void* hChunk = (void*) GetFirstChunkFromSuite(theSuite);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    
    /* here we can't request a certain size; we can just check whether we got enough, or not */
    size_t allocated_size = GetSizeOfMemoryChunk(hChunk);
    if (allocated_size < size + 4)
    {
        /* not enough */
        _srm_free_suite(theSuite);
        return 0;
    }
    
    *(struct memSuite **)ptr = theSuite;
    //~ printf("srm_malloc(%s) => %x hSuite=%x\n", format_memory_size(size), ptr+4, theSuite);
    return ptr + 4;
}

void _srm_free(void* ptr)
{
    if (!ptr) return;
    if ((intptr_t)ptr & 3) return;
    struct memSuite * hSuite = *(struct memSuite **)(ptr - 4);
    //~ printf("shoot_free(%x) hSuite=%x\n", ptr, hSuite);
    _srm_free_suite(hSuite);
}

int _srm_get_free_space()
{
    if (srm_allocated)
    {
        /* can't alloc any more */
        return 0;
    }
    
    /* bogus value, slightly larger than real, so the memory backend will try this one */
    /* there's no other way to get such large buffers, so there's nothing to lose by trying it */
    return 40*1024*1024;
}

static void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
    srm_alloc_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
