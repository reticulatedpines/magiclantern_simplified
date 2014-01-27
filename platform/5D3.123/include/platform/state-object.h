#ifndef __platform_state_object_h
#define __platform_state_object_h

#define DISPLAY_STATE DISPLAY_STATEOBJ
#define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 20
#define EVF_STATE (*(struct state_object **)0x25F1c)
#define MOVREC_STATE (*(struct state_object **)0x277BC)
#define SSS_STATE (*(struct state_object **)0x25D54)

#endif // __platform_state_object_h
