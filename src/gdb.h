
#ifndef _GDB_H_
#define _GDB_H_

#define GDB_TRANSMIT_BUFFER_SIZE 1024
#define GDB_STRING_BUFFER_SIZE 1024
#define GDB_STACK_SIZE 64 /* words */

#define GDB_SIGNAL_HUP   1//, "SIGHUP", "Hangup")
#define GDB_SIGNAL_INT   2//, "SIGINT", "Interrupt")
#define GDB_SIGNAL_QUIT  3//, "SIGQUIT", "Quit")
#define GDB_SIGNAL_ILL   4//, "SIGILL", "Illegal instruction")
#define GDB_SIGNAL_TRAP  5//, "SIGTRAP", "Trace/breakpoint trap")
#define GDB_SIGNAL_ABRT  6//, "SIGABRT", "Aborted")
#define GDB_SIGNAL_EMT   7//, "SIGEMT", "Emulation trap")


#define GDB_REG_STALL_MAGIC 0xDEADFACE

#define GDB_BKPT_OPCODE 0xe7ffdefe
#define GDB_BKPT_COUNT 64

#define GDB_BKPT_FLAG_ENABLED             0x01
#define GDB_BKPT_FLAG_ARMED               0x02
#define GDB_BKPT_FLAG_TASK_STALLED        0x04
#define GDB_BKPT_FLAG_RESUME              0x08
#define GDB_BKPT_FLAG_WATCHPOINT          0x10
#define GDB_BKPT_FLAG_WATCHPOINT_REACHED  0x20
#define GDB_BKPT_FLAG_WATCHPOINT_LINK     0x40

#define GDB_TASK_STATE_STALL     7

#define GDB_STALL_STACK_SIZE     32

#define GDB_LINK_NONE 0xFFFFFFFF

typedef struct
{
    uint32_t id;
    uint32_t linkId;
    uint32_t address;
    uint32_t flags;
    uint32_t hitcount;
    
    /* original task context at breakpoint */
    uint32_t ctx[17];
    
    /* pointer to associated task */
    struct task *taskStruct;
    uint32_t taskState;
    
    /* address of callback when this watch/breakpoint is triggered (void* instead of breakpoint_t* as compilation will fail) */
    void (*callback)(void *);
    
    /* temporary context to store registers for scheduler to restore our stall routine */
    uint32_t tempStack[GDB_STALL_STACK_SIZE];
    uint32_t unStall;
} breakpoint_t;


breakpoint_t *gdb_quick_watchpoint(uint32_t address);
breakpoint_t *gdb_add_watchpoint(uint32_t address, uint32_t linkedAddress, void (*callback)(breakpoint_t *));
breakpoint_t *gdb_add_bkpt(uint32_t address, uint32_t flags);
uint32_t gdb_instr_is_pc_modifying(uint32_t opcode);
char *gdb_get_callstack(breakpoint_t *bkpt);
uint32_t gdb_setup();
void gdb_delete_bkpt(breakpoint_t *bkpt);

#endif
