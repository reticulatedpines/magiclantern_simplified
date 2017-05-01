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

    if (qemu_loglevel_mask(EOS_LOG_RAM_DBG))
    {
        /* perform a self-test to make sure the loggers capture
         * all memory write events correctly (not sure how to check reads)
         */
        eos_log_selftest(s, addr, value, size, flags);
    }
    else
    {
        /* with -d mem, log memory accesses in the same way as I/O */
        /* -d mem_dbg does not print them unless you also specify io */
        mode |= FORCE_LOG;
    }
    

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

static void eos_log_calls(CPUState *cpu, TranslationBlock *tb)
{
    ARMCPU *arm_cpu = ARM_CPU(cpu);
    CPUARMState *env = &arm_cpu->env;
    
    static FILE * idc = NULL;
    static int stderr_dup = 0;
    static uint32_t prev_pc = 0;
    static uint32_t prev_lr = 0;
    static uint32_t prev_sp = 0;
    static uint32_t prev_size = 0;
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
    
    /* bit array for every possible PC & ~3 */
    static uint32_t saved_pcs[(1 << 30) / 32] = {0};
    
    /* when a function call is made:
     * - LR is updated with the return address
     * - stack is decremented
     * - note: the above might also happen during a context switch,
     *   so use a heuristic to filter them out
     */
    if (lr != prev_lr && sp <= prev_sp &&
        abs((int)sp - (int)prev_sp) < 1024)
    {
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

        /* log each call to console */
        fprintf(stderr, "%08X: call 0x%X (%s)\n", prev_pc, pc, env->thumb ? "Thumb" : "ARM");
    }
    
    if (pc != prev_pc && pc == (prev_lr & ~1))
    {
        fprintf(stderr, "%08X: return to 0x%X (%s)\n", prev_pc, pc, env->thumb ? "Thumb" : "ARM");
    }
    prev_pc = pc;
    prev_lr = lr;
    prev_sp = sp;
    prev_size = tb->size;
}

static void tb_exec_cb(void *opaque, CPUState *cpu, TranslationBlock *tb)
{
    if (qemu_loglevel_mask(EOS_LOG_CALLS))
    {
        eos_log_calls(cpu, tb);
    }
}

void eos_logging_init(EOSState *s)
{
    cpu_set_tb_exec_cb(tb_exec_cb, s);

    if (qemu_loglevel_mask(EOS_LOG_MEM)) {
        fprintf(stderr, "Enabling memory access logging.\n");
        int access_mode =
            (qemu_loglevel_mask(EOS_LOG_MEM_R) ? PROT_READ : 0) |
            (qemu_loglevel_mask(EOS_LOG_MEM_W) ? PROT_WRITE : 0);
        memory_set_access_logging_cb(eos_log_mem, s, access_mode);
    }
}
