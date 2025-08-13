#ifndef __platform_state_object_h
#define __platform_state_object_h

#define DISPLAY_STATE DISPLAY_STATEOBJ
#define INPUT_SET_IMAGE_VRAM_PARAMETER_MUTE_FLIP_CBR 22
#define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 23
#define EVF_STATE (*(struct state_object **)0x7CFEC) // ok for 70D nikfreak
#define MOVREC_STATE (*(struct state_object **)0x7CE48) // ok for 70D nikfreak
#define SSS_STATE (*(struct state_object **)0x91BD8) // ok for 70D nikfreak

#endif // __platform_state_object_h