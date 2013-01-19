#include "all_features.h"

#define FEATURE_SWAP_MENU_ERASE
#define FEATURE_EYEFI_TRICKS
#define FEATURE_REC_NOTIFY_BEEP

// the 60D has very little RAM; disable some debug things

#undef FEATURE_SHOW_TASKS
#undef FEATURE_SHOW_CPU_USAGE
#undef FEATURE_SHOW_GUI_EVENTS
#undef FEATURE_SNAP_SIM

#undef CONFIG_STRESS_TEST
#undef CONFIG_BENCHMARKS
#undef FLEXINFO_DEVELOPER_MENU

#undef CONFIG_TSKMON
