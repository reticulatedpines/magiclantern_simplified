#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"
#include "elf.h"
#include "hw/boards.h"
#include "hw/loader.h"

#include "../eos.h"
#include "../model_list.h"
#include "logging.h"
#include "memcheck.h"
#include "backtrace.h"

#define CALLSTACK_RIGHT_ALIGN 80

static uint32_t DebugMsg_addr = 0;

static const char * eos_lookup_symbol(uint32_t pc)
{
    if (pc < 0x100)
    {
        /* very low address? don't bother */
        return "";
    }

    /* known function(s)? */
    if ((pc & ~1) == (DebugMsg_addr & ~1))
    {
        return "DebugMsg";
    }

    /* looks like most (all?) symbols are loaded with Thumb bit cleared */
    const char * name = lookup_symbol(pc & ~1);

    if (!(name && name[0]))
    {
        /* not found? try to look it up with Thumb bit set */
        name = lookup_symbol(pc | 1);
    }

    return name;
}

/* whether the addresses from this memory region should be analyzed (by any logging tools) */
static inline int should_log_memory_region(MemoryRegion * mr, int is_write)
{
    int is_read = !is_write;

    if (mr->ram && qemu_loglevel_mask(EOS_LOG_RAM)) {
        if ((is_read  && qemu_loglevel_mask(EOS_LOG_RAM_R)) ||
            (is_write && qemu_loglevel_mask(EOS_LOG_RAM_W))) {
            return 1;
        }
    }

    if (mr->rom_device && qemu_loglevel_mask(EOS_LOG_ROM)) {
        if ((is_read  && qemu_loglevel_mask(EOS_LOG_ROM_R)) ||
            (is_write && qemu_loglevel_mask(EOS_LOG_ROM_W))) {
            return 1;
        }
    }

    if (mr->name == NULL)
    {
        /* unmapped? */
        assert(!mr->ram);
        assert(!mr->rom_device);
        return 1;
    }

    return 0;
}

/* whether the addresses from this memory region should be printed in logs */
/* same as above, but with the EOS_PR[int] modifier */
/* fixme: duplicate code */
static inline int should_log_memory_region_verbosely(MemoryRegion * mr, int is_write)
{
    int is_read = !is_write;

    if (mr->ram && qemu_loglevel_mask(EOS_PR(EOS_LOG_RAM))) {
        if ((is_read  && qemu_loglevel_mask(EOS_PR(EOS_LOG_RAM_R))) ||
            (is_write && qemu_loglevel_mask(EOS_PR(EOS_LOG_RAM_W)))) {
            return 1;
        }
    }

    if (mr->rom_device && qemu_loglevel_mask(EOS_PR(EOS_LOG_ROM))) {
        if ((is_read  && qemu_loglevel_mask(EOS_PR(EOS_LOG_ROM_R))) ||
            (is_write && qemu_loglevel_mask(EOS_PR(EOS_LOG_ROM_W)))) {
            return 1;
        }
    }

    if (mr->name == NULL)
    {
        /* unmapped? */
        assert(!mr->ram);
        assert(!mr->rom_device);
        return 1;
    }

    return 0;
}

static void eos_log_selftest(EOSState *s, hwaddr addr, uint64_t value, uint32_t size, int flags)
{
    int is_write = flags & 1;
    int no_check = flags & NOCHK_LOG;

    /* check reads - they must match the memory contents */
    if (!no_check && !is_write)
    {
        uint64_t check;
        uint64_t mask = (1 << (size*8)) - 1;
        assert(size <= 4);
        cpu_physical_memory_read(addr, &check, size);
        if ((check & mask) != (value & mask))
        {
            fprintf(stderr, "FIXME: %x: %x vs %x (R%d)\n", (int)addr, (int)value, (int)check, size);
        }
    }

    if (is_write)
    {
        /* rebuild a second copy of the RAM */
        static uint32_t * buf = 0; if (!buf) buf = calloc(1, s->model->ram_size);
        static uint32_t * ram = 0; if (!ram) ram = calloc(1, s->model->ram_size);

        /* ckeck both copies every now and then to make sure they are identical (slow) */
        static int k = 0; k++;
        if ((k % 0x100000 == 0) && (!no_check))
        {
            cpu_physical_memory_read(0, ram, s->model->ram_size);
            for (int i = 0; i < s->model->ram_size/4; i++)
            {
                if (buf[i] != ram[i])
                {
                    fprintf(stderr, "FIXME: %x: %x vs %x (W%d)\n", i*4, (int)buf[i], (int)ram[i], size);
                    buf[i] = ram[i];
                }
            }
        }
        
        uint32_t address = addr;
        if (address >= 0x40001000)
        {
            address &= ~0x40000000;
        }
        if (address < 0x40000000)
        {
            switch (size)
            {
                case 1:
                    ((uint8_t *)buf)[address] = value;
                    break;
                case 2:
                    ((uint16_t *)buf)[address/2] = value;
                    break;
                case 4:
                    ((uint32_t *)buf)[address/4] = value;
                    break;
                default:
                    assert(0);
            }
        }
    }
}

static void eos_callstack_log_mem(EOSState *s, hwaddr _addr, uint64_t _value, uint32_t size, int flags);
static void eos_romcpy_log_mem(EOSState *s, MemoryRegion *mr, hwaddr _addr, uint64_t _value, uint32_t size, int flags);

void eos_log_mem(void * opaque, hwaddr addr, uint64_t value, uint32_t size, int flags)
{
    const char * msg = "";
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    int is_write = flags & 1;
    int mode = is_write ? MODE_WRITE : MODE_READ;

    /* find out what kind of memory is this */
    /* fixme: can be slow */
    hwaddr l = 4;
    hwaddr addr1;
    MemoryRegion * mr = address_space_translate(&address_space_memory, addr, &addr1, &l, is_write);

    if (!should_log_memory_region(mr, is_write))
    {
        return;
    }

    EOSState* s = (EOSState*) opaque;

    if (qemu_loglevel_mask(EOS_LOG_RAM_DBG))
    {
        /* perform a self-test to make sure the loggers capture
         * all memory write events correctly (not sure how to check reads)
         */
        eos_log_selftest(s, addr, value, size, flags);
    }

    if (qemu_loglevel_mask(EOS_LOG_CALLSTACK))
    {
        eos_callstack_log_mem(s, addr, value, size, flags);
    }

    if (qemu_loglevel_mask(EOS_LOG_CALLS))
    {
        /* calls implies callstack; nothing to do here */
        /* will just print the function calls as they happen */
    }

    if (qemu_loglevel_mask(EOS_LOG_ROMCPY))
    {
        eos_romcpy_log_mem(s, mr, addr, value, size, flags);
    }

    if (qemu_loglevel_mask(EOS_LOG_RAM_MEMCHK))
    {
        /* in memcheck.c */
        eos_memcheck_log_mem(s, addr, value, size, flags);
    }

    if (!should_log_memory_region_verbosely(mr, is_write))
    {
        /* only print the items specifically asked by the user */
        return;
    }

    /* we are going log memory accesses in the same way as I/O */
    /* even if -d io was not specified */
    mode |= FORCE_LOG;

    switch (size)
    {
        case 1:
            msg = "8-bit";
            value &= 0xFF;
            break;
        case 2:
            msg = "16-bit";
            value &= 0xFFFFF;
            break;
        case 4:
            value &= 0xFFFFFFFF;
            break;
        default:
            assert(0);
    }

    if (is_write)
    {
        /* log the old value as well */
        msg_arg2 = (intptr_t) msg;
        msg_arg1 = 0;
        msg = "was 0x%X; %s";
        cpu_physical_memory_read(addr, &msg_arg1, size);
    }

    /* all our memory region names start with eos. */
    assert(!mr->name || strncmp(mr->name, "eos.", 4) == 0);
    const char * name = (mr->name) ? mr->name + 4 : KLRED"UNMAPPED"KRESET;
    io_log(name, s, addr, mode, value, value, msg, msg_arg1, msg_arg2);
}



/* ----------------------------------------------------------------------------- */


static char idc_path[100];
static FILE * idc = NULL;

/* QEMU is usually closed with CTRL-C, so call this when finished */
static void close_idc(void)
{
    fprintf(idc, "}\n");
    fclose(idc); idc = 0;
    fprintf(stderr, "%s saved.\n", idc_path);
}

static void eos_idc_log_call(EOSState *s, CPUState *cpu, CPUARMState *env,
    TranslationBlock *tb, uint32_t prev_pc, uint32_t prev_lr, uint32_t prev_size)
{
    static int stderr_dup = 0;

    if (!idc)
    {
        snprintf(idc_path, sizeof(idc_path), "%s/calls.idc", s->model->name);
        fprintf(stderr, "Exporting called functions to %s.\n", idc_path);
        idc = fopen(idc_path, "w");
        assert(idc);

        atexit(close_idc);

        fprintf(idc, "/* List of functions called during execution. */\n");
        fprintf(idc, "/* Generated from QEMU. */\n\n");
        fprintf(idc, "#include <idc.idc>\n\n");
        fprintf(idc, "static main() {\n");

        stderr_dup = dup(fileno(stderr));
    }

    /* bit array for every possible PC & ~3 */
    static uint32_t saved_pcs[(1 << 30) / 32] = {0};

    uint32_t pc = env->regs[15];
    uint32_t lr = env->regs[14];
    uint32_t sp = env->regs[13];

    /* log each called function to IDC, only once */
    int pca = pc >> 2;
    if (!(saved_pcs[pca/32] & (1 << (pca%32))))
    {
        saved_pcs[pca/32] |= (1 << pca%32);
        
        /* log_target_disas writes to stderr; redirect it to our output file */
        /* todo: any other threads that might output to stderr? */
        assert(stderr_dup);
        fflush(stderr); fflush(idc);
        dup2(fileno(idc), fileno(stderr));
        fprintf(stderr, "  /* from "); log_target_disas(cpu, prev_pc, prev_size, 0);
        fprintf(stderr, "   *   -> "); log_target_disas(cpu, tb->pc, tb->size, 0);
        char * task_name = eos_get_current_task_name(s);
        fprintf(stderr, "   * %s%sPC:%x->%x LR:%x->%x SP:%x */\n",
            task_name ? task_name : "", task_name ? " " : "",
            prev_pc, pc, prev_lr, lr, sp
        );
        fprintf(stderr, "  SetReg(0x%X, \"T\", %d);\n", pc, env->thumb);
        fprintf(stderr, "  MakeCode(0x%X);\n", pc);
        fprintf(stderr, "  MakeFunction(0x%X, BADADDR);\n", pc);
        fprintf(stderr, "\n");
        dup2(stderr_dup, fileno(stderr));
    }
}

/* call stack reconstruction */
/* fixme: adapt panda's callstack_instr rather than reinventing the wheel */

/* todo: move it in EOSState? */
static uint32_t interrupt_level = 0;

static uint8_t get_stackid(EOSState *s)
{
    if (interrupt_level)
    {
        return 0xFE;
    }

    return eos_get_current_task_id(s) & 0x7F;
}

struct call_stack_entry
{
    union
    {
        struct
        {
            uint32_t R0;  uint32_t R1; uint32_t R2;  uint32_t R3;
            uint32_t R4;  uint32_t R5; uint32_t R6;  uint32_t R7;
            uint32_t R8;  uint32_t R9; uint32_t R10; uint32_t R11;
            uint32_t R12; uint32_t sp; uint32_t lr;  uint32_t pc;
        };
        uint32_t regs[16];
    };
    uint32_t last_pc;
    uint32_t num_args;
    uint32_t is_tail_call;      /* boolean: whether it's a tail function call */
    uint32_t interrupt_id;      /* 0 = regular code, nonzero = interrupt */
    uint32_t direct_jumps[4];   /* DIGIC 6 code uses many of those */
} __attribute__((packed));

static struct call_stack_entry call_stacks[256][256];
static int call_stack_num[256] = {0};

static inline void call_stack_push(uint8_t id, uint32_t * regs,
    uint32_t pc, uint32_t last_pc, uint32_t is_tail_call, uint32_t interrupt_id)
{
    assert(call_stack_num[id] < COUNT(call_stacks[0]));

    struct call_stack_entry * entry = &call_stacks[id][call_stack_num[id]];
    memcpy(entry->regs, regs, sizeof(entry->regs));
    entry->num_args = 4;        /* fixme */
    entry->pc = pc;             /* override pc (actually, use the one that includes the Thumb flag) */
    entry->last_pc = last_pc;   /* on Thumb, LR is not enough to find this because of variable instruction size */
    entry->is_tail_call = is_tail_call;
    entry->interrupt_id = interrupt_id;
    memset(entry->direct_jumps, 0, sizeof(entry->direct_jumps));
    call_stack_num[id]++;
}

#if 0
static uint32_t call_stack_pop(uint8_t id)
{
    assert(call_stack_num[id] > 0);
    return call_stacks[id][--call_stack_num[id]].lr;
}
#endif

static uint32_t callstack_frame_size(uint8_t id, unsigned level)
{
    if (level == 0)
    {
        /* unknown */
        return 0;
    }

    uint32_t sp = call_stacks[id][level].sp;
    uint32_t next_sp = call_stacks[id][level-1].sp;
    if (sp <= next_sp && next_sp - sp < 0x10000)
    {
        /* stack decreased => easy */
        return next_sp - sp;
    }

    /* stack increased => unknown */
    return 0;
}

uint32_t eos_callstack_get_caller_param(EOSState *s, int call_depth, enum param_type param_type)
{
    uint8_t id = get_stackid(s);

    if (param_type == CALL_DEPTH)
    {
        return call_stack_num[id];
    }

    int level = call_stack_num[id] - call_depth - 1;
    assert(level >= 0);

    switch (param_type)
    {
        case CALLER_STACKFRAME_SIZE:
            return callstack_frame_size(id, level);

        case CALLER_PC:
            return call_stacks[id][level].pc;

        case CALL_LOCATION:
            return call_stacks[id][level].last_pc;

        case CALLER_LR:
            return call_stacks[id][level].lr;

        case CALLER_SP:
            return call_stacks[id][level].sp;

        case CALLER_NUM_ARGS:
            return call_stacks[id][level].num_args;

        default:
            break;
    }

    /* default: positive value = function argument index */
    int arg_index = param_type;

    if (arg_index < 4)
    {
        /* first 4 args are in registers */
        return call_stacks[id][level].regs[arg_index];
    }
    else
    {
        /* all others are on the stack */
        /* assume they are in the first caller's stack frame */
        uint32_t frame_size = callstack_frame_size(id, level);
        assert((arg_index - 4) < frame_size / 4);
        uint32_t arg_addr = call_stacks[id][level].sp + (arg_index - 4) * 4;
        uint32_t arg = 0;
        cpu_physical_memory_read(arg_addr, &arg, 4);
        return arg;
    }
}

int eos_indent(int initial_len, int target_indent)
{
    char buf[256];
    int len = target_indent - initial_len;
    if (len < 0)
    {
        fprintf(stderr, "\n");
        len = target_indent;
    }
    assert(len < sizeof(buf));
    memset(buf, ' ', len);
    buf[len] = 0;
    fprintf(stderr, "%s", buf);
    return len;
}

static int call_stack_indent(uint8_t id, int initial_len, int max_initial_len)
{
    int len = initial_len;
    len += eos_indent(initial_len, max_initial_len + call_stack_num[id]);
    return len;
}

int eos_callstack_indent(EOSState *s)
{
    if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
        return call_stack_indent(get_stackid(s), 0, 0);
    } else {
        return 0;
    }
}

int eos_callstack_get_indent(EOSState *s)
{
    if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
        return call_stack_num[get_stackid(s)];
    } else {
        return 0;
    }
}

int eos_callstack_print(EOSState *s, const char * prefix, const char * sep, const char * suffix)
{
    uint8_t id = get_stackid(s);

    if (!call_stack_num[id])
    {
        /* empty? */
        return 0;
    }

    int len = fprintf(stderr, "%s", prefix);
    uint32_t pc = CURRENT_CPU->env.regs[15];
    uint32_t sp = CURRENT_CPU->env.regs[13];

    uint32_t stack_top, stack_bot;
    if (eos_get_current_task_stack(s, &stack_top, &stack_bot))
    {
        len += fprintf(stderr, "[%x-%x] ", stack_top, stack_bot);
    }

    len += fprintf(stderr, "(%x:%x)%s", pc, sp, sep);
    for (int k = call_stack_num[id]-1; k >= 0; k--)
    {
        uint32_t lr = call_stacks[id][k].lr;
        uint32_t sp = call_stacks[id][k].sp;
        uint32_t ii = call_stacks[id][k].interrupt_id;
        len += fprintf(stderr, "%s%x:%x%s", ii ? "i:" : "", lr, sp, sep);
    }
    len += fprintf(stderr, "%s", suffix);
    return len;
}

static int print_args(uint32_t * regs);

void eos_callstack_print_verbose(EOSState *s)
{
    uint8_t id = get_stackid(s);

    if (!qemu_loglevel_mask(EOS_LOG_CALLSTACK)) {
        /* no callstack instrumentation enabled
         * try to figure it out from the stack */
        eos_backtrace_rebuild(s, 0, 0);
        return;
    }

    if (!call_stack_num[id])
    {
        /* empty? */
        return;
    }

    uint32_t pc = CURRENT_CPU->env.regs[15];
    uint32_t lr = CURRENT_CPU->env.regs[14];
    uint32_t sp = CURRENT_CPU->env.regs[13];

    uint32_t stack_top, stack_bot;
    if (eos_get_current_task_stack(s, &stack_top, &stack_bot))
    {
        int len = fprintf(stderr, "Current stack: [%x-%x] sp=%x", stack_top, stack_bot, sp);
        len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
        len += eos_print_location(s, pc, lr, " at ", "\n");
    }

    for (int k = 0; k < call_stack_num[id]; k++)
    {
        struct call_stack_entry * entry = &call_stacks[id][k];
        uint32_t pc = entry->pc;
        uint32_t sp = entry->sp;
        uint32_t ret = entry->last_pc;

        int len = eos_indent(0, k);
        
        if (entry->interrupt_id)
        {
            len += fprintf(stderr, "interrupt %02Xh", entry->interrupt_id);
            len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
            eos_print_location(s, ret, sp, " at ", "\n");
            uint32_t pc0 = pc & ~1;
            uint32_t vbar = CURRENT_CPU->env.cp15.vbar_s;
            assert(pc0 == vbar + 0x18);
        }
        else
        {
            /* print direct jumps, if any */
            /* use any of their names for the entire group */
            const char * name = eos_lookup_symbol(pc);
            for (int i = COUNT(entry->direct_jumps)-1; i >= 0; i--) {
                if (entry->direct_jumps[i]) {
                    const char * jump_name = eos_lookup_symbol(entry->direct_jumps[i]);
                    if (jump_name && jump_name[0]) {
                        name = jump_name;
                    }
                    len += fprintf(stderr, "0x%X -> ", entry->direct_jumps[i]);
                }
            }
            len += fprintf(stderr, "0x%X", pc);

            if (name && name[0]) {
                len += fprintf(stderr, " %s", name);
            }
            len += print_args(entry->regs);
            len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
            eos_print_location(s, ret, sp, " at ", " (pc:sp)\n");
        }
    }
}

int eos_print_location(EOSState *s, uint32_t pc, uint32_t lr, const char * prefix, const char * suffix)
{
    char name_suffix[512];

    const char * name = eos_lookup_symbol(pc);
    if (name && name[0]) {
        snprintf(name_suffix, sizeof(name_suffix), " (%s)%s", name, suffix);
        suffix = name_suffix;
    }

    if (interrupt_level) {
        return fprintf(stderr, "%s[INT-%02X:%x:%x]%s", prefix, s->irq_id, pc, lr, suffix);
    } else {
        char * task_name = eos_get_current_task_name(s);
        if (task_name) {
            return fprintf(stderr, "%s[%s:%x:%x]%s", prefix, task_name, pc, lr, suffix);
        } else {
            return fprintf(stderr, "%s[%x:%x]%s", prefix, pc, lr, suffix);
        }
    }
}

int eos_print_location_gdb(EOSState *s)
{
    uint32_t ret = CURRENT_CPU->env.regs[14] - 4;

    if (qemu_loglevel_mask(EOS_LOG_CALLSTACK)) {
        uint8_t id = get_stackid(s);
        int level = call_stack_num[id] - 1;
        uint32_t stack_lr = level >= 0 ? call_stacks[id][level].lr : 0;
        ret = stack_lr - 4;
    }

    /* on multicore machines, print CPU index for each message */
    if (CPU_NEXT(first_cpu)) {
        fprintf(stderr, "[CPU%d] ", current_cpu->cpu_index);
    }

    if (interrupt_level) {
        return fprintf(stderr, "[%s     INT-%02Xh:%08x %s] ", KCYN, s->irq_id, ret, KRESET);
    } else {
        const char * task_name = eos_get_current_task_name(s);
        if (!task_name) task_name = "";
        return fprintf(stderr, "[%s%12s:%08x %s] ", KCYN, task_name, ret, KRESET);
    }
}

static int print_call_location(EOSState *s, uint32_t pc, uint32_t lr)
{
    return eos_print_location(s, pc, lr, " at ", "\n");
}

/* check whether a guest memory address range can be safely dereferenced */
static int check_address_range(uint32_t ptr, uint32_t len)
{
    hwaddr l = len;
    hwaddr addr1;
    MemoryRegion * mr = address_space_translate(&address_space_memory, ptr, &addr1, &l, 0);

    if (!mr)
    {
        return 0;
    }

    if (l != len)
    {
        return 0;
    }

    return mr->ram || mr->rom_device;
}

/* from dm-spy-experiments */
static int is_sane_ptr(uint32_t ptr, uint32_t len)
{
    if (ptr < 0x1000)
    {
        return 0;
    }

    if ((ptr & 0xF0000000) != ((ptr + len) & 0xF0000000))
    {
        return 0;
    }

    switch (ptr & 0xF0000000)
    {
        case 0x00000000:
        case 0x10000000:
        case 0x40000000:
        case 0x50000000:
        case 0xE0000000:
        case 0xF0000000:
            /* looks fine at first sight; now perform a thorough check */
            return check_address_range(ptr, len);
    }

    return 0;
}

static const char * looks_like_string(uint32_t addr)
{
    if (!is_sane_ptr(addr, 64))
    {
        return 0;
    }

    static char buf[64];
    const int min_len = 4;
    const int max_len = sizeof(buf);
    cpu_physical_memory_read(addr, buf, sizeof(buf));
    buf[sizeof(buf)-1] = 0;

    for (char * p = buf; p < buf + max_len; p++)
    {
        unsigned char c = *p;
        if (c == 0 && p > buf + min_len)
        {
            if (p + 1 == buf + max_len)
            {
                *(p-1) = *(p-2) = *(p-3) = '.';
            }
            return buf;
        }
        if ((c < 32 || c > 127) && c != 7 && c != 10 && c != 13)
        {
            return 0;
        }
    }
    return 0;
}

static const char * string_escape(const char * str)
{
    static char buf[128];
    char * out = buf;
    for (const char * c = str; *c && out + 2 < buf + sizeof(buf); c++)
    {
        switch ((unsigned char)(*c))
        {
            case 10:
                *out = '\\'; out++;
                *out = 'n'; out++;
                break;
            case 13:
                *out = '\\'; out++;
                *out = 'r'; out++;
                break;
            case 7:
                *out = '\\'; out++;
                *out = 'a'; out++;
                break;
            case 32 ... 127:
                *out = *c; out++;
                break;
            default:
                *out = '?'; out++;
        }
    }
    *out = 0;
    return buf;
}

static int print_arg(uint32_t arg)
{
    int len = fprintf(stderr, "%x", arg);

    const char * name = eos_lookup_symbol(arg);
    if (name && name[0])
    {
        return len + fprintf(stderr, " %s", name);
    }

    const char * str = 0;
    if ((str = looks_like_string(arg)))
    {
        return len + fprintf(stderr, " \"%s\"", string_escape(str));
    }

    if (is_sane_ptr(arg, 4))
    {
        uint32_t mem_arg;
        cpu_physical_memory_read(arg, &mem_arg, sizeof(mem_arg));
        if ((str = looks_like_string(mem_arg)))
        {
            return len + fprintf(stderr, " &\"%s\"", string_escape(str));
        }
    }

    return len;
}

static int print_args(uint32_t * regs)
{
    /* fixme: guess the number of arguments */
    int len = 0;
    len += fprintf(stderr, "(");
    len += print_arg(regs[0]);
    len += fprintf(stderr, ", ");
    len += print_arg(regs[1]);
    len += fprintf(stderr, ", ");
    len += print_arg(regs[2]);
    len += fprintf(stderr, ", ");
    len += print_arg(regs[3]);
    len += fprintf(stderr, ")");
    return len;
}

static void eos_callstack_log_mem(EOSState *s, hwaddr _addr, uint64_t _value, uint32_t size, int flags)
{
    uint32_t addr = _addr;
    uint32_t value = _value;
    int is_write = flags & 1;
    int is_read = !is_write;
    uint32_t sp = CURRENT_CPU->env.regs[13];

    if (is_read && 
        addr > sp &&
        addr < sp + 0x100)
    {
        uint8_t id = get_stackid(s);
        int call_depth = call_stack_num[id];
        if (call_depth)
        {
            uint32_t caller1_sp = eos_callstack_get_caller_param(s, 0, CALLER_SP);

            if (addr >= caller1_sp)
            {
                /* read access in 1st caller stack frame or beyond? */
                uint32_t caller1_frame_size = eos_callstack_get_caller_param(s, 0, CALLER_STACKFRAME_SIZE);
                uint32_t caller2_sp = caller1_sp + caller1_frame_size;

                if (addr < caller2_sp)
                {
                    /* reading more than 4 arguments from stack? */

                    int level = call_stack_num[id] - 1;
                    int num_args = call_stacks[id][level].num_args;
                    for (int i = 0; i < num_args; i++)
                    {
                        uint32_t arg = eos_callstack_get_caller_param(s, 0, i);
                        if (arg >= sp && arg <= addr)
                        {
                            /* found a pointer to this address
                             * probably a local variable */
                            return;
                        }
                    }

                    int arg_num = 5 + (addr - caller1_sp) / 4;
                    if (arg_num > 9)
                    {
                        return;
                    }

                    call_stacks[id][level].num_args = MAX(arg_num, call_stacks[id][level].num_args);

                    if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
                        uint32_t pc = CURRENT_CPU->env.regs[15];
                        uint32_t lr = CURRENT_CPU->env.regs[14];
                        int len = eos_callstack_indent(s);
                        len += fprintf(stderr, "arg%d = ", arg_num);
                        len += print_arg(value);
                        len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
                        print_call_location(s, pc, lr);
                        if (arg_num > 10)
                        {
                            eos_callstack_indent(s);
                            eos_callstack_print(s, "cstack:", " ", "\n");
                            assert(0);
                        }
                    }
                }
            }
        }
    }

    if (is_write && qemu_loglevel_mask(EOS_LOG_CALLS))
    {
        uint8_t id = get_stackid(s);
        int call_depth = call_stack_num[id];
        if (call_depth)
        {
            int level = call_stack_num[id] - 1;
            int num_args = call_stacks[id][level].num_args;
            for (int i = 0; i < num_args; i++)
            {
                uint32_t arg = eos_callstack_get_caller_param(s, 0, i);
                if (addr == arg)
                {
                    uint32_t pc = CURRENT_CPU->env.regs[15];
                    uint32_t lr = CURRENT_CPU->env.regs[14];
                    int len = eos_callstack_indent(s);
                    len += fprintf(stderr, "*%x = %x", addr, value);
                    len += eos_indent(len, CALLSTACK_RIGHT_ALIGN - 4);
                    len += fprintf(stderr, "arg%d", i + 1);
                    len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
                    print_call_location(s, pc, lr);
                }
            }
        }
    }
}

static int check_abi_register_usage(EOSState *s, CPUARMState *env, int id, int k, uint32_t prev_pc, int output)
{
    int warnings = 0;

    /* check whether the scratch registers were preserved as specified in the ABI */
    for (int i = 4; i <= 11; i++)
    {
        if (env->regs[i] != call_stacks[id][k].regs[i])
        {
            warnings++;

            if (output)
            {
                int len = eos_callstack_indent(s);
                len += fprintf(stderr, KCYN"Warning: R%d not restored"KRESET" (0x%x -> 0x%x)", i, call_stacks[id][k].regs[i], env->regs[i]);
                len -= strlen(KCYN KRESET);
                len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
                uint32_t stack_lr = call_stacks[id][k].lr;
                print_call_location(s, prev_pc, stack_lr);
            }
        }
    }

    uint32_t sp = env->regs[13];
    if (sp != call_stacks[id][k].sp)
    {
        warnings++;

        if (output)
        {
            /* fixme: this is OK when ML patches the startup process, but not OK otherwise */
            int len = eos_callstack_indent(s);
            len += fprintf(stderr, KCYN"Warning: SP not restored"KRESET" (0x%x -> 0x%x)", call_stacks[id][k].sp, sp);
            len -= strlen(KCYN KRESET);
            len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
            uint32_t stack_lr = call_stacks[id][k].lr;
            print_call_location(s, prev_pc, stack_lr);
        }
    }

    return warnings;
}

/* store the exec log state for each task
 * (to avoid disruption from interrupts) */
struct call_stack_exec_state
{
    uint32_t pc;
    uint32_t lr;
    uint32_t sp;
    uint32_t size;
};

static struct call_stack_exec_state cs_exec_states[COUNT(call_stacks)];

static void eos_callstack_log_exec(EOSState *s, CPUState *cpu, TranslationBlock *tb)
{
    ARMCPU *arm_cpu = ARM_CPU(cpu);
    CPUARMState *env = &arm_cpu->env;

    /* use local state for speed */
    static uint32_t prev_pc = 0xFFFFFFFF;
    static uint32_t prev_lr = 0;
    static uint32_t prev_sp = 0;
    static uint32_t prev_size = 0;

    /* pc, lr, sp */
    uint32_t pc = env->regs[15] | env->thumb;
    uint32_t lr = env->regs[14];
    uint32_t sp = env->regs[13];

    /* with the Thumb bit cleared */
    uint32_t pc0 = pc & ~1;
    uint32_t lr0 = lr & ~1;
    uint32_t prev_pc0 = prev_pc & ~1;

    uint32_t vbar = env->cp15.vbar_s;
    assert(vbar == 0 || s->model->digic_version == 7);

    /* tb->pc always has the Thumb bit cleared */
    assert(pc0 == tb->pc);

    /* for some reason, this may called multiple times on the same PC */
    if (prev_pc0 == pc0 && prev_sp == sp && prev_lr == lr) return;

    /* our uninitialized stubs are 0 - don't log this address */
    if (!pc0) return;

    if (0)
    {
        fprintf(stderr, "   * PC:%x->%x LR:%x->%x SP:%x->%x */\n",
            prev_pc, pc, prev_lr, lr, prev_sp, sp
        );
    }

    /* only check cases where PC is "jumping" */
    if (pc == prev_pc + prev_size)
    {
        goto end;
    }

    if (pc0 == vbar + 0x18)
    {
        /* handle interrupt jumps first */

        if (interrupt_level == 0)
        {
            /* interrupted a regular DryOS task
             * save the previous state (we'll need to restore it when returning from interrupt) */
            uint8_t id = get_stackid(s);
            assert(id != 0xFE);
            //fprintf(stderr, "Saving state [%x]: pc %x, lr %x, sp %x, size %x\n", id, prev_pc, prev_lr, prev_sp, prev_size);
            cs_exec_states[id] = (struct call_stack_exec_state) {
                .pc = prev_pc,
                .lr = prev_lr,
                .sp = prev_sp,
                .size = prev_size
            };
        }

        interrupt_level++;
        uint8_t id = get_stackid(s);
        assert(id == 0xFE);
        if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
            int len = call_stack_indent(id, 0, 0);
            len += fprintf(stderr, KCYN"interrupt %02Xh"KRESET, s->irq_id);
            len -= strlen(KCYN KRESET);
            len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
            print_call_location(s, prev_pc, prev_lr);
        }
        if (interrupt_level == 1) assert(call_stack_num[id] == 0);
        assert(s->irq_id);
        call_stack_push(id, env->regs, pc, prev_pc, 0, s->irq_id);
        goto end;
    }

    if (prev_pc0 == vbar + 0x18)
    {
        /* jump from the interrupt vector - ignore */
        goto end;
    }

/* when returning from interrupt */
recheck:

    /* when returning from a function call,
     * it may decrement the call stack by 1 or more levels
     * see e.g. tail calls (B func) - in this case, one BX LR
     * does two returns (or maybe more, if tail calls are nested)
     * 
     * idea from panda callstack_instr: just look up each PC in the call stack
     * for speed: only check this for PC not advancing by 1 instruction
     * that's not enough, as about 30% of calls are "jumps" (!)
     * other heuristic: a return must either set PC=LR (BX LR) or change SP
     */

    if (pc == lr || sp != prev_sp)
    {
        uint8_t id = get_stackid(s);
        int interrupts_found = 0;

        for (int k = call_stack_num[id]-1; k >= 0; k--)
        {
            if (call_stacks[id][k].interrupt_id)
            {
                /* handled later */
                interrupts_found++;
                continue;
            }

            if (pc == call_stacks[id][k].lr)
            {
                /* pop tail calls, if any */
                while (k > 0 && call_stacks[id][k].is_tail_call) {
                    k--;
                    assert(pc == call_stacks[id][k].lr);
                }

                call_stack_num[id] = k;

                if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
                    int len = call_stack_indent(id, 0, 0);
                    len += fprintf(stderr, "return %x to 0x%X", env->regs[0], pc | env->thumb);
                    len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);

                    /* print LR from the call stack, so it will always show the caller */
                    int level = call_stack_num[id] - 1;
                    uint32_t stack_lr = level >= 0 ? call_stacks[id][level].lr : 0;
                    print_call_location(s, prev_pc, stack_lr);
                }

                if (interrupts_found) {
                    call_stack_indent(id, 0, 0);
                    fprintf(stderr, KBLU"FIXME: missed reti"KRESET"\n");
                    interrupt_level -= interrupts_found;
                }

                check_abi_register_usage(s, env, id, k, prev_pc, 1);

                /* to check whether this heuristic affects the results */
                /* (normally it should be optimized out...) */
                if (0 && !(pc == lr || sp != prev_sp))
                {
                    assert(0);
                }

                /* todo: callback here? */

                goto end;
            }
        }
    }

    /* when a function call is made:
     * - LR contains the return address (but that doesn't mean it's always modified, e.g. calls in a loop)
     * - PC jumps (it does not execute the next instruction after the call)
     * - SP is unchanged (it may be decremented inside the function, but not when executing the call)
     * - note: the first two also happen when handling an interrupt, so we check this case earlier
     */
    if (lr0 == prev_pc0 + prev_size)
    {
    function_call:
        assert(sp == prev_sp);

        /* assume anything that doesn't set LR to next instruction is a tail call */
        int tail_call = (lr0 != prev_pc0 + prev_size);

        uint8_t id = get_stackid(s);

        if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
            const char * name = eos_lookup_symbol(pc);
            int len = call_stack_indent(id, 0, 0);
            len += fprintf(stderr, "%scall 0x%X", tail_call ? "tail " : "", pc | env->thumb);
            if (name && name[0]) {
                len += fprintf(stderr, " %s", name);
            }
            len += print_args(env->regs);
            len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);

            /* print LR from the call stack, so it will always show the caller */
            int level = call_stack_num[id] - 1;
            uint32_t stack_lr = level >= 0 ? call_stacks[id][level].lr : 0;
            print_call_location(s, prev_pc, stack_lr);
        }

        if (qemu_loglevel_mask(EOS_LOG_IDC)) {
            /* save to IDC if requested */
            eos_idc_log_call(s, cpu, env, tb, prev_pc, prev_lr, prev_size);
        }

        call_stack_push(id, env->regs, pc, prev_pc, tail_call, 0);

#ifdef BKT_CROSSCHECK_CALLSTACK
        if (!interrupt_level) {
            /* testing mode */
            eos_backtrace_rebuild(s, 0, 0);
        }
#endif

        /* todo: callback here? */

        goto end;
    }

    /* check all other PC jumps */
    /* skip first instruction (we need valid prev_pc) */
    if (prev_pc != 0xFFFFFFFF)
    {
        uint32_t insn;
        cpu_physical_memory_read(prev_pc0, &insn, sizeof(insn));

        if (prev_pc & 1)
        {
            /* previous instruction was Thumb */
            /* http://read.pudn.com/downloads159/doc/709030/Thumb-2SupplementReferenceManual.pdf */
            switch (prev_size)
            {
                case 4:
                {
                    /* 32-bit Thumb instruction */
                    /* see Thumb-2SupplementReferenceManual p.52 for encoding */
                    if (insn == 0xc000e9bd)
                    {
                        /* RFEIA SP!
                         * called both when exiting from interrupt and outside interrupts */
                        goto maybe_task_switch;
                    }
                    else if ((insn & 0xD000F800) == 0x8000F000)
                    {
                        /* conditional branch (p.71) - ignore */
                        goto end;
                    }
                    else if ((insn & 0xD000F800) == 0x9000F000)
                    {
                        /* unconditional branch (p.71) */
                        goto jump_instr;
                    }
                    else if ((insn & 0xFFF0FFFF) == 0xF000E8DF)
                    {
                        /* TBB [PC, Rn] (p.473) */
                        goto end;
                    }
                    break;
                }

                case 2:
                {
                    /* 16-bit Thumb instruction */
                    /* see Thumb-2SupplementReferenceManual p.43 for encoding */
                    insn &= 0xFFFF;

                    if (insn == 0x4760)
                    {
                        /* BX IP */
                        /* frequently used in interrupt handler */
                        /* workaround for the direct jump test */
                        uint8_t id = get_stackid(s);
                        int level = call_stack_num[id] - 1;
                        uint32_t stack_pc = (level >= 0) ? call_stacks[id][level].pc : 0;
                        if (prev_pc - 8 == stack_pc)
                        {
                            prev_pc -= 8;
                        }
                        goto jump_instr;
                    }

                    if (insn == 0x4718)
                    {
                        /* BX R3 */
                        /* used for indirect tail calls in a few places */
                        goto jump_instr;
                    }

                    if ((insn & 0xF800) == 0xE000)
                    {
                        /* unconditional branch */
                        goto jump_instr;
                    }

                    if ((insn & 0xF000) == 0xD000)
                    {
                        /* conditional branch - ignore */
                        goto end;
                    }

                    if (((insn & 0xFD00) == 0xB100) ||
                        ((insn & 0xFD00) == 0xB900))
                    {
                        /* CBZ/CBNZ - ignore */
                        goto end;
                    }

                    break;
                }

                default:
                {
                    assert(0);
                }
            }
        }
        else /* ARM (32-bit instructions) */
        {
            if ((insn & 0xFF000000) == 0xEA000000 ||    /* B dest */
                (insn & 0xFFFFFFF0) == 0xE12FFF10)      /* BX Rn */
            {
                /* unconditional branch */
                goto jump_instr;
            }

            if ((insn & 0x0F000000) == 0x0A000000)
            {
                /* conditional branch - ignore */
                /* fixme: a few of those are actually valid tail calls */
                goto end;
            }

            if ((insn & 0xFF7FF000) == 0xe51ff000)
            {
                /* LDR PC, [PC, #off] */
                /* long jump */
                goto jump_instr;
            }

            if (insn == 0xe8fddfff || insn == 0xe8d4ffff)
            {
                /* DryOS:   LDMFD SP!, {R0-R12,LR,PC}^
                 * VxWorks: LDMIA R4, {R0-PC}^
                 * They can be called from interrupts or from regular code. */
            maybe_task_switch:
                if (interrupt_level == 0) {
                    /* DryOS task switch outside interrupts? save the previous state
                     * we'll need to restore it when returning from interrupt back to this task
                     * fixme: duplicate code */
                    uint8_t id = get_stackid(s);
                    assert(id != 0xFE);
                    //fprintf(stderr, "Saving state [%x]: pc %x, lr %x, sp %x, size %x\n", id, prev_pc, prev_lr, prev_sp, prev_size);
                    cs_exec_states[id] = (struct call_stack_exec_state) {
                        .pc = prev_pc,
                        .lr = prev_lr,
                        .sp = prev_sp,
                        .size = prev_size
                    };
                    goto end;
                } else {
                    /* return from interrupt to a DryOS task */
                    goto reti;
                }
            }

            if (insn == 0xe8fd901f || insn == 0xe8fd800f)
            {
                /* DryOS:   LDMFD SP!, {R0-R4,R12,PC}^
                 * VxWorks: LDMFD SP!, {R0-R3,PC}^  */

            reti:
                /* this must be return from interrupt */
                /* note: internal returns inside an interrupt were handled as normal returns */
                assert(interrupt_level > 0);
                uint8_t id = get_stackid(s);
                assert(id == 0xFE);

                int interrupt_entries = 0;
                for (int k = call_stack_num[id]-1; k >= 0; k--)
                {
                    interrupt_entries += (call_stacks[id][k].interrupt_id ? 1 : 0);
                }
                assert(interrupt_level == interrupt_entries);

                /* interrupts may be nested */
                uint32_t old_pc = 0;
                for (int k = call_stack_num[id]-1; k >= 0; k--)
                {
                    if (call_stacks[id][k].interrupt_id)
                    {
                        interrupt_level--;
                        call_stack_num[id] = k;
                        uint32_t stack_ppc = call_stacks[id][k].last_pc;
                        if (pc == stack_ppc || pc == stack_ppc + 4)
                        {
                            old_pc = stack_ppc;
                            break;
                        }
                    }
                }

                if (!old_pc)
                {
                    /* context switch? */
                    old_pc = call_stacks[id][0].last_pc;
                    assert(call_stack_num[id] == 0);
                    assert(interrupt_level == 0);
                }

                if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
                    int len = call_stack_indent(id, 0, 0);
                    len += fprintf(stderr, KCYN"return from interrupt"KRESET" to %x", pc);
                    if (pc != old_pc && pc != old_pc + 4) len += fprintf(stderr, " (old=%x)", old_pc);
                    len -= strlen(KCYN KRESET);
                    len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
                    print_call_location(s, prev_pc, prev_lr);
                }

                if (call_stack_num[id] == 0)
                {
                    /* return to a DryOS task
                     * what if the interrupt interrupted one task right in the "middle"
                     * of a function call or return?
                     * note: we detect these by looking at prev_pc -> pc etc (using state variables).
                     * imagine this:
                     *   1234: BL 5678
                     *   <interrupt triggered here>
                     *   1238: next instruction
                     *   5678: STMFD ...
                     * we detect the call by checking whether PC jumped
                     * (here, from prev_pc = 1234 to somewhere else - the destination
                     *  would be the called function) and LR was set to next instruction
                     * (here, prev_pc + 4 = 1238).
                     * When the interrupt breaks this logic, we need to re-create
                     * the "normal" conditions (as if the interrupt wasn't there)
                     * and do the usual call/return checks. 
                     */
                    assert(old_pc == call_stacks[id][0].last_pc);

                    /* get the new stack ID (a regular DryOS stack) 
                     * and restore our state from there */
                    uint8_t id = get_stackid(s);
                    assert(id != 0xFE);
                    prev_pc   = cs_exec_states[id].pc; prev_pc0 = prev_pc & ~1;
                    prev_lr   = cs_exec_states[id].lr;
                    prev_sp   = cs_exec_states[id].sp;
                    prev_size = cs_exec_states[id].size;
                    //fprintf(stderr, "Restoring state [%x]: pc %x->%x lr %x->%x sp %x->%x size %x->%x\n", id, prev_pc, pc, prev_lr, lr, prev_sp, sp, prev_size, tb->size);

                    if (pc != prev_pc && pc != prev_pc + 4)
                    {
                        /* PC jump when returning from interrupt?
                         * may be a call or a return - check it */
                        goto recheck;
                    }
                }
                goto end;
            }
        }

        /* jump instruction - could it be a tail function call? */
        if (0)
        {
        jump_instr:;
            uint8_t id = get_stackid(s);
            int level = call_stack_num[id] - 1;
            struct call_stack_entry * entry = &call_stacks[id][level];
            uint32_t stack_pc = (level >= 0) ? entry->pc : 0;

            if (prev_pc == stack_pc)
            {
                /* many DIGIC 6 functions have wrappers that simply jump to another function */
                /* don't be too verbose on these, but also make sure it's really just a simple jump */
                if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
                    int len = call_stack_indent(id, 0, 0);
                    len += fprintf(stderr, "-> 0x%X", pc | env->thumb);
                    const char * name = eos_lookup_symbol(pc);
                    if (name && name[0]) {
                        len += fprintf(stderr, " %s", name);
                    }
                    len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);

                    /* print LR from the call stack, so it will always show the caller */
                    int level = call_stack_num[id] - 1;
                    uint32_t stack_lr = level >= 0 ? entry->lr : 0;
                    print_call_location(s, prev_pc, stack_lr);
                }
                for (int i = 0; i < 14; i++) {
                    if (i == 12) continue;
                    if (env->regs[i] != entry->regs[i]) {
                        fprintf(stderr, "R%d changed\n", i);
                        assert(0);
                    }
                }

                /* update PC of the last call on the stack, to allow chaining more direct jumps */
                /* (consider it is the same function call) */
                assert(level >= 0);
                assert(entry->direct_jumps[3] == 0);
                entry->direct_jumps[3] = entry->direct_jumps[2];
                entry->direct_jumps[2] = entry->direct_jumps[1];
                entry->direct_jumps[1] = entry->direct_jumps[0];
                entry->direct_jumps[0] = entry->pc;
                entry->pc = pc;
                goto end;
            }
            else /* not a direct jump to some function */
            {
                if (check_abi_register_usage(s, env, id, level, prev_pc, 0) > 0)
                {
                    /* this must be a jump inside a function
                     * do not report it */
                     goto end;
                }

                /* jumps from the current function? */
                if (prev_pc > stack_pc &&
                    prev_pc < stack_pc + 0x1000)
                {
                    /* jumps out of the current function? */
                    if (pc < stack_pc ||        /* jump "behind" the current function? */
                        pc > prev_pc + 0x100)   /* large jump forward? */
                    {
                        /* assume tail call
                         * fixme: in IDC, these must be defined before their caller,
                         * as IDA frequently gets them wrong */
                        if (!qemu_loglevel_mask(EOS_LOG_NO_TAIL_CALLS)) {
                            goto function_call;
                        }
                        /* with -d notail, report as jump */
                    }
                    else
                    {
                        /* most likely a jump inside a small function (not caught by ABI check) */
                        /* fixme: there are a few actual tail calls not caught by this */
                        if (0) {
                            fprintf(stderr, "short jump?");
                            print_call_location(s, prev_pc, prev_lr);
                        }
                        goto end;
                    }
                }
                else
                {
                    /* jump not from the current function?
                     * note: most cases appear to be valid tail calls,
                     * but not identified properly because of
                     * a missed function call right before this */
                    if (!qemu_loglevel_mask(EOS_LOG_NO_TAIL_CALLS)) {
                        /* note: many warnings with -d notail */
                        /* there's also a false warning at first jump */
                        if (!(id == 0x7F && call_stack_num[id] == 0))
                        {
                            int len = call_stack_indent(id, 0, 0);
                            len += fprintf(stderr, KCYN"Warning: missed function call?"KRESET);
                            len -= strlen(KCYN KRESET);
                            len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
                            print_call_location(s, prev_pc, prev_lr);
                        }

                        /* heuristic: assume large jumps are tail calls */
                        if (abs((int)pc - (int)prev_pc) > 0x100)
                        {
                            goto function_call;
                            /* with -d notail, report as jump */
                        }
                    }
                }
            }
            /* anything that falls through the cracks is handled below */
        }

        /* unknown jump case, to be diagnosed manually */
        if (qemu_loglevel_mask(EOS_LOG_CALLS)) {
            static uint32_t prev_jump = 0;
            if (pc != prev_jump)
            {
                prev_jump = pc;
                uint8_t id = get_stackid(s);
                int len = call_stack_indent(id, 0, 0);
                len += fprintf(stderr, KCYN"jump to 0x%X"KRESET" lr=%x", pc | env->thumb, lr);
                len -= strlen(KCYN KRESET);
                len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
                print_call_location(s, prev_pc, prev_lr);
                call_stack_indent(id, 0, 0);
                /* hm, target_disas used to look at flags for ARM or Thumb... */
                int t0 = env->thumb; env->thumb = prev_pc & 1;
                assert(prev_size);
                target_disas(stderr, CPU(arm_env_get_cpu(env)), prev_pc0, prev_size, 0);
                env->thumb = t0;
            }
        }
    }

end:
    prev_pc = pc;
    prev_lr = lr;
    prev_sp = sp;
    prev_size = tb->size;
}

static void print_task_switch(EOSState *s, uint32_t pc, uint32_t prev_pc, uint32_t prev_lr)
{
    int len = eos_callstack_indent(s);
    len += fprintf(stderr,
        KCYN"Task switch"KRESET" to %s:%x %s",
        eos_get_current_task_name(s), pc, eos_lookup_symbol(pc)
    );
    len -= strlen(KCYN KRESET);
    len += eos_indent(len, CALLSTACK_RIGHT_ALIGN);
    print_call_location(s, prev_pc, prev_lr);
}

static void eos_tasks_log_exec(EOSState *s, CPUState *cpu, TranslationBlock *tb)
{
    ARMCPU *arm_cpu = ARM_CPU(cpu);
    CPUARMState *env = &arm_cpu->env;
    
    static uint32_t prev_pc = 0xFFFFFFFF;
    static uint32_t prev_lr = 0;
    uint32_t pc = env->regs[15] | env->thumb;
    uint32_t lr = env->regs[14];

    /* for task switch, only check large PC jumps */
    if (abs((int)pc - (int)prev_pc) > 16 &&
        s->model->current_task_addr)
    {
        /* fixme: this method catches all task switches,
         * but requires reading current_task_ptr from guest memory at every PC jump */
        uint32_t current_task_ptr;
        static uint32_t previous_task_ptr;
        cpu_physical_memory_read(s->model->current_task_addr, &current_task_ptr, 4);
        if (current_task_ptr != previous_task_ptr)
        {
            previous_task_ptr = current_task_ptr;
            print_task_switch(s, pc, prev_pc, prev_lr);
        }
    }

    prev_pc = pc;
    prev_lr = lr;
}

#ifdef __SIZEOF_INT128__

static uint32_t block_start;        /* destination address */
static uint32_t block_size;
static uint32_t block_offset;       /* start + offset = source address */
static uint32_t block_pc;           /* address of code that copies this block */
static uint32_t last_read_addr;
static uint32_t last_read_size;
static __uint128_t last_read_value;
static uint32_t last_write_addr;
static uint32_t last_write_size;
static __uint128_t last_write_value;

static char dd_path[100];
static FILE * dd = 0;

static void close_dd(void)
{
    fclose(dd); dd = 0;
    fprintf(stderr, "%s saved.\n", dd_path);
}

static void romcpy_log_block(EOSState *s)
{
    uint32_t block_rom_start = block_start + block_offset;

    fprintf(stderr, "[ROMCPY] 0x%-8X -> 0x%-8X size 0x%-8X at 0x%-8X\n",
        block_rom_start, block_start, block_size, block_pc
    );

    hwaddr l = block_size;
    hwaddr block_rom_start_rel;
    MemoryRegion * mr = address_space_translate(&address_space_memory, block_rom_start, &block_rom_start_rel, &l, 0);
    assert(l == block_size);

    if (strcmp(mr->name, "eos.rom0") == 0 ||
        strcmp(mr->name, "eos.rom1") == 0)
    {
        if (!dd)
        {
            snprintf(dd_path, sizeof(dd_path), "%s/romcpy.sh", s->model->name);
            fprintf(stderr, "Logging ROM-copied blocks to %s.\n", dd_path);
            dd = fopen(dd_path, "w");
            assert(dd);
            atexit(close_dd);
        }

        /* fixme: dd executes a bit slow with bs=1 */
        fprintf(dd, "dd if=ROM%c.BIN of=%s.0x%X.bin bs=1 skip=$((0x%X)) count=$((0x%X))\n",
            mr->name[7],
            s->model->name, block_start, (int)block_rom_start_rel, block_size
        );
    }
}

static void romcpy_log_n_reset_block(EOSState *s)
{
    if (block_size >= 64)
    {
        /* block large enough to report? */
        romcpy_log_block(s);
    }

    /* reset block */
    block_start = 0;
    block_size = 0;
    block_offset = 0;
    block_pc = 0;
}

static void romcpy_new_block(uint32_t write_addr, uint32_t read_addr, uint32_t size, uint32_t pc)
{
    block_start  = write_addr;
    block_size   = size;
    block_offset = read_addr - write_addr;
    block_pc = pc;
}

static void eos_romcpy_log_mem(EOSState *s, MemoryRegion *mr, hwaddr _addr, uint64_t _value, uint32_t size, int flags)
{
    if (flags & NOCHK_LOG)
    {
        /* this is from our DMA; we don't want these to be logged :) */
        return;
    }

    uint32_t addr = _addr;
    uint32_t value = _value;
    int is_write = flags & 1;
    int is_read = !is_write;
    static int prev_read = 0;
    int was_read = prev_read;
    prev_read = is_read;

    if (is_read)
    {
        if (mr->rom_device)
        {
            if (was_read && addr == last_read_addr + last_read_size)
            {
                last_read_value |= ((__int128_t)value << (last_read_size * 8));
                last_read_size += size;
                qemu_log_mask(EOS_LOG_VERBOSE, "%d-bit read from %x (accumulated)\n", last_read_size * 8, last_read_addr);
            }
            else
            {
                last_read_addr = addr;
                last_read_value = value;
                last_read_size = size;
                qemu_log_mask(EOS_LOG_VERBOSE, "%d-bit read from %x\n", last_read_size * 8, last_read_addr);
            }
        }
    }
    else /* write */
    {
        if (mr->ram)
        {
            if (size < last_read_size && addr == last_write_addr + last_write_size)
            {
                last_write_value |= ((__int128_t)value << (last_write_size * 8));
                last_write_size += size;
                qemu_log_mask(EOS_LOG_VERBOSE, "%d-bit write to %x (accumulated)\n", last_write_size * 8, last_write_addr);
            }
            else
            {
                last_write_addr = addr;
                last_write_value = value;
                last_write_size = size;
                qemu_log_mask(EOS_LOG_VERBOSE, "%d-bit write to %x\n", last_write_size * 8, last_write_addr);
            }

            if (last_write_size == last_read_size)
            {
                /* reset accumulation */
                uint32_t item_size = last_write_size;
                last_write_size = last_read_size = 0;
                qemu_log_mask(EOS_LOG_VERBOSE, "%d-bit R/W\n", item_size * 8);

                if (last_write_value == last_read_value)
                {
                    uint32_t offset = last_read_addr - last_write_addr;
                    qemu_log_mask(EOS_LOG_VERBOSE, "%d-bit copy from %x to %x (offset %x)\n", item_size * 8, last_read_addr, last_write_addr, offset);
                    qemu_log_mask(EOS_LOG_VERBOSE, "current block: %x-%x (offset %x)\n", block_start, block_start + block_size, block_offset);

                    if (offset == block_offset)
                    {
                        if (last_write_addr == block_start + block_size)
                        {
                            /* growing current block to right */
                            block_size += item_size;
                            qemu_log_mask(EOS_LOG_VERBOSE, "grow block to right: %x-%x\n", block_start, block_start + block_size);
                            return;
                        }
                        if (last_write_addr == block_start - item_size)
                        {
                            /* growing current block to left */
                            block_start -= item_size;
                            block_size += item_size;
                            qemu_log_mask(EOS_LOG_VERBOSE, "grow block to left: %x-%x\n", block_start, block_start + block_size);
                            return;
                        }
                    }

                    /* not growing current block; assume a new one might have been started */
                    qemu_log_mask(EOS_LOG_VERBOSE, "new block (previous %x-%x)\n", block_start, block_start + block_size);
                    uint32_t pc = CURRENT_CPU->env.regs[15];
                    romcpy_log_n_reset_block(s);
                    romcpy_new_block(last_write_addr, last_read_addr, item_size, pc);
                }
            }
        }
        else /* some other value written to memory */
        {
            qemu_log_mask(EOS_LOG_VERBOSE, "reset block (previous %x-%x)\n", block_start, block_size);
            romcpy_log_n_reset_block(s);
        }
    }
}

#else /* no int128_t available */

static void eos_romcpy_log_mem(EOSState *s, MemoryRegion *mr, hwaddr _addr, uint64_t _value, uint32_t size, int flags)
{
	fprintf(stderr, "FIXME: ROMCPY not supported on this platform.\n");
}
#endif

static uint64_t saved_loglevel = 0;

static void tb_exec_cb(void *opaque, CPUState *cpu, TranslationBlock *tb)
{
    if (current_cpu->cpu_index)
    {
        /* ignore CPU1 for now */
        return;
    }

    if (qemu_loglevel_mask(EOS_LOG_AUTOEXEC) &&
        saved_loglevel != 0 &&
        saved_loglevel != qemu_loglevel)
    {
        if ((tb->pc & ~0x40000000) == 0x800000)
        {
            fprintf(stderr, "[EOS] enabling verbose logging for autoexec.bin...\n");
            qemu_loglevel = saved_loglevel;
            saved_loglevel = 0;
        }
    }

    if (qemu_loglevel_mask(EOS_LOG_TASKS))
    {
        eos_tasks_log_exec(opaque, cpu, tb);
    }

    if (qemu_loglevel_mask(EOS_LOG_CALLSTACK))
    {
        /* - callstack only exposes this functionality
         *   to other "modules" on request
         * - calls is verbose and implies callstack
         */
        eos_callstack_log_exec(opaque, cpu, tb);
    }

    if (qemu_loglevel_mask(EOS_LOG_RAM_MEMCHK))
    {
        ARMCPU *arm_cpu = ARM_CPU(cpu);
        CPUARMState *env = &arm_cpu->env;
        eos_memcheck_log_exec(opaque, tb->pc, env);
    }

#ifdef BKT_CROSSCHECK_EXEC
    eos_bkt_log_exec(opaque);
#endif

    if (qemu_loglevel_mask(EOS_LOG_DEBUGMSG) && DebugMsg_addr)
    {
        static uint32_t prev_pc = 0;
        if (tb->pc == DebugMsg_addr && tb->pc != prev_pc)
        {
            if (qemu_loglevel_mask(EOS_LOG_DEBUGMSG) &&
                qemu_loglevel_mask(EOS_LOG_VERBOSE))
            {
                eos_callstack_print_verbose(opaque);
            }
            DebugMsg_log(opaque);
        }
        prev_pc = tb->pc;
    }
}

static void load_symbols(const char * elf_filename)
{
    fprintf(stderr, "[EOS] loading symbols from %s ", elf_filename);
    uint64_t lo, hi;
    int size = load_elf(elf_filename, 0, 0, 0, &lo, &hi, 0, EM_ARM, 1);
    fprintf(stderr, "(%X - %X)\n", (int) lo, (int) hi);
    assert(size > 0);
}

void eos_getenv_hex(const char * env_name, uint32_t * var, uint32_t default_value)
{
    char * env = getenv(env_name);

    if (env)
    {
        *var = strtoul(env, NULL, 16);
    }
    else
    {
        *var = default_value;
    }
}

void eos_logging_init(EOSState *s)
{
    eos_getenv_hex("QEMU_EOS_DEBUGMSG", &DebugMsg_addr, 0);

    if (qemu_loglevel_mask(EOS_LOG_CALLSTACK    |
                           EOS_LOG_RAM_MEMCHK   |
                           EOS_LOG_TASKS        |
                           EOS_LOG_AUTOEXEC     |
                           EOS_LOG_DEBUGMSG     |
                           0))
    {
        fprintf(stderr, "[EOS] enabling code execution logging.\n");
        cpu_set_tb_exec_cb(tb_exec_cb, s);
    }

    if (qemu_loglevel_mask(EOS_LOG_MEM))
    {
        int mem_access_mode =
            (qemu_loglevel_mask(EOS_LOG_MEM_R) ? PROT_READ : 0) |
            (qemu_loglevel_mask(EOS_LOG_MEM_W) ? PROT_WRITE : 0);

        fprintf(stderr, "[EOS] enabling memory access logging (%s%s).\n",
            (mem_access_mode & PROT_READ) ? "R" : "",
            (mem_access_mode & PROT_WRITE) ? "W" : ""
        );

        memory_set_access_logging_cb(eos_log_mem, s, mem_access_mode);

        /* make sure the backends are enabled */
        if (qemu_loglevel_mask(EOS_PR(EOS_LOG_RAM_R))) assert(qemu_loglevel_mask(EOS_LOG_RAM_R));
        if (qemu_loglevel_mask(EOS_PR(EOS_LOG_RAM_W))) assert(qemu_loglevel_mask(EOS_LOG_RAM_W));
        if (qemu_loglevel_mask(EOS_PR(EOS_LOG_ROM_R))) assert(qemu_loglevel_mask(EOS_LOG_ROM_R));
        if (qemu_loglevel_mask(EOS_PR(EOS_LOG_ROM_W))) assert(qemu_loglevel_mask(EOS_LOG_ROM_W));
    }

    if (qemu_loglevel_mask(EOS_LOG_RAM_MEMCHK))
    {
        eos_memcheck_init(s);
    }

    if (qemu_loglevel_mask(CPU_LOG_TB_NOCHAIN))
    {
        fprintf(stderr, "[EOS] enabling singlestep.\n");
        singlestep = 1;
    }

    const char * ml_path = getenv("QEMU_ML_PATH");
    if (ml_path && ml_path[0])
    {
        char sym[512];
        snprintf(sym, sizeof(sym), "%s/autoexec", ml_path);
        load_symbols(sym);
        snprintf(sym, sizeof(sym), "%s/magiclantern", ml_path);
        load_symbols(sym);
    }

    if (qemu_loglevel_mask(EOS_LOG_AUTOEXEC) && qemu_loglevel)
    {
        saved_loglevel = qemu_loglevel;
        qemu_loglevel &= ~(
            EOS_LOG_VERBOSE |
            EOS_LOG_CALLS   |
            EOS_LOG_IO      |
            EOS_LOG_MEM     |
            CPU_LOG_EXEC    |
        0);

        if (saved_loglevel != qemu_loglevel)
        {
            fprintf(stderr, "[EOS] disabling verbose logging until autoexec.bin starts...\n");
        }
    }
}
