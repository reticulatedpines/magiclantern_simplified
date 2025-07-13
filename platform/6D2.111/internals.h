/**
 * Camera internals for 6D2 1.0.5
 */

/** This camera has a DIGIC VII chip */
#define CONFIG_DIGIC_VII

// has inter-core RPC (so far this has always been dependent on SGI, 0xc)
#define CONFIG_RPC

// Cam has MMU (by itself, does nothing, see CONFIG_MMU_REMAP)
#define CONFIG_MMU

/** Digic 7 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

// wanted to get raw_rec_vsync_cbr running via vsync_func,
// for mlv_lite
#define CONFIG_EVF_STATE_SYNC

// has LV
#define CONFIG_LIVEVIEW

// Does the mode *dial* have movie mode?
// 6D2 does not, even though it does have a dedicated switch for movie.
#define CONFIG_NO_DEDICATED_MOVIE_MODE

// enable state objects hooks
#define CONFIG_STATE_OBJECT_HOOKS

// SRM is untested, this define is to allowing building
// without SRM_BUFFER_SIZE being found
#define CONFIG_MEMORY_SRM_NOT_WORKING

#define CONFIG_MALLOC_STRUCT_V2

#define CONFIG_TASK_STRUCT_V2_SMP
#define CONFIG_TASK_ATTR_STRUCT_V5
