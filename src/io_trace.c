/**
 * This installs a data abort handler in the MMIO region
 * (a subset of C0000000 - CFFFFFFF) and logs all reads/writes from this range.
 *
 * These logs are very useful for emulation.
 *
 * Based on mem_prot.
 */

#include <dryos.h>
#include <dm-spy.h>

#define ASM_VAR  __attribute__((section(".text"))) __attribute__((used))

/* select the MMIO range to be logged: */

#ifdef CONFIG_DIGIC_VI
    /* DIGIC 6 (Cortex R4):
     * DRBAR, mask 0xFFFFFFE0: base address
     *  DRSR, mask 0x0000003E: region size (min. 4096, power of 2)
     *  DRSR, mask 0x00000001: enabled (must be 1)
     *  DRSR, mask 0x0000FF00: sub-region disable bits
     * Cortex R4 TRM, 4.3.20 c6, MPU memory region programming registers
     */
    #define REGION_BASE(base) (base & 0xFFFFFFE0)
    #define REGION_SIZE(size) ((LOG2(size/2) << 1) | 1)
#else
    /* DIGIC V and earlier (ARM946E-S):
     * PRBSn, mask FFFFF000: base address
     *             0000001E: region size (min. 4096, power of 2)
     *             00000001: enabled (must be 1)
     * ARM946E-S TRM, 2.3.9 Register 6, Protection Region Base and Size Registers
     */
    #define REGION(base, size) ((base & 0xFFFFF000) | (LOG2(size/2) << 1) | 1)
#endif

/* address ranges can be found at magiclantern.wikia.com/wiki/Register_Map
 * or in QEMU source: contrib/qemu/eos/eos.c, eos_handlers
 * examples:
 * REGION(0xC0220000, 0x010000): GPIO (LEDs, chip select signals etc)
 * REGION(0xC0820000, 0x001000): SIO communication (SPI)
 * REGION(0xC0210000, 0x001000): timers (activity visible only with early startup logging, as in da607f7 or !1dd2792)
 * REGION(0xC0243000, 0x001000): HPTimer (high resolution timers)
 * REGION(0xC0200000, 0x002000): interrupt controller (it works! interrupts are interrupted!)
 * REGION(0xC0F00000, 0x010000): EDMAC #0-15 and many others
 * REGION(0xC0F00000, 0x100000): EDMAC (all), display (C0F14) and many others (more likely to crash)
 * REGION(0xC0C00000, 0x100000): SDIO/SFIO (also paired with SDDMA/SFDMA, unsure whether they can be logged at the same time)
 * REGION(0xC0500000, 0x100000): SDDMA/SFDMA/CFDMA
 * REGION(0xC0E20000, 0x010000): JPCORE (JP57, lossless)
 * REGION(0xC0000000, 0x1000000): nearly everything except EEKO? (DIGIC <= 5)
 * REGION(0xC0000000, 0x20000000): everything including EEKO? (DIGIC 5)
 * REGION(0xE0000000, 0x1000): DFE (5D2, 50D); untested
 */

#ifdef CONFIG_DIGIC_VI

//static ASM_VAR uint32_t protected_region_base = REGION_BASE(0xC0000000);
//static ASM_VAR uint32_t protected_region_size = REGION_SIZE(0x20000000);

/* This logs the entire MMIO activity, from B0000000 to DFFFFFFF:
 * BFE00000 - BFEFFFFF: used for communicating with Omar
 * BFF00000 - BFFFFFFF: used for communicating with Zico (MRZM)
 * C0000000 - DFFFFFFF: regular MMIO range */
static ASM_VAR uint32_t protected_region_base = REGION_BASE(0x80000000);
static ASM_VAR uint32_t protected_region_size = REGION_SIZE(0x80000000) | 0xC700;

/* How it works: on DIGIC 6, each memory protection region can be divided
 * in 8 equal subregions. You may enable/disable any of them (bit set = subregion disabled).
 * Subregion 80000000 - 8FFFFFFF: disabled (covers BTCM)
 * Subregion 90000000 - 9FFFFFFF: disabled (unused?)
 * Subregion A0000000 - AFFFFFFF: disabled (unused?)
 * Subregion B0000000 - BFFFFFFF: enabled  (Omar/Zico communication)
 * Subregion C0000000 - CFFFFFFF: enabled  (regular MMIO range - used in earlier models)
 * Subregion D0000000 - DFFFFFFF: enabled  (regular MMIO range - also used, to a lesser extent, in DIGIC 5)
 * Subregion E0000000 - EFFFFFFF: disabled (unused? there is a memory region configured as 0xEE000000 - EFFFFFFF)
 * Subregion F0000000 - FFFFFFFF: disabled (ROM)
 */

#else /* DIGIC V and earlier */

static ASM_VAR uint32_t protected_region = REGION(0xC0000000,
                                                  0x20000000);

#endif

/* number of 32-bit integers recorded for one MMIO event (power of 2) */
#define RECORD_SIZE 8

static uint32_t trap_orig = 0;

/* buffer size must be power of 2 and multiple of RECORD_SIZE, to simplify bounds checking */
static ASM_VAR uint32_t * buffer = 0;
static ASM_VAR uint32_t buffer_index = 0;
static uint32_t buffer_count = 0;   /* how many uint32_t's we have allocated */

/*
 * TCM usage in main firmware
 * ==========================
 *
 * checked 60D, 500D, 5D2, 50D, 1300D, 5D3, 70D, 700D, 100D, EOSM in QEMU
 * procedure:
 *   - fill both TCMs with 0xBADCAFFE when PC reaches F8010000 or F80C0000
 *     (doable from either QEMU dbi/logging.c or GDB breakpoint)
 *   - confirm that it still boots the GUI and you can navigate Canon menus
 *   - examine TCM contents with 'x/1024 0' and 'x/1024 0x40000000' in GDB
 *
 * DIGIC 4:
 * 400006F8-40000AF7 ISR handler table (256 entries?)
 * 40000AF8-40000EF7 ISR argument table
 * 40000000-400006F7 possibly unused (no code reads/writes it in QEMU)
 * 40000EF8-40000FFF possibly unused
 *
 * DIGIC 5 and 1300D:
 * 40000000-400007FF ISR handler table (512 entries, some unused?)
 * 40000800-40000FFF ISR argument table
 * 40000E00-40000FFF possibly unused ISR entries? probably best not to rely on it
 *
 * DIGIC 4 and 5:
 * 00000000-00000020 exception vectors (0x18 = IRQ, 0x10 = Data Abort etc)
 * 00000020-00000040 exception routines (used with LDR PC, [PC, #20])
 * 00000120-000001BF some executable code (60D: copied from FF055CA8; 1300D,100D: 00000100-000001BF; 70D: not present)
 * 000004B0-000006C0 interrupt handler (code jumps there from 0x18; end address may vary slightly)
 * 000006C4-00001000 interrupt stack (start address may vary slightly; used until about 0xf00)
 *
 * other exception stacks are at UND:400007fc FIQ:4000077c ABT:400007bc SYS/USR:4000057c (IRQ was 4000067c)
 * these were set up from bootloader, but they are no longer valid in main firmware
 * pick SP at the bottom of the interrupt stack, roughly 0x700-0x740
 *
 * 80D:
 * 00000000-00002BFF ATCM, used by Canon code
 * 00002C00-00003FFF ATCM, unused
 * 80000000-8000A4BF BTCM, used by Canon code
 * 8000A4C0-8000FFFF BTCM, unused
 *
 * 5D4:
 * 00000000-0000338F ATCM, used by Canon code
 * 00003390-00003FFF ATCM, unused
 * 80000000-80009C7F BTCM, used by Canon code
 * 80009C80-8000FFFF BTCM, unused
 *
 * 750D/760D:
 * 00000000-00003D4F ATCM, used by Canon code
 * 00003D50-00003FFF ATCM, unused
 * 80000000-8000923F BTCM, used by Canon code
 * 80009240-8000FFFF BTCM, unused
 *
 */

static void __attribute__ ((naked)) trap()
{
    /* data abort exception occurred. switch stacks, log the access,
     * enable permissions and re-execute trapping instruction */
    asm(
        /* set up a stack in some unused area in the TCM (we need 64 bytes) */
#ifdef CONFIG_DIGIC_VI
        "MOV    SP,     #0x0000FF00\n"
        "ORR    SP, SP, #0x80000000\n"
#else /* DIGIC V and earlier */
        "MOV    SP, #0x740\n"
#endif

        /* save context, including flags */
        "STMFD  SP!, {LR}\n"            /* LR_ABT (where the exception happened) */
        "STMFD  SP!, {R0-R12, LR}\n"    /* R0-R12,LR of the interrupted mode (LR will be updated later) */
        "MRS    R0,  CPSR\n"            /* read condition flags (must be done after saving R0) */
        "STMFD  SP!, {R0}\n"            /* store condition flags on the stack */

        /* save registers from the interrupted mode */
        "MRS    R8, SPSR\n"             /* CPSR of the interrupted mode */
        "AND    R1, R8, #0x1F\n"        /* what was the interrupted CPU mode? */
        "ORR    R1, #0xC0\n"            /* keep the interrupts disabled and stay in ARM mode */
        "MOV    R12, SP\n"              /* keep the SP_ABT before switching */
        "MSR    CPSR_c, R1\n"           /* switch to the interrupted mode (it won't be user) */
        "STR    LR, [R12, #0x38]\n"     /* store LR of the interrupted mode */
        "MSR    CPSR_c, R0\n"           /* back to Data Abort mode */

        /* prepare to save information about trapping code */
        "LDR    R4, buffer\n"           /* load buffer address */
        "LDR    R2, buffer_index\n"     /* load buffer index from memory */
        "TST    R2, #0x3FC00000\n"      /* check for buffer overflow (16MB buffer) */
        "MOVNE  R2, #0x00400000\n"      /* we have allocated 0x400000+8 words (0x80000+1 records) */

        /* interrupted code was ARM or Thumb? */
        "TST    R8, #0x20\n"            /* R8 contains SPSR */

        /* store the program counter */
        "SUB    R5, LR, #8\n"           /* retrieve PC where the exception happened */
        "MOVEQ  R6, R5\n"               /* if we have interrupted ARM code, store just the PC */
        "ORRNE  R6, R5, #1\n"           /* otherwise, store the Thumb bit as well */
        "STR    R6, [R4, R2, LSL#2]\n"  /* store PC at index [0], possibly with the Thumb bit set */
        "ADD    R2, #1\n"               /* increment index */

        /* get and store DryOS task name and interrupt ID */
        "LDR    R0, =current_task\n"
        "LDR    R1, [R0, #4]\n"         /* 1 if running in interrupt, 0 otherwise; other values? */
        "LDR    R0, [R0]\n"
        "LDR    R0, [R0, #0x24]\n"

        "STR    R0, [R4, R2, LSL#2]\n"  /* store task name at index [1] */
        "ADD    R2, #1\n"               /* increment index */

        "LDR    R0, =current_interrupt\n"
        "LDR    R0, [R0]\n"             /* interrupt ID is shifted by 2 on DIGIC 5 and earlier, but not on DIGIC 6 */
        "ORR    R0, R1, LSL#31\n"       /* store whether the interrupt ID is valid, in the MSB */
        "STR    R0, [R4, R2, LSL#2]\n"  /* store interrupt ID at index [2] */
        "ADD    R2, #1\n"               /* increment index */

        /* prepare to re-execute the old instruction */
        /* copy it into this routine (cacheable memory) */
        "SUB    R5, LR, #8\n"           /* retrieve PC where the exception happened */
        "LDR    R6, [R5]\n"             /* read the old instruction */

#ifdef CONFIG_DIGIC_VI
        /* we have 3 cases:
         * - ARM instruction (4 bytes wide)
         * - Thumb instruction (2 bytes wide)
         * - Thumb instruction (4 bytes wide)
         *
         * - ARM vs Thumb: bit 5 in SPSR is set in Thumb mode
         * - Thumb instruction: wide if bits [15:11] of current halfword
         *   are 0b11101/0b11110/0b11111 (A6.1 in ARMv7-AR)
         */

        /* last check was: TST R8, #0x20; R8 contains SPSR */
        "BNE    interrupted_thumb\n"
#endif /* CONFIG_DIGIC_VI */

        /* ARM code interrupted */
        "interrupted_arm:\n"

        /* where are we going to copy the old instruction? */
        "ADR    R1, trapped_instruction_arm\n"

        /* store the old instruction */
        "STR    R6, [R1]\n"

        /* clean the cache for this address (without touching the cache hacks),
         * then disable our memory protection region temporarily for re-execution */
        "MOV    R0, #0x00\n"
        "MCR    p15, 0, R1, c7, c10, 1\n"   /* first clean that address in dcache */
        "MCR    p15, 0, R0, c7, c10, 4\n"   /* then drain write buffer */
        "MCR    p15, 0, R1, c7, c5, 1\n"    /* flush icache line for that address */

#ifdef CONFIG_DIGIC_VI
        "MOV    R7, #0x07\n"
        "MCR    p15, 0, R7, c6, c2, 0\n"    /* write RGNR (adjust memory region #7) */
        "MCR    p15, 0, R0, c6, c1, 2\n"    /* enable full access to memory (disable region #7) */
#else /* DIGIC V and earlier */
        "MCR    p15, 0, R0, c6, c7, 0\n"    /* enable full access to memory */
#endif

#ifdef CONFIG_QEMU
        /* disassemble the instruction */
        "LDR    R3, =0xCF123010\n"
        "STR    R5, [R3]\n"
#endif

        /* find the source and destination registers */
        "MOV    R1, R6, LSR#12\n"           /* extract destination register */
        "AND    R1, #0xF\n"
        "STR    R1, destination\n"          /* store it to memory; will use it later */

#ifdef CONFIG_DIGIC_VI
        "MRC    p15, 0, R0, c5, c0, 0\n"    /* read DFSR */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store DFSR (fault status) at index [3] */
        "ADD    R2, #1\n"                   /* increment index */

        "MRC    p15, 0, R0, c6, c0, 0\n"    /* read DFAR */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store DFAR (fault address) at index [4] */
        "ADD    R2, #1\n"                   /* increment index */

#else /* DIGIC V and earlier */

        /* no DFAR; we need to manually decode the instruction to figure it out */
        "MOV    R1, R6, LSR#16\n"           /* extract source register */
        "AND    R1, #0xF\n"
        "LDR    R0, [SP, R1, LSL#2]\n"      /* load source register contents from stack */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store source register at index [3] */
        "ADD    R2, #1\n"                   /* increment index */

        "AND    R1, R6, #0xF\n"             /* extract index register (only valid in "register offset" addressing modes; will decode later) */
        "LDR    R0, [SP, R1, LSL#2]\n"      /* load index register contents from stack */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store index register at index [4] */
        "ADD    R2, #1\n"                   /* increment index */
#endif

        "STR    R2, buffer_index\n"         /* store buffer index to memory */

        /* restore context, with LR of interuped mode */
        "LDMFD  SP!, {R0}\n"
        "MSR    CPSR_f, R0\n"
        "LDMFD  SP!, {R0-R12, LR}\n"

        /* placeholder for executing the old instruction (as ARM) */
        "trapped_instruction_arm:\n"
        ".word 0x00000000\n"

        /* save context once again (sans flags) */
        "STMFD  SP!, {R0-R12, LR}\n"

#ifdef CONFIG_QEMU
        /* print the register and value loaded from MMIO */
        "LDR    R3, =0xCF123000\n"
        "LDR    R0, destination\n"
        "LDR    R0, [SP, R0, LSL#2]\n"
        "STR    R0, [R3,#0xC]\n"
        "MOV    R0, #10\n"
        "STR    R0, [R3]\n"
#endif

        /* store the result (value read from / written to MMIO) */
        "LDR    R4, buffer\n"               /* load buffer address */
        "LDR    R2, buffer_index\n"         /* load buffer index from memory */
        "LDR    R0, destination\n"          /* load destination register index from memory */
        "LDR    R0, [SP, R0, LSL#2]\n"      /* load destination register contents from stack */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store destination register at index [5] */
        "ADD    R2, #1\n"                   /* increment index */

        /* timestamp the event (requires MMIO access; do this before re-enabling memory protection) */
#ifdef CONFIG_DIGIC_VI
        "LDR    R0, =0xD400000C\n"          /* 32-bit microsecond timer */
#else /* DIGIC V and earlier */
        "LDR    R0, =0xC0242014\n"          /* 20-bit microsecond timer */
#endif
        "LDR    R0, [R0]\n"
        "STR    R0, [R4, R2, LSL#2]\n"      /* store timestamp at index[6]; will be unwrapped in post */
        "ADD    R2, #1\n"                   /* increment index */

        "STR    LR, [R4, R2, LSL#2]\n"      /* store LR of the interrupted mode at index [7] */
        "ADD    R2, #1\n"                   /* increment index (total 8 = RECORD_SIZE) */
        "STR    R2, buffer_index\n"         /* store buffer index to memory */

        /* re-enable memory protection */
#ifdef CONFIG_DIGIC_VI
        "LDR    R0, protected_region_size\n"
        "MCR    p15, 0, R0, c6, c1, 2\n"
#else /* DIGIC V and earlier */
        "LDR    R0, protected_region\n"
        "MCR    p15, 0, R0, c6, c7, 0\n"
#endif

        /* restore context */
        "LDMFD  SP!, {R0-R12, LR}\n"

        /* restore LR_ABT (where we should continue the execution) */
        "LDMFD  SP!, {LR}\n"

        /* continue the execution after the trapped instruction */
        "SUBS   PC, LR, #4\n"

        /* ------------------------------------------ */

        "destination:\n"
        ".word 0x00000000\n"

        /* ARM code path finished */
        /* FIXME: reduce amount of duplicate code */
        /* ----------------------------------------------------- */

#ifdef CONFIG_DIGIC_VI
        /* Thumb code interrupted */
        "interrupted_thumb:\n"

        /* where are we going to copy the old instruction? */
        "ADR    R1, trapped_instruction_thumb\n"

        /* for "narrow" Thumb instructions, replace the upper 16 bits with a NOP */
        /* the old instruction is in R6 */
        "AND    R7, R6, #0xF800\n"
        "CMP    R7, #0xF800\n"
        "CMPNE  R7, #0xF000\n"
        "CMPNE  R7, #0xE800\n"
        "BICNE  R6, #0xFF000000\n"
        "BICNE  R6, #0x00FF0000\n"
        "ORRNE  R6, #0xBF000000\n"

        /* store the old instruction */
        "STR    R6, [R1]\n"

        /* adjust LR_ABT for next instruction, if we have interrupted a "narrow" one */
        "SUBNE  LR, LR, #2\n"
        "STRNE  LR, [SP, #0x3C]\n"

        /* clean the cache for this address (without touching the cache hacks),
         * then disable our memory protection region temporarily for re-execution */
        "MOV    R0, #0x00\n"
        "MCR    p15, 0, R1, c7, c10, 1\n"   /* first clean that address in dcache */
        "MCR    p15, 0, R0, c7, c10, 4\n"   /* then drain write buffer */
        "MCR    p15, 0, R1, c7, c5, 1\n"    /* flush icache line for that address */
        "MOV    R7, #0x07\n"
        "MCR    p15, 0, R7, c6, c2, 0\n"    /* write RGNR (adjust memory region #7) */
        "MCR    p15, 0, R0, c6, c1, 2\n"    /* enable full access to memory (disable region #7) */

#ifdef CONFIG_QEMU
        /* disassemble the instruction (as Thumb) */
        "LDR    R3, =0xCF123010\n"
        "ORR    R0, R5, #1\n"
        "STR    R0, [R3]\n"
#endif

        /* find the destination register */
        "ANDNE  R1, R6, #0x7\n"             /* Rt, for "narrow" instruction (T1 encoding) */
        "MOVEQ  R1, R6, LSR#28\n"           /* Rt, for "wide" instruction (T2 encoding) */
        "STR    R1, destination\n"          /* store it to memory; will use it later */

        "MRC    p15, 0, R0, c5, c0, 0\n"    /* read DFSR */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store DFSR (fault status) at index [3] */
        "ADD    R2, #1\n"                   /* increment index */

        "MRC    p15, 0, R0, c6, c0, 0\n"    /* read DFAR */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store DFAR (fault address) at index [4] */
        "ADD    R2, #1\n"                   /* increment index */

        "STR    R2, buffer_index\n"         /* store buffer index to memory */

        /* switch to Thumb mode */
        "MOV    R12, PC\n"
        "ADD    R12, #5\n"
        "BX     R12\n"
        ".code  16\n"
        ".syntax unified\n"

        /* restore context, with LR of interrupted mode */
        "NOP\n"
        "LDMFD  SP!, {R0}\n"
        "MSR    CPSR_f, R0\n"
        "LDMFD  SP!, {R0-R12, LR}\n"

        /* placeholder for executing the old instruction (as Thumb) */
        "trapped_instruction_thumb:\n"
        ".word 0x00000000\n"

        /* back to ARM mode */
        /* careful with alignment */
        "BX     PC\n"
        ".code  32\n"

        /* save context once again (sans flags) */
        "STMFD  SP!, {R0-R12, LR}\n"

#ifdef CONFIG_QEMU
        /* print the register and value loaded from MMIO */
        "LDR    R3, =0xCF123000\n"
        "LDR    R0, destination\n"
        "LDR    R0, [SP, R0, LSL#2]\n"
        "STR    R0, [R3,#0xC]\n"
        "MOV    R0, #10\n"
        "STR    R0, [R3]\n"
#endif

        /* store the result (value read from / written to MMIO) */
        "LDR    R4, buffer\n"               /* load buffer address */
        "LDR    R2, buffer_index\n"         /* load buffer index from memory */
        "LDR    R0, destination\n"          /* load destination register index from memory */
        "LDR    R0, [SP, R0, LSL#2]\n"      /* load destination register contents from stack */
        "STR    R0, [R4, R2, LSL#2]\n"      /* store destination register at index [5] */
        "ADD    R2, #1\n"                   /* increment index */

        /* timestamp the event (requires MMIO access; do this before re-enabling memory protection) */
        "LDR    R0, =0xD400000C\n"          /* 32-bit microsecond timer */
        "LDR    R0, [R0]\n"
        "STR    R0, [R4, R2, LSL#2]\n"      /* store timestamp at index[6]; will be unwrapped in post */
        "ADD    R2, #1\n"                   /* increment index */

        "STR    LR, [R4, R2, LSL#2]\n"      /* store LR of the interrupted mode at index [7] */
        "ADD    R2, #1\n"                   /* increment index (total 8 = RECORD_SIZE) */
        "STR    R2, buffer_index\n"         /* store buffer index to memory (this requires ARM mode) */

        /* re-enable memory protection */
        "LDR    R0, protected_region_size\n"
        "MCR    p15, 0, R0, c6, c1, 2\n"

        /* restore context */
        "LDMFD  SP!, {R0-R12, LR}\n"

        /* restore LR_ABT (where we should continue the execution) */
        "LDMFD  SP!, {LR}\n"

        /* continue the execution after the trapped instruction */
        /* note: LR was adjusted earlier for "narrow" instructions */
        "SUBS   PC, LR, #4\n"

        /* Thumb code path finished */
        /* ----------------------------------------------------- */
#endif  /* CONFIG_DIGIC_VI */
    );
}

#define TRAP_INSTALLED (MEM(0x0000002C) == (uint32_t) &trap)

void io_trace_uninstall()
{
    ASSERT(buffer);
    ASSERT(trap_orig);
    ASSERT(TRAP_INSTALLED);

    if (!TRAP_INSTALLED)
    {
        /* not installed, nothing to do */
        return;
    }

    uint32_t int_status = cli();

    /* remove our trap handler */
    MEM(0x0000002C) = (uint32_t)trap_orig;

    sync_caches();

    asm(
        /* enable full access to memory */
#ifdef CONFIG_DIGIC_VI
        "MOV    R0, #0x07\n"
        "MCR    p15, 0, R0, c6, c2, 0\n"    /* write RGNR (adjust memory region #7) */
        "MOV    R0, #0x00\n"
        "MCR    p15, 0, R0, c6, c1, 2\n"    /* enable full access to memory (disable region #7) */
#else /* DIGIC V and earlier */
        "MOV     R0, #0x00\n"
        "MCR     p15, 0, r0, c6, c7, 0\n"
#endif
        ::: "r0"
    );

    sync_caches();

    sei(int_status);
}

/* to be called after uninstallation, to free the buffer */
void io_trace_cleanup()
{
    ASSERT(buffer);
    ASSERT(!TRAP_INSTALLED);

    if (TRAP_INSTALLED)
    {
        /* called at the wrong moment; don't do more damage */
        return;
    }

#if 0
    free(buffer);
    buffer = 0;
#endif
}

void io_trace_prepare()
{
#if 0
    extern int ml_started;
    if (!ml_started)
    {
        qprintf("[io_trace] FIXME: large allocators not available\n");
        return;
    }
#endif

    qprintf("[io_trace] allocating memory...\n");

    /* allocate RAM */
    buffer_index = 0;
    buffer_count = 4*1024*1024;
    int alloc_size = (buffer_count + RECORD_SIZE) * sizeof(buffer[0]);
    ASSERT(!buffer);
#if 0
    /* FIXME: no large allocators yet */
    buffer = malloc(alloc_size);
#else

    #ifdef CONFIG_80D
    /* hardcoded address, model-specific, see log_start() for details */
    buffer = (void *) 0x28000000;
    #endif

    #ifdef CONFIG_5D4
    /* let's hope it's OK; appears to be used during bursts */
    buffer = (void *) 0x20B00000;
    #endif

#endif
    if (!buffer) return;
    memset(buffer, 0, alloc_size);
}

void io_trace_install()
{
    if (!buffer)
    {
        qprintf("[io_trace] no buffer allocated\n");
        return;
    }

    qprintf("[io_trace] installing...\n");

    uint32_t int_status = cli();

    /* install data abort handler */
    trap_orig = MEM(0x0000002C);
    MEM(0x0000002C) = (uint32_t) &trap;

    /* also needed before? */
    sync_caches();

    /* set buffer/cache bits for the logged region */
    asm(
#ifdef CONFIG_DIGIC_VI

        /* enable memory protection */
        "MOV    R7, #0x07\n"
        "MCR    p15, 0, R7, c6, c2, 0\n"        /* write RGNR (adjust memory region #7) */
        "LDR    R0, protected_region_base\n"
        "MCR    p15, 0, R0, c6, c1, 0\n"        /* write DRBAR (base address) */
        "LDR    R0, protected_region_size\n"
        "MCR    p15, 0, R0, c6, c1, 2\n"        /* write DRSR (size and enable register) */
        "MOV    R0, #0x005\n"                   /* like 0xC0000000, but with AP bits disabled */
        "MCR    p15, 0, R0, c6, c1, 4\n"        /* write DRACR (access control register) */
        : : : "r7"

#else /* DIGIC V and earlier */

        /* set area uncacheable (already set up that way, but...) */
        "mrc    p15, 0, R4, c2, c0, 0\n"
        "bic    r4, #0x80\n"
        "mcr    p15, 0, R4, c2, c0, 0\n"
        "mrc    p15, 0, R4, c2, c0, 1\n"
        "bic    r4, #0x80\n"
        "mcr    p15, 0, R4, c2, c0, 1\n"

        /* set area non-bufferable (already set up that way, but...) */
        "mrc    p15, 0, R4, c3, c0, 0\n"
        "bic    r4, #0x80\n"
        "mcr    p15, 0, R4, c3, c0, 0\n"

        /* set access permissions (disable data access in the protected area) */
        "mrc    p15, 0, R4, c5, c0, 2\n"
        "bic    R4, #0xF0000000\n"
        "mcr    p15, 0, R4, c5, c0, 2\n"

        /* enable memory protection */
        "ldr    r0, protected_region\n"
        "mcr    p15, 0, r0, c6, c7, 0\n"

        : : : "r4"
#endif
    );

    sync_caches();

    sei(int_status);

    qprintf("[io_trace] installed.\n");
}

static const char * interrupt_name(int i)
{
    static char name[] = "INT-00h";
    int i0 = (i & 0xF);
    int i1 = (i >> 4) & 0xF;
    int i2 = (i >> 8) & 0xF;
    name[3] = i2 ? '0' + i2 : '-';
    name[4] = i1 < 10 ? '0' + i1 : 'A' + i1 - 10;
    name[5] = i0 < 10 ? '0' + i0 : 'A' + i0 - 10;
    return name;
}

uint32_t io_trace_log_get_index()
{
    return buffer_index / RECORD_SIZE;
}

uint32_t io_trace_log_get_nmax()
{
    return buffer_count / RECORD_SIZE;
}

static uint32_t ror(uint32_t word, uint32_t count)
{
    return word >> count | word << (32 - count);
}

int io_trace_log_message(uint32_t msg_index, char * msg_buffer, int msg_size)
{
    uint32_t i = msg_index * RECORD_SIZE;
    if (i >= buffer_index) return 0;

#ifdef CONFIG_DIGIC_VI
    /* we have enough metadata, no need to decode the instructions manually */
    uint32_t pc   = buffer[i];
    uint32_t dfsr = buffer[i+3];
    uint32_t dfar = buffer[i+4];
    uint32_t val  = buffer[i+5];
    uint32_t us   = buffer[i+6];
    uint32_t lr   = buffer[i+7];

    const char * task_name = (const char *) buffer[i+1];
    uint32_t interrupt = buffer[i+2];

    if (interrupt & 0x80000000)
    {
        task_name = interrupt_name(interrupt & 0xFFF);
    }

    char task_name_padded[11] = "           ";
    int spaces = 10 - strlen(task_name);
    if (spaces < 0) spaces = 0;
    snprintf(task_name_padded + spaces, 11 - spaces, "%s", task_name);

    int len = snprintf( msg_buffer, msg_size, "%d.%06d  %s:%08x:%08x:MMIO : ", us/1000000, us%1000000, task_name_padded, pc, lr);

    int is_ldr = (dfsr & 0x800) ? 0 : 1;

    len += snprintf(msg_buffer + len, msg_size - len,
        "[0x%08X] %s 0x%08X\n",
        dfar,                   /* DFAR - MMIO register address */
        is_ldr ? "->" : "<-",   /* direction (read or write), from DFSR */
        val                     /* MMIO register value */
    );
#else
    /* assuming ARM code only */
    uint32_t pc = buffer[i];
    uint32_t insn = MEM(pc);
    uint32_t Rn = buffer[i+3];
    uint32_t Rm = buffer[i+4];
    uint32_t Rd = buffer[i+5];
    char *   task_name = (char *) buffer[i+1];
    uint32_t interrupt = buffer[i+2];
    uint32_t us_timer = buffer[i+6];

    uint32_t is_ldr = insn & (1 << 20);
    uint32_t offset = 0;
    char raw[48] = "";

    if ((insn & 0x0F000000) == 0x05000000)
    {
        /* ARM ARM:
         * A5.2.2 Load and Store Word or Unsigned Byte - Immediate offset
         * A5.2.5 Load and Store Word or Unsigned Byte - Immediate pre-indexed
         */

        offset = (insn & 0xFFF);
    }
    else if ((insn & 0x0D200000) == 0x04000000)
    {
        /* A5.2.8  Load and Store Word or Unsigned Byte - Immediate post-indexed
         * A5.2.9  Load and Store Word or Unsigned Byte - Register post-indexed
         * A5.2.10 Load and Store Word or Unsigned Byte - Scaled register post-indexed
         */
        offset = 0;
    }
    else if ((insn & 0x0F000000) == 0x07000000)
    {
        /* A5.2.3 Load and Store Word or Unsigned Byte - Register offset
         * A5.2.4 Load and Store Word or Unsigned Byte - Scaled register offset
         * A5.2.6 Load and Store Word or Unsigned Byte - Register pre-indexed
         * A5.2.7 Load and Store Word or Unsigned Byte - Scaled register pre-indexed
         */
        uint32_t shift = (insn >> 5) & 0x3;
        uint32_t shift_imm = (insn >> 7) & 0x1F;

        /* index == offset */
        switch (shift)
        {
            case 0b00:  /* LSL */
                offset = Rm << shift_imm;
                qprintf("%x: %x [Rn, Rm, LSL#%d] => %x\n", pc, insn, shift_imm, offset);
                break;
            case 0b01:  /* LSR */
                offset = Rm >> (shift_imm ? shift_imm : 32);
                qprintf("%x: %x [Rn, Rm, LSR#%d] => %x\n", pc, insn, shift_imm, offset);
                break;
            case 0b10:  /* ASR */
                offset = (int32_t) Rm >> (shift_imm ? shift_imm : 32);
                qprintf("%x: %x [Rn, Rm, ASR#%d] => %x\n", pc, insn, shift_imm, offset);
                break;
            case 0b11:  /* ROR or RRX */
                offset = (shift_imm) ? ror(Rm, shift_imm) : 0 /* FIXME: C not saved; important? */;
                qprintf("%x: %x [Rn, Rm, ROR#%d] => %x\n", pc, insn, shift_imm, offset);
                ASSERT(shift_imm);
                break;
        }
    }
    else if ((insn & 0x0F600F90) == 0x01000090)
    {
        /* A5.3.3 Miscellaneous Loads and Stores - Register offset */
        offset = Rm;
    }
    else
    {
        /* unhandled case; print raw values to assist troubleshooting */
        snprintf(raw, sizeof(raw), " (%08X, Rn=%08X, Rm=%08X)", insn, Rn, Rm);
        qprintf("%x: %x ???\n", pc, insn);
    }

    /* all of the above may use the sign bit */
    if (!(insn & (1 << 23)))
    {
        offset = -offset;
    }

    char msg[128];
    snprintf(msg, sizeof(msg),
        "[0x%08X] %s 0x%08X%s",
        Rn + offset,            /* Rn - MMIO register (to be interpreted after disassembling the instruction) */
        is_ldr ? "->" : "<-",   /* assume LDR or STR; can you find a counterexample? */
        Rd,                     /* Rd - destination register (MMIO register value) */
        raw                     /* optional raw values */
    );

    /* we don't store this in the "blockchain", so we don't really need block_size */
    struct debug_msg dm = {
        .msg            = msg,
        .class_name     = "MMIO",
        .us_timer       = us_timer,
        .pc             = pc,
        .task_name      = task_name,
        .interrupt      = interrupt,
    };

    int len = debug_format_msg(&dm, msg_buffer, msg_size);

    if (buffer[buffer_count - RECORD_SIZE])
    {
        /* index wrapped around? */
        printf("[MMIO] warning: buffer full\n");
        len += snprintf(msg_buffer + len, msg_size - len,
            "[MMIO] warning: buffer full\n");
        buffer[buffer_count - RECORD_SIZE] = 0;
    }
#endif

    return len;
}

static inline void local_io_trace_pause()
{
    asm volatile (
        /* enable full access to memory */
#ifdef CONFIG_DIGIC_VI
        "MOV    R0, #0x07\n"
        "MCR    p15, 0, R0, c6, c2, 0\n"    /* write RGNR (adjust memory region #7) */
        "MOV    R0, #0x00\n"
        "MCR    p15, 0, R0, c6, c1, 2\n"    /* enable full access to memory (disable region #7) */
#else /* DIGIC V and earlier */
        "MOV     R0, #0x00\n"
        "MCR     p15, 0, R0, c6, c7, 0\n"
#endif
        ::: "r0"
    );
}

static inline void local_io_trace_resume()
{
    asm volatile (
        /* re-enable memory protection */
#ifdef CONFIG_DIGIC_VI
        "MOV    R0, #0x07\n"
        "MCR    p15, 0, R0, c6, c2, 0\n"    /* write RGNR (adjust memory region #7) */
        "LDR    R0, protected_region_size\n"
        "MCR    p15, 0, R0, c6, c1, 2\n"    /* enable region #7 */
#else /* DIGIC V and earlier */
        "LDR    R0, protected_region\n"
        "MCR    p15, 0, R0, c6, c7, 0\n"
#endif
        ::: "r0"
    );
}

/* public wrappers */
void io_trace_pause()
{
    uint32_t old = cli();
    if (TRAP_INSTALLED)
    {
        local_io_trace_pause();
    }
    sei(old);
}

void io_trace_resume()
{
    uint32_t old = cli();
    if (TRAP_INSTALLED)
    {
        local_io_trace_resume();
    }
    sei(old);
}

/* get timer value without logging it as MMIO access */
uint32_t io_trace_get_timer()
{
    uint32_t old = cli();

    if (!TRAP_INSTALLED)
    {
#ifdef CONFIG_DIGIC_VI
        uint32_t timer = MEM(0xD400000C);
#else
        uint32_t timer = MEM(0xC0242014);
#endif
        sei(old);
        return timer;
    }

    local_io_trace_pause();
#ifdef CONFIG_DIGIC_VI
    uint32_t timer = MEM(0xD400000C);
#else
    uint32_t timer = MEM(0xC0242014);
#endif
    local_io_trace_resume();

    sei(old);
    return timer;
}

void io_trace_dump()
{
    #ifdef CONFIG_80D
    /* hardcoded address, model-specific, see log_start() for details */
    char * msg_buffer = (void *) 0x2AB00000;
    int buffer_size = 32 * 1024 * 1024;
    #endif

    #ifdef CONFIG_5D4
    char * msg_buffer = (void *) 0x3C500000;
    int buffer_size = 12 * 1024 * 1024;
    #endif

    int msg_len = 0;

    uint32_t n = io_trace_log_get_index();
    msg_len += snprintf(msg_buffer + msg_len, buffer_size - msg_len, "[MMIO] Saving %d events...\n", n);

    for (uint32_t i = 0; i < n; i++)
    {
        char * msg = msg_buffer + msg_len;
        msg_len += io_trace_log_message(i, msg_buffer + msg_len, buffer_size - msg_len);
        qprintf(msg);
    }

    qprintf("Saving MMIO log %X size %X...\n", msg_buffer, msg_len);
    extern void dump_file(char* name, uint32_t addr, uint32_t size);
    dump_file("MMIO.LOG", (uint32_t) msg_buffer, msg_len);
}
