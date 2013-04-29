#include "all_features.h"

#undef FEATURE_ARROW_SHORTCUTS // idk why :P
#undef FEATURE_IMAGE_POSITION
#undef FEATURE_FPS_OVERRIDE
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_FPS_RAMPING
#undef FEATURE_EXPO_ISO_DIGIC // not working, no idea why -> test on master?
#undef FEATURE_SHUTTER_LOCK // Canon has it
//#undef FEATURE_IMAGE_EFFECTS
#undef FEATURE_FPS_OVERRIDE

#define FEATURE_VIDEO_HACKS
#define FEATURE_ISR_HOOKS
#define FEATURE_AFMA_TUNING
#define FEATURE_UNREGISTER_PROP
