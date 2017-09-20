/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "vram.h"
#include "lens.h"
#include "timer.h"

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

/** This just goes into the bss */
#define RELOCSIZE 0x10000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
static uint8_t _reloc[ RELOCSIZE ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}

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

    /*
     * in entry2() (0xff010134) make this change to
     * return to our code before calling cstart().
     * This should be a "BL cstart" instruction.
     */
    INSTR( HIJACK_INSTR_BL_CSTART ) = RET_INSTR;

    /*
     * in cstart() (0xff010ff4) make these changes:
     * calls bzero(), then loads bs_end and calls
     * create_init_task
     */
    // Reserve memory after the BSS for our application
    INSTR( HIJACK_INSTR_BSS_END ) = (uintptr_t) _bss_end;

    // Fix the calls to bzero32() and create_init_task()
    FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, bzero32 );
    FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, create_init_task );

    // Set our init task to run instead of the firmware one
    INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;
    
    // Make sure that our self-modifying code clears the cache
    sync_caches();

    // We enter after the signature, avoiding the
    // relocation jump that is at the head of the data
    thunk reloc_entry = (thunk)( RELOCADDR + 0xC );
    reloc_entry();

    /*
    * We're back!
    * The RAM copy of the firmware startup has:
    * 1. Poked the DMA engine with what ever it does
    * 2. Copied the rw_data segment to 0x1900 through 0x20740
    * 3. Zeroed the BSS from 0x20740 through 0x47550
    * 4. Copied the interrupt handlers to 0x0
    * 5. Copied irq 4 to 0x480.
    * 6. Installed the stack pointers for CPSR mode D2 and D3
    * (we are still in D3, with a %sp of 0x1000)
    * 7. Returned to us.
    *
    * Now is our chance to fix any data segment things, or
    * install our own handlers.
    */

    // This will jump into the RAM version of the firmware,
    // but the last branch instruction at the end of this
    // has been modified to jump into the ROM version
    // instead.
    void (*ram_cstart)(void) = (void*) &INSTR( cstart );
    ram_cstart();

    // Unreachable
    while(1)
        ;
}

extern void * _malloc(int);
extern void   _free(void *);

extern void * _AllocateMemory(int);
extern void   _FreeMemory(void *);

__attribute__((optimize("-fno-ipa-sra")))
static void malloc_test(const char * allocator_name, void * (*alloc)(int), void (*afree)(void*))
{
    qprintf("Allocating memory using %s...\n", allocator_name);
    uint32_t * p = alloc(32 * sizeof(p[0]));
    qprintf("Allocated from %X to %X\n", p, p + 127);
    qprintf("Reading without initializing...\n", p);
    p[5] = p[10] + 1;       /* this one should be easy to catch */
    p[6] = p[5] + 2;        /* this one won't trigger yet */
    qprintf("Reading outside bounds...\n", p);
    p[10] = p[-1] - 1;      /* this will be caught only on tracked memory heaps */
    p[11] = p[32] + 32;     /* same */
    p[12] = p[-10] - 10;    /* this may end up in some nearby block (so the error might not be caught) */
    p[13] = p[64] + 64;     /* same */
    /* writing outside bounds may disrupt future code execution */
    //qprintf("Writing outside bounds...\n", p);
    //p[-1] = -1;
    //p[32] = 32;
    //p[-10] = -10;
    //p[64] = 64;
    qprintf("Freeing memory...\n");
    afree(p);
    qprintf("Use after free...\n", p);
    p[15] = p[20] + 1;
    qprintf("%s test complete.\n\n", allocator_name);
}

static void malloc_tests()
{
    malloc_test("malloc", _malloc, _free);
    malloc_test("AllocateMemory", _AllocateMemory, _FreeMemory);
}

/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
static int
my_init_task(int a, int b, int c, int d)
{
    init_task(a,b,c,d);

    task_create("malloc_tests", 0x1e, 0x4000, malloc_tests, 0 );

    return 0;
}

/* dummy stubs */

void disp_set_pixel(int x, int y, int c) { }
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ... ) { return 0; }
int bfnt_draw_char() { return 0; }
