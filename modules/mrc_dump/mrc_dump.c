#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#define MEM(x) (*(int*)(x))

unsigned int mrc_cp = 15;
unsigned int mrc_crn = 0;
unsigned int mrc_crm = 0;
unsigned int mrc_op1 = 0;
unsigned int mrc_op2 = 0;

unsigned int mrc_value = 0;


static MENU_UPDATE_FUNC(mrc_dump_update_ins)
{
    MENU_SET_NAME("MRC p%d, %d, Rd, c%d, c%d, %d", mrc_cp, mrc_op1, mrc_crn, mrc_crm, mrc_op2);
    MENU_SET_VALUE("");
}

static MENU_UPDATE_FUNC(mrc_dump_update_val)
{
    MENU_SET_VALUE("0x%08X", mrc_value);
}

static MENU_SELECT_FUNC(mrc_dump_select)
{
    /* generate instruction into uncacheable memory (else we would have to flush I and D caches) */
    void *func_buf = alloc_dma_memory(32);
    unsigned int addr_func = UNCACHEABLE(func_buf);
    unsigned int (*func)() = addr_func;
    
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
        .name = "Instr",
        .min = 0,
        .max = 0,
        .icon_type = IT_BOOL,
        .update = mrc_dump_update_ins,
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
