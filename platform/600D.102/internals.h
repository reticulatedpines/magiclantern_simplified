/**
 * Camera internals for 600D 1.0.2
 */

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 600D port is pretty stable, so I think we can enable properties safely. **/
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

/** This camera has a 3:2 screen, 720x480 **/
#define CONFIG_3_2_SCREEN

/** We only have a single red LED **/
//~ #define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

/** This camera has a mirror lockup feature **/
#define CONFIG_MLU

/** This camera reports focus info in LiveView **/
#define CONFIG_LV_FOCUS_INFO

/** No level sensor **/
//~ #define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
//~ #define CONFIG_AUTO_BRIGHTNESS

/** There is a Q menu in Play mode, with image protect, rate etc **/
#define CONFIG_Q_MENU_PLAYBACK

/** It has a flip-out display **/
#define CONFIG_VARIANGLE_DISPLAY

/** Battery does not report exact percentage **/
//~ #define CONFIG_BATTERY_INFO

/** We can do bulb exposures **/
#define CONFIG_BULB

/** Bulb mode is done by going to M mode and setting shutter speed beyond 30s **/
//~ #define CONFIG_SEPARATE_BULB_MODE

/** We can control audio settings from ML **/
#define CONFIG_AUDIO_CONTROLS

/** Zoom button can be used while recording (for Magic Zoom) **/
//~ #define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can redirect the display buffer to some arbitrary address, just by changing YUV422_LV_BUFFER_DISPLAY_ADDR **/
#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can implement display filters (features that alter the LiveView image in real-time) **/
#define CONFIG_DISPLAY_FILTERS

/** We can override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
#define CONFIG_FRAME_ISO_OVERRIDE

/** But we can't override the digital ISO component via FRAME_ISO **/
#define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can also override shutter on a per-frame basis */
#define CONFIG_FRAME_SHUTTER_OVERRIDE

/** We can't change ExpSim from ML :( **/
//~ #define CONFIG_EXPSIM

/** We can playback sounds via ASIF DMA **/
#define CONFIG_BEEP

/** This camera has trouble saving Kelvin and/or WBShift in movie mode, so ML has to do this instead **/
#define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We know how to use DMA_MEMCPY **/
#define CONFIG_DMA_MEMCPY

/** We should warn the user if movie exposure is Auto, otherwise he may report it as a bug **/
#define CONFIG_MOVIE_AE_WARNING

/** We can display some extra info in photo mode (not LiveView) **/
#define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** Show 4 char if camera support only 3 in photo mode (not LiveView) **/
//~#define AVAIL_SHOT_WORKAROUND // not needed on 600D

/** FIO_RenameFile works **/
#define CONFIG_FIO_RENAMEFILE_WORKS

/** Perfect sync using EVF_STATE **/
#define CONFIG_EVF_STATE_SYNC

/** This camera loads ML into the AllocateMemory pool **/
//#define CONFIG_ALLOCATE_MEMORY_POOL

/** We have access to raw data in both photo mode and in LiveView */
#define CONFIG_RAW_LIVEVIEW
#define CONFIG_RAW_PHOTO

/** for 600D */
#define CONFIG_EDMAC_MEMCPY

/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#define CONFIG_EDMAC_RAW_SLURP
