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
#define waveform_height			256
#define waveform_width			(720/2)

static int global_draw = 1;
CONFIG_INT( "global.draw", global_draw_bk, 1 );
CONFIG_INT( "zebra.draw",	zebra_draw,	2 );
CONFIG_INT( "zebra.level-hi",	zebra_level_hi,	245 );
CONFIG_INT( "zebra.level-lo",	zebra_level_lo,	10 );
CONFIG_INT( "zebra.delay",	zebra_delay,	1000 );
CONFIG_INT( "crop.draw",	crop_draw,	1 ); // index of crop file
CONFIG_INT( "crop.playback", cropmark_playback, 0);
CONFIG_INT( "falsecolor.draw", falsecolor_draw, 2);
CONFIG_INT( "falsecolor.shortcutkey", falsecolor_shortcutkey, 1);
int falsecolor_displayed = 0;

CONFIG_INT( "focus.peaking", focus_peaking, 0);
CONFIG_INT( "focus.peaking.thr", focus_peaking_pthr, 10); // 1%
CONFIG_INT( "focus.peaking.color", focus_peaking_color, 5); // R,G,B,C,M,Y,cc1,cc2
//~ int get_crop_black_border() { return crop_black_border; }

//~ CONFIG_INT( "edge.draw",	edge_draw,	0 );
//~ CONFIG_INT( "enable-liveview",	enable_liveview, 1 );
CONFIG_INT( "hist.draw",	hist_draw,	1 );
CONFIG_INT( "hist.x",		hist_x,		720 - hist_width - 4 );
CONFIG_INT( "hist.y",		hist_y,		100 );
//~ CONFIG_INT( "waveform.draw",	waveform_draw,	0 );
//~ CONFIG_INT( "waveform.x",	waveform_x,	720 - waveform_width );
//~ CONFIG_INT( "waveform.y",	waveform_y,	480 - 50 - waveform_height );
//~ CONFIG_INT( "waveform.bg",	waveform_bg,	0x26 ); // solid black
CONFIG_INT( "timecode.x",	timecode_x,	720 - 160 );
CONFIG_INT( "timecode.y",	timecode_y,	0 );
CONFIG_INT( "timecode.width",	timecode_width,	160 );
CONFIG_INT( "timecode.height",	timecode_height, 20 );
CONFIG_INT( "timecode.warning",	timecode_warning, 120 );
static unsigned timecode_font	= FONT(FONT_MED, COLOR_RED, COLOR_BG );

CONFIG_INT( "clear.preview", clearpreview, 1); // 2 is always
CONFIG_INT( "clear.preview.delay", clearpreview_delay, 1000); // ms

CONFIG_INT( "spotmeter.size",		spotmeter_size,	5 );
CONFIG_INT( "spotmeter.draw",		spotmeter_draw, 1 ); // 0 off, 1 on, 2 on without dots

PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
PROP_INT(PROP_DOF_PREVIEW_MAYBE, dofpreview);

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


int movie_elapsed_time = 0;
int movie_elapsed_ticks = 0;
int recording = 0;

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
						: *(uint16_t*)0x1300ce;
	_vram_info.pitch = _vram_info.width * 2;
	_vram_info.height = recording ? (video_mode_resolution == 0 ? 974 : 
									video_mode_resolution == 1 ? 580 : 
									video_mode_resolution == 2 ? 480 : 0)
						: *(uint16_t*)0x1300d0;

	struct vram_info * vram = &_vram_info;
	return vram;
}

struct vram_info * get_yuv422_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = YUV422_LV_BUFFER;
	_vram_info.width = 720;
	_vram_info.pitch = _vram_info.width * 2;
	_vram_info.height = 480;

	struct vram_info * vram = &_vram_info;
	return vram;
}

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
void
hist_build(void* vram, int width, int pitch)
{
    DebugMsg(DM_MAGIC, 3, "hist_build: %x, %d, %d", vram, width, pitch);
	uint32_t * 	v_row = (uint32_t*) vram;
	int x,y;

	hist_max = 0;

	// memset() causes err70?  Too much memory bandwidth?
	for( x=0 ; x<hist_width ; x++ )
		hist[x] = 0;
	//~ for( y=0 ; y<waveform_width ; y++ )
		//~ for( x=0 ; x<waveform_height ; x++ )
		//~ {
			//~ waveform[y][x] = 0;
			//~ asm( "nop\nnop\nnop\nnop\n" );
		//~ }

	for( y=1 ; y<480; y++, v_row += (pitch/4) )
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
			unsigned count = ++ (hist[ hist_level ]);
			if( hist_level && count > hist_max )
				hist_max = count;

			// Update the waveform plot
			//~ waveform[ (x * waveform_width) / width ][ (p * waveform_height) / 65536 ]++;
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
    DebugMsg(DM_MAGIC, 3, "***************** hist_draw_image **********************");
	uint8_t * const bvram = bmp_vram();

	// Align the x origin, just in case
	x_origin &= ~3;

	uint8_t * row = bvram + x_origin + y_origin * BMPPITCH;
	if( hist_max == 0 )
		hist_max = 1;

	unsigned i, y;

	for( i=0 ; i<hist_width ; i++ )
	{
		// Scale by the maximum bin value
		const uint32_t size = (hist[i] * hist_height) / hist_max;
		uint8_t * col = row + i;

		// vertical line up to the hist size
		for( y=hist_height ; y>0 ; y-- , col += BMPPITCH )
			*col = y > size ? COLOR_BG : (falsecolor_displayed ? false_colour[(i * 256 / hist_width) & 0xFF]: COLOR_WHITE);
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
#if 0
static void
waveform_draw_image(
	unsigned		x_origin,
	unsigned		y_origin
)
{
	// Ensure that x_origin is quad-word aligned
	x_origin &= ~3;

	uint8_t * const bvram = bmp_vram();
	unsigned pitch = BMPPITCH;
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
#endif

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
// thresholded edge detection
static void
draw_zebra_and_focus( void )
{
	if (!global_draw) return;
	
	fps_ticks++;
	
	if (falsecolor_displayed) 
	{
		if (falsecolor_draw == 3) aj_DisplayFalseColour_n_CalcHistogram(); 
		else if (falsecolor_draw == 2) draw_false_downsampled();
		else draw_false();
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
  		static int dirty_pixels[MAX_DIRTY_PIXELS];
  		static int dirty_pixels_num = 0;
  		static int very_dirty = 0;
  		bmp_ov_loc_size_t os;
  		calc_ov_loc_size(&os);
  		struct vram_info * hd_vram = get_yuv422_hd_vram();
  		uint8_t * const hdvram = UNCACHEABLE(hd_vram->vram);
		int hd_width  = hd_vram->width<<1;
  		int hd_height = hd_vram->height;
		int hd_pitch =  hd_vram->pitch;

		int bm_lv_y = (os.bmp_ex_y-os.bmp_ex_x*9/16);
		if((ext_monitor_hdmi || ext_monitor_rca) && !recording || !(ext_monitor_hdmi || ext_monitor_rca) && !recording){
			bm_lv_y>>=1;
		}
		os.bmp_ex_x>>=1; 
		os.bmp_of_x>>=1;

//		bmp_printf(FONT_MED, 30, 100, "HD %dx%d %d %d", hd_width, hd_height, os.bmp_of_y,  bm_lv_y);

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
  
  		int step = (recording ? 2 : 1);
		int off_cor = recording && (ext_monitor_hdmi || ext_monitor_rca);
		for( y = os.bmp_of_y + bm_lv_y; y < (os.bmp_ex_y+os.bmp_of_y-bm_lv_y); y+=2 ) {
			uint32_t * const hd_row = (uint32_t*)( hdvram + (y-os.bmp_of_y-(off_cor?bm_lv_y:0)) * hd_height/(os.bmp_ex_y-(off_cor?bm_lv_y+os.bmp_of_y:0)) * hd_pitch ); // 2 pixels
			int b_row_off = y * BMPPITCH;
			uint16_t * const b_row = (uint16_t*)( bvram + b_row_off );   // 2 pixels
			uint16_t * const m_row = (uint16_t*)( bvram_mirror + b_row_off );   // 2 pixels
  
  			uint32_t color_over = zebra_color_word_row(COLOR_RED, y);
  			uint32_t color_under = zebra_color_word_row(COLOR_BLUE, y);
  			uint32_t color_over_2 = zebra_color_word_row(COLOR_RED, y+1);
  			uint32_t color_under_2 = zebra_color_word_row(COLOR_BLUE, y+1);
  
			for ( x = os.bmp_of_x; x < (os.bmp_ex_x + os.bmp_of_x); x+=step ) {

				#define BP (b_row[x])
				#define MP (m_row[x])
				#define BN (b_row[x + (BMPPITCH>>1)])
				#define MN (m_row[x + (BMPPITCH>>1)])

				uint32_t pixel = hd_row[(x-os.bmp_of_x)*(hd_width>>2)/os.bmp_ex_x];

				uint16_t bp = BP;
				uint16_t mp = MP;
				uint16_t bn = BN;
				uint16_t mn = MN;
				
//				BP=(pixel&0xff)>>8 | (pixel&0xff000000)>>24;

				if (zd) {
					int zebra_done = 0;
					if (bp != 0 && bp != mp) { little_cleanup(b_row + x, m_row + x); zebra_done = 1; }
					if (bn != 0 && bn != mn) { little_cleanup(b_row + x + (BMPPITCH>>1), m_row + x + (BMPPITCH>>1)); zebra_done = 1; }
					if(!zebra_done) {
						uint32_t p0 = pixel & 0xFF00;
						if (p0 > zlh) {
							BP = MP = color_over;
							BN = MN = color_over_2;
						}
						else if (p0 < zll) {
							BP = MP = color_under;
							BN = MN = color_under_2;
						}
						else if (pixel) {
							BN = MN = BP = MP = 0;
						}
					}
  				}

				if(focus_peaking) {
					int32_t p0 = (pixel >> 24) & 0xFF;
					int32_t p1 = (pixel >>  8) & 0xFF;
					int32_t d = ABS(p0-p1);
					if (d < thr) continue;
					// executed for 1% of pixels
					if (n_over > MAX_DIRTY_PIXELS) { // threshold too low, abort
						thr = MIN(thr+2, 255);
						continue;
					}
					n_over++;

					int color = get_focus_color(thr, d);
					color = (color << 8) | color;   
					if ((bp == 0 || bp == mp) && (bn == 0 || bn == mn)) { // safe to draw
						BP = BN = MP = MN = color;
						if (dirty_pixels_num < MAX_DIRTY_PIXELS) {
							dirty_pixels[dirty_pixels_num++] = (x<<1) + b_row_off;
						}
					}
  				}
  				#undef MP
  				#undef BP
				#undef BN
				#undef MN
  			}
  		}
		bmp_printf(FONT_LARGE, 10, 50, "%d ", thr);
		if (1000 * n_over / os.bmp_ex_x<<1 * os.bmp_ex_y / step / 2 > focus_peaking_pthr) {
			thr++;
		} else {
			thr--;
		}
		int thr_min = (lens_info.iso > 1600 ? 15 : 10);
		thr = COERCE(thr, thr_min, 255);
  	}
}


// clear only zebra, focus assist and whatever else is in BMP VRAM mirror
static void
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
}

void
draw_false_downsampled( void )
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
		uint32_t * const v_row = (uint32_t*)( lvram + y * lvpitch );        // 2 pixel
		uint16_t * const b_row = (uint16_t*)( bvram + y * BMPPITCH);          // 2 pixel
		uint16_t * const m_row = (uint16_t*)( bvram_mirror + y * BMPPITCH );  // 2 pixel
		
		uint32_t* lvp; // that's a moving pointer through lv vram
		uint16_t* bp;  // through bmp vram
		uint16_t* mp;  // through mirror

		for (lvp = v_row + 100/2, bp = b_row + 100/2, mp = m_row + 100/2; lvp < v_row + (720-100)/2 ; lvp++, bp++, mp++)
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
			//~ &&  y <  waveform_y + waveform_height
			//~ &&  x >= waveform_x
			//~ &&  x <  waveform_x + waveform_width
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

static void clearpreview_setup(mode) // 0 = disable display, 1 = enable display
{
	if (mode == 0 && bmp_enabled && lv_drawn())
	{
		bmp_enabled = 0;
		global_draw = 0;
	}
	if (mode == 1)
	{
		bmp_enabled = 1;
		global_draw = global_draw_bk;
	}
}
static void
clearpreview_toggle( void * priv )
{
	int * ptr = priv;
	*ptr = mod(*ptr + 1, 3);
	if (*ptr != 2) clearpreview_setup(1);
}

static void
clearpreview_toggle_reverse( void * priv )
{
	int * ptr = priv;
	*ptr = mod(*ptr - 1, 3);
	clearpreview_setup(*ptr == 2 ? 0 : 1);
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
	if (!global_draw) bmp_fill(0, 0, 0, 720, 480);
	global_draw_bk = global_draw;
}

#define MAX_CROP_NAME_LEN 15
#define MAX_CROPMARKS 9
int num_cropmarks = 0;
char cropmark_names[MAX_CROPMARKS][MAX_CROP_NAME_LEN];
static void find_cropmarks()
{
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( "B:/CROPMKS", &file );
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
		if (s)
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
		//23456789012
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
		//23456789012
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
		//23456789012
		"Zebras      : %s, %d..%d",
		z == 1 ? "ON " : (z == 2 ? "Auto" : "OFF"),
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
		fc == 1 ? "Plain C" : fc == 2 ? "Lo-res C" : fc == 3 ? "ASM (AJ)" : "OFF",
		falsecolor_shortcutkey ? ", Key" : ""
	);
}
static void
falsecolor_toggle( void * priv )
{
	falsecolor_draw = mod(falsecolor_draw + 1, 4);
}
static void
falsecolor_toggle_reverse( void * priv )
{
	falsecolor_draw = mod(falsecolor_draw - 1, 4);
}
static void
falsecolor_key_toggle( void * priv )
{
	falsecolor_shortcutkey = !falsecolor_shortcutkey;
}

static void
focus_peaking_display( void * priv, int x, int y, int selected )
{
	unsigned f = *(unsigned*) priv;
	if (f)
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Focus Peak  : ON, %d.%d, %s", 
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
		//23456789012
		"Cropmk%s(%d/%d): %s%s",
		 (cropmark_playback ? "P" : "s"),
		 index, num_cropmarks,
		 index  ? cropmark_names[index-1] : "OFF",
		 (cropmarks || !index) ? "" : "!" // ! means error
	);
}


static void
edge_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Edgedetect  : %s",
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
		"Histogram   : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}

static void
global_draw_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Global Draw : %s",
		global_draw_bk ? "ON " : "OFF"
	);
}

static void
waveform_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Waveform    : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
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
		(mode == 2 ? "Always" :
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
	int * draw_ptr = priv;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Spotmeter   : %s",
		(*draw_ptr == 0) ? "OFF   " : (*draw_ptr == 1 ? "Percent" : *draw_ptr == 2 ? "IRE (AJ)" : "IRE (Piers)")
	);
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

PROP_INT(PROP_HOUTPUT_TYPE, lv_disp_mode);

void spotmeter_step()
{
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
	const unsigned		scaled = (100 * sum) / 65536;
	
	// spotmeter color: 
	// black on transparent, if brightness > 60%
	// white on transparent, if brightness < 50%
	// previous value otherwise
	
	// if false color is active, draw white on semi-transparent gray
	
	static int fg = 0;
	if (scaled > 60) fg = COLOR_BLACK;
	if (scaled < 50 || falsecolor_displayed) fg = COLOR_WHITE;
	int bg = falsecolor_displayed ? COLOR_BG : 0;

	int xc = (ext_monitor_hdmi && !recording) ? 480 : 360 - 2 * font_med.width;
	int yc = (ext_monitor_hdmi && !recording) ? 270 : 240 - font_med.height/2;
	if (spotmeter_draw == 1)
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
		int ire_piers = (((int)sum >> 8) - 16) * (100-7) / 219 + 7;  // formula from Piers: (16...235) -> (7.5...100)
		int ire = spotmeter_draw == 2 ? ire_aj : ire_piers;
		
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
			spotmeter_draw == 2 ? "AJ" : "Piers"
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
		.select		= menu_binary_toggle,
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
		.select		= falsecolor_toggle,
		.select_reverse = falsecolor_toggle_reverse, 
		.select_auto = falsecolor_key_toggle,
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
		.select			= menu_quaternary_toggle,
		.select_reverse = menu_quaternary_toggle_reverse,
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
		.select			= menu_binary_toggle,
		.select_reverse = focus_peaking_adjust_color, 
		.select_auto    = focus_peaking_adjust_thr,
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

PROP_HANDLER(PROP_MVR_REC_START)
{
	crop_dirty = 10;
	recording = buf[0];
	if (!recording)
	{
		movie_elapsed_ticks = 0;
		movie_elapsed_time = 0;
	}
	return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_REC_TIME )
{
	uint32_t value = buf[0];
	if (shooting_type == 4 && recording) // movie mode
	{
		movie_elapsed_ticks++;
		
		//~ bmp_printf(FONT_MED, 30,30, "ticks=%d, time=%d", movie_elapsed_ticks, movie_elapsed_time);
		value = value * movie_elapsed_time / movie_elapsed_ticks;
		bmp_printf(
			value < timecode_warning ? timecode_font : FONT_MED,
			timecode_x + 5 * fontspec_font(timecode_font)->width,
			timecode_y,
			"%4d:%02d",
			value / 60,
			value % 60
		);
	}
	else movie_elapsed_ticks = 0;
	return prop_cleanup( token, property );
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


//this function is a mess... but seems to work
static void
zebra_task( void )
{
	DebugMsg( DM_MAGIC, 3, "Starting zebra_task");
    menu_add( "Video", zebra_menus, COUNT(zebra_menus) );


	msleep(2000);
	set_global_draw(global_draw_bk);
	find_cropmarks();
	load_cropmark(crop_draw);
	int k;

	while(1)
	{
zebra_task_loop:
		k++;

		msleep(10); // safety msleep :)
		if (cropmark_playback && gui_state == GUISTATE_PLAYMENU)
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
			}
			else // 
			{
				cropmark_redraw();
			}
		}

		if (gui_menu_shown())
		{
			clrscr_mirror();
			while (gui_menu_shown()) msleep(100);
			crop_dirty = 1;
		}

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

			clrscr();         // long press... clear everything
			clearpreview_setup(0);
			while (get_halfshutter_pressed()) msleep(100);
			clearpreview_setup(1);
			crop_dirty = 1;
		}
		else if (clearpreview == 2 && !gui_menu_shown()) // always clear overlays
		{ // in this mode, BMP & global_draw are disabled, but Canon code may draw on the screen
			if (gui_state == 0)
			{
				msleep(200);
				if (!lv_drawn() || gui_state != 0) continue;
				bmp_enabled = 1;
				clrscr();
				//~ draw_movie_bars();
				clearpreview_setup(0);
				crop_dirty = 1;
				msleep(200);
			}
			else
			{
				bmp_enabled = 1; // Quick menu => enable drawings
				global_draw = 1;
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
			if( hist_draw  && k % 16 == 0)
			{
				struct vram_info * vram = get_yuv422_vram();
				hist_build(vram->vram, vram->width, vram->pitch);
				//~ int off = (hist_x > 350 && ext_monitor_hdmi && !recording ? 200 : 200); // if hist is in the right half, anchor it to the right edge
				hist_draw_image( hist_x, hist_y );
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
		if (shooting_type == 4 && recording) movie_elapsed_time++;
		
		//~ bmp_printf(FONT_MED, 10, 80, "%d fps", fps_ticks);
		fps_ticks = 0;
	}
}

TASK_CREATE( "movie_clock_task", movie_clock_task, 0, 0x19, 0x1000 );
