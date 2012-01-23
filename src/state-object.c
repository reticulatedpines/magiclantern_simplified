/** 
 * Experiments on state objects 
 * 
 * http://magiclantern.wikia.com/wiki/StateObjects
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "state-object.h"
#include "property.h"

#ifdef CONFIG_60D
#define voi_state (*(struct state_object **)0x269D8)
#define evf_state (*(struct state_object **)0x4ff8)
#define movrec_state (*(struct state_object **)0x5A40)
#endif

#ifdef CONFIG_600D
#define evf_state (*(struct state_object **)0x51CC)
#endif


static void stateobj_matrix_copy_for_patching(struct state_object * stateobj)
{
    int size = stateobj->max_inputs * stateobj->max_states * sizeof(struct state_transition);
    struct state_transition * new_matrix = (struct state_transition *)AllocateMemory(size);
    memcpy(new_matrix, stateobj->state_matrix, size);
    stateobj->state_matrix = new_matrix;
}

static void stateobj_install_hook(struct state_object * stateobj, int input, int state, void* newfunc)
{
    if ((uint32_t)(stateobj->state_matrix) & 0xFF000000) // that's in ROM, make a copy to allow patching
        stateobj_matrix_copy_for_patching(stateobj);
    STATE_FUNC(stateobj,input,state) = newfunc;
}

int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);
    int new_state = self->current_state;

    //~ if (HALFSHUTTER_PRESSED)
    {
        if (input == 3)
            digic_scan_step();
    }
    //~ bmp_printf(FONT_LARGE, 50, 50, "%s (%d)--%d-->(%d) ", self->name, old_state, input, new_state);
    return ans;
}

static int stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = stateobj_spy;
}


static void loop_test()
{
    bmp_printf(FONT_LARGE, 50, 100, "state: %x %d ", evf_state->state_matrix, evf_state->current_state);
}

FILE* log = 0;

static void state_task(void* unused)
{
    msleep(5000);
    beep();
    FIO_RemoveFile(CARD_DRIVE "digic.log");
    log = FIO_CreateFile(CARD_DRIVE "digic.log");
    stateobj_start_spy(evf_state);
    /*while(1)
    {
        msleep(100);
        loop_test();
    }*/
}

TASK_CREATE("state_task", state_task, 0, 0x1d, 0x1000 );


int R     = 0xC0F10000;
int R_end = 0xC0FF0000;
int V = 0;

void check_outcome()
{
    int d = get_spot_motion(150, get_global_draw());
    int y,u,v = 0;
    get_spot_yuv(200, &y, &u, &v);
    int del = MEMX(R) - V;
    if (log)
        my_fprintf(log, "%8x = %8x%s%3d => diff=%4d, yuv=(%d,%d,%d)\n",
            R, V, del>0 ? "+" : "-", ABS(del),
            d, y, u, v
        );
    bmp_printf(FONT_LARGE, 0, 50, "%x=%x => %d    ", R, MEMX(R), d);
}

int lv_refreshed = 0;
void lv_refresh()
{
    PauseLiveView();
    ResumeLiveView();
    msleep(500);
    lv_refreshed = 1;
}

int delta[] = {1,2,3,5,10,-1,-2,-5,-10,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000};
void digic_scan_step()
{
    static int k = 0;

    if (k == 0) // init
    {
        V = MEMX(R);
    }

    if (k < 70)
    {
        // change the value in current digic register
        EngDrvOut(R, V + delta[k/5]);

        if (k % 5 == 4) // check to see what happened
        {
            //~ check_outcome();
            task_create("check_outcome", 0x1c, 0, check_outcome, 0);
        }
    }
    
    k++;
    if (k == 70)  // restore original value and refresh liveview to cancel any side effects from invalid digic commands
    {
        EngDrvOut(R, V);
        lv_refreshed = 0;
        task_create("lv_refresh", 0x1c, 0, lv_refresh, 0);
    }
    if (k > 70 && lv_refreshed) // liveview refreshed, ready for next step
    {
        R += 4;
        if (R > R_end)
        {
            if (log) FIO_CloseFile(log);
            log = 0;
            NotifyBox(10000, "Done :)");
            beep();
        }
        else
        {
            if (log) my_fprintf(log, "\n");
            k = 0;
        }
    }
}
