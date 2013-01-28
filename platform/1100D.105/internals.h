/**
 * Camera internals for 1100D 1.0.5
 */

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 1100D port is pretty stable, so I think we can enable properties safely. **/
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

/** We only have a single red LED **/
//~ #define CONFIG_BLUE_LED

/** There is no LCD sensor that turns the display off **/
//~ #define CONFIG_LCD_SENSOR

/** This camera does not have a mirror lockup feature **/
//~ #define CONFIG_MLU

/** This camera reports focus info in LiveView **/
#define CONFIG_LV_FOCUS_INFO

/** No level sensor **/
//~ #define CONFIG_ELECTRONIC_LEVEL

/** Define this if the camera has an ambient light sensor used for auto brightness **/
//~ #define CONFIG_AUTO_BRIGHTNESS

/** There is a Q menu in Play mode, with image protect, rate etc **/
#define CONFIG_Q_MENU_PLAYBACK

/** No flip-out display **/
//~ #define CONFIG_VARIANGLE_DISPLAY

/** Battery does not report exact percentage **/
//~ #define CONFIG_BATTERY_INFO

/** We can do bulb exposures **/
#define CONFIG_BULB

/** Bulb mode is done by going to M mode and setting shutter speed beyond 30s **/
//~ #define CONFIG_SEPARATE_BULB_MODE

/** We can't control audio settings from ML **/
//~ #define CONFIG_AUDIO_CONTROLS

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

/** We can't change ExpSim from ML :( **/
//~ #define CONFIG_EXPSIM

/** We can playback sounds via ASIF DMA **/
#define CONFIG_BEEP

/** This camera has trouble saving Kelvin and/or WBShift in movie mode, so ML has to do this instead **/
#define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We don't know how to use DMA_MEMCPY (yet) **/
//~ #define CONFIG_DMA_MEMCPY

/** We should't warn the user if movie exposure is Auto **/
//~ #define CONFIG_MOVIE_AE_WARNING

/** We can display extra info in photo mode (not LiveView) **/
#define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** FIO_RenameFile works **/
#define CONFIG_FIO_RENAMEFILE_WORKS

/** Perfect sync using EVF_STATE **/
#define CONFIG_EVF_STATE_SYNC

/** This camera loads ML into the AllocateMemory pool **/
#define CONFIG_ALLOCATE_MEMORY_POOL

/** FPS override: we can change both timer A and B */
//~ #define CONFIG_FPS_TIMER_A_ONLY

/** FPS override: Canon changes FPS registers often; we need to undo their changes asap */
#define CONFIG_FPS_AGGRESSIVE_UPDATE

/** This camera has a mono microphone input, so we should display only one audio meter **/
#define CONFIG_MONO_MIC

/** 1100D has very little RAM; use the malloc/free pool for bitmaps too (and only load small bitmaps) */
#define CONFIG_USE_MALLOC_FOR_SMALL_THINGS
#define CONFIG_USE_MALLOC_FOR_BMP
