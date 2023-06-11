/**
 * Camera internals for 6D2 1.0.5
 */

/** This camera has a DIGIC VII chip */
#define CONFIG_DIGIC_VII

/** Digic 7 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

#define CONFIG_NEW_TASK_STRUCTS
#define CONFIG_TASK_STRUCT_V2_SMP
#define CONFIG_TASK_ATTR_STRUCT_V5
