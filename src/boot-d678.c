/** \file
 * Startup code for DIGIC 7 & 8
 */

#include "dryos.h"
#include "boot.h"
#include "boot-d678.h"

#if !defined(CONFIG_DIGIC_678)
    #error "Expected D678"
#endif

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

// This reserves space for some early Canon code to be copied into,
// edited by us, then executed.  The build locates this in bss of
// our binary, so the location is known.
//
// See reboot.c for the prior part in the process.
//
// Note that some instructions in the copied region may load
// constants or call code located nearby.  If so, you should
// ensure the LEN constants mean these are also copied.
//
// Some cams have the cstart region far from the firmware_entry region.
// If firmware_entry and cstart are close, make FIRMWARE_ENTRY_LEN
// enough to cover both.
//
// If they're far apart, define CSTART_LEN, as well as FIRMWARE_ENTRY_LEN,
// each for their own region.  The starts are already defined in stubs.S.
// If in doubt about sizes, it's safe to make the regions larger than needed,
// you lose some space for ML / DryOS.
//
// We then copy both regions so they are adjacent in our buffer,
// compacting them and correcting the relevant calls
// so the middle can be skipped.
//
// Values must be 4 aligned.
#ifdef CSTART_LEN
    static uint32_t _reloc[(FIRMWARE_ENTRY_LEN + CSTART_LEN)/ 4];
#else
    static uint32_t _reloc[FIRMWARE_ENTRY_LEN / 4];
#endif
#define RELOCADDR ((uintptr_t)_reloc)

static uint32_t reloc_addr(uint32_t addr)
{
    // converts an address from "normal" cam range
    // to its address within our copied code in reloc buffer
#ifdef CSTART_LEN
    if (addr >= ((uint32_t)cstart & 0xfffffffe))
    {
        return RELOCADDR + (addr - ((uint32_t)cstart & 0xfffffffe)) + FIRMWARE_ENTRY_LEN;
    }
#endif
    return RELOCADDR + addr - ROMBASEADDR;
}

static void patch_thumb_branch(uint32_t pc, uint32_t dest)
{
    // Converts the pc address to within the reloc buffer range,
    // and patches the reloc copy to branch to dest.
    //
    // pc should be an address in "normal" cam range for code,
    // dest should be an address within ML address range.
    //
    // pc should be part of the rom region that has been copied
    // to reloc buffer.  The instruction at pc should be b.w, bx or blx.
    //
    // pc should be a Thumb instruction.
    //
    // Type of branch to patch in is detected from dest,
    // Thumb targets should be specified with LSb set.

    qprint("[BOOT] orig pc: "); qprintn(pc); qprint("\n");
    pc = reloc_addr(pc);
    qprint("[BOOT] fixing up branch at "); qprintn(pc);
    qprint(" (ROM: "); qprintn(pc); qprint(") to "); qprintn(dest); qprint("\n");

    // See ARMv7-A / ARMv7-R Ref Manual for encodings

    if (dest % 2) // Thumb target, use BL, firmware is Thumb
    {
        uint32_t opcode = 0xd000f000; // BL fixed bits
        uint32_t offset = dest - (pc + 4);
        uint32_t s = (offset >> 24) & 1;
        uint32_t i1 = (offset >> 23) & 1;
        uint32_t i2 = (offset >> 22) & 1;
        uint32_t imm10 = (offset >> 12) & 0x3ff;
        uint32_t imm11 = (offset >> 1) & 0x7ff;
        uint32_t j1 = (!(i1 ^ s)) & 0x1;
        uint32_t j2 = (!(i2 ^ s)) & 0x1;

        *(uint32_t *)pc = opcode | (s << 10) | imm10 | (j1 << 29) | (j2 << 27) | (imm11 << 16);
    }
    else // ARM target, use BLX
    {
        uint32_t opcode = 0xc000f000; // BLX fixed bits
        uint32_t offset = dest - (pc + 4);
        uint32_t s = (offset >> 24) & 1;
        uint32_t i1 = (offset >> 23) & 1;
        uint32_t i2 = (offset >> 22) & 1;
        uint32_t imm10H = (offset >> 12) & 0x3ff;
        uint32_t imm10L = (offset >> 2) & 0x3ff;
        uint32_t j1 = (!(i1 ^ s)) & 0x1;
        uint32_t j2 = (!(i2 ^ s)) & 0x1;

        *(uint32_t *)pc = opcode | (s << 10) | imm10H | (j1 << 29) | (j2 << 27) | (imm10L << 17);
    }
}

static void my_bzero32(void *buf, size_t len)
{
    bzero32(buf, len);
}

static void my_create_init_task(struct dryos_init_info *dryos, uint32_t init_task, uint32_t c)
{
#ifdef CONFIG_R5
    // DIGIC X re-use the same memory range for coprocessors and autoexec.bin
    //
    // On regular first stage boot, that memory chunk is initialized, then
    // decision is made where to go next: autoexex, firmware update, fromutil,
    // main firmware...
    //
    // If any file is loaded from card, it not only uses that buffer to load,
    // but Canon code also erases all unused part of a buffer.
    //
    // This call executes the function that originally initializes that memory.
    // Call is required here (and not in reboot.c) for safety reasons - reboot.c
    // runs still from buffer in question.
    // Here we already run from relocated code, so it is safe to reinitialize
    // memory.
    extern void reinit_autoexec_memory(void);
    reinit_autoexec_memory();
#endif

    // We wrap Canon's create_init_task, allowing us to modify the
    // struct that it takes, which holds a bunch of OS info.
    // We adjust sizes of memory regions to reserve space for ML.
    //
    // This may, depending on consts.h for the cam, move up both
    // sys_objs start and sys_mem start.
    // The effects of this have not been fully tested.

    // replace Canon's init_task with ours
    init_task = (uint32_t)my_init_task;

    // Reserve memory by reducing the user_mem pool and, if necessary for the
    // requested size, moving up the start of sys_objs and sys_mem.
    // ML goes in the gap.  RESTARTSTART defines the start address of the gap,
    // ML_RESERVED_MEM the size.
    ml_reserved_mem = ML_RESERVED_MEM;

    // align up to 8, DryOS does this for the various mem regions
    // that we are adjusting.
    if (ml_reserved_mem % 8 != 0)
        ml_reserved_mem += 8 - ml_reserved_mem % 8;

    if (RESTARTSTART > dryos->sys_objs_start)
    {   // I don't know of a reason to extend user_mem or leave a gap so this
        // is probably a mistake.
        qprint("[BOOT] unexpected RESTARTSTART address > sys_objs_start\n");
        goto fail;
    }

    // the RESTARTSTART > sys_objs_start guard means mem to steal from user will be positive
    uint32_t steal_from_user_size = dryos->sys_objs_start - RESTARTSTART;
    if (steal_from_user_size > ML_MAX_USER_MEM_STOLEN)
    {
        qprint("[BOOT] RESTARTSTART possibly unsafe, too much stolen from user_mem: ");
        qprintn(steal_from_user_size); qprint("\n");
        goto fail;
    }

    int32_t sys_offset_increase = ml_reserved_mem - steal_from_user_size;
    if (sys_offset_increase < 0)
    { // user mem is enough, no need to move sys mem
        sys_offset_increase = 0;
    }
    if (sys_offset_increase > ML_MAX_SYS_MEM_INCREASE)
    {   // SJE 0x40000 is the most I've tested, and only on 200D
        qprint("[BOOT] sys_offset_increase possibly unsafe, not tested this high, aborting: ");
        qprintn(sys_offset_increase); qprint("\n");
        goto fail;
    }

    qprint("[BOOT] reserving memory: "); qprintn(ml_reserved_mem); qprint("\n");
    qprint("before: user_mem_size = "); qprintn(dryos->user_mem_len); qprint("\n");
    // shrink user_mem
    dryos->user_mem_len -= steal_from_user_size;
    qprint(" after: user_mem_size = "); qprintn(dryos->user_mem_len); qprint("\n");

    // move sys_mem later in ram
    dryos->sys_objs_start += sys_offset_increase;
    dryos->sys_objs_end += sys_offset_increase;
    dryos->sys_mem_start += sys_offset_increase;

    create_init_task(dryos, init_task, c);

    return;

fail:
    while(1); // SJE FIXME kitor wants to do cool stuff here
}

/* M50: Canon code calling these cache functions expects R3 to be preserved after the call */
/* trick to prevent our C compiler for overwriting R3: two unused parameters */
static void my_dcache_clean(uint32_t addr, uint32_t size, uint32_t keep1, uint32_t keep2)
{
    extern void dcache_clean(uint32_t, uint32_t, uint32_t, uint32_t);
    dcache_clean(addr, size, keep1, keep2);
}

static void my_icache_invalidate(uint32_t addr, uint32_t size, uint32_t keep1, uint32_t keep2)
{
    extern void icache_invalidate(uint32_t, uint32_t, uint32_t, uint32_t);
    icache_invalidate(addr, size, keep1, keep2);
}

#if defined(CONFIG_750D) || defined(CONFIG_5D4) // maybe this should be CONFIG_DIGIC_VI
static void my_pre_cstart_func(void)
{
    extern void pre_cstart_func(void);
    pre_cstart_func();
}
#endif

void
__attribute__((noreturn,noinline,naked))
copy_and_restart(int offset)
{
    // This function runs very early, before Canon initialisation code.
    // We want to edit their code to take control, but it's in ROM.
    // We copy the early code into a RWX region; the _reloc buffer
    // defined above.  We will later run this instead of the ROM code,
    // but before doing so we make our edits.

    zero_bss();

    // Copy the firmware to somewhere safe in memory
    const uint8_t *const firmware_start = (void *)ROMBASEADDR;
    const uint32_t firmware_len = FIRMWARE_ENTRY_LEN;
    uint8_t *const new_image = (void *)RELOCADDR;

    blob_memcpy(new_image, firmware_start, firmware_start + firmware_len);
#if defined(CSTART_LEN)
    const uint8_t *const cstart_start = (uint8_t *)((uint32_t)cstart & 0xfffffffe);
    blob_memcpy(new_image + firmware_len,
                cstart_start,
                cstart_start + CSTART_LEN);
#endif

#ifdef CONFIG_DIGIC_78
    // Fix cache maintenance calls before cstart
    patch_thumb_branch(BR_DCACHE_CLN_1, (uint32_t)my_dcache_clean);
    patch_thumb_branch(BR_DCACHE_CLN_2, (uint32_t)my_dcache_clean);
    patch_thumb_branch(BR_ICACHE_INV_1, (uint32_t)my_icache_invalidate);
    patch_thumb_branch(BR_ICACHE_INV_2, (uint32_t)my_icache_invalidate);

    // On D78, there's an indirect branch to branch to cstart,
    // the first branch goes to absolute cstart original address.
    // Patch the setup for that into a relative branch to our reloc'd cstart.
    patch_thumb_branch(BR_BR_CSTART, reloc_addr((uint32_t)cstart));

    /* there are two more functions in cstart that don't require patching */
    /* the first one is within the relocated code; it initializes the per-CPU data structure at VA 0x1000 */
    /* the second one is called only when running on CPU1; assuming our code only runs on CPU0 */
#endif

#if defined(CSTART_LEN)
    // if we're compacting firmware_entry and cstart,
    // we need to patch the jump
    uint32_t reloc_cstart = reloc_addr((uint32_t)cstart_start);
    patch_thumb_branch(BR_CSTART, reloc_cstart | 0x1);
#endif

    // if firmware_entry calls code in the cstart reloc'd region,
    // we also need to patch that
#if defined(CONFIG_750D) || defined(CONFIG_5D4) // maybe this should be CONFIG_DIGIC_VI
    patch_thumb_branch(BR_PRE_CSTART, (uint32_t)my_pre_cstart_func);
#endif

    // Fix the calls to bzero32() and create_init_task() in cstart.
    //
    // Our my_create_init_task wraps Canon create_init_task
    // and modifies OS memory layout to make room for ML
    patch_thumb_branch(BR_BZERO32, (uint32_t)my_bzero32);
    patch_thumb_branch(BR_CREATE_ITASK, (uint32_t)my_create_init_task);

    // Make sure that our self-modifying code clears the cache
    sync_caches();

    // Jump to copied firmware code in our modified buffer.
#ifdef CONFIG_DIGIC_VI
    // In D6 this code starts in ARM.  This function is Thumb.
    // The first few instructions do nothing apart from switch mode to Thumb,
    // so we can instead skip them.
    thunk __attribute__((long_call)) reloc_entry = (thunk)(RELOCADDR + 0xc + 1);
#elif defined(CONFIG_DIGIC_78)
    thunk __attribute__((long_call)) reloc_entry = (thunk)(RELOCADDR + 1);
#endif
    qprint("[BOOT] jumping to relocated startup code at "); qprintn((uint32_t)reloc_entry); qprint("\n");
    reloc_entry();

    // Unreachable
    while(1)
        ;
}
