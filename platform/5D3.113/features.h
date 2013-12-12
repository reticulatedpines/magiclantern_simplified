#include "all_features.h"

#undef FEATURE_IMAGE_REVIEW_PLAY // Not needed, one can press Zoom right away
#undef FEATURE_QUICK_ZOOM // Canon has it
#undef FEATURE_QUICK_ERASE // Canon has it
#undef FEATURE_IMAGE_EFFECTS // None working
#undef FEATURE_MOVIE_REC_KEY // Canon has it
#undef FEATURE_NITRATE_WAV_RECORD // Not implemented
#undef FEATURE_AF_PATTERNS // Canon has it
#undef FEATURE_MLU_HANDHELD // Not needed, Canon's silent mode is much better
#undef FEATURE_SHUTTER_LOCK // Canon has a dedicated button for it
#undef FEATURE_FLASH_TWEAKS // No built-in flash
#undef FEATURE_FLASH_NOFLASH
#undef FEATURE_MOVIE_RESTART // Not working

#define FEATURE_KEN_ROCKWELL_ZOOM_5D3
#define FEATURE_ZOOM_TRICK_5D3 // Not reliable
//~ #define FEATURE_REMEMBER_LAST_ZOOM_POS_5D3 // Too many conflicts with other features
#undef FEATURE_IMAGE_POSITION

//~ #define FEATURE_VIDEO_HACKS


#undef FEATURE_VOICE_TAGS // No sound recorded

#define FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW
