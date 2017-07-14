/* Attempt to rebuild the call stack directly from stack contents
 * --------------------------------------------------------------
 * 
 * Advantage: no need to monitor program execution continuously
 * (no instrumentation needed until this gets called, unlike callstack)
 * 
 * The same file can be compiled to run on the camera (binary less than 1K).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"

#include "../eos.h"
#include "../model_list.h"
#include "logging.h"
#include "backtrace.h"

static uint32_t MEM(uint32_t addr)
{
    uint32_t buf;
    cpu_physical_memory_read(addr, &buf, sizeof(buf));
    return buf;
}

#ifdef BKT_TRACK_STATS
/* some stats */
static int stacks_completed = 0;        /* number of call stacks fully identified */
static int stacks_incomplete = 0;       /* num. where at least one caller was identified (besides LR) */
static int stacks_gaveup = 0;           /* num. where no caller could be identified */
static int callers_found = 0;           /* number of callers found (total over all stacks) */
static int gaveups_loop = 0;            /* num. of callers missed because the simulation got stuck in a loop */
static int gaveups_insn = 0;            /* num. of callers missed because the simulation found some unhandled instruction */
static int random_helped = 0;           /* num. of callers where taking random branches helped (when the deterministic method got stuck in a loop) */
#endif

static uint32_t ror(uint32_t word, uint32_t count)
{
    return word >> count | word << (32 - count);
}

static uint32_t decode_immediate_shifter_operand(uint32_t insn)
{
    uint32_t inmed_8 = insn & 0xFF;
    uint32_t rotate_imm = (insn & 0xF00) >> 7;
    return ror(inmed_8, rotate_imm);
}

static inline int32_t decode_signed_offset12(uint32_t insn)
{
    uint32_t off = (insn & 0x00000FFF);
    if (insn & (1 << 23))
        return off;
    return -off;
}

static uint32_t branch_destination(uint32_t insn, uint32_t pc)
{
    int32_t off = (((int32_t) insn & 0x00FFFFFF) << 8) >> 6;
    uint32_t dest = pc + off + 8;
    return dest;
}

#ifndef BKT_ASSUME_TAIL_CALL_AFTER_POP_LR
static int is_mov(uint32_t insn)
{
    if ((insn & 0xFDFF0000) == 0xE1A00000)
    {
        if ((insn & 0x02000090) == 0x00000090)
        {
            /* Multiply instruction extension space */
            return 0;
        }
        return 1;
    }
    return 0;
}

static int is_compare(uint32_t insn)
{
    if ((insn & 0xFDF0F000) == 0xE1500000 ||
        (insn & 0xFDF0F000) == 0xE1700000 ||
        (insn & 0xFDF0F000) == 0xE1100000 ||
        (insn & 0xFDF0F000) == 0xE1300000)
    {
        if ((insn & 0x02000090) == 0x00000090)
        {
            /* Multiply instruction extension space */
            return 0;
        }
        /* CMP, CMN, TST, TEQ */
        return 1;
    }
    return 0;
}

static int possibly_func_start(uint32_t dest)
{
    uint32_t insn = MEM(dest);
    if (is_mov(insn) || is_compare(insn)) {
        insn = MEM(dest+4);
    }
    if (is_mov(insn) || is_compare(insn)) {
        insn = MEM(dest+8);
    }
    if ((insn & 0xFFFF0000) == 0xE92D0000)
    {
        /* STMFD */
        return 1;
    }
    if (insn == 0xe52de004)
    {
        /* STR LR, [SP,#-4]! */
        return 1;
    }
    return 0;
}
#endif

static const char * called_func(uint32_t pc)
{
    static char buf[16];
    snprintf(buf, sizeof(buf), "0xUNKNOWN ");
    uint32_t insn = MEM(pc);
    if ((insn & 0x0E000000) == 0x0A000000)
    {
        snprintf(buf, sizeof(buf), "0x%-8X", branch_destination(insn, pc));
    }
    return buf;
}

static void handle_pop_pc(uint32_t pc, uint32_t dest, uint32_t *next_pc)
{
    qemu_log_mask(BKT_LOG_VERBOSE, "%08x: POP PC => %x\n", pc, dest);
    *next_pc = dest;
}

/* stack change should be handled by the caller */
static void handle_tail_call_pop_lr(uint32_t pc, uint32_t dest, uint32_t *plr, uint32_t *next_pc)
{
    qemu_log_mask(BKT_LOG_VERBOSE, "%08x: POP LR => %x\n", pc, dest);

    *plr = dest;

#ifdef BKT_ASSUME_TAIL_CALL_AFTER_POP_LR
    /* all cases that were not found to be tail calls by the #else branch,
     * appear to be valid tail calls, just harder to identify by pattern matching
     * to simplify things, let's just assume a POP LR is followed by a tail call. */
    *next_pc = *plr;
#else
    /* if testing this, change the function return type to int and check it */
    uint32_t ins2 = MEM(pc + 4);

    if (is_mov(ins2)) {
        ins2 = MEM(pc + 8);
    }
    if (is_mov(ins2)) {
        ins2 = MEM(pc + 12);
    }
    /* fixme: not very robust on conditional branches */
    if ((ins2 & 0x0F000000) == 0x0A000000 ||    /* B{cond} func */
        (ins2 & 0x0FFFFFF0) == 0x012FFF10)      /* BX{cond} Rn */
    {
        /* we are not interested in following tail calls 
         * just go back to the caller */
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: tail call\n", pc+4);
        *next_pc = *plr;
        return 1;
    }
    else
    {
        fprintf(stderr, "*** %x no tail\n", pc);
    }
    return 0;
#endif
}

/* emulate one instruction, updating pc, lr and sp */
/* fixme: ARM only */
/* returns 1 if a return address was found; otherwise 0 */
static int sim_instr(EOSState *s,
    uint32_t *ppc, uint32_t *plr, uint32_t *psp
#ifdef BKT_RANDOM_BRANCHES
    , uint32_t *pcond
#endif
)
{
    #define assert_retry(x) if (!(x)) { \
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: assert at line %d\n", pc, __LINE__); \
        return -1; \
    }

    const uint32_t pc = *ppc;
    uint32_t lr = *plr;
    uint32_t sp = *psp;
    uint32_t next_pc = pc + 4;

    if (pc & 1)
    {
        /* fixme: not implemented for Thumb code */
        /* still, some parts of the code are ARM
         * just stop on the first Thumb instruction for now */
        assert_retry(0);
    }

    uint32_t insn = MEM(pc);

#ifdef BKT_RANDOM_BRANCHES
    uint32_t cond = *pcond;

    if (cond != 0xE0000000)
    {
        /* instruction matching current condition? */
        if ((insn & 0xF0000000) == cond)
        {
            /* patch the instruction to emulate as if it would be unconditional */
            insn = (insn & 0x0FFFFFFF) | 0xE0000000;
            qemu_log_mask(BKT_LOG_VERBOSE, "%08x: patch condition (%x -> %x)\n", pc, MEM(pc), insn);
        }
    }
#endif

    if (qemu_loglevel_mask(BKT_LOG_DISAS))
    {
        CPUARMState *env = &(CURRENT_CPU->env);
        target_disas(stderr, CPU(arm_env_get_cpu(env)), pc, 4, 0);
    }

#ifdef BKT_RANDOM_BRANCHES
    if (is_compare(insn))
    {
        /* decide to execute only one random condition from next instructions
         * this should handle grouped instructions that depend on the same condition
         * (execute either none or all of them)
         */
        uint32_t cond_pc = pc;
        cond = 0xF0000000;
        do
        {
            cond_pc += 4;
            uint32_t next = MEM(cond_pc);
            if ((rand() & 1) ||                         /* use condition from this instruction? */
               ((next & 0xF0000000) == 0xE0000000))     /* no more conditional instructions? */
            {
                cond = next & 0xF0000000;
            }
        }
        while (cond == 0xF0000000);
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: cond = %x (from %x)\n", pc, cond, cond_pc);
        *pcond = cond;
        goto ret_regular_insn;
    }
#endif

    if (insn == 0xe12fff1e ||                   /* BX LR */
        insn == 0xe1a0f00e)                     /* MOV PC, LR */
    {
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: RET to %x\n", pc, lr);
        assert_retry(lr);
        next_pc = lr;
        goto ret_caller_found;
    }

    if ((insn & 0xFF000000) == 0xEA000000)      /* B dest */
    {
        /* unconditional branch */
        uint32_t dest = branch_destination(insn, pc);
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: B %x\n", pc, dest);
#ifndef BKT_ASSUME_TAIL_CALL_AFTER_POP_LR
        if (possibly_func_start(dest))
        {
            /* this appears to be a branch to a function (tail call) 
             * just go back to the caller */
            qemu_log_mask(BKT_LOG_VERBOSE, "%08x: tail call\n", pc+4);
            assert_retry(lr);
            next_pc = lr;
            goto ret_caller_found;
        }
#endif
        next_pc = dest;
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFFF000) == 0xE28DD000)      /* ADD SP, SP, #off */
    {
        uint32_t off = decode_immediate_shifter_operand(insn);
        sp += off;
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: sp += %x => %x\n", pc, off, sp);
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFFF000) == 0xE24DD000)      /* SUB SP, SP, #off */
    {
        uint32_t off = decode_immediate_shifter_operand(insn);
        sp -= off;
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: sp -= %x => %x\n", pc, off, sp);
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFF0000) == 0xE92D0000)      /* STMFD SP!, {regs} */
    {
        /* pushing SP/PC/LR is not handled (we would need to keep track of the modified stack) */
        assert_retry((insn & 0xE000) == 0);
        int num_regs = __builtin_popcount(insn & 0xFFFF);
        sp -= num_regs * 4;
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: sp -= %d*4 => %x\n", pc, num_regs, sp);
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFF0000) == 0xE8BD0000)      /* LDMFD SP!, {regs} */
    {
        int num_regs = __builtin_popcount(insn & 0xFFFF);
        /* pop both LR and PC? not handled */
        assert_retry((insn & 0xC000) != 0xC000);
        sp += num_regs * 4;
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: sp += %d*4 => %x\n", pc, num_regs, sp);
        if (insn & 0x8000)
        {
            handle_pop_pc(pc, MEM(sp - 4), &next_pc);
            goto ret_caller_found;
        }
        if (insn & 0x4000)
        {
            handle_tail_call_pop_lr(pc, MEM(sp - 4), &lr, &next_pc);
            goto ret_caller_found;
        }
        #ifdef BKT_HANDLE_UNLIKELY_CASES
        if (insn & 0x2000)
        {
            /* POP SP (unlikely) */
            assert(0);
        }
        #endif
        goto ret_regular_insn;
    }

    if ((insn & 0xFF7F0000) == 0xe41d0000)      /* LDR Rn, [SP], #off */
    {
        uint32_t dest = MEM(sp);
        int reg = (insn & 0xF000) >> 12;
        sp += decode_signed_offset12(insn);

        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: pop R%d\n", pc, reg);

        switch (reg)
        {
            case 15:
                handle_pop_pc(pc, dest, &next_pc);
                goto ret_caller_found;
            case 14:
                handle_tail_call_pop_lr(pc, dest, &lr, &next_pc);
                goto ret_caller_found;
            #ifdef BKT_HANDLE_UNLIKELY_CASES
            case 13:
                assert(0);
                break;
            #endif
        }
        goto ret_regular_insn;
    }

/* the following cases are unlikely to appear
 * optionally ignore them to keep the binary small
 * even so, we are still going to pass the BKT_CROSSCHECK_CALLSTACK tests
 * for every single function called until booting the GUI.
 */
#ifdef BKT_HANDLE_UNLIKELY_CASES

    if ((insn & 0xFF7F0000) == 0xe52d0000)      /* STR Rn, [SP, #off]! */
    {
        /* pushing PC/LR/SP unhandled */
        int reg = (insn & 0xF000) >> 12;
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: push R%d\n", pc, reg);
        sp += decode_signed_offset12(insn);
        assert_retry(reg != 13);
        assert_retry(reg != 14);
        assert_retry(reg != 15);
        goto ret_regular_insn;
    }

    if (insn == 0xe1a0e00f)                     /* MOV LR, PC */
    {
        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: MOV LR, PC\n", pc);
        lr = pc + 8;
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFFFFF0) == 0xe1a0e000)      /* mov lr, rn */
    {
        /* invalidate LR */
        qemu_log_mask(BKT_LOG_VERBOSE, "*** %x: invalid LR\n", pc);
        lr = 0;
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFFF000) == 0xe3a0e000)      /* mov lr, #imm */
    {
        lr = decode_immediate_shifter_operand(insn);
        goto ret_regular_insn;
    }

    if ((insn & 0xFF7F0000) == 0xe51f0000)      /* LDR Rn, [PC, #off] */
    {
        int off = decode_signed_offset12(insn);
        uint32_t dest = MEM(pc + off + 8);
        int reg = (insn & 0xF000) >> 12;

        qemu_log_mask(BKT_LOG_VERBOSE, "%08x: ldr R%d, =%x (%d)\n", pc, reg, dest, off);

        switch (reg)
        {
            case 15:
                next_pc = dest;
                break;
            case 14:
                qemu_log_mask(BKT_LOG_VERBOSE, "*** %x: LDR LR\n", pc);
                lr = dest;
                break;
            case 13:
                sp = dest;
                break;
        }
        goto ret_regular_insn;
    }

    if (insn == 0xe8fd901f ||                   /* LDMFD SP!, {R0-R4,R12,PC}^ */
        insn == 0xe8fddfff ||                   /* LDMFD SP!, {R0-R12,LR,PC}^ */
        insn == 0xe82d407f ||                   /* stmda	sp!, {r0, r1, r2, r3, r4, r5, r6, lr} */
        insn == 0xe9bd407f ||                   /* ldmib	sp!, {r0, r1, r2, r3, r4, r5, r6, lr} */
        (insn & 0xFFF0F000) == 0xe590d000 ||    /* LDR SP, [Rn] */
        (insn & 0xFFFFFFF0) == 0xe12ff000 ||    /* MSR CPSR_fsxc, Rn */
        (insn & 0xFFFFF000) == 0xe321f000 ||    /* MSR CPSR_c, #off */
        (insn & 0xFFFFFFF0) == 0xe04dd000 ||    /* SUB SP, SP, Rn */
        (insn & 0xFFFFF000) == 0xe24bd000 ||    /* SUB SP, FP, #off */
    0)
    {
        /* unhandled - just fail this code path and hope for the best */
        qemu_log_mask(BKT_LOG_VERBOSE, "%x: unhandled\n", pc);
        assert_retry(0);
    }

    if ((insn & 0xFFF00000) == 0xE8b00000)    /* LDM{cond} Rn!, {regs} */
    {
        /* make sure it doesn't touch SP, LR (invalidate), PC */
        /* otherwise, not interesting */
        qemu_log_mask(BKT_LOG_VERBOSE, "%x: ldm\n", pc);
        assert_retry((insn & 0xA000) == 0);
        if (insn & 0x4000) lr = 0;
    }

    if ((insn & 0xFFFFF000) == 0xe28ee000)
    {
        /* add lr, lr, #off */
        qemu_log_mask(BKT_LOG_VERBOSE, "%x: add lr\n", pc);
        lr += decode_immediate_shifter_operand(insn);
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFFF000) == 0xe24ee000)
    {
        /* sub lr, lr, #off */
        qemu_log_mask(BKT_LOG_VERBOSE, "%x: sub lr\n", pc);
        lr -= decode_immediate_shifter_operand(insn);
        goto ret_regular_insn;
    }

    if ((insn & 0xFFFFF000) == 0xe3a0d000)
    {
        /* mov sp, #imm */
        qemu_log_mask(BKT_LOG_VERBOSE, "%x: mov sp\n", pc);
        sp = decode_immediate_shifter_operand(insn);
        goto ret_regular_insn;
    }
#endif /* BKT_HANDLE_UNLIKELY_CASES */

ret_regular_insn:
    *ppc = next_pc; *plr = lr; *psp = sp;
    return 0;

ret_caller_found:
    *ppc = next_pc; *plr = lr; *psp = sp;
    return 1;

    #undef assert_retry
}

static uint32_t find_caller(EOSState *s, uint32_t pc, uint32_t *psp)
{
    uint32_t pc0 = pc;
    uint32_t lr = 0;
    uint32_t sp = *psp;
    int iter;

    /* we need to find one code path that returns from the function */
    /* first we try the deterministic path (no conditional branches taken) */
    /* if that doesn't work, we'll try taking them randomly */
#ifdef BKT_RANDOM_BRANCHES
    uint32_t cond = 0xE0000000;
    for (int retries = 0; retries < 10; retries++)
#endif
    {
        for (iter = 0; iter < 1000; iter++)
        {
#ifdef BKT_RANDOM_BRANCHES
            if (retries == 0)
            {
                /* don't take any conditional branches on first try */
                cond = 0xE0000000;
            }
            int ret = sim_instr(s, &pc, &lr, &sp, &cond);
#else
            int ret = sim_instr(s, &pc, &lr, &sp);
#endif
            if (ret == -1)
            {
                /* retry from start */
                pc = pc0;
                lr = 0;
                sp = *psp;
                break;
            }
            if (ret == 1)
            {
#ifdef BKT_TRACK_STATS
                callers_found++;
#ifdef BKT_RANDOM_BRANCHES
                if (retries)
                {
                    random_helped++;
                }
#endif
#endif
                *psp = sp;
                return pc;
            }
        }
        qemu_log_mask(BKT_LOG_VERBOSE, "[BKT] retrying (%d)...\n", iter);
    }

#ifdef BKT_TRACK_STATS
    if (iter == 1000)
    {
        gaveups_loop++;
    }
    else
    {
        gaveups_insn++;
    }
#endif

    /* fail */
#ifndef BKT_CROSSCHECK_CALLSTACK
    fprintf(stderr, "[BKT] giving up.\n");
#endif

    return 0;
}

void eos_backtrace_rebuild(EOSState *s, char * buf, int size)
{
    uint32_t pc = CURRENT_CPU->env.regs[15] | CURRENT_CPU->env.thumb;
    uint32_t lr = CURRENT_CPU->env.regs[14];
    uint32_t sp = CURRENT_CPU->env.regs[13];

    uint32_t stack_top, stack_bot;
    if (!eos_get_current_task_stack(s, &stack_top, &stack_bot))
    {
        return;
    }

    uint32_t lrs[64];
    uint32_t sps[64];
    int i = 0;
    lrs[i] = lr;
    sps[i] = sp;
    i++;

#ifdef BKT_CROSSCHECK_CALLSTACK
    /* we need something to cross-check against :) */
    assert(qemu_loglevel_mask(EOS_LOG_CALLSTACK));

    int max_depth = eos_callstack_get_caller_param(s, 0, CALL_DEPTH);
    int d = 1;
    int error = 0;
#endif

    /* rebuild the call stack trace by walking the stack */
    while (1)
    {
        lr = find_caller(s, lr, &sp);

        if (sp >= stack_top)
        {
            /* finished */
#ifdef BKT_TRACK_STATS
            stacks_completed++;
#endif
            break;
        }

        if (!lr)
        {
            /* gave up */
#ifdef BKT_TRACK_STATS
            if (i > 1)
            {
                /* at least 1 caller identified? */
                stacks_incomplete++;
            }
            else
            {
                stacks_gaveup++;
            }
#endif
            break;
        }

#ifdef BKT_CROSSCHECK_CALLSTACK
        if (d >= max_depth)
        {
            error = 1;
        }
        else
        {
            uint32_t expected_lr = eos_callstack_get_caller_param(s, d++, CALL_LOCATION) + 4;
            if (lr != expected_lr)
            {
                error = 1;
            }
        }
#endif

        /* store the result */
        lrs[i] = lr;
        sps[i] = sp;
        i++;
    }

#ifdef BKT_CROSSCHECK_CALLSTACK
    if (error)
    {
        /* print some info to diagnose the error */
        eos_callstack_print_verbose(s);
        uint32_t lr = CURRENT_CPU->env.regs[14];
        uint32_t sp = CURRENT_CPU->env.regs[13];
        d = 1;
        while (sp < stack_top)
        {
            lr = find_caller(s, lr, &sp);
            if (sp >= stack_top) break;
            if (!lr) break;            
            uint32_t expected_lr = eos_callstack_get_caller_param(s, d++, CALL_LOCATION) + 4;
            printf("lr %x exp %x %s\n", lr, expected_lr, lr == expected_lr ? "" : "!!!");
        }
        assert(!error);
    }
#endif

#ifdef BKT_CROSSCHECK_CALLSTACK
    if (0)  /* disable output during self tests */
#endif
    {
        int len = fprintf(stderr, "Current stack: [%x-%x] sp=%x lr=%x", stack_top, stack_bot, sps[0], lrs[0]);
        len += eos_indent(len, 80);
        len += eos_print_location(s, pc, lrs[0], "  @ ", "\n");

        /* now go backwards and print the stack trace */
        int depth = 0;
        while (--i >= 0)
        {
            int len = eos_indent(0, depth++);
            len += fprintf(stderr, "%s ", called_func(lrs[i] - 4));
            len += eos_indent(len, 80);
            len += eos_print_location(s, lrs[i] - 4, sps[i], "  @ ", " (pc:sp)\n");
        }
    }

#ifdef BKT_TRACK_STATS
    if (rand() % 1000 == 4)
    {
        fprintf(stderr, "stacks ok:%d incomplete:%d gaveup:%d; callers ok:%d gaveup: %d (loop) %d (insn) randomhelped:%d\n", stacks_completed, stacks_incomplete, stacks_gaveup, callers_found, gaveups_loop, gaveups_insn, random_helped);
    }
#endif
}

/* this serves as unit test, to check whether instructions
 * are emulated correctly in sim_instr (regarding SP, LR and PC)
 * to enable: define BKT_CROSSCHECK_EXEC in backtrace.h
 * and run with e.g. -d callstack */
void eos_bkt_log_exec(EOSState *s)
{
    static uint32_t prev_pc = 0;
    static uint32_t prev_lr = 0;
    static uint32_t prev_sp = 0;
    uint32_t pc = CURRENT_CPU->env.regs[15];
    uint32_t lr = CURRENT_CPU->env.regs[14];
    uint32_t sp = CURRENT_CPU->env.regs[13];

    if (pc == prev_pc) return;
    if (pc == 0x18) goto end;
    if (sp != prev_sp || lr != prev_lr)
    {
        /* does our simulation match QEMU's? */
        uint32_t insn = MEM(prev_pc);
        if ((insn & 0xF0000000) != 0xE0000000)
        {
            /* conditional instruction - ignore for now */
            goto end;
        }
        uint32_t spc = prev_pc;
        uint32_t slr = prev_lr;
        uint32_t ssp = prev_sp;

#ifdef BKT_RANDOM_BRANCHES
        uint32_t scond = 0xE0000000;
        int ret = sim_instr(s, &spc, &slr, &ssp, &scond);
#else
        int ret = sim_instr(s, &spc, &slr, &ssp);
#endif
        if (ret != -1)
        {
            if (ssp != sp)
            {
                fprintf(stderr, "SP mismatch: expected %x->%x, got %x (=> %x at %x,%x)\n", prev_sp, sp, ssp, ret, prev_pc, pc);
                CPUARMState *env = &(CURRENT_CPU->env);
                target_disas(stderr, CPU(arm_env_get_cpu(env)), prev_pc, 4, 0);
                assert(0);
            }

            if (slr != lr)
            {
                if (slr == 0)
                {
                    /* LR invalidated (unhandled) */
                    goto end;
                }
                uint32_t insn = MEM(prev_pc);
                if ((insn & 0x0F000000) == 0x0B000000 ||    /* BL{cond} */
                    (insn & 0x0FFFFFF0) == 0x012fff30)      /* BLX{cond} Rn */
                {
                    /* we just skip the call and assume proper ABI behavior */
                    goto end;
                }
                fprintf(stderr, "LR mismatch: expected %x->%x, got %x (=> %x at %x,%x)\n", prev_lr, lr, slr, ret, prev_pc, pc);
                CPUARMState *env = &(CURRENT_CPU->env);
                target_disas(stderr, CPU(arm_env_get_cpu(env)), prev_pc, 4, 0);
                //assert(0);
            }
        }
    }
end:
    prev_pc = pc;
    prev_lr = lr;
    prev_sp = sp;
}
