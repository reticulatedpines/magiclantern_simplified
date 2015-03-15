// Helper library to replace Canon stubs and allow ML emulation in QEMU

#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"
#include "compiler.h"

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN 0xCF123004
#define REG_DUMP_VRAM 0xCF123008

/*
 * FIO access to a local directory
 * A:/ mapped to cfcard/ and B:/ mapped to sdcard/
 * Single-user, single-task for now (only one file open at a time)
 */
#define REG_FIO_NUMERIC_ARG0    0xCF123F00  // R/W
#define REG_FIO_NUMERIC_ARG1    0xCF123F04  // R/W
#define REG_FIO_NUMERIC_ARG2    0xCF123F08  // R/W
#define REG_FIO_NUMERIC_ARG3    0xCF123F0C  // R/W
#define REG_FIO_BUFFER          0xCF123F10  // R/W; buffer position auto-increments; used to pass filenames or to get data
#define REG_FIO_BUFFER_SEEK     0xCF123F14  // MEM(REG_FIO_BUFFER_SEEK) = position;
#define REG_FIO_GET_FILE_SIZE   0xCF123F20  // filename in buffer; size = MEM(REG_FIO_GET_FILE_SIZE);
#define REG_FIO_OPENDIR         0xCF123F24  // path name in buffer; ok = MEM(REG_FIO_OPENDIR);
#define REG_FIO_CLOSEDIR        0xCF123F28  // ok = MEM(REG_FIO_CLOSEDIR);
#define REG_FIO_READDIR         0xCF123F2C  // ok = MEM(REG_FIO_READDIR); dir name in buffer; size, mode, time in arg0-arg2
#define REG_FIO_OPENFILE        0xCF123F34  // file name in buffer; ok = MEM(REG_FIO_OPENFILE);
#define REG_FIO_CLOSEFILE       0xCF123F38  // ok = MEM(REG_FIO_CLOSEFILE);
#define REG_FIO_READFILE        0xCF123F3C  // size in arg0; pos in arg1; bytes_read = MEM(REG_FIO_READFILE); contents in buffer

// all DIGIC V cameras require a RAM offset, take it from stubs.S
#if defined(CONFIG_650D)
#define RAM_OFFSET (0xFFA4DF58-0x1900)
#elif defined(CONFIG_5D3)
#define RAM_OFFSET 0xFF9F07C0
#elif defined(CONFIG_6D)
#define RAM_OFFSET 0xFFCC34D4
#elif defined(CONFIG_EOSM)
#define RAM_OFFSET 0xFFA68D58
#else
#define RAM_OFFSET 0
#endif

int qprintf(const char * fmt, ...) // prints in the QEMU console
{
    va_list ap;
    char buf[256];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    
    for (char* c = buf; *c; c++)
        MEM(REG_PRINT_CHAR) = *c;
    
    return 0;
}

int qfio_printf(const char * fmt, ...) // sends data to QEMU FIO buffer (to pass filenames etc)
{
    va_list ap;
    char buf[256];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    
    MEM(REG_FIO_BUFFER_SEEK) = 0;
    
    for (char* c = buf; *c; c++)
        MEM(REG_FIO_BUFFER) = *c;

    MEM(REG_FIO_BUFFER) = 0;
    
    return 0;
}

char* qfio_read0() // read a null-terminated string from QEMU FIO buffer
{
    MEM(REG_FIO_BUFFER_SEEK) = 0;
    
    static char buf[1000];
    
    for (int i = 0; i < COUNT(buf); i++)
    {
        buf[i] = MEM(REG_FIO_BUFFER);
        
        if (!buf[i])
        {
            break;
        }
    }
    
    return buf;
}

void qfio_read(char* buf, int num) // read "num" bytes from QEMU FIO buffer, into "buf"
{
    MEM(REG_FIO_BUFFER_SEEK) = 0;
    
    for (int i = 0; i < num; i++)
    {
        buf[i] = MEM(REG_FIO_BUFFER);
    }
}

int
streq( const char * a, const char * b )
{
    while( *a && *b )
        if( *a++ != *b++ )
            return 0;
    return *a == *b;
}

int
snprintf(
    char *          buf,
    size_t          max_len,
    const char *        fmt,
    ...
)
{
    va_list         ap;
    va_start( ap, fmt );
    int len = vsnprintf( buf, max_len - 1, fmt, ap );
    va_end( ap );
    return len;
}

#define NULL_STUB_BODY(name) int q_##name(void) { qprintf("*** " #name "\n"); return 0; }
#define NULL_STUB_BODY_STR(name) int q_##name(char* str) { qprintf("*** " #name "('%s')\n", str); return 0; }
#define NULL_STUB_BODY_HEX(name) int q_##name(int hex) { qprintf("*** " #name "(%x)\n", hex); return 0; }
#define NULL_STUB_BODY_INT(name) int q_##name(int num) { qprintf("*** " #name "(%d)\n", num); return 0; }
#define NULL_STUB_BODY_STR_INT(name) int q_##name(char* str, int num) { qprintf("*** " #name "('%s', %d)\n", str, num); return 0; }

NULL_STUB_BODY_INT(cstart)
NULL_STUB_BODY_STR_INT(msg_queue_create)
NULL_STUB_BODY_HEX(CreateRecursiveLock)
NULL_STUB_BODY_HEX(prop_register_slave)
NULL_STUB_BODY_HEX(_prop_request_change)
NULL_STUB_BODY_HEX(LoadCalendarFromRTC)
NULL_STUB_BODY_HEX(is_taskid_valid)
NULL_STUB_BODY_HEX(GUI_Control)

int q_CreateResLockEntry()
{
    qprintf("*** CreateResLockEntry()\n");
    return 1;
}

void launch(void (*func)(void*))
{
    func(0);
}

void q_task_create(char* name, uint32_t priority, uint32_t stack_size, void * entry, void * arg)
{
    qprintf("*** task_create('%s', %d, %x, %x, %x)\n", name, priority, stack_size, entry, arg);
    launch(entry);
}

void q_msleep(int ms)
{
    qprintf("*** msleep(%d)\n", ms);
}

void* q_create_named_semaphore(char* name, int val)
{
    qprintf("*** create_named_semaphore('%s', %d)\n", name, val);
    return 0;
}

int q_take_semaphore(void * sem)
{
    qprintf("*** take_semaphore(%x)\n", sem);
    return 0;
}

int q_give_semaphore(void * sem)
{
    qprintf("*** give_semaphore(%x)\n", sem);
    return 0;
}


void q_call(char* func)
{
    qprintf("*** call('%s')\n", func);
    
    if (streq(func, "dispcheck"))
    {
        *(volatile uint32_t*) REG_DUMP_VRAM = 0;
        return;
    }

    if (streq(func, "shutdown"))
    {
        *(volatile uint32_t*) REG_SHUTDOWN = 1;
        return;
    }
}

void* q__AllocateMemory(size_t size)
{
    // dumb alloc, no free
    qprintf("*** AllocateMemory(%x)", size);
    
    static uint32_t alloc_ptr = 0x10000000;
    void* ans = (void*)ALIGN32(alloc_ptr + 64);
    alloc_ptr = ALIGN32(alloc_ptr + size + 128);
    qprintf(" => %x\n", ans);
    return ans;
}

void* q__malloc(size_t size) { return q__AllocateMemory(size); }
void* q__alloc_dma_memory(size_t size) { return q__AllocateMemory(size); }
NULL_STUB_BODY_HEX(_free);
NULL_STUB_BODY_HEX(_FreeMemory);
NULL_STUB_BODY_HEX(_free_dma_memory);

void q_DryosDebugMsg(int class, int level, char* fmt, ...)
{
    va_list ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );

    qprintf("[DebugMsg] (%d,%d) %s\n", class, level, buf);
}

int q_GetMemoryInformation(int* total, int* free)
{
    /* just some dummy numbers */
    *total = 10*1024*1024;
    *free = 5*1024*1024;
    return 0;
}
extern int q_GetSizeOfMaxRegion(int* max_region)
{
    *max_region = 2*1024*1024;
    return 0;
}

void * q_readdir(struct fio_file * file)
{
    int ok = MEM(REG_FIO_READDIR);
    if (ok)
    {
        snprintf(file->name, sizeof(file->name), qfio_read0());
        file->size = MEM(REG_FIO_NUMERIC_ARG0);
        file->mode = MEM(REG_FIO_NUMERIC_ARG1);
        file->timestamp = MEM(REG_FIO_NUMERIC_ARG2);
        return (void*) 0;
    }
    else
    {
        return (void*) 1;
    }
}

void * q__FIO_FindFirstEx(const char * dirname, struct fio_file * file)
{
    qprintf("*** FIO_FindFirstEx('%s', %x)\n", dirname, file);
    qfio_printf("%s", dirname);
    int ok = MEM(REG_FIO_OPENDIR);
    if (ok)
    {
        return q_readdir(file);
    }
    else
    {
        return (void*) 1;
    }
}

void * q_FIO_FindNextEx(struct fio_dirent * dirent, struct fio_file * file)
{
    qprintf("*** FIO_FindNextEx(%x, %x)\n", dirent, file);
    return q_readdir(file);
}

int q_FIO_FindClose(struct fio_dirent * dirent)
{
    qprintf("*** FIO_FindClose(%x)\n", dirent);
    int ok = MEM(REG_FIO_CLOSEDIR);
    return ok;
}

int q__FIO_GetFileSize(const char * filename, int* size)
{
    qfio_printf("%s", filename);
    *size = MEM(REG_FIO_GET_FILE_SIZE);
    qprintf("*** FIO_GetFileSize('%s') => %d\n", filename, *size);
    return *size >= 0 ? 0 : -1;
}

int q__FIO_CreateFile(const char * filename)
{
    static int fd = 1;
    qprintf("*** FIO_CreateFile('%s') => %d\n", filename, fd);
    return fd++;
}

int q_FIO_WriteFile(int fd, char* buf, int size)
{
    qprintf("*** FIO_WriteFile(%x, %d)\n{{{\n", fd, size);
    for (int i = 0; i < MIN(size, 128); i++)
        *(volatile uint32_t*)REG_PRINT_CHAR = buf[i];
    qprintf("\n}}}\n");
    return size;
}

static int fio_read_pos = 0;

FILE* q__FIO_OpenFile( const char* filename, unsigned mode )
{
    qprintf("*** FIO_OpenFile(%s)\n", filename);
    qfio_printf("%s", filename);
    fio_read_pos = 0;
    int ok = MEM(REG_FIO_OPENFILE);
    return ok ? 1 : 0;
}

int q_FIO_ReadFile( FILE* stream, void* ptr, size_t count )
{
    qprintf("*** FIO_ReadFile(%x, %x, %d)\n", stream, ptr, count);
    int total_bytes = 0;
    while (total_bytes < count)
    {
        MEM(REG_FIO_NUMERIC_ARG0) = count;
        MEM(REG_FIO_NUMERIC_ARG1) = fio_read_pos;
        int bytes = MEM(REG_FIO_READFILE);
        qfio_read(ptr, bytes);
        ptr += bytes;
        total_bytes += bytes;
        fio_read_pos += bytes;
        if (bytes == 0)
        {
            break;
        }
    }
    return total_bytes;
}

uint64_t q_FIO_SeekFile( FILE* stream, size_t position, int whence )
{
    qprintf("*** FIO_SeekFile(%x, %x, %d)\n", stream, position, whence);

    switch (whence)
    {
        case SEEK_SET:
            fio_read_pos = position;
            break;
        case SEEK_CUR:
            fio_read_pos += position;
            break;
        case SEEK_END:
            fio_read_pos = -position;
            break;
    }

    return 0;
}

uint64_t q_FIO_SeekSkipFile( FILE* stream, uint64_t position, int whence )
{
    qprintf("*** FIO_SeekFile(%x, %x, %d)\n", stream, (int)position, whence);
    return q_FIO_SeekFile(stream, position, whence);
}

int q_FIO_CloseFile(int fd)
{
    qprintf("*** FIO_CloseFile(%x)\n", fd);
    int ok = MEM(REG_FIO_CLOSEFILE);
    return ok;
}

#define STUB_MAP(name) &name, &q_##name,

void q_create_init_task(int unused, void (*init_task)(void*))
{
    qprintf("create_init_task(%x)\n", init_task);
    launch(init_task);
}

void q_init_task()
{
    qprintf("*** init_task\n");
    //~ cam_init();
}

extern thunk prop_register_slave;
extern thunk _prop_request_change;
extern thunk is_taskid_valid;
extern thunk CreateResLockEntry;
extern thunk _alloc_dma_memory;
extern thunk _free_dma_memory;
extern thunk _AllocateMemory;
extern thunk _FreeMemory;
extern thunk _malloc;
extern thunk _free;
extern thunk GetMemoryInformation;
extern thunk GetSizeOfMaxRegion;

extern thunk _FIO_FindFirstEx;
extern thunk _FIO_GetFileSize;
extern thunk _FIO_CreateFile;
extern thunk _FIO_OpenFile;

#define MAGIC (void*)0x12345678
void*  stub_mappings[] = {
    MAGIC, MAGIC, (void*)RAM_OFFSET,
    //~ STUB_MAP(create_init_task)
    //~ STUB_MAP(init_task)
    //~ STUB_MAP(task_create)
    //~ STUB_MAP(msleep)
    //~ STUB_MAP(_malloc)
    //~ STUB_MAP(_free)
    //~ STUB_MAP(_alloc_dma_memory)
    //~ STUB_MAP(_free_dma_memory)
    //~ STUB_MAP(_AllocateMemory)
    //~ STUB_MAP(_FreeMemory)
    //~ STUB_MAP(GetMemoryInformation)
    //~ STUB_MAP(GetSizeOfMaxRegion)
    STUB_MAP(DryosDebugMsg)
    //~ STUB_MAP(call)
    //~ STUB_MAP(create_named_semaphore)
    //~ STUB_MAP(take_semaphore)
    //~ STUB_MAP(give_semaphore)
    //~ STUB_MAP(msg_queue_create)
    //~ STUB_MAP(CreateRecursiveLock)
    STUB_MAP(_FIO_FindFirstEx)
    STUB_MAP(FIO_FindNextEx)
    STUB_MAP(FIO_FindClose)
    STUB_MAP(_FIO_GetFileSize)
    STUB_MAP(_FIO_CreateFile)
    STUB_MAP(_FIO_OpenFile)
    STUB_MAP(FIO_ReadFile)
    STUB_MAP(FIO_WriteFile)
    STUB_MAP(FIO_CloseFile)
    STUB_MAP(FIO_SeekSkipFile)
    
    STUB_MAP(prop_register_slave)
    STUB_MAP(_prop_request_change)
    //~ STUB_MAP(LoadCalendarFromRTC)
    //~ STUB_MAP(is_taskid_valid)
    //~ STUB_MAP(GUI_Control)
    STUB_MAP(CreateResLockEntry)
    MAGIC, MAGIC,
};

void exit(int arg)
{
    *(volatile uint32_t*) REG_SHUTDOWN = 1;
    while(1);
}
