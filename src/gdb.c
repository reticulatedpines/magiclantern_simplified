
/*
   GDB stub for Magic Lantern on ARM946 CPU in Canon EOS DSLRs
   Contains some code ported over from rockbox project
*/



#include "dryos.h"
#include "gdb.h"
#include "cache_hacks.h"
#include "bmp.h"


struct task_attr_str {
  unsigned int entry;
  unsigned int args;
  unsigned int stack;
  unsigned int size;
  unsigned int used; // 0x10
  void* name;
  unsigned int off_18;
  unsigned int flags;
  unsigned char wait_id;
  unsigned char pri;
  unsigned char state;
  unsigned char fpu;
  unsigned int id;
}; // size = 0x28



extern unsigned int task_max;
extern int is_taskid_valid(int, int, void*);
extern void (*pre_task_hook) (void);
extern void (*post_task_hook)(uint32_t *,uint32_t *,uint32_t *);
extern void (*pre_isr_hook)  (uint32_t);
extern void (*post_isr_hook) (uint32_t);
extern uint32_t *current_task_ctx;


int gdb_attached_pid = -1;
volatile int gdb_running = 0;
int gdb_installed = 0;
breakpoint_t *gdb_current_bkpt = NULL;

/* used for gdb_get_callstack */
char gdb_callstack[256];

breakpoint_t gdb_breakpoints[GDB_BKPT_COUNT];
uint32_t gdb_exceptions_handled = 0;
uint32_t gdb_context_buffer[GDB_STACK_SIZE + 18 /* context */];
void (*gdb_orig_undef_handler)(void) = 0;


void gdb_main_task(void);

#if defined(CONFIG_GDBSTUB)
uint32_t gdb_recv_buffer_length = 0;
uint32_t gdb_send_buffer_length = 0;
uint8_t gdb_recv_buffer[GDB_TRANSMIT_BUFFER_SIZE];
uint8_t gdb_send_buffer[GDB_TRANSMIT_BUFFER_SIZE];
char *gdb_send_ptr = NULL;
#endif


#if defined(USE_HOOKS)
uint32_t gdb_pre_task_hook_cnt = 0;
uint32_t gdb_post_task_hook_cnt = 0;
uint32_t gdb_pre_isr_hook_cnt = 0;
uint32_t gdb_post_isr_hook_cnt = 0;

void (*orig_pre_task_hook) (void) = 0;
void (*orig_post_task_hook)(uint32_t *,uint32_t *,uint32_t *) = 0;
void (*orig_pre_isr_hook)  (uint32_t) = 0;
void (*orig_post_isr_hook) (uint32_t) = 0;
#endif


extern uint32_t gdb_undef_stack;
void gdb_undef_handler(void);

#if defined(CONFIG_GDBSTUB)
void gdb_task_stall(void);
void gdb_task_resume(void);

asm(
    ".globl gdb_task_stall\n"
    "gdb_task_stall:\n"

    "gdb_task_stall_loop:\n"
    "MOV     R0, #0x64\n"
#if !defined(POSITION_INDEPENDENT)
    "BL      msleep\n"
#endif
    "LDR     R0, [R7, #0x00]\n"
    "CMP     R0, #0x01\n"
    "BNE     gdb_task_stall_loop\n"

    ".globl gdb_task_resume\n"
    "gdb_task_resume:\n"
    ".word   0xe7ffdefe\n"
    
    "gdb_task_stall_lockup:\n"
    "B       gdb_task_stall_lockup\n"
    
);
#endif
    
asm(
    ".globl gdb_undef_handler\n"
    "gdb_undef_handler:\n"

    /* keep space for CPSR */
    "LDR     SP, gdb_undef_stack\n"
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
    "BL      gdb_exception_handler\n"

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

    "gdb_undef_stack:\n"
    ".word 0x00000000\n"
);

uint32_t gdb_strlen(char *str)
{
    uint32_t length = 0;
    
    while(str[length])
    {
        length++;
    }
    return length;
}

char *gdb_strncpy(char *dst, char *src, uint32_t length)
{
    uint32_t pos = 0;
    
    while(pos < length)
    {
        dst[pos] = src[pos];
        if(!src[pos])
        {
            return dst;
        }
        pos++;
    }
    dst[pos] = 0;
    
    return dst;
}

char *gdb_strncat(char *dst, char *src, uint32_t length)
{
    uint32_t pos = gdb_strlen(dst);
    
    gdb_strncpy(&dst[pos], src, length);
    
    return dst;
}

char *gdb_strlcat(char *dst, char *src, uint32_t length)
{
    uint32_t pos = gdb_strlen(dst);
    
    gdb_strncpy(&dst[pos], src, length - pos);
    
    return dst;
}

void *gdb_memcpy(void *dst, void *src, uint32_t length)
{
    while(length--)
    {
        ((uint8_t*)dst)[length] = ((uint8_t*)src)[length];
    }
    
    return dst;
}

void *gdb_memset(void *dst, uint8_t val, uint32_t length)
{
    while(length--)
    {
        ((uint8_t*)dst)[length] = val;
    }
    
    return dst;
}

/* tell if given opcode is a "clean" BL (ret=1) or some branch/BX/MOV (ret=2) or if it is not PC-modifying (ret=0) */
uint32_t gdb_instr_is_pc_modifying(uint32_t opcode)
{
    /* check if it was a BL 0x.. */
    if((opcode & 0x0F000000) == 0x0B000000)
    {
        return 1;
    }
    
    /* check if it was a B 0x.. */
    if((opcode & 0x0E000000) == 0x0A000000)
    {
        return 2;
    }
    
    /* check if it was smth like a MOV R15,... */
    if((opcode & 0x0E0F0010) == 0x000F0000)
    {
        return 2;
    }
    if((opcode & 0x0E0F0090) == 0x000F0010)
    {
        return 2;
    }
    
    /* check if it was smth like a BX R.. */
    if((opcode& 0x0FF000F0) == 0x01200030)
    {
        return 2;
    }
    
    /* load with immediate offset */
    if((opcode & 0x0E10F000) == 0x0410F000)
    {
        return 2;
    }
    
    /* load with register offset */
    if((opcode & 0x0E10F010) == 0x0610F000)
    {
        return 2;
    }
    
    /* load multiple with R15 set */
    if((opcode & 0x0E108000) == 0x08108000)
    {
        return 2;
    }

    return 0;
}

char *gdb_get_callstack(breakpoint_t *bkpt)
{
    uint32_t *sp = (uint32_t*)bkpt->ctx[13];
    uint32_t pos = 1;
    uint32_t depth = 0;
    char tmpBuf[9];
    
    /* gdb_word2hexword will write the first 8 byte */
    tmpBuf[8] = 0;
    gdb_strncpy(gdb_callstack, "BL: ", GDB_STRING_BUFFER_SIZE);
    
    while(depth < 5 && pos < 100)
    {
        uint32_t lr = sp[pos];
        
        /* address in ROM? -> might be a LR */
        if((lr & 0xF8000000) == 0xF8000000)
        {
            uint32_t *pc = (uint32_t*)(lr - 4);
            
            /* check if opcode before LR position was a BL/B/BX/... */
            if(gdb_instr_is_pc_modifying(*pc))
            {
                /* yes, so it might have been our caller */
                gdb_word2hexword(tmpBuf, (uint32_t)pc);
                
                /* mark BX/B with a dot, BLs without one */
                if(gdb_instr_is_pc_modifying(*pc) != 1)
                {
                    gdb_strlcat(gdb_callstack, ".", GDB_STRING_BUFFER_SIZE);
                }
                else
                {
                    gdb_strlcat(gdb_callstack, " ", GDB_STRING_BUFFER_SIZE);
                }
                gdb_strlcat(gdb_callstack, tmpBuf, GDB_STRING_BUFFER_SIZE);
                gdb_strlcat(gdb_callstack, " ", GDB_STRING_BUFFER_SIZE);
                depth++;
            }
        }
        pos++;        
    }
    
    return gdb_callstack;
}


#if defined(GDB_TASK_CTX)
struct task *gdb_get_current_task()
{
    return (struct task *)((uint32_t) current_task_ctx - 0x50);
}
#endif

#if defined(GDB_USE_HOOKS)

void gdb_pre_task_hook()
{
    gdb_pre_task_hook_cnt++;
    if(orig_pre_task_hook)
    {
        orig_pre_task_hook();
    }
}

void gdb_post_task_hook(uint32_t *next_task_ctx, uint32_t *old_task_ctx, uint32_t *active_task_ctx)
{
    struct task *current = (struct task *)((uint32_t) active_task_ctx - 0x50);
    struct task *next = (struct task *)((uint32_t) next_task_ctx - 0x50);
    int pos = 0;

    gdb_post_task_hook_cnt++;
    
    /* wont work yet, i think parameters are incorrect */
#if 0
    /* check if 'current' is a debugged task and unarm all enabled breakpoints */
    if(current->taskId == gdb_attached_pid)
    {
        for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
        {
            if(gdb_breakpoints[pos].flags & GDB_BKPT_FLAG_ARMED)
            {
                gdb_unarm_bkpt(&gdb_breakpoints[pos]);
            }
        }
    }
    /* check if 'next' is a debugged task and arm all enabled breakpoints */
    if(next->taskId == gdb_attached_pid)
    {
        for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
        {
            if(gdb_breakpoints[pos].flags & GDB_BKPT_FLAG_ENABLED)
            {
                gdb_arm_bkpt(&gdb_breakpoints[pos]);
            }
        }
    }
#endif
    if(orig_post_task_hook)
    {
        orig_post_task_hook(next_task_ctx, old_task_ctx, active_task_ctx);
    }
}

void gdb_post_isr_hook(uint32_t isrId)
{
    gdb_pre_isr_hook_cnt++;
    if(orig_pre_isr_hook)
    {
        orig_pre_isr_hook(isrId);
    }
}

void gdb_pre_isr_hook(uint32_t isrId)
{
    gdb_post_isr_hook_cnt++;
    if(orig_post_isr_hook)
    {
        orig_post_isr_hook(isrId);
    }
}
#endif

/* find the corresponding breakpoint struct for an address, return NULL if there is none */
breakpoint_t *gdb_find_bkpt(uint32_t address)
{
    int pos = 0;

    for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
    {
        if(gdb_breakpoints[pos].address == address)
        {
            return &gdb_breakpoints[pos];
        }
    }

    return NULL;
}

/*  */
breakpoint_t *gdb_find_stalled_bkpt()
{
    int pos = 0;

    for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
    {
        if(gdb_breakpoints[pos].flags & GDB_BKPT_FLAG_TASK_STALLED)
        {
            return &gdb_breakpoints[pos];
        }
    }

    return NULL;
}

/* delete breakpoint (and watchpoint) */
void gdb_delete_bkpt(breakpoint_t *bkpt)
{
    gdb_unarm_bkpt(bkpt);
    if(bkpt->linkId != GDB_LINK_NONE)
    {
        breakpoint_t *link = &gdb_breakpoints[bkpt->linkId];
            
        /* and arm linked watchpoint */
        gdb_unarm_bkpt(link);
        link->flags &= ~GDB_BKPT_FLAG_ENABLED;
        link->linkId = GDB_LINK_NONE;
    }
    bkpt->flags &= ~GDB_BKPT_FLAG_ENABLED;
    bkpt->linkId = GDB_LINK_NONE;
}

/* remove breakpoint instruction (unarm) and set flags accordingly */
void gdb_unarm_bkpt(breakpoint_t *bkpt)
{
    cache_fake(bkpt->address, MEM(bkpt->address), TYPE_ICACHE);
    bkpt->flags &= ~GDB_BKPT_FLAG_ARMED;
}

/* place breakpoint instruction (unarm) and set flags accordingly */
void gdb_arm_bkpt(breakpoint_t *bkpt)
{
    cache_fake(bkpt->address, GDB_BKPT_OPCODE, TYPE_ICACHE);
    bkpt->flags |= GDB_BKPT_FLAG_ARMED;
}

extern struct task *task_queue_head;

/* test function to resume task at address */
void gdb_continue_stalled_task(uint32_t address)
{
    int pos = 0;

    for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
    {
        if(gdb_breakpoints[pos].address == address)
        {
            if(gdb_breakpoints[pos].flags & GDB_BKPT_FLAG_ARMED)
            {
                gdb_unarm_bkpt(&gdb_breakpoints[pos]);
            }

            if(gdb_breakpoints[pos].taskStruct != NULL)
            {
                /* mark this breakpoint to be resumed */
                gdb_breakpoints[pos].flags |= GDB_BKPT_FLAG_RESUME;
                gdb_breakpoints[pos].unStall = 1;

                //uint32_t queue_pos = 0;
                //struct task **task_queue = task_queue_head;
                //
                ///* find a free slot in the task list */
                //while(task_queue[queue_pos] != NULL)
                //{
                //    queue_pos++;
                //}
                //
                ///* okay then restore the task state */
                //gdb_breakpoints[pos].taskStruct->currentState = gdb_breakpoints[pos].taskState;
                //gdb_breakpoints[pos].taskStruct->yieldRequest = 0;
                /* and finally requeue task so it will resume */
                //task_queue[queue_pos] = gdb_breakpoints[pos].taskStruct;
            }
        }
    }
}


/* thats a "quick" function to be called via ptp and its base address
 * check the linker .map file for the base address and call it using ptpcam with one parameter - the address to look at
 */
breakpoint_t *gdb_quick_watchpoint(uint32_t address)
{
    return gdb_add_watchpoint(address, 0, NULL);
}

/* thats a "quick" function to be called via ptp and its base address.
 * check the linker .map file for the base address and call it using ptpcam
 */
uint32_t gdb_quick_clear()
{
    for(uint32_t pos = 0; pos < GDB_BKPT_COUNT; pos++)
    {
        if(gdb_breakpoints[pos].flags & GDB_BKPT_FLAG_ENABLED)
        {
            gdb_unarm_bkpt(&gdb_breakpoints[pos]);
            gdb_breakpoints[pos].flags &= ~GDB_BKPT_FLAG_ENABLED;
        }
    }
    
    return 0;
}

/* add a watchpoint at given address. this watchpoint is triggered each time, the processor is going to
   execute the opcode at given address.
   
   address:
       the address to shortly interrupt execution at, capture registers and continue
   linkAddress:
       the address of preferrably the next instruction in execution flow.
       set to 0 to autmatically select instruction. (first parameter must not be on a branch instruction then)
   callback:
       if provided, this function will get called every time the watchpoint is triggered
*/
breakpoint_t *gdb_add_watchpoint(uint32_t address, uint32_t linkedAddress, void (*callback)(breakpoint_t *))
{
    breakpoint_t *bp1 = NULL;
    breakpoint_t *bp2 = NULL;
    
    /* no link address given? try to use next instruction */
    if(linkedAddress == 0)
    {
        /* make sure the watchpoint is not on some branch or similar */
        if(gdb_instr_is_pc_modifying(*(uint32_t*)address))
        {
            return NULL;
        }
        linkedAddress = address + 4;
    }    
    
    /* create watchpoints, but only enable first one. just in case some jump goes directly to second one */
    bp1 = gdb_add_bkpt(address, GDB_BKPT_FLAG_ARMED | GDB_BKPT_FLAG_WATCHPOINT);
    bp2 = gdb_add_bkpt(linkedAddress, GDB_BKPT_FLAG_WATCHPOINT);
    
    /* link those two watchpoints */
    bp1->linkId = bp2->id;
    bp2->linkId = bp1->id;
    
    /* mark this one as the link watchpoint - this one is dedicated for enabling the first one again */
    bp2->flags |= GDB_BKPT_FLAG_WATCHPOINT_LINK;
    
    /* and set callback, if provided */
    bp1->callback = (void (*)(void *)) callback;
    
    return bp1;
}

/* add or update a breakpoint at given address. valid flags are
   GDB_BKPT_FLAG_WATCHPOINT (capture registers, unarm and continue execution)
   GDB_BKPT_FLAG_ARMED (already arm this break/watchpoint
*/
breakpoint_t * gdb_add_bkpt(uint32_t address, uint32_t flags)
{
    int pos = 0;

    for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
    {
        if((gdb_breakpoints[pos].address == address) || ((gdb_breakpoints[pos].flags & (GDB_BKPT_FLAG_ENABLED | GDB_BKPT_FLAG_TASK_STALLED)) == 0) )
        {
            /* still armed? */
            if(gdb_breakpoints[pos].flags & GDB_BKPT_FLAG_ARMED)
            {
                gdb_unarm_bkpt(&gdb_breakpoints[pos]);
            }

            gdb_breakpoints[pos].id = pos;
            gdb_breakpoints[pos].linkId = GDB_LINK_NONE;
            gdb_breakpoints[pos].address = address;
            gdb_breakpoints[pos].flags = GDB_BKPT_FLAG_ENABLED;
            gdb_breakpoints[pos].hitcount = 0;
            gdb_breakpoints[pos].taskStruct = NULL;
            gdb_breakpoints[pos].taskState = 0;
            gdb_breakpoints[pos].unStall = 0;
            gdb_breakpoints[pos].callback = 0;

            if(flags & GDB_BKPT_FLAG_WATCHPOINT)
            {
                gdb_breakpoints[pos].flags |= GDB_BKPT_FLAG_WATCHPOINT;
            }

            if(flags & GDB_BKPT_FLAG_ARMED)
            {
                gdb_arm_bkpt(&gdb_breakpoints[pos]);
            }

            return &gdb_breakpoints[pos];
        }
    }

    return NULL;
}



/* this is the C code of our undefined instruction handler. it may alter registers given in ctx
    context save format:
    [R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 SP LR PC CPSR]
*/
void gdb_exception_handler(uint32_t *ctx)
{
#if defined(CONFIG_GDBSTUB)
    /* was this the "resume" breakpoint in stall function? */
    if(ctx[15] == ((uint32_t)&gdb_task_resume))
    {
        /* this is a task that got re-scheduled. check if we set its state to "running" */
        breakpoint_t *bkpt = (breakpoint_t *)ctx[5];
        struct task *task = (struct task *)ctx[6];

        gdb_exceptions_handled += 0x0100;

        if(ctx[4] != GDB_REG_STALL_MAGIC)
        {
            /* sanity check failed. that task was not stalled by us. stall again */
            gdb_exceptions_handled |= 0xE1000000;
            ctx[15] = (uint32_t)&gdb_task_stall;
            return;
        }

        if(bkpt->taskStruct != task)
        {
            /* sanity check failed. that task was not stalled by us. stall again */
            gdb_exceptions_handled |= 0xE2000000;
            ctx[15] = (uint32_t)&gdb_task_stall;
            return;
        }

#if 0
        if(task->currentState == GDB_TASK_STATE_STALL)
        {
            /* sanity check failed. that task was not rescheduled by us. stall again */
            gdb_exceptions_handled |= 0xE4000000;
            ctx[15] = (uint32_t)&gdb_task_stall;
            return;
        }
#endif

        if((bkpt->flags & GDB_BKPT_FLAG_RESUME) == 0)
        {
            /* sanity check failed. that task was not rescheduled by us. stall again */
            gdb_exceptions_handled |= 0xE8000000;
            ctx[15] = (uint32_t)&gdb_task_stall;
            return;
        }

        gdb_exceptions_handled |= 0x10000000;

        /* restore original context */
        gdb_memcpy(ctx, bkpt->ctx, 17*4);

        /* make sure there is no BP instruction at resume point */
        gdb_unarm_bkpt(bkpt);

        /* update flags */
        bkpt->flags &= ~GDB_BKPT_FLAG_RESUME;
        bkpt->unStall = 0;

        /* this task should run now again at the position where it stopped. hopefully. */
    }
    else
#endif
    {
        breakpoint_t *bkpt = gdb_find_bkpt(ctx[15]);

        if(!bkpt)
        {
            /* uhmm. we did not place that breakpoint there - for now just skip the instruction */
            ctx[15] += 4;
            /* well, this would dump some wrong information, but should kill the offending task. some alternative */
            //gdb_orig_undef_handler();
        }
        else if(bkpt->flags & GDB_BKPT_FLAG_WATCHPOINT)
        {
            bkpt->hitcount++;

            gdb_exceptions_handled += 0x0001;

            /* this is a watchpoint, backup context */
            gdb_memcpy(bkpt->ctx, ctx, 17*4);

            /* mark as reached */
            bkpt->flags |= GDB_BKPT_FLAG_WATCHPOINT_REACHED;

            /* and unarm watchpoint */
            gdb_unarm_bkpt(bkpt);
            
            if(bkpt->callback)
            {
                bkpt->callback(bkpt);
            }
            
            if(bkpt->linkId != GDB_LINK_NONE)
            {
                breakpoint_t *link = &gdb_breakpoints[bkpt->linkId];
                    
                /* and arm linked watchpoint */
                gdb_arm_bkpt(link);
            }
        }
#if defined(CONFIG_GDBSTUB)
        else
        {
            gdb_exceptions_handled += 0x0001;
            bkpt->hitcount++;

            /* backup original context into breakpoint data */
            gdb_memcpy(bkpt->ctx, ctx, 17*4);

            /* were interrupts disabled? this would cause problems as the task cannot sleep */
            if(ctx[16] & 0xC0)
            {
                gdb_exceptions_handled |= 0x00800000;
                return;
            }

            /* mark as reached */
            bkpt->flags |= GDB_BKPT_FLAG_TASK_STALLED;
            
#if defined(GDB_TASK_CTX)
            /* we reached a real breakpoint. we should stall the task and inform the debugger about that event */
            struct task *current = gdb_get_current_task();

            bkpt->taskStruct = current;
            bkpt->taskState = current->currentState;
            
            /* ToDo: not supported yet - first breakpoint should decide PID to debug */
            if(gdb_attached_pid < 0)
            {
                gdb_attached_pid = current->taskId;
            }
#endif

            /* set flag being checked in stalling code */
            bkpt->unStall = 0;
            
            /* make the task lock, next task will get activated with timer interrupt */
            ctx[4] = GDB_REG_STALL_MAGIC;
            ctx[5] = (uint32_t)bkpt;
            ctx[6] = (uint32_t)(bkpt->taskStruct);
            ctx[7] = (uint32_t)&(bkpt->unStall);
            ctx[13] = (uint32_t)&(bkpt->tempStack[GDB_STALL_STACK_SIZE]);
            ctx[15] = (uint32_t)&gdb_task_stall;
        }
#endif
    }
}

uint32_t gdb_install_hooks()
{
#ifdef USE_HOOKS
    //orig_pre_task_hook = pre_task_hook;
    orig_post_task_hook = post_task_hook;
    orig_pre_isr_hook = pre_isr_hook;
    orig_post_isr_hook = post_isr_hook;

    //pre_task_hook = gdb_pre_task_hook;
    post_task_hook = gdb_post_task_hook;
    pre_isr_hook = gdb_pre_isr_hook;
    post_isr_hook = gdb_post_isr_hook;
#endif
    return 1;
}

uint32_t gdb_install_handler()
{
    uint32_t undef_handler_addr = 0;
    uint32_t undef_handler_opcode = *(uint32_t*)0x04;
    
    /* make sure undef handler address is loaded into PC using PC relative LDR */
    if((undef_handler_opcode & 0xFFFFF000) != 0xE59FF000)
    {
        return 0;
    }
    
    /* extract offset from LDR */
    undef_handler_addr = (undef_handler_opcode & 0x00000FFF) + 0x04 + 0x08;
    
    /* first install stack etc */
    gdb_undef_stack = (uint32_t)&gdb_context_buffer[GDB_STACK_SIZE + 17];
    gdb_context_buffer[17] = 0xDEADBEEF;

    /* then patch handler */
    gdb_orig_undef_handler = (void*)MEM(undef_handler_addr);
    MEM(undef_handler_addr) = (uint32_t)&gdb_undef_handler;
    
    return 1;
}

uint32_t gdb_setup()
{
    int pos = 0;

    for(pos = 0; pos < GDB_BKPT_COUNT; pos++)
    {
        gdb_breakpoints[pos].flags = 0;
    }

    icache_lock();

#if defined(CONFIG_GDBSTUB)
    gdb_memset(gdb_send_buffer, 0, GDB_TRANSMIT_BUFFER_SIZE);
    gdb_memset(gdb_recv_buffer, 0, GDB_TRANSMIT_BUFFER_SIZE);
    gdb_send_ptr = (char*)gdb_send_buffer;
#endif

    if(!gdb_install_handler())
    {
        return 0;
    }
    
    if(!gdb_install_hooks())
    {
        return 0;
    }
    
#if defined(CONFIG_GDBSTUB)
    task_create("gdbstub_task", 0x1e, 0, gdb_main_task, 0);
#endif
    
    gdb_installed = 1;
    return 1;
}

void gdb_byte2hexbyte(char *s, int byte)
{
    static char hexchars[] = "0123456789abcdef";
    s[0] = hexchars[(byte >> 4) & 0xf];
    s[1] = hexchars[byte & 0xf];
}

int gdb_hexnibble2dec(char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
    {
        return ch - 'a' + 10;
    }
    if ((ch >= '0') && (ch <= '9'))
    {
        return ch - '0';
    }
    if ((ch >= 'A') && (ch <= 'F'))
    {
        return ch - 'A' + 10;
    }
    return -1;
}

void gdb_word2hexword(char *s, uint32_t val)
{
    int i;
    
    for (i = 0; i < 4; i++)
    {
        gdb_byte2hexbyte(&s[i * 2], (val >> ((3-i) * 8)) & 0xff);
    }
}

void gdb_word2hexword_rev(char *s, uint32_t val)
{
    int i;
    
    for (i = 0; i < 4; i++)
    {
        gdb_byte2hexbyte(&s[i * 2], (val >> (i * 8)) & 0xff);
    }
}

int gdb_hexbyte2dec(char *s)
{
    return (gdb_hexnibble2dec(s[0]) << 4) + gdb_hexnibble2dec(s[1]);
}

unsigned long gdb_hexword2dec(char *s)
{
    int i;
    unsigned long r = 0;
    
    for (i = 3; i >= 0; i--)
    {
        r = (r << 8) + gdb_hexbyte2dec(s + i * 2);
    }
    return r;
}

uint32_t gdb_get_hexnumber(char **buffer, uint32_t *value)
{
    uint32_t done = 0;
    uint32_t parsed = 0;
    uint32_t retval = 0;
    char *args = *buffer;

    while(!done)
    {
        int val = gdb_hexnibble2dec(*args);

        if(val >= 0)
        {
            retval <<= 4;
            retval |= val;
            args++;
            parsed++;
        }
        else
        {
            done = 1;
        }
    }

    *value = retval;
    *buffer = args;

    return parsed;
}


#if defined(CONFIG_GDBSTUB)

/* gets called when new data arrived in buffer */
void gdb_recv_callback(uint32_t length)
{
    gdb_recv_buffer_length = length;
}

/* gets called when data from buffer was uploaded to host */
void gdb_send_callback()
{
    /* set the pointer to first payload byte again */
    gdb_memset(gdb_send_buffer, 0, GDB_TRANSMIT_BUFFER_SIZE);
    gdb_send_buffer_length = 0;
    gdb_send_ptr = (char*)gdb_send_buffer;
}


void gdb_send_append(char *buf, uint32_t len)
{
    uint32_t written = 0;

    while(*buf && (written < len))
    {
        *gdb_send_ptr = *buf;
        gdb_send_ptr++;
        buf++;
        written++;
    }

    *gdb_send_ptr = 0;
}

void gdb_send_packet(char *buf)
{
    int done = 0;
    
    do
    {
        int i = 0;
        char tmp[3];
        int checksum = 0;
        
        /* wait for send buffer being empty */
        while(gdb_send_buffer_length != 0)
        {
            msleep(1);
        }    

        gdb_send_append("$", 1);

        checksum = 0;
        for (i = 0; buf[i]; i++)
        {
            checksum += buf[i];
        }

        gdb_send_append(buf, i);

        tmp[0] = '#';
        gdb_byte2hexbyte(tmp + 1, checksum & 0xff);
        gdb_send_append(tmp, 3);

        /* mark for being sent */
        gdb_send_buffer_length = strlen((char*)gdb_send_buffer) + 1;
        
        /* wait for '+' */
        while((gdb_recv_buffer_length == 0) || (gdb_send_buffer_length != 0))
        {
            msleep(1);
        }
        
        if(gdb_recv_buffer[0] == '+')
        {
            done = 1;
        }
        
        /* shift remaining data to left */
        gdb_memcpy(gdb_recv_buffer, &(gdb_recv_buffer[1]), GDB_TRANSMIT_BUFFER_SIZE-1);
        gdb_recv_buffer_length--;
    }
    while(!done);
}

void gdb_get_packet(char *buf, uint32_t len)
{
    uint32_t done = 0;

    /* wait for data and for send buffer being empty */
    while((gdb_recv_buffer_length == 0) || (gdb_send_buffer_length != 0))
    {
        msleep(1);
    }
    
	while (!done)
	{
        uint32_t checksum = 0;
        uint32_t count = 0;
        uint32_t escaped = 0;
        char ch = 0;
        char *recvBuffer = (char *)gdb_recv_buffer;
        
        while (count < len) 
		{
            ch = *recvBuffer;
            recvBuffer++;
            
            if (!escaped) 
			{
                if (ch == '$') 
				{
                    checksum = 0;
                    count = 0;
                } 
				else if (ch == '#')
				{
                    break;
				}
                else if (ch == 0x7d) 
				{
                    escaped = 1;
                    checksum += ch;
                } 
				else 
				{
                    checksum += ch;
                    buf[count] = ch;
                    count++;
                }
            } 
			else 
			{
                escaped = 0;
                checksum += ch;
                buf[count] = ch ^ 0x20;
                count++;
            }
        }
        buf[count] = 0;

        if (ch == '#') 
		{
            int rchksum;

            ch = *recvBuffer;
            recvBuffer++;
            rchksum = gdb_hexnibble2dec(ch) << 4;
            ch = *recvBuffer;
            recvBuffer++;
            rchksum += gdb_hexnibble2dec(ch);

            if ((rchksum > 0) && (checksum & 0xff) != (uint32_t)rchksum)
			{
                /* send immediate response */
                gdb_send_append("-", 1);
                gdb_send_buffer_length = 2;
                
                /* mark as invalid */
                buf[0] = 0;
                return;
			}
            else 
			{
                /* send immediate response */
                gdb_send_append("+", 1);
                gdb_send_buffer_length = 2;
                
                /* mark buffer empty */
                gdb_memset(gdb_recv_buffer, 0, GDB_TRANSMIT_BUFFER_SIZE);
                gdb_recv_buffer_length = 0;
                return;
            }
        }
    }
}


void gdb_reply_error(int n, char *reply)
{
    reply[0] = 'E';
    gdb_byte2hexbyte(reply + 1, n);
    reply[3] = 0;
}

void gdb_reply_signal(int signal, char *reply)
{
    reply[0] = 'S';

    gdb_byte2hexbyte(reply + 1, signal);
    reply[3] = 0;
}

void gdb_reply_ok(char *reply)
{
    gdb_strncpy(reply, "OK", 2);
}

unsigned long gdb_get_general_reg(int n)
{
    if(gdb_current_bkpt)
    {
        return gdb_current_bkpt->ctx[n];
    }
    
    return 0;
}

void gdb_set_general_reg(int n, unsigned long v)
{
    if(gdb_current_bkpt)
    {
        gdb_current_bkpt->ctx[n] = v;
    }
}

unsigned long gdb_get_cpsr(void)
{
    if(gdb_current_bkpt)
    {
        return gdb_current_bkpt->ctx[16];
    }
    
    return 0;
}

void gdb_set_cpsr(unsigned long v)
{
    if(gdb_current_bkpt)
    {
        gdb_current_bkpt->ctx[16] = v;
    }
}

void gdb_cmd_regs_reply(char *buf)
{
    int i;
    char *p;

    p = buf;
    for (i = 0; i < 16; i++) {
        gdb_word2hexword_rev(p, gdb_get_general_reg(i));
        p += 8;
    }

    for (i = 0; i < 8; i++) {
        gdb_memset(p, '0', 16);
        p += 16;
    }

    gdb_word2hexword_rev(p, 0);
    p += 8;
    gdb_word2hexword_rev(p, gdb_get_cpsr());
    p[8] = 0;
}

void gdb_cmd_get_register(char *args, char *reply)
{
    uint32_t r = 0;

    gdb_get_hexnumber(&args, &r);

    if (r < 16)
    {
        gdb_word2hexword_rev(reply, gdb_get_general_reg(r));
        reply[8] = 0;
    }
    else if (r == 25)
    {
        gdb_word2hexword_rev(reply, gdb_get_cpsr());
        reply[8] = 0;
    }
    else
    {
        gdb_word2hexword_rev(reply, 0);
        reply[8] = 0;
    }
}

void gdb_cmd_set_register(char *args, char *reply)
{
    uint32_t r = 0;
    int v = 0;

    gdb_get_hexnumber(&args, &r);

    /* sanity check */
    if(*args != '=')
    {
        gdb_reply_error(0, reply);
        return;
    }

    /* now parse little endian register value */
    args++;
    v = gdb_hexword2dec(args);

    if (r < 16)
    {
        gdb_set_general_reg(r, v);
    }
    else if (r == 25)
    {
        gdb_set_cpsr(v);
    }

    gdb_reply_ok(reply);
}

void gdb_cmd_set_registers(char *args, char *reply) {
    char *p;
    int i, len;

    len = strlen(args);

    p = args;
    for (i = 0; i < 16 && len >= (i + 1) * 8; i++)
    {
        gdb_set_general_reg(i, gdb_hexword2dec(p));
        p += 8;
    }

    if (len >= 16 * 8 + 8 * 16 + 2 * 8)
    {
        p += 8 * 16 + 8;
        gdb_set_cpsr(gdb_hexword2dec(p));
    }

    gdb_reply_ok(reply);
}

void gdb_cmd_get_memory(char *args, char *reply)
{
    unsigned long addr, len, i;

    gdb_get_hexnumber(&args, &addr);

    /* sanity check */
    if(*args != ',')
    {
        gdb_reply_error(0, reply);
        return;
    }

    args++;
    gdb_get_hexnumber(&args, &len);

    if (len > (GDB_TRANSMIT_BUFFER_SIZE - 16) / 2) {
        gdb_reply_error(1, reply);
        return;
    }

    for (i = 0; i < len; i++)
	{
        gdb_byte2hexbyte(reply + i * 2, *((uint8_t *)(addr + i)));
	}

    reply[len * 2] = 0;
}

void gdb_cmd_put_memory(char *args, char *reply)
{
    unsigned long addr, len, i;

    gdb_get_hexnumber(&args, &addr);

    /* sanity check */
    if(*args != ',')
    {
        gdb_reply_error(0, reply);
        return;
    }

    args++;
    gdb_get_hexnumber(&args, &len);
    
    /* sanity check */
    if(*args != ':')
    {
        gdb_reply_error(0, reply);
        return;
    }
    
    args++;
    for (i = 0; i < len; i++)
    {
        *((uint8_t *)(addr + i)) = gdb_hexbyte2dec(args + i * 2);
    }
}

void gdb_cmd_put_memory_binary(char *args, char *reply)
{
    unsigned long addr, len, i;

    gdb_get_hexnumber(&args, &addr);

    /* sanity check */
    if(*args != ',')
    {
        gdb_reply_error(0, reply);
        return;
    }

    args++;
    gdb_get_hexnumber(&args, &len);
    
    /* sanity check */
    if(*args != ':')
    {
        gdb_reply_error(0, reply);
        return;
    }
    
    args++;
    for (i = 0; i < len; i++)
    {
        *((uint8_t *)(addr + i)) = args[i];
    }
}

void gdb_cmd_del_breakpoint(char *args, char *reply)
{
    unsigned long addr;
    breakpoint_t *bkpt = NULL;

    /* sanity check */
    while((*args != ',') && (*args != 0))
    {
        args++;
    }
    
    /* sanity check */
    if(*args != ',')
    {
        gdb_reply_error(0, reply);
        return;
    }
    
    args++;
    gdb_get_hexnumber(&args, &addr);
    
    bkpt = gdb_find_bkpt(addr);

    /* if found, unarm and disable */    
    if(bkpt)
    {
        gdb_unarm_bkpt(bkpt);
        bkpt->flags &= ~GDB_BKPT_FLAG_ENABLED;
    }    
}

void gdb_cmd_add_breakpoint(char *args, char *reply)
{
    unsigned long addr;

    /* sanity check */
    while(*args != ',' && *args)
    {
        args++;
    }
    
    /* sanity check */
    if(*args != ',')
    {
        gdb_reply_error(0, reply);
        return;
    }
    
    args++;
    gdb_get_hexnumber(&args, &addr);
    gdb_add_bkpt(addr, GDB_BKPT_FLAG_ARMED);
    gdb_reply_ok(reply);
}

void gdb_cmd_old_breakpoint(char *args, char *reply)
{
    unsigned long addr;
    breakpoint_t *bkpt = NULL;
    
    gdb_get_hexnumber(&args, &addr);
    
    /* sanity check */
    if(*args != ',')
    {
        gdb_reply_error(0, reply);
        return;
    }
    args++;
    
    switch(*args)
    {
        case 'S':
            gdb_add_bkpt(addr, GDB_BKPT_FLAG_ARMED);
            gdb_reply_ok(reply);
            return;

        case 'C':
            bkpt = gdb_find_bkpt(addr);

            /* if found, unarm and disable */    
            if(bkpt)
            {
                gdb_unarm_bkpt(bkpt);
                bkpt->flags &= ~GDB_BKPT_FLAG_ENABLED;
                gdb_reply_ok(reply);
                return;
            }
            break;
    }
    
	gdb_reply_error(0xFF, reply);    
}

void gdb_cmd_go_signal(char *args, char *reply)
{
    reply[0] = 0;
}

void gdb_cmd_go(char *args, char *reply)
{
    gdb_reply_ok(reply);
    
    /* check for current breakpoint */    
    if(gdb_current_bkpt && (gdb_current_bkpt->flags & GDB_BKPT_FLAG_TASK_STALLED))
    {
        /* mark this breakpoint to be resumed */
        gdb_current_bkpt->flags &= ~GDB_BKPT_FLAG_TASK_STALLED;
        gdb_current_bkpt->flags |= GDB_BKPT_FLAG_RESUME;
        gdb_current_bkpt->unStall = 1;
        //bmp_printf(FONT_MED, 0, 300, "gdb_cmd_go: resumed 0x%08X", gdb_current_bkpt->address);
    }
    else
    {
        //bmp_printf(FONT_MED, 0, 300, "gdb_cmd_go: nothing to resume, waiting");
    }
    
    /* wait for some breakpoint to be reached */
    while(gdb_find_stalled_bkpt() == NULL)
    {
        msleep(1);
    }
    
    gdb_current_bkpt = gdb_find_stalled_bkpt();
    
    /* reached breakpoint */
	gdb_reply_signal(GDB_SIGNAL_TRAP,reply);
}

/* single step not supported, use it to synchronize breakpoint status */
void gdb_cmd_step(char *args, char *reply)
{
    gdb_current_bkpt = gdb_find_stalled_bkpt();
	gdb_reply_signal(GDB_SIGNAL_TRAP,reply);
}

void gdb_cmd_query(char *args, char *reply)
{
#if 0
    if(!strncmp("fThreadInfo", args, 11))
    {
        char buf[8];
        int taskId;
        struct task_attr_str task_attr;
        
        strcpy(reply, "m");
        
        for (taskId = 1; taskId < (int)task_max; taskId++)
        {
            if (is_taskid_valid(1, taskId, &task_attr) == 0) 
            {
                sprintf(buf, "%X,", taskId);
                strcpy(&reply[strlen(reply)], buf);
            }
        }
    }
    else if(!strncmp("C", args, 1))
    {
        sprintf(reply, "QC%X", gdb_attached_pid);
    }
    else
#endif   
    {
        gdb_strncpy(reply, "", 0);
    }
}

void gdb_main_task(void)
{
	int running = 1;
    char packet_buf[GDB_TRANSMIT_BUFFER_SIZE];
    char reply_buf[GDB_TRANSMIT_BUFFER_SIZE];

	while (running)
	{
        gdb_get_packet(packet_buf, sizeof(packet_buf) - 1);

        switch (packet_buf[0])
		{
            /* 	Indicate the reason the target halted. The reply is the same as for step and continue. */
            case '?':
                gdb_reply_signal(0, reply_buf);
                break;

            /* 	Enable extended mode. In extended mode, the remote server is made persistent. The `R' packet is used to restart the program being debugged.
	            The remote target both supports and has enabled extended mode.
                reply `OK'
             */
            case '!':
                gdb_reply_ok(reply_buf);
                break;

            /* Detach GDB from the remote system. Sent to the remote target before GDB disconnects. */
            case 'D':
				running = 0;
                break;

            /* Set thread for subsequent operations (`m', `M', `g', `G', et.al.). c = `c' for thread used in step and continue; t... can be -1 for all threads. c = `g' for thread used in other operations. If zero, pick a thread, any thread.  */
            case 'H':
                gdb_reply_ok(reply_buf);
                break;

            case 'p':
                gdb_cmd_get_register(packet_buf + 1, reply_buf);
                break;

            /* Pn...=r... Write register n... with value r..., which contains two hex digits for each byte in the register (target byte order).  */
            case 'P':
                gdb_cmd_set_register(packet_buf + 1, reply_buf);
                break;

            /* Read general registers
               Each byte of register data is described by two hex digits. The bytes with the register are transmitted in target byte order. 
               The size of each register and their position within the `g' packet are determined by the GDB internal macros REGISTER_RAW_SIZE and REGISTER_NAME macros.
               The specification of several standard g packets is specified below. 
             */
            case 'g':
                gdb_cmd_regs_reply(reply_buf);
                break;

            case 'G':
                gdb_cmd_set_registers(packet_buf + 1, reply_buf);
                break;

            case 'm':
                gdb_cmd_get_memory(packet_buf + 1, reply_buf);
                break;

            case 'M':
                gdb_cmd_put_memory(packet_buf + 1, reply_buf);
                break;

            /* Xaddr,length:XX.. addr is address, length is number of bytes, XX... is binary data. The characters $, #, and 0x7d are escaped using 0x7d */
            case 'X':
                gdb_cmd_put_memory_binary(packet_buf + 1, reply_buf);
                break;

            case 'z':
                gdb_cmd_del_breakpoint(packet_buf + 1, reply_buf);
                break;

            case 'Z':
                gdb_cmd_add_breakpoint(packet_buf + 1, reply_buf);
                break;

            case 'B':
                gdb_cmd_old_breakpoint(packet_buf + 1, reply_buf);
                break;

            /* Request info about query. In general GDB queries have a leading upper case letter. Custom vendor queries should use a company prefix (in lower case) ex: `qfsf.var'. 
               query may optionally be followed by a `,' or `;' separated list. Stubs must ensure that they match the full query name. 
             */
            case 'q':
                gdb_cmd_query(packet_buf + 1, reply_buf);
                break;

            case 'c':
                gdb_cmd_go(packet_buf + 1, reply_buf);
                break;

            case 'C':
                gdb_cmd_go_signal(packet_buf + 1, reply_buf);
                break;

            /* kill */
            case 'k':
                gdb_reply_ok(reply_buf);
                break;

            case 's':
                gdb_cmd_step(packet_buf + 1, reply_buf);
                break;

            default:
                reply_buf[0] = 0;
        }

        gdb_send_packet(reply_buf);
    }
}

void gdb_api_log(char *msg)
{
    int i;
    char reply_buf[GDB_TRANSMIT_BUFFER_SIZE];

    reply_buf[0] = 'O';
    i = 1;
    while (*msg && i + 2 <= GDB_TRANSMIT_BUFFER_SIZE - 1)
    {
        gdb_byte2hexbyte(reply_buf + i, *msg++);
        i += 2;
    }
    reply_buf[i] = 0;
    gdb_send_packet(reply_buf);
}
#endif
