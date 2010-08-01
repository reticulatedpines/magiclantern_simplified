/** \file
 * Zebra stripes, contrast edge detection and crop marks.
 *
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * Edge detection code by Robert Thiel <rthiel@gmail.com>
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
#include "property.h"


static struct bmp_file_t * cropmarks;
static volatile unsigned lv_drawn = 0;
static volatile unsigned sensor_cleaning = 1;

#define vram_start_line	33
#define vram_end_line	380

#define hist_height			64
#define hist_width			128
#define waveform_height			256
#define waveform_width			(720/2)

CONFIG_INT( "zebra.draw",	zebra_draw,	1 );
CONFIG_INT( "zebra.level",	zebra_level,	0xF000 );
CONFIG_INT( "crop.draw",	crop_draw,	1 );
CONFIG_STR( "crop.file",	crop_file,	"B:/cropmarks.bmp" );
CONFIG_INT( "edge.draw",	edge_draw,	0 );
CONFIG_INT( "enable-liveview",	enable_liveview, 1 );
CONFIG_INT( "hist.draw",	hist_draw,	1 );
CONFIG_INT( "hist.x",		hist_x,		720 - hist_width - 4 );
CONFIG_INT( "hist.y",		hist_y,		100 );
CONFIG_INT( "waveform.draw",	waveform_draw,	0 );
CONFIG_INT( "waveform.x",	waveform_x,	720 - waveform_width );
CONFIG_INT( "waveform.y",	waveform_y,	480 - 50 - waveform_height );
CONFIG_INT( "waveform.bg",	waveform_bg,	0x26 ); // solid black
CONFIG_INT( "timecode.x",	timecode_x,	720 - 160 );
CONFIG_INT( "timecode.y",	timecode_y,	32 );
CONFIG_INT( "timecode.width",	timecode_width,	160 );
CONFIG_INT( "timecode.height",	timecode_height, 20 );
CONFIG_INT( "timecode.warning",	timecode_warning, 120 );
static unsigned timecode_font	= FONT(FONT_MED, COLOR_RED, COLOR_BG );


/** Sobel edge detection */
static int32_t
edge_detect(
	uint32_t *		buf,
	uint32_t		pitch
)
{	
	const uint32_t		pixel1	= buf[0];
	const int32_t		p00	= (pixel1 & 0xFFFF);
	const int32_t		p01	= pixel1 >> 16;
	const uint32_t		pixel2	= buf[1];
	const int32_t		p02	= (pixel2 & 0xFFFF);
	const uint32_t		pixel3	= buf[pitch];
	const int32_t		p10	= (pixel3 & 0xFFFF);
	const int32_t		p11	= pixel3 >> 16;
	const uint32_t		pixel4	= buf[pitch+1];
	const int32_t		p12	= (pixel4 & 0xFFFF);
	
	int32_t sx1 = p00 - p11;
	int32_t sy1 = p01 - p10;
	
	int32_t sx2 = p01 - p12;
	int32_t sy2 = p02 - p11;

	// abs value
	sx1 = ( sx1 ^ (sx1 >> 15) ) - (sx1 >> 15);
	sy1 = ( sy1 ^ (sy1 >> 15) ) - (sy1 >> 15);
	
	sx2 = ( sx2 ^ (sx2 >> 15) ) - (sx2 >> 15);
	sy2 = ( sy2 ^ (sy2 >> 15) ) - (sy2 >> 15);
	
	return (((sx2 + sy2) >> 1 ) & 0xFF00 ) | ((sx1 + sy1) >> 9);
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
	const unsigned dx = x/2;
	// Check for contrast
	uint32_t grad = edge_detect(
		&v_row[dx],
		vram_pitch
	);
	
	// Check for any high gradients in either pixel
	if( (grad & 0xF8F8) == 0 )
		return 0;

	// Color coding (using the blue colors starting at 0x70)
	b_row[dx] = 0x7070 | ((grad & 0xF8F8) >> 3) ;			
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
	uint32_t p0 = (pixel >> 16) & 0xFFFF;
	uint32_t p1 = (pixel >>  0) & 0xFFFF;

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


/** Store the waveform data for each of the waveform_width bins with
 * 128 levels
 */
static uint32_t waveform[ waveform_width ][ waveform_height ];

/** Store the histogram data for each of the 128 bins */
static uint32_t hist[ hist_width ];

/** Maximum value in the histogram so that at least one entry fills
 * the box */
static uint32_t hist_max;


/** Generate the histogram data from the YUV frame buffer.
 *
 * Walk the frame buffer two pixels at a time, in 32-bit chunks,
 * to avoid err70 while recording.
 *
 * Average two adjacent pixels to try to reduce noise slightly.
 *
 * Update the hist_max for the largest number of bin entries found
 * to scale the histogram to fit the display box from top to
 * bottom.
 */
static void
hist_build( void )
{
	struct vram_info *	vram = &vram_info[ vram_get_number(2) ];
	const uint32_t * 	v_row = (uint32_t*) vram->vram;
	const unsigned		width = vram->width;
	uint32_t x,y;

	hist_max = 0;

#if 0
	memset( hist, 0, sizeof(hist) );
	memset( waveform, 0, sizeof(waveform) );
#else
	// memset() causes err70?  Too much memory bandwidth?
	for( x=0 ; x<hist_width ; x++ )
		hist[x] = 0;
	for( y=0 ; y<waveform_width ; y++ )
		for( x=0 ; x<waveform_height ; x++ )
		{
			waveform[y][x] = 0;
			asm( "nop\nnop\nnop\nnop\n" );
		}
#endif

	for( y=vram_start_line ; y<vram_end_line; y++, v_row += (vram->pitch/2) )
	{
		for( x=0 ; x<width ; x += 2 )
		{
			// Average each of the two pixels
			uint32_t pixel = v_row[x/2];
			uint32_t p1 = (pixel >> 16) & 0xFFFF;
			uint32_t p2 = (pixel >>  0) & 0xFFFF;
			uint32_t p = (p1+p2) / 2;

			uint32_t hist_level = ( p * hist_width ) / 65536;

			// Ignore the 0 bin.  It generates too much noise
			unsigned count = ++hist[ hist_level ];
			if( hist_level && count > hist_max )
				hist_max = count;

			// Update the waveform plot
			waveform[ (x * waveform_width) / width ][ (p * waveform_height) / 65536 ]++;
		}
	}
}
	

/** Draw the histogram image into the bitmap framebuffer.
 *
 * Draw one pixel at a time; it seems to be ok with err70.
 * Since there is plenty of math per pixel this doesn't
 * swamp the bitmap framebuffer hardware.
 */
static void
hist_draw_image(
	unsigned		x_origin,
	unsigned		y_origin
)
{
	uint8_t * const bvram = bmp_vram();

	// Align the x origin, just in case
	x_origin &= ~3;

	uint8_t * row = bvram + x_origin + y_origin * bmp_pitch();
	if( hist_max == 0 )
		hist_max = 1;

	unsigned i, y;

	for( i=0 ; i<hist_width ; i++ )
	{
		// Scale by the maximum bin value
		const uint32_t size = (hist[i] * hist_height) / hist_max;
		uint8_t * col = row + i;

		// vertical line up to the hist size
		for( y=hist_height ; y>0 ; y-- , col += bmp_pitch() )
			*col = y > size ? COLOR_BG : COLOR_WHITE;
	}

	// Draw some extra just to add a black bar on the right side
	bmp_fill(
		COLOR_BG,
		x_origin + hist_width,
		y_origin,
		4,
		hist_height
	);

	if(0) bmp_printf(
		FONT(FONT_SMALL,COLOR_RED,COLOR_WHITE),
		x_origin,
		y_origin,
		"max %d",
		(int) hist_max
	);

	hist_max = 0;
}


/** Draw the waveform image into the bitmap framebuffer.
 *
 * Draw one pixel at a time; it seems to be ok with err70.
 * Since there is plenty of math per pixel this doesn't
 * swamp the bitmap framebuffer hardware.
 */
static void
waveform_draw_image(
	unsigned		x_origin,
	unsigned		y_origin
)
{
	// Ensure that x_origin is quad-word aligned
	x_origin &= ~3;

	uint8_t * const bvram = bmp_vram();
	unsigned pitch = bmp_pitch();
	uint8_t * row = bvram + x_origin + y_origin * pitch;
	if( hist_max == 0 )
		hist_max = 1;

	unsigned i, y;

	// vertical line up to the hist size
	for( y=waveform_height-1 ; y>0 ; y-- )
	{
		uint32_t pixel = 0;

		for( i=0 ; i<waveform_width ; i++ )
		{

			uint32_t count = waveform[ i ][ y ];
			// Scale to a grayscale
			count = (count * 42) / 128;
			if( count > 42 )
				count = 0x0F;
			else
			if( count >  0 )
				count += 0x26;
			else
			// Draw a series of colored scales
			if( y == (waveform_height*1)/4 )
				count = COLOR_BLUE;
			else
			if( y == (waveform_height*2)/4 )
				count = 0xE; // pink
			else
			if( y == (waveform_height*3)/4 )
				count = COLOR_BLUE;
			else
				count = waveform_bg; // transparent

			pixel <<= 8;
			pixel |= count;

			if( (i & 3) != 3 )
				continue;

			// Draw the pixel, rounding down to the nearest
			// quad word write (and then nop to avoid err70).
			*(uint32_t*)( row + (i & ~3)  ) = pixel;
			pixel = 0;
			asm( "nop" );
			asm( "nop" );
			asm( "nop" );
			asm( "nop" );
		}

		row += pitch;
	}
}


/** Master video overlay drawing code.
 *
 * This routine controls the display of the zebras, histogram,
 * edge detection, cropmarks and so on.
 *
 * The stacking order of the overlays is:
 *
 * - Histogram
 * - Cropping bitmap
 * - Zebras
 * - Edge detection
 *
 * This should be done with a proper OO controller that allows modules
 * to register new drawing functions, but for right now they are hardcoded.
 */
static void
draw_zebra( void )
{
	uint8_t * const bvram = bmp_vram();

	// If we don't have a bitmap vram yet, nothing to do.
	if( !bvram )
		return;

	static struct vram_info _vram_info;
	vram_get_pos_and_size(
		&_vram_info.width,
		&_vram_info.height,
		&_vram_info.pitch,
		&_vram_info.vram
	);

	bmp_printf(FONT_SMALL, 300, 300, "bmp=%08x vram=%08x %d %d %d",
		bvram,
		_vram_info.vram,
		_vram_info.width,
		_vram_info.pitch,
		_vram_info.height
	);
	return;

	// If we are not drawing edges, or zebras or crops, nothing to do
	if( !edge_draw && !zebra_draw && !hist_draw && !waveform_draw )
	{
		if( !crop_draw )
			return;
		if( !cropmarks )
			return;
	}

	//struct vram_info * vram = &vram_info[ vram_get_number(2) ];
	struct vram_info * vram = &_vram_info;

	//hist_build();

	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	// 33 is the bottom of the meters; 55 is the crop mark
	uint32_t x,y;
	for( y=33 ; y < 390; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bvram + y * bmp_pitch() );

		// Iterate over the pixels in the scan row
		// two at a time to read the pixel buf in 32 bit chunks
		// otherwise we get err70 aborts while drawing regions
		// in the bitmap vram.
		for( x=2 ; x < vram->width-2 ; x+=2 )
		{
			// Abort as soon as the new menu is drawn
			if( gui_menu_task || !lv_drawn )
				return;

			// Ignore the regions where the histogram will be drawn
			if( hist_draw
			&&  y >= hist_y
			&&  y <  hist_y + hist_height
			&&  x >= hist_x
			&&  x <  hist_x + hist_width + 4
			)
				continue;

			// Ignore the regions where the waveform will be drawn
			if( waveform_draw
			&&  y >= waveform_y
			&&  y <  waveform_y + waveform_height
			&&  x >= waveform_x
			&&  x <  waveform_x + waveform_width
			)
				continue;

			// Ignore the timecode region
			if( y >= timecode_y
			&&  y <  timecode_y + timecode_height
			&&  x >= timecode_x
			&&  x <  timecode_x + timecode_width
			)
				continue;

			if( crop_draw && check_crop( x, y, b_row, v_row, vram->pitch ) )
				continue;

			if( edge_draw && check_edge( x, y, b_row, v_row, vram->pitch ) )
				continue;

			if( zebra_draw && check_zebra( x, y, b_row, v_row, vram->pitch ) )
				continue;

			// Nobody drew on it, make it clear
			b_row[x/2] = 0;
		}
	}

	if( hist_draw )
		hist_draw_image( hist_x, hist_y );
	if( waveform_draw )
		waveform_draw_image( waveform_x, waveform_y );
}


static void
zebra_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x4000) & 0xF000;
}


static void
zebra_display( void * priv, int x, int y, int selected )
{
	bmp_printf( 
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Zebra thrs: 0x%04x",
		*(unsigned*) priv
	);
}

static void
zebra_draw_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Zebras:     %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}

static void
crop_display( void * priv, int x, int y, int selected )
{
	extern int retry_count;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Cropmarks:  %s %d",
		cropmarks ? (*(unsigned*) priv ? "ON " : "OFF") : "NO FILE",
		retry_count
	);
}


static void
edge_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Edgedetect: %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}


static void
hist_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Histogram:  %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}

static void
waveform_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Waveform:   %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}


struct menu_entry zebra_menus[] = {
	{
		.priv		= &zebra_draw,
		.select		= menu_binary_toggle,
		.display	= zebra_draw_display,
	},
	{
		.priv		= &zebra_level,
		.select		= zebra_toggle,
		.display	= zebra_display,
	},
	{
		.priv		= &crop_draw,
		.select		= menu_binary_toggle,
		.display	= crop_display,
	},
	{
		.priv		= &edge_draw,
		.select		= menu_binary_toggle,
		.display	= edge_display,
	},
	{
		.priv		= &hist_draw,
		.select		= menu_binary_toggle,
		.display	= hist_display,
	},
	{
		.priv		= &waveform_draw,
		.select		= menu_binary_toggle,
		.display	= waveform_display,
	},
};


PROP_HANDLER( PROP_LV_ACTION )
{
	// LV_START==0, LV_STOP=1
	lv_drawn = !buf[0];
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_GUI_STATE )
{
	// PLAYMENU==0, IDLE==1
	lv_drawn = !buf[0];
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_ACTIVE_SWEEP_STATUS )
{
	// Let us know when the sensor is done cleaning
	sensor_cleaning = buf[0];
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_MVR_REC_START )
{
	if( buf[0] == 2 )
		bmp_printf(
			timecode_font,
			timecode_x,
			timecode_y,
			"REC: "
		);
	return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_REC_TIME )
{
	unsigned value = buf[0];
	value /= 200; // why? it seems to work out
	bmp_printf(
		value < timecode_warning ? timecode_font : FONT_MED,
		timecode_x + 5 * fontspec_font(timecode_font)->width,
		timecode_y,
		"%4d:%02d",
		value / 60,
		value % 60
	);
	return prop_cleanup( token, property );
}


static void
zebra_task( void )
{
	lv_drawn = 0;
	cropmarks = bmp_load( crop_file );

	DebugMsg( DM_MAGIC, 3,
		"%s: Zebras=%s threshold=%x cropmarks=%x liveview=%d",
		__func__,
		zebra_draw ? "ON " : "OFF",
		zebra_level,
		(unsigned) cropmarks,
		enable_liveview
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
		draw_zebra();
		msleep(1000);
	}


#ifndef CONFIG_550D
#error "liveview!"
	if( enable_liveview )
	{
/*
		DebugMsg( DM_MAGIC, 3, "Waiting for sweep status to end" );
		while( sensor_cleaning )
			msleep(100);
*/
		DebugMsg( DM_MAGIC, 3, "Entering liveview" );
		call( "FA_StartLiveView" );

		// Clear the bitmap display, just in case
		bmp_fill( 0, 0, 0, 720, 480 );
	}
#endif

	//menu_add( "Video", zebra_menus, COUNT(zebra_menus) );

	while(1)
	{
		if( !gui_menu_task && lv_drawn )
		{
			draw_zebra();
			msleep( 100 );
		} else {
			// Don't display the zebras over the menu.
			// wait a while and then try again
			msleep( 500 );
		}
	}
}


TASK_CREATE( "zebra_task", zebra_task, 0, 0x18, 0x1000 );
