#define FEATURE_VRAM_RGBA

#define FEATURE_DONT_CLICK_ME

#define FEATURE_SCREENSHOT
//#define FEATURE_SD_AUTOTUNE // exists, but doesn't seem to improve over stock speeds

// cam doesn't expose call("TurnOnDisplay"),
// (or TurnOffDisplay), so we must find stubs
// and call directly
#define CONFIG_NO_DISPLAY_CALLS

// Half works?  shutter_count_plus_lv_actuations seems to go up correctly,
// but shutter_count doesn't seem to change
#define FEATURE_SHOW_SHUTTER_COUNT

//#define FEATURE_DONT_CLICK_ME

#define CONFIG_SGI_HANDLERS
#define CONFIG_MMU_REMAP

#define CONFIG_PROP_REQUEST_CHANGE

#define CONFIG_CRASH_LOG

//#define CONFIG_IMAGE_CAPTURE_NOT_WORKING
#define FEATURE_INTERVALOMETER

#undef CONFIG_ADDITIONAL_VERSION
#undef CONFIG_AUTOBACKUP_ROM
