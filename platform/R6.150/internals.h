/**
 * Camera internals for EOS R6 1.5.0
 */

/** This camera has a DIGIC VIII chip */
// Actually, Digic X, see WIP for proper support:
// https://github.com/reticulatedpines/magiclantern_simplified/pull/62
// but this is not yet ready for merge
#define CONFIG_DIGIC_VIII

/** Digic 8 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

/** Execute platform prepare function in post_init_task **/
#define CONFIG_PLATFORM_POST_INIT

/** disable SRM for now
 * in current state SRM_AllocateMemoryResourceFor1stJob makes camera crash
 * even if just one buffer is requrested.
 */
#define CONFIG_MEMORY_SRM_NOT_WORKING

/* has LV */
#define CONFIG_LIVEVIEW

/* enable state objects hooks */
#define CONFIG_STATE_OBJECT_HOOKS
