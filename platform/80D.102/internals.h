/**
 * Camera internals for 80D 1.0.2
 */

/** This camera has both a CF and an SD slot **/
#define CONFIG_DUAL_SLOT

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
//~ #define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
//~ #define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs DryOS **/
//~ #define CONFIG_VXWORKS

/** This camera has a DIGIC VI chip */
#define CONFIG_DIGIC_VI

/** This camera has an APS-C sensor */
//~ #define CONFIG_FULLFRAME

/** This camera has LiveView and can record video **/
#define CONFIG_LIVEVIEW
#define CONFIG_MOVIE

/* not sure about the others; disable them for now */

/** This camera has a 3:2 screen, I think **/
//~ #define CONFIG_4_3_SCREEN

/** We don't have a DirectPrint blue LED **/
// #define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

/** This camera has a mirror lockup feature **/
//~ #define CONFIG_MLU

/** This camera reports focus info in LiveView **/
//~ #define CONFIG_LV_FOCUS_INFO

/** Reports roll angle (maybe pitch too?) **/
//~ #define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
//~ #define CONFIG_AUTO_BRIGHTNESS // not 100% sure

/** There is a Q menu in Play mode, with image protect, rate etc **/
//~ #define CONFIG_Q_MENU_PLAYBACK

/** No flip-out display **/
//~ #define CONFIG_VARIANGLE_DISPLAY

/** It has a 5D2-like battery which reports exact percentage **/
//~ #define CONFIG_BATTERY_INFO

/** We can do bulb exposures **/
//~ #define CONFIG_BULB

/** There is a separate bulb mode on the mode dial (other cameras have BULB = M + shutter speed beyond 30s) **/
//~ #define CONFIG_SEPARATE_BULB_MODE

/** We can control audio settings from ML**/
//~ #define CONFIG_AUDIO_CONTROLS

/** Zoom button can't be used while recording (for Magic Zoom) **/
//~ #define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can redirect the display buffer but not easily **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** So we can implement 5DII style display filters. **/
//~ #define CONFIG_DISPLAY_FILTERS

/** Not sure whether we can override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
/** Will leave it off for now **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE
//~ #define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can change ExpSim from ML **/
//~ #define CONFIG_EXPSIM

/** We can playback sounds via ASIF DMA **/
//~ #define CONFIG_BEEP

/** This camera has no trouble saving Kelvin and/or WBShift in movie mode **/
//~ #define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We know how to use DMA_MEMCPY, of course :) **/
//~ #define CONFIG_DMA_MEMCPY

/** We should not warn the user if movie exposure is Auto **/
//~ #define CONFIG_MOVIE_AE_WARNING

/** We can display some extra info in photo mode (not LiveView) **/
//~ #define CONFIG_PHOTO_MODE_INFO_DISPLAY

/* we can use virtual keyboard here */
//#define CONFIG_VKBD_IMPLEMENTED

/** We can record movies in regular photo modes - M, P, Tv, Av... */
//~ #define CONFIG_NO_DEDICATED_MOVIE_MODE

/** We have access to raw data in both photo mode and in LiveView */
//~ #define CONFIG_RAW_PHOTO
//~ #define CONFIG_RAW_LIVEVIEW

/** We know how to unregister properties */
/** (but that doesn't mean we should use it, because existing prop handlers were not designed with this in mind */
//~ #define CONFIG_UNREGISTER_PROP

/** We can adjust AFMA (AF microadjustment) */
//~ #define CONFIG_AFMA

/** The camera accepts extended AFMA values (default range: -20...20; extended: -100...100) */
/** This can be dangerous, as the values are outside Canon limits */
//~ #define CONFIG_AFMA_EXTENDED

/** We can use the DMA controller to copy data */
//~ #define CONFIG_EDMAC_MEMCPY

/** LV RAW has trouble with 10x zoom, disable it */
//~ #define CONFIG_RAW_DISABLE_IN_10X_ZOOM

/** Use joystick for one-finger menu navigation */
//~ #define CONFIG_JOY_CENTER_ACTIONS
