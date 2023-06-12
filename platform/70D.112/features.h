#include "all_features.h"

#undef FEATURE_QUICK_ERASE    // Canon has it
#undef FEATURE_IMAGE_EFFECTS  // None working
#undef FEATURE_AF_PATTERNS    // Canon has it
#undef FEATURE_MLU_HANDHELD   // Not needed, Canon's silent mode is much better
#undef FEATURE_SHUTTER_LOCK   // Canon has a dedicated button for it
#undef FEATURE_IMAGE_POSITION // assume it is not needed with a variangle display
#undef FEATURE_ARROW_SHORTCUTS // No suitable button found

// Really, this simply doesn't work
// Tried it for a felt hundred hours
// TIMER_B has untraceable problems
// Using TIMER_A_ONLY causes banding / patterns 
#undef FEATURE_FPS_OVERRIDE

/* see comments in lens.c */
#undef FEATURE_FOLLOW_FOCUS
#undef FEATURE_RACK_FOCUS
#undef FEATURE_FOCUS_STACKING

/* disable RAW zebras as they cause problems in QR and LV too */
#undef FEATURE_RAW_ZEBRAS
#undef FEATURE_NITRATE

// we got enough infos on main display and top lcd.
// Mainly this got disabled due to the bottom line
// (time, battery) flickering 
// ToDo: alternative coordinates not using outer ones
// custom builds may include it til we achieve it
#undef FEATURE_FLEXINFO

/* Audio Features */
#define FEATURE_AUDIO_METERS
#define FEATURE_AUDIO_REMOTE_SHOT
