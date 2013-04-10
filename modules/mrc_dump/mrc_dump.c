#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#define MEM(x) (*(volatile int*)(x))

unsigned int mrc_cp = 15;
unsigned int mrc_crn = 0;
unsigned int mrc_crm = 0;
unsigned int mrc_op1 = 0;
unsigned int mrc_op2 = 0;

unsigned int mrc_value = 0;

typedef struct tmp
{
	int crn;
	int op1;
	int crm;
	int op2;
	char *desc;
} reg_entry;

reg_entry regs[] = {
	/* System control and configuration registers */
	{  0,  0,  0,  0, "System Control: ID Code Register"     },
	{  0,  0,  1,  0, "System Control: CPUID Register C1,0: Processor Feature Register 0"  },
	{  0,  0,  1,  1, "System Control: CPUID Register C1,1: Processor Feature Register 1"  },
	{  0,  0,  1,  2, "System Control: CPUID Register C1,2: Debug Feature Register 0"  },
	{  0,  0,  1,  3, "System Control: CPUID Register C1,3"  },
	{  0,  0,  1,  4, "System Control: CPUID Register C1,4: Memory Model Feature Register 0"  },
	{  0,  0,  1,  5, "System Control: CPUID Register C1,5: Memory Model Feature Register 1"  },
	{  0,  0,  1,  6, "System Control: CPUID Register C1,6: Memory Model Feature Register 2"  },
	{  0,  0,  1,  7, "System Control: CPUID Register C1,7: Memory Model Feature Register 3"  },
	{  0,  0,  2,  0, "System Control: CPUID Register C2,0"  },
	{  0,  0,  2,  1, "System Control: CPUID Register C2,1"  },
	{  0,  0,  2,  2, "System Control: CPUID Register C2,2"  },
	{  0,  0,  2,  3, "System Control: CPUID Register C2,3"  },
	{  0,  0,  2,  4, "System Control: CPUID Register C2,4"  },
	{  0,  0,  2,  5, "System Control: CPUID Register C2,5"  },
	{  0,  0,  2,  6, "System Control: CPUID Register C2,6"  },
	{  0,  0,  2,  7, "System Control: CPUID Register C2,7"  },
	{  0,  0,  3,  0, "System Control: CPUID Register C3,0"  },
	{  0,  0,  3,  1, "System Control: CPUID Register C3,1"  },
	{  0,  0,  3,  2, "System Control: CPUID Register C3,2"  },
	{  0,  0,  3,  3, "System Control: CPUID Register C3,3"  },
	{  0,  0,  3,  4, "System Control: CPUID Register C3,4"  },
	{  0,  0,  3,  5, "System Control: CPUID Register C3,5"  },
	{  0,  0,  3,  6, "System Control: CPUID Register C3,6"  },
	{  0,  0,  3,  7, "System Control: CPUID Register C3,7"  },
	{  0,  0,  4,  0, "System Control: CPUID Register C4,0"  },
	{  0,  0,  4,  1, "System Control: CPUID Register C4,1"  },
	{  0,  0,  4,  2, "System Control: CPUID Register C4,2"  },
	{  0,  0,  4,  3, "System Control: CPUID Register C4,3"  },
	{  0,  0,  4,  4, "System Control: CPUID Register C4,4"  },
	{  0,  0,  4,  5, "System Control: CPUID Register C4,5"  },
	{  0,  0,  4,  6, "System Control: CPUID Register C4,6"  },
	{  0,  0,  4,  7, "System Control: CPUID Register C4,7"  },
	{  0,  0,  5,  0, "System Control: CPUID Register C5,0"  },
	{  0,  0,  5,  1, "System Control: CPUID Register C5,1"  },
	{  0,  0,  5,  2, "System Control: CPUID Register C5,2"  },
	{  0,  0,  5,  3, "System Control: CPUID Register C5,3"  },
	{  0,  0,  5,  4, "System Control: CPUID Register C5,4"  },
	{  0,  0,  5,  5, "System Control: CPUID Register C5,5"  },
	{  0,  0,  5,  6, "System Control: CPUID Register C5,6"  },
	{  0,  0,  5,  7, "System Control: CPUID Register C5,7"  },
	{  0,  0,  6,  0, "System Control: CPUID Register C6,0"  },
	{  0,  0,  6,  1, "System Control: CPUID Register C6,1"  },
	{  0,  0,  6,  2, "System Control: CPUID Register C6,2"  },
	{  0,  0,  6,  3, "System Control: CPUID Register C6,3"  },
	{  0,  0,  6,  4, "System Control: CPUID Register C6,4"  },
	{  0,  0,  6,  5, "System Control: CPUID Register C6,5"  },
	{  0,  0,  6,  6, "System Control: CPUID Register C6,6"  },
	{  0,  0,  6,  7, "System Control: CPUID Register C6,7"  },
	{  0,  0,  7,  0, "System Control: CPUID Register C7,0"  },
	{  0,  0,  7,  1, "System Control: CPUID Register C7,1"  },
	{  0,  0,  7,  2, "System Control: CPUID Register C7,2"  },
	{  0,  0,  7,  3, "System Control: CPUID Register C7,3"  },
	{  0,  0,  7,  4, "System Control: CPUID Register C7,4"  },
	{  0,  0,  7,  5, "System Control: CPUID Register C7,5"  },
	{  0,  0,  7,  6, "System Control: CPUID Register C7,6"  },
	{  0,  0,  7,  7, "System Control: CPUID Register C7,7"  },
	{  1,  0,  0,  0, "System Control: Control Register"  },
	{  1,  0,  0,  1, "System Control: Auxiliary Control Register"  },
	{  1,  0,  0,  2, "System Control: Coprocessor Access Control Register"  },
	{  1,  0,  1,  0, "System Control: Secure Configuration Register"  },
	{  1,  0,  1,  1, "System Control: Secure Debug Enable Register"  },
	{  1,  0,  1,  2, "System Control: Non-secure Access Control Register"  },
	{ 12,  0,  0,  0, "System Control: Non-secure or Secure Vector Base Address Register"  },
	{ 12,  0,  0,  1, "System Control: Monitor Vector Base Address Register"  },
	{ 12,  0,  1,  0, "System Control: Interrupt Status Register"  },
	
	/* MMU control and configuration registers */
	{  0,  0,  0,  3, "MMU Control: TLB Type Register" },
	{  2,  0,  0,  0, "MMU Control: Translation Table Base Register 0" },
	{  2,  0,  0,  1, "MMU Control: Translation Table Base Register 1" },
	{  2,  0,  0,  2, "MMU Control: Translation Table Base Control Register" },
	{  3,  0,  0,  0, "MMU Control: Domain Access Control Register" },
	{  5,  0,  0,  0, "MMU Control: Data Fault Status Register" },
	{  5,  0,  0,  1, "MMU Control: Instruction Fault Status Register" },
	{  6,  0,  0,  1, "MMU Control: Watchpoint Fault Address Register" },
    
	{  6,  0,  0,  0, "MMU Control: Memory Region 0" },
	{  6,  0,  1,  0, "MMU Control: Memory Region 1" },
	{  6,  0,  2,  0, "MMU Control: Memory Region 2" },
	{  6,  0,  3,  0, "MMU Control: Memory Region 3" },
	{  6,  0,  4,  0, "MMU Control: Memory Region 4" },
	{  6,  0,  5,  0, "MMU Control: Memory Region 5" },
	{  6,  0,  6,  0, "MMU Control: Memory Region 6" },
	{  6,  0,  7,  0, "MMU Control: Memory Region 7" },
    
	{  6,  0,  0,  2, "MMU Control: Instruction Fault Address Register" },
	{  8,  0, -1, -1, "MMU Control: TLB Operations Register" },
	{ 10,  0,  0, -1, "MMU Control: TLB Lockdown Register" },
	{ 10,  0,  2,  0, "MMU Control: Primary Region Remap Register" },
	{ 10,  0,  2,  1, "MMU Control: Normal Memory Remap Register" },
	{ 13,  0,  0,  0, "MMU Control: FCSE PID Register" },
	{ 13,  0,  0,  1, "MMU Control: Context ID Register" },
	{ 13,  0,  0,  2, "MMU Control: User Read/Write Thread and Process ID Register" },
	{ 13,  0,  0,  3, "MMU Control: User Read Only Thread and Process ID Register" },
	{ 13,  0,  0,  4, "MMU Control: Privileged Only Thread and Process ID Register" },
	{ 15,  0,  2,  4, "MMU Control: Peripheral Port Memory Remap Register" },
	{ 15,  5,  4,  2, "MMU Control: TLB Lockdown Index Register" },
	{ 15,  5,  5,  2, "MMU Control: TLB Lockdown VA Register" },
	{ 15,  5,  6,  2, "MMU Control: TLB Lockdown PA Register" },
	{ 15,  5,  7,  2, "MMU Control: TLB Lockdown Attributes Register" },	
	
	/* Cache control and configuration registers */
	{  0,  0,  0,  1, "Cache Control: Cache Type Register" },
	{  7,  0,  5,  0, "Cache Control: Flush instruction cache" },
	{  7,  0,  5,  1, "Cache Control: Flush instruction cache single entry" },
	{  7,  0,  6,  0, "Cache Control: Flush data cache" },
	{  7,  0,  6,  1, "Cache Control: Flush data cache single entry" },
	{  7,  0, 10,  1, "Cache Control: Clean data cache entry" },
	{  7,  0, 10,  2, "Cache Control: Clean data cache entry" },
	{  7,  0, 13,  1, "Cache Control: Prefetch instruction cache line" },
	{  7,  0, 14,  1, "Cache Control: Clean and flush data cache entry" },
	{  7,  0, 14,  2, "Cache Control: Clean and flush data cache entry" },
	{  9,  0,  0,  0, "Cache Control: Data Cache Lockdown Register" },
	{  9,  0,  0,  1, "Cache Control: Instruction Cache Lockdown Register" },
	{  9,  0,  8,  0, "Cache Control: Cache Behavior Override  Register" },	
	
	/* TCM control and configuration registers */
	{  0,  0,  0,  2, "TCM Control: TCM Status Register" },
	{  9,  0,  1,  0, "TCM Control: Data TCM Region Register" },
	{  9,  0,  1,  1, "TCM Control: Instruction TCM Region Register" },
	{  9,  0,  1,  2, "TCM Control: Data TCM Non-secure Access Control Register" },
	{  9,  0,  1,  3, "TCM Control: Instruction TCM Non-secure Access Control Register" },
	{  9,  0,  2,  0, "TCM Control: TCM Selection Register" },	
	
	/* Cache Master Valid Registers */
	{ 15,  3,  8, -1, "Cache Master: Instruction Cache Master Valid Register" },
	{ 15,  3, 12, -1, "Cache Master: Data Cache Master Valid Register" },
	
	/* DMA control and configuration registers */
	{ 11,  0,  0,  0, "DMA Control: Present" },
	{ 11,  0,  0,  1, "DMA Control: Queued" },
	{ 11,  0,  0,  2, "DMA Control: Running" },
	{ 11,  0,  0,  3, "DMA Control: Interrupting" },
	{ 11,  0,  1,  0, "DMA Control: DMA User Accessibility Register" },
	{ 11,  0,  2,  0, "DMA Control: DMA Channel Number Register" },
	{ 11,  0,  3,  0, "DMA Control: Stop" },
	{ 11,  0,  3,  1, "DMA Control: Start" },
	{ 11,  0,  3,  2, "DMA Control: Clear" },
	{ 11,  0,  4,  0, "DMA Control: DMA Control Register" },
	{ 11,  0,  5,  0, "DMA Control: DMA Internal Start Address Register" },
	{ 11,  0,  6,  0, "DMA Control: DMA External Start Address Register" },
	{ 11,  0,  7,  0, "DMA Control: DMA Internal End Address Register" },
	{ 11,  0,  8,  0, "DMA Control: DMA Channel Status Register" },
	{ 11,  0, 15,  0, "DMA Control: DMA Context ID Register" },
	
	/* System performance monitor registers */
	{ 15,  0, 12,  0, "System Performance: Performance Monitor Control Register" },
	{ 15,  0, 12,  1, "System Performance: Cycle Counter Register" },
	{ 15,  0, 12,  2, "System Performance: Count Register 0" },
	{ 15,  0, 12,  3, "System Performance: Count Register 1" },
	
	
	/* System validation registers */
	{ 15,  0,  9,  0, "System Validation: Secure User and Non-secure Access Validation Control Register" },
	{ 15,  0, 12,  4, "System Validation: Reset counter" },
	{ 15,  0, 12,  5, "System Validation: Interrupt counter" },
	{ 15,  0, 12,  6, "System Validation: Fast interrupt counter" },
	{ 15,  0, 12,  7, "System Validation: External debug request counter" },
	{ 15,  0, 13,  1, "System Validation: Start reset counter" },
	{ 15,  0, 13,  2, "System Validation: Start interrupt counter" },
	{ 15,  0, 13,  3, "System Validation: Start reset and interrupt counters" },
	{ 15,  0, 13,  4, "System Validation: Start fast interrupt counter" },
	{ 15,  0, 13,  5, "System Validation: Start reset and fast interrupt counters" },
	{ 15,  0, 13,  6, "System Validation: Start interrupt and fast interrupt counters" },
	{ 15,  0, 13,  7, "System Validation: Start reset, interrupt and fast interrupt counters" },
	{ 15,  1, 13, -1, "System Validation: Start external debug request counter" },
	{ 15,  2, 13,  1, "System Validation: Stop reset counter" },
	{ 15,  2, 13,  2, "System Validation: Stop interrupt counter" },
	{ 15,  2, 13,  3, "System Validation: Stop reset and interrupt counters" },
	{ 15,  2, 13,  4, "System Validation: Stop fast interrupt counter" },
	{ 15,  2, 13,  5, "System Validation: Stop reset and fast interrupt counters" },
	{ 15,  2, 13,  6, "System Validation: Stop interrupt and fast interrupt counters" },
	{ 15,  2, 13,  7, "System Validation: Stop reset, interrupt and fast interrupt counters" },
	{ 15,  3, 13, -1, "System Validation: Stop external debug request counter" },
	{ 15,  0, 14, -1, "System Validation: System Validation Cache Size Mask Register" },

	/*  */
	{ 15,  0,  0,  0, "System Control: Debug Override Register" },
	{ 15,  0,  8,  2, "Wait for interrupt" },
	{  7,  0,  0,  4, "Wait for interrupt" },
	{  7,  0, 10,  4, "Drain write buffer" },
    
	{  0,  0,  0,  0, NULL }
};

/* MCR/MRC p15, op1, Rd, CRn, CRm, op2 */

char *mrc_dump_get_desc (int crn, int op1, int crm, int op2)
{
	int pos = 0;
	
	while(regs[pos].desc != NULL)
	{
		if(regs[pos].crn == crn || regs[pos].crn == -1)
		{
			if(regs[pos].op1 == op1 || regs[pos].op1 == -1)
			{
				if(regs[pos].crm == crm || regs[pos].crm == -1)
				{
					if(regs[pos].op2 == op2 || regs[pos].op2  == -1)
					{
						return regs[pos].desc;
					}
				}
			}
		}
		pos++;
	}
	
	return NULL;
}


void mrc_dump_process()
{
    /* generate instruction into uncacheable memory (else we would have to flush I and D caches) */
    void *func_buf = alloc_dma_memory(32);
    unsigned int addr_func = (unsigned int)UNCACHEABLE(func_buf);
    unsigned int (*func)() = (unsigned int (*)())addr_func;
    
    /* build MRC instruction */
    unsigned int instr = 0xEE100010;
    
    instr |= (mrc_op1<<21);
    instr |= (mrc_crn<<16);
    instr |= (0<<12);
    instr |= (mrc_cp<<8);
    instr |= (mrc_op2<<5);
    instr |= mrc_crm;
    
    /* write along with RET */
    MEM(addr_func) = instr;
    MEM(addr_func + 4) = 0xE1A0F00E;
    
    /* call generated code */
    mrc_value = func();
    
    free_dma_memory(func_buf);
}

static MENU_UPDATE_FUNC(mrc_dump_update_ins)
{
    MENU_SET_NAME("MRC p%d, %d, Rd, c%d, c%d, %d", mrc_cp, mrc_op1, mrc_crn, mrc_crm, mrc_op2);
    MENU_SET_VALUE("");
}

static MENU_UPDATE_FUNC(mrc_dump_update_str)
{
    if(mrc_cp == 15)
    {
        char *str = mrc_dump_get_desc(mrc_crn, mrc_op1, mrc_crm, mrc_op2);
        if(str)
        {
            MENU_SET_NAME(str);
            MENU_SET_HELP(str);
        }
    }
    MENU_SET_VALUE("");
}

static MENU_UPDATE_FUNC(mrc_dump_update_val)
{
    MENU_SET_VALUE("0x%08X", mrc_value);
    mrc_dump_process();
}

static MENU_SELECT_FUNC(mrc_dump_select)
{
    mrc_dump_process();
}

static struct menu_entry mrc_dump_menu[] =
{
    {
        .name = "cp_num",
        .priv = &mrc_cp,
        .max = 15,
        .icon_type = IT_ALWAYS_ON,
    },
    {
        .name = "opcode1",
        .priv = &mrc_op1,
        .max = 7,
        .icon_type = IT_ALWAYS_ON,
    },
    {
        .name = "CRn",
        .priv = &mrc_crn,
        .max = 15,
        .help = "",
        .icon_type = IT_ALWAYS_ON,
    },
    {
        .name = "CRm",
        .priv = &mrc_crm,
        .max = 15,
        .icon_type = IT_ALWAYS_ON,
    },
    {
        .name = "opcode2",
        .priv = &mrc_op2,
        .max = 7,
        .icon_type = IT_ALWAYS_ON,
    },
    {
        .name = "Instruction",
        .min = 0,
        .max = 0,
        .icon_type = IT_BOOL,
        .update = mrc_dump_update_ins,
    },
    {
        .name = "(unknown)",
        .min = 0,
        .max = 0,
        .icon_type = IT_BOOL,
        .update = mrc_dump_update_str,
    },
    {
        .name = "Read",
        .update = mrc_dump_update_val,
        .select = mrc_dump_select,
    },
};

unsigned int mrc_dump_init()
{
    menu_add("R", mrc_dump_menu, COUNT(mrc_dump_menu));
    return 0;
}

unsigned int mrc_dump_deinit()
{
    return 0;
}




MODULE_INFO_START()
    MODULE_INIT(mrc_dump_init)
    MODULE_DEINIT(mrc_dump_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Description", "Dump Coprocessor regs.")
MODULE_STRINGS_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

MODULE_PARAMS_START()
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()
