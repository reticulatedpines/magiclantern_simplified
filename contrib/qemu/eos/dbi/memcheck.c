#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "hw/boards.h"
#include "../eos.h"
#include "../model_list.h"
#include "memcheck.h"
#include "logging.h"

/* readelf magiclantern -a | grep " memcpy$" */
/* memccpy is next - useful to find out the size */
static uint32_t ml_memcpy = 0;
static uint32_t ml_memcpy_size = 0;

/* our own read-after-free, in __mem_free */
/* if wrong: lots of warning messages containing f12eeeed during ML module loading */
/* not needed if MEMCHECK_CHECK is undefined */
static const int ml_double_free_check = 0;

struct memcheck_stubs
{
    /* any malloc/free-like routines */
    uint32_t malloc[4];
    uint32_t free[4];

    /* any memcpy-like routines */
    uint32_t memcpy[4];
    uint32_t memcpy_end[4];

    /* routines for initializing a memory heap */
    uint32_t init_heap;
    uint32_t checked_heaps[4];

    /* malloc initialization follows a different pattern */
    uint32_t init_malloc;
    uint32_t malloc_struct;

    /* routine for allocating from a heap */
    uint32_t heap_alloc;
    uint32_t heap_free;

    /* allow these routines to use-after-free */
    uint32_t heap_routines[8][2];

    /* address used to store whether the guest is handling interrupts or not */
    uint32_t interrupt_active;

    /* interrupt end addresses, from main interrupt handling routine
     * (called from 0x18 -> 0x4B0 -> other handlers -> two exit paths */
    uint32_t isr_end[2];

    /* gdb breakpoint at ML's my_task_dispatch_hook and see who calls it */
    uint32_t calls_task_dispatch_hook;
};

static struct memcheck_stubs stubs;

static bool interrupt_handling;

/* cache these values so we don't have to look them up every time */
static uint32_t atcm_start, atcm_end, btcm_start, btcm_end;

static inline int is_tcm(uint64_t addr)
{
    /* atcm_start is 0 (assert on init) */
    return (addr < atcm_end) || (addr >= btcm_start && addr < btcm_end);
}

/* 2 bits for each address */
/* ull because it's shifted and used against a 64-bit value */
#define MS_NOINIT 1ull     /* not initialized */
#define MS_FREED  2ull     /* freed */
#define MS_MASK   3ull     /* mask for all of the above */
static uint64_t mem_status[0xC0000000ull * 2 / 64];

static inline uint64_t fixup_addr(uint64_t addr)
{
    if (!is_tcm(addr))
    {
        addr &= ~0x40000000;
    }
    assert(addr < 0xC0000000);
    return addr;
}

static inline bool is_uninitialized(uint64_t addr)
{
    uint64_t i = 2 * fixup_addr(addr);
    return mem_status[i/64] & (MS_NOINIT << (i & 63));
}

static inline bool is_freed(uint64_t addr)
{
    uint64_t i = 2 * fixup_addr(addr);
    return mem_status[i/64] & (MS_FREED << (i & 63));
}

static inline void set_initialized(uint64_t addr)
{
    uint64_t i = 2 * fixup_addr(addr);
    mem_status[i/64] &= ~(MS_NOINIT << (i & 63));
}

static inline void set_uninitialized(uint64_t addr)
{
    uint64_t i = 2 * fixup_addr(addr);
    mem_status[i/64] |= (MS_NOINIT << (i & 63));
}

static inline void set_freed(uint64_t addr)
{
    uint64_t i = 2 * fixup_addr(addr);
    mem_status[i/64] |= (MS_FREED << (i & 63));
}

static inline void clr_freed(uint64_t addr)
{
    uint64_t i = 2 * fixup_addr(addr);
    mem_status[i/64] &= ~(MS_FREED << (i & 63));
}

static void mem_set_status(uint64_t start, uint64_t end, uint64_t status)
{

    /* can be optimized; keep it simple for now */
    for (uint64_t addr = start; addr < end; addr++)
    {
        uint64_t i = 2 * fixup_addr(addr);
        mem_status[i/64] &= ~(MS_MASK << (i & 63));
        mem_status[i/64] |= (status << (i & 63));
    }
}

static void copy_mem_status(uint32_t src, uint32_t dst, uint32_t size)
{
    dst = fixup_addr(dst);

    if (src >= 0xF0000000)
    {
        /* reading from ROM */
        for (uint32_t i = 0; i < size; i++)
        {
            /* only change the initialization flag */
            set_initialized(dst + i);
        }
        return;
    }

    src = fixup_addr(src);

    /* can be optimized; keep it simple for now */
    for (uint32_t i = 0; i < size; i++)
    {
        /* only copy the initialization flag */
        if (is_uninitialized(src + i)) {
            set_uninitialized(dst + i);
        } else {
            set_initialized(dst + i);
        }
    }
}

/* weak checksum on memory status flags, for internal use only */
static uint64_t mem_status_checksum(uint64_t src, uint64_t size)
{
    if (src >= 0xF0000000)
    {
        return -1;
    }

    src = fixup_addr(src);

    uint64_t check = 0;

    for (uint64_t addr = src; addr < src + size; addr++)
    {
        uint64_t i = 2 * addr;
        uint64_t flags = mem_status[i/64] & (MS_MASK << (i & 63));
        check += flags;
        check += (flags) ? 0 : (addr - src);
    }

    return check;
}

static int is_heap_routine(EOSState *s, uint32_t pc)
{
    /* malloc/free are allowed to use after free */

    for (int i = 0; i < COUNT(stubs.heap_routines); i++)
    {
        assert(stubs.heap_routines[i][0] <= stubs.heap_routines[i][1]);

        if (pc >= stubs.heap_routines[i][0] &&
            pc <= stubs.heap_routines[i][1])
        {
            return 1;
        }

    }

    if (pc == ml_double_free_check)
    {
        return 1;
    }

    return 0;
}

static int is_memcpy(EOSState *s, uint32_t pc)
{
    if ((pc >= ml_memcpy && pc <= ml_memcpy + ml_memcpy_size) ||
        (pc >= stubs.memcpy[0] && pc <= stubs.memcpy_end[0])  ||
        (pc >= stubs.memcpy[1] && pc <= stubs.memcpy_end[1])  ||
        (pc >= stubs.memcpy[2] && pc <= stubs.memcpy_end[2])  ||
        (pc >= stubs.memcpy[3] && pc <= stubs.memcpy_end[3]))
    {
        return 1;
    }

    return 0;
}

static void diagnose_addr(uint32_t addr);

static void print_location_KLRED(EOSState *s, uint32_t pc, uint32_t lr)
{
    eos_callstack_indent(s);
    eos_print_location(s, pc, lr, KLRED, " ");
}

void eos_memcheck_log_mem(EOSState *s, hwaddr addr, uint64_t value, uint32_t size, int flags)
{
    int is_write = flags & 1;
    int is_read = !is_write;
    int no_check = flags & NOCHK_LOG;
    uint32_t pc = CURRENT_CPU->env.regs[15];
    uint32_t lr = CURRENT_CPU->env.regs[14];

    /* fixme: only first byte is checked */
    if (!no_check &&
        is_freed(addr) &&                   /* check for use after free */
        (is_write || !is_memcpy(s, pc)) &&  /* don't check memcpy for reads */
        !is_heap_routine(s, pc))            /* don't check malloc/free routines */
    {
        print_location_KLRED(s, pc, lr);
        fprintf(stderr, "address %x %s after free (%x)" KRESET "\n",
            (int)addr, is_write ? "written" : "read", (int)value
        );
        diagnose_addr(addr);
        eos_callstack_print(s, KLRED"Call stack: ", " ", KRESET"\n"); 
    }

    /* only interrupts and other TCM code are allowed to use the TCM */
    /* with few exceptions */
    if (is_tcm(addr) &&
        pc >= 0x1000 &&
        !(addr == stubs.interrupt_active && is_read) && /* allow reading this flag from anywhere */
        !interrupt_handling)
    {
        if (addr == 0 && is_read)
        {
            /* it might be ML's own null pointer check (in task_dispatch_hook) */
            /* let's figure it out from the stack */
            uint32_t stack[32];
            uint32_t sp = CURRENT_CPU->env.regs[13];
            cpu_physical_memory_read(sp, stack, sizeof(stack));
            for (int i = 0; i < COUNT(stack); i++)
            {
                if (stack[i] == stubs.calls_task_dispatch_hook + 4)
                {
                    /* fixme: less convoluted check? */
                    goto ignore;
                }
            }
        }
        print_location_KLRED(s, pc, lr);
        fprintf(stderr, "address %x %s TCM (%x)"KRESET"\n",
            (int)addr, is_write ? "written to" : "read from", (int)value
        );
        eos_callstack_print(s, KLRED"Call stack: ", " ", KRESET"\n"); 
ignore:;
    }

    if (is_read)
    {
        /* fixme: only first byte is checked */
        /* fixme: why do we have to exclude heap routines here? */
        if (!no_check &&
            is_uninitialized(addr) &&
            !is_freed(addr) &&
            !is_memcpy(s, pc) &&
            !is_heap_routine(s, pc))
        {
            print_location_KLRED(s, pc, lr);
            fprintf(stderr, "address %x uninitialized (read %x)"KRESET"\n",
                (int)addr, (int)value
            );
            diagnose_addr(addr);
            eos_callstack_print(s, KLRED"Call stack: ", " ", KRESET"\n"); 
        }
    }
    else /* write */
    {
        /* set each byte as initialized; don't change freed status */
        switch (size)
        {
            case 4:
                set_initialized(addr+3);
                set_initialized(addr+2);
            case 2:
                set_initialized(addr+1);
            case 1:
                set_initialized(addr);
                break;
            default:
                assert(0);
        }
    }
}

/* execution hook */

static int malloc_lr[4] = {0};
static int malloc_size[4] = {0};

struct malloc_list_item
{
    uint32_t ptr;           /* start pointer for allocated blocks */
    uint32_t ptf;           /* start pointer for freed blocks */
    uint32_t size;          /* block size */
    uint32_t caller;        /* caller address */
    int seq;                /* sequence ID (number of malloc calls before this) to compute age */
};

/* circular buffer */
struct malloc_list_item malloc_list[16384] = {{0}};
static int malloc_idx = 0;  /* index in malloc_list */
static int malloc_num = 0;  /* number of malloc calls */

static void exec_log_malloc(EOSState *s, uint32_t pc, CPUARMState *env)
{
    int lr = env->regs[14];

    if (pc == stubs.malloc[0] || pc == stubs.malloc[1] || pc == stubs.malloc[2] || pc == stubs.malloc[3])
    {
        /* allow a few multi-tasked calls */
        int id = (malloc_lr[0] == 0) ? 0 :
                 (malloc_lr[1] == 0) ? 1 :
                 (malloc_lr[2] == 0) ? 2 :
                 (malloc_lr[3] == 0) ? 3 :
                                      -1 ;
        assert(id >= 0);

        assert(malloc_lr[id] == 0);
        malloc_lr[id] = env->regs[14];
        malloc_size[id] = env->regs[0];

        if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
            eos_callstack_indent(s);
            eos_print_location(s, pc, lr, "", " ");
            fprintf(stderr, "malloc(%x)\n", malloc_size[id]);
        }
    }

    for (int id = 0; id < COUNT(malloc_lr); id++)
    {
        if (pc == malloc_lr[id])
        {
            int malloc_ptr = env->regs[0];
            if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
                eos_callstack_indent(s);
                eos_print_location(s, pc, lr, "", " ");
                fprintf(stderr, "malloc => %x\n", malloc_ptr);
            }
            mem_set_status(malloc_ptr, malloc_ptr + malloc_size[id], MS_NOINIT);
            assert(is_uninitialized(malloc_ptr));

            int malloc_idx0 = malloc_idx;
            int keep_free_blocks = 1;

            while (malloc_list[malloc_idx].ptr || 
                  (malloc_list[malloc_idx].ptf && keep_free_blocks))
            {
                malloc_idx++;

                if (malloc_idx == COUNT(malloc_list))
                {
                    malloc_idx = 0;
                }

                if (malloc_idx == malloc_idx0)
                {
                    fprintf(stderr, "Warning: discarding one free block\n");
                    keep_free_blocks = 0;
                }
            }
            assert(malloc_list[malloc_idx].ptf == 0);
            malloc_list[malloc_idx] = (struct malloc_list_item) {
                .ptr    = malloc_ptr,
                .ptf    = 0,
                .size   = malloc_size[id],
                .caller = malloc_lr[id],
                .seq    = malloc_num++,
            };
            malloc_lr[id] = 0;
        }
    }

    if (pc == stubs.free[0] || pc == stubs.free[1] || pc == stubs.free[2] || pc == stubs.free[3])
    {
        int free_ptr = env->regs[0];
        int free_size = -1;
        for (int i = 0; i < COUNT(malloc_list); i++)
        {
            /* fixme: going backwards from malloc_idx may be faster on average */
            if (malloc_list[i].ptr == free_ptr)
            {
                free_size = malloc_list[i].size;
                mem_set_status(free_ptr, free_ptr + free_size, MS_FREED | MS_NOINIT);
                assert(is_freed(free_ptr));
                malloc_list[i].ptr = 0;
                malloc_list[i].ptf = free_ptr;
                malloc_list[i].caller = env->regs[14];
            }
        }
        if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
            eos_callstack_indent(s);
            eos_print_location(s, pc, lr, "", " ");
            fprintf(stderr, "free %x size %x\n", free_ptr, free_size);
        }
    }

    if (pc == stubs.init_heap)   /* init_heap */
    {
        int start = env->regs[0];
        int size  = env->regs[1];
        eos_callstack_indent(s);
        eos_print_location(s, pc, lr, "", " ");
        fprintf(stderr, "init_heap %x %x\n", start, start+size);

        if (start == stubs.checked_heaps[0] ||
            start == stubs.checked_heaps[1] ||
            start == stubs.checked_heaps[2] ||
            start == stubs.checked_heaps[3])
        {
            fprintf(stderr, "Checking this heap.\n");
            mem_set_status(start, start+size, MS_FREED | MS_NOINIT);
        }
    }

    if (pc == stubs.init_malloc)
    {
        uint32_t start, end;
        cpu_physical_memory_read(stubs.malloc_struct, &start, 4);
        cpu_physical_memory_read(stubs.malloc_struct + 4, &end, 4);

        fprintf(stderr, "init_malloc %x %x\n", start, end);
        fprintf(stderr, "Checking this heap.\n");
        mem_set_status(start, end, MS_FREED | MS_NOINIT);
    }
}

static uint32_t memcpy_lr[4]  = {0};
static uint32_t memcpy_src[4] = {0};
static uint32_t memcpy_dst[4] = {0};
static uint32_t memcpy_num[4] = {0};
static uint64_t memcpy_chk[4] = {0};

static int memcpy_overlaps(uint32_t dst, uint32_t src, uint32_t size)
{
    uint32_t src_s = src & ~0x40000000;
    uint32_t dst_s = dst & ~0x40000000;
    uint32_t src_e = src_s + size;
    uint32_t dst_e = dst_s + size;

    return ((src_s < dst_s && src_e > dst_s) ||
            (src_s > dst_s && src_s < dst_e));
}

static void exec_log_memcpy(EOSState *s, uint32_t pc, CPUARMState *env)
{
    if (pc == stubs.memcpy[0] || pc == stubs.memcpy[1] || pc == stubs.memcpy[2] || pc == stubs.memcpy[3] ||
        pc == ml_memcpy)
    {
        /* allow a few multi-tasked calls */
        int id = (memcpy_lr[0] == 0) ? 0 :
                 (memcpy_lr[1] == 0) ? 1 :
                 (memcpy_lr[2] == 0) ? 2 :
                 (memcpy_lr[3] == 0) ? 3 :
                                      -1 ;
        assert(id >= 0);

        assert(memcpy_lr[id] == 0);
        memcpy_dst[id] = env->regs[0];
        memcpy_src[id] = env->regs[1];
        memcpy_num[id] = env->regs[2];
        memcpy_lr[id]  = env->regs[14];
        if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
            eos_callstack_indent(s);
            eos_print_location(s, pc, memcpy_lr[id], "", " ");
            fprintf(stderr,"memcpy(%x, %x, %x)\n",
                memcpy_dst[id], memcpy_src[id], memcpy_num[id]
            );
        }

        if ((int32_t)memcpy_num[id] < 0)
        {
            print_location_KLRED(s, pc, memcpy_lr[id]);
            fprintf(stderr, "negative size argument to memcpy(%x, %x, %x)?"KRESET"\n",
                memcpy_dst[id], memcpy_src[id], memcpy_num[id]
            );
        }

        if (memcpy_overlaps(memcpy_dst[id], memcpy_src[id], memcpy_num[id]))
        {
            print_location_KLRED(s, pc, memcpy_lr[id]);
            fprintf(stderr, "source and destination overlap in memcpy(%x, %x, %x)"KRESET"\n",
                memcpy_dst[id], memcpy_src[id], memcpy_num[id]
            );
        }

        /* note: memcpy writes to memory */
        /* we'll let it do whatever it does, and we will
         * overwrite the initialization flags when it returns.
         * note: it must not change the flags of src
         */
        memcpy_chk[id] = mem_status_checksum(memcpy_src[id], memcpy_num[id]);
        if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
            eos_callstack_indent(s);
            fprintf(stderr, "Flags checksum: %"PRIx64"\n", memcpy_chk[id]);
        }
    }
    for (int id = 0; id < COUNT(memcpy_lr); id++)
    {
        if (pc == memcpy_lr[id])
        {
            if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
                eos_callstack_indent(s);
                eos_print_location(s, pc, memcpy_lr[id], "", " ");
                fprintf(stderr, "memcpy(%x, %x, %x) finished.\n",
                    memcpy_dst[id], memcpy_src[id], memcpy_num[id]
                );
            }
            if (!memcpy_overlaps(memcpy_dst[id], memcpy_src[id], memcpy_num[id]))
            {
                assert(memcpy_chk[id] == mem_status_checksum(memcpy_src[id], memcpy_num[id]));
            }
            copy_mem_status(memcpy_src[id], memcpy_dst[id], memcpy_num[id]);
            memcpy_lr[id] = 0;
        }
    }
}

/* fixme: many addresses hardcoded to 500D */
void eos_memcheck_log_exec(EOSState *s, uint32_t pc, CPUARMState *env)
{
    /* for some reason, this may called multiple times on the same PC */
    static uint32_t prev_pc = 0xFFFFFFFF;
    if (prev_pc == pc) return;
    prev_pc = pc;

    /* our uninitialized stubs are 0 - don't log this address */
    if (!pc) return;

    /* enable this to look for uninitialized memory accesses
     * exploitable before jumping to main firmware */
    if (0 && pc == s->model->firmware_start)    /* aka ROMBASEADDR */
    {
        fprintf(stderr, "Marking all memory as uninitialized...\n");
        mem_set_status(0, s->model->ram_size, MS_NOINIT);
        mem_set_status(atcm_start, atcm_end, MS_NOINIT);
        mem_set_status(btcm_start, btcm_end, MS_NOINIT);
        uint32_t hack = 0x4157554B;
        cpu_physical_memory_write(0x3cc000, &hack, 4);
    }

    exec_log_malloc(s, pc, env);
    exec_log_memcpy(s, pc, env);

    if (pc == 0x18)
    {
        interrupt_handling = 1;
    }
    else if (pc == stubs.isr_end[0] || pc == stubs.isr_end[1])
    {
        interrupt_handling = 0;
    }
}

static void diagnose_addr(uint32_t addr)
{
    int found = 0;
    int printed = 0;
    int age_found = INT_MAX;

    for (int k = 0; k < COUNT(malloc_list); k++)
    {
        int i = MOD(malloc_idx - k, COUNT(malloc_list));
        if (malloc_list[i].ptr)
        {
            uint32_t start = malloc_list[i].ptr;
            uint32_t end = start + malloc_list[i].size;
            if (addr >= start && addr < end)
            {
                /* valgrind is GPL, so we can copy their messages :) */
                fprintf(stderr, 
                    KLRED"Address %x is %d bytes inside a block of size %d (%x-%x) alloc'd at %x"KRESET"\n",
                    addr, addr - start, end - start, start, end, malloc_list[i].caller
                );
                found++; printed++;
            }
        }

        if (is_freed(addr) && malloc_list[i].ptf)
        {
            uint32_t start = malloc_list[i].ptf;
            uint32_t end = start + malloc_list[i].size;
            if (addr >= start && addr < end)
            {
                /* fixme: print blocks sorted by age */
                /* initially they are found in sorted order, but that changes on longer runs */
                /* workaround: always print if a newer location is found (even if it's a little more verbose) */
                int age = malloc_num - malloc_list[i].seq;
                int show = (found == 0) || age < age_found || qemu_loglevel_mask(EOS_LOG_VERBOSE);
                age_found = MIN(age_found, age);

                if (show)
                {
                    fprintf(stderr, 
                        KLRED"Address %x is %d bytes inside a block of size %d (%x-%x) age %d free'd at %x"KRESET"\n",
                        addr, addr - start, end - start, start, end, age, malloc_list[i].caller
                    );
                    printed++;
                }
                found++;
            }
        }
    }

    if (found > printed)
    {
        fprintf(stderr, KLRED"%d older location(s) not displayed (use -d memchk,v to show them)"KRESET"\n", found - printed);
    }
}

void eos_memcheck_init(EOSState *s)
{
    atcm_start  = ATCM_ADDR;
    atcm_end    = ATCM_ADDR + ATCM_SIZE;
    btcm_start  = BTCM_ADDR;
    btcm_end    = BTCM_ADDR + BTCM_SIZE;
    assert(atcm_start == 0);

    fprintf(stderr, "Marking all memory as uninitialized...\n");
    mem_set_status(0, s->model->ram_size, MS_NOINIT);
    mem_set_status(atcm_start, atcm_end, MS_NOINIT);
    mem_set_status(btcm_start, btcm_end, MS_NOINIT);

    uint32_t ml_memccpy;
    eos_getenv_hex("QEMU_EOS_ML_MEMCPY", &ml_memcpy, 0);
    eos_getenv_hex("QEMU_EOS_ML_MEMCCPY", &ml_memccpy, 0);
    ml_memcpy_size = ml_memccpy - ml_memcpy - 4;

    if (ml_memcpy)
    {
        fprintf(stderr, "ML memcpy: %x - %x\n", ml_memcpy, ml_memcpy + ml_memcpy_size);
    }
    else
    {
        fprintf(stderr, "FIXME: ML memcpy stub unknown (set QEMU_EOS_ML_MEMCC?PY).\n");
        ml_memcpy_size = 0;
    }

    /* todo: identify stubs in a way that does not require hardcoding? */
    if (strcmp(s->model->name, "500D") == 0)
    {
        stubs = (struct memcheck_stubs) {
            .malloc         = { 0xFF018F70, 0xFF06ACB4 },
            .free           = { 0xFF019044, 0xFF06B044 },
            .memcpy         = { 0xFF3EBBC4 },
            .memcpy_end     = { 0xFF3EBC2C },
            .init_heap      =   0xFF06A4CC,
            .checked_heaps  = { 0x300000 },
            .init_malloc    =   0xFF018C04,   /* after the call */
            .malloc_struct  =   0x24D48,
            .heap_routines  = {
                                { 0xff06a4f4, 0xFF06B5CC }, /* AllocateMemory */
                                { 0xFF018F70, 0xFF019110 }, /* malloc */
                                { 0xff06c1c0, 0xFF06C1EC }, /* PackMem? */
                                { 0xff06c15c, 0xFF06C188 }, /* PackMem get size? */
                                { 0xFF06CC4C, 0xFF06CCA8 }, /* GetNextMemoryChunk?! why? */
                                { 0xff06beec, 0xFF06BF98 }, /* DeleteMemorySuite?! why? */
                              },
            .interrupt_active           = 0x664,
            .isr_end                    = { 0x660, 0x64C },
            .calls_task_dispatch_hook   = 0xff012dd0,
        };
    }
    else
    {
        fprintf(stderr, "FIXME: no memcheck stubs for %s.\n", s->model->name);
    }
}
