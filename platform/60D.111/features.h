#include "all_features.h"

#define FEATURE_SWAP_MENU_ERASE
#define FEATURE_EYEFI_TRICKS
#define FEATURE_REC_NOTIFY_BEEP

// the 60D has very little RAM; disable some debug things

#undef FEATURE_EXPO_PRESET
#undef FEATURE_UPSIDE_DOWN // do we really need it with the swivel screen?
#undef FEATURE_LV_ZOOM_AUTO_EXPOSURE // not working well anyway

//#undef FEATURE_SHOW_TASKS
//#undef FEATURE_SHOW_CPU_USAGE
#undef FEATURE_SHOW_GUI_EVENTS
#undef FEATURE_SNAP_SIM
#undef FEATURE_SHOW_IMAGE_BUFFERS_INFO

#undef FLEXINFO_DEVELOPER_MENU

//#undef CONFIG_TSKMON

#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_VOICE_TAGS
