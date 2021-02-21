/* extended memory, allocated from shoot memory buffer */

#ifndef _exmem_h_
#define _exmem_h_

struct memSuite
{
    char* signature; // MemSuite
    int size;
    int num_chunks;
    void *last_chunk; // points to last chunks chunk.prev
    void *first_chunk; // points to first chunks chunk.prev
    int mem_type; // not used during free, so mem types may vary within a suite
};

struct memChunk
{
    char* signature; // MemChunk
    void *prev; // points to previous chunks chunk.prev or to suite.last_chunk
    void *next; // points to next chunks chunk.prev or to suite.last_chunk
    int size;
    int remain;
    void *heap_ptr;
    void *memory_address;
    int ring_heap_handle;
    void (*free_cbr)(void *addr);
    int mem_type;
};

#define CHECK_SUITE_SIGNATURE(suite) ASSERT(streq((suite)->signature, "MemSuite"));
#define CHECK_CHUNK_SIGNATURE(chunk) ASSERT(streq((chunk)->signature, "MemChunk"));

/* these return a memory suite, which consists of one or more memory chunks */
/* it is up to user to iterate through the chunks */
struct memSuite *shoot_malloc_suite(size_t size);
void shoot_free_suite(struct memSuite * hSuite);

/* this returns a memory suite with a single contiguous block, but may fail because of memory fragmentation */
struct memSuite * shoot_malloc_suite_contig(size_t size);

/* this returns a memory suite with large (30-40 MB) constant-size blocks (normally used for RAW capture) */
/* this is on top of what you can get with shoot_malloc_suite */
/* num_requested_buffers can be 0 for autodetection */
struct memSuite * srm_malloc_suite(int num_requested_buffers);
void srm_free_suite(struct memSuite * suite);

/* dump the contents of a memsuite */
unsigned int exmem_save_buffer(struct memSuite * hSuite, char *file);

/* fill the entire memsuite with some char */
unsigned int exmem_clear(struct memSuite * hSuite, char fill);

/* MemorySuite routines */
int AllocateMemoryResource(int size, void (*cbr)(unsigned int, struct memSuite *), unsigned int ctx, int type);
int AllocateContinuousMemoryResource(int size, void (*cbr)(unsigned int, struct memSuite *), unsigned int ctx, int type);
int FreeMemoryResource(struct memSuite *hSuite, void (*cbr)(unsigned int), unsigned int ctx);

int GetNumberOfChunks(struct memSuite * suite);
int GetSizeOfMemorySuite(struct memSuite * suite);
struct memChunk * GetFirstChunkFromSuite(struct memSuite * suite);
struct memChunk * GetNextMemoryChunk(struct memSuite * suite, struct memChunk * chunk);
int GetSizeOfMemoryChunk(struct memChunk * chunk);
void* GetMemoryAddressOfMemoryChunk(struct memChunk * chunk);

struct memSuite * CreateMemorySuite(void* first_chunk_address, size_t first_chunk_size, uint32_t flags);
void DeleteMemorySuite(struct memSuite * suite);
struct memChunk * CreateMemoryChunk(void* address, size_t size, uint32_t flags);
int AddMemoryChunk(struct memSuite * suite, struct memChunk * chunk);

/* similar to shoot_malloc, for very large buffers (32-40 MB) */
/* usage caveat: you *must* free these as soon as you finish working with them,
 * because you can't take pictures as long as one of those is allocated */
void* _srm_malloc(size_t size);
void _srm_free(void* ptr);
int _srm_get_free_space();
int _srm_get_max_region();
struct memSuite * _srm_malloc_suite(int num_requested_buffers);
void _srm_free_suite(struct memSuite * suite);

void SRM_AllocateMemoryResourceFor1stJob(void (*callback)(void** dst_ptr, void* raw_buffer, uint32_t raw_buffer_size), void** dst_ptr);
void SRM_FreeMemoryResourceFor1stJob(void* raw_buffer, int unk1_zero, int unk2_zero);

#endif
