#define FEATURE_VRAM_RGBA

// Don't Click Me can be used by devs to put custom code in debug.c,
// run_test(), for triggering from Debug menu.
#define FEATURE_DONT_CLICK_ME

// "working" - but is the number correct?
#define FEATURE_SHOW_SHUTTER_COUNT

// working but slightly hackish, don't yet have a good
// way to determine free stack size
#define FEATURE_SHOW_FREE_MEMORY

// partially working; wrong colorspace and not
// dumping all the images that old cams support
#define FEATURE_SCREENSHOT

// duplicate all printf (console) output to uart
#define CONFIG_COPY_CONSOLE_TO_UART

// cam doesn't expose call("TurnOnDisplay"),
// (or TurnOffDisplay), so we must find stubs
// and call directly
#define CONFIG_NO_DISPLAY_CALLS

// Disable 30min LV timer.  This requires prop_request_change.
// Also requires LV and State objects; but you
// don't need any actual objects found in state-object.h
#define CONFIG_PROP_REQUEST_CHANGE
#define CONFIG_STATE_OBJECT_HOOKS
#define CONFIG_LIVEVIEW
#define FEATURE_POWERSAVE_LIVEVIEW

// Mostly working - task display is too crowded.
// Maybe CPU usage should update faster?
#define CONFIG_TSKMON
#define FEATURE_SHOW_TASKS
#define FEATURE_SHOW_CPU_USAGE
#define FEATURE_SHOW_GUI_EVENTS

#define FEATURE_INTERVALOMETER

// Optionally sticky halfshutter.  Works but likely not required.
//#define FEATURE_STICKY_HALFSHUTTER

// These two needed to enable mlv_lite
#define FEATURE_PICSTYLE
#define CONFIG_RAW_LIVEVIEW

// enable global draw
#define FEATURE_GLOBAL_DRAW
#define FEATURE_CROPMARKS

// Enable remapping ROM pages to RAM
#define CONFIG_SGI_HANDLERS
#define CONFIG_MMU_REMAP

// ROM has a function to automatically determine a fast(er) speed for SD card.
// TODO: possibly we could integrate sd_uhs.mo behind this same feature flag and menu.
#define FEATURE_SD_AUTOTUNE

// We are able to override the MOV / MP4 29:59 limit
#define FEATURE_OVERRIDE_MOVIE_30_MIN_LIMIT

// Fast disk logging
#define FEATURE_DISK_LOG

// This works but is limited; no stack unwinding, since existing code
// assumes ARM, not Thumb.  Alex recommends a good looking fix:
// http://www.mcternan.me.uk/ArmStackUnwinding/
#define CONFIG_CRASH_LOG

#undef CONFIG_ADDITIONAL_VERSION
#undef CONFIG_AUTOBACKUP_ROM
