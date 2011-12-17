/** 
 * Experiments on state objects 
 * 
 * http://magiclantern.wikia.com/wiki/StateObjects
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "state-object.h"

#define voi_state (*(struct state_object **)0x269D8)
#define evf_state (*(struct state_object **)0x4ff8)
#define movrec_state (*(struct state_object **)0x5A40)

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
    bmp_printf(FONT_LARGE, 50, 50, "%s (%d)--%d-->(%d) ", self->name, old_state, input, new_state);
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

static void state_task(void* unused)
{
    msleep(3000);
    stateobj_start_spy(movrec_state);
    while(1)
    {
        msleep(100);
        loop_test();
    }
}

TASK_CREATE("state_task", state_task, 0, 0x1d, 0x1000 );
