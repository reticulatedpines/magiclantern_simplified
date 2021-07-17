#define FEATURE_VRAM_RGBA

//enable FEATURE_COMPOSITOR_XCM only in full build
#ifndef ML_MINIMAL_OBJ
#define FEATURE_COMPOSITOR_XCM
#endif

// Don't Click Me menu looks to be intended as a place
// for devs to put custom code in debug.c run_test(),
// and allowing triggering from a menu context.
#define FEATURE_DONT_CLICK_ME

#define FEATURE_SHOW_SHUTTER_COUNT
#define FEATURE_SHOW_TOTAL_SHOTS

// working but incomplete, some allocators don't report
// anything yet as they're faked / not yet found
#define FEATURE_SHOW_FREE_MEMORY

#define CONFIG_ADDITIONAL_VERSION
#define FEATURE_SCREENSHOT

#undef CONFIG_CRASH_LOG
#undef CONFIG_TSKMON
#undef CONFIG_PROP_REQUEST_CHANGE
#undef CONFIG_AUTOBACKUP_ROM
