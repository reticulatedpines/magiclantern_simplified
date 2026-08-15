#define FEATURE_VRAM_RGBA

#define FEATURE_DONT_CLICK_ME

#define FEATURE_SCREENSHOT
//#define FEATURE_SD_AUTOTUNE // exists, but doesn't seem to improve over stock speeds

// These two needed to enable mlv_lite
#define FEATURE_PICSTYLE
#define CONFIG_RAW_LIVEVIEW

// cam doesn't expose call("TurnOnDisplay"),
// (or TurnOffDisplay), so we must find stubs
// and call directly
#define CONFIG_NO_DISPLAY_CALLS

// Half works?  shutter_count_plus_lv_actuations seems to go up correctly,
// but shutter_count doesn't seem to change
#define FEATURE_SHOW_SHUTTER_COUNT

#define CONFIG_SGI_HANDLERS
#define CONFIG_MMU_REMAP

#define FEATURE_GLOBAL_DRAW
//#define FEATURE_CROPMARKS // wants IMGPLAY_ZOOM_LEVEL_ADDR

#define CONFIG_PROP_REQUEST_CHANGE
#define CONFIG_STATE_OBJECT_HOOKS
#define CONFIG_LIVEVIEW
#define FEATURE_POWERSAVE_LIVEVIEW

#define CONFIG_CRASH_LOG

//#define CONFIG_IMAGE_CAPTURE_NOT_WORKING
#define FEATURE_INTERVALOMETER

#define CONFIG_AUTOBACKUP_ROM

// We are able to override the MOV / MP4 29:59 limit
// (needs MVR_TIME_LIMIT_NORMAL_FPS / MVR_TIME_LIMIT_HIGH_FPS in consts.h)
#define FEATURE_OVERRIDE_MOVIE_30_MIN_LIMIT

#undef CONFIG_ADDITIONAL_VERSION
