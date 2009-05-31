#ifndef _bmp_h_
#define _bmp_h_

/** \file
 * Drawing routines for the bitmap VRAM.
 *
 * These are Magic Lantern routines to draw shapes and text into
 * the LVRAM for display on the LCD or HDMI output.
 */

#include "dryos.h"

/** Returns a pointer to the BMP vram */
static inline uint8_t *
bmp_vram(void)
{
	return bmp_vram_info.vram2;
}


/** Returns the width, pitch and height of the BMP vram.
 * These should check the current mode since we might be in
 * HDMI output mode, which uses the full 960x540 space.
 */
static inline uint32_t bmp_width(void) { return 720; }
static inline uint32_t bmp_pitch(void) { return 960; }
static inline uint32_t bmp_height(void) { return 480; }


/* Font size and width are hard-coded by the font generation program.
 * There is only one font.
 * At only one size.
 * Deal with it.
 */
#define font_width	8
#define font_height	12
extern const unsigned char font[];


extern void
bmp_printf(
	unsigned		x,
	unsigned		y,
	const char *		fmt,
	...
) __attribute__((format(printf,3,4)));


extern void
bmp_hexdump(
	unsigned		x,
	unsigned		y,
	const void *		buf,
	size_t			len
);


extern void
bmp_puts(
	unsigned		x,
	unsigned		y,
	const char *		s
);


/** Fill a section of bitmap memory with solid color
 * Only has a four-pixel resolution in X.
 */
extern void
bmp_fill(
	uint8_t			color,
	uint32_t		x,
	uint32_t		y,
	uint32_t		w,
	uint32_t		h
);

#endif
