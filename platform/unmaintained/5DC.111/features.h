// Small feature set, we'll just define each one
// look in all_features.h

/** Expo menu **/

    //#define FEATURE_WHITE_BALANCE not working
    //#define FEATURE_ML_AUTO_ISO not working
	//#define FEATURE_EXPO_LOCK

/** Overlay menu **/

    #define FEATURE_GLOBAL_DRAW
	#define FEATURE_ZEBRA
//    #define FEATURE_ZEBRA_FAST //not tested
	#define FEATURE_FOCUS_PEAK
	#define FEATURE_SPOTMETER
	#define FEATURE_HISTOGRAM
	#define FEATURE_WAVEFORM
	#define FEATURE_VECTORSCOPE
	#define FEATURE_OVERLAYS_IN_PLAYBACK_MODE
	//#define FEATURE_FALSE_COLOR

/** Shoot menu **/

	#define FEATURE_HDR_BRACKETING
	#define FEATURE_INTERVALOMETER
//#define FEATURE_BULB_RAMPING
//#define FEATURE_BULB_TIMER //needs fixes
	#define FEATURE_MLU
//#define FEATURE_MLU_HANDHELD // not working
	#define FEATURE_MLU_DIRECT_PRINT_SHORTCUT // for 5Dc

	#define FEATURE_FLASH_TWEAKS // flash no flash not working
    #define FEATURE_SNAP_SIM

/** Focus menu **/

/* No focus menu with those enabled
    #define FEATURE_TRAP_FOCUS
    #define FEATURE_AF_PATTERNS*/

/** Display menu **/

//    #define FEATURE_LV_SATURATION
//	#define FEATURE_UNIWB_CORRECTION

/** Prefs menu **/

    #define FEATURE_SET_MAINDIAL
//#define FEATURE_PLAY_EXPOSURE_FUSION //? not shown
//#define FEATURE_PLAY_COMPARE_IMAGES //? not shown
	#define FEATURE_PLAY_TIMELAPSE
	#define FEATURE_PLAY_EXPOSURE_ADJUST
	#define FEATURE_IMAGE_REVIEW_PLAY
	#define FEATURE_QUICK_ZOOM
	#define FEATURE_QUICK_ERASE
//#define FEATURE_STICKY_DOF //not working
//#define FEATURE_STICKY_HALFSHUTTER //not working
    #define FEATURE_WARNINGS_FOR_BAD_SETTINGS

/** Debug menu **/

	#define FEATURE_SCREENSHOT

	#define FEATURE_DONT_CLICK_ME

	#define FEATURE_SHOW_TASKS
//#define FEATURE_SHOW_CPU_USAGE don't works, need fix
//#define FEATURE_SHOW_GUI_EVENTS not working ifdisplay off

    #define FEATURE_SHOW_IMAGE_BUFFERS_INFO
    #define FEATURE_SHOW_FREE_MEMORY
//#define FEATURE_SHOW_SHUTTER_COUNT //not working
    #define FEATURE_SHOW_CMOS_TEMPERATURE

