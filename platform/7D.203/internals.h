/**
 * Camera internals for 7D 2.0.3
 */

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 7D port is young, but for development we can enable properties safely. **/
#define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
#define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs DryOS **/
//~ #define CONFIG_VXWORKS

/** This camera has an APS-C sensor */
//~ #define CONFIG_FULLFRAME

/** This camera has LiveView and can record video **/
#define CONFIG_LIVEVIEW
#define CONFIG_MOVIE

/** This camera has a 4:3 screen, 720x480 **/
#define CONFIG_4_3_SCREEN

/** We don't have a DirectPrint blue LED **/
// #define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

/** This camera has a mirror lockup feature **/
#define CONFIG_MLU

/** This camera reports focus info in LiveView **/
#define CONFIG_LV_FOCUS_INFO

/** Reports roll angle (maybe pitch too?) **/
#define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
#define CONFIG_AUTO_BRIGHTNESS // not 100% sure

/** There is a Q menu in Play mode, with image protect, rate etc **/
#define CONFIG_Q_MENU_PLAYBACK

/** No flip-out display **/
//~ #define CONFIG_VARIANGLE_DISPLAY

/** It has a 5D2-like battery which reports exact percentage **/
#define CONFIG_BATTERY_INFO

/** We can do bulb exposures **/
#define CONFIG_BULB

/** There is a separate bulb mode on the mode dial (other cameras have BULB = M + shutter speed beyond 30s) **/
#define CONFIG_SEPARATE_BULB_MODE

/** We can't control audio settings from ML **/
//~ #define CONFIG_AUDIO_CONTROLS

/** Zoom button can't be used while recording (for Magic Zoom) **/
#define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can't redirect the display buffer **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can't implement display filters (features that alter the LiveView image in real-time) **/
//~ #define CONFIG_DISPLAY_FILTERS

/** Not sure whether we can override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
/** Will leave it off for now **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE
//~ #define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can change ExpSim from ML **/
#define CONFIG_EXPSIM

/** We can playback sounds via ASIF DMA **/
#define CONFIG_BEEP

/** This camera has no trouble saving Kelvin and/or WBShift in movie mode **/
//~ #define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We know how to use DMA_MEMCPY, of course :) **/
#define CONFIG_DMA_MEMCPY

/** We should not warn the user if movie exposure is Auto **/
//~ #define CONFIG_MOVIE_AE_WARNING

/** We can display some extra info in photo mode (not LiveView) **/
#define CONFIG_PHOTO_MODE_INFO_DISPLAY

/* we can use virtual keyboard here */
//#define CONFIG_VKBD_IMPLEMENTED

/** We can record movies in regular photo modes - M, P, Tv, Av... */
#define CONFIG_NO_DEDICATED_MOVIE_MODE

/** Advanced EXMEM stubs are known and can be used */
#define CONFIG_FULL_EXMEM_SUPPORT
