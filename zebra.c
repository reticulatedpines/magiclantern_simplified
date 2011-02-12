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
#include "gui.h"
#include "lens.h"


static struct bmp_file_t * cropmarks_array[3] = {0};
static struct bmp_file_t * cropmarks = 0;

#define hist_height			64
#define hist_width			128
#define WAVEFORM_MAX_HEIGHT			240
#define WAVEFORM_MAX_WIDTH			360
#define WAVEFORM_HALFSIZE (waveform_draw == 1)
#define WAVEFORM_WIDTH (WAVEFORM_HALFSIZE ? WAVEFORM_MAX_WIDTH/2 : WAVEFORM_MAX_WIDTH)
#define WAVEFORM_HEIGHT (WAVEFORM_HALFSIZE ? WAVEFORM_MAX_HEIGHT/2 : WAVEFORM_MAX_HEIGHT)

CONFIG_INT( "global.draw", global_draw, 1 );
CONFIG_INT( "zebra.draw",	zebra_draw,	2 );
CONFIG_INT( "zebra.level-hi",	zebra_level_hi,	245 );
CONFIG_INT( "zebra.level-lo",	zebra_level_lo,	10 );
CONFIG_INT( "zebra.delay",	zebra_delay,	1000 );
CONFIG_INT( "crop.draw",	crop_draw,	1 ); // index of crop file
CONFIG_INT( "crop.playback", cropmark_playback, 0);
CONFIG_INT( "falsecolor.draw", falsecolor_draw, 2);
//~ CONFIG_INT( "falsecolor.shortcutkey", falsecolor_shortcutkey, 1);
#define falsecolor_shortcutkey (falsecolor_draw == 2)
int falsecolor_displayed = 0;

CONFIG_INT( "focus.peaking", focus_peaking, 0);
CONFIG_INT( "focus.peaking.thr", focus_peaking_pthr, 10); // 1%
CONFIG_INT( "focus.peaking.color", focus_peaking_color, 7); // R,G,B,C,M,Y,cc1,cc2

CONFIG_INT( "focus.graph", focus_graph, 1);
//~ int get_crop_black_border() { return crop_black_border; }

//~ CONFIG_INT( "edge.draw",	edge_draw,	0 );
CONFIG_INT( "hist.draw",	hist_draw,	1 );
CONFIG_INT( "hist.x",		hist_x,		720 - hist_width - 4 );
CONFIG_INT( "hist.y",		hist_y,		100 );
CONFIG_INT( "waveform.draw",	waveform_draw,	0 );
//~ CONFIG_INT( "waveform.x",	waveform_x,	720 - WAVEFORM_WIDTH );
//~ CONFIG_INT( "waveform.y",	waveform_y,	480 - 50 - WAVEFORM_WIDTH );
CONFIG_INT( "waveform.bg",	waveform_bg,	0x26 ); // solid black
CONFIG_INT( "timecode.x",	timecode_x,	720 - 160 );
CONFIG_INT( "timecode.y",	timecode_y,	0 );
CONFIG_INT( "timecode.width",	timecode_width,	160 );
CONFIG_INT( "timecode.height",	timecode_height, 20 );
CONFIG_INT( "timecode.warning",	timecode_warning, 120 );
static unsigned timecode_font	= FONT(FONT_MED, COLOR_RED, COLOR_BG );

CONFIG_INT( "clear.preview", clearpreview, 1); // 2 is always
CONFIG_INT( "clear.preview.delay", clearpreview_delay, 1000); // ms

CONFIG_INT( "spotmeter.size",		spotmeter_size,	5 );
CONFIG_INT( "spotmeter.draw",		spotmeter_draw, 1 ); // 0 off, 1: on (center), 2: under center marker
CONFIG_INT( "spotmeter.formula",		spotmeter_formula, 0 ); // 0 percent, 1 IRE AJ, 2 IRE Piers

CONFIG_INT( "unified.loop", unified_loop, 2); // temporary; on/off/auto
CONFIG_INT( "zebra.density", zebra_density, 0); 
CONFIG_INT( "hd.vram", use_hd_vram, 0); 

CONFIG_INT( "time.indicator", time_indicator, 3); // 0 = off, 1 = current clip length, 2 = time remaining until filling the card, 3 = time remaining until 4GB
CONFIG_INT( "time.ticks.4gb", ticks_4gb, 540); // how many ticks of PROP_REC_TIME are sent in a 4 GB movie

static void
unified_loop_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"UnifLoop (experim): %s",
		unified_loop == 0 ? "OFF" : unified_loop == 1 ? "ON" : "Auto"
	);
}

static void
zebra_mode_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zebra Density: %d", zebra_density);
}

static void
use_hd_vram_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Use HD VRAM: %s", use_hd_vram?"yes":"no");
}

PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
PROP_INT(PROP_DOF_PREVIEW_MAYBE, dofpreview);
int recording = 0;

uint8_t false_colour[256] = {
0x0E, 0x0E, 0x0E, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6F 
};


int crop_dirty = 0;
int ext_monitor_rca = 0;
int ext_monitor_hdmi = 0;
int lv_dispsize = 1;
PROP_HANDLER(PROP_USBRCA_MONITOR)
{
	ext_monitor_rca = buf[0];
	crop_dirty = 10;
	return prop_cleanup( token, property );
}
PROP_HANDLER(PROP_HDMI_CHANGE)
{
	ext_monitor_hdmi = buf[0];
	crop_dirty = 10;
	return prop_cleanup( token, property );
}
PROP_HANDLER(PROP_LV_DISPSIZE)
{
	lv_dispsize = buf[0];
	crop_dirty = 10;
	return prop_cleanup( token, property );
}

int video_mode_crop = 0;
int video_mode_fps = 0;
int video_mode_resolution = 0; // 0 if full hd, 1 if 720p, 2 if 480p
PROP_HANDLER(PROP_VIDEO_MODE)
{
	video_mode_crop = buf[0];
	video_mode_fps = buf[2];
	video_mode_resolution = buf[1];
	return prop_cleanup( token, property );
}

int gui_state;
PROP_HANDLER(PROP_GUI_STATE) {
	gui_state = buf[0];
	if (gui_state == GUISTATE_IDLE) crop_dirty = 10;
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_LV_AFFRAME ) {
	crop_dirty = 10; // redraw cropmarks after 10 cycles
	return prop_cleanup( token, property );
}


// how to use a config setting in more than one file?!
//extern int* p_cfg_draw_meters;

int get_global_draw()
{
	return global_draw;
}
void set_global_draw(int g)
{
	global_draw = g;
}

struct vram_info * get_yuv422_hd_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = YUV422_HD_BUFFER;
	_vram_info.width = recording ? (video_mode_resolution == 0 ? 1720 : 
									video_mode_resolution == 1 ? 1280 : 
									video_mode_resolution == 2 ? 640 : 0)
								  : lv_dispsize > 1 ? 1024
								  : shooting_mode != SHOOTMODE_MOVIE ? 1056
								  : (video_mode_resolution == 0 ? 1056 : 
								  	video_mode_resolution == 1 ? 1024 :
									 video_mode_resolution == 2 ? (video_mode_crop? 640:1024) : 0);
	_vram_info.pitch = _vram_info.width << 1; 
	_vram_info.height = recording ? (video_mode_resolution == 0 ? 974 : 
									video_mode_resolution == 1 ? 580 : 
									video_mode_resolution == 2 ? 480 : 0)
								  : lv_dispsize > 1 ? 680
								  : shooting_mode != SHOOTMODE_MOVIE ? 704
								  : (video_mode_resolution == 0 ? 704 : 
								  	video_mode_resolution == 1 ? 680 :
									 video_mode_resolution == 2 ? (video_mode_crop? 480:680) : 0);

	return &_vram_info;
}

void* get_fastrefresh_422_buf()
{
	switch (*(uint32_t*)0x246c)
	{
		case 0x40d07800:
			return 0x4c233800;
		case 0x4c233800:
			return 0x4f11d800;
		case 0x4f11d800:
			return 0x40d07800;
	}
	return 0;
}

struct vram_info * get_yuv422_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = get_fastrefresh_422_buf();

	if(ext_monitor_hdmi && !recording) {
		_vram_info.pitch=YUV422_LV_PITCH_HDMI;
		_vram_info.height=YUV422_LV_HEIGHT_HDMI;
	}else if(ext_monitor_rca) {
		_vram_info.pitch=YUV422_LV_PITCH_RCA;
		_vram_info.height=YUV422_LV_HEIGHT_RCA;
	} else {
		_vram_info.pitch=YUV422_LV_PITCH;
		_vram_info.height=YUV422_LV_HEIGHT;
	}

	_vram_info.width = _vram_info.pitch >> 1;

	return &_vram_info;
}

/** Sobel edge detection */
/*
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
	unsigned		vram_pitch, // unused?
	uint16_t * 		m_row
)
{
	// Determine if we are a zig or a zag line
	if (((y >> 3) ^ (x >> 3)) & 1) return 0;

	uint32_t pixel = v_row[x/2];
	uint32_t p0 = ((pixel >> 16) & 0xFF00) >> 8; // odd bytes are luma
	uint32_t p1 = ((pixel >>  0) & 0xFF00) >> 8;

	// If neither pixel is overexposed or underexposed, ignore it
	if( p0 <= zebra_level_hi && p1 <= zebra_level_hi && p0 >= zebra_level_lo && p1 >= zebra_level_lo)
		return 0;

 	uint8_t zebra_color_1 = 12; // red
   if (p0 < zebra_level_lo || p1 < zebra_level_lo)
        zebra_color_1 = 13; // blue 

	b_row[x/2] = (zebra_color_1<<8) | (zebra_color_1<<0);
	m_row[x/2] = (zebra_color_1<<8) | (zebra_color_1<<0);
	return 1;
}


static unsigned
check_crop(
	unsigned		x,
	unsigned		y,
	uint16_t *		b_row,
	uint32_t *		v_row,
	unsigned		vram_pitch,
	uint16_t * m_row
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
	m_row[ x/2 ] = pix;
	return 1;
}
*/

/** Store the waveform data for each of the WAVEFORM_WIDTH bins with
 * 128 levels
 */
static uint32_t** waveform = 0;

/** Store the histogram data for each of the "hist_width" bins */
static uint32_t* hist = 0;
static uint32_t* hist_r = 0;
static uint32_t* hist_g = 0;
static uint32_t* hist_b = 0;

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
void
hist_build(void* vram, int width, int pitch)
{
	uint32_t * 	v_row = (uint32_t*) vram;
	int x,y;

	histo_init();
	if (!hist) return;
	if (!hist_r) return;
	if (!hist_g) return;
	if (!hist_b) return;

	hist_max = 0;
	for( x=0 ; x<hist_width ; x++ )
	{
		hist[x] = 0;
		hist_r[x] = 0;
		hist_g[x] = 0;
		hist_b[x] = 0;
	}

	if (waveform_draw)
	{
		waveform_init();
		for( y=0 ; y<WAVEFORM_WIDTH ; y++ )
			for( x=0 ; x<WAVEFORM_HEIGHT ; x++ )
				waveform[y][x] = 0;
	}

	for( y=1 ; y<480; y++, v_row += (pitch/4) )
	{
		for( x=0 ; x<width ; x += 2 )
		{
			// Average each of the two pixels
			uint32_t pixel = v_row[x/2];
			uint32_t p1 = (pixel >> 16) & 0xFF00;
			uint32_t p2 = (pixel >>  0) & 0xFF00;
			int Y = ((p1+p2) / 2) >> 8;

			if (hist_draw == 2)
			{
				int8_t U = (pixel >>  0) & 0xFF;
				int8_t V = (pixel >> 16) & 0xFF;
				int R = Y + 1437 * V / 1024;
				int G = Y -  352 * U / 1024 - 731 * V / 1024;
				int B = Y + 1812 * U / 1024;
				uint32_t R_level = COERCE(( R * hist_width ) / 256, 0, hist_width-1);
				uint32_t G_level = COERCE(( G * hist_width ) / 256, 0, hist_width-1);
				uint32_t B_level = COERCE(( B * hist_width ) / 256, 0, hist_width-1);
				hist_r[R_level]++;
				hist_g[G_level]++;
				hist_b[B_level]++;
			}

			uint32_t hist_level = COERCE(( Y * hist_width ) / 0xFF, 0, hist_width-1);

			// Ignore the 0 bin.  It generates too much noise
			unsigned count = ++ (hist[ hist_level ]);
			if( hist_level && count > hist_max )
				hist_max = count;
			
			// Update the waveform plot
			if (waveform_draw) waveform[ COERCE((x * WAVEFORM_WIDTH) / width, 0, WAVEFORM_WIDTH-1)][ COERCE((Y * WAVEFORM_HEIGHT) / 0xFF, 0, WAVEFORM_HEIGHT-1) ]++;
		}
	}
}

void get_under_and_over_exposure(uint32_t thr_lo, uint32_t thr_hi, int* under, int* over)
{
	*under = -1;
	*over = -1;
	struct vram_info * vramstruct = get_yuv422_vram();
	if (!vramstruct) return;

	*under = 0;
	*over = 0;
	void* vram = vramstruct->vram;
	int width = vramstruct->width;
	int pitch = vramstruct->pitch;
	uint32_t * 	v_row = (uint32_t*) vram;
	int x,y;
	for( y=1 ; y<480; y++, v_row += (pitch/4) )
	{
		for( x=0 ; x<width ; x += 2 )
		{
			uint32_t pixel = v_row[x/2];
			uint32_t p1 = (pixel >> 16) & 0xFFFF;
			uint32_t p2 = (pixel >>  0) & 0xFFFF;
			uint32_t p = ((p1+p2) / 2) >> 8;
			if (p < thr_lo) (*under)++;
			if (p > thr_hi) (*over)++;
		}
	}
}

static int hist_rgb_color(int y, int sizeR, int sizeG, int sizeB)
{
	switch ((y > sizeR ? 0 : 1) |
			(y > sizeG ? 0 : 2) |
			(y > sizeB ? 0 : 4))
	{
		case 0b000: return COLOR_BLACK;
		case 0b001: return COLOR_RED;
		case 0b010: return 7; // green
		case 0b100: return 9; // strident blue
		case 0b011: return COLOR_YELLOW;
		case 0b110: return 5; // cyan
		case 0b101: return 14; // magenta
		case 0b111: return COLOR_WHITE;
	}
	return 0;
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
	if (!lv_drawn()) return;
	uint8_t * const bvram = bmp_vram();

	// Align the x origin, just in case
	x_origin &= ~3;

	uint8_t * row = bvram + x_origin + y_origin * BMPPITCH;
	if( hist_max == 0 )
		hist_max = 1;

	unsigned i, y;

	for( i=0 ; i < hist_width ; i++ )
	{
		// Scale by the maximum bin value
		const uint32_t size = (hist[i] * hist_height) / hist_max;
		const uint32_t sizeR = (hist_r[i] * hist_height) / hist_max;
		const uint32_t sizeG = (hist_g[i] * hist_height) / hist_max;
		const uint32_t sizeB = (hist_b[i] * hist_height) / hist_max;

		uint8_t * col = row + i;
		// vertical line up to the hist size
		for( y=hist_height ; y>0 ; y-- , col += BMPPITCH )
		{
			if (hist_draw == 2) // RGB
				*col = hist_rgb_color(y, sizeR, sizeG, sizeB);
			else
				*col = y > size ? COLOR_BG : (falsecolor_displayed ? false_colour[(i * 256 / hist_width) & 0xFF]: COLOR_WHITE);
		}
	}

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
    if (!lv_drawn()) return;
	waveform_init();
	// Ensure that x_origin is quad-word aligned
	x_origin &= ~3;

	uint8_t * const bvram = bmp_vram();
	unsigned pitch = BMPPITCH;
	uint8_t * row = bvram + x_origin + y_origin * pitch;
	if( hist_max == 0 )
		hist_max = 1;

	unsigned i, y;

	// vertical line up to the hist size
	for( y=WAVEFORM_HEIGHT-1 ; y>0 ; y-- )
	{
		uint32_t pixel = 0;

		for( i=0 ; i<WAVEFORM_WIDTH ; i++ )
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
			if( y == (WAVEFORM_HEIGHT*1)/4 )
				count = COLOR_BLUE;
			else
			if( y == (WAVEFORM_HEIGHT*2)/4 )
				count = 0xE; // pink
			else
			if( y == (WAVEFORM_HEIGHT*3)/4 )
				count = COLOR_BLUE;
			else
				count = waveform_bg; // transparent

			pixel |= (count << ((i & 3)<<3));

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

static FILE * g_aj_logfile = INVALID_PTR;
unsigned int aj_create_log_file( char * name)
{
   FIO_RemoveFile( name );
   g_aj_logfile = FIO_CreateFile( name );
   if ( g_aj_logfile == INVALID_PTR )
   {
      bmp_printf( FONT_SMALL, 120, 40, "FCreate: Err %s", name );
      return( 0 );  // FAILURE
   }
   return( 1 );  // SUCCESS
}

void aj_close_log_file( void )
{
   if (g_aj_logfile == INVALID_PTR)
      return;
   FIO_CloseFile( g_aj_logfile );
   g_aj_logfile = INVALID_PTR;
}

void dump_seg(uint32_t start, uint32_t size, char* filename)
{
    DEBUG();
    aj_create_log_file(filename);
    FIO_WriteFile( g_aj_logfile, (const void *) start, size );
    aj_close_log_file();
    DEBUG();
}

void dump_big_seg(int k, char* filename)
{
    DEBUG();
    aj_create_log_file(filename);
    
    int i;
    for (i = 0; i < 16; i++)
    {
		DEBUG();
		uint32_t start = (k << 28 | i << 24);
		bmp_printf(FONT_LARGE, 50, 50, "DUMP %x %8x ", i, start);
		FIO_WriteFile( g_aj_logfile, (const void *) start, 0x1000000 );
	}
    
    aj_close_log_file();
    DEBUG();
}

static void dump_vram()
{
	//dump_big_seg(1, "B:/1.bin");
	//dump_big_seg(4, "B:/4.bin");
	//~ dump_seg(0x44000080, 1920*1080*2, "B:/hd.bin");
	//~ dump_seg(YUV422_IMAGE_BUFFER, 1920*1080*2, "B:/VRAM.BIN");
}

static uint8_t* bvram_mirror = 0;

void spotmeter_step();

int fps_ticks = 0;

void fail(char* msg)
{
	bmp_printf(FONT_LARGE, 30, 100, msg);
	while(1) msleep(1);
}
void waveform_init()
{
	if (!waveform)
	{
		waveform = AllocateMemory(WAVEFORM_MAX_WIDTH * sizeof(uint32_t*));
		if (!waveform) fail("Waveform malloc failed");
		int i;
		for (i = 0; i < WAVEFORM_MAX_WIDTH; i++) {
			waveform[i] = AllocateMemory(WAVEFORM_MAX_HEIGHT * sizeof(uint32_t));
			if (!waveform[i]) fail("Waveform malloc failed");
		}
	}
}

void histo_init()
{
	if (!hist) hist = AllocateMemory(hist_width * sizeof(uint32_t*));
	if (!hist) fail("Hist malloc failed");

	if (!hist_r) hist_r = AllocateMemory(hist_width * sizeof(uint32_t*));
	if (!hist_r) fail("HistR malloc failed");

	if (!hist_g) hist_g = AllocateMemory(hist_width * sizeof(uint32_t*));
	if (!hist_g) fail("HistG malloc failed");

	if (!hist_b) hist_b = AllocateMemory(hist_width * sizeof(uint32_t*));
	if (!hist_b) fail("HistB malloc failed");
}

static void bvram_mirror_init()
{
	if (!bvram_mirror)
	{
		bvram_mirror = AllocateMemory(BMPPITCH*540 + 100);
		if (!bvram_mirror) 
		{	
			bmp_printf(FONT_MED, 30, 30, "Failed to allocate BVRAM mirror");
			return;
		}
		bzero32(bvram_mirror, 960*540);
	}
}

int get_focus_color(int thr, int d)
{
	return
		focus_peaking_color == 0 ? COLOR_RED :
		focus_peaking_color == 1 ? 7 :
		focus_peaking_color == 2 ? COLOR_BLUE :
		focus_peaking_color == 3 ? 5 :
		focus_peaking_color == 4 ? 14 :
		focus_peaking_color == 5 ? 15 :
		focus_peaking_color == 6 ? 	(thr > 50 ? COLOR_RED :
									 thr > 40 ? 19 /*orange*/ :
									 thr > 30 ? 15 /*yellow*/ :
									 thr > 20 ? 5 /*cyan*/ : 
									 9 /*light blue*/) :
		focus_peaking_color == 7 ? ( d > 50 ? COLOR_RED :
									 d > 40 ? 19 /*orange*/ :
									 d > 30 ? 15 /*yellow*/ :
									 d > 20 ? 5 /*cyan*/ : 
									 9 /*light blue*/) : 1;
}

static void little_cleanup(uint8_t* bp, uint8_t* mp)
{
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
	bp++; mp++;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
	bp++; mp++;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
	bp++; mp++;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
}

static int zebra_color_word_row(int c, int y)
{
	uint32_t cw = 0;
	switch(y % 4)
	{
		case 0:
			cw  = c  | c  << 8;
			break;
		case 1:
			cw  = c << 8 | c << 16;
			break;
		case 2:
			cw = c  << 16 | c << 24;
			break;
		case 3:
			cw  = c  << 24 | c ;
			break;
	}
	return cw;
}

// thresholded edge detection
static void draw_zebra_and_focus_unified( void )
{
	if (!global_draw) return;
	
	fps_ticks++;
	
	if (falsecolor_displayed) 
	{
		draw_false_downsampled();
		return;
	}

	bvram_mirror_init();

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;
	if (lv_dispsize != 1) return; // zoom not handled, better ignore it

	uint32_t x,y;
	int zd = (zebra_draw == 1) || (zebra_draw == 2 && recording == 0);  // when to draw zebras
	if (focus_peaking || zd) {
  		// clear previously written pixels
  		#define MAX_DIRTY_PIXELS 5000
  		static int* dirty_pixels = 0;
  		if (!dirty_pixels) dirty_pixels = AllocateMemory(MAX_DIRTY_PIXELS * sizeof(int));
  		if (!dirty_pixels) return;
  		static int dirty_pixels_num = 0;
  		static int very_dirty = 0;
  		bmp_ov_loc_size_t os;
  		calc_ov_loc_size(&os);
  		struct vram_info * _vram;
  		
  		if(use_hd_vram) { 
  			_vram=get_yuv422_hd_vram();
		} else {
			_vram=get_yuv422_vram();
		}
  		uint8_t * const vram = /*UNCACHEABLE*/(_vram->vram);
		int vr_width  = _vram->width<<1;
  		int vr_height = _vram->height;
		int vr_pitch =  _vram->pitch;

		int bm_lv_y = 0;

		if(shooting_mode == SHOOTMODE_MOVIE) {
			bm_lv_y = (os.bmp_ex_y-os.bmp_ex_x*9/16);
			if(((ext_monitor_hdmi || ext_monitor_rca) && !recording ) || (!ext_monitor_hdmi && !ext_monitor_rca)){
				bm_lv_y>>=1;
			}
		}

		os.bmp_ex_x>>=2; //reduce x size to quad pixels 
		os.bmp_of_x>>=2; //reduce offset to quad pixels
		
		int vr_x_of_corr = 0;
		int vr_x_ex_corr = 0;
		int vr_y_of_corr = 0;
		int vr_y_ex_corr = 0;
		
		if(use_hd_vram) {
			vr_y_of_corr = recording ? bm_lv_y:0;
			vr_y_ex_corr = recording ? bm_lv_y+os.bmp_of_y:0;
			if(!ext_monitor_hdmi && !ext_monitor_rca) {
				vr_y_ex_corr<<=1;
			}
		} else {
			vr_x_of_corr = os.bmp_of_x; // number of double pixels we go left
			vr_x_ex_corr = os.bmp_of_x<<((ext_monitor_hdmi || ext_monitor_rca)&&recording?2:3);
			
			vr_y_of_corr=-os.bmp_of_y;
			vr_height=os.lv_ex_y;
			if(ext_monitor_hdmi && video_mode_resolution) {
				vr_height>>=1;
				if(video_mode_crop) { // FIXME crop mode with external displays
					vr_x_of_corr<<=2;
					vr_height>>=1;
					vr_width>>=2;
				}
			}
		}
		
//		bmp_printf(FONT_MED, 30, 100, "HD %dx%dp:%d vxc:%d vxo:%d vyc:%d vyo:%d byo:%d blvy:%d", vr_width>>1, vr_height, vr_pitch, vr_x_ex_corr, vr_x_of_corr, vr_y_ex_corr, vr_y_of_corr, os.bmp_of_y,  bm_lv_y);

  		int ymin = os.bmp_of_y + bm_lv_y;
  		int ymax = os.bmp_ex_y + os.bmp_of_y - bm_lv_y;
  		int xmin = os.bmp_of_x;
  		int xmax = os.bmp_ex_x + os.bmp_of_x;

 		ymin=COERCE(ymin, 0, 540);
  		ymax=COERCE(ymax, 0, 540);
  		xmin=COERCE(xmin, 0, 960);
  		xmax=COERCE(xmax, 0, 960);

  		static int16_t* xcalc = 0;
  		if (!xcalc) xcalc = AllocateMemory(960 * 2);
  		if (!xcalc) return;
  		static int xcalc_done=0;
  		
  		if(!xcalc_done || crop_dirty) {
	  		for (x = xmin; x < xmax; x++) {
  				xcalc[x]=(x-os.bmp_of_x+vr_x_of_corr)*((vr_width>>2)-vr_x_ex_corr)/os.bmp_ex_x;
			}
			xcalc_done=1;
		}

		uint32_t zlh = zebra_level_hi << 8;
		uint32_t zll = zebra_level_lo << 8;
		if(focus_peaking) {
			int i;
			for (i = 0; i < dirty_pixels_num; i++) {
				dirty_pixels[i] = COERCE(dirty_pixels[i], 0, 960*540);
				#define B1 *(uint16_t*)(bvram + dirty_pixels[i])
				#define B2 *(uint16_t*)(bvram + dirty_pixels[i] + BMPPITCH)
				#define M1 *(uint16_t*)(bvram_mirror + dirty_pixels[i])
				#define M2 *(uint16_t*)(bvram_mirror + dirty_pixels[i] + BMPPITCH)
				if ((B1 == 0 || B1 == M1) && (B2 == 0 || B2 == M2))
					B1 = B2 = M1 = M2 = 0;
				#undef B1
				#undef B2
				#undef M1
				#undef M2
			}
			dirty_pixels_num = 0;
  		}
  
  		static int thr = 50;
  		int n_over = 0;
  		
  		
		for( y = ymin; y < ymax; y+=2 ) {
			uint32_t * const vr_row = (uint32_t*)( vram + (y-os.bmp_of_y-vr_y_of_corr) * vr_height/(os.bmp_ex_y-vr_y_ex_corr) * vr_pitch ); // 2 pixels
			int b_row_off = y * BMPPITCH;
			uint32_t * const b_row = (uint32_t*)( bvram + b_row_off );   // 4 pixels
			uint32_t * const m_row = (uint32_t*)( bvram_mirror + b_row_off );   // 4 pixels
  
			for ( x = xmin; x < xmax; x++ ) {
				#define BP (b_row[x])
				#define MP (m_row[x])
				#define BN (b_row[x + (BMPPITCH>>2)])
				#define MN (m_row[x + (BMPPITCH>>2)])

				uint32_t pixel = vr_row[xcalc[x]];

				uint32_t bp = BP;
				uint32_t mp = MP;
				uint32_t bn = BN;
				uint32_t mn = MN;
				
//				BP=(pixel&0xff)>>8 | (pixel&0xff000000)>>24;

				if (zd) {
					int zebra_done = 0;
					if (bp != 0 && bp != mp) { little_cleanup(b_row + x, m_row + x); zebra_done = 1; }
					if (bn != 0 && bn != mn) { little_cleanup(b_row + x + (BMPPITCH>>1), m_row + x + (BMPPITCH>>1)); zebra_done = 1; }

					if(!zebra_done) {
						uint32_t p0 = pixel & 0xFF00;
						int color = 0;
					
						if (p0 > zlh) {
							color = COLOR_RED;
						} else if (p0 < zll) {
							color = COLOR_BLUE;
						}
						
						switch(zebra_density) {
							case 0:
								BP = MP = color;
								BN = MN = color<<16;
								break;
							case 1:
								BP = MP = color<<8 | color;
								BN = MN = color<<24 | color<<16;
								break;
							case 2:
								if(!(y&2)) {
										BP = MP = color<<8 | color;
										BN = MN = color<<16 | color<<8;
								} else {
										BP = MP = color<<16 | color<<24;
										BN = MN = color<<24 | color;
								}
								break;
						}
					}
  				}

				if(focus_peaking) {
					int32_t p0 = (pixel >> 24) & 0xFF;
					int32_t p1 = (pixel >>  8) & 0xFF;
					int32_t d = ABS(p0-p1);
					if (d < thr) continue;
					n_over++;
					// executed for 1% of pixels
					if (n_over > MAX_DIRTY_PIXELS) { // threshold too low, abort
						thr = MIN(thr+2, 255);
						continue;
					}

					int color = get_focus_color(thr, d);
					color = (color << 8) | color;   
					if ((bp == 0 || bp == mp) && (bn == 0 || bn == mn)) { // safe to draw
						BP = BN = MP = MN = color;
						if (dirty_pixels_num < MAX_DIRTY_PIXELS) {
							dirty_pixels[dirty_pixels_num++] = (x<<2) + b_row_off;
						}
					}
  				}
  				#undef MP
  				#undef BP
				#undef BN
				#undef MN
  			}
  		}
		int yy=250 * n_over / (os.bmp_ex_x * (os.bmp_ex_y - (bm_lv_y<<1)));
		bmp_printf(FONT_LARGE, 10, 50, "%d ", thr);
//		bmp_printf(FONT_LARGE, 10, 50, "%d %d %d>%d ", thr, n_over, yy, focus_peaking_pthr);
		if ( yy > focus_peaking_pthr) {
			thr++;
		} else {
			thr--;
		}
		int thr_min = (lens_info.iso > 1600 ? 15 : 10);
		thr = COERCE(thr, thr_min, 255);
  	}
}

int focus_peaking_debug = 0;

// thresholded edge detection
static void
draw_zebra_and_focus( void )
{
	if (unified_loop == 1) { draw_zebra_and_focus_unified(); return; }
	if (unified_loop == 2 && (ext_monitor_hdmi || ext_monitor_rca || (shooting_mode == SHOOTMODE_MOVIE && video_mode_resolution != 0)))
		{ draw_zebra_and_focus_unified(); return; }
	
	if (!global_draw) return;
	
	fps_ticks++;
	
	if (falsecolor_displayed) 
	{
		draw_false_downsampled();
		return;
	}
	// HD to LV coordinate transform:
	// non-record: 1056 px: 1.46 ratio (yuck!)
	// record: 1720: 2.38 ratio (yuck!)
	
	// How to scan?
	// Scan the HD vram and do ratio conversion only for the 1% pixels displayed

	bvram_mirror_init();

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;
	//~ int BMPPITCH = bmp_pitch();
	uint32_t x,y;

	if (focus_peaking)
	{
		// clear previously written pixels
		#define MAX_DIRTY_PIXELS 5000
  		static int* dirty_pixels = 0;
  		if (!dirty_pixels) dirty_pixels = AllocateMemory(MAX_DIRTY_PIXELS * sizeof(int));
  		if (!dirty_pixels) return;
		static int dirty_pixels_num = 0;
		static int very_dirty = 0;
		int i;
		for (i = 0; i < dirty_pixels_num; i++)
		{
			dirty_pixels[i] = COERCE(dirty_pixels[i], 0, 950*540);
			#define B1 *(uint16_t*)(bvram + dirty_pixels[i])
			#define B2 *(uint16_t*)(bvram + dirty_pixels[i] + BMPPITCH)
			#define M1 *(uint16_t*)(bvram_mirror + dirty_pixels[i])
			#define M2 *(uint16_t*)(bvram_mirror + dirty_pixels[i] + BMPPITCH)
			if ((B1 == 0 || B1 == M1) && (B2 == 0 || B2 == M2))
				B1 = B2 = M1 = M2 = 0;
			#undef B1
			#undef B2
			#undef M1
			#undef M2
		}
		dirty_pixels_num = 0;

		if (lv_dispsize != 1) return; // zoom not handled, better ignore it
		
		int bm_pitch = (ext_monitor_hdmi && !recording) ? 960 : 720; // or other value for ext monitor
		int bm_width = bm_pitch;  // 8-bit palette image
		int bm_height = (ext_monitor_hdmi && !recording) ? 540 : 480;
		
		struct vram_info * hd_vram = get_yuv422_hd_vram();
		uint8_t * const hdvram = UNCACHEABLE(hd_vram->vram);
		int hd_pitch  = hd_vram->pitch;
		int hd_height = hd_vram->height;
		int hd_width  = hd_vram->width;
		
		//~ bmp_printf(FONT_MED, 30, 100, "HD %dx%d ", hd_width, hd_height);
		
		int bm_skipv = 50;
		int bm_skiph = 100;
		int hd_skipv = bm_skipv * hd_height / bm_height;
		int hd_skiph = bm_skiph * hd_width / bm_width;
		
		static int thr = 50;
		
		int n_over = 0;
		int n_total = 0;
		// look in the HD buffer

		int rec_off = (recording ? 90 : 0);
		int step = (focus_peaking == 1) 
						? (recording ? 2 : 1)
						: (recording ? 4 : 2);
		for( y = hd_skipv; y < hd_height - hd_skipv; y += 2 )
		{
			uint32_t * const hd_row = (uint32_t*)( hdvram + y * hd_pitch ); // 2 pixels
			uint32_t * const hd_row_end = hd_row + hd_width/2 - hd_skiph/2;
			
			uint32_t* hdp; // that's a moving pointer
			for (hdp = hd_row + hd_skiph/2 ; hdp < hd_row_end ; hdp += step )
			{
				#define PX_AB (*hdp)        // current pixel group
				#define PX_CD (*(hdp + 1))  // next pixel group
				#define a ((int32_t)(PX_AB >>  8) & 0xFF)
				#define b ((int32_t)(PX_AB >> 24) & 0xFF)
				#define c ((int32_t)(PX_CD >>  8) & 0xFF)
				#define d ((int32_t)(PX_CD >> 24) & 0xFF)
				
				#define mBC MIN(b,c)
				#define AE MIN(a,b)
				#define BE MIN(a, mBC)
				#define CE MIN(mBC, d)

				#define MBC MAX(b,c)
				#define AD MAX(a,b)
				#define BD MAX(a, MBC)
				#define CD MAX(MBC, d)

				#define BED MAX(AE,MAX(BE,CE))
				#define BDE MIN(AD,MIN(BD,CD))

				#define SIGNBIT(x) (x & (1<<31))
				#define CHECKSIGN(a,b) (SIGNBIT(a) ^ SIGNBIT(b) ? 0 : 0xFF)
				#define D1 (b-a)
				#define D2 (c-b)
				#define D3 (d-c)

				#define e_morph (ABS(b - ((BDE + BED) >> 1)) << 3)
				//~ #define e_opposite_sign (MAX(0, - (c-b)*(b-a)) >> 3)
				//~ #define e_sign3 CHECKSIGN(D1,D3) & CHECKSIGN(D1,-D2) & ((ABS(D1) + ABS(D2) + ABS(D3)) >> 1)

				int e = (focus_peaking == 1) ? ABS(D1) :
						(focus_peaking == 2) ? e_morph : 0;
				#undef a
				#undef b
				#undef c
				#undef d
				#undef z
				
				/*if (focus_peaking_debug)
				{
					int b_row_off = COERCE((y + rec_off) * bm_width / hd_width, 0, 539) * BMPPITCH;
					uint16_t * const b_row = (uint16_t*)( bvram + b_row_off );   // 2 pixels
					int x = 2 * (hdp - hd_row) * bm_width / hd_width;
					x = COERCE(x, 0, 960);
					int c = (COERCE(e, 0, thr*2) * 41 / thr / 2) + 38;
					b_row[x/2] = c | (c << 8);
				}*/
				
				n_total++;
				if (e < thr) continue;
				// else
				{ // executed for 1% of pixels
					n_over++;
					if (n_over > MAX_DIRTY_PIXELS) // threshold too low, abort
					{
						thr = MIN(thr+2, 255);
						return;
					}

					int color = get_focus_color(thr, e);
					//~ int color = COLOR_RED;
					color = (color << 8) | color;   
					int b_row_off = COERCE((y + rec_off) * bm_width / hd_width, 0, 539) * BMPPITCH;
					uint16_t * const b_row = (uint16_t*)( bvram + b_row_off );   // 2 pixels
					uint16_t * const m_row = (uint16_t*)( bvram_mirror + b_row_off );   // 2 pixels
					
					int x = 2 * (hdp - hd_row) * bm_width / hd_width;
					x = COERCE(x, 0, 960);
					
					uint16_t pixel = b_row[x/2];
					uint16_t mirror = m_row[x/2];
					uint16_t pixel2 = b_row[x/2 + BMPPITCH/2];
					uint16_t mirror2 = m_row[x/2 + BMPPITCH/2];
					if ((pixel == 0 || pixel == mirror) && (pixel2 == 0 || pixel2 == mirror2)) // safe to draw
					{
						b_row[x/2] = color;
						b_row[x/2 + BMPPITCH/2] = color;
						m_row[x/2] = color;
						m_row[x/2 + BMPPITCH/2] = color;
						if (dirty_pixels_num < MAX_DIRTY_PIXELS)
						{
							dirty_pixels[dirty_pixels_num++] = x + b_row_off;
						}
					}
				}
			}
		}
		bmp_printf(FONT_LARGE, 10, 50, "%d ", thr);
		if (1000 * n_over / n_total > focus_peaking_pthr) thr++;
		else thr--;
		
		int thr_min = (lens_info.iso > 1600 ? 15 : 10);
		thr = COERCE(thr, thr_min, 255);
	}
	
	int zd = (zebra_draw == 1) || (zebra_draw == 2 && recording == 0);  // when to draw zebras
	if (zd)
	{
		uint32_t zlh = zebra_level_hi << 8;
		uint32_t zll = zebra_level_lo << 8;

		uint8_t * const lvram = YUV422_LV_BUFFER;
		int lvpitch = YUV422_LV_PITCH;
		for( y = 0; y < 480; y += 2 )
		{
			uint32_t color_over = zebra_color_word_row(COLOR_RED, y);
			uint32_t color_under = zebra_color_word_row(COLOR_BLUE, y);
			uint32_t color_over_2 = zebra_color_word_row(COLOR_RED, y+1);
			uint32_t color_under_2 = zebra_color_word_row(COLOR_BLUE, y+1);
			
			uint32_t * const v_row = (uint32_t*)( lvram + y * lvpitch );          // 2 pixels
			uint32_t * const b_row = (uint32_t*)( bvram + y * BMPPITCH);          // 4 pixels
			uint32_t * const m_row = (uint32_t*)( bvram_mirror + y * BMPPITCH );  // 4 pixels
			
			uint32_t* lvp; // that's a moving pointer through lv vram
			uint32_t* bp;  // through bmp vram
			uint32_t* mp;  // through mirror

			for (lvp = v_row, bp = b_row, mp = m_row ; lvp < v_row + YUV422_LV_PITCH/4 ; lvp += 2, bp++, mp++)
			{
				#define BP (*bp)
				#define MP (*mp)
				#define BN (*(bp + BMPPITCH/4))
				#define MN (*(mp + BMPPITCH/4))
				if (BP != 0 && BP != MP) { little_cleanup(bp, mp); continue; }
				if (BN != 0 && BN != MN) { little_cleanup(bp + BMPPITCH/4, mp + BMPPITCH/4); continue; }
				uint32_t p0 = *lvp & 0xFF00;
				if (p0 > zlh)
				{
					BP = MP = color_over;
					BN = MN = color_over_2;
				}
				else if (p0 < zll)
				{
					BP = MP = color_under;
					BN = MN = color_under_2;
				}
				else if (BP)
					BN = MN = BP = MP = 0;
				#undef MP
				#undef BP
			}
		}
	}
}


// clear only zebra, focus assist and whatever else is in BMP VRAM mirror
void
clrscr_mirror( void )
{
	if (!global_draw) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;

	uint32_t x,y;
	for( y = 0; y < 480; y++ )
	{
		uint32_t * const b_row = (uint32_t*)( bvram + y * BMPPITCH);
		uint32_t * const m_row = (uint32_t*)( bvram_mirror + y * BMPPITCH );
		
		uint32_t* bp;  // through bmp vram
		uint32_t* mp;  // through mirror

		for (bp = b_row, mp = m_row ; bp < b_row + BMPPITCH / 4; bp++, mp++)
		{
			#define BP (*bp)
			#define MP (*mp)
			if (BP != 0)
			{ 
				if (BP == MP) BP = MP = 0;
				else little_cleanup(bp, mp);
			}			
			#undef MP
			#undef BP
		}
	}
}

/*
void
draw_false( void )
{
	if (!global_draw) return;
	bvram_mirror_init();
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;

	uint32_t x,y;
	uint8_t * const lvram = CACHEABLE(YUV422_LV_BUFFER);
	int lvpitch = YUV422_LV_PITCH;
	for( y = 100; y < 480-100; y++ )
	{
		uint16_t * const v_row = (uint16_t*)( lvram + y * lvpitch );        // 1 pixel
		uint8_t * const b_row = (uint8_t*)( bvram + y * BMPPITCH);          // 1 pixel
		uint8_t * const m_row = (uint8_t*)( bvram_mirror + y * BMPPITCH );  // 1 pixel
		
		uint16_t* lvp; // that's a moving pointer through lv vram
		uint8_t* bp;  // through bmp vram
		uint8_t* mp;  // through mirror

		for (lvp = v_row + 100, bp = b_row + 100, mp = m_row + 100; lvp < v_row + 720-100 ; lvp++, bp++, mp++)
		{
			if (*bp != 0 && *bp != *mp) continue;
			*mp = *bp = false_colour[(*lvp) >> 8];
		}
	}
}*/

void
draw_false_downsampled( void )
{
	if (!global_draw) return;
	bvram_mirror_init();
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;

	uint32_t x,y;
	uint8_t * const lvram = UNCACHEABLE(YUV422_LV_BUFFER);
	int lvpitch = YUV422_LV_PITCH;
	for( y = 0; y < 480; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( lvram + y * lvpitch );        // 2 pixel
		uint16_t * const b_row = (uint16_t*)( bvram + y * BMPPITCH);          // 2 pixel
		uint16_t * const m_row = (uint16_t*)( bvram_mirror + y * BMPPITCH );  // 2 pixel
		
		uint32_t* lvp; // that's a moving pointer through lv vram
		uint16_t* bp;  // through bmp vram
		uint16_t* mp;  // through mirror

		for (lvp = v_row, bp = b_row, mp = m_row; lvp < v_row + 720 ; lvp++, bp++, mp++)
		{
			if (*bp != 0 && *bp != *mp) continue;
			int16_t c = false_colour[((*lvp) >> 8) & 0xFF];
			*mp = *bp = c | (c << 8);
		}
	}
}

/*
static void
draw_zebra( void )
{
	if (!lv_drawn()) return;
	
	uint8_t * const bvram = bmp_vram();
    uint32_t a = 0;
    
	bvram_mirror_init();

    DebugMsg(DM_MAGIC, 3, "***************** draw_zebra() **********************");
    DebugMsg(DM_MAGIC, 3, "zebra_draw = %d, cfg_draw_meters = %x", zebra_draw, ext_cfg_draw_meters() );
    //~ dump_seg(0x40D07800, 1440*480, "B:/vram1.dat");
    
	// If we don't have a bitmap vram yet, nothing to do.
	if( !bvram )
	{
		DebugMsg( DM_MAGIC, 3, "draw_zebra() no bvram, fail");
		return;
	}
	if (!bvram_mirror)
	{
		DebugMsg( DM_MAGIC, 3, "draw_zebra() no bvram_mirror, fail");
		return;
	}

	struct vram_info * vram = get_yuv422_vram();
    if (hist_draw)
    {
        // something is fishy here => camera refused to boot due to this function...
        hist_build(vram->vram, vram->width, vram->pitch);
    }

    DebugMsg(DM_MAGIC, 3, "yay!");

	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	// 33 is the bottom of the meters; 55 is the crop mark
	uint32_t x,y;
	int cfg_draw_meters = ext_cfg_draw_meters();
	//int BMPPITCH = bmp_pitch();
	
	int zd = (zebra_draw == 1) || (zebra_draw == 2 && recording == 0);  // when to draw zebras
	for( y=1 ; y < 480; y++ )
	{
        // if audio meters are enabled, don't draw in this area
        if (y < 33 && (cfg_draw_meters == 1 || (cfg_draw_meters == 2 && shooting_mode == SHOOTMODE_MOVIE))) continue;
        
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bvram + y * BMPPITCH );
		uint16_t * const m_row = (uint16_t*)( bvram_mirror + y * BMPPITCH );

		//~ bmp_printf(FONT_MED, 30, 50, "Row: %8x/%8x", b_row, m_row);
		//~ bmp_printf(FONT_MED, 30, 70, "Pixel: %8x/%8x", b_row[8], m_row[8]);

		// Iterate over the pixels in the scan row
		// two at a time to read the pixel buf in 32 bit chunks
		// otherwise we get err70 aborts while drawing regions
		// in the bitmap vram.
		for( x=2 ; x < vram->width-2 ; x+=2 ) // width = 720
		{
			// Abort as soon as the new menu is drawn
			if( gui_menu_task || !lv_drawn() )
				return;

			uint16_t pixel = b_row[x/2];
			uint16_t mirror = m_row[x/2];

			// cropmarks: black border in movie mode
			if (crop_black_border && (pixel == 0 || pixel == (COLOR_BG << 8 | COLOR_BG)) && shooting_mode == SHOOTMODE_MOVIE && (y < 40 || y > 440))
			{
				b_row[x/2] = (2 << 8 | 2); // black borders by default
				if( crop_draw) check_crop( x, y, b_row, v_row, vram->pitch, m_row);
			}

			if (pixel != 0 && pixel != mirror)
			{
				continue; // Canon code has drawn here, do not overwrite
			}
				
			// Ignore the regions where the histogram will be drawn
			if( hist_draw
			&&  y >= hist_y
			&&  y <  hist_y + hist_height
			&&  x >= hist_x
			&&  x <  hist_x + hist_width + 4
			)
				continue; // histogram does not overwrite any Canon stuff => no problem!

			// Ignore the regions where the waveform will be drawn
			//~ if( waveform_draw
			//~ &&  y >= waveform_y
			//~ &&  y <  waveform_y + WAVEFORM_HEIGHT
			//~ &&  x >= waveform_x
			//~ &&  x <  waveform_x + WAVEFORM_WIDTH
			//~ )
				//~ continue;

			// Ignore the timecode region
			//~ if( y >= timecode_y
			//~ &&  y <  timecode_y + timecode_height
			//~ &&  x >= timecode_x
			//~ &&  x <  timecode_x + timecode_width
			//~ )
				//~ continue;

			if( crop_draw && check_crop( x, y, b_row, v_row, vram->pitch, m_row) )
			{
				//~ m_row[x/2] = b_row[x/2];
				continue;
			}
			//~ if( edge_draw && check_edge( x, y, b_row, v_row, vram->pitch ) )
				//~ continue;

			if( zd && check_zebra( x, y, b_row, v_row, vram->pitch, m_row) )
			{
				//~ m_row[x/2] = b_row[x/2];
				continue;
			}

			// Nobody drew on it, make it clear
			if (pixel) b_row[x/2] = 0;
			//m_row[x/2] = 0;
		}
	}

	if( hist_draw )
		hist_draw_image( hist_x, hist_y );

	if( spotmeter_draw)
		spotmeter_step();
	//~ if( waveform_draw )
		//~ waveform_draw_image( waveform_x, waveform_y );
    DebugMsg(DM_MAGIC, 3, "***************** draw_zebra done **********************");
}*/

static void
zebra_lo_toggle( void * priv )
{
	zebra_level_lo = mod(zebra_level_lo + 1, 50);
}

static void
zebra_lo_toggle_reverse( void * priv )
{
	zebra_level_lo = mod(zebra_level_lo - 1, 50);
}

static void
clearpreview_toggle( void * priv )
{
	int * ptr = priv;
	*ptr = mod(*ptr + 1, 3);
}

static void
clearpreview_toggle_reverse( void * priv )
{
	int * ptr = priv;
	*ptr = mod(*ptr - 1, 3);
}


static void
zebra_hi_toggle( void * priv )
{
	zebra_level_hi = 200 + mod(zebra_level_hi - 200 + 1, 56);
}

static void
zebra_hi_toggle_reverse( void * priv )
{
	zebra_level_hi = 200 + mod(zebra_level_hi - 200 - 1, 56);
}

static void global_draw_toggle(void* priv)
{
	menu_binary_toggle(priv);
	if (!global_draw && lv_drawn()) bmp_fill(0, 0, 0, 720, 480);
}

#define MAX_CROP_NAME_LEN 15
#define MAX_CROPMARKS 9
int num_cropmarks = 0;
char cropmark_names[MAX_CROPMARKS][MAX_CROP_NAME_LEN];
static void find_cropmarks()
{
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( "B:/CROPMKS/", &file );
	if( IS_ERROR(dirent) )
	{
		bmp_printf( FONT_LARGE, 40, 40,
			"%s: dirent=%08x!",
			__func__,
			(unsigned) dirent
		);
		return;
	}
	int k = 0;
	do {
		char* s = strstr(file.name, ".BMP");
		if (s && !strstr(file.name, "~"))
		{
			if (k >= MAX_CROPMARKS)
			{
				bmp_printf(FONT_LARGE, 0, 50, "TOO MANY CROPMARKS (max=%d)", MAX_CROPMARKS);
				break;
			}
			snprintf(cropmark_names[k], MAX_CROP_NAME_LEN, "%s", file.name);
			k++;
		}
	} while( FIO_FindNextEx( dirent, &file ) == 0);
	num_cropmarks = k;
}
static void load_cropmark(int i)
{
	if (cropmarks)
	{
		cropmark_draw(1); // delete old cropmark from screen
		FreeMemory(cropmarks);
		cropmarks = 0;
	}
	
	i = COERCE(i, 0, num_cropmarks);
	if (i)
	{
		char bmpname[100];
		snprintf(bmpname, sizeof(bmpname), "B:/CROPMKS/%s", cropmark_names[i-1]);
		cropmarks = bmp_load(bmpname);
		if (!cropmarks) bmp_printf(FONT_LARGE, 0, 50, "LOAD ERROR %d:%s   ", i, bmpname);
	}
}

static void
crop_toggle( int sign )
{
	msleep(100);
	crop_draw = mod(crop_draw + sign, num_cropmarks + 1);  // 0 = off, 1..num_cropmarks = cropmarks
	load_cropmark(crop_draw);
	msleep(100);
}

static void crop_toggle_forward(void* priv)
{
	crop_toggle(1);
}

static void crop_toggle_reverse(void* priv)
{
	crop_toggle(-1);
}

static void
zebra_hi_display( void * priv, int x, int y, int selected )
{
	bmp_printf( 
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ZebraThrHI  : %d   ",
		*(unsigned*) priv
	);
}

static void
zebra_lo_display( void * priv, int x, int y, int selected )
{
	bmp_printf( 
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ZebraThrLO  : %d   ",
		*(unsigned*) priv
	);
}


static void
zebra_draw_display( void * priv, int x, int y, int selected )
{
	unsigned z = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zebras      : %s, %d..%d",
		z == 1 ? "ON " : (z == 2 ? "NRec" : "OFF"),
		zebra_level_lo, zebra_level_hi
	);
}

static void
falsecolor_display( void * priv, int x, int y, int selected )
{
	unsigned fc = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"False Color : %s%s",
		fc ? (falsecolor_shortcutkey ? "Shortcut Key": "Always ON") : "OFF"
	);
}

static void
focus_debug_display( void * priv, int x, int y, int selected )
{
	unsigned fc = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"FPeak debug : %s",
		focus_peaking_debug ? "ON" : "OFF"
	);
}

static void
focus_peaking_display( void * priv, int x, int y, int selected )
{
	unsigned f = *(unsigned*) priv;
	if (f)
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Focus Peak  : %s,%d.%d,%s", 
			focus_peaking == 1 ? "HDIF" : 
			focus_peaking == 2 ? "MORF" : "?",
			focus_peaking_pthr / 10, focus_peaking_pthr % 10, 
			focus_peaking_color == 0 ? "R" :
			focus_peaking_color == 1 ? "G" :
			focus_peaking_color == 2 ? "B" :
			focus_peaking_color == 3 ? "C" :
			focus_peaking_color == 4 ? "M" :
			focus_peaking_color == 5 ? "Y" :
			focus_peaking_color == 6 ? "cc1" :
			focus_peaking_color == 7 ? "cc2" : "err"
		);
	else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Focus Peak  : OFF"
		);
}

static void focus_peaking_adjust_thr(void* priv)
{
	if (focus_peaking)
	{
		focus_peaking_pthr += (focus_peaking_pthr < 10 ? 1 : 5);
		if (focus_peaking_pthr > 50) focus_peaking_pthr = 1;
	}
}
static void focus_peaking_adjust_color(void* priv)
{
	if (focus_peaking)
		focus_peaking_color = mod(focus_peaking_color + 1, 8);
}
static void
crop_display( void * priv, int x, int y, int selected )
{
	extern int retry_count;
	int index = crop_draw;
	index = COERCE(index, 0, num_cropmarks);
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Cropmk%s(%d/%d): %s%s",
		 (cropmark_playback ? "P" : "s"),
		 index, num_cropmarks,
		 index  ? cropmark_names[index-1] : "OFF",
		 (cropmarks || !index) ? "" : "!" // ! means error
	);
}

static void
focus_graph_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Focus Graph : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}

int get_focus_graph() { return focus_graph; }

static void
edge_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Edgedetect  : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}

static void
time_indicator_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Time Indicator: %s",
		time_indicator == 1 ? "Elapsed" :
		time_indicator == 2 ? "Remain.Card" :
		time_indicator == 3 ? "Remain.4GB" : "OFF"
	);
}

static void
hist_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Histo/Wavefm: %s/%s",
		hist_draw == 1 ? "Luma" : hist_draw == 2 ? "RGB" : "OFF",
		waveform_draw == 1 ? "Small" : waveform_draw == 2 ? "Large" : "OFF"
	);
	bmp_printf(FONT_MED, x + 460, y+5, "[SET/Q]");
}

static void
global_draw_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Global Draw : %s",
		global_draw ? "ON " : "OFF"
	);
}

static void
waveform_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Waveform    : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}
static void 
waveform_toggle(void* priv)
{
	waveform_draw = mod(waveform_draw+1, 3);
	bmp_fill(0, 360, 240-50, 360, 240);
}


static void
clearpreview_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	int mode = *(int*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ClrScreen   : %s",
		(mode == 0 ? "OFF" : 
		(mode == 1 ? "HalfShutter" : 
		(mode == 2 ? "WhenIdle" :
		"Error")))
	);
}


static void
spotmeter_menu_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Spotmeter   : %s%s",
		spotmeter_draw == 0 ? "OFF" : spotmeter_draw == 1 ? "  " : "  ",
		spotmeter_draw == 0 ? "" : (spotmeter_formula == 0 ? "Percent" : spotmeter_formula == 1 ? "IRE -1..101" : "IRE 0..108")
	);
	if (spotmeter_draw == 1) bmp_draw_rect(COLOR_WHITE, x + 14 * font_large.width + 10, y + 10, 10, 10);
	if (spotmeter_draw == 2) bmp_printf(FONT_SMALL, x + 14 * font_large.width, y + 10, "123");
}

static void 
spotmeter_formula_toggle(void* priv)
{
	spotmeter_formula = mod(spotmeter_formula + 1, 3);
}



void get_spot_yuv(int dx, uint8_t* Y, int8_t* U, int8_t* V)
{
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return;
	const uint16_t*		vr = vram->vram;
	const unsigned		width = vram->width;
	const unsigned		pitch = vram->pitch;
	const unsigned		height = vram->height;
	unsigned		x, y;

	unsigned sy = 0;
	int32_t su = 0, sv = 0; // Y is unsigned, U and V are signed
	// Sum the values around the center
	for( y = height/2 - dx ; y <= height/2 + dx ; y++ )
	{
		for( x = width/2 - dx ; x <= width/2 + dx ; x++ )
		{
			uint16_t p = vr[ x + y * width ];
			sy += p & 0xFF00;
			if (x % 2) su += (int)(p & 0x00FF); else sv += (int)(p & 0x00FF); // U and V may be reversed
		}
	}

	sy /= (2 * dx + 1) * (2 * dx + 1);
	su /= (dx + 1) * (2 * dx + 1);
	sv /= (dx + 1) * (2 * dx + 1);

	*Y = sy >> 8;
	*U = su;
	*V = sv;
}


int get_spot_focus(int dx)
{
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return;
	const uint32_t*		vr = vram->vram; // 2px
	const unsigned		width = vram->width;
	const unsigned		pitch = vram->pitch;
	const unsigned		height = vram->height;
	unsigned		x, y;
	
	unsigned sf = 0;
	unsigned br = 0;
	// Sum the absolute difference of values around the center
	for( y = height/2 - dx ; y <= height/2 + dx ; y++ )
	{
		for( x = width/2 - dx ; x <= width/2 + dx ; x += 2 )
		{
			uint32_t p = vr[ x/2 + y * width/2 ];
			int32_t p0 = (p >> 24) & 0xFF;
			int32_t p1 = (p >>  8) & 0xFF;
			sf += ABS(p1 - p0);
			br += p1 + p0;
		}
	}
	return sf / (br >> 14);
}

PROP_INT(PROP_HOUTPUT_TYPE, lv_disp_mode);

void spotmeter_step()
{
    if (!lv_drawn()) return;
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return;
	
	const uint16_t*		vr = vram->vram;
	const unsigned		width = vram->width;
	const unsigned		pitch = vram->pitch;
	const unsigned		height = vram->height;
	const unsigned		dx = spotmeter_size;
	unsigned		sum = 0;
	unsigned		x, y;

	// Sum the values around the center
	for( y = height/2 - dx ; y <= height/2 + dx ; y++ )
	{
		for( x = width/2 - dx ; x <= width/2 + dx ; x++ )
			sum += (vr[ x + y * width]) & 0xFF00;
	}

	sum /= (2 * dx + 1) * (2 * dx + 1);

	// Scale to 100%
	const unsigned		scaled = (100 * sum) / 0xFF00;
	
	// spotmeter color: 
	// black on transparent, if brightness > 60%
	// white on transparent, if brightness < 50%
	// previous value otherwise
	
	// if false color is active, draw white on semi-transparent gray
	
	static int fg = 0;
	if (scaled > 60) fg = COLOR_BLACK;
	if (scaled < 50 || falsecolor_displayed) fg = COLOR_WHITE;
	int bg = falsecolor_displayed ? COLOR_BG : 0;

	int xc = (ext_monitor_hdmi && !recording) ? 480 : 360;
	int yc = (ext_monitor_hdmi && !recording) ? 270 : 240;
	if (spotmeter_draw == 1) // square marker
	{
		bmp_draw_rect(fg, xc - dx, yc - dx, 2*dx+1, 2*dx+1);
		yc += dx + 20;
	}
	yc -= font_med.height/2;
	xc -= 2 * font_med.width;

	if (spotmeter_formula == 0)
	{
		bmp_printf(
			FONT(FONT_MED, fg, bg),
			xc, yc, 
			"%3d%%",
			scaled
		);
	}
	else
	{
		int ire_aj = (((int)sum >> 8) - 2) * 102 / 253 - 1; // formula from AJ: (2...255) -> (-1...101)
		int ire_piers = ((int)sum >> 8) * 108/255;           // formula from Piers: (0...255) -> (0...108)
		int ire = (spotmeter_formula == 1) ? ire_aj : ire_piers;
		
		bmp_printf(
			FONT(FONT_MED, fg, bg),
			xc, yc, 
			"%s%3d", // why does %4d display garbage?!
			ire < 0 ? "-" : " ",
			ire < 0 ? -ire : ire
		);
		bmp_printf(
			FONT(FONT_SMALL, fg, 0),
			xc + font_med.width*4, yc,
			"IRE\n%s",
			spotmeter_formula == 1 ? "-1..101" : "0..108"
		);
	}
}

void hdmi_test_toggle(void* priv)
{
	ext_monitor_hdmi = !ext_monitor_hdmi;
}

int crop_offset = -40;
void crop_off_toggle(void* priv)
{
	crop_offset++;
}
void crop_off_toggle_rev(void* priv)
{
	crop_offset--;
}

static void
crop_off_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	int * draw_ptr = priv;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"crop offset : %d", crop_offset
	);
}

struct menu_entry zebra_menus[] = {
	{
		.priv		= &global_draw,
		.select		= global_draw_toggle,
		.display	= global_draw_display,
	},
	{
		.priv		= &hist_draw,
		.select		= menu_ternary_toggle,
		.select_auto = waveform_toggle,
		.display	= hist_display,
	},
	{
		.priv		= &zebra_draw,
		.select		= menu_ternary_toggle,
		.select_reverse = zebra_lo_toggle, 
		.select_auto = zebra_hi_toggle,
		.display	= zebra_draw_display,
	},
	{
		.priv		= &falsecolor_draw,
		.display	= falsecolor_display,
		.select		= menu_ternary_toggle,
		.select_reverse = menu_ternary_toggle_reverse, 
	},
	{
		.priv		= &cropmark_playback,
		.display	= crop_display,
		.select		= crop_toggle_forward,
		.select_reverse		= crop_toggle_reverse,
		.select_auto = menu_binary_toggle,
	},
	{
		.priv			= &spotmeter_draw,
		.select			= menu_ternary_toggle,
		.select_reverse = menu_ternary_toggle_reverse,
		.select_auto	= spotmeter_formula_toggle,
		.display		= spotmeter_menu_display,
	},
	{
		.priv			= &clearpreview,
		.display		= clearpreview_display,
		.select			= clearpreview_toggle,
		.select_reverse	= clearpreview_toggle_reverse,
	},
	{
		.priv			= &focus_peaking,
		.display		= focus_peaking_display,
		.select			= menu_ternary_toggle,
		.select_reverse = focus_peaking_adjust_color, 
		.select_auto    = focus_peaking_adjust_thr,
	},
	{
		.priv			= &focus_graph,
		.display		= focus_graph_display,
		.select			= menu_binary_toggle,
	},
	//~ {
		//~ .display		= crop_off_display,
		//~ .select			= crop_off_toggle,
		//~ .select_reverse = crop_off_toggle_rev, 
	//~ },
	
	//~ {
		//~ .priv = "[debug] HDMI test", 
		//~ .display = menu_print, 
		//~ .select = hdmi_test_toggle,
	//~ }
		//~ {
		//~ .priv = "[debug] dump vram", 
		//~ .display = menu_print, 
		//~ .select = dump_vram,
	//~ }
	//~ {
		//~ .priv		= &edge_draw,
		//~ .select		= menu_binary_toggle,
		//~ .display	= edge_display,
	//~ },
	//~ {
		//~ .priv		= &waveform_draw,
		//~ .select		= menu_binary_toggle,
		//~ .display	= waveform_display,
	//~ },
};

struct menu_entry dbg_menus[] = {
	{
		.priv		= &unified_loop,
		.select		= menu_ternary_toggle,
		.display	= unified_loop_display,
	},
	/*{
		.priv		= &zebra_density,
		.select		= menu_ternary_toggle,
		.display	= zebra_mode_display,
	},*/
	{
		.priv		= &use_hd_vram,
		.select		= menu_binary_toggle,
		.display	= use_hd_vram_display,
	},
	/*{
		.priv = &focus_peaking_debug,
		.select = menu_binary_toggle, 
		.display = focus_debug_display,
	}*/
};

struct menu_entry movie_menus[] = {
	{
		.priv		= &time_indicator,
		.select		= menu_quaternary_toggle,
		.select_reverse	= menu_quaternary_toggle_reverse,
		.display	= time_indicator_display,
	},
};

PROP_INT(PROP_ACTIVE_SWEEP_STATUS, sensor_cleaning);


#if 0
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
#endif

int movie_elapsed_time = 0;
int movie_elapsed_ticks = 0;
int rec_time_card = 0;
int rec_time_4gb = 0;

PROP_HANDLER(PROP_MVR_REC_START)
{
	crop_dirty = 10;
	recording = buf[0];
	if (!recording)
	{
		movie_elapsed_ticks = 0;
		movie_elapsed_time = 0;
		movie_elapsed_ticks = 0;
		rec_time_4gb = ticks_4gb; // this may need calibration
	}
	return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_REC_TIME)
{
	if (recording)
	{
		rec_time_card = buf[0]; // countdown, in seconds, showing time remaining until filling the card, assumming a fixed bitrate
		movie_elapsed_ticks++;  // if different bitrate is used, this is update more (or less) often
		rec_time_4gb--;         // countdown, shows time remaining until filling 4GB
	}
	return prop_cleanup(token, property);
}

void time_indicator_show()
{
	if (!recording) return;
	
	// time until filling the card, adjusted for actual bitrate
	int time_cardfill = rec_time_card * movie_elapsed_time / movie_elapsed_ticks;
	
	// time until 4 GB or filling the card, whichever comes sooner, adjusted for actual bitrate
	int time_4gb = MIN(rec_time_4gb, rec_time_card) * movie_elapsed_time / movie_elapsed_ticks;
	
	// what to display
	int dispvalue = time_indicator == 1 ? movie_elapsed_time :
					time_indicator == 2 ? time_cardfill :
					time_indicator == 3 ? time_4gb : 0;

	//bmp_printf(FONT_MED, 0, 180, "%d %d %d %d ", movie_elapsed_time, movie_elapsed_ticks, rec_time_card, rec_time_4gb);
	
	if (time_indicator)
	{
		bmp_printf(
			time_4gb < timecode_warning ? timecode_font : FONT_MED,
			timecode_x + 5 * fontspec_font(timecode_font)->width,
			timecode_y,
			"%4d:%02d",
			dispvalue / 60,
			dispvalue % 60
		);
	}
}

static void draw_movie_bars()
{
	//~ if (shooting_mode == SHOOTMODE_MOVIE && video_mode_resolution < 2)
	//~ {
		//~ bmp_fill( crop_black_border ? COLOR_BLACK : COLOR_BG, 0, 0, 960, 40 );
		//~ bmp_fill( crop_black_border ? COLOR_BLACK : COLOR_BG, 0, 440, 960, 40 );
	//~ }
}

PROP_HANDLER(PROP_LV_ACTION)
{
	crop_dirty = 10;
	return prop_cleanup( token, property );
}


void 
cropmark_draw(int del)
{
	if (!get_global_draw()) return;
	clrscr_mirror();
	bmp_ov_loc_size_t os;
	calc_ov_loc_size(&os);
	bmp_draw_scaled_ex(cropmarks, os.bmp_of_x, os.bmp_of_y, os.bmp_ex_x, os.bmp_ex_y, bvram_mirror, del);
}
static void
cropmark_redraw()
{
	if (cropmarks)
	{
		int del = lv_dispsize == 1 ? 0 : 1;
		cropmark_draw(del); // erase cropmarks in zoom mode
	}
	else
		clrscr_mirror();
}

// those functions will do nothing if called multiple times (it's safe to do this)
int _bmp_cleared = 0;
void bmp_on()
{
	if (!_bmp_cleared) call("MuteOff");
	_bmp_cleared = 1;
}
void bmp_off()
{
	if (_bmp_cleared) call("MuteOn");
	_bmp_cleared = 0;
}

int _lvimage_cleared = 0;
void lvimage_on()
{
	if (!_lvimage_cleared) call("MuteOffImage");
	_lvimage_cleared = 1;
}
void lvimage_off()
{
	if (_lvimage_cleared) call("MuteOnImage");
	_lvimage_cleared = 0;
}

int _display_is_off = 0;
void display_on()
{
	if (_display_is_off)
	{
		if (lv_drawn()) lvimage_on(); // might save a bit of power
		call("TurnOnDisplay");
		_display_is_off = 0;
	}
}
void display_off()
{
	if (!_display_is_off)
	{
		if (lv_drawn()) lvimage_off(); // might save a bit of power
		call("TurnOffDisplay");
		_display_is_off = 1;
	}
}


//this function is a mess... but seems to work
static void
zebra_task( void )
{
	DebugMsg( DM_MAGIC, 3, "Starting zebra_task");
    menu_add( "LiveV", zebra_menus, COUNT(zebra_menus) );
    menu_add( "Debug", dbg_menus, COUNT(dbg_menus) );
    menu_add( "Movie", movie_menus, COUNT(movie_menus) );

	msleep(2000);
	
	find_cropmarks();
	load_cropmark(crop_draw);
	int k;

	while(1)
	{
zebra_task_loop:
		k++;
		
		msleep(10); // safety msleep :)
		if (cropmarks && cropmark_playback && gui_state == GUISTATE_PLAYMENU)
		{
			cropmark_redraw();
			msleep(1000);
		}
		if (!lv_drawn()) { msleep(100); continue; }

		int fcp = falsecolor_displayed;
		falsecolor_displayed = (falsecolor_draw && ((!falsecolor_shortcutkey) || (falsecolor_shortcutkey && (dofpreview || FLASH_BTN_MOVIE_MODE))));
		if (fcp != falsecolor_displayed)
		{
			if (falsecolor_displayed) // first time displaying false color from shortcut key
			{
				// there's a beautiful message saying "This function is not available in movie mode"
				// but users want to get rid of this
				if (shooting_mode == SHOOTMODE_MOVIE && !recording)
				{
					bmp_fill(0, 0, 330, 720, 480-330);
					msleep(50);
					bmp_fill(0, 0, 330, 720, 480-330);
					cropmark_redraw();
				}
				else
				{
					clrscr_mirror();
				}
			}
			else // false color no longer displayed
			{
				cropmark_redraw();
			}
		}

		if (gui_menu_shown())
		{
			display_on();
			bmp_on();
			clrscr_mirror();
			while (gui_menu_shown()) msleep(100);
			crop_dirty = 1;
		}
		
		if (get_halfshutter_pressed()) display_on();

		// clear overlays on shutter halfpress
		if (clearpreview == 1 && get_halfshutter_pressed() && !dofpreview && !gui_menu_shown()) // preview image without any overlays
		{
			cropmark_redraw(); // short press... clear only zebra and focus assist and redraw cropmarks
			int i;
			for (i = 0; i < clearpreview_delay/10; i++)
			{
				msleep(10);
				if (!get_halfshutter_pressed() || dofpreview) goto zebra_task_loop;
			}
			
			bmp_off();
			while (get_halfshutter_pressed()) msleep(100);
			bmp_on();
		}
		else if (clearpreview == 2) // always clear overlays
		{ // in this mode, BMP & global_draw are disabled, but Canon code may draw on the screen
			if (gui_state == 0 && !gui_menu_shown() && !get_halfshutter_pressed() && !falsecolor_displayed && !LV_ADJUSTING_ISO) 
			{
				bmp_off();
			}
			else 
			{
				bmp_on();
				draw_zebra_and_focus();
			}
		}
		else if(!gui_menu_shown() && global_draw) // normal zebras
		{
			draw_zebra_and_focus();
		}
		else msleep(100); // nothing to do (idle), but keep it responsive

		if (global_draw && !gui_menu_shown())
		{
			if (k % 16 == 0)
			{
				if (hist_draw || waveform_draw)
				{
					struct vram_info * vram = get_yuv422_vram();
					hist_build(vram->vram, vram->width, vram->pitch);
				}
				if( hist_draw )
					hist_draw_image( hist_x, hist_y );
				if( waveform_draw )
					waveform_draw_image( 720 - WAVEFORM_WIDTH, 480 - WAVEFORM_HEIGHT - 50 );
			}


			if(spotmeter_draw && k % 4 == 0)
			{
				spotmeter_step();
			}
			if (crop_dirty)
			{
				crop_dirty--;
				if (!crop_dirty)
				{
					cropmark_redraw();
				}
			}
		}
	}
}


TASK_CREATE( "zebra_task", zebra_task, 0, 0x1f, 0x1000 );

void test_fps(int* x)
{
	int x0 = 0;
	int F = 0;
	int f = 0;
	bmp_printf(FONT_SMALL, 10, 100, "testing %x...", x);
	while(F < 500)
	{
		if (x0 != *x)
		{
			x0 = *x;
			fps_ticks++;
		}
		F++;
		msleep(1);
	}
	bmp_printf(FONT_SMALL, 10, 100, "testing done.");
	return;
}

static void
movie_clock_task( void )
{
	while(1)
	{
		msleep(1000);
		if (shooting_type == 4 && recording) 
		{
			movie_elapsed_time++;
			time_indicator_show();
		}
		
		//~ bmp_printf(FONT_MED, 10, 80, "%d fps", fps_ticks);
		fps_ticks = 0;
	}
}

TASK_CREATE( "movie_clock_task", movie_clock_task, 0, 0x19, 0x1000 );
