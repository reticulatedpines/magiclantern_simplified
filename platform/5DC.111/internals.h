/**
 * Camera internals for 5D classic 1.1.1
 */

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 5Dc port is young, but I think we can enable properties safely. **/
#define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
#define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs VxWorks **/
#define CONFIG_VXWORKS

/** This camera has a full-frame sensor */
#define CONFIG_FULLFRAME

/** This camera does not have LiveView and can't record video **/
//~ #define CONFIG_LIVEVIEW
//~ #define CONFIG_MOVIE

/** This camera has a 4:3 screen, 720x240 **/
#define CONFIG_4_3_SCREEN

/** We have a DirectPrint blue LED **/
#define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

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

/** We can't do bulb exposures **/
//~ #define CONFIG_BULB
//~ #define CONFIG_SEPARATE_BULB_MODE

/** No audio **/
//~ #define CONFIG_AUDIO_CONTROLS

/** No LV, no Magic Zoom **/
//~ #define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can't redirect the display buffer **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can't implement display filters (features that alter the LiveView image in real-time) **/
//~ #define CONFIG_DISPLAY_FILTERS

/** We can't override ISO on a per-frame basis **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE
//~ #define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can't change ExpSim from ML **/
//~ #define CONFIG_EXPSIM

/** We can't playback sounds via ASIF DMA **/
//~ #define CONFIG_BEEP

/** No movie mode **/
//~ #define CONFIG_WB_WORKAROUND

/** We can't restore ML files after formatting the card in the camera **/
//~ #define CONFIG_RESTORE_AFTER_FORMAT

/** We don't know how to use DMA_MEMCPY (yet) **/
//~ #define CONFIG_DMA_MEMCPY

/** No movie mode **/
#define CONFIG_MOVIE_AE_WARNING

/** We can't display extra info in photo mode **/
//~ #define CONFIG_PHOTO_MODE_INFO_DISPLAY
