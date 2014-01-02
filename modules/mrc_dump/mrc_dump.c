
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

static unsigned int mrc_cp = 15;
static unsigned int mrc_crn = 0;
static unsigned int mrc_crm = 0;
static unsigned int mrc_op1 = 0;
static unsigned int mrc_op2 = 0;
static unsigned int mrc_value = 0;

static int mrc_dump_page = 1;

typedef struct tmp
{
    int cp;
    int crn;
    int op1;
    int crm;
    int op2;
    char *desc;
} reg_entry;

static reg_entry cp_regs[] =
{
    /* System control and configuration registers */
    { 15,  0,  0,  0,  0, "System Control: ID Code Register"  },
    { 15,  1,  0,  0,  0, "System Control: Control Register"  },
    { 15,  6,  0,  0,  0, "Protection: Memory Region 0" },
    { 15,  6,  0,  1,  0, "Protection: Memory Region 1" },
    { 15,  6,  0,  2,  0, "Protection: Memory Region 2" },
    { 15,  6,  0,  3,  0, "Protection: Memory Region 3" },
    { 15,  6,  0,  4,  0, "Protection: Memory Region 4" },
    { 15,  6,  0,  5,  0, "Protection: Memory Region 5" },
    { 15,  6,  0,  6,  0, "Protection: Memory Region 6" },
    { 15,  6,  0,  7,  0, "Protection: Memory Region 7" },

    /* Cache control and configuration registers */
    { 15,  0,  0,  0,  1, "Cache Control: Cache type register" },
    { 15,  2,  0,  0,  0, "Cache Control: Data cachable bits" },
    { 15,  2,  0,  0,  1, "Cache Control: Instruction cachable bits" },
    { 15,  3,  0,  0,  0, "Cache Control: Data bufferable bits" },

    { 15,  5,  0,  0,  2, "Cache Control: Data acc. perm. bits" },
    { 15,  5,  0,  0,  3, "Cache Control: Instr. acc. perm. bits" },

    { 15,  9,  0,  0,  0, "Cache Control: Data cache lockdown ctrl" },
    { 15,  9,  0,  0,  1, "Cache Control: Instr. cache lockdown ctrl" },

    /* TCM control and configuration registers */
    { 15,  0,  0,  0,  2, "TCM Control: TCM Size Register" },
    { 15,  9,  0,  1,  0, "TCM Control: Data TCM config" },
    { 15,  9,  0,  1,  1, "TCM Control: Instr TCM config" },

    /* built in self test */
    { 15, 15,  0,  0,  1, "BIST: TAG BIST Control" },
    { 15, 15,  1,  0,  1, "BIST: TCM BIST Control" },

    { 15, 15,  0,  0,  2, "BIST: Instr. TAG BIST Address" },
    { 15, 15,  0,  0,  3, "BIST: Instr. TAG BIST General" },
    { 15, 15,  0,  0,  6, "BIST: Data TAG BIST Address" },
    { 15, 15,  0,  0,  7, "BIST: Data TAG BIST General" },

    { 15, 15,  1,  0,  2, "BIST: Instr. TCM BIST Address" },
    { 15, 15,  1,  0,  3, "BIST: Instr. TCM BIST General" },
    { 15, 15,  1,  0,  6, "BIST: Data TCM BIST Address" },
    { 15, 15,  1,  0,  7, "BIST: Data TCM BIST General" },

    { 15, 15,  2,  0,  2, "BIST: Instr. Cache RAM BIST Address" },
    { 15, 15,  2,  0,  3, "BIST: Instr. Cache RAM BIST General" },
    { 15, 15,  2,  0,  6, "BIST: Data Cache RAM BIST Address" },
    { 15, 15,  2,  0,  7, "BIST: Data Cache RAM BIST General" },

    /* Misc */
    { 15, 13,  0,  0,  1, "Trace: Process ID" },
    { 15, 15,  0,  0,  0, "Test: Test state" },
    { 15, 15,  1,  1,  0, "Trace control" },
    
    {  0,  0,  0,  0,  0, NULL }
};

/* MCR/MRC p15, op1, Rd, CRn, CRm, op2 */

char *mrc_dump_get_desc (int cp, int crn, int op1, int crm, int op2)
{
    int pos = 0;

    while(cp_regs[pos].desc != NULL)
    {
        if(cp_regs[pos].cp == cp || cp_regs[pos].cp == -1)
        {
            if(cp_regs[pos].crn == crn || cp_regs[pos].crn == -1)
            {
                if(cp_regs[pos].op1 == op1 || cp_regs[pos].op1 == -1)
                {
                    if(cp_regs[pos].crm == crm || cp_regs[pos].crm == -1)
                    {
                        if(cp_regs[pos].op2 == op2 || cp_regs[pos].op2  == -1)
                        {
                            return cp_regs[pos].desc;
                        }
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
    void *alloc_buf = malloc(32);
    void *func_buf = UNCACHEABLE(alloc_buf);
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

    free(alloc_buf);
}

static MENU_UPDATE_FUNC(mrc_dump_update_all)
{
    if (!info->can_custom_draw) return;
    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    int skip = (mrc_dump_page - 1) * (450 / font_large.height);
    int k = 0;
    int y = 0;
    int printed = 0;

    for(int pos = 0; pos < COUNT(cp_regs); pos++)
    {
        char *str = cp_regs[pos].desc;

        if(!str)
        {
            break;
        }

        k++;
        if (k <= skip)
        {
            continue;
        }

        mrc_cp = cp_regs[pos].cp;
        mrc_crn = cp_regs[pos].crn;
        mrc_op1 = cp_regs[pos].op1;
        mrc_crm = cp_regs[pos].crm;
        mrc_op2 = cp_regs[pos].op2;
        mrc_dump_process();

        printed = 1;

        if(k%2)
        {
            bmp_fill(COLOR_GRAY(10), 0, y, 720, font_large.height);
        }

        int yasm = y + font_small.height;
        bmp_printf(
            SHADOW_FONT(FONT_SMALL), 10, y,
            "%s", str
        );

        bmp_printf(
            SHADOW_FONT(FONT_MED), 10, yasm,
            "MRC p%d, %d, Rd, c%d, c%d, %d",
            mrc_cp, mrc_op1, mrc_crn, mrc_crm, mrc_op2
        );

        bmp_printf(
            SHADOW_FONT(FONT(FONT_LARGE, COLOR_YELLOW, COLOR_BLACK)),
            720 - 8*font_large.width, y,
            "%8x",
            mrc_value
        );

        y += font_large.height;

        if (y > 440)
        {
            bmp_printf(FONT(FONT_MED, COLOR_CYAN, COLOR_BLACK), 710 - 7*font_med.width, y, "more...");
            break;
        }
    }
    if (!printed)
    {
        mrc_dump_page = 1;
    }
}

static struct menu_entry mrc_dump_menu[] =
{
    {
        .name = "Show MRC regs",
        .select = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            {
                .name = "Read all",
                .update = mrc_dump_update_all,
                .priv = &mrc_dump_page,
                .max = 100,
                .icon_type = IT_ACTION,
            },
            MENU_EOL,
        },
    }
};

unsigned int mrc_dump_init()
{
    menu_add("Debug", mrc_dump_menu, COUNT(mrc_dump_menu));
    return 0;
}

unsigned int mrc_dump_deinit()
{
    menu_remove("Debug", mrc_dump_menu, COUNT(mrc_dump_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(mrc_dump_init)
    MODULE_DEINIT(mrc_dump_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

MODULE_PARAMS_START()
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()
