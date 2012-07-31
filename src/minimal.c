/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "config.h"
#include "version.h"
#include "bmp.h"
#include "menu.h"
#include "version.h"
#include "property.h"
#include "consts.h"

/** If CONFIG_EARLY_PORT is defined, only a few things will be enabled */
#undef CONFIG_EARLY_PORT

/** These are called when new tasks are created */
int my_init_task(int a, int b, int c, int d);
void my_bzero( uint8_t * base, uint32_t size );

/** This just goes into the bss */
#define RELOCSIZE 0x3000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
static uint8_t _reloc[ RELOCSIZE ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

/** Was this an autoboot or firmware file load? */
int autoboot_loaded;


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

    // Set the flag if this was an autoboot load
    autoboot_loaded = (offset == 0);

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
    clean_d_cache();
    flush_caches();

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

#ifndef CONFIG_EARLY_PORT
    // Install our task creation hooks
    //~ task_dispatch_hook = my_task_dispatch_hook;
#endif

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

/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */


void run_test()
{
    FILE * f = FIO_CreateFile(CARD_DRIVE "FF000000.BIN");
    if (f != (void*) -1)
    {
        FIO_WriteFile(f, (void*) 0xFF000000, 0x1000000);
        FIO_CloseFile(f);
    }
}

int
my_init_task(int a, int b, int c, int d)
{
    int ans = init_task(a,b,c,d);
    
    msleep(5000);
    
    task_create("test", 0x1e, 0x1000, run_test, 0 );
}
