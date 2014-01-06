/**
 * fpu_emu: Emulate ARM FPA code in software, uses illegal instruction trap
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <tasks.h>
#include <config.h>

/* this file uses FPA11 structure, so include the definition */
#include <nwfpe/fpa11.h>

#define FPU_EMU_STACK_SIZE 512
#define FPU_EMU_MAX_TASKS 256


/* the stack pointer is defined in assembly code, declare it here */
extern uint32_t fpu_emu_stack;

/* this buffer stores both stack and the context of the trapping task */
uint32_t fpu_emu_context_buffer[FPU_EMU_STACK_SIZE + 18 /* context */];
void (*fpu_emu_orig_undef_handler)(void) = NULL;
uint32_t fpu_emu_undef_handler_addr = 0;

/* pointer to the trapping task ctx, will point into fpu_emu_context_buffer */
uint32_t *fpu_emu_current_ctx = NULL;

/* counters and task information */
uint32_t fpu_emu_call_count = 0;
uint32_t fpu_emu_error_count = 0;
uint32_t fpu_emu_signal_count = 0;
uint32_t fpu_emu_trap_addr = 0;
int32_t fpu_emu_trap_task = 0;

/* for every task we have a separate FPU register set */
static FPA11 *fpu_emu_fp_ctx[FPU_EMU_MAX_TASKS];



/* this is the C code of our undefined instr handler. it may alter registers given in ctx
    context save format:
    [R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 SP LR PC CPSR]
*/
void fpu_emu_handler(uint32_t *ctx)
{
    fpu_emu_trap_addr = ctx[15];
    fpu_emu_trap_task = get_current_task() & 0xFF;
    uint32_t opcode = MEM(fpu_emu_trap_addr);
    
    /* store task context */
    fpu_emu_current_ctx = ctx;
    
    /* check condition (NE, LT, ...) */
    if(checkCondition(opcode, ctx[16]))
    {
        if(!EmulateAll(opcode))
        {
            fpu_emu_error_count++;
        }
        else
        {
            fpu_emu_call_count++;
        }
    }
    ctx[15] += 4;
    
    /* we could try the next instruction for a massive speedup, but play safe for now */
    
    fpu_emu_current_ctx = NULL;
}

void __attribute__((naked)) fpu_emu_undef_handler()
{
    /* undefined instruction exception occurred. switch stacks, call handler and return */
    asm(
        /* keep space for CPSR */
        "LDR     SP, fpu_emu_stack\n"
        "SUB     SP, #0x04\n"

        /* then store PC */
        "SUB     LR, #0x04\n"
        "STMFD   SP!, {LR}\n"

        /* then LR and SP - but this is not possible yet, so just decrease SP */
        "SUB     SP, #0x08\n"

        /* then store shared registers */
        "STMFD   SP!, {R0-R12}\n"

        /* get R15(PC) and SPSR(CPSR_last) */
        "MRS     R3, SPSR\n"

        /* switch back to last mode, disbling IRQ/FIQ and get SP and LR */
        "ORR     R5, R3, #0xC0\n"
        "MRS     R6, CPSR\n"
        "MSR     CPSR_cxsf, R5\n"
        "MOV     R0, SP\n"
        "MOV     R1, LR\n"
        "MSR     CPSR_cxsf, R6\n"

        /* and store SP, LR and then CPSR  */
        "ADD     R4, SP, #0x3C\n"
        "STMFD   R4, {R0-R1}\n"
        "ADD     R4, SP, #0x44\n"
        "STMFD   R4, {R3}\n"

        "MOV     R0, SP\n"
        "BL      fpu_emu_handler\n"

        /* all the way back. get CPSR */
        "ADD     R4, SP, #0x40\n"
        "LDMFD   R4, {R3}\n"

        /* restore SP and LR */
        "ADD     R4, SP, #0x34\n"
        "LDMFD   R4, {R0-R1}\n"

        /* switch back to last mode, disbling IRQ/FIQ and set SP and LR */
        "ORR     R5, R3, #0xC0\n"
        "MRS     R6, CPSR\n"
        "MSR     CPSR_cxsf, R5\n"
        "MOV     SP, R0\n"
        "MOV     LR, R1\n"
        "MSR     CPSR_cxsf, R6\n"

        /* restore SPSR */
        "MSR     SPSR_cxsf, R3\n"
        "LDMFD   SP!, {R0-R12}\n"

        /* skip SP and LR, get PC then skip CPSR again as these are restored alredy */
        "ADD     SP, #0x08\n"
        "LDMFD   SP!, {LR}\n"
        "ADD     SP, #0x04\n"

        /* jump back */
        "MOVS    PC, LR\n"

        "fpu_emu_stack:\n"
        ".word 0x00000000\n"
    );
}

static uint32_t fpu_emu_install()
{
    uint32_t undef_handler_opcode = *(uint32_t*)0x04;
    
    /* make sure abort handler address is loaded into PC using PC relative LDR */
    if((undef_handler_opcode & 0xFFFFF000) != 0xE59FF000)
    {
        bmp_printf(FONT(FONT_MED, COLOR_RED, COLOR_BLACK), 0, 0, "fpu_emu: Failed to find hook point");
        return -1;
    }
    
    /* extract offset from LDR */
    fpu_emu_undef_handler_addr = (undef_handler_opcode & 0x00000FFF) + 0x04 + 0x08;
    
    /* first install stack etc */
    fpu_emu_stack = (uint32_t)&fpu_emu_context_buffer[FPU_EMU_STACK_SIZE + 17];
    fpu_emu_context_buffer[17] = 0xDEADBEEF;

    /* then patch handler */
    fpu_emu_orig_undef_handler = (void*)MEM(fpu_emu_undef_handler_addr);
    MEM(fpu_emu_undef_handler_addr) = (uint32_t)&fpu_emu_undef_handler;
    
    if(MEM(fpu_emu_undef_handler_addr) != (uint32_t)&fpu_emu_undef_handler)
    {
        bmp_printf(FONT(FONT_MED, COLOR_RED, COLOR_BLACK), 0, 0, "fpu_emu: Failed to install trap handler");
        return -1;
    }

    return 0;
}

static uint32_t fpu_emu_uninstall()
{
    /* just remove undefined instruction trap pointer */
    MEM(fpu_emu_undef_handler_addr) = (uint32_t)fpu_emu_orig_undef_handler;

    return 0;
}

static MENU_UPDATE_FUNC(fpu_emu_update_count)
{
    MENU_SET_VALUE("OK:%d S:%d E:%d", fpu_emu_call_count, fpu_emu_signal_count, fpu_emu_error_count);
}

static MENU_UPDATE_FUNC(fpu_emu_update_addr)
{
    MENU_SET_VALUE("0x%08X", fpu_emu_trap_addr);
}

static MENU_UPDATE_FUNC(fpu_emu_update_task)
{
    if(fpu_emu_trap_task)
    {
        char *name = (char*)get_task_name_from_id(fpu_emu_trap_task);
        MENU_SET_VALUE("%d: %s", fpu_emu_trap_task, name);
    }
    else
    {
        MENU_SET_VALUE("(none)");
    }
}


static uint32_t __attribute__((naked)) fpu_emu_test(uint32_t a, uint32_t b)
{
    asm("\
        FLTS F0, R0;\
        FLTS F1, R1;\
        FLTS F2, R1;\
        \
        ADFS F0, F0, F1;\
        MUFS F0, F2, F0;\
        MUFS F0, F2, F0;\
        DVFS F0, F0, #5;\
        \
        FIX R0, F0;\
        MOV PC, LR;\
    ");
}

static MENU_SELECT_FUNC(fpu_emu_select)
{
    uint32_t ret = fpu_emu_test(40, 50);
    
    if(ret == 45000)
    {
        NotifyBox(2000, "FPU Emulation confirmed working");
    }
    else
    {
        NotifyBox(2000, "FPU Emulation returned invalid value");
    }
}


static struct menu_entry fpu_emu_menu[] =
{
    {
        .name = "FPU Emu",
        .select = menu_open_submenu,
        .children = (struct menu_entry[])
        {
            {
                .name = "Test FPU",
                .update = fpu_emu_update_count,
                .select = fpu_emu_select,
            },
            {
                .name = "Last PC",
                .update = fpu_emu_update_addr,
            },
            {
                .name = "Task",
                .update = fpu_emu_update_task,
            },
            MENU_EOL,
        }
    }
};

static unsigned int fpu_emu_init()
{
    /* initialize FPU states for every possible task */
    for(int pos = 0; pos < COUNT(fpu_emu_fp_ctx); pos++)
    {
        fpu_emu_fp_ctx[pos] = malloc(sizeof(FPA11));
        nwfpe_init_fpa(fpu_emu_fp_ctx[pos]);
    }
    
    fpu_emu_install();
    
    menu_add("Debug", fpu_emu_menu, COUNT(fpu_emu_menu));
    
    return 0;
}

static unsigned int fpu_emu_deinit()
{
    fpu_emu_uninstall();
    return 0;
}

/* accessor functions for NWFPE code to read/write tasks CPU registers */
uint32_t fpu_emu_read_reg(uint32_t reg)
{
    return fpu_emu_current_ctx[reg];
}

void fpu_emu_write_reg(uint32_t reg, uint32_t val)
{
    fpu_emu_current_ctx[reg] = val;
}

/* this function must return a pointer to the current tasks FPU state */
FPA11 *fpu_emu_get_fpa11()
{
    return fpu_emu_fp_ctx[fpu_emu_trap_task];
}


/* the libraries were designed for linux with MMU systems where user memory maps differently
   and is not directly accessible from kernel context. not needed for our MMU-less system */
void put_user(uint32_t val, void *addr) 
{
    MEM(addr) = val;
}

void get_user(uint32_t *val, void *addr) 
{
    *val = MEM(addr);
}

/* just count the signals, but dont handle yet. do we need signalling? */
void float_raise(signed char flags)
{
    fpu_emu_signal_count++;
}

/* those are missing, i hope they are correct */
void __attribute__((naked)) __ashldi3()
{
    asm("\
        subs    r3, r2, #32          ;\
        rsb     r12, r2, #32         ;\
        movmi   r1, r1, lsl r2       ;\
        movpl   r1, r0, lsl r3       ;\
        orrmi   r1, r1, r0, lsr r12  ;\
        mov     r0, r0, lsl r2       ;\
        mov     pc, lr               ;\
    ");
}

void __attribute__((naked)) __lshrdi3()
{
    asm("\
        subs    r3, r2, #32          ;\
        rsb     r12, r2, #32         ;\
        movmi   r0, r0, lsr r2       ;\
        movpl   r0, r1, lsr r3       ;\
        orrmi   r0, r0, r1, lsl r12  ;\
        mov     r1, r1, lsr r2       ;\
        mov     pc, lr               ;\
    ");
}

MODULE_INFO_START()
    MODULE_INIT(fpu_emu_init)
    MODULE_DEINIT(fpu_emu_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

MODULE_CONFIGS_START()
MODULE_CONFIGS_END()
