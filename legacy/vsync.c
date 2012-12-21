/** 
 * Vsync for LiveView
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "state-object.h"
#include "menu.h"
#include "lens.h"
#include "propvalues.h"
#include "config.h"

CONFIG_INT("vsync.state", vsync_state, 0);
CONFIG_INT("vsync.input", vsync_input, 0);

#ifdef CONFIG_550D
#define DISPLAY_STATE (*(struct state_object **)0x245c)
#define MOVREC_STATE (*(struct state_object **)0x5B34)
#define LV_STATE (*(struct state_object **)0x4B74)
#define LV_STRUCT_PTR 0x1d14
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x60)
#endif

#ifdef CONFIG_500D
#define MOVREC_STATE (*(struct state_object **)0x7AF4)
#define LV_STRUCT_PTR 0x1d78
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x56)
#endif

#ifdef CONFIG_50D
#define MOVREC_STATE (*(struct state_object **)0x6CDC)
#define LV_STRUCT_PTR 0x1D74
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#endif

#ifdef CONFIG_5D2
#define MOVREC_STATE (*(struct state_object **)0x7C90)
#define LV_STRUCT_PTR 0x1D78
#define FRAME_ISO *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5C)
#endif

#ifdef CONFIG_60D
#define EVF_STATE (*(struct state_object **)0x4ff8)
//~ #define DISPLAY_STATE (*(struct state_object **)0x2508)
#endif


int vsync_last_msg = 0;
struct msg_queue * vsync_msg_queue = 0;

void vsync_signal()
{
    msg_queue_post(vsync_msg_queue, 1);
}

void lv_vsync()
{
    int msg;
    msg_queue_receive(vsync_msg_queue, &msg, 0);
    //~ NotifyBox(1000, "vsync recv %d", msg);
    //~ if (video_mode_resolution == 0 || !is_movie_mode()) msleep(10);
}


static int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);

    //~ bmp_printf(FONT_LARGE, 100, 100, "%d %d ", input, self->current_state);
    //~ if (self == EVF_STATE && input == 5)
    if (self == DISPLAY_STATE && (vsync_input == 0 || input == vsync_input) && (vsync_state == 0 || old_state == vsync_state))
        vsync_signal();

    return ans;
}

static void stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = (void*)stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = (void*)stateobj_spy;
}


static struct menu_entry vsync_menu[] = {
    {
        .name = "VSync state",
        .priv       = &vsync_state,
        .min = 0,
        .max = 100,
    },
    {
        .name = "VSync input",
        .priv       = &vsync_input,
        .min = 0,
        .max = 100,
    }
};


static void vsync_init()
{
    menu_add( "Debug", vsync_menu, COUNT(vsync_menu) );
    vsync_msg_queue = msg_queue_create("vsync_mq", 1);
    stateobj_start_spy(DISPLAY_STATE);
}

INIT_FUNC("vsync", vsync_init);
