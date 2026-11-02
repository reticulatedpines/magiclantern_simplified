#ifndef __platform_state_object_h
#define __platform_state_object_h

#define DISPLAY_STATE DISPLAY_STATEOBJ
#define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 20
#define EVF_STATE (*(struct state_object **)0x48EC)        // mk11174
#define MOVREC_STATE (*(struct state_object **)0x5664)     // mk11174
#define SDS_FRONT3_STATE (*(struct state_object **)0x31CC) // mk11174 still not 100%, maybe alex can confirm?

#endif // __platform_state_object_h
