#include "dryos.h"
#include "bmp.h"

// experimental memory allocation from shooting buffer (~160MB on 5D2)


static struct semaphore * alloc_sem = 0;
static struct semaphore * free_sem = 0;

static void allocCBR(unsigned int a, struct memSuite *b)
{
    MEM(a) = (unsigned int)b;
    give_semaphore(alloc_sem);
}

static void freeCBR(unsigned int a)
{
    give_semaphore(free_sem);
}

struct memSuite
{
    char* signature; // MemSuite
    int size;
    int num_chunks;
    int first_chunk_maybe;
};


void* shoot_malloc(size_t size)
{
    struct memSuite * hSuite = 0;
    AllocateMemoryResource(size + 4, allocCBR, (unsigned int)&hSuite, 0x50);
    int r = take_semaphore(alloc_sem, 1000);
    if (r) return 0;
    if (hSuite && hSuite->num_chunks != 1)
    {
        FreeMemoryResource(hSuite, freeCBR, 0);
        return 0;
        
        /*
        // let's try again, maybe we are luckier this time
        // keep the old suite allocated, otherwise we'll get it fragmented again
        msleep(1000);
        struct memSuite * hSuite2 = 0;
        AllocateMemoryResource(size + 4, allocCBR, &hSuite2);
        int r2 = take_semaphore(alloc_sem, 1000);

        FreeMemoryResource(hSuite, freeCBR, 0);
        take_semaphore(free_sem, 0);

        if (r2 == 0 && hSuite2->num_chunks == 1) // yes!
        {
            hSuite = hSuite2;
        }
        else if (hSuite2) // boo...
        {
            FreeMemoryResource(hSuite2, freeCBR, 0);
            take_semaphore(free_sem, 0);
            return 0;
        }
        */
    }
    //~ bmp_hexdump(FONT_SMALL, 0, 100, hSuite, 32*10);
    void* hChunk = (void*) GetFirstChunkFromSuite(hSuite);
    //~ bmp_hexdump(FONT_SMALL, 0, 300, hChunk, 32*10);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    *(struct memSuite **)ptr = hSuite;
    return ptr + 4;
}

/* just try if we can allocate that much RAM, but don't return it (free it right away) */
int shoot_malloc_fragmented_test(size_t size)
{
    struct memSuite * hSuite = 0;
    AllocateMemoryResource(size, allocCBR, (unsigned int)&hSuite, 0x50);
    int r = take_semaphore(alloc_sem, 1000);
    if (r) return 0;
    FreeMemoryResource(hSuite, freeCBR, 0);
    return 1;
}

void shoot_free(void* ptr)
{
    if (!ptr) return;
    if ((intptr_t)ptr & 3) return;
    struct memSuite * hSuite = *(struct memSuite **)(ptr - 4);
    FreeMemoryResource(hSuite, freeCBR, 0);
    take_semaphore(free_sem, 0);
}

void exmem_test()
{
#if defined(CONFIG_5D3) || defined(CONFIG_7D)
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

static void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
