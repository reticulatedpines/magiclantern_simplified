#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#define MEM(x) (*(volatile int*)(x))
#define MRC_DUMP_STACK_SIZE 128


extern unsigned int mem_prot_irq_end_part_calls;
extern unsigned int mem_prot_irq_end_full_calls;
extern unsigned int mem_prot_irq_entry_calls;
extern unsigned int mem_prot_irq_depth;

extern unsigned int mem_prot_trap_count;
extern unsigned int mem_prot_trap_addr;
extern unsigned int mem_prot_trap_task;

extern unsigned int mem_prot_irq_orig;
extern unsigned int mem_prot_trap_stackptr;
unsigned int mem_prot_trap_stack[MRC_DUMP_STACK_SIZE];

unsigned int mem_prot_hook_stackhead = 0;
unsigned int mem_prot_irq_end_full_addr = 0;
unsigned int mem_prot_irq_end_part_addr = 0;
unsigned int mem_prot_hook_full = 0;
unsigned int mem_prot_hook_part = 0;


void __attribute__ ((naked)) mem_prot_trap()
{
    /* data abort exception occurred. switch stacks, call handler and skip trapping instruction */
    asm(
        "STR     SP, mem_prot_trap_stackptr_bak\n"
        "LDR     SP, mem_prot_trap_stackptr\n"
        
        "STMFD   SP!, {R0-R12, LR}\n"
        
        /* save information about trapping code */
        "BL      get_current_task\n"
        "AND     R0, #0xFF\n"
        "STR     R0, mem_prot_trap_task\n"
        "LDR     R0, mem_prot_trap_count\n"
        "ADD     R0, #0x01\n"
        "STR     R0, mem_prot_trap_count\n"
        "SUB     R0, R14, #8\n"
        "STR     R0, mem_prot_trap_addr\n"
        
        /* was it the irq handler? in that case allow writes */
        "CMP     R0, #0x01000\n"
        "BGT     mem_prot_trap_nowrite\n"
        
        /* enable full access to memory */
        "MOV     R0, #0x00\n"
        "MCR     p15, 0, r0, c6, c7, 0\n"
        
        "LDMFD   SP!, {R0-R12, LR}\n"
        "LDR     SP, mem_prot_trap_stackptr_bak\n"
        
        /* execute instruction again */
        "SUBS    PC, R14, #8\n"
        
        /* ------------------------------------------ */
        
        /* skip over instruction */
        "mem_prot_trap_nowrite:\n"
        "LDMFD   SP!, {R0-R12, LR}\n"
        "LDR     SP, mem_prot_trap_stackptr_bak\n"
        "SUBS    PC, R14, #4\n"
        
        /* ------------------------------------------ */
        
        "mem_prot_trap_stackptr:\n"
        ".word 0x00000000\n"
        "mem_prot_trap_stackptr_bak:\n"
        ".word 0x00000000\n"
        "mem_prot_trap_count:\n"
        ".word 0x00000000\n"
        "mem_prot_trap_addr:\n"
        ".word 0x00000000\n"
        "mem_prot_trap_task:\n"
        ".word 0x00000000\n"
    );
}

void __attribute__ ((naked)) mem_prot_irq_entry()
{
    /* irq code is being called. disable protection. */
    asm(
        /* first save scratch register to buffer */
        "str r0, mem_prot_irq_tmp\n"
        
        /* enable full access to memory as the IRQ handler relies on it */
        "mov r0, #0x00\n"
        "mcr p15, 0, r0, c6, c7, 0\n"
        
        /* update some counters */
        "ldr r0, mem_prot_irq_entry_calls\n"
        "add r0, #0x01\n"
        "str r0, mem_prot_irq_entry_calls\n"
        "ldr r0, mem_prot_irq_depth\n"
        "add r0, #0x01\n"
        "str r0, mem_prot_irq_depth\n"

        /* now restore scratch and jump to IRQ handler */
        "ldr r0, mem_prot_irq_tmp\n"
        "ldr pc, mem_prot_irq_orig\n"

        "mem_prot_irq_tmp:\n"
        ".word 0x00000000\n"
        "mem_prot_irq_orig:\n"
        ".word 0x00000000\n"
        "mem_prot_irq_entry_calls:\n"
        ".word 0x00000000\n"
    );
}

void __attribute__ ((naked)) mem_prot_irq_end_full()
{
    asm(
        /* save int flags and disable all interrupts */
        "mrs    r2, CPSR\n"
        "orr    r1, r2, #0xC0\n"
        "msr    CPSR_c, r1\n"
        "and    r2, r2, #0xC0\n"
        
        /* update some counters */
        "ldr    r0, mem_prot_irq_end_full_calls\n"
        "add    r0, #0x01\n"
        "str    r0, mem_prot_irq_end_full_calls\n"
        "ldr    r0, mem_prot_irq_depth\n"
        "sub    r0, #0x01\n"
        "str    r0, mem_prot_irq_depth\n"
        
        "cmp    r0, #0x00\n"
        "bne    mem_prot_irq_end_full_nested\n"
        
        /* enable memory protection again */
        "mov    r0, #0x17\n"
        "mcr    p15, 0, r0, c6, c7, 0\n"
        
        "mem_prot_irq_end_full_nested:\n"
        
        /* restore int flags */
        "mrs    r1, CPSR\n"
        "bic    r1, r1, #0xC0\n"
        "and    r2, r2, #0xC0\n"
        "orr    r1, r1, r2\n"
        "msr    CPSR_c, r1\n" 
        
        "LDMFD  SP!, {R0-R12,LR,PC}^\n"
        
        "mem_prot_irq_end_full_calls:\n"
        ".word 0x00000000\n"
    );
}

void __attribute__ ((naked)) mem_prot_irq_end_part()
{
    asm(
        /* save int flags and disable all interrupts */
        "mrs    r2, CPSR\n"
        "orr    r1, r2, #0xC0\n"
        "msr    CPSR_c, r1\n"
        "and    r2, r2, #0xC0\n"
        
        /* update some counters */
        "ldr    r0, mem_prot_irq_end_part_calls\n"
        "add    r0, #0x01\n"
        "str    r0, mem_prot_irq_end_part_calls\n"
        "ldr    r0, mem_prot_irq_depth\n"
        "sub    r0, #0x01\n"
        "str    r0, mem_prot_irq_depth\n"
        
        "cmp    r0, #0x00\n"
        "bne    mem_prot_irq_end_part_nested\n"
        
        /* enable memory protection again */
        "mov    r0, #0x17\n"
        "mcr    p15, 0, r0, c6, c7, 0\n"
        
        "mem_prot_irq_end_part_nested:\n"
        
        /* restore int flags */
        "mrs    r1, CPSR\n"
        "bic    r1, r1, #0xC0\n"
        "and    r2, r2, #0xC0\n"
        "orr    r1, r1, r2\n"
        "msr    CPSR_c, r1\n" 
        
        "LDMFD   SP!, {R0-R4,R12,PC}^\n"
        
        "mem_prot_irq_end_part_calls:\n"
        ".word 0x00000000\n"
        "mem_prot_irq_depth:\n"
        ".word 0x00000000\n"
    );
}

unsigned int mem_prot_find_hooks()
{
    unsigned int addr = MEM(0x00000030);
    
    while(MEM(addr) != 0xEEEEEEEE && addr < 0x1000)
    {
        if(MEM(addr) == 0xE8FDDFFF)
        {
            if(mem_prot_hook_full)
            {
                return 1;
            }
            mem_prot_hook_full = addr;
        }
        if(MEM(addr) == 0xE8FD901F)
        {
            if(mem_prot_hook_part)
            {
                return 2;
            }
            mem_prot_hook_part = addr;
        }
        addr += 4;
    }
    
    /* failed to find irq stack head */
    if(addr >= 0x1000)
    {
        /* use free space in IV for our pointers. vectors RESET and BKPT are not used. */
        mem_prot_irq_end_full_addr = 0x00;
        mem_prot_irq_end_part_addr = 0x14;
    }
    else
    {
        /* leave some space to prevent false stack overflow alarms (if someone ever checked...) */
        mem_prot_hook_stackhead = addr + 0x100;
        mem_prot_irq_end_full_addr = mem_prot_hook_stackhead + 0;
        mem_prot_irq_end_part_addr = mem_prot_hook_stackhead + 4;
    }
    
    if(mem_prot_hook_full && mem_prot_hook_part)
    {
        return 0;
    }
    
    return 4;
}

void mem_prot_install()
{
    unsigned int err = mem_prot_find_hooks();
    if(err)
    {
        bmp_printf(FONT(FONT_MED, COLOR_RED, COLOR_BLACK), 0, 0, "mem_prot: Failed to find hook points (err %d).", err);
        return;
    }
    
    unsigned int int_status = cli();
    
    /* place jumps to our interrupt end code */
    MEM(mem_prot_irq_end_full_addr) = &mem_prot_irq_end_full;
    MEM(mem_prot_irq_end_part_addr) = &mem_prot_irq_end_part;
    
    /* install pre-irq hook */
    mem_prot_irq_orig = MEM(0x00000030);
    MEM(0x00000030) = (unsigned int)&mem_prot_irq_entry;
    
    /* place a LDR PC, [PC, rel_offset] at irq end  to jump to our code */
    MEM(mem_prot_hook_full) = 0xE59FF000 | ((mem_prot_hook_stackhead + 0) - mem_prot_hook_full - 8);
    MEM(mem_prot_hook_part) = 0xE59FF000 | ((mem_prot_hook_stackhead + 4) - mem_prot_hook_part - 8);
    
    /* install data abort handler */
    MEM(0x0000002C) = (unsigned int)&mem_prot_trap;
    
    /* set up its own stack */
    mem_prot_trap_stackptr = (unsigned int)&mem_prot_trap_stack[MRC_DUMP_STACK_SIZE];
    
    
    /* set range 0x00000000 - 0x00001000 buffer/cache bits. protection will get enabled after next interrupt */
    asm(
        /* set area cachable */
        "mrc    p15, 0, R4, c2, c0, 0\n"
        "orr    r4, #0x80\n"
        "mcr    p15, 0, R4, c2, c0, 0\n"
        "mrc    p15, 0, R4, c2, c0, 1\n"
        "orr    r4, #0x80\n"
        "mcr    p15, 0, R4, c2, c0, 1\n"
        
        /* set area bufferable */
        "mrc    p15, 0, R4, c3, c0, 0\n"
        "orr    r4, #0x80\n"
        "mcr    p15, 0, R4, c3, c0, 0\n"
        
        /* set access permissions */
        "mrc    p15, 0, R4, c5, c0, 2\n"
        "lsl    R4, #0x04\n"
        "lsr    R4, #0x04\n"
        "orr    R4, #0x60000000\n"
        "mcr    p15, 0, R4, c5, c0, 2\n"
        
        "mrc    p15, 0, R4, c5, c0, 3\n"
        "lsl    R4, #0x04\n"
        "lsr    R4, #0x04\n"
        "orr    R4, #0x60000000\n"
        "mcr    p15, 0, R4, c5, c0, 3\n"
        
        : : : "r4"
    );
    
    clean_d_cache();
    asm(
        "mov r0, #0\n"
        "mcr p15, 0, r0, c7, c5, 0\n" // clean I cache
        : : : "r0"
    );
    sei(int_status);
}

static MENU_UPDATE_FUNC(mem_prot_update_count)
{
    MENU_SET_VALUE("%d", mem_prot_trap_count);
}

static MENU_UPDATE_FUNC(mem_prot_update_addr)
{
    MENU_SET_VALUE("0x%08X", mem_prot_trap_addr);
}

static MENU_UPDATE_FUNC(mem_prot_update_task)
{
    if(mem_prot_trap_task)
    {
        char *name = (char*)get_task_name_from_id(mem_prot_trap_task);
        MENU_SET_VALUE("%d: %s", mem_prot_trap_task, name);
    }
    else
    {
        MENU_SET_VALUE("(none)");
    }
}


static MENU_SELECT_FUNC(mem_prot_select)
{
    MEM(0) = MEM(0)+1;
}


static struct menu_entry mem_prot_menu[] =
{
    {
        .name = "Mem Protection",
        .select = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            {
                .name = "Exceptions",
                .update = mem_prot_update_count,
                .select = mem_prot_select,
            },
            {
                .name = "Last PC",
                .update = mem_prot_update_addr,
            },
            {
                .name = "Task",
                .update = mem_prot_update_task,
            },
            MENU_EOL,
        }
    }
};

unsigned int mem_prot_init()
{
    menu_add("Debug", mem_prot_menu, COUNT(mem_prot_menu));
    mem_prot_install();
    return 0;
}

unsigned int mem_prot_deinit()
{
    menu_remove("Debug", mem_prot_menu, COUNT(mem_prot_menu));
    return 0;
}




MODULE_INFO_START()
    MODULE_INIT(mem_prot_init)
    MODULE_DEINIT(mem_prot_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Description", "Install a memory protection handler.")
MODULE_STRINGS_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

MODULE_PARAMS_START()
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()
