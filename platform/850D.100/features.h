//#define CONFIG_HELLO_WORLD

#define FEATURE_VRAM_RGBA

#define FEATURE_SHOW_SHUTTER_COUNT

#define FEATURE_SCREENSHOT

#define FEATURE_DONT_CLICK_ME

//#define CONFIG_MARK_UNUSED_MEMORY_AT_STARTUP

// Working, not thoroughly tested.
// Cannot remap rom1 due to reads hanging cam
// if accessed as early as we do remap.
#define CONFIG_MMU_REMAP
#define CONFIG_SGI_HANDLERS

// Needed for consistent ML display due to racing
// for vram layers
#define CONFIG_COMPOSITOR_XCM
#define CONFIG_COMPOSITOR_DEDICATED_LAYER
#define CONFIG_COMPOSITOR_XCM_V2

// mostly working - task display is too crowded.
// Maybe CPU usage should update faster?
#define CONFIG_TSKMON
#define FEATURE_SHOW_TASKS
#define FEATURE_SHOW_CPU_USAGE
#define FEATURE_SHOW_GUI_EVENTS

// Not all of this feature works yet, stack unwinding is disabled
// in code.  Current approach assumes ARM and makes assumptions
// that crash.
//
// Alex recommends a good looking fix:
// http://www.mcternan.me.uk/ArmStackUnwinding/
#define CONFIG_CRASH_LOG

// We can't yet rely on image capture.  Cam crashes due to null pointer,
// I think?  If it fails to AF lock, for example.
#define CONFIG_IMAGE_CAPTURE_NOT_WORKING

#undef CONFIG_PROP_REQUEST_CHANGE
#undef CONFIG_ADDITIONAL_VERSION
#undef CONFIG_AUTOBACKUP_ROM
