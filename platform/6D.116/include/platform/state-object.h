#ifndef __platform_state_object_h
#define __platform_state_object_h

#define DISPLAY_STATE DISPLAY_STATEOBJ
#define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 23
#define EVF_STATE (*(struct state_object**)0x76D6C)
#define SSS_STATE (*(struct state_object **)0x76BC8)
#define MOVREC_STATE (*(struct state_object **)0x7883C)

#endif
