/**
 * Camera internals for EOS R5 1.5.2
 */

/** This camera has a DIGIC X chip */
#define CONFIG_DIGIC_X

/** Digic 6 and up does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

/** disable SRM for now
 * in current state SRM_AllocateMemoryResourceFor1stJob makes camera crash
 * even if just one buffer is requested.
 */
#define CONFIG_MEMORY_SRM_NOT_WORKING

#define CONFIG_MALLOC_STRUCT_V2

#define CONFIG_TASK_STRUCT_V2_SMP
#define CONFIG_TASK_ATTR_STRUCT_V5
