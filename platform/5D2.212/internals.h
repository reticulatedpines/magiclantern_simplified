/**
 * Camera internals for 5D2 2.1.2
 */

/** This camera has a CF slot */
#define CONFIG_CF_SLOT

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 5D2 port is pretty stable, so I think we can enable properties safely. **/
#define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
#define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs DryOS **/
//~ #define CONFIG_VXWORKS

/** This camera has a full-frame sensor */
#define CONFIG_FULLFRAME

/** This camera has LiveView and can record video **/
#define CONFIG_LIVEVIEW
#define CONFIG_MOVIE

/** This camera has a 4:3 screen, 720x480 **/
#define CONFIG_4_3_SCREEN

/** We have a DirectPrint blue LED **/
#define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

/** This camera has a mirror lockup feature **/
#define CONFIG_MLU

/** This camera reports focus info in LiveView **/
#define CONFIG_LV_FOCUS_INFO

/** No level sensor **/
//~ #define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
#define CONFIG_AUTO_BRIGHTNESS

/** No Q menu in Play mode **/
//~ #define CONFIG_Q_MENU_PLAYBACK

/** No flip-out display **/
//~ #define CONFIG_VARIANGLE_DISPLAY

/** Battery reports exact percentage **/
#define CONFIG_BATTERY_INFO

/** We can do bulb exposures **/
#define CONFIG_BULB

/** There is a separate bulb mode on the mode dial (other cameras have BULB = M + shutter speed beyond 30s) **/
#define CONFIG_SEPARATE_BULB_MODE

/** We can control audio settings from ML **/
#define CONFIG_AUDIO_CONTROLS

/** Zoom button can't be used while recording (for Magic Zoom) **/
#define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can redirect the display buffer to some arbitrary address, but only with an ugly hack **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can implement display filters (features that alter the LiveView image in real-time) **/
#define CONFIG_DISPLAY_FILTERS

/** We can override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
#define CONFIG_FRAME_ISO_OVERRIDE

/** And we can override the digital ISO component via FRAME_ISO too **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can change ExpSim from ML **/
#define CONFIG_EXPSIM

/** We can set ExpSim to Movie too (not just photo) **/
#define CONFIG_EXPSIM_MOVIE

/** We can playback sounds via ASIF DMA **/
#define CONFIG_BEEP

/** This camera has no trouble saving Kelvin and/or WBShift in movie mode **/
//~ #define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We don't know how to use DMA_MEMCPY (yet) **/
//~ #define CONFIG_DMA_MEMCPY

/** We should warn the user if movie exposure is Auto, otherwise he may report it as a bug **/
#define CONFIG_MOVIE_AE_WARNING

/** We can display some extra info in photo mode (not LiveView) **/
#define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** No auto ISO limits in Canon menus **/
#define CONFIG_NO_AUTO_ISO_LIMITS

/** You can't AF by pressing shutter halfway in LiveView */
#define CONFIG_NO_HALFSHUTTER_AF_IN_LIVEVIEW

/** We can record movies in regular photo modes - M, P, Tv, Av... */
#define CONFIG_NO_DEDICATED_MOVIE_MODE

/** We can adjust AFMA (AF microadjustment) */
#define CONFIG_AFMA

/** The camera accepts extended AFMA values (default range: -20...20; extended: -100...100) */
/** This can be dangerous, as the values are outside Canon limits */
#define CONFIG_AFMA_EXTENDED

/** We can use the DMA controller to copy data */
#define CONFIG_EDMAC_MEMCPY

/** We have access to raw data in both photo mode and in LiveView */
#define CONFIG_RAW_PHOTO
#define CONFIG_RAW_LIVEVIEW
