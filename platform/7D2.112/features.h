#define FEATURE_VRAM_RGBA

//#define CONFIG_HELLO_WORLD

// Don't Click Me menu looks to be intended as a place
// for devs to put custom code in debug.c run_test(),
// and allowing triggering from a menu context.
//#define FEATURE_DONT_CLICK_ME

//#define FEATURE_SHOW_SHUTTER_COUNT

// working but incomplete, some allocators don't report
// anything yet as they're faked / not yet found
//#define FEATURE_SHOW_FREE_MEMORY

#define FEATURE_SCREENSHOT

//#define CONFIG_TSKMON
#define FEATURE_SHOW_TASKS
//#define FEATURE_SHOW_CPU_USAGE
//#define FEATURE_SHOW_GUI_EVENTS

// enable global draw
//#define FEATURE_GLOBAL_DRAW
//#define FEATURE_CROPMARKS

//#define CONFIG_PROP_REQUEST_CHANGE
//#define CONFIG_STATE_OBJECT_HOOKS
//#define CONFIG_LIVEVIEW
//#define FEATURE_POWERSAVE_LIVEVIEW

// explicitly disable stuff that don't work or may break things
#undef CONFIG_AUTOBACKUP_ROM
#undef CONFIG_ADDITIONAL_VERSION
