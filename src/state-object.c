/** 
 * Experiments on state objects 
 * 
 * http://magiclantern.wikia.com/wiki/StateObjects
 * 
 **/


#include "dryos.h"
#include "bmp.h"
#include "beep.h"
#include "state-object.h"
#include <platform/state-object.h>
#include "property.h"
#include "fps.h"
#include "module.h"

/* to refactor with CBR */
extern void lv_vsync_signal();
extern void hdr_step();
extern void raw_lv_vsync();
extern int hdr_kill_flicker();
extern void digic_zoom_overlay_step(int force_off);
extern void vignetting_correction_apply_regs();
extern void raw_buffer_intercept_from_stateobj();
extern int display_filter_lv_vsync(int old_state, int x, int input, int z, int t);

#ifdef CONFIG_STATE_OBJECT_HOOKS

/*
static void stateobj_matrix_copy_for_patching(struct state_object * stateobj)
{
    int size = stateobj->max_inputs * stateobj->max_states * sizeof(struct state_transition);
    struct state_transition * new_matrix = (struct state_transition *)malloc(size);
    memcpy(new_matrix, stateobj->state_matrix, size);
    stateobj->state_matrix = new_matrix;
}

static void stateobj_install_hook(struct state_object * stateobj, int input, int state, void* newfunc)
{
    if ((uint32_t)(stateobj->state_matrix) & 0xFF000000) // that's in ROM, make a copy to allow patching
        stateobj_matrix_copy_for_patching(stateobj);
    STATE_FUNC(stateobj,input,state) = newfunc;
}
*/

static volatile int vsync_counter = 0;
#ifndef CONFIG_7D_MASTER
/* waits for N LiveView frames */
int wait_lv_frames(int num_frames)
{
    #ifdef CONFIG_QEMU
    return 0;   /* fixme: call the vsync hook from qemu-util */
    #endif
    
    vsync_counter = 0;
    int count = 0;
    int frame_duration = 1000000 / fps_get_current_x1000();
    while (vsync_counter < num_frames)
    {
        /* handle FPS override changes during the wait */
        frame_duration = MAX(frame_duration, 1000000 / fps_get_current_x1000());
        msleep(20);
        count++;
        if (count > num_frames * frame_duration * 2 / 20)
        {
            /* timeout */
            return 0;
        }
        if (!lv)
        {
            /* LiveView closed */
            return 0;
        }
    }
    return 1;
}
#endif
static void FAST vsync_func() // called once per frame.. in theory :)
{
    vsync_counter++;

    #if defined(CONFIG_MODULES)
    module_exec_cbr(CBR_VSYNC);
    #endif
    
    #ifdef CONFIG_EDMAC_RAW_SLURP
    raw_lv_vsync();
    #endif

    #if !defined(CONFIG_EVF_STATE_SYNC)
    // for those cameras, it's called from a different spot of the evf state object
    hdr_step();
    #endif

    #if !defined(CONFIG_DIGIC_V) && !defined(CONFIG_7D)
    vignetting_correction_apply_regs();
    #endif

    #ifdef FEATURE_FPS_OVERRIDE
    #ifdef CONFIG_FPS_UPDATE_FROM_EVF_STATE
    extern void fps_update_timers_from_evfstate();
    fps_update_timers_from_evfstate();
    #endif
    #endif

    extern void digic_iso_step();
    digic_iso_step();
    
    extern void image_effects_step();
    image_effects_step();

    #ifdef FEATURE_DISPLAY_SHAKE
    display_shake_step();
    #endif
}

#ifdef CONFIG_550D
int display_is_on_550D = 0;
int get_display_is_on_550D() { return display_is_on_550D; }
#endif

#ifdef FEATURE_SHOW_STATE_FPS
#define num_states 4
#define num_inputs 32
static int state_matrix[num_states][num_inputs];
#endif

static int (*StateTransition)(void*,int,int,int,int) = 0;
static int FAST stateobj_lv_spy(struct state_object * self, int x, int input, int z, int t)
{
    int old_state = self->current_state;

#ifdef FEATURE_SHOW_STATE_FPS
    if (self == DISPLAY_STATE) {
        state_matrix[old_state][input]++;
    }
#endif
#ifdef CONFIG_550D
    if (self == DISPLAY_STATE && old_state != 0 && input == 0) // TurnOffDisplay_action
        display_is_on_550D = 0;
#endif

// sync ML overlay tools (especially Magic Zoom) with LiveView
// this is tricky...
#if defined(CONFIG_DIGIC_V)
    if (self == DISPLAY_STATE && (input == INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER))
        lv_vsync_signal();
#elif defined(CONFIG_5D2)
    if (self == LV_STATE)//&& old_state == 4)
    {
        //~ lv_vsync_signal();
    }
#elif defined(CONFIG_60D)
    if (self == EVF_STATE && input == 5 && old_state == 5) // evfReadOutDoneInterrupt
        lv_vsync_signal();
#elif defined(CONFIG_600D)
    if (self == EVF_STATE && old_state == 5) {  
		//600D Goes 3 - 4 - 5 5 and 3 ever 1/2 frame
        lv_vsync_signal();
	}
#endif
    // sync display filters (for these, we need to redirect display buffers
    #ifdef DISPLAY_STATE
    #ifdef CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
    if (self == DISPLAY_STATE && input == INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER)
    {
        #ifdef FEATURE_HDR_VIDEO
        if (hdr_kill_flicker() == CBR_RET_CONTINUE)
        #endif
        #ifdef CONFIG_DISPLAY_FILTERS
        if (display_filter_lv_vsync(old_state, x, input, z, t) == CBR_RET_CONTINUE)
        {
            #ifdef FEATURE_MAGIC_ZOOM
            digic_zoom_overlay_step(0);
            #endif
        }
        #endif
    }
    #endif
    #endif
    
#if defined(CONFIG_5D2) || defined(CONFIG_50D)
    if (self == LV_STATE && old_state == 2 && input == 2) // lvVdInterrupt
    {
        display_filter_lv_vsync(old_state, x, input, z, t);
    }
#endif

    int ans = StateTransition(self, x, input, z, t);

#ifdef CONFIG_550D
    if (self == DISPLAY_STATE)
        display_is_on_550D = (self->current_state == 1);
#endif


// sync digic functions (like overriding ISO or image effects)

    #if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_500D)
    if (self == LV_STATE && input==4 && old_state==4) // AJ_ResetPSave_n_WB_n_LVREC_MVR_EV_EXPOSURESTARTED => perfect sync for digic on 5D2 :)
    #elif defined(CONFIG_550D)
    if (self == LV_STATE && input==5 && old_state == 5) // SYNC_GetEngineResource => perfect sync for digic :)
    #elif defined(CONFIG_EVF_STATE_SYNC)
    if (self == EVF_STATE && input == 5 && old_state == 5) // evfReadOutDoneInterrupt => perfect sync for digic :)
    #else
    if (0)
    #endif
    {
        vsync_func();
    }
    
    #if defined(CONFIG_7D_MASTER) || defined(CONFIG_7D)
    if (self == LV_STATE && input==3 && old_state == 3) {
        extern void vignetting_correction_apply_lvmgr(int);
        vignetting_correction_apply_lvmgr(x);
    }
    #endif
    
    #if !defined(CONFIG_7D_MASTER) && defined(CONFIG_7D)
    if (self == LV_STATE && input==5 && old_state == 5)       
    { 
        display_filter_lv_vsync(old_state, x, input, z, t);
        vsync_func();
    }
    #endif
    #ifdef EVF_STATE
    if (self == EVF_STATE && input == 4 && old_state == 5) // evfSetParamInterrupt
    {
        #if defined(CONFIG_EVF_STATE_SYNC) // exception for overriding ISO
        hdr_step();
        #endif
        
        #ifdef CONFIG_DIGIC_V
        vignetting_correction_apply_regs();
        #endif
    }
    #endif
    return ans;
}

#ifdef CONFIG_5DC
static int stateobj_em_spy(struct state_object * self, int x, int input, int z, int t)
{
    int ans = StateTransition(self, x, input, z, t);

    if (z == 0x0) { fake_simple_button(BGMT_PRESS_HALFSHUTTER); }
    if (z == 0xB) { fake_simple_button(BGMT_UNPRESS_HALFSHUTTER); }
    return ans;
}
#endif

static int stateobj_start_spy(struct state_object * stateobj, void* spy)
{
    ASSERT(streq(stateobj->type, "StateObject"));

    if (!StateTransition)
        StateTransition = (void *)stateobj->StateTransition_maybe;
    
    // double check if all states use the same transition function (they do, in theory)
    else if ((void*)StateTransition != (void*)stateobj->StateTransition_maybe)
    {
        #ifndef CONFIG_7D_MASTER
        beep();
        #endif
        return 1;
    }
    
    stateobj->StateTransition_maybe = spy;
    return 0; //not used currently
}

static void state_init(void* unused)
{
    #ifdef DISPLAY_STATE
        stateobj_start_spy(DISPLAY_STATE, stateobj_lv_spy);
    #endif
    #ifdef LV_STATE
        while(!LV_STATE) msleep(50);
        stateobj_start_spy(LV_STATE, stateobj_lv_spy);
    #endif
    #ifdef EVF_STATE
        stateobj_start_spy(EVF_STATE, stateobj_lv_spy);
    #endif
    
    #ifdef EMState
        stateobj_start_spy(EMState, stateobj_em_spy);
    #endif

    #ifdef CONFIG_550D
    display_is_on_550D = (DISPLAY_STATEOBJ->current_state != 0);
    #endif
}

#ifndef CONFIG_QEMU // in QEMU, the state objects are not initialized, so just skip this for now to avoid infinite loops
INIT_FUNC("state_init", state_init);
#endif

#ifdef FEATURE_SHOW_STATE_FPS
void update_state_fps() {
    NotifyBox(1000,"Logging");
    FILE* state_log_file = 0;
    state_log_file = FIO_CreateFile("state.log");
    if(state_log_file) {
        for(int i=0;i<num_states;++i) {
            for(int j=0;j<num_inputs;++j) {
                if(state_matrix[i][j]) {
                    my_fprintf(state_log_file,"%02d %02d %03d\n", i, j, state_matrix[i][j]);
                    state_matrix[i][j] = 0;
                }
            }
        }
        FIO_CloseFile(state_log_file);
        state_log_file = 0;
    }
    NotifyBox(1000,"Done");

}
#endif

#endif
