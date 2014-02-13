#include "all_features.h"

#undef FEATURE_ARROW_SHORTCUTS // idk why :P
#undef FEATURE_IMAGE_POSITION
#undef FEATURE_FPS_OVERRIDE
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_FPS_RAMPING
#undef FEATURE_EXPO_ISO_DIGIC // not working, no idea why -> test on master?
#undef FEATURE_SHUTTER_LOCK // Canon has it
#undef FEATURE_IMAGE_EFFECTS // they work in preview only and cause trouble
#undef FEATURE_FPS_OVERRIDE

#undef FEATURE_WIND_FILTER //Blocks Meters, esp external audio
#undef FEATURE_HEADPHONE_MONITORING // need code on master (?)

#define FEATURE_VIDEO_HACKS

#undef FEATURE_AF_PATTERNS

