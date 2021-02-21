/**
 * Camera internals for 50D 1.0.9
 */

/** This camera has a CF slot */
#define CONFIG_CF_SLOT

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 50D port is pretty stable, so I think we can enable properties safely. **/
#define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
#define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs DryOS **/
//~ #define CONFIG_VXWORKS

/** This camera has a DIGIC IV chip */
#define CONFIG_DIGIC_IV

/** This camera has an APS-C sensor */
//~ #define CONFIG_FULLFRAME

/** This camera loads ML into the AllocateMemory pool **/
//~ #define CONFIG_ALLOCATE_MEMORY_POOL

/** This camera has LiveView and can record video [ ;) ] **/
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

/** No audio **/
//~ #define CONFIG_AUDIO_CONTROLS

/** Zoom button can be used while recording (for Magic Zoom) - if I remember well **/
//~ #define CONFIG_ZOOM_BTN_NOT_WORKING_WHILE_RECORDING

/** We can redirect the display buffer **/
//~ #define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER_EASILY
#define CONFIG_CAN_REDIRECT_DISPLAY_BUFFER

/** Therefore, we can implement display filters (features that alter the LiveView image in real-time) **/
#define CONFIG_DISPLAY_FILTERS

/** We can override ISO on a per-frame basis, by changing FRAME_ISO (e.g. for HDR video or gradual exposure) **/
#define CONFIG_FRAME_ISO_OVERRIDE

/** We can also override shutter on a per-frame basis */
/* really? no other old camera can do this */
//~ #define CONFIG_FRAME_SHUTTER_OVERRIDE

/** And we can override the digital ISO component via FRAME_ISO too **/
//~ #define CONFIG_FRAME_ISO_OVERRIDE_ANALOG_ONLY

/** We can change ExpSim from ML **/
#define CONFIG_EXPSIM

/** We can't playback sounds via ASIF DMA **/
//~ #define CONFIG_BEEP

/** This camera has no trouble saving Kelvin and/or WBShift in movie mode **/
//~ #define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** DMA_MEMCPY  **/
//~ #define CONFIG_DMA_MEMCPY

/** We shouldn't warn the user if movie exposure is Auto **/
//~ #define CONFIG_MOVIE_AE_WARNING

/** We can't display some extra info in photo mode (not LiveView) - things are misaligned **/
//~ #define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** Canon drawing code has to be disabled in LiveView, otherwise it will erase all ML graphics **/
#define CONFIG_KILL_FLICKER

/** No auto ISO limits in Canon menus **/
#define CONFIG_NO_AUTO_ISO_LIMITS

/** We can adjust AFMA (AF microadjustment) */
#define CONFIG_AFMA

/** The camera accepts extended AFMA values (default range: -20...20; extended: -100...100) */
/** This can be dangerous, as the values are outside Canon limits */
#define CONFIG_AFMA_EXTENDED

/** We can record movies in regular photo modes - M, P, Tv, Av... */
#define CONFIG_NO_DEDICATED_MOVIE_MODE

/** We can use the DMA controller to copy data */
#define CONFIG_EDMAC_MEMCPY

/** We have access to raw data in both photo mode and in LiveView */
#define CONFIG_RAW_PHOTO
#define CONFIG_RAW_LIVEVIEW

/** There are no manual exposure controls in movie mode => we need expo override */
#define CONFIG_NO_MANUAL_EXPOSURE_MOVIE

/** LV RAW has trouble with 10x zoom, disable it */
#define CONFIG_RAW_DISABLE_IN_10X_ZOOM
#define CONFIG_RAW_DISABLE_IN_10X_ZOOM_WEAK

/** Use joystick for one-finger menu navigation */
#define CONFIG_LONG_PRESS_JOYSTICK_MENU

/** Focus commands are not confirmed in LiveView (lens_focus - requires pfAfComplete_counter stub) */
#define CONFIG_FOCUS_COMMANDS_PROP_NOT_CONFIRMED
