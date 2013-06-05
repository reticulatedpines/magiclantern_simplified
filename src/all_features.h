/**
 * Main ML feature set
 *
 * You can:
 *
 * 1) include this file from platform/CAMERA/features.h (recommended for stable ports) and override a few things,
 *       or
 * 2) you can start from scratch with a minimal feature set (just define what you need).
 *
 * Basically, if feature X works on most cameras, it's probably a good idea to put it here.
 *
 * If feature X depends on some backend thingie, you can add the condition here
 *
 * e.g.
 *
 *      #ifdef CONFIG_VARIANGLE_DISPLAY
 *          #define FEATURE_IMAGE_ORIENTATION
 *          #define FEATURE_AUTO_MIRRORING_HACK
 *      #endif
 *
 * can be read as "all cameras with a flip-out display will have these two features".
 *
 */

/** Audio menu **/

#ifdef CONFIG_AUDIO_CONTROLS
    #define FEATURE_ANALOG_GAIN
    #define FEATURE_DIGITAL_GAIN
    #define FEATURE_AGC_TOGGLE
    #define FEATURE_WIND_FILTER
    #define FEATURE_INPUT_SOURCE
    #define FEATURE_MIC_POWER
    #define FEATURE_HEADPHONE_MONITORING
    #define FEATURE_HEADPHONE_OUTPUT_VOLUME
#endif

    #define FEATURE_AUDIO_METERS
    #define FEATURE_BEEP
    #define FEATURE_WAV_RECORDING
    #define FEATURE_VOICE_TAGS

/** Expo menu **/

    #define FEATURE_EXPO_ISO_DIGIC

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_WHITE_BALANCE
    #define FEATURE_EXPO_ISO
    #define FEATURE_EXPO_ISO_HTP
    #define FEATURE_EXPO_SHUTTER
    #define FEATURE_EXPO_APERTURE
    //~ #define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY // only for rebels, not for high-end bodies
    //~ #define FEATURE_INTERMEDIATE_ISO_INTERCEPT_SCROLLWHEEL // 550D trick

    #define FEATURE_PICSTYLE
    #define FEATURE_REC_PICSTYLE

    #define FEATURE_EXPO_LOCK
    #define FEATURE_EXPO_PRESET

#ifdef CONFIG_EXPSIM
    #define FEATURE_EXPSIM
#endif

    #define FEATURE_EXPO_OVERRIDE

#ifdef CONFIG_RAW_PHOTO
    #define FEATURE_AUTO_ETTR
#endif

#endif

/** Overlay menu **/

    #define FEATURE_GLOBAL_DRAW
    #define FEATURE_ZEBRA
    #define FEATURE_ZEBRA_FAST
    #define FEATURE_FOCUS_PEAK
    //~ #define FEATURE_FOCUS_PEAK_DISP_FILTER // too slow
    #define FEATURE_MAGIC_ZOOM
    #define FEATURE_CROPMARKS
    #define FEATURE_GHOST_IMAGE
    #define FEATURE_SPOTMETER
    #define FEATURE_FALSE_COLOR
    #define FEATURE_HISTOGRAM
    #define FEATURE_WAVEFORM
    #define FEATURE_VECTORSCOPE

    #define FEATURE_OVERLAYS_IN_PLAYBACK_MODE


#if defined(CONFIG_RAW_PHOTO) || defined(CONFIG_RAW_LIVEVIEW)
    #define FEATURE_RAW_HISTOGRAM
    #define FEATURE_RAW_SPOTMETER
#endif

#ifdef CONFIG_RAW_PHOTO
    #define FEATURE_RAW_ZEBRAS
#endif

/** Movie menu **/
    #define FEATURE_NITRATE
    //~ #define FEATURE_NITRATE_WAV_RECORD // not user-proof at all
    //~ #define FEATURE_VIDEO_HACKS // for experts only

    #define FEATURE_REC_INDICATOR
    #define FEATURE_MOVIE_LOGGING
    #define FEATURE_MOVIE_RESTART
    //~ #define FEATURE_MOVIE_AUTOSTOP_RECORDING // will be moved to scripts
    #define FEATURE_REC_NOTIFY
    //~ #define FEATURE_REC_NOTIFY_BEEP // this may interfere with sound recording, so only enable it where we know it works
    #define FEATURE_MOVIE_REC_KEY
    #define FEATURE_FORCE_LIVEVIEW

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_SHUTTER_LOCK
#endif

#ifdef CONFIG_FRAME_ISO_OVERRIDE
    #define FEATURE_GRADUAL_EXPOSURE
#endif

    #define FEATURE_FPS_OVERRIDE
    #define FEATURE_FPS_RAMPING
    #define FEATURE_FPS_WAV_RECORD

#ifdef CONFIG_FRAME_ISO_OVERRIDE
    #define FEATURE_HDR_VIDEO
    //~ #define FEATURE_HDR_EXTENDED // for experts only
#endif

    #define FEATURE_IMAGE_EFFECTS

    #define FEATURE_VIGNETTING_CORRECTION

#ifdef CONFIG_FRAME_SHUTTER_OVERRIDE
    #define FEATURE_SHUTTER_FINE_TUNING
#endif

    // 50D movie hacks
    //~ #define FEATURE_MOVIE_RECORDING_50D
    //~ #define FEATURE_MOVIE_RECORDING_50D_SHUTTER_HACK
    //~ #define FEATURE_LVAE_EXPO_LOCK

/** Shoot menu **/

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_HDR_BRACKETING
#endif
    #define FEATURE_INTERVALOMETER
    //~ #define FEATURE_INTERVALOMETER_MOVIE // will be moved to scripts

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_BULB_RAMPING
    #define FEATURE_BULB_TIMER
#endif

#ifdef CONFIG_LCD_SENSOR
    #define FEATURE_LCD_SENSOR_REMOTE
#endif
    #define FEATURE_AUDIO_REMOTE_SHOT
    #define FEATURE_MOTION_DETECT
    #define FEATURE_SILENT_PIC
    //~ #define FEATURE_SILENT_PIC_HIRES
    //~ #define FEATURE_SILENT_PIC_JPG // bleeding edge

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_MLU
    #define FEATURE_MLU_HANDHELD

    //~ #define FEATURE_MLU_HANDHELD_DEBUG
    //~ #define FEATURE_MLU_DIRECT_PRINT_SHORTCUT // for 5Dc

    #define FEATURE_FLASH_TWEAKS
    #define FEATURE_FLASH_NOFLASH
//~ #define FEATURE_LV_3RD_PARTY_FLASH // rebels only

    // can do permanent damage, for research only!
    //~ #define FEATURE_PICQ_DANGEROUS
#endif

    #define FEATURE_SNAP_SIM


#ifdef CONFIG_RAW_LIVEVIEW
    #define FEATURE_SILENT_PIC_RAW_BURST
    //~ #define FEATURE_SILENT_PIC_RAW // - compatibility mode; define only one of those, not both
#endif

#ifdef CONFIG_RAW_PHOTO
    #define FEATURE_POST_DEFLICKER
#endif

/** Focus menu **/
    #define FEATURE_TRAP_FOCUS

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_FOLLOW_FOCUS
    #define FEATURE_RACK_FOCUS
    #define FEATURE_FOCUS_STACKING
    #define FEATURE_AF_PATTERNS
    //~ #define FEATURE_MOVIE_AF // not reliable
#endif

/** Display menu **/
    #define FEATURE_LV_BRIGHTNESS_CONTRAST
    #define FEATURE_LV_SATURATION
    #define FEATURE_LV_DISPLAY_GAIN
    #define FEATURE_COLOR_SCHEME
    #define FEATURE_CLEAR_OVERLAYS

#ifdef CONFIG_DISPLAY_FILTERS
    #define FEATURE_DEFISHING_PREVIEW
    #define FEATURE_ANAMORPHIC_PREVIEW
#endif

#ifdef CONFIG_ELECTRONIC_LEVEL
#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_LEVEL_INDICATOR
#endif
#endif

    #define FEATURE_SCREEN_LAYOUT
    #define FEATURE_IMAGE_POSITION
    #define FEATURE_UPSIDE_DOWN

#ifdef CONFIG_VARIANGLE_DISPLAY
#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_IMAGE_ORIENTATION   // for flip-out display only
    #define FEATURE_AUTO_MIRRORING_HACK
#endif
#endif

    #define FEATURE_FORCE_HDMI_VGA
    #define FEATURE_UNIWB_CORRECTION

    //~ #define FEATURE_DISPLAY_SHAKE // looks ugly :P

/** Prefs menu **/

    #define FEATURE_SET_MAINDIAL
    #define FEATURE_PLAY_EXPOSURE_FUSION
    #define FEATURE_PLAY_COMPARE_IMAGES
    #define FEATURE_PLAY_TIMELAPSE
    #define FEATURE_PLAY_EXPOSURE_ADJUST
    #define FEATURE_PLAY_422

    #define FEATURE_IMAGE_REVIEW_PLAY
    #define FEATURE_QUICK_ZOOM
#ifdef CONFIG_Q_MENU_PLAYBACK
    #define FEATURE_LV_BUTTON_PROTECT
    #define FEATURE_LV_BUTTON_RATE
#endif
    #define FEATURE_QUICK_ERASE

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_LV_ZOOM_SETTINGS
    #define FEATURE_LV_ZOOM_SHARP_CONTRAST
    #ifdef CONFIG_EXPSIM
    #define FEATURE_LV_ZOOM_AUTO_EXPOSURE
    #endif
    //~ #define FEATURE_ZOOM_TRICK_5D3 // not reliable

    #define FEATURE_LV_FOCUS_BOX_FAST
    #define FEATURE_LV_FOCUS_BOX_SNAP
    #define FEATURE_LV_FOCUS_BOX_AUTOHIDE

    #define FEATURE_ARROW_SHORTCUTS

    #define FEATURE_STICKY_DOF
    #define FEATURE_STICKY_HALFSHUTTER
    //~ #define FEATURE_AUTO_BURST_PICQ // rebels only

#endif
    //~ #define FEATURE_SWAP_MENU_ERASE // useful for 60D only

    #define FEATURE_WARNINGS_FOR_BAD_SETTINGS

#ifdef CONFIG_PROP_REQUEST_CHANGE
    #define FEATURE_POWERSAVE_LIVEVIEW
#endif

    #define FEATURE_LV_DISPLAY_PRESETS

    //~ #define FEATURE_EYEFI_TRICKS  // EyeFi tricks confirmed working only on 600D-60D

    //~ #define FEATURE_KEN_ROCKWELL_ZOOM_5D3

    //~ #define FEATURE_DIGITAL_ZOOM_SHORTCUT

/** Debug menu **/

    //~ #define FEATURE_SHOW_OVERLAY_FPS

    #define FEATURE_SCREENSHOT
    #define FEATURE_SCREENSHOT_422

    #define FEATURE_DONT_CLICK_ME

    #define FEATURE_SHOW_TASKS
    #define FEATURE_SHOW_CPU_USAGE
    #define FEATURE_SHOW_GUI_EVENTS

    #define FEATURE_SHOW_IMAGE_BUFFERS_INFO
    #define FEATURE_SHOW_FREE_MEMORY
    #define FEATURE_SHOW_SHUTTER_COUNT
    #define FEATURE_SHOW_CMOS_TEMPERATURE
    #define FEATURE_SHOW_EDMAC_INFO

    //~ #define FEATURE_SHOW_SIGNATURE // useful when developing
    //~ #define FEATURE_GUIMODE_TEST // dangerous
    //~ #define FEATURE_SHOW_STATE_FPS // useful for finding vsync hooks
