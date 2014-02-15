#include "all_features.h"

/* Works just fine */
#define FEATURE_SWAP_INFO_PLAY

/*Not Working Or Unnecessary*/
#undef FEATURE_QUICK_ZOOM    //Canon Has it
#undef FEATURE_IMAGE_EFFECTS // Missing Zero Sharpness :( Regs have changed
#undef FEATURE_IMAGE_REVIEW_PLAY // not needed, one can press Zoom right away

/*Work Relatively well*/
#define FEATURE_FOCUS_PEAK_DISP_FILTER
#define FEATURE_ZOOM_TRICK_5D3 // Doubleclick to zoom/shortcut
#define FEATURE_KEN_ROCKWELL_ZOOM_5D3 // Play From Image Review Mode - Did not bring up play

/* Audio Features */
#define FEATURE_HEADPHONE_MONITORING
#define FEATURE_NITRATE_WAV_RECORD
#define FEATURE_WAV_RECORDING
#define FEATURE_AUDIO_METERS
