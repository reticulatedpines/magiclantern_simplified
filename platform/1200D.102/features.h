#include "all_features.h"

// No MLU on the 1200D
#undef FEATURE_MLU
#undef FEATURE_MLU_HANDHELD

// Disable almost all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_BEEP
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT

#undef FEATURE_ARROW_SHORTCUTS

// Causes green tint on live view
#undef FEATURE_LV_BRIGHTNESS_CONTRAST

// Reported not to work - http://www.magiclantern.fm/forum/index.php?topic=12627.msg189609#msg189609
#undef FEATURE_MAGIC_ZOOM_FULL_SCREEN

// not much memory
#undef FEATURE_FLEXINFO
