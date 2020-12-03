/** \file
 * Startup code for DIGIC 4 and 5
 * (ML loaded into AllocateMemory heap)
 */

#define BOOT_USE_INIT_TASK_PATCHED

#include "dryos.h"
#include "boot.h"
#include "reloc.h"

#if !defined(CONFIG_ALLOCATE_MEMORY_POOL)
#error CONFIG_ALLOCATE_MEMORY_POOL must be defined.
#endif

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

/** This just goes into the bss */
#define RELOCSIZE 0x3000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
static uint32_t _reloc[ RELOCSIZE / 4 ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

void
__attribute__((noreturn,noinline,naked))
copy_and_restart( int offset )
{
    zero_bss();

    // Copy the firmware to somewhere safe in memory
    const uint8_t * const firmware_start = (void*) ROMBASEADDR;
    const uint32_t firmware_len = RELOCSIZE;
    uint32_t * const new_image = (void*) RELOCADDR;

    blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

    // Fix the calls to bzero32() and create_init_task()
    FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, bzero32 );
    FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, create_init_task );

    // Set our init task to run instead of the firmware one
    qprint("[BOOT] changing init_task from "); qprintn(INSTR( HIJACK_INSTR_MY_ITASK ));
    qprint("to "); qprintn((uint32_t) my_init_task); qprint("\n");
    INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;
    
    // Make sure that our self-modifying code clears the cache
    sync_caches();

    // We enter after the signature, avoiding the
    // relocation jump that is at the head of the data
    thunk reloc_entry = (thunk)( RELOCADDR + 0xC );
    reloc_entry();

    // Unreachable
    while(1)
        ;
}

/* qprintf should be fine from now on; undefine the "guard" definition */
#undef qprintf

#define ITASK_LEN   (ROM_ITASK_END - ROM_ITASK_START)
#define CREATETASK_MAIN_LEN (ROM_CREATETASK_MAIN_END - ROM_CREATETASK_MAIN_START)

init_task_func init_task_patched(void)
{
    // We shrink the AllocateMemory pool in order to make space for ML binary
    // Example for the 1100D firmware
    // ff0197d8: init_task:
    // ff01984c: b CreateTaskMain
    //
    // ff0123c4 CreateTaskMain:
    // ff0123e4: mov r1, #13631488  ; 0xd00000  <-- end address
    // ff0123e8: mov r0, #3997696   ; 0x3d0000  <-- start address
    // ff0123ec: bl  allocatememory_init_pool

    // So... we need to patch CreateTaskMain, which is called by init_task.
    //
    // First we use Trammell's reloc.c code to relocate init_task and CreateTaskMain...

    static char init_task_reloc_buf[ITASK_LEN+64];
    static char CreateTaskMain_reloc_buf[CREATETASK_MAIN_LEN+64];
    
    int (*new_init_task)(int,int,int,int) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        ROM_ITASK_START,
        ROM_ITASK_END,
        (uintptr_t) init_task_reloc_buf
    );

    int (*new_CreateTaskMain)(void) = (void*)reloc(
        0,      // we have physical memory
        0,      // with no virtual offset
        ROM_CREATETASK_MAIN_START,
        ROM_CREATETASK_MAIN_END,
        (uintptr_t) CreateTaskMain_reloc_buf
    );
    
    const uintptr_t init_task_offset = (intptr_t)new_init_task - (intptr_t)init_task_reloc_buf - (intptr_t)ROM_ITASK_START;
    const uintptr_t CreateTaskMain_offset = (intptr_t)new_CreateTaskMain - (intptr_t)CreateTaskMain_reloc_buf - (intptr_t)ROM_CREATETASK_MAIN_START;

    // Done relocating, now we can patch things.

    uint32_t* addr_AllocMem_end     = (void*)(CreateTaskMain_reloc_buf + ROM_ALLOCMEM_END + CreateTaskMain_offset);
    uint32_t* addr_BL_AllocMem_init = (void*)(CreateTaskMain_reloc_buf + ROM_ALLOCMEM_INIT + CreateTaskMain_offset);
    uint32_t* addr_B_CreateTaskMain = (void*)(init_task_reloc_buf + ROM_B_CREATETASK_MAIN + init_task_offset);

    qprint("[BOOT] changing AllocMem limits:\n");
    qdisas((uint32_t)addr_AllocMem_end);
    qdisas((uint32_t)addr_AllocMem_end + 4);

    /* check if the patched addresses are, indeed, a BL and a B instruction */
    if ((((*addr_BL_AllocMem_init) >> 24) != (BL_INSTR(0,0) >> 24)) ||
        (((*addr_B_CreateTaskMain) >> 24) != (B_INSTR(0,0)  >> 24)))
    {
        qprintf("Please check ROM_ALLOCMEM_INIT and ROM_B_CREATETASK_MAIN.\n");
        while(1);                                       /* refuse to boot */
    }

    #if defined(CONFIG_6D) || defined(CONFIG_100D)
    /* R0: 0x44C000 (start address, easier to patch, change to 0x4E0000 => reserve 592K for ML) */
    /* R1: 0xD3C000 [6D] / 0xC3C000 [100D] (end address, unchanged) */
    addr_AllocMem_end[1] = MOV_R0_0x4E0000_INSTR;
    ml_reserved_mem = 0x4E0000 - RESTARTSTART;
    #elif defined(CONFIG_550D) || defined(CONFIG_600D)
    // change end limit from 0xd00000 to 0xc70000 => reserve 576K for ML
    *addr_AllocMem_end = MOV_R1_0xC70000_INSTR;
    ml_reserved_mem = 0xD00000 - RESTARTSTART;
    #else
    // change end limit from 0xd00000 to 0xc80000 => reserve 512K for ML
    *addr_AllocMem_end = MOV_R1_0xC80000_INSTR;
    ml_reserved_mem = 0xD00000 - RESTARTSTART;
    #endif

    qdisas((uint32_t)addr_AllocMem_end);
    qdisas((uint32_t)addr_AllocMem_end + 4);

    // relocating CreateTaskMain does some nasty things, so, right after patching,
    // we jump back to ROM version; at least, what's before patching seems to be relocated properly
    *addr_BL_AllocMem_init = B_INSTR(addr_BL_AllocMem_init, ROM_ALLOCMEM_INIT);
    
    // replace call to CreateMainTask (last sub in init_task)
    *addr_B_CreateTaskMain = B_INSTR(addr_B_CreateTaskMain, new_CreateTaskMain);
    
    /* before we execute code, make sure a) data caches are drained and b) instruction caches are clean */
    sync_caches();
    
    // Well... let's cross the fingers and call the relocated stuff
    return new_init_task;
}
