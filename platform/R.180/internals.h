/**
 * Camera internals for EOS R 1.8.0
 */

/** This camera has a DIGIC VIII chip */
#define CONFIG_DIGIC_VIII

/** Digic 8 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

/** Execute platform prepare function in post_init_task **/
#define CONFIG_PLATFORM_POST_INIT
