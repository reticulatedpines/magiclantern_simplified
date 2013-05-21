/**
 * Camera internals for 500D 1.1.1
 */

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 500D port is fairly stable, so I think we can enable properties safely. **/
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

/** We have a DirectPrint blue LED **/
#define CONFIG_BLUE_LED

/** We have LCD sensor that turns the display off **/
#define CONFIG_LCD_SENSOR

/** This camera has a mirror lockup feature **/
#define CONFIG_MLU

/** This camera does not report focus info in LiveView **/
//~ #define CONFIG_LV_FOCUS_INFO

/** No level sensor **/
//~ #define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
//~ #define CONFIG_AUTO_BRIGHTNESS

/** No Q menu in Play mode **/
//~ #define CONFIG_Q_MENU_PLAYBACK

/** No flip-out display **/
//~ #define CONFIG_VARIANGLE_DISPLAY

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

/** We can't redirect the display buffer **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can't implement display filters (features that alter the LiveView image in real-time) **/
//~ #define CONFIG_DISPLAY_FILTERS

/** We can't override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE

/** And, of course, we can't mess with the digital ISO component via FRAME_ISO **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can change ExpSim from ML **/
#define CONFIG_EXPSIM

/** We can playback sounds via ASIF DMA **/
#define CONFIG_BEEP

/** This camera has trouble saving Kelvin and/or WBShift in movie mode, so ML has to do this instead **/
#define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We don't know how to use DMA_MEMCPY (yet) **/
//~ #define CONFIG_DMA_MEMCPY

/** We should not warn the user if movie exposure is Auto, because that's the only setting **/
//~ #define CONFIG_MOVIE_AE_WARNING

/** We can display some extra info in photo mode (not LiveView) **/
#define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** No auto ISO limits in Canon menus **/
#define CONFIG_NO_AUTO_ISO_LIMITS

/** FPS override: we can change both timer A and B */
//~ #define CONFIG_FPS_TIMER_A_ONLY

/** FPS override: Canon changes FPS registers often; we need to undo their changes asap */
#define CONFIG_FPS_AGGRESSIVE_UPDATE

/** This camera has a mono microphone input, so we should display only one audio meter **/
#define CONFIG_MONO_MIC

/** You can't AF by pressing shutter halfway in LiveView */
#define CONFIG_NO_HALFSHUTTER_AF_IN_LIVEVIEW

/** We can use the DMA controller to copy data */
#define CONFIG_EDMAC_MEMCPY
