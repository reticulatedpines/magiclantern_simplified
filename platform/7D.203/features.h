#include "all_features.h"

#undef FEATURE_ARROW_SHORTCUTS // idk why :P
#undef FEATURE_IMAGE_POSITION
#undef FEATURE_FPS_OVERRIDE
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_FPS_RAMPING
#undef FEATURE_EXPO_ISO_DIGIC // not working, no idea why
#undef FEATURE_SHUTTER_LOCK // Canon has it

#define FEATURE_VIDEO_HACKS
#define FEATURE_ISR_HOOKS
