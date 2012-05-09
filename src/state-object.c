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

#ifdef CONFIG_550D
#define DISPLAY_STATE DISPLAY_STATEOBJ
#define MOVREC_STATE (*(struct state_object **)0x5B34)
#endif

#ifdef CONFIG_60D
#define VOI_STATE (*(struct state_object **)0x269D8)
#define EVF_STATE (*(struct state_object **)0x4ff8)
#define MOVREC_STATE (*(struct state_object **)0x5A40)
#endif

#ifdef CONFIG_600D
#define EVF_STATE (*(struct state_object **)0x51CC)
#define MOVREC_STATE (*(struct state_object **)0x5EF8)
#endif

#ifdef CONFIG_5D2
#define MOVREC_STATE (*(struct state_object **)0x7C90)
#define LV_STATE (*(struct state_object **)0x4528)
#endif

#ifdef CONFIG_500D
#define MOVREC_STATE (*(struct state_object **)0x7AF4)
#define LV_STATE (*(struct state_object **)0x4804)
#endif

#ifdef CONFIG_50D
#define MOVREC_STATE (*(struct state_object **)0x6CDC)
#define LV_STATE (*(struct state_object **)0x4580)
#endif

#ifdef CONFIG_5D3
#define EVF_STATE (*(struct state_object **)0x2600c)
#define MOVREC_STATE (*(struct state_object **)0x27850)
#endif

/*
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
*/

static void vsync_func() // called once per frame.. in theory :)
{
    #if !defined(CONFIG_60D) && !defined(CONFIG_600D)  && !defined(CONFIG_1100D) // for those cameras, we call it from cartridge_AfStopPath
    hdr_step();
    #endif
    
    digic_iso_step();
    image_effects_step();
}

int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    
    #ifdef MOVREC_STATE
    if (self == MOVREC_STATE && recording) // mvrEncodeDone
    {
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        if (self->current_state == 4 && input == 3) // mvrExpStarted
        #endif
        #ifndef CONFIG_550D
            vsync_func();
        #endif
    }
    #endif
    

    int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);
    int new_state = self->current_state;

    #ifdef MOVREC_STATE
    if (self == MOVREC_STATE && recording) // mvrEncodeDone
    {
        #ifdef CONFIG_550D
            vsync_func();
        #endif
        //~ bmp_printf(FONT_LARGE, 50, 50, "%d--(%d)-->%d %d ", old_state, input, new_state, MVR_FRAME_NUMBER);
        return ans;
    }
    #endif

    #ifdef LV_STATE
    if (self == LV_STATE && input == 2 && !recording)
    {
        vsync_func();
        return ans;
    }
    #endif
    
    #ifdef DISPLAY_STATE
    if (self == DISPLAY_STATE && input == 18 && !recording) // SetImageVramParameter_pFlipCBR
    {
        vsync_func();
        return ans;
    }
    #endif

    #ifdef EVF_STATE
    if (self == EVF_STATE && input == 3 && !recording)
    {
        vsync_func();
        return ans;
    }
    #endif

    return ans;
}

static int stateobj_start_spy(struct state_object * stateobj)
{
    StateTransition = stateobj->StateTransition_maybe;
    stateobj->StateTransition_maybe = stateobj_spy;
}

static void state_init(void* unused)
{
    #ifdef DISPLAY_STATE
        stateobj_start_spy(DISPLAY_STATE);
    #endif
    #ifdef LV_STATE
        stateobj_start_spy(LV_STATE);
    #endif
    #ifdef MOVREC_STATE
        stateobj_start_spy(MOVREC_STATE);
    #endif
    #ifdef EVF_STATE
        stateobj_start_spy(EVF_STATE);
    #endif
}

INIT_FUNC("state_init", state_init);
