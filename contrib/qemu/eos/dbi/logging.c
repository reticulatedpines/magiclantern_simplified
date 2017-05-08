#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "hw/boards.h"
#include "../eos.h"
#include "../model_list.h"
#include "logging.h"
#include "memcheck.h"

static inline int should_log_memory_region(MemoryRegion * mr)
{
    if (mr->ram && qemu_loglevel_mask(EOS_LOG_RAM)) {
        return 1;
    }

    if (mr->rom_device && qemu_loglevel_mask(EOS_LOG_ROM)) {
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

    if (!should_log_memory_region(mr))
    {
        return;
    }

    EOSState* s = (EOSState*) opaque;
    bool some_tool_executed = false;

    if (qemu_loglevel_mask(EOS_LOG_RAM_DBG))
    {
        /* perform a self-test to make sure the loggers capture
         * all memory write events correctly (not sure how to check reads)
         */
        eos_log_selftest(s, addr, value, size, flags);
        some_tool_executed = true;
    }

    if (qemu_loglevel_mask(EOS_LOG_RAM_MEMCHK))
    {
        /* in memcheck.c */
        eos_memcheck_log_mem(s, addr, value, size, flags);
        some_tool_executed = true;
    }

    if (some_tool_executed && !(qemu_loglevel_mask(EOS_LOG_VERBOSE) &&
                                qemu_loglevel_mask(EOS_LOG_IO)))
    {
        /* when executing some memory checking tool,
         * do not log messages unless -d io,verbose is specified
         */
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
    assert(strncmp(mr->name, "eos.", 4) == 0);
    io_log(mr->name + 4, s, addr, mode, value, value, msg, msg_arg1, msg_arg2);
}



/* ----------------------------------------------------------------------------- */



static void eos_idc_log_call(CPUState *cpu, CPUARMState *env,
    TranslationBlock *tb, uint32_t prev_pc, uint32_t prev_lr, uint32_t prev_sp, uint32_t prev_size)
{
    static FILE * idc = NULL;
    static int stderr_dup = 0;

    if (!idc)
    {
        char idc_path[100];

        snprintf(idc_path, sizeof(idc_path), "%s.idc", MACHINE_GET_CLASS(current_machine)->name);
        fprintf(stderr, "Exporting called functions to %s.\n", idc_path);
        idc = fopen(idc_path, "w");
        assert(idc);
        
        /* QEMU is usually closed with CTRL-C, so call this when finished */
        void close_idc(void)
        {
            fprintf(idc, "}\n");
            fclose(idc);
        }
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
        fprintf(stderr, "   * PC:%x->%x LR:%x->%x SP:%x->%x */\n",
            prev_pc, pc, prev_lr, lr, prev_sp, sp
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
    uint32_t lr;    /* location of call (LR) */
    uint32_t sp;    /* stack pointer before the call */
};

static struct call_stack_entry call_stacks[256][128];
static int call_stack_num[256] = {0};

static void call_stack_push(uint8_t id, uint32_t lr, uint32_t sp)
{
    assert(call_stack_num[id] < COUNT(call_stacks[0]));
    call_stacks[id][call_stack_num[id]++] = (struct call_stack_entry) {
        .lr = lr,
        .sp = sp,
    };
}

#if 0
static uint32_t call_stack_pop(uint8_t id)
{
    assert(call_stack_num[id] > 0);
    return call_stacks[id][--call_stack_num[id]].lr;
}
#endif

static int indent(int initial_len, int target_indent)
{
    char buf[128];
    int len = target_indent - initial_len;
    assert(len < sizeof(buf));
    memset(buf, ' ', len);
    buf[len] = 0;
    fprintf(stderr, "%s", buf);
    return len;
}

static int call_stack_indent(uint8_t id, int initial_len, int max_initial_len)
{
    int len = initial_len;
    len += indent(initial_len, max_initial_len + call_stack_num[id]);
    return len;
}

int eos_callstack_indent(EOSState *s)
{
    return call_stack_indent(get_stackid(s), 0, 0);
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
    for (int k = call_stack_num[id]-1; k >= 0; k--)
    {
        uint32_t pc = call_stacks[id][k].lr;
        len += fprintf(stderr, "%x%s", pc, sep);
    }
    len += fprintf(stderr, "%s", suffix);
    return len;
}

static void print_location(EOSState *s, uint32_t prev_pc, uint32_t prev_lr)
{
    if (interrupt_level) {
        fprintf(stderr, "at [INT-%02x:%x:%x]\n", s->irq_id, prev_pc, prev_lr);
    } else {
        char * task_name = eos_get_current_task_name(s);
        if (task_name) {
            fprintf(stderr, "at [%s:%x:%x]\n", task_name, prev_pc, prev_lr);
        } else {
            fprintf(stderr, "at [%x:%x]\n", prev_pc, prev_lr);
        }
    }
}

static void eos_log_calls(EOSState *s, CPUState *cpu, TranslationBlock *tb)
{
    ARMCPU *arm_cpu = ARM_CPU(cpu);
    CPUARMState *env = &arm_cpu->env;
    
    static uint32_t prev_pc = 0xFFFF0000;
    static uint32_t prev_lr = 0;
    static uint32_t prev_sp = 0;
    static uint32_t prev_size = 0;
    uint32_t pc = env->regs[15];
    uint32_t lr = env->regs[14];
    uint32_t sp = env->regs[13];
    assert(pc == tb->pc);

    if (0)
    {
        fprintf(stderr, "   * PC:%x->%x LR:%x->%x SP:%x->%x */\n",
            prev_pc, pc, prev_lr, lr, prev_sp, sp
        );
    }
    /* when returning from a function call,
     * it may decrement the call stack by 1 or more levels
     * see e.g. tail calls (B func) - in this case, one BX LR
     * does two returns (or maybe more, if tail calls are nested)
     * 
     * idea from panda callstack_instr: just look up each PC in the call stack
     * (for speed: only check this for PC not advancing by 1 instruction)
     */

    if (pc != prev_pc + 4)
    {
        uint8_t id = get_stackid(s);

        if (call_stack_num[id])
        {
            prev_lr = call_stacks[id][call_stack_num[id]-1].lr;
        }

        for (int k = call_stack_num[id]-1; k >= 0; k--)
        {
            if (interrupt_level && k == 0)
            {
                /* return from interrupt; handled later */
                continue;
            }

            if (pc == call_stacks[id][k].lr)
            {
                call_stack_num[id] = k;
                if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
                    int len = call_stack_indent(id, 0, 0);
                    len += fprintf(stderr, "return to 0x%X (%s)", pc, env->thumb ? "Thumb" : "ARM");
                    len += indent(len, 64);
                    print_location(s, prev_pc, prev_lr);
                }
                goto end;
            }
        }
    }


    /* when a function call is made:
     * - LR is updated with the return address
     * - stack may be decremented (not always)
     * - note: the above might also happen during a context switch,
     *   so use a heuristic to filter them out
     */
    if (lr != prev_lr && sp <= prev_sp &&
        abs((int)sp - (int)prev_sp) < 1024)
    {
        uint8_t id = get_stackid(s);

        if (lr == pc + 4 && pc == prev_pc + 4)
        {
            /* we have executed a MOV LR, PC */
            /* let's ignore it without updating prev_lr */
            uint32_t insn;
            cpu_physical_memory_read(prev_pc, &insn, sizeof(insn));
            assert(insn == 0xe1a0e00f);
            assert(prev_sp == sp);
            lr = prev_lr;
            goto end;
        }
        if (lr == prev_pc + 4)
        {
            eos_idc_log_call(cpu, env, tb, prev_pc, prev_lr, prev_sp, prev_size);
            if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
                int len = call_stack_indent(id, 0, 0);
                len += fprintf(stderr, "call 0x%X (%s)", pc, env->thumb ? "Thumb" : "ARM");
                len += indent(len, 64);
                print_location(s, prev_pc, prev_lr);
            }
            call_stack_push(id, lr, sp);
            goto end;
        }
    }

    /* check all other large PC jumps */
    if (abs((int)pc - (int)prev_pc) > 16)
    {
        uint8_t id = get_stackid(s);

        if (pc == 0x18)
        {
            interrupt_level++;
            id = get_stackid(s);
            assert(id == 0xFE);
            if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
                int len = call_stack_indent(id, 0, 0);
                len += fprintf(stderr, KCYN"interrupt"KRESET);
                len -= strlen(KCYN KRESET);
                len += indent(len, 64);
                print_location(s, prev_pc, prev_lr);
            }
            if (interrupt_level == 1) assert(call_stack_num[id] == 0);
            call_stack_push(id, prev_pc, sp);
            goto end;
        }

        if (prev_pc == 0x18)
        {
            /* jump from the interrupt vector - ignore */
            goto end;
        }

        uint32_t insn;
        cpu_physical_memory_read(prev_pc, &insn, sizeof(insn));

        if ((insn & 0x0F000000) == 0x0A000000)
        {
            /* branch - ignore */
            goto end;
        }

        if ((insn == 0xe8fd901f || insn == 0xe8fddfff) && prev_pc < 0x1000)
        {
            /* this must be return from interrupt */
            /* note: internal returns inside an interrupt were handled as normal returns */
            assert(interrupt_level > 0);
            assert(id == 0xFE);
            if (interrupt_level == 1) assert(call_stack_num[id] == 1);
            else assert(call_stack_num[id] >= 1);

            /* it may return to the old "userspace" address,
             * or maybe to another one (if task switched) */
            uint32_t old_pc = call_stacks[id][0].lr;

            /* interrupts may be nested - just clear the stack */
            call_stack_num[id] = 0;

            if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
                int len = call_stack_indent(id, 0, 0);
                len += fprintf(stderr, KCYN"return from interrupt"KRESET" to %x", pc);
                if (pc != old_pc && pc != old_pc + 4) len += fprintf(stderr, " (old=%x)", old_pc);
                len -= strlen(KCYN KRESET);
                len += indent(len, 64);
                print_location(s, prev_pc, prev_lr);
            }

            interrupt_level = 0;
            goto end;
        }

        /* unknown jump case, to be diagnosed manually */
        if (qemu_loglevel_mask(EOS_LOG_VERBOSE)) {
            int len = call_stack_indent(id, 0, 0);
            len += fprintf(stderr, KCYN"PC jump? 0x%X (%s)"KRESET, pc, env->thumb ? "Thumb" : "ARM");
            len -= strlen(KCYN KRESET);
            len += indent(len, 64);
            print_location(s, prev_pc, prev_lr);
            call_stack_indent(id, 0, 0);
            target_disas(stderr, CPU(arm_env_get_cpu(env)), prev_pc, 4, 0);
        }
    }

end:
    prev_pc = pc;
    prev_lr = lr;
    prev_sp = sp;
    prev_size = tb->size;
}

static void tb_exec_cb(void *opaque, CPUState *cpu, TranslationBlock *tb)
{
    if (qemu_loglevel_mask(EOS_LOG_CALLS))
    {
        eos_log_calls(opaque, cpu, tb);
    }

    if (qemu_loglevel_mask(EOS_LOG_RAM_MEMCHK))
    {
        ARMCPU *arm_cpu = ARM_CPU(cpu);
        CPUARMState *env = &arm_cpu->env;
        eos_memcheck_log_exec(opaque, tb->pc, env);
    }
}

void eos_logging_init(EOSState *s)
{
    cpu_set_tb_exec_cb(tb_exec_cb, s);

    if (qemu_loglevel_mask(EOS_LOG_MEM))
    {
        fprintf(stderr, "Enabling memory access logging.\n");
        int mem_access_mode =
            (qemu_loglevel_mask(EOS_LOG_MEM_R) ? PROT_READ : 0) |
            (qemu_loglevel_mask(EOS_LOG_MEM_W) ? PROT_WRITE : 0);
        memory_set_access_logging_cb(eos_log_mem, s, mem_access_mode);
    }

    if (qemu_loglevel_mask(EOS_LOG_RAM_MEMCHK))
    {
        eos_memcheck_init(s);
    }
}
