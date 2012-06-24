/** 
 * Spy the dmstate object and steal debug messages
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "state-object.h"
#include "config.h"
#include "menu.h"

#ifdef CONFIG_60D
#define dm_state (*(struct state_object **)0x7318)
#define MAX_MSG 3119 // trial and error, try with gui messages
#endif

static CONFIG_INT("dm.enable", dm_enable, 0);
static CONFIG_INT("dm.global.level", dm_global_level, 0xff);
static CONFIG_INT("dm.class", dm_class, 137);
static CONFIG_INT("dm.class.level", dm_class_level, 0);

FILE* logfile = 0;
static void log_msg(int k)
{
    int a = MEM(MEM(0x2D14)+0x10);
    //~ int b = a + 85 * MEM(8 + a + 3*60);
    static int y = 0;

    //~ if (logfile)
    /*
    my_fprintf(logfile,
    //~ bmp_printf(FONT_SMALL, 0, y,
        "%d:%s\n",
        //~ "%d:%s\n                                                                                          ",
        k, a + 0x88 + k * 84
    );*/
    y += font_small.height;
    if (y > 450) y = 0;

    MEM(a + 0x88 + k * 84 + 30) = 0;
    NotifyBox(5000, "%d:\n%s", k, a + 0x88 + k * 84);

}

static int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    int ans = StateTransition(self, x, input, z, t);
    if (input == 2)
    {
        static int last = -1;
        for (int k = last+1; k <= t; k++)
        {
            log_msg(k % MAX_MSG);
            last = k;
        }
    }
    return ans;
}

static void stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = (void*)stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = (void*)stateobj_spy;
}

static void dm_update()
{
    if (dm_enable)
    {
        dmstart();
        FIO_RemoveFile(CARD_DRIVE "ML/LOGS/dm.log");
        logfile = FIO_CreateFile(CARD_DRIVE "ML/LOGS/dm.log");
    }
    else
    {
        dmstop();
        FIO_CloseFile(logfile);
        logfile = 0;
    }

    dm_set_store_level(0xFF, dm_global_level);
    dm_set_print_level(0xFF, dm_global_level);

    dm_set_store_level(dm_class, dm_class_level);
    dm_set_print_level(dm_class, dm_class_level);
}

static void dm_toggle(void* priv, int dir)
{
    dm_enable = !dm_enable;
    dm_update();
}

static struct menu_entry debug_menus[] = {
    {
        .name = "Debug Logging",
        .priv = &dm_enable,
        .max = 1,
        .select = dm_toggle,
        .help = "Enable debug logging.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Log class",
                .priv = &dm_class,
                .min = 0,
                .max = 255,
                .help = "Choose which class of messages to log."
            },
            {
                .name = "Class level",
                .priv = &dm_class_level,
                .min = 0,
                .max = 255,
                .help = "Debug level for selected class. 0 = all messages."
            },
            {
                .name = "Global level",
                .priv = &dm_global_level,
                .min = 0,
                .max = 255,
                .help = "Global debug level. Higher = less messages saved."
            },
            MENU_EOL
        },
    },
};

static void dmspy_task(void* unused)
{
    menu_add("Debug", debug_menus, COUNT(debug_menus));
    dm_update();
    DEBUG("hello world");
    stateobj_start_spy(dm_state);
    TASK_RETURN;
}

TASK_CREATE("dmspy_task", dmspy_task, 0, 0x1d, 0x1000 );

