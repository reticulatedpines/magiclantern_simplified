#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "hw/boards.h"
#include "../eos.h"

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
}
