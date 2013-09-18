#include "all_features.h"

// 50D movie hacks
#define FEATURE_MOVIE_RECORDING_50D
#define CONFIG_MOVIE_RECORDING_50D_SHUTTER_HACK

/* slows down LiveView; has other side effects on 5D2/50D */
#undef FEATURE_FOCUS_PEAK_DISP_FILTER //oh yea!
#define FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW

// no audio at all
#undef FEATURE_AUDIO_METERS
#undef FEATURE_BEEP
#undef FEATURE_WAV_RECORDING
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT
#undef FEATURE_NITRATE_WAV_RECORD
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_SHUTTER_FINE_TUNING //Too Flashy
#undef FEATURE_FLEXINFO

#undef FEATURE_UPSIDE_DOWN // not working, http://www.magiclantern.fm/forum/index.php?topic=4430

