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

// wanted to get raw_rec_vsync_cbr running via vsync_func,
// for mlv_lite
#define CONFIG_EVF_STATE_SYNC

// This camera has LiveView and can record video
#define CONFIG_LIVEVIEW

// Does the mode *dial* have movie mode?
#define CONFIG_NO_DEDICATED_MOVIE_MODE

// Has an EDMAC unit that we have some control over.
// V3 implies we can't directly do rectangular sub-region
// copies, but we can do fast copies.  This allows removing
// top and bottom border easily, but not the sides.
#define CONFIG_EDMAC_MEMCPY
#define CONFIG_EDMAC_COPY_RECT_V3

// enable state objects hooks
#define CONFIG_STATE_OBJECT_HOOKS

// SRM is untested, this define is to allowing building
// without SRM_BUFFER_SIZE being found
//#define CONFIG_MEMORY_SRM_NOT_WORKING

// Cam has very few spare tasks for ML, steal more mem
// during boot to raise the limit
#define CONFIG_INCREASE_MAX_TASKS 7

#define CONFIG_MALLOC_STRUCT_V2

#define CONFIG_TASK_STRUCT_V2
#define CONFIG_TASK_ATTR_STRUCT_V3
