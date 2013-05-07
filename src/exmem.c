#include "dryos.h"
#include "bmp.h"

// experimental memory allocation from shooting buffer (~160MB on 5D2)

static int alloc_sem_timed_out = 0;
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

void shoot_free_suite(struct memSuite * hSuite)
{
    FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore(free_sem, 0);
}

static void allocCBR(unsigned int a, struct memSuite *hSuite)
{
    /* in case we timed out last time, immediately free the newly allocated suite (its the one that timed out) */
    if(alloc_sem_timed_out)
    {
        alloc_sem_timed_out = 0;
        shoot_free_suite(hSuite);
        return;
    }
    MEM(a) = (unsigned int)hSuite;
    give_semaphore(alloc_sem);
}

unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file)
{
    unsigned int written = 0;
#ifdef CONFIG_FULL_EXMEM_SUPPORT
    
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
    
#endif
    return written;
}

unsigned int exmem_clear(struct memSuite * hSuite, char fill)
{
    unsigned int written = 0;
#ifdef CONFIG_FULL_EXMEM_SUPPORT
    
    struct memChunk *currentChunk;
    unsigned char *chunkAddress;
    unsigned int chunkAvail;
    
    currentChunk = GetFirstChunkFromSuite(hSuite);
    
    while(currentChunk)
    {
        chunkAvail = GetSizeOfMemoryChunk(currentChunk);
        chunkAddress = (unsigned char*)GetMemoryAddressOfMemoryChunk(currentChunk);
        
        memset(chunkAddress, fill, chunkAvail);
        written += chunkAvail;
        currentChunk = GetNextMemoryChunk(hSuite, currentChunk);
    }
#endif
    return written;
}

/* when size is set to zero, it will try to allocate the maximum possible block */
struct memSuite *shoot_malloc_suite(size_t size)
{
    struct memSuite * hSuite = NULL;
    
    if(size > 0)
    {
        ASSERT(!alloc_sem_timed_out);
        AllocateMemoryResource(size, allocCBR, (unsigned int)&hSuite, 0x50);
        
        int r = take_semaphore(alloc_sem, 100);
        if (r)
        {
            alloc_sem_timed_out = 1;
            return NULL;
        }

        ASSERT(size == hSuite->size);
    }
    else
    {
        /* allocate some backup that will service the queued allocation request that fails during the loop */
        int backup_size = 4 * 1024 * 1024;
        int max_size = 0;
        struct memSuite *backup = shoot_malloc_suite(backup_size);

        for(int size = 4; size < 1024; size += 4)
        {
            struct memSuite *testSuite = shoot_malloc_suite(size * 1024 * 1024);
            if(testSuite)
            {
                shoot_free_suite(testSuite);
                max_size = size * 1024 * 1024 + backup_size;
            }
            else
            {
                break;
            }
        }
        /* now free the backup suite. this causes the queued allocation before to get finished. as we timed out, it will get freed immediately in exmem.c:allocCBR */
        shoot_free_suite(backup);
        
        hSuite = shoot_malloc_suite(max_size);
    }
    
    return hSuite;
}

struct memSuite * shoot_malloc_suite_contig(size_t size)
{
    if(size > 0)
    {
        ASSERT(!alloc_sem_timed_out);

        struct memSuite * hSuite = NULL;
        AllocateMemoryResourceForSingleChunck(size, allocCBR, (unsigned int)&hSuite, 0x50);

        int r = take_semaphore(alloc_sem, 100);
        if (r)
        {
            alloc_sem_timed_out = 1;
            return NULL;
        }
        
        ASSERT(size == hSuite->size);
        console_printf("%d\n", size);
        return hSuite;
    }
    else
    {
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
        
        return shoot_malloc_suite_contig(max_size);
    }
}

void* shoot_malloc(size_t size)
{
    struct memSuite * theSuite = shoot_malloc_suite_contig(size + 4);
    if (!theSuite) return 0;
    
    /* now we only have to tweak some things so it behaves like plain malloc */
    void* hChunk = (void*) GetFirstChunkFromSuite(theSuite);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    *(struct memSuite **)ptr = theSuite;
    return ptr + 4;
}

void shoot_free(void* ptr)
{
    if (!ptr) return;
    if ((intptr_t)ptr & 3) return;
    struct memSuite * hSuite = *(struct memSuite **)(ptr - 4);
    FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore(free_sem, 0);
}

#if 0
void exmem_test()
{
#ifdef CONFIG_FULL_EXMEM_SUPPORT
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
#else
    msleep(2000);
    info_led_on();
    void* p = shoot_malloc(20000000);
    NotifyBox(2000, "%x ", p);
    msleep(2000);
    shoot_free(p);
    info_led_off();
#endif
}
#endif

static void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
