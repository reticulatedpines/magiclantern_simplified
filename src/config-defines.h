/**
 *  Conditional defines for features that apply to certain camera families
 */

#if defined(CONFIG_5DC) || defined(CONFIG_40D)
#define CONFIG_VXWORKS
#endif

#if defined(CONFIG_60D) || defined(CONFIG_7D) || (defined(CONFIG_5D3) && !defined(CONFIG_5D3_MINIMAL))
#define CONFIG_ELECTRONIC_LEVEL
#endif

#if defined(CONFIG_5D2) || defined(CONFIG_5D3) // not sure about 7D
#define CONFIG_AUTO_BRIGHTNESS
#endif

#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_1100D) // maybe 7D too
#define CONFIG_Q_MENU_PLAYBACK // camera has a Q menu in playback mode; this menu can be tweaked a bit (e.g. LV button = Protect or Rate)
#endif

#if defined(CONFIG_600D) || defined(CONFIG_60D) // EyeFi tricks confirmed working only on 600D-60D
#define CONFIG_EYEFI
#endif

#if defined(CONFIG_600D) || defined(CONFIG_60D) // enable some tricks for flip-out display
#define CONFIG_VARIANGLE_DISPLAY
#endif

