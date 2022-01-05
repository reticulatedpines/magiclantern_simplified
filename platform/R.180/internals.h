/**
 * Camera internals for EOS R 1.8.0
 */

/** This camera has a DIGIC VIII chip */
#define CONFIG_DIGIC_VIII

/** Digic 8 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

/** Execute platform prepare function in post_init_task **/
#define CONFIG_PLATFORM_POST_INIT

/** disable SRM for now
 * in current state SRM_AllocateMemoryResourceFor1stJob makes camera crash
 * even if just one buffer is requested.
 */
#define CONFIG_MEMORY_SRM_NOT_WORKING

/* has LV */
#define CONFIG_LIVEVIEW

/* hooking EFV_STATE ends with EvfCap crashes, requires investigation */
//#define CONFIG_STATE_OBJECT_HOOKS

/** Large total memory, leading to unusual memory mapping,
 * CACHEABLE / UNCACHEABLE changes
 */
#define CONFIG_MEM_2GB
