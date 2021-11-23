/** \file
 * Startup code for DIGIC 4 and 5
 * ("Classic" boot process)
 */

#include "dryos.h"
#include "boot.h"

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

/** This just goes into the bss */
static uint32_t _reloc[FIRMWARE_ENTRY_LEN / 4];
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
    const uint32_t firmware_len = FIRMWARE_ENTRY_LEN;
    uint32_t * const new_image = (void*) RELOCADDR;

    blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

    /*
     * in cstart() (0xff010ff4) make these changes:
     * calls bzero(), then loads bs_end and calls
     * create_init_task
     */
    // Reserve memory after the BSS for our application
    // This is done by resizing the malloc memory pool (user_mem_start in DryOS memory map),
    // We are going to change its start address, to begin right after our BSS,
    // (which is the last segment in our binary - see magiclantern.lds.S). */
    // Malloc memory is usually specified by its start and end address.
    // Exception: DIGIC 6 uses start address + size.
    // Cannot use qprintf here (no snprintf).
    qprint("[BOOT] changing user_mem_start from "); qprintn(INSTR(HIJACK_INSTR_BSS_END));
    qprint("to "); qprintn((uintptr_t)_bss_end); qprint("\n");
    INSTR( HIJACK_INSTR_BSS_END ) = (uintptr_t) _bss_end;
    ml_reserved_mem = (uintptr_t)_bss_end - RESTARTSTART;

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
