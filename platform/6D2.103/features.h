//#define CONFIG_HELLO_WORLD

#define FEATURE_VRAM_RGBA

// Don't Click Me menu looks to be intended as a place
// for devs to put custom code in debug.c run_test(),
// and allowing triggering from a menu context.
//#define FEATURE_DONT_CLICK_ME

// prevent ML attempting stack unwinding in some cases.
// This does not yet work (assumes ARM, not Thumb).  Alex recommends
// a good looking fix:
// http://www.mcternan.me.uk/ArmStackUnwinding/
#undef CONFIG_CRASH_LOG

#undef CONFIG_ADDITIONAL_VERSION
#undef CONFIG_PROP_REQUEST_CHANGE
#undef CONFIG_AUTOBACKUP_ROM
