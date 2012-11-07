/**
 *  Conditional defines for features that apply to certain camera families
 */

/** 
 * Debug features
 */

#define CONFIG_STRESS_TEST
#define CONFIG_BENCHMARKS
//~ #define CONFIG_DEBUGMSG 1
//~ #define CONFIG_ISO_TESTS
//~ #define CONFIG_DIGIC_POKE
//~ #define CONFIG_HEXDUMP

/**
 * Camera-specific stuff
 */

#if defined(CONFIG_5DC) || defined(CONFIG_40D)
#define CONFIG_VXWORKS
#endif

#if defined(CONFIG_50D)
// Canon graphics redraw continuously (even when completely disabled) and will erase ML graphics.
// Therefore, ML has to disable them completely.
// This can be enabled on any other camera for testing.
// Used in zebra.c and debug.c.
#define CONFIG_KILL_FLICKER
#endif


#if defined(CONFIG_60D) || defined(CONFIG_7D) || (defined(CONFIG_5D3) && !defined(CONFIG_5D3_MINIMAL))
#define CONFIG_ELECTRONIC_LEVEL
#endif

#if defined(CONFIG_5D2) || defined(CONFIG_5D3) // not sure about 7D
#define CONFIG_AUTO_BRIGHTNESS
#endif

#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D) // maybe 7D too
#define CONFIG_Q_MENU_PLAYBACK // camera has a Q menu in playback mode; this menu can be tweaked a bit (e.g. LV button = Protect or Rate)
#endif

#if defined(CONFIG_600D) || defined(CONFIG_60D) // EyeFi tricks confirmed working only on 600D-60D
#define CONFIG_EYEFI
#endif

#if defined(CONFIG_600D) || defined(CONFIG_60D) // enable some tricks for flip-out display
#define CONFIG_VARIANGLE_DISPLAY
#endif

#if defined(CONFIG_60D) || defined(CONFIG_5D2) || defined(CONFIG_5D3) || defined(CONFIG_7D)
#ifndef CONFIG_5D3_MINIMAL
#ifndef CONFIG_7D_MINIMAL
#define CONFIG_BATTERY_INFO // 5D2-like battery which reports exact percentage
#endif
#endif
#endif

#if defined(CONFIG_60D) || defined(CONFIG_5D2) || defined(CONFIG_5D3) || defined(CONFIG_7D)
#define CONFIG_SEPARATE_BULB_MODE // other cameras have BULB = M + shutter speed beyond 30s
#endif

#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_7D) || defined(CONFIG_1100D)
#define CONFIG_SILENT_PIC_HIRES
#endif

#if defined(CONFIG_7D) || defined(CONFIG_600D) || defined(CONFIG_1100D)
#define CONFIG_SILENT_PIC_JPG
#endif

#if !defined(CONFIG_50D) && !defined(CONFIG_VXWORKS) && !defined(CONFIG_1100D)
#define CONFIG_AUDIO_REMOTE_SHOT
#endif

#if defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_500D) || defined(CONFIG_1100D)
#define CONFIG_LV_3RD_PARTY_FLASH
#endif

#if defined(CONFIG_5D2) || defined(CONFIG_7D)
#define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING
#endif
