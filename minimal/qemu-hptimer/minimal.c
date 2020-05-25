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

char* get_current_task_name()
{
    /* DryOS: right after current_task we have a flag
     * set to 1 when handling an interrupt */
    uint32_t interrupt_active = MEM((uintptr_t)&current_task + 4);
    
    if (!interrupt_active)
    {
        return current_task->name;
    }
    else
    {
        static char isr[] = "**INT-00h**";
        int i = current_interrupt >> 2;
        int i0 = (i & 0xF);
        int i1 = (i >> 4) & 0xF;
        int i2 = (i >> 8) & 0xF;
        isr[5] = i2 ? '0' + i2 : '-';
        isr[6] = i1 < 10 ? '0' + i1 : 'A' + i1 - 10;
        isr[7] = i0 < 10 ? '0' + i0 : 'A' + i0 - 10;
        return isr;
    }
}

static void hptimer_cbr(int a, void* b)
{
    qprintf("Hello from HPTimer (%d, %d) %s\n", a, b, get_current_task_name());
}

void qemu_hptimer_test()
{
    qprintf("Hello from task %s\n", get_current_task_name());

    /* one HPTimer is easy to emulate, but getting them to work in multitasking is hard */
    /* note: configuring 20 timers might cause a few of them to say NOT_ENOUGH_MEMORY */
    /* this test will stress both the HPTimers and the interrupt engine */
    SetHPTimerAfterNow(9000, hptimer_cbr, hptimer_cbr, (void*) 9);
    SetHPTimerAfterNow(6000, hptimer_cbr, hptimer_cbr, (void*) 6);
    SetHPTimerAfterNow(8000, hptimer_cbr, hptimer_cbr, (void*) 8);
    SetHPTimerAfterNow(4000, hptimer_cbr, hptimer_cbr, (void*) 4);
    SetHPTimerAfterNow(7000, hptimer_cbr, hptimer_cbr, (void*) 7);
    SetHPTimerAfterNow(9000, hptimer_cbr, hptimer_cbr, (void*) 10);
    SetHPTimerAfterNow(5000, hptimer_cbr, hptimer_cbr, (void*) 5);
    SetHPTimerAfterNow(2000, hptimer_cbr, hptimer_cbr, (void*) 2);
    SetHPTimerAfterNow(3000, hptimer_cbr, hptimer_cbr, (void*) 3);
    SetHPTimerAfterNow(1000, hptimer_cbr, hptimer_cbr, (void*) 1);

    SetHPTimerAfterNow(19000, hptimer_cbr, hptimer_cbr, (void*) 19);
    SetHPTimerAfterNow(16000, hptimer_cbr, hptimer_cbr, (void*) 16);
    SetHPTimerAfterNow(18000, hptimer_cbr, hptimer_cbr, (void*) 18);
    SetHPTimerAfterNow(14000, hptimer_cbr, hptimer_cbr, (void*) 14);
    SetHPTimerAfterNow(17000, hptimer_cbr, hptimer_cbr, (void*) 17);
    SetHPTimerAfterNow(19000, hptimer_cbr, hptimer_cbr, (void*) 20);
    SetHPTimerAfterNow(15000, hptimer_cbr, hptimer_cbr, (void*) 15);
    SetHPTimerAfterNow(12000, hptimer_cbr, hptimer_cbr, (void*) 12);
    SetHPTimerAfterNow(13000, hptimer_cbr, hptimer_cbr, (void*) 13);
    SetHPTimerAfterNow(11000, hptimer_cbr, hptimer_cbr, (void*) 11);
    msleep(5000);
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
    qprintf("Hello from task %s\n", get_current_task_name());

    init_task(a,b,c,d);

    qprintf("Hello from task %s\n", get_current_task_name());

    task_create("run_test", 0x1e, 0x4000, qemu_hptimer_test, 0 );

    return 0;
}

/* dummy stubs */

void disp_set_pixel(int x, int y, int c) { }
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ... ) { return 0; }
int bfnt_draw_char() { return 0; }
