/**
 * Camera internals for 5D Mark IV 1.1.2
 */

/** This camera has a DIGIC VI chip */
#define CONFIG_DIGIC_VI

/** Digic 6 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT