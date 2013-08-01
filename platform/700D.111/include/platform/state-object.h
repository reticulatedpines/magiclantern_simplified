#ifndef __platform_state_object_h
#define __platform_state_object_h

#define DISPLAY_STATE DISPLAY_STATEOBJ
    #define INPUT_SET_IMAGE_VRAM_PARAMETER_MUTE_FLIP_CBR 23
    #define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 24
#define EVF_STATE (*(struct state_object **)0x25B0C)
#define SSS_STATE (*(struct state_object **)0x257C4)

#endif // __platform_state_object_h
