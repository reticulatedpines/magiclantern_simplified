/**
 * Camera internals for 90D 1.1.1
 */

// This camera has a DIGIC VIII chip
#define CONFIG_DIGIC_VIII

// Digic 8 does not have bitmap font in ROM, try to load it from card
#define CONFIG_NO_BFNT

// has LV
#define CONFIG_LIVEVIEW

// enable state objects hooks
//#define CONFIG_STATE_OBJECT_HOOKS

// Large total memory, leading to unusual memory mapping,
// CACHEABLE / UNCACHEABLE changes
#define CONFIG_MEM_2GB
