#ifndef __platform_state_object_h
#define __platform_state_object_h

#define EVF_STATE (*(struct state_object **)0x404E4)
#define SSS_STATE (*(struct state_object **)0x4018C)
#define MOVREC_STATE (*(struct state_object **)0x42254)
#define DISPLAY_STATE DISPLAY_STATEOBJ //Works for Photo Overlay
#define INPUT_SET_IMAGE_VRAM_PARAMETER_MUTE_FLIP_CBR 26
#define INPUT_ENABLE_IMAGE_PHYSICAL_SCREEN_PARAMETER 27
	//#define FACE_STATE (*(struct state_object **)0x40A4C)
	//#define DISP2_STATE (*(struct state_object **)0x3EBBC)
	//#define LVCDEV_STATE (*(struct state_object **)0x43978)
	//#define CLR_CALC_STATE (*(struct state_object **)0x43B74) 
	//~ #define DISPLAY_STATE FACE_STATE

#endif // __platform_state_object_h
