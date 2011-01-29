#ifndef _bmp_h_
#define _bmp_h_

/** \file
 * Drawing routines for the bitmap VRAM.
 *
 * These are Magic Lantern routines to draw shapes and text into
 * the LVRAM for display on the LCD or HDMI output.
 */

/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"
#include "font.h"

extern int bmp_enabled;

/** Returns a pointer to the real BMP vram */
static inline uint8_t *
bmp_vram(void)
{
	return bmp_vram_info[1].vram2;
}
#define BMPPITCH 960

/** Font specifiers include the font, the fg color and bg color */
#define FONT_MASK		0x000F0000
//~ #define FONT_HUGE		0x00080000
#define FONT_LARGE		0x00030000
#define FONT_MED		0x00020000
#define FONT_SMALL		0x00010000

#define FONT(font,fg,bg)	( 0 \
	| ((font) & FONT_MASK) \
	| ((bg) & 0xFF) << 8 \
	| ((fg) & 0xFF) << 0 \
)

static inline struct font *
fontspec_font(
	unsigned		fontspec
)
{
	switch( fontspec & FONT_MASK )
	{
	default:
	case FONT_SMALL:	return &font_small;
	case FONT_MED:		return &font_med;
	case FONT_LARGE:	return &font_large;
	//~ case FONT_HUGE:		return &font_huge;
	}
}


static inline unsigned
fontspec_fg(
	unsigned		fontspec
)
{
	return (fontspec >> 0) & 0xFF;
}

static inline unsigned
fontspec_bg(
	unsigned		fontspec
)
{
	return (fontspec >> 8) & 0xFF;
}



static inline unsigned
fontspec_height(
	unsigned		fontspec
)
{
	return fontspec_font(fontspec)->height;
}


extern void
bmp_printf(
	unsigned		fontspec,
	unsigned		x,
	unsigned		y,
	const char *		fmt,
	...
) __attribute__((format(printf,4,5)));

extern void
con_printf(
	unsigned		fontspec,
	const char *		fmt,
	...
) __attribute__((format(printf,2,3)));

extern void
bmp_hexdump(
	unsigned		fontspec,
	unsigned		x,
	unsigned		y,
	const void *		buf,
	size_t			len
);


extern void
bmp_puts(
	unsigned		fontspec,
	unsigned *		x,
	unsigned *		y,
	const char *		s
);

/** Fill the screen with a bitmap palette */
extern void
bmp_draw_palette( void );


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


/** Some selected colors */
#define COLOR_EMPTY		0x00 // total transparent
#define COLOR_BG		0x14 // transparent gray
#define COLOR_BG_DARK		0x03 // transparent black
#define COLOR_WHITE		0x01 // Normal white
#define COLOR_BLUE		0x0B // normal blue
#define COLOR_RED		0x08 // normal red
#define COLOR_YELLOW		0x0F // normal yellow
#define COLOR_BLACK 2

static inline uint32_t
color_word(
	uint8_t			color
)
{
	return 0
		| ( color << 24 )
		| ( color << 16 )
		| ( color <<  8 )
		| ( color <<  0 )
		;
}


/** BMP file format.
 * Offsets and meaning from:
 *	http://www.fastgraph.com/help/bmp_header_format.html
 */
struct bmp_file_t
{
	uint16_t		signature;	// off 0
	uint32_t		size;		// off 2, in bytes
	uint16_t		res_0;		// off 6, must be 0
	uint16_t		res_1;		// off 8. must be 0
	uint8_t *		image;		// off 10, offset in bytes
	uint32_t		hdr_size;	// off 14, must be 40
	uint32_t		width;		// off 18, in pixels
	uint32_t		height;		// off 22, in pixels
	uint16_t		planes;		// off 26, must be 1
	uint16_t		bits_per_pixel;	// off 28, 1, 4, 8 or 24
	uint32_t		compression;	// off 30, 0=none, 1=RLE8, 2=RLE4
	uint32_t		image_size;	// off 34, in bytes + padding
	uint32_t		hpix_per_meter;	// off 38, unreliable
	uint32_t		vpix_per_meter;	// off 42, unreliable
	uint32_t		num_colors;	// off 46
	uint32_t		num_imp_colors;	// off 50
} PACKED;

SIZE_CHECK_STRUCT( bmp_file_t, 54 );

extern struct bmp_file_t *
bmp_load(
	const char *		name
);

typedef struct bmp_ov_loc_size 
{
	int bmp_of_x; //live view x offset within OSD
	int bmp_of_y; //live view y offset within OSD
	int bmp_ex_x; //live view x extend
	int bmp_ex_y; //live view y extend
	int bmp_sz_x; //bitmap x size
	int bmp_sz_y; //bitmap y size
	int lv_ex_x;
	int lv_ex_y;
} bmp_ov_loc_size_t;

void calc_ov_loc_size(bmp_ov_loc_size_t *os);
void clrscr();
void bmp_draw(struct bmp_file_t * bmp, int x0, int y0, uint8_t* const mirror, int clear);
void bmp_draw_scaled(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax);
uint8_t bmp_getpixel(int x, int y);

#endif
