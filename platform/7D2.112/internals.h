/**
 * Camera internals for 7D2 1.1.2
 */

// This camera has both a CF and an SD slot
#define CONFIG_DUAL_SLOT

// This camera has a Dual DIGIC VI chip
#define CONFIG_DIGIC_VI
#define CONFIG_DUAL_DIGIC

// Digic 6 does not have bitmap font in ROM, try to load it from card
#define CONFIG_NO_BFNT

// This camera has LiveView and can record video
#define CONFIG_LIVEVIEW

// enable state objects hooks
#define CONFIG_STATE_OBJECT_HOOKS
