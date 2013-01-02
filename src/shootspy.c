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

// 550D:
//~ #define SCS_STATE (*(struct state_object **)0x31cc)
//~ #define SCSES_STATE (*(struct state_object **)0x31D0)
//~ #define SCSSR_STATE (*(struct state_object **)0x31D4)

// 5D2:
#define SCS_STATE (*(struct state_object **)0x3168)
#define SCSES_STATE (*(struct state_object **)0x316c)
#define SCSSR_STATE (*(struct state_object **)0x3170)

#define SBS_STATE (*(struct state_object **)0x31C4)
#define SDS_REAR_STATE (*(struct state_object **)0x363C)
#define SDS_FRONT1_STATE (*(struct state_object **)0x36B0)
#define SDS_FRONT2_STATE (*(struct state_object **)0x36B4)
#define SDS_FRONT3_STATE (*(struct state_object **)0x36B8)
#define SDS_FRONT4_STATE (*(struct state_object **)0x36BC)

#define SPS_STATE (*(struct state_object **)0x320C)
#define FSS_STATE (*(struct state_object **)0x3c24)
#define FCS_STATE (*(struct state_object **)0x3c34)

#define LOG_SIZE 10000
static char log[LOG_SIZE] = "";

static int (*StateTransition)(void*,int,int,int,int) = 0;
static int stateobj_spy(struct state_object * self, int x, int input, int z, int t)
{
    int old_state = self->current_state;
    int ans = StateTransition(self, x, input, z, t);
    int new_state = self->current_state;
    
    if (self == SBS_STATE) { STR_APPEND(log, "SBS  :"); }
    else if (self == SCS_STATE) { STR_APPEND(log, "SCS  :"); }
    else if (self == SCSES_STATE) { STR_APPEND(log, "SCSes:"); }
    else if (self == SCSSR_STATE) { STR_APPEND(log, "SCSsr:"); }
    else if (self == SDS_REAR_STATE) { STR_APPEND(log, "SDSr :"); }
    else if (self == SDS_FRONT1_STATE) { STR_APPEND(log, "SDSf1:"); }
    else if (self == SDS_FRONT2_STATE) { STR_APPEND(log, "SDSf2:"); }
    else if (self == SDS_FRONT3_STATE) { STR_APPEND(log, "SDSf3:"); }
    else if (self == SDS_FRONT4_STATE) { STR_APPEND(log, "SDSf4:"); }
    else if (self == SPS_STATE) { STR_APPEND(log, "SPS  :"); }
    else if (self == FSS_STATE) { STR_APPEND(log, "FSS  :"); }
    else if (self == FCS_STATE) { STR_APPEND(log, "FCS  :"); }
    
    STR_APPEND(log, "(%d) -- %2d -->(%d)\n", old_state, input, new_state);
    
    return ans;
}

static int stateobj_start_spy(struct state_object * stateobj)
{
    if (!StateTransition)
        StateTransition = (void *)stateobj->StateTransition_maybe;
    
    else if ((void*)StateTransition != (void*)stateobj->StateTransition_maybe) // make sure all states use the same transition function
    {
        beep();
        return;
    }
  stateobj->StateTransition_maybe = (void *)stateobj_spy;
  return 0; //not used currently
}

static void shootspy_init(void* unused)
{
    log[0] = 0;
    stateobj_start_spy(SCS_STATE);
    stateobj_start_spy(SCSES_STATE);
    stateobj_start_spy(SCSSR_STATE);
    stateobj_start_spy(SBS_STATE);
    stateobj_start_spy(SDS_REAR_STATE);
    stateobj_start_spy(SDS_FRONT1_STATE);
    stateobj_start_spy(SDS_FRONT2_STATE);
    stateobj_start_spy(SDS_FRONT3_STATE);
    stateobj_start_spy(SDS_FRONT4_STATE);
    stateobj_start_spy(SPS_STATE);
    stateobj_start_spy(FSS_STATE);
    stateobj_start_spy(FCS_STATE);
}

void save_log()
{
    NotifyBox(1000, "%d ", strlen(log));
    FILE* f = FIO_CreateFileEx(CARD_DRIVE"shoot.log");
    FIO_WriteFile(f, UNCACHEABLE(log), strlen(log));
    FIO_CloseFile(f);
    beep();
}

INIT_FUNC("shootspy_init", shootspy_init);
