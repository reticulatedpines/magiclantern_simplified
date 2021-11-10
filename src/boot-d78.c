/** \file
 * Startup code for DIGIC 7 & 8
 */

#include "dryos.h"
#include "boot.h"

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

/** This just goes into the bss */
#ifdef CONFIG_DIGIC_VI
// SJE FIXME move RELOCSIZE to per cam dir and include via header
    #define RELOCSIZE 0x3d100
#elif defined CONFIG_DIGIC_78
    #define RELOCSIZE 0x1000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
#else
    #error "Expected D678"
#endif
static uint32_t _reloc[ RELOCSIZE / 4 ];
#define RELOCADDR ((uintptr_t) _reloc)

static inline uint32_t thumb_branch_instr(uint32_t pc, uint32_t dest, uint32_t opcode)
{
    /* thanks atonal */
    //uint32_t offset = dest - ((pc + 4) & ~3); /* according to datasheets, this should be the correct calculation -> ALIGN(PC, 4) */
    uint32_t offset = dest - (pc + 4);  /* this one works for BX */
    uint32_t s = (offset >> 24) & 1;
    uint32_t i1 = (offset >> 23) & 1;
    uint32_t i2 = (offset >> 22) & 1;
    uint32_t imm10 = (offset >> 12) & 0x3ff;
    uint32_t imm11 = (offset >> 1) & 0x7ff;
    uint32_t j1 = (!(i1 ^ s)) & 0x1;
    uint32_t j2 = (!(i2 ^ s)) & 0x1;

    return opcode | (s << 10) | imm10 | (j1 << 29) | (j2 << 27) | (imm11 << 16);
}

#define THUMB_B_W_INSTR(pc,dest)    thumb_branch_instr((uint32_t)(pc), (uint32_t)(dest), 0x9000f000)
#define THUMB_BL_INSTR(pc,dest)     thumb_branch_instr((uint32_t)(pc), (uint32_t)(dest), 0xd000f000)
#define THUMB_BLX_INSTR(pc,dest)    thumb_branch_instr((uint32_t)(pc), (uint32_t)(dest), 0xc000f000)

#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#ifdef CONFIG_DIGIC_78
// SJE FIXME let's stop using so many insane macros
    #define FIXUP_BRANCH( rom_addr, dest_addr ) \
        qprint("[BOOT] fixing up branch at "); qprintn((uint32_t) &INSTR( rom_addr )); \
        qprint(" (ROM: "); qprintn(rom_addr); qprint(") to "); qprintn((uint32_t)(dest_addr)); qprint("\n"); \
        INSTR( rom_addr ) = THUMB_BL_INSTR( &INSTR( rom_addr ), (dest_addr) )
#elif defined(CONFIG_DIGIC_VI)
    #define FIXUP_BRANCH( rom_addr, dest_addr ) \
        qprint("[BOOT] fixing up branch at "); qprintn((uint32_t) &INSTR( rom_addr )); \
        qprint(" (ROM: "); qprintn(rom_addr); qprint(") to "); qprintn((uint32_t)(dest_addr)); qprint("\n"); \
        INSTR( rom_addr ) = THUMB_BLX_INSTR( &INSTR( rom_addr ), (dest_addr) )
#else
    #error "Expected D678!"
#endif

static void my_bzero32(void* buf, size_t len)
{
    bzero32(buf, len);
}

#ifdef CONFIG_DIGIC_678
// SJE FIXME - probably move this to a header
struct dryos_init_info
{
    uint32_t sys_mem_start;
    uint32_t sys_mem_len;
    uint32_t user_mem_start;
    uint32_t user_mem_len;
    uint32_t sys_objs_start;
    uint32_t sys_objs_end;

/* more fields are here but we don't use them.
   D45 cams are shorter, look to be missing one
   of condition_max, timer_max or vector_max.

   Since we only use pointer to this struct,
   we never reserve space for it and don't care.

    uint32_t prio_max;
    uint32_t task_max;
    uint32_t semaphore_max;
    uint32_t event_max;
    uint32_t message_q_max;
    uint32_t mutex_max;
    uint32_t condition_max;
    uint32_t timer_max;
    uint32_t vector_max;
    uint32_t unk_01;
    uint32_t unk_02;
    uint32_t unk_03;
    uint32_t unk_04;
    uint32_t unk_05;
    uint32_t unk_06;
    uint32_t unk_07;
    uint32_t unk_08;
    uint32_t prio_default;
    uint32_t stack_default;
    uint32_t stack_idle;
    uint32_t stack_init;
*/
};
#endif

#ifdef CONFIG_QEMU
static void dump_dryos_init_info(struct dryos_init_info *dryos)
{
    qprintn(dryos->sys_mem_start); qprint("\n");
    qprintn(dryos->sys_mem_len); qprint("\n");
    qprintn(dryos->user_mem_start); qprint("\n");
    qprintn(dryos->user_mem_len); qprint("\n");
    qprintn(dryos->sys_objs_start); qprint("\n");
    qprintn(dryos->sys_objs_end); qprint("\n");
}
#endif

static void my_create_init_task(struct dryos_init_info *dryos, uint32_t init_task, uint32_t c)
{
    // We wrap Canon's create_init_task, allowing us to modify the
    // struct that it takes, which holds a bunch of OS info.
    // We adjust sizes of memory regions to reserve space for ML.
    //
    // This may, depending on consts.h for the cam, move up both
    // sys_objs start and sys_mem start.
    // The effects of this have not been fully tested.

    // Reserve memory by reducing the user_mem pool and, if necessary for the
    // requested size, moving up the start of sys_objs and sys_mem.
    // ML goes in the gap.  RESTARTSTART defines the start address of the gap,
    // ML_RESERVED_MEM the size.
    ml_reserved_mem = ML_RESERVED_MEM;
    dump_dryos_init_info(dryos);

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
        qprint("[BOOT] RESTARTSTART possibly unsafe, too much stolen from user_mem\n");
        goto fail;
    }

    int32_t sys_offset_increase = ml_reserved_mem - steal_from_user_size;
    if (sys_offset_increase < 0)
    {
        qprint("[BOOT] sys_mem_offset_increase was negative, shouldn't happen!\n");
        goto fail;
    }
    if (sys_offset_increase > ML_MAX_SYS_MEM_INCREASE)
    {   // SJE 0x40000 is the most I've tested, and only on 200D
        qprint("[BOOT] sys_offset_increase possibly unsafe, not tested this high, aborting\n");
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

    dump_dryos_init_info(dryos);

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
    const uint32_t firmware_len = RELOCSIZE;
    uint32_t *const new_image = (void *)RELOCADDR;

    blob_memcpy(new_image, firmware_start, firmware_start + firmware_len);

#ifdef CONFIG_DIGIC_78
    // Fix cache maintenance calls before cstart
    FIXUP_BRANCH(HIJACK_FIXBR_DCACHE_CLN_1, my_dcache_clean);
    FIXUP_BRANCH(HIJACK_FIXBR_DCACHE_CLN_2, my_dcache_clean);
    FIXUP_BRANCH(HIJACK_FIXBR_ICACHE_INV_1, my_icache_invalidate);
    FIXUP_BRANCH(HIJACK_FIXBR_ICACHE_INV_2, my_icache_invalidate);

    // SJE FIXME - this comment is untrue for 200D,
    // it's a relative jump.  Is it true for any cams?

    // Fix the absolute jump to cstart
    FIXUP_BRANCH(HIJACK_INSTR_BL_CSTART, &INSTR(cstart));

    /* there are two more functions in cstart that don't require patching */
    /* the first one is within the relocated code; it initializes the per-CPU data structure at VA 0x1000 */
    /* the second one is called only when running on CPU1; assuming our code only runs on CPU0 */
#endif

    // Fix the calls to bzero32() and create_init_task() in cstart
    FIXUP_BRANCH(HIJACK_FIXBR_BZERO32, my_bzero32);
    FIXUP_BRANCH(HIJACK_FIXBR_CREATE_ITASK, my_create_init_task);

    // Set our init task to run instead of the firmware one
    qprint("[BOOT] changing init_task from "); qprintn(INSTR(HIJACK_INSTR_MY_ITASK));
    qprint("to "); qprintn((uint32_t)my_init_task); qprint("\n");
    INSTR(HIJACK_INSTR_MY_ITASK) = (uint32_t)my_init_task;

    // Make sure that our self-modifying code clears the cache
    sync_caches();

    // jump to Canon firmware (Thumb code)
#ifdef CONFIG_DIGIC_VI
    thunk __attribute__((long_call)) reloc_entry = (thunk)(RELOCADDR + 0xc + 1);
#elif defined(CONFIG_DIGIC_78)
    thunk __attribute__((long_call)) reloc_entry = (thunk)(RELOCADDR + 1);
#else
    #error "Expected D678"
#endif
    qprint("[BOOT] jumping to relocated startup code at "); qprintn((uint32_t)reloc_entry); qprint("\n");
    reloc_entry();

    // Unreachable
    while(1)
        ;
}
