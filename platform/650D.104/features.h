#include "all_features.h"

#define FEATURE_LV_3RD_PARTY_FLASH // requires props
#define FEATURE_EYEFI_TRICKS

// Disable all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_VOICE_TAGS
#undef FEATURE_BEEP //works, but messes up module menu
#undef FEATURE_AUDIO_METERS // works, but cam not stable yet

// Audio features working
#define FEATURE_AUDIO_REMOTE_SHOT // generic enabled until AudioIC is programmed

//~ #undef FEATURE_ARROW_SHORTCUTS

// Not working :(
#undef FEATURE_IMAGE_EFFECTS
#undef FEATURE_DEFISHING_PREVIEW
#undef FEATURE_ANAMORPHIC_PREVIEW

#undef FEATURE_LV_BUTTON_PROTECT
#undef FEATURE_LV_BUTTON_RATE

#undef FEATURE_TRAP_FOCUS

// Glitchy
#undef FEATURE_STICKY_DOF
#undef FEATURE_STICKY_HALFSHUTTER
//~ #undef FLEXINFO_DEVELOPER_MENU - disable from flexinfo.h

#define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY
//~ #define FEATURE_SHOW_SIGNATURE
//~ #define FEATURE_SHOW_STATE_FPS
#define FEATURE_FOCUS_PEAK_DISP_FILTER

#define FEATURE_CROP_MODE_HACK
