/**
 *  Conditional defines for features that apply to certain camera families
 * 
 *  After editing this file, run: make clean && make
 * 
 */




/** 
 * Enable these for early ports
 */

/** If CONFIG_EARLY_PORT is defined, only a few things will be enabled (e.g. changing version string) */
//~ #define CONFIG_EARLY_PORT

/** Load fonts and print Hello World (disable CONFIG_EARLY_PORT); will not start any other ML tasks, handlers etc. */
//~ #define CONFIG_HELLO_WORLD

/** Safe mode, don't alter properties (they are persistent). Highly recommended for new ports. */
#if defined(CONFIG_5D3_MINIMAL) || defined(CONFIG_7D) || defined(CONFIG_40D)
#define CONFIG_DISABLE_PROP_REQUEST_CHANGE
#endif



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

#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D) || defined(CONFIG_5D3)
#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY // by changing YUV422_LV_BUFFER_DISPLAY_ADDR
#endif

#if defined(CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY) || defined(CONFIG_5D2)
#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER // some cameras may have specific hacks and still do this, but harder
#define CONFIG_DISPLAY_FILTERS
#endif

#if !defined(CONFIG_500D) && !defined(CONFIG_7D) && !defined(CONFIG_VXWORKS)
#define CONFIG_FRAME_ISO_OVERRIDE // e.g. for HDR video or gradual exposure
#endif

#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D) || defined(CONFIG_5D3)
#define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY // you can't override digital ISO component via FRAME_ISO
#endif
