#include "all_features.h"
/* Can't Be Implemented */
#undef FEATURE_FORCE_LIVEVIEW // Already Live View
#undef FEATURE_MLU // No Mirror
#undef FEATURE_MLU_HANDHELD
#undef FEATURE_STICKY_DOF // No DOF button
#undef FEATURE_IMAGE_EFFECTS // DigicV new effects check "Art Filter"
#undef FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY // Well.. it will work in 1 mode
#undef FEATURE_AF_PATTERNS // No regular AF
#undef FEATURE_VOICE_TAGS // Just to be sure
//#undef FEATURE_SHUTTER_FINE_TUNING //It works! Timer values are not applied until you press record (this is normal for EOSM)


/* Working (trimmed heavy video/crop features for DIGIC 5 boot memory headroom) */
#undef FEATURE_CROP_MODE_HACK
#undef FEATURE_MOVIE_RECORDING_50D
#undef FEATURE_FPS_RAMPING
#define FEATURE_AUDIO_REMOTE_SHOT

/* Some Hope Yet */
#undef FEATURE_TRAP_FOCUS
#undef FEATURE_FOLLOW_FOCUS
// #undef FEATURE_RACK_FOCUS
// #undef FEATURE_FOCUS_STACKING
#undef FEATURE_GHOST_IMAGE // No way to pick image but works.
// #undef FEATURE_SET_MAINDIAL // Set taken over by Q
// #undef FEATURE_PLAY_EXPOSURE_FUSION // https://www.magiclantern.fm/forum/index.php?topic=9741.msg211033#msg211033
#undef FEATURE_PLAY_COMPARE_IMAGES
#undef FEATURE_PLAY_TIMELAPSE
#undef FEATURE_PLAY_EXPOSURE_ADJUST
#undef FEATURE_QUICK_ZOOM
#undef FEATURE_QUICK_ERASE
#undef FEATURE_LV_FOCUS_BOX_FAST
#undef FEATURE_LV_FOCUS_BOX_SNAP
#undef FEATURE_ARROW_SHORTCUTS
#undef FEATURE_MAGIC_ZOOM_FULL_SCREEN // https://bitbucket.org/hudson/magic-lantern/issue/2272/full-screen-magic-zoom-is-garbled-on-700d
                                      // https://bitbucket.org/hudson/magic-lantern/issues/2842

//#define FEATURE_EYEFI_TRICKS
//#define FEATURE_JUNKIE_MENU
