#include "dryos.h"

// experimental memory allocation from shooting buffer (~160MB on 5D2)

static struct semaphore * alloc_sem = 0;
static struct semaphore * free_sem = 0;

static void allocCBR(int a, int b)
{
    MEM(a) = b;
    give_semaphore(alloc_sem);
}

static void freeCBR(int a, int b)
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

void* shoot_malloc(int size)
{
    struct memSuite * hSuite = 0;
    AllocateMemoryResource(size + 4, allocCBR, &hSuite);
    int r = take_semaphore(alloc_sem, 1000);
    if (r) return 0;
    if (hSuite && hSuite->num_chunks != 1) 
    {
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
    }
    //~ bmp_hexdump(FONT_SMALL, 0, 100, hSuite, 32*10);
    void* hChunk = (void*) GetFirstChunkFromSuite_maybe(hSuite);
    //~ bmp_hexdump(FONT_SMALL, 0, 300, hChunk, 32*10);
    void* ptr = (void*) GetMemoryAddressOfMemoryChunk(hChunk);
    *(struct memSuite **)ptr = hSuite;
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

void exmem_test()
{
    msleep(2000);
    info_led_on();
    void* p = shoot_malloc(20000000);
    NotifyBox(2000, "%x ", p);
    msleep(2000);
    shoot_free(p);
    info_led_off();
}

void exmem_init()
{
    alloc_sem = create_named_semaphore(0,0);
    free_sem = create_named_semaphore(0,0);
}

INIT_FUNC("exmem", exmem_init);
