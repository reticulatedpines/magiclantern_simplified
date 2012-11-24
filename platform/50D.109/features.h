#include "all_features.h"

// 50D movie hacks
#define FEATURE_MOVIE_RECORDING_50D
#define FEATURE_MOVIE_RECORDING_50D_SHUTTER_HACK
//~ #define FEATURE_LVAE_EXPO_LOCK // unreliable, and we have full manual controls now

// no audio at all
#undef FEATURE_AUDIO_METERS
#undef FEATURE_BEEP
#undef FEATURE_WAV_RECORDING
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT
#undef FEATURE_NITRATE_WAV_RECORD
#undef FEATURE_FPS_WAV_RECORD
