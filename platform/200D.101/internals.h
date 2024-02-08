/**
 * Camera internals for 200D 1.0.0
 */

// This camera has a DIGIC VII chip
#define CONFIG_DIGIC_VII

// has inter-core RPC (so far this has always been dependent on SGI, 0xc)
#define CONFIG_RPC

// Cam has MMU (by itself, does nothing, see CONFIG_MMU_REMAP)
#define CONFIG_MMU

// Cam can wrap init1, allowing control of cpu1 before tasks are started
#define CONFIG_INIT1_HIJACK

// This camera loads ML into the AllocateMemory pool
#define CONFIG_ALLOCATE_MEMORY_POOL

// Digic 7 does not have bitmap font in ROM, try to load it from card
#define CONFIG_NO_BFNT

// No movie mode on the main dial
#define CONFIG_NO_DEDICATED_MOVIE_MODE

// wanted to get raw_rec_vsync_cbr running via vsync_func,
// for mlv_lite
#define CONFIG_EVF_STATE_SYNC

// has LV
#define CONFIG_LIVEVIEW

// enable state objects hooks
#define CONFIG_STATE_OBJECT_HOOKS

#define CONFIG_NEW_TASK_STRUCTS
#define CONFIG_TASK_STRUCT_V2_SMP
#define CONFIG_TASK_ATTR_STRUCT_V5
