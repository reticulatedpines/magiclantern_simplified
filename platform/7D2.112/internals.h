/**
 * Camera internals for 7D2 1.1.2
 */

// This camera has both a CF and an SD slot
#define CONFIG_DUAL_SLOT

// This camera has a Dual DIGIC VI chip
#define CONFIG_DIGIC_VI
#define CONFIG_DUAL_DIGIC

// Digic 6 does not have bitmap font in ROM, try to load it from card
#define CONFIG_NO_BFNT

// This camera has LiveView and can record video
#define CONFIG_LIVEVIEW

// enable state objects hooks
#define CONFIG_STATE_OBJECT_HOOKS

// SRM is untested, this define is to allowing building
// without SRM_BUFFER_SIZE being found
#define CONFIG_MEMORY_SRM_NOT_WORKING

// SJE FIXME this is intended as temporary, so I can commit the code
// without needing to find versions for all cams.  Once all cams
// are converted and tested, we should retire CONFIG_NEW_TASK_STRUCTS
#define CONFIG_NEW_TASK_STRUCTS
#define CONFIG_TASK_STRUCT_V2
#define CONFIG_TASK_ATTR_STRUCT_V3
