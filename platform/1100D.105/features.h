#include "all_features.h"

#define FEATURE_LV_3RD_PARTY_FLASH
#define FEATURE_EYEFI_TRICKS


// No MLU on the 1100D
#undef FEATURE_MLU
#undef FEATURE_MLU_HANDHELD

// Disable almost all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_BEEP
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT

// No DISP button
#undef FEATURE_ARROW_SHORTCUTS

#define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY

// disabled, because autoexec.bin gets to big and 600D does not boot
#undef FEATURE_SHOW_TASKS
#undef FEATURE_SHOW_CPU_USAGE
#undef FEATURE_SHOW_GUI_EVENTS
#undef FEATURE_SHOW_EDMAC_INFO
#undef FEATURE_FLEXINFO
