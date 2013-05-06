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

/* these return a memory suite, which consists of one or more memory chunks */
/* it is up to user to iterate through the chunks */
struct memSuite *shoot_malloc_suite(size_t size);
void shoot_free_suite(struct memSuite * hSuite);

/* this returns a memory suite with a single contiguous block, but may fail because of memory fragmentation */
struct memSuite * shoot_malloc_suite_contig(size_t size);

/* these behave just like malloc/free, but may fail because of memory fragmentation */
void * shoot_malloc( size_t len );
void shoot_free( void * buf );

/* dump the contents of a memsuite */
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

/* fill the entire memsuite with some char */
unsigned int exmem_clear(struct memSuite * hSuite, char fill);

/* MemorySuite routines */
int AllocateMemoryResource(int size, void (*cbr)(unsigned int, struct memSuite *), unsigned int ctx, int type);
int AllocateMemoryResourceForSingleChunck(int size, void (*cbr)(unsigned int, struct memSuite *), unsigned int ctx, int type);
int FreeMemoryResource(struct memSuite *hSuite, void (*cbr)(unsigned int), unsigned int ctx);
struct memChunk *GetFirstChunkFromSuite(struct memSuite *hSuite);
struct memChunk *GetNextMemoryChunk(struct memSuite *hSuite, struct memChunk *hPos);
unsigned int GetSizeOfMemoryChunk(struct memChunk *chunk);
struct memChunk *GetRemainOfMemoryChunk(struct memChunk *chunk);
unsigned int GetMemoryAddressOfMemoryChunk(struct memChunk *chunk);
int GetNumberOfChunks(struct memSuite *hSuite);
int GetSizeOfMemorySuite(struct memSuite *hSuite);

#endif
