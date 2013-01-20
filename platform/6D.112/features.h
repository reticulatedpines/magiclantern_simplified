#include "all_features.h"

//Not Working Or Unnecessary
#undef FEATURE_QUICK_ZOOM    //Canon Has it
#undef FEATURE_AF_PATTERNS   // Not working will disable auto af points
#undef FEATURE_IMAGE_EFFECTS // none working
#undef FEATURE_FLASH_TWEAKS // no built-in flash
#undef FEATURE_IMAGE_REVIEW_PLAY // not needed, one can press Zoom right away

//#define FEATURE_ZOOM_TRICK_5D3
///#define HIJACK_CACHE_HACK Not working Yet
#define FEATURE_SILENT_PIC_HIRES
//#define FEATURE_SILENT_PIC_JPG Does not work that way anymore.
#define FEATURE_HDR_EXTENDED // needs FRAME_SHUTTER_TIMER... will look
#define FEATURE_VIDEO_HACKS

#define CONFIG_HEXDUMP
#define CONFIG_BEEP
//#define CONFIG_AUDIO_CONTROLS
#define FEATURE_HEADPHONE_MONITORING
#define FEATURE_AUDIO_METERS
#define FEATURE_BEEP
#define FEATURE_NITRATE_WAV_RECORD
#define FEATURE_WAV_RECORDING
#define FEATURE_HEADPHONE_OUTPUT_VOLUME
#define FEATURE_FOCUS_PEAK_DISP_FILTER


// the 6D has very little RAM; disable some debug things
#undef FEATURE_SHOW_TASKS
#undef FEATURE_SHOW_CPU_USAGE
#undef FEATURE_SHOW_GUI_EVENTS
#undef FEATURE_SNAP_SIM


#undef CONFIG_TSKMON

