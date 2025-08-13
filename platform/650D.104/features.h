#include "all_features.h"

//~ #define FEATURE_LV_3RD_PARTY_FLASH // doesn't work, https://bitbucket.org/hudson/magic-lantern/issue/2081/650d-and-external-flash-in-liveview-mode

#define FEATURE_EYEFI_TRICKS

// Disable all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_VOICE_TAGS
#undef FEATURE_BEEP //works, but messes up module menu

// Audio features working
#define FEATURE_AUDIO_REMOTE_SHOT // generic enabled until AudioIC is programmed

// Not working :(
#undef FEATURE_IMAGE_EFFECTS
//~ #undef FEATURE_DEFISHING_PREVIEW
//~ #undef FEATURE_ANAMORPHIC_PREVIEW

#undef FEATURE_LV_BUTTON_PROTECT
#undef FEATURE_LV_BUTTON_RATE

#undef FEATURE_TRAP_FOCUS

#undef FEATURE_MAGIC_ZOOM_FULL_SCREEN // https://bitbucket.org/hudson/magic-lantern/issue/2272/full-screen-magic-zoom-is-garbled-on-700d

#define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY
#define FEATURE_FOCUS_PEAK_DISP_FILTER

#define FEATURE_CROP_MODE_HACK
