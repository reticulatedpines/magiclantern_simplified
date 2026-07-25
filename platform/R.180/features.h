#define FEATURE_VRAM_RGBA

#define CONFIG_COMPOSITOR_XCM
#define CONFIG_COMPOSITOR_DEDICATED_LAYER
#define CONFIG_COMPOSITOR_XCM_V1

/* Remap ROM pages to RAM -- enables ML ROM patching on this body (DIGIC 8 MMU) */
#define CONFIG_SGI_HANDLERS
#define CONFIG_MMU_REMAP

// Don't Click Me menu looks to be intended as a place
// for devs to put custom code in debug.c run_test(),
// and allowing triggering from a menu context.
#define FEATURE_DONT_CLICK_ME

#define FEATURE_SHOW_SHUTTER_COUNT

// working but incomplete, some allocators don't report
// anything yet as they're faked / not yet found
#define FEATURE_SHOW_FREE_MEMORY

#define CONFIG_TSKMON
#define FEATURE_SHOW_TASKS
#define FEATURE_SHOW_CPU_USAGE
#define FEATURE_SHOW_GUI_EVENTS

#define CONFIG_ADDITIONAL_VERSION
#define FEATURE_SCREENSHOT

// enable global draw
#define FEATURE_GLOBAL_DRAW
#define FEATURE_CROPMARKS

// enable for testing gui structure changes
#define CONFIG_RESTORE_AFTER_FORMAT

// Image capture now works: lens_take_picture uses the CameraConductor
// remote-release path, which finalizes the job cleanly and tolerates AF-lock
// failure (verified on hardware: easy subject captures, hard subject fails
// gracefully, no crash). So the old "not working" gate is lifted.
// #define CONFIG_IMAGE_CAPTURE_NOT_WORKING

#define FEATURE_PICSTYLE
#define CONFIG_PROP_REQUEST_CHANGE

// capture-driven features (ride on take_a_pic, now working)
#define FEATURE_INTERVALOMETER
#define FEATURE_HDR_BRACKETING

#undef CONFIG_CRASH_LOG
#undef CONFIG_AUTOBACKUP_ROM
