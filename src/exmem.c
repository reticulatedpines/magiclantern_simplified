#include "dryos.h"

// experimental memory allocation from shooting buffer (~160MB on 5D2)

static struct semaphore * alloc_sem = 0;
static void allocCBR(int a, int b)
{
    MEM(a) = b;
    give_semaphore(alloc_sem);
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
    if (!alloc_sem) alloc_sem = create_named_semaphore(0,0);
    struct memSuite * hSuite = 0;
    AllocateMemoryResource(size, allocCBR, &hSuite);
    int r = take_semaphore(alloc_sem, 1000);
    if (r) return 0;
    if (hSuite->num_chunks != 1) { beep(); return 0; }
    //~ bmp_hexdump(FONT_SMALL, 0, 100, hSuite, 32*10);
    void* hChunk = GetFirstChunkFromSuite_maybe(hSuite);
    //~ bmp_hexdump(FONT_SMALL, 0, 300, hChunk, 32*10);
    return GetMemoryAddressOfMemoryChunk(hChunk);
}
