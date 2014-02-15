/**
 * Camera internals for EOS-M 1.0.6
 */

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The EOS_M port is very young, so we don't enable these for now. **/
#define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
#define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs DryOS **/
//~ #define CONFIG_VXWORKS

/** This camera has a DIGIC V chip */
#define CONFIG_DIGIC_V

/** This camera has an APS-C sensor */
//~ #define CONFIG_FULLFRAME

/** This camera has LiveView and can record video **/
#define CONFIG_LIVEVIEW
#define CONFIG_MOVIE

/** This camera has a 3:2 screen, 720x480 **/
#define CONFIG_3_2_SCREEN

/** We only have a single LED **/
//~ #define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

/** This camera has no miror **/
//~ #define CONFIG_MLU

/** This camera reports focus info in LiveView **/
//~ #define CONFIG_LV_FOCUS_INFO

/** Sensor gives some data Needs Help **/
//~ #define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
//~ #define CONFIG_AUTO_BRIGHTNESS

/** idk **/
//~ #define CONFIG_Q_MENU_PLAYBACK

/** No flip-out display **/
//~ #define CONFIG_VARIANGLE_DISPLAY

/** Battery does not report exact percentage **/
//~ #define CONFIG_BATTERY_INFO

/** We can do bulb exposures (well, I hope) **/
#define CONFIG_BULB

/** There is no Bulb Mode **/
//~ #define CONFIG_SEPARATE_BULB_MODE

/** We can't control audio settings from ML **/
//~ #define CONFIG_AUDIO_CONTROLS

/** No zoom button **/
#define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can redirect the display buffer to some arbitrary address, just by changing YUV422_LV_BUFFER_DISPLAY_ADDR **/
/** Well, I hope so **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
// ~#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can implement display filters (features that alter the LiveView image in real-time) **/
#define CONFIG_DISPLAY_FILTERS

/** We can override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
#define CONFIG_FRAME_ISO_OVERRIDE

/** But we can't override the digital ISO component via FRAME_ISO **/
#define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can also override shutter on a per-frame basis */
#define CONFIG_FRAME_SHUTTER_OVERRIDE

/** We can't change ExpSim from ML (at least not yet) **/
#define CONFIG_EXPSIM

/** Asif hangs camera.. will look for a fix **/
//~ #define CONFIG_BEEP

/** This camera has no trouble saving Kelvin and/or WBShift in movie mode **/
#define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We don't know how to use DMA_MEMCPY (yet) **/
#define CONFIG_DMA_MEMCPY

/** We know how to use edmac_memcpy. This one is really fast (600MB/s!) */
#define CONFIG_EDMAC_MEMCPY

/** We shouldn't warn the user if movie exposure is Auto **/
#define CONFIG_MOVIE_AE_WARNING

/** No photo mode outside LiveView **/
//~ #define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** No additional_version stub on this DryOS version **/
#define CONFIG_NO_ADDITIONAL_VERSION

/** Touchscreen support **/
// Needs more hacking, I'll fix it once i get the EOSM - nanomad
//~ #define CONFIG_TOUCHSCREEN

/** Perfect sync using EVF_STATE **/
#define CONFIG_EVF_STATE_SYNC

/** FPS override: Canon changes FPS registers often; we need to undo their changes asap */
#define CONFIG_FPS_AGGRESSIVE_UPDATE

/** FIO_RenameFile works **/
#define CONFIG_FIO_RENAMEFILE_WORKS

/** FPS override: change timers from EVF state */
#define CONFIG_FPS_UPDATE_FROM_EVF_STATE

/** There is a Movie Mode, needs research */
#define CONFIG_NO_DEDICATED_MOVIE_MODE

/** We have access to Raw data */
#define CONFIG_RAW_LIVEVIEW
#define CONFIG_RAW_PHOTO

/** We know how to use engine resource locks */
#define CONFIG_ENGINE_RESLOCK

/** We can control audio settings from ML **/
//~ #define CONFIG_AUDIO_CONTROLS
