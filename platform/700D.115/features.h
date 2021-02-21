#include "all_features.h"

//~ #define FEATURE_LV_3RD_PARTY_FLASH // requires props

// Disable all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_VOICE_TAGS
#undef FEATURE_BEEP // works, but cam unstable as of now

// Audio features that work
#define FEATURE_AUDIO_REMOTE_SHOT

// Not working :(
#undef FEATURE_IMAGE_EFFECTS
#undef FEATURE_LV_BUTTON_PROTECT
#undef FEATURE_LV_BUTTON_RATE
#undef FEATURE_TRAP_FOCUS
#undef FEATURE_MAGIC_ZOOM_FULL_SCREEN // https://bitbucket.org/hudson/magic-lantern/issue/2272/full-screen-magic-zoom-is-garbled-on-700d

// Works
#define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY
#define FEATURE_FOCUS_PEAK_DISP_FILTER
#define FEATURE_CROP_MODE_HACK
#define FEATURE_EYEFI_TRICKS
