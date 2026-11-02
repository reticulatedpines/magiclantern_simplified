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

// ML lives in the same 512K AllocateMemory pool as on 1100D;
// same size-hungry features disabled, or autoexec.bin gets too big and does not boot
#undef FEATURE_JUNKIE_MENU
#undef FEATURE_STICKY_HALFSHUTTER
#undef FEATURE_SHOW_TASKS
#undef FEATURE_SHOW_CPU_USAGE
#undef FEATURE_SHOW_GUI_EVENTS
#undef FEATURE_SHOW_IMAGE_BUFFERS_INFO
#undef FEATURE_SHOW_SHUTTER_COUNT
#undef FEATURE_SHOW_CMOS_TEMPERATURE
#undef FEATURE_SHOW_EDMAC_INFO
#undef FEATURE_SHOW_FREE_MEMORY
#undef CONFIG_TSKMON
