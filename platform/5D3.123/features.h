#include "all_features.h"

#undef FEATURE_IMAGE_REVIEW_PLAY // Not needed, one can press Zoom right away
#undef FEATURE_QUICK_ZOOM // Canon has it
#undef FEATURE_QUICK_ERASE // Canon has it
#undef FEATURE_IMAGE_EFFECTS // None working
#undef FEATURE_MOVIE_REC_KEY // Canon has it
#undef FEATURE_NITRATE_WAV_RECORD // Not implemented
#undef FEATURE_AF_PATTERNS // Canon has it
#undef FEATURE_MLU_HANDHELD // Not needed, Canon's silent mode is much better
#undef FEATURE_SHUTTER_LOCK // Canon has a dedicated button for it
#undef FEATURE_FLASH_TWEAKS // No built-in flash
#undef FEATURE_FLASH_NOFLASH


//~ #define FEATURE_KEN_ROCKWELL_ZOOM_5D3 // anybody using it?

#define FEATURE_ZOOM_TRICK_5D3 // Not reliable
//~ #define FEATURE_REMEMBER_LAST_ZOOM_POS_5D3 // Too many conflicts with other features
#undef FEATURE_IMAGE_POSITION

//~ #define FEATURE_VIDEO_HACKS

#define CONFIG_HEXDUMP

#undef FEATURE_VOICE_TAGS // No sound recorded

#define FEATURE_LV_FOCUS_BOX_SNAP_TO_X5_RAW




/* The features below were broken by the 1.2.3 update (they were working on 1.1.3) */
/* Canon changed a lot of things on the display side, and a lot of DIGIC (ENGIO) registers
 * have to be found again. Most of them were found by blind (brute force) register pokes in older cameras:
 * http://magiclantern.wikia.com/wiki/Register_Map/Brute_Force
 */

/*
 * The fast zebras are no longer implemented with C0F140cc;
 * things were moved to 0xC0F14394 and some other registers that must be reverse-engineered
 * Look at 0xff12e304 set_fast_zebras to find the registers 
 * (e.g. 0xC0F14394 is at MEM(0x42D3C-8), where 0x42D3C is fast_zebras_struct.off_0x14)
 * then grab DIGIC Poke and try to understand what each bit field does.
 * So far: threshold unchanged, highlight color now uses 8 bits, could not find underexposure and blinking flags.
 * 0x0800f8 does red overexposure zebras.
 * 
 * To find Canon values written to these registers, run this code while toggling Canon overexposure warnings back and forth:
 * 
 * static void spy_zebras()
 * {
 *   if (display_is_on())
 *       bmp_printf(FONT_MED, 0, 0, "%x:%x    \n%x:%x   ", MEM(0x42D3C-8),MEM(0x42D3C), MEM(0x42D48-8), MEM(0x42D48));
 * }
 * gdb_add_watchpoint(0xff12de24, 0, &spy_zebras);
 * 
 */
#undef FEATURE_ZEBRA_FAST

/* Pitch adjustment register 0xc0f140e8 was changed */
#undef FEATURE_MAGIC_ZOOM_FULL_SCREEN

/* Brightness/contrast were moved to different registers (probably register values changed too) */
#undef FEATURE_LV_BRIGHTNESS_CONTRAST
#undef FEATURE_LV_SATURATION

/* Swap UV works (anybody using it?), extreme chroma doesn't */
#undef FEATURE_LV_CRAZY_COLORS
