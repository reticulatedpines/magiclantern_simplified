/**
 * Camera internals for 850D 1.0.0
 */

// This camera has a DIGIC VIII chip
#define CONFIG_DIGIC_VIII

// Digic 8 does not have bitmap font in ROM, try to load it from card
#define CONFIG_NO_BFNT

// has LV
#define CONFIG_LIVEVIEW

// enable state objects hooks
#define CONFIG_STATE_OBJECT_HOOKS

// Large total memory, leading to unusual memory mapping,
// CACHEABLE / UNCACHEABLE changes
#define CONFIG_MEM_2GB

#define CONFIG_NEW_TASK_STRUCTS
#define CONFIG_TASK_STRUCT_V2_SMP
#define CONFIG_TASK_ATTR_STRUCT_V5
