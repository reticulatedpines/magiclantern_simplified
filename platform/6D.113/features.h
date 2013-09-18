#include "all_features.h"


/*Not Working Or Unnecessary*/
#undef FEATURE_QUICK_ZOOM    //Canon Has it
#undef FEATURE_AF_PATTERNS   // Not working will disable auto af points
#undef FEATURE_IMAGE_EFFECTS // Missing Zero Sharpness :( Regs have changed
#undef FEATURE_IMAGE_REVIEW_PLAY // not needed, one can press Zoom right away
#undef CONFIG_STRESS_TEST // We don't need this so much.
#undef CONFIG_MEMPATCH_CHECK // Reports 0 for total.


/*Work Relatively well*/
#define CONFIG_FPS_AGGRESSIVE_UPDATE
#define FEATURE_FOCUS_PEAK_DISP_FILTER
#define FEATURE_ZOOM_TRICK_5D3 // Doubleclick to zoom/shortcut
#define FEATURE_KEN_ROCKWELL_ZOOM_5D3 // Play From Image Review Mode - Did not bring up play
#define CONFIG_AFMA_EXTENDED
/* Development */

/* Debugging Stuff */
#define CONFIG_HEXDUMP
#define CONFIG_DIGIC_POKE
//~ #define FEATURE_SHOW_SIGNATURE
//~ #define CONFIG_DUMPROM Lawyers will kill your camera.. and you.


/* Audio Features */
#define FEATURE_HEADPHONE_MONITORING
#define FEATURE_NITRATE_WAV_RECORD
#define FEATURE_WAV_RECORDING
#define FEATURE_AUDIO_METERS

//~ #define FEATURE_HEADPHONE_OUTPUT_VOLUME
