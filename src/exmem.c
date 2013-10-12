#define NO_MALLOC_REDIRECT

#include "dryos.h"
#include "bmp.h"

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

void shoot_free_suite(struct memSuite * hSuite)
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
    
    FILE *f = FIO_CreateFileEx(file);
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
    
    int r = take_semaphore(suite_info->sem, 100);
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

struct memSuite *shoot_malloc_suite(size_t size)
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
                shoot_free_suite(testSuite);
                max_size = size * 1024 * 1024;
            }
            else
            {
                break;
            }
        }
        /* now free the backup suite. this causes the queued allocation before to get finished. as we timed out, it will get freed immediately in exmem.c:allocCBR */
        shoot_free_suite(backup);
        
        /* allocating max_size + backup_size was reported to fail sometimes */
        struct memSuite * hSuite = shoot_malloc_suite_int(max_size + backup_size - 1024 * 1024, 1);
        entire_memory_allocated = hSuite;   /* we need to know which memory suite ate all the RAM; when this is freed, we can shoot_malloc again */
        
        return hSuite;
    }
}

struct memSuite * shoot_malloc_suite_contig(size_t size)
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
    else
    {
        //~ entire_memory_allocated = (void*) -1;   /* temporary, just mark as busy */

        /* find the largest chunk and try to allocate it */
        struct memSuite * hSuite = shoot_malloc_suite(0);
        if (!hSuite) return 0;
        
        int max_size = 0;
        struct memChunk * hChunk = GetFirstChunkFromSuite(hSuite);
        while(hChunk)
        {
            int size = GetSizeOfMemoryChunk(hChunk);
            max_size = MAX(max_size, size);
            hChunk = GetNextMemoryChunk(hSuite, hChunk);
        }
        
        shoot_free_suite(hSuite);
        
        hSuite = shoot_malloc_suite_contig(max_size);
        entire_memory_allocated = hSuite;   /* we need to know which memory suite ate all the RAM; when this is freed, we can shoot_malloc again */
        return hSuite;
    }
}

void* _shoot_malloc(size_t size)
{
    struct memSuite * theSuite = shoot_malloc_suite_contig(size + 4);
    if (!theSuite) return 0;
    
    /* now we only have to tweak some things so it behaves like plain malloc */
    void* hChunk = (void*) GetFirstChunkFromSuite(theSuite);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    *(struct memSuite **)ptr = theSuite;
    return ptr + 4;
}

void _shoot_free(void* ptr)
{
    if (!ptr) return;
    if ((intptr_t)ptr & 3) return;
    struct memSuite * hSuite = *(struct memSuite **)(ptr - 4);
    FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore(free_sem, 0);
    if (hSuite == entire_memory_allocated) entire_memory_allocated = 0;
}

/* just a dummy heuristic for now */
int _shoot_get_free_space()
{
    if (entire_memory_allocated)
    {
        return 0;
    }
    else
    {
        return 30*1024*1024;
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

static void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
