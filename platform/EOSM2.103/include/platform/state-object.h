#ifndef __platform_state_object_h
#define __platform_state_object_h

#define DISPLAY_STATE DISPLAY_STATEOBJ
#define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 28
#define EVF_STATE    (*(struct state_object **)0x91CF0)
#define MOVREC_STATE (*(struct state_object **)0x93AF8)
#define SSS_STATE    (*(struct state_object **)0x9169C)

/** Unused **/
// #define INPUT_SET_IMAGE_VRAM_PARAMETER_MUTE_FLIP_CBR ??
// #define INPUT_SET_IMAGE_VRAM_PARAMETER_MUTE_FLIP_CBR 26
#endif // __platform_state_object_h
