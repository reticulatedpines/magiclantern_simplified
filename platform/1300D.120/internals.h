/**
 * Camera internals for 1300D 1.2.0
 */
#define CONFIG_DIGIC_IV
#define CONFIG_DIGIC_45

/** Properties are persistent (saved in NVRAM) => a mistake can cause permanent damage. Undefine this for new ports. */
/** The 1300D port is very early, so I think we should not enable properties. **/
#define CONFIG_PROP_REQUEST_CHANGE

/** 
 * State object hooks are pieces of code that run in Canon tasks (state objects). See state-object.c . 
 * They might slow down Canon code, so here you can disable all of them (useful for debugging or early ports) 
 */
/** No additional_version stub on this DryOS version **/
#define CONFIG_NO_ADDITIONAL_VERSION

#define CONFIG_STATE_OBJECT_HOOKS

/** This camera runs DryOS **/
//~ #define CONFIG_VXWORKS

/** This camera has a DIGIC IV chip */
//~#define CONFIG_DIGIC_IV
//~#define CONFIG_KILL_FLICKER

/** This camera uses new-style DryOS task hooks */
#define CONFIG_NEW_DRYOS_TASK_HOOKS

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
/* to be checked */
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
//#define CONFIG_BATTERY_INFO

/** We can do bulb exposures **/
#define CONFIG_BULB

/** Bulb mode is done by going to M mode and setting shutter speed beyond 30s **/
//~ #define CONFIG_SEPARATE_BULB_MODE

/** We can't control audio settings from ML **/
//~ #define CONFIG_AUDIO_CONTROLS

/** Zoom button can be used while recording (for Magic Zoom) **/
/* to be checked */
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

/** We can change ExpSim from ML **/
/* to be checked */
#define CONFIG_EXPSIM

/** We can playback sounds via ASIF DMA **/
//#define CONFIG_BEEP
#undef CONFIG_BEEP

/** This camera has trouble saving Kelvin and/or WBShift in movie mode, so ML has to do this instead **/
/* to be checked */
#define CONFIG_WB_WORKAROUND

/** We can restore ML files after formatting the card in the camera **/
#define CONFIG_RESTORE_AFTER_FORMAT

/** We can use DMA_MEMCPY but it has no real benefit **/
//~ #define CONFIG_DMA_MEMCPY

/** We know how to use edmac_memcpy. This one is really fast (600MB/s!) */
#define CONFIG_EDMAC_MEMCPY

/** We should warn the user if movie exposure is Auto **/
#define CONFIG_MOVIE_AE_WARNING

/** We can display some extra info in photo mode (not LiveView) **/
#define CONFIG_PHOTO_MODE_INFO_DISPLAY

/** No additional_version stub on this DryOS version **/
#define CONFIG_NO_ADDITIONAL_VERSION

/** FIO_RenameFile works **/
#define CONFIG_FIO_RENAMEFILE_WORKS

/** Perfect sync using EVF_STATE **/
#define CONFIG_EVF_STATE_SYNC

/** This camera loads ML into the AllocateMemory pool **/
//#define CONFIG_ALLOCATE_MEMORY_POOL

/** FPS override: we can change both timer A and B */
//~ #define CONFIG_FPS_TIMER_A_ONLY

/** FPS override: Canon changes FPS registers often; we need to undo their changes asap */

#define CONFIG_FPS_AGGRESSIVE_UPDATE

/** This camera has a mono microphone input, so we should display only one audio meter **/
#define CONFIG_MONO_MIC

/** This camera uses the exposure comp button to open ML menu */
#define CONFIG_MENU_WITH_AV

/** We don't have access to Raw data (yet) */
#define CONFIG_RAW_LIVEVIEW
#define CONFIG_RAW_PHOTO


/** Hide Canon bottom bar from DebugMsg hook */
//cristi
//~#define CONFIG_LVAPP_HACK_DEBUGMSG

/** Workaround for menu timeout in LiveView */
// not work on 1300D 
//#define CONFIG_MENU_TIMEOUT_FIX

/** Use a patched LiveViewApp dialog hander to hide Canon bottom bar */
/** FIXME: long calls during relocation **/
//~ #define CONFIG_LVAPP_HACK_RELOC

/** this method bypasses Canon's lv_save_raw and slurps the raw data directly from connection #0 */
#define CONFIG_EDMAC_RAW_SLURP
