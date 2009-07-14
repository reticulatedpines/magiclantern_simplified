/** \file
 * Zebra stripes, contrast edge detection and crop marks.
 *
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
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"


unsigned zebra_level = 0xF000;
unsigned zebra_draw = 1;
unsigned crop_draw = 1;
unsigned edge_draw = 0;
struct bmp_file_t * cropmarks;


/** Draw white thin crop marks
 *  And draw the 16:9 crop marks for full time
 * The screen is 480 vertical lines, but we only want to
 * show 720 * 9 / 16 == 405 of them.  If we use this number,
 * however, things don't line up right.
 *
 * \return 1 if a crop mark was drawn
 * \todo Determine the actual Cinemascope aspect ratio
 */
static int
draw_matte(
	unsigned		y,
	void *			bmp_row
)
{
	if( !crop_draw )
		return 0;

	if( y != 55 && y != 375 )
		return 0;

	const uint32_t		width	= 720;
	uint32_t *		row	= bmp_row;
	unsigned		x;

	for( x = 0; x<width/4 ; x++ )
		*(row++) = color_word( COLOR_WHITE );

	return 1;
}

static int32_t convolve_x[3][3] = {
	{ -1, 0, +1, },
	{ -2, 0, +2, },
	{ -1, 0, +1, },
};

static int32_t convolve_y[3][3] = {
	{ +1, +2, +1, },
	{  0,  0,  0, },
	{ -1, -2, -1, },
};


static int32_t
convolve(
	int32_t c[3][3],
	uint16_t * buf,
	uint32_t pitch
)
{
	int32_t value = 0;
	value += c[0][0] * buf[ -pitch - 1 ];
	value += c[0][1] * buf[ -pitch ];
	value += c[0][2] * buf[ -pitch + 1 ];

	value += c[1][0] * buf[ -1 ];
	value += c[1][1] * buf[ 0 ];
	value += c[1][2] * buf[ +1 ];

	value += c[2][0] * buf[ +pitch - 1 ];
	value += c[2][1] * buf[ +pitch ];
	value += c[2][2] * buf[ +pitch + 1 ];

	return value;
}


int32_t
edge_detect(
	uint16_t *		buf,
	uint32_t		pitch
)
{
	int32_t gx = convolve( convolve_x, buf, pitch );
	int32_t gy = convolve( convolve_y, buf, pitch );

	return gx*gx + gy*gy;
}


static unsigned
check_edge(
	unsigned		x,
	unsigned		y,
	uint16_t *		b_row,
	uint32_t *		v_row,
	unsigned		vram_pitch
)
{
	const uint8_t contrast_color = 0x0E; // pink
	const int32_t edge_level = zebra_level * zebra_level;

	// Check for contrast
	int32_t grad = edge_detect(
		(uint16_t*) &v_row[x/2],
		vram_pitch
	);

	if( grad > edge_level )
		return 0;

	b_row[x/2] = 0
		| (contrast_color << 8)
		| contrast_color;

	return 1;
}


static unsigned
check_zebra(
	unsigned		x,
	unsigned		y,
	uint16_t *		b_row,
	uint32_t *		v_row,
	unsigned		vram_pitch
)
{
	const uint8_t zebra_color_0 = COLOR_BG; // 0x6F; // bright read
	const uint8_t zebra_color_1 = 0x5F; // dark red

	uint32_t pixel = v_row[x/2];
	int32_t p0 = (pixel >> 16) & 0xFFFF;
	int32_t p1 = (pixel >>  0) & 0xFFFF;

	// If neither pixel is overexposed, ignore it
	if( p0 < zebra_level && p1 < zebra_level )
		return 0;

	// Determine if we are a zig or a zag line
	uint32_t zag = ((y >> 3) ^ (x >> 3)) & 1;

	// Build the 16-bit word to write both pixels
	// simultaneously into the BMP VRAM
	uint16_t zebra_color_word = zag
		? (zebra_color_0<<8) | (zebra_color_0<<0)
		: (zebra_color_1<<8) | (zebra_color_1<<0);

	b_row[x/2] = zebra_color_word;
	return 1;
}


static unsigned
check_crop(
	unsigned		x,
	unsigned		y,
	uint16_t *		b_row,
	uint32_t *		v_row,
	unsigned		vram_pitch
)
{
	if( !cropmarks )
		return 0;

	uint8_t * pixbuf = &cropmarks->image[
		x + cropmarks->width * (cropmarks->height - y)
	];
	uint16_t pix = *(uint16_t*) pixbuf;
	if( pix == 0 )
		return 0;

	b_row[ x/2 ] = pix;
	return 1;
}


static void
draw_zebra( void )
{
	uint8_t * const bvram = bmp_vram();

	// If we don't have a bitmap vram yet, nothing to do.
	if( !bvram )
		return;

	// If we are not drawing edges, or zebras or crops, nothing to do
	if( !edge_draw && !zebra_draw )
	{
		if( !crop_draw )
			return;
		if( !cropmarks )
			return;
	}

	struct vram_info * vram = &vram_info[ vram_get_number(2) ];

	uint32_t x,y;


	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	// 33 is the bottom of the meters; 55 is the crop mark
	for( y=33 ; y < 390; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bvram + y * bmp_pitch() );

		for( x=2 ; x < vram->width-2 ; x+=2 )
		{
			unsigned pixels = 0;
			if( cropmarks && check_crop( x, y, b_row, v_row, vram->pitch ) )
				continue;

			if( edge_draw && check_edge( x, y, b_row, v_row, vram->pitch ) )
				continue;

			if( zebra_draw && check_zebra( x, y, b_row, v_row, vram->pitch ) )
				continue;

			// Nobody drew on it, make it clear
			b_row[x/2] = 0;
		}
	}
}


void zebra_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x4000) & 0xF000;
}

void zebra_display( void * priv, int x, int y, int selected )
{
	bmp_printf( 
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zebra level: %04x",
		*(unsigned*) priv
	);
}

void zebra_draw_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = !*ptr;
}

void zebra_draw_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zebras %s",
		*(unsigned*) priv ? " on" : "off"
	);
}

void crop_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = !*ptr;
}

void crop_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Crop %s",
		*(unsigned*) priv ? " on" : "off"
	);
}

struct menu_entry zebra_menus[] = {
	{
		.priv		= &zebra_draw,
		.select		= zebra_draw_toggle,
		.display	= zebra_draw_display,
	},
	{
		.priv		= &zebra_level,
		.select		= zebra_toggle,
		.display	= zebra_display,
	},
	{
		.priv		= &crop_draw,
		.select		= crop_toggle,
		.display	= crop_display,
	},
};

int
zebra_task( void )
{
	msleep( 3000 );

	zebra_draw = config_int( global_config, "zebra.draw", 1 );
	zebra_level = config_int( global_config, "zebra.level", 0xF000 );
	crop_draw = config_int( global_config, "crop.draw", 1 );
	edge_draw = config_int( global_config, "edge.draw", 1 );

	cropmarks = bmp_load( "A:/cropmarks.bmp" );

	menu_add( "Video", zebra_menus, COUNT(zebra_menus) );

	DebugMsg( DM_MAGIC, 3, "Zebras %s, threshold %x cropmarks %x",
		zebra_draw ? "on" : "off",
		zebra_level,
		(unsigned) cropmarks
	);

	if( cropmarks )
	{
		DebugMsg( DM_MAGIC, 3,
			"Cropmarks: %dx%d @ %d",
			cropmarks->width,
			cropmarks->height,
			cropmarks->bits_per_pixel
		);
	}

	while(1)
	{
		if( !gui_show_menu )
		{
			draw_zebra();
			msleep( 100 );
		} else {
			// Don't display the zebras over the menu.
			// wait a while and then try again
			msleep( 500 );
		}
	}

	return 1;
}


TASK_CREATE( "zebra_task", zebra_task, 0, 0x1f, 0x1000 );
