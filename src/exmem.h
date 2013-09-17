/* extended memory, allocated from shoot memory buffer */

#ifndef _exmem_h_
#define _exmem_h_

struct memSuite
{
    char* signature; // MemSuite
    int size;
    int num_chunks;
    int first_chunk_maybe;
};

struct memChunk
{
    char* signature; // MemChunk
    int off_0x04;
    int next_chunk_maybe;
    int size;
    int remain;
};

#define CHECK_SUITE_SIGNATURE(suite) ASSERT(streq((suite)->signature, "MemSuite"));
#define CHECK_CHUNK_SIGNATURE(chunk) ASSERT(streq((chunk)->signature, "MemChunk"));

/* these return a memory suite, which consists of one or more memory chunks */
/* it is up to user to iterate through the chunks */
struct memSuite *shoot_malloc_suite(size_t size);
void shoot_free_suite(struct memSuite * hSuite);

/* this returns a memory suite with a single contiguous block, but may fail because of memory fragmentation */
struct memSuite * shoot_malloc_suite_contig(size_t size);

/* these behave just like malloc/free, but may fail because of memory fragmentation */
void * _shoot_malloc( size_t len );
void _shoot_free( void * buf );

/* dump the contents of a memsuite */
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

/* fill the entire memsuite with some char */
unsigned int exmem_clear(struct memSuite * hSuite, char fill);

/* MemorySuite routines */
int AllocateMemoryResource(int size, void (*cbr)(unsigned int, struct memSuite *), unsigned int ctx, int type);
int AllocateMemoryResourceForSingleChunk(int size, void (*cbr)(unsigned int, struct memSuite *), unsigned int ctx, int type);
int FreeMemoryResource(struct memSuite *hSuite, void (*cbr)(unsigned int), unsigned int ctx);

int GetNumberOfChunks(struct memSuite * suite);
int GetSizeOfMemorySuite(struct memSuite * suite);
struct memChunk * GetFirstChunkFromSuite(struct memSuite * suite);
struct memChunk * GetNextMemoryChunk(struct memSuite * suite, struct memChunk * chunk);
int GetSizeOfMemoryChunk(struct memChunk * chunk);
void* GetMemoryAddressOfMemoryChunk(struct memChunk * chunk);

struct memSuite * CreateMemorySuite(void* src, size_t length, int idk);
#endif
