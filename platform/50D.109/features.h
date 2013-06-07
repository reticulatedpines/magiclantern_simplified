#include "all_features.h"

// 50D movie hacks
#define FEATURE_MOVIE_RECORDING_50D
#define FEATURE_MOVIE_RECORDING_50D_SHUTTER_HACK

// silent pic 
#define FEATURE_SILENT_PIC_RAW_BURST // compiles and works for single pics burst seems to have issues
//#define FEATURE_SILENT_PIC_RAW  ---> Set Burst or single not both! single gives undefined reference to `silent_pic_raw_vsync'
//~ #define FEATURE_LVAE_EXPO_LOCK // unreliable, and we have full manual controls now

#define FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW

// no audio at all
#undef FEATURE_AUDIO_METERS
#undef FEATURE_BEEP
#undef FEATURE_WAV_RECORDING
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT
#undef FEATURE_NITRATE_WAV_RECORD
#undef FEATURE_FPS_WAV_RECORD

#undef FEATURE_UPSIDE_DOWN // not working, http://www.magiclantern.fm/forum/index.php?topic=4430

#define FEATURE_AFMA_TUNING // compiles fine but not tested
