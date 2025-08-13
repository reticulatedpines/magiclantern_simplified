#include "all_features.h"

#undef FEATURE_QUICK_ERASE    // Canon has it
#undef FEATURE_IMAGE_EFFECTS  // None working
#undef FEATURE_MLU_HANDHELD   // Not needed, Canon's silent mode is much better

/* Audio Features */
#define FEATURE_AUDIO_METERS
#define FEATURE_AUDIO_REMOTE_SHOT
#define FEATURE_CROP_MODE_HACK

/* Not working */
#undef FEATURE_TRAP_FOCUS
#undef FEATURE_MAGIC_ZOOM_FULL_SCREEN // https://bitbucket.org/hudson/magic-lantern/issues/2842
