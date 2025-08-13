#define CONFIG_HELLO_WORLD
#define FEATURE_VRAM_RGBA

#define CONFIG_COMPOSITOR_XCM
#define CONFIG_COMPOSITOR_DEDICATED_LAYER

// We can't yet rely on image capture.  Cam crashes due to null pointer,
// I think?  If it fails to AF lock, for example.
#define CONFIG_IMAGE_CAPTURE_NOT_WORKING

#undef CONFIG_CRASH_LOG
#undef CONFIG_PROP_REQUEST_CHANGE
#undef CONFIG_AUTOBACKUP_ROM
