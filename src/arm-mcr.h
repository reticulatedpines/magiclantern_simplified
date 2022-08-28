/** \file
 * ARM control registers
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _arm_mcr_h_
#define _arm_mcr_h_

#define inline inline __attribute__((always_inline))

asm(
    ".text\n"
    ".align 4\n"
);

#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include "compiler.h"
#include "internals.h"  /* from platform directory (for CONFIG_DIGIC_VI) */

typedef void (*thunk)(void);


#if 0
typedef signed long    int32_t;
typedef unsigned long    uint32_t;
typedef signed short    int16_t;
typedef unsigned short    uint16_t;
typedef signed char    int8_t;
typedef unsigned char    uint8_t;

typedef uint32_t    size_t;
typedef int32_t        ssize_t;
#endif

static inline uint32_t
read_lr( void )
{
    uint32_t lr;
    asm __volatile__ ( "mov %0, %%lr" : "=&r"(lr) );
    return lr;
}

static inline uint32_t
read_sp( void )
{
    uint32_t sp;
    asm __volatile__ ( "mov %0, %%sp" : "=&r"(sp) );
    return sp;
}

static inline uint32_t
read_cpsr( void )
{
    uint32_t cpsr;
    asm __volatile__ ( "MRS %0, CPSR" : "=&r"(cpsr) );
    return cpsr;
}

/** Routines to enable / disable interrupts */
static inline uint32_t
cli(void)
{
    uint32_t old_irq;
    
    asm __volatile__ (
        "mrs %0, CPSR\n"
        "orr r1, %0, #0xC0\n" // set I flag to disable IRQ
        "msr CPSR_c, r1\n"
        "and %0, %0, #0xC0\n"
        : "=r"(old_irq) : : "r1"
    );
    return old_irq; // return the flag itself
}

static inline void
sei( uint32_t old_irq )
{
    asm __volatile__ (
        "mrs r1, CPSR\n"
        "bic r1, r1, #0xC0\n"
        "and %0, %0, #0xC0\n"
        "orr r1, r1, %0\n"
        "msr CPSR_c, r1" : : "r"(old_irq) : "r1" );
}

#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
/* from https://app.assembla.com/spaces/chdk/subversion/source/HEAD/trunk/lib/armutil/cache.c */

// ARMv7 cache control (based on U-BOOT cache_v7.c, utils.h, armv7.h)

/* Invalidate entire I-cache and branch predictor array */
static void __attribute__((naked,noinline)) _icache_flush_all(void)
{
    /*
     * Invalidate all instruction caches to PoU.
     * Also flushes branch target cache.
     */
    asm volatile (
        "mov    r1, #0\n"
#if defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
        "mcr    p15, 0, r1, c7, c1, 0\n"        /* Invalidate entire instruction cache Inner Shareable (Multiprocessing Extensions) */
#else
        "mcr    p15, 0, r1, c7, c5, 0\n"        /* Instruction cache invalidate all to PoU */
#endif
        "mcr    p15, 0, r1, c7, c5, 6\n"        /* Invalidate all branch predictors */
        "dsb    sy\n"
        "isb    sy\n"
        "bx     lr\n"
    );
}

/* Values for Ctype fields in CLIDR */
#define ARMV7_CLIDR_CTYPE_NO_CACHE      0
#define ARMV7_CLIDR_CTYPE_INSTRUCTION_ONLY  1
#define ARMV7_CLIDR_CTYPE_DATA_ONLY     2
#define ARMV7_CLIDR_CTYPE_INSTRUCTION_DATA  3
#define ARMV7_CLIDR_CTYPE_UNIFIED       4

#define ARMV7_DCACHE_INVAL_ALL      1
#define ARMV7_DCACHE_CLEAN_ALL    2
#define ARMV7_DCACHE_INVAL_RANGE    3
#define ARMV7_DCACHE_CLEAN_INVAL_RANGE  4

#define ARMV7_CSSELR_IND_DATA_UNIFIED   0
#define ARMV7_CSSELR_IND_INSTRUCTION    1

#define CCSIDR_LINE_SIZE_OFFSET     0
#define CCSIDR_LINE_SIZE_MASK       0x7
#define CCSIDR_ASSOCIATIVITY_OFFSET 3
#define CCSIDR_ASSOCIATIVITY_MASK   (0x3FF << 3)
#define CCSIDR_NUM_SETS_OFFSET      13
#define CCSIDR_NUM_SETS_MASK        (0x7FFF << 13)

typedef unsigned int u32;
typedef int s32;

static inline s32 log_2_n_round_up(u32 n)
{
    s32 log2n = -1;
    u32 temp = n;

    while (temp) {
        log2n++;
        temp >>= 1;
    }

    if (n & (n - 1))
        return log2n + 1; /* not power of 2 - round up */
    else
        return log2n; /* power of 2 */
}

static u32 get_clidr(void)
{
    u32 clidr;

    /* Read current CP15 Cache Level ID Register */
    asm volatile ("mrc p15,1,%0,c0,c0,1" : "=r" (clidr));
    return clidr;
}

static u32 get_ccsidr(void)
{
    u32 ccsidr;

    /* Read current CP15 Cache Size ID Register */
    asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));
    return ccsidr;
}

#ifdef CONFIG_MMU
static u32 get_ttbr0(void)
{
    u32 ttbr0;

    /* Read TTBR0 */
    asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbr0));
    return ttbr0;
}

static u32 get_ttbr1(void)
{
    u32 ttbr1;

    /* Read TTBR1 */
    asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r" (ttbr1));
    return ttbr1;
}
#endif

static void set_csselr(u32 level, u32 type)
{   u32 csselr = level << 1 | type;

    /* Write to Cache Size Selection Register(CSSELR) */
    asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r" (csselr));
}

static void v7_clean_dcache_level_setway(u32 level, u32 num_sets,
                     u32 num_ways, u32 way_shift,
                     u32 log2_line_len)
{
    int way, set, setway;

    /*
     * For optimal assembly code:
     *  a. count down
     *  b. have bigger loop inside
     */
    for (way = num_ways - 1; way >= 0 ; way--) {
        for (set = num_sets - 1; set >= 0; set--) {
            setway = (level << 1) | (set << log2_line_len) |
                 (way << way_shift);
            /* Clean data/unified cache line by set/way */
            asm volatile (" mcr p15, 0, %0, c7, c10, 2"
                    : : "r" (setway));
        }
    }
    /* DSB to make sure the operation is complete */
    asm volatile("dsb sy\n");
}

static void v7_maint_dcache_level_setway(u32 level, u32 operation)
{
    u32 ccsidr;
    u32 num_sets, num_ways, log2_line_len, log2_num_ways;
    u32 way_shift;

    set_csselr(level, ARMV7_CSSELR_IND_DATA_UNIFIED);

    ccsidr = get_ccsidr();

    log2_line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >>
                CCSIDR_LINE_SIZE_OFFSET) + 2;
    /* Converting from words to bytes */
    log2_line_len += 2;

    num_ways  = ((ccsidr & CCSIDR_ASSOCIATIVITY_MASK) >>
            CCSIDR_ASSOCIATIVITY_OFFSET) + 1;
    num_sets  = ((ccsidr & CCSIDR_NUM_SETS_MASK) >>
            CCSIDR_NUM_SETS_OFFSET) + 1;
    /*
     * According to ARMv7 ARM number of sets and number of ways need
     * not be a power of 2
     */
    log2_num_ways = log_2_n_round_up(num_ways);

    way_shift = (32 - log2_num_ways);

    if (operation == ARMV7_DCACHE_CLEAN_ALL)
        v7_clean_dcache_level_setway(level, num_sets, num_ways,
                      way_shift, log2_line_len);
}

static void v7_maint_dcache_all(u32 operation)
{
    u32 level, cache_type, level_start_bit = 0;

    u32 clidr = get_clidr();

    for (level = 0; level < 7; level++) {
        cache_type = (clidr >> level_start_bit) & 0x7;
        if ((cache_type == ARMV7_CLIDR_CTYPE_DATA_ONLY) ||
            (cache_type == ARMV7_CLIDR_CTYPE_INSTRUCTION_DATA) ||
            (cache_type == ARMV7_CLIDR_CTYPE_UNIFIED))
            v7_maint_dcache_level_setway(level, operation);
        level_start_bit += 3;
    }
}

static void _dcache_clean_all(void) {
    asm volatile("dsb sy\n");
    v7_maint_dcache_all(ARMV7_DCACHE_CLEAN_ALL);
    /* anything else? */

    #if defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
    /* guess: tell the other CPU to do the same? (see B2.2.5 in ARM ARM v7) */
    asm volatile("dsb sy\n");
    *(volatile uint32_t *)0xC1100730 = 0;
    asm volatile("dsb sy\n");
    #endif
}

static inline void _sync_caches()
{
    /* Self-modifying code (from uncacheable memory) */
    /* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka14041.html */
    /* http://infocenter.arm.com/help/topic/com.arm.doc.ihi0053b/IHI0053B_arm_c_language_extensions_2013.pdf */
    /* https://app.assembla.com/spaces/chdk/subversion/source/HEAD/trunk/lib/armutil/cache.c */
    /* http://www.magiclantern.fm/forum/index.php?topic=17360.msg191399#msg191399 */
    uint32_t old = cli();
    _dcache_clean_all(); /* Clean the cache so that the new stuff is written out to memory */
    _icache_flush_all(); /* Invalidate the instruction cache and branch predictor */
    sei(old);
}

#else  /* DIGIC 2...5 */
/*
     naming conventions
    --------------------
    
    FLUSH instruction cache:
        just mark all entries in the I cache as invalid. no other effect than slowing down code execution until cache is populated again.
    
    DRAIN write buffer:
        halt CPU execution until every RAM write operation has finished.
    
    FLUSH data cache / cache entry:
        invalidating the cache content *without* writing back the dirty data into memory. (dangerous!)
        this will simply mark any data in cache (or in the line) as invalid, no matter if it was written back into RAM.
        e.g. 
            mcr p15, 0, Rd, c7, c6, 0  # whole D cache
            mcr p15, 0, Rd, c7, c6, 1  # single line in D cache
    
    CLEAN data cache entry:
        cleaning is the process of checking a single cahe entry and writing it back into RAM if it was not written yet.
        this will ensure that the contents of the data cache are also in RAM. only possible for a single line, so we have to loop though all lines.
        e.g.
            mcr p15, 0, Rd, c7, c10, 1 # Clean data cache entry (by address)
            mcr p15, 0, Rd, c7, c14, 1 # Clean data cache entry (by index/segment)
            
    CLEAN and FLUSH data cache entry:
        the logical consequence - write it back into RAM and mark cache entry as invalid. As if it never was in cache.
        e.g.
            mcr p15, 0, Rd, c7, c10, 2 # Clean and flush data cache entry (by address)
            mcr p15, 0, Rd, c7, c14, 2 # Clean and flush data cache entry (by index/segment)

*/

/* do you really want to call that? */
static inline void _flush_caches()
{
    uint32_t reg = 0;
    asm(
        "mov %0, #0\n"
        "mcr p15, 0, %0, c7, c5, 0\n" // entire I cache
        "mov %0, #0\n"
        "mcr p15, 0, %0, c7, c6, 0\n" // entire D cache
        "mcr p15, 0, %0, c7, c10, 4\n" // drain write buffer
        : : "r"(reg)
    );
}

/* write back all data into RAM and mark as invalid in data cache */
static inline void _clean_d_cache()
{
    /* assume 8KB data cache */
    /* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0092b/ch04s03s04.html */
    uint32_t segment = 0;
    do {
        uint32_t line = 0;
        for( ; line != 0x800 ; line += 0x20 )
        {
            asm(
                "mcr p15, 0, %0, c7, c14, 2"
                : : "r"( line | segment )
            );
        }
    } while( segment += 0x40000000 );
    
    /* ensure everything is written into RAM, halts CPU execution until pending operations are done */
    uint32_t reg = 0;
    asm(
        "mcr p15, 0, %0, c7, c10, 4\n" // drain write buffer
        : : "r"(reg)
    );
}

/* mark all entries in I cache as invalid */
static inline void _flush_i_cache()
{
    asm(
        "mov r0, #0\n"
        "mcr p15, 0, r0, c7, c5, 0\n" // flush I cache
        : : : "r0"
    );
}

/* ensure data is written into RAM and the instruction cache is empty so everything will get fetched again */
static inline void _sync_caches()
{
    uint32_t old = cli();
    _clean_d_cache();
    _flush_i_cache();
    sei(old);
}

#endif

/* in patch.c; this also reapplies cache patches, if needed */
extern void sync_caches();

#if 0
// This must be a macro
#define setup_memory_region( region, value ) \
    asm __volatile__ ( "mcr p15, 0, %0, c6, c" #region "\n" : : "r"(value) )

#define set_d_cache_regions( value ) \
    asm __volatile__ ( "mcr p15, 0, %0, c2, c0\n" : : "r"(value) )

#define set_i_cache_regions( value ) \
    asm __volatile__ ( "mcr p15, 0, %0, c2, c0, 1\n" : : "r"(value) )

#define set_d_buffer_regions( value ) \
    asm __volatile__ ( "mcr p15, 0, %0, c3, c0\n" : : "r"(value) )

#define set_d_rw_regions( value ) \
    asm __volatile__ ( "mcr p15, 0, %0, c5, c0, 0\n" : : "r"(value) )

#define set_i_rw_regions( value ) \
    asm __volatile__ ( "mcr p15, 0, %0, c5, c0, 1\n" : : "r"(value) )

static inline void
set_control_reg( uint32_t value )
{
    asm __volatile__ ( "mcr p15, 0, %0, c3, c0\n" : : "r"(value) );
}

static inline uint32_t
read_control_reg( void )
{
    uint32_t value;
    asm __volatile__ ( "mrc p15, 0, %0, c3, c0\n" : "=r"(value) );
    return value;
}


static inline void
set_d_tcm( uint32_t value )
{
    asm( "mcr p15, 0, %0, c9, c1, 0\n" : : "r"(value) );
}

static inline void
set_i_tcm( uint32_t value )
{
    asm( "mcr p15, 0, %0, c9, c1, 1\n" : : "r"(value) );
}

static inline void
select_normal_vectors( void )
{
    uint32_t reg;
    asm(
        "mrc p15, 0, %0, c1, c0\n"
        "bic %0, %0, #0x2000\n"
        "mcr p15, 0, %0, c1, c0\n"
        : "=r"(reg)
    );
}
#endif

/**
 * Some common instructions.
 * Thanks to ARMada by g3gg0 for some of the black magic :)
 */
#define RET_INSTR    0xe12fff1e    // bx lr
#define FAR_CALL_INSTR    0xe51ff004    // ldr pc, [pc,#-4]
#define LOOP_INSTR    0xeafffffe    // 1: b 1b
#define NOP_INSTR    0xe1a00000    // mov r0, r0
#define MOV_R0_0_INSTR 0xe3a00000
#define MOV_R0_0x450000_INSTR 0xE3A00845
#define MOV_R1_0xC80000_INSTR 0xE3A01732
#define MOV_R1_0xC60000_INSTR 0xE3A018C6
#define MOV_R1_0xC70000_INSTR 0xE3A018C7
#define MOV_R0_0x4E0000_INSTR 0xE3A0084E

#define MOV_RD_IMM_INSTR(rd,imm)\
    ( 0xE3A00000 \
    | (rd << 15) \
    )

#define BL_INSTR(pc,dest) \
    ( 0xEB000000 \
    | ((( ((uint32_t)dest) - ((uint32_t)pc) - 8 ) >> 2) & 0x00FFFFFF) \
    )

#define B_INSTR(pc,dest) \
    ( 0xEA000000 \
    | ((( ((uint32_t)dest) - ((uint32_t)pc) - 8 ) >> 2) & 0x00FFFFFF) \
    )

#define ROR(val,count)   (ROR32(val,(count)%32))
#define ROR32(val,count) (((val) >> (count))|((val) << (32-(count))))

/** Simple boot loader memcpy.
 *
 * \note This is not general purpose; len must be > 0 and must be % 4
 */
static inline void
blob_memcpy(
    void *        dest_v,
    const void *    src_v,
    const void *    end
)
{
    uint32_t *    dest = dest_v;
    const uint32_t * src = src_v;
    const uint32_t len = ((const uint32_t*) end) - src;
    uint32_t i;

    for( i=0 ; i<len ; i++ )
        dest[i] = src[i];
}

/* get ID of current CPU core
 * for single-core models, return 0 */
static inline uint32_t get_cpu_id( void )
{
#if defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
    /* Dual core Cortex A9 */
    /* Extract CPU ID bits from the MPIDR register */
    /* http://infocenter.arm.com/help/topic/com.arm.doc.ddi0388e/CIHEBGFG.html */
    uint32_t cpu_id;
    asm __volatile__ (
        "MRC p15, 0, %0, c0, c0, 5\n"   /* read MPIDR register */
        "AND %0, %0, #3\n"              /* Cortex A9: up to 4 CPU cores */
        : "=&r"(cpu_id));
    return cpu_id;
#else
    /* assuming single-core */
    return 0;
#endif
}

#if defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)

/* Canon stub; used in AllocateMemory/FreeMemory and others */
/* it clears the interrupts, does some LDREX/STREX and returns CPSR */
extern uint32_t cli_spin_lock(volatile uint32_t * lock);

/* unlocking is inlined */
static inline void sei_spin_unlock(volatile uint32_t * lock, uint32_t old_status)
{
    asm volatile ( "DMB" );
    *lock = 0;
    asm volatile ( "DSB" );
    asm volatile ( "MSR CPSR_c, %0" :: "r"(old_status) );
}
#endif

#endif
