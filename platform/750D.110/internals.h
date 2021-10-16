/**
 * Camera internals for 750D 1.1.0
 */

/** This camera has a DIGIC VI chip */
#define CONFIG_DIGIC_VI

/** Digic 6 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

/** disable SRM for early port
 */
//#define CONFIG_MEMORY_SRM_NOT_WORKING
