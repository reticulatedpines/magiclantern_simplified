#include "all_features.h"

//Not Working Or Unnecessary
#undef FEATURE_QUICK_ZOOM    //Canon Has it
#undef FEATURE_AF_PATTERNS   // Not working will disable auto af points
#undef FEATURE_IMAGE_EFFECTS // none working
//~ #undef FEATURE_FLASH_TWEAKS // They work. More to come.
#undef FEATURE_IMAGE_REVIEW_PLAY // not needed, one can press Zoom right away
#undef FEATURE_MLU_HANDHELD // not needed, Canon's silent mode is much better

//Work Relatively well
//#define CONFIG_KILL_FLICKER // Kill canon popups
#define CONFIG_FPS_AGGRESSIVE_UPDATE
#define FEATURE_MOVIE_AUTOSTOP_RECORDING
#define FEATURE_REC_ON_RESUME
#define FEATURE_FOCUS_PEAK_DISP_FILTER
#define FEATURE_ZOOM_TRICK_5D3 // Doubleclick to zoom/shortcut
#define FEATURE_KEN_ROCKWELL_ZOOM_5D3 // Play From Image Review Mode - Did not bring up play
#define FEATURE_AFMA_TUNING
#define CONFIG_AFMA_EXTENDED
#define FEATURE_PREFIX //Named HDR Bracketing B01, B02, etc
#define FEATURE_NOHELP //No one can help you now
#undef CONFIG_MEMPATCH_CHECK // Reports 0 for total.
#define FEATURE_AUDIO_METERS


// Development
//#define FEATURE_SILENT_PIC_JPG // Does not work that way anymore.
//#define FEATURE_HDR_EXTENDED // Broken By new menu change, please wait :(
//~ #define FEATURE_SHOW_SIGNATURE
//~ #define CONFIG_DUMPROM Lawyers will kill your camera.. and you.
//~ #define FEATURE_SILENT_PIC_HIRES //Needs check of alignment.
#undef FEATURE_VOICE_TAGS // Asif stop is broken^HHHH ... Fixed

#define FEATURE_PROP_DISPLAY
#define CONFIG_HEXDUMP
#define CONFIG_DIGIC_POKE

#undef CONFIG_TSKMON

