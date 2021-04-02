//#define CONFIG_HELLO_WORLD

#define FEATURE_VRAM_RGBA

// partially working
//#define FEATURE_SHOW_SHUTTER_COUNT
//#define FEATURE_SHOW_TASKS

// prevent ML attempting stack unwinding in some cases.
// This does not yet work (assumes ARM, not Thumb).  Alex recommends
// a good looking fix:
// http://www.mcternan.me.uk/ArmStackUnwinding/
#undef CONFIG_CRASH_LOG
#undef CONFIG_TSKMON
#undef CONFIG_PROP_REQUEST_CHANGE
#undef CONFIG_AUTOBACKUP_ROM
