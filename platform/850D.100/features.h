//#define CONFIG_HELLO_WORLD

#define FEATURE_VRAM_RGBA
#define FEATURE_SHOW_SHUTTER_COUNT
#define FEATURE_SHOW_TOTAL_SHOTS

// prevent ML attempting stack unwinding in some cases.
// This does not yet work (assumes ARM, not Thumb).  Alex recommends
// a good looking fix:
// http://www.mcternan.me.uk/ArmStackUnwinding/
#undef CONFIG_CRASH_LOG
#undef CONFIG_PROP_REQUEST_CHANGE
#undef CONFIG_ADDITIONAL_VERSION
#undef CONFIG_AUTOBACKUP_ROM
