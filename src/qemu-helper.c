// Helper library to replace Canon stubs and allow ML emulation in QEMU

#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"
#include "compiler.h"

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN 0xCF123004
#define REG_DUMP_VRAM 0xCF123008

// all DIGIC V cameras require a RAM offset, take it from stubs.S
#if defined(CONFIG_650D)
#define RAM_OFFSET (0xFFA4DF58-0x1900)
#elif defined(CONFIG_5D3)
#define RAM_OFFSET 0xFF9DEF48
#elif defined(CONFIG_6D)
#define RAM_OFFSET 0xFFCC34D4
#elif defined(CONFIG_EOSM)
#define RAM_OFFSET 0xFFA68D58
#else
#define RAM_OFFSET 0
#endif

#if defined(CONFIG_60D) || defined(CONFIG_600D)
#define AllocateMemory AllocateMemory_do
#endif

#ifdef CONFIG_5D3
#define FIO_FindFirstEx _FIO_FindFirstEx
#define FIO_GetFileSize _FIO_GetFileSize
#define FIO_CreateFile _FIO_CreateFile
extern thunk _FIO_FindFirstEx;
extern thunk _FIO_GetFileSize;
extern thunk _FIO_CreateFile;
#endif

#define BMP_VRAM_ADDR 0x003638100

int qprintf(const char * fmt, ...) // prints in the QEMU console
{
    va_list ap;
    char buf[256];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    
    for (char* c = buf; *c; c++)
        *(volatile uint32_t*)REG_PRINT_CHAR = *c;
    
    return 0;
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

NULL_STUB_BODY(init_task)
NULL_STUB_BODY_INT(cstart)
NULL_STUB_BODY_STR_INT(msg_queue_create)
NULL_STUB_BODY_HEX(CreateRecursiveLock)
NULL_STUB_BODY_HEX(prop_register_slave)
NULL_STUB_BODY_HEX(LoadCalendarFromRTC)
NULL_STUB_BODY_HEX(is_taskid_valid)
NULL_STUB_BODY_HEX(GUI_Control)

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
        *(volatile uint32_t*) REG_DUMP_VRAM = BMP_VRAM_ADDR;
        return;
    }

    if (streq(func, "shutdown"))
    {
        *(volatile uint32_t*) REG_SHUTDOWN = 1;
        return;
    }
}

void* q_AllocateMemory(size_t size)
{
    // dumb alloc, no free
    qprintf("*** AllocateMemory(%x)", size);
    
    static uint32_t alloc_ptr = 0x10000000;
    void* ans = (void*)ALIGN32(alloc_ptr + 64);
    alloc_ptr = ALIGN32(alloc_ptr + size + 128);
    qprintf(" => %x\n", ans);
    return ans;
}

void* q_AllocateMemory_do(void* pool, size_t size) { return q_AllocateMemory(size); }
void* q_malloc(size_t size) { return q_AllocateMemory(size); }
void* q_alloc_dma_memory(size_t size) { return q_AllocateMemory(size); }
NULL_STUB_BODY_HEX(free);
NULL_STUB_BODY_HEX(FreeMemory);
NULL_STUB_BODY_HEX(free_dma_memory);

void q_DryosDebugMsg(int class, int level, char* fmt, ...)
{
    va_list ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );

    qprintf("[DebugMsg] (%d,%d) %s\n", class, level, buf);
}

void * q_FIO_FindFirstEx(const char * dirname, struct fio_file * file)
{
    qprintf("*** FIO_FindFirstEx('%s', %x)\n", dirname, file);
    return (void*)1;
}

int q_FIO_GetFileSize(const char * filename, int* size)
{
    qprintf("*** FIO_GetFileSize('%s')\n", filename);
    *size = 0;
    return 0;
}

int q_FIO_CreateFile(const char * filename)
{
    static int fd = 1;
    qprintf("*** FIO_CreateFile('%s') => %d\n", filename, fd);
    return fd++;
}

int q_FIO_CloseFile(int fd)
{
    qprintf("*** FIO_CloseFile(%x)\n", fd);
    return -1;
}

int q_FIO_WriteFile(int fd, char* buf, int size)
{
    qprintf("*** FIO_WriteFile(%x, %d)\n{{{\n", fd, size);
    for (int i = 0; i < MIN(size, 128); i++)
        *(volatile uint32_t*)REG_PRINT_CHAR = buf[i];
    qprintf("\n}}}\n");
    return size;
}

#define STUB_MAP(name) &name, &q_##name,

void cam_init()
{
    // set BMP VRAM
    bmp_vram_info[1].vram2 = (void*) BMP_VRAM_ADDR;

    // fake display on
    #ifdef DISPLAY_STATEOBJ
    DISPLAY_STATEOBJ->current_state = 1;
    #else
    DISPLAY_IS_ON = 1;
    #endif
}

void q_create_init_task(int unused, void (*init_task)(void*))
{
    qprintf("create_init_task(%x)\n", init_task);
    cam_init();
    launch(init_task);
}

extern thunk AllocateMemory_do;
extern thunk msg_queue_create;
extern thunk prop_register_slave;
extern thunk is_taskid_valid;
extern thunk GUI_Control;

#define MAGIC (void*)0x12345678
void*  stub_mappings[] = {
    MAGIC, MAGIC, RAM_OFFSET,
    STUB_MAP(create_init_task)
    STUB_MAP(init_task)
    STUB_MAP(task_create)
    STUB_MAP(msleep)
    STUB_MAP(malloc)
    STUB_MAP(free)
    STUB_MAP(alloc_dma_memory)
    STUB_MAP(free_dma_memory)
    STUB_MAP(AllocateMemory)
    STUB_MAP(FreeMemory)
    STUB_MAP(DryosDebugMsg)
    STUB_MAP(call)
    STUB_MAP(create_named_semaphore)
    STUB_MAP(take_semaphore)
    STUB_MAP(give_semaphore)
    STUB_MAP(msg_queue_create)
    STUB_MAP(CreateRecursiveLock)
    STUB_MAP(FIO_FindFirstEx)
    STUB_MAP(FIO_GetFileSize)
    STUB_MAP(FIO_CreateFile)
    STUB_MAP(FIO_WriteFile)
    STUB_MAP(FIO_CloseFile)
    
    STUB_MAP(prop_register_slave)
    STUB_MAP(LoadCalendarFromRTC)
    STUB_MAP(is_taskid_valid)
    STUB_MAP(GUI_Control)
    
    MAGIC, MAGIC,
};

void exit(int arg)
{
    *(volatile uint32_t*) REG_SHUTDOWN = 1;
    while(1);
}
