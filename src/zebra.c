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

//~ #if 1
#ifdef CONFIG_50D
#define CONFIG_KILL_FLICKER // this will block all Canon drawing routines when the camera is idle 
#endif                      // but it will display ML graphics

#ifdef CONFIG_1100D
#include "disable-this-module.h"
#endif

int lv_paused = 0;

void waveform_init();
void histo_init();
void do_disp_mode_change();
void show_overlay();
void transparent_overlay_from_play();
void transparent_overlay_offset_clear(void* priv);
void draw_histogram_and_waveform();
void schedule_transparent_overlay();
void lens_display_set_dirty();
void defish_draw();

//~ static struct bmp_file_t * cropmarks_array[3] = {0};
static struct bmp_file_t * cropmarks = 0;

#define hist_height			64
#define hist_width			128
#define WAVEFORM_WIDTH 180
#define WAVEFORM_HEIGHT 120
#define WAVEFORM_FACTOR (1 << (waveform_draw - 1)) // 1, 2 or 4
#define WAVEFORM_OFFSET (waveform_draw <= 2 ? 60 : 0)

#define BVRAM_MIRROR_SIZE (BMPPITCH*540)

CONFIG_INT("lv.disp.profiles", disp_profiles_0, 1);

static CONFIG_INT("disp.mode", disp_mode, 0);
static CONFIG_INT("disp.mode.aaa", disp_mode_a, 0x285041);
static CONFIG_INT("disp.mode.bbb", disp_mode_b, 0x295001);
static CONFIG_INT("disp.mode.ccc", disp_mode_c,  0x88090);
static CONFIG_INT("disp.mode.xxx", disp_mode_x, 0x2c5051);

       CONFIG_INT( "transparent.overlay", transparent_overlay, 0);
static CONFIG_INT( "transparent.overlay.x", transparent_overlay_offx, 0);
static CONFIG_INT( "transparent.overlay.y", transparent_overlay_offy, 0);
int transparent_overlay_hidden = 0;

static CONFIG_INT( "global.draw", 	global_draw, 1 );
static CONFIG_INT( "zebra.draw",	zebra_draw,	0 );
static CONFIG_INT( "zebra.level-hi",	zebra_level_hi,	245 );
static CONFIG_INT( "zebra.level-lo",	zebra_level_lo,	10 );
       CONFIG_INT( "zebra.nrec",	zebra_nrec,	0 );
static CONFIG_INT( "crop.draw",	crop_draw,	0 ); // index of crop file
       CONFIG_INT( "crop.movieonly", cropmark_movieonly, 1);
static CONFIG_INT( "falsecolor.draw", falsecolor_draw, 0);
static CONFIG_INT( "falsecolor.palette", falsecolor_palette, 0);
static CONFIG_INT( "zoom.overlay.mode", zoom_overlay_mode, 0);
static CONFIG_INT( "zoom.overlay.size", zoom_overlay_size, 4);
static CONFIG_INT( "zoom.overlay.pos", zoom_overlay_pos, 1);
static CONFIG_INT( "zoom.overlay.split", zoom_overlay_split, 0);
static CONFIG_INT( "zoom.overlay.split.zerocross", zoom_overlay_split_zerocross, 1);
int get_zoom_overlay_mode() 
{ 
	if (!get_global_draw()) return 0;
	if (is_movie_mode() && video_mode_resolution != 0) return 0;
	return zoom_overlay_mode;
}
int get_zoom_overlay_z() 
{ 
	if (!get_global_draw()) return 0;
	if (is_movie_mode() && video_mode_resolution != 0) return 0;
	return zoom_overlay_mode == 1 || zoom_overlay_mode == 2;
}

int zoom_overlay = 0;
int zoom_overlay_countdown = 0;
int get_zoom_overlay() 
{ 
	if (!get_global_draw()) return 0;
	return zoom_overlay;
}

int zoom_overlay_dirty = 0;

static CONFIG_INT( "focus.peaking", focus_peaking, 0);
static CONFIG_INT( "focus.peaking.thr", focus_peaking_pthr, 10); // 1%
static CONFIG_INT( "focus.peaking.color", focus_peaking_color, 7); // R,G,B,C,M,Y,cc1,cc2

//~ static CONFIG_INT( "focus.graph", focus_graph, 0);
//~ int get_crop_black_border() { return crop_black_border; }

//~ static CONFIG_INT( "edge.draw",	edge_draw,	0 );
static CONFIG_INT( "hist.draw",	hist_draw,	0 );
//~ static CONFIG_INT( "hist.x",		hist_x,		720 - hist_width - 4 );
//~ static CONFIG_INT( "hist.y",		hist_y,		100 );
static CONFIG_INT( "waveform.draw",	waveform_draw,	0 );
//~ static CONFIG_INT( "waveform.x",	waveform_x,	720 - WAVEFORM_WIDTH );
//~ static CONFIG_INT( "waveform.y",	waveform_y,	480 - 50 - WAVEFORM_WIDTH );
static CONFIG_INT( "waveform.bg",	waveform_bg,	0x26 ); // solid black

static CONFIG_INT( "clear.preview", clearscreen, 1); // 2 is always
static CONFIG_INT( "clear.preview.delay", clearscreen_delay, 1000); // ms

static CONFIG_INT( "spotmeter.size",		spotmeter_size,	5 );
static CONFIG_INT( "spotmeter.draw",		spotmeter_draw, 1 );
static CONFIG_INT( "spotmeter.formula",		spotmeter_formula, 0 ); // 0 percent, 1 IRE AJ, 2 IRE Piers

//~ static CONFIG_INT( "unified.loop", unified_loop, 2); // temporary; on/off/auto
//~ static CONFIG_INT( "zebra.density", zebra_density, 0); 
//~ static CONFIG_INT( "hd.vram", use_hd_vram, 0); 

CONFIG_INT("idle.display.turn_off.after", idle_display_turn_off_after, 0); // this also enables power saving for intervalometer
static CONFIG_INT("idle.display.dim.after", idle_display_dim_after, 0);
static CONFIG_INT("idle.display.gdraw_off.after", idle_display_global_draw_off_after, 0);
static CONFIG_INT("idle.rec", idle_rec, 0);


int crop_redraw_flag = 0; // redraw cropmarks now
int crop_dirty = 0;       // redraw cropmarks after some time (unit: 0.1s)
int clearscreen_countdown = 20;

void ChangeColorPaletteLV(int x)
{
	//~ #ifndef CONFIG_50D
	//~ if (zebra_should_run())
		//~ GMT_LOCK( if (zebra_should_run()) ChangeColorPalette(x); )
	//~ #endif
}

//~ int recording = 0;

uint8_t false_colour[][256] = {
	{0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
	{0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x26, 0x26, 0x27, 0x27, 0x27, 0x27, 0x27, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x29, 0x29, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x31, 0x31, 0x31, 0x31, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x33, 0x33, 0x33, 0x33, 0x33, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x35, 0x35, 0x35, 0x35, 0x35, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x37, 0x37, 0x37, 0x37, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x39, 0x39, 0x39, 0x39, 0x39, 0x3A, 0x3A, 0x3A, 0x3A, 0x3A, 0x3A, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x43, 0x43, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x45, 0x45, 0x45, 0x45, 0x45, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x47, 0x47, 0x47, 0x47, 0x47, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
	{0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
	{0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
	{0x0E, 0x26, 0x30, 0x26, 0x30, 0x26, 0x30, 0x26, 0x30, 0x27, 0x31, 0x27, 0x31, 0x27, 0x31, 0x27, 0x31, 0x28, 0x32, 0x28, 0x32, 0x28, 0x32, 0x28, 0x32, 0x29, 0x33, 0x29, 0x33, 0x29, 0x33, 0x29, 0x33, 0x2a, 0x34, 0x2a, 0x34, 0x2a, 0x34, 0x2a, 0x34, 0x2a, 0x35, 0x2b, 0x35, 0x2b, 0x35, 0x2b, 0x35, 0x2b, 0x36, 0x2c, 0x36, 0x2c, 0x36, 0x2c, 0x36, 0x2c, 0x37, 0x2d, 0x37, 0x2d, 0x37, 0x2d, 0x37, 0x2d, 0x38, 0x2e, 0x38, 0x2e, 0x38, 0x2e, 0x38, 0x2e, 0x38, 0x2f, 0x39, 0x2f, 0x39, 0x2f, 0x39, 0x2f, 0x39, 0x30, 0x3a, 0x30, 0x3a, 0x30, 0x3a, 0x30, 0x3a, 0x31, 0x3b, 0x31, 0x3b, 0x31, 0x3b, 0x31, 0x3b, 0x32, 0x3c, 0x32, 0x3c, 0x32, 0x3c, 0x32, 0x3c, 0x33, 0x3d, 0x33, 0x3d, 0x33, 0x3d, 0x33, 0x3d, 0x33, 0x3e, 0x34, 0x3e, 0x34, 0x3e, 0x34, 0x3e, 0x34, 0x3f, 0x35, 0x3f, 0x35, 0x3f, 0x35, 0x3f, 0x35, 0x40, 0x36, 0x40, 0x36, 0x40, 0x36, 0x40, 0x36, 0x41, 0x37, 0x41, 0x37, 0x41, 0x37, 0x41, 0x37, 0x41, 0x38, 0x42, 0x38, 0x42, 0x38, 0x42, 0x38, 0x42, 0x39, 0x43, 0x39, 0x43, 0x39, 0x43, 0x39, 0x43, 0x3a, 0x44, 0x3a, 0x44, 0x3a, 0x44, 0x3a, 0x44, 0x3b, 0x45, 0x3b, 0x45, 0x3b, 0x45, 0x3b, 0x45, 0x3c, 0x46, 0x3c, 0x46, 0x3c, 0x46, 0x3c, 0x46, 0x3c, 0x47, 0x3d, 0x47, 0x3d, 0x47, 0x3d, 0x47, 0x3d, 0x48, 0x3e, 0x48, 0x3e, 0x48, 0x3e, 0x48, 0x3e, 0x49, 0x3f, 0x49, 0x3f, 0x49, 0x3f, 0x49, 0x3f, 0x4a, 0x40, 0x4a, 0x40, 0x4a, 0x40, 0x4a, 0x40, 0x4a, 0x41, 0x4b, 0x41, 0x4b, 0x41, 0x4b, 0x41, 0x4b, 0x42, 0x4c, 0x42, 0x4c, 0x42, 0x4c, 0x42, 0x4c, 0x43, 0x4d, 0x43, 0x4d, 0x43, 0x4d, 0x43, 0x4d, 0x44, 0x4e, 0x44, 0x4e, 0x44, 0x4e, 0x44, 0x4e, 0x08},
};

void crop_set_dirty(int value)
{
	crop_dirty = MAX(crop_dirty, value);
}

PROP_HANDLER(PROP_USBRCA_MONITOR)
{
	static int first_time = 1;
	if (!first_time) redraw_after(2000);
	first_time = 0;
	return prop_cleanup( token, property );
}
PROP_HANDLER(PROP_HDMI_CHANGE)
{
	redraw_after(4000);
	return prop_cleanup( token, property );
}

volatile int lcd_position = 0;
volatile int display_dont_mirror_dirty;
PROP_HANDLER(PROP_LCD_POSITION)
{
	if (lcd_position != (int)buf[0]) display_dont_mirror_dirty = 1;
	lcd_position = buf[0];
	redraw_after(100);
	return prop_cleanup( token, property );
}

/*int gui_state;
PROP_HANDLER(PROP_GUI_STATE) {
	gui_state = buf[0];
	if (gui_state == GUISTATE_IDLE) crop_set_dirty(40);
	return prop_cleanup( token, property );
}*/

PROP_HANDLER( PROP_LV_AFFRAME ) {
	afframe_set_dirty();
	return prop_cleanup( token, property );
}


// how to use a config setting in more than one file?!
//extern int* p_cfg_draw_meters;

int idle_globaldraw_disable = 0;

int get_global_draw() // menu setting, or off if 
{
	return global_draw && !idle_globaldraw_disable && !sensor_cleaning && bmp_is_on() && tft_status == 0 && recording != 1 && !lv_paused && !(recording && !lv);
}

int get_global_draw_setting() // whatever is set in menu
{
	return global_draw;
}

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

int hist_get_percentile_level(int percentile)
{
	int total = 0;
	int i;
	for( i=0 ; i < hist_width ; i++ )
		total += hist[i];
	
	int thr = total * percentile / 100;  // 50% => median
	int n = 0;
	for( i=0 ; i < hist_width ; i++ )
	{
		n += hist[i];
		if (n >= thr)
			return i * 100 / hist_width;
	}
	return -1; // invalid argument?
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

#define ZEBRA_COLOR_WORD_SOLID(x) ( (x) | (x)<<8 | (x)<<16 | (x)<<24 )
static int zebra_rgb_color(int underexposed, int clipR, int clipG, int clipB, int y)
{
	if (underexposed) return zebra_color_word_row(79, y);
	
	switch ((clipR ? 0 : 1) |
			(clipG ? 0 : 2) |
			(clipB ? 0 : 4))
	{
		case 0b000: return zebra_color_word_row(COLOR_BLACK, y);
		case 0b001: return zebra_color_word_row(COLOR_RED,1);
		case 0b010: return zebra_color_word_row(7, 1); // green
		case 0b100: return zebra_color_word_row(9, 1); // strident blue
		case 0b011: return y&2 ? 0 : ZEBRA_COLOR_WORD_SOLID(COLOR_YELLOW);
		case 0b110: return y&2 ? 0 : ZEBRA_COLOR_WORD_SOLID(5); // cyan
		case 0b101: return y&2 ? 0 : ZEBRA_COLOR_WORD_SOLID(14); // magenta
		case 0b111: return 0;
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
	unsigned		y_origin,
	int highlight_level
)
{
	if (!PLAY_MODE)
	{
		if (!expsim) return;
	}
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;

	// Align the x origin, just in case
	x_origin &= ~3;

	uint8_t * row = bvram + x_origin + y_origin * BMPPITCH;
	if( hist_max == 0 )
		hist_max = 1;

	unsigned i, y;
	
	if (highlight_level >= 0) 
		highlight_level = highlight_level * hist_width / 256;

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
			if (highlight_level >= 0)
			{
				int hilight = ABS(i-highlight_level) <= 1;
				*col = y > size + hilight ? COLOR_BG : (hilight ? COLOR_RED : COLOR_WHITE);
			}
			else if (hist_draw == 2) // RGB
				*col = hist_rgb_color(y, sizeR, sizeG, sizeB);
			else
				*col = y > size ? COLOR_BG : (falsecolor_draw ? false_colour[falsecolor_palette][(i * 256 / hist_width) & 0xFF]: COLOR_WHITE);
		}
	}
}

void hist_highlight(int level)
{
	hist_draw_image( os.x_max - hist_width, os.y0 + 100, level );
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
	if (!PLAY_MODE)
	{
		if (!expsim) return;
	}
	waveform_init();
	// Ensure that x_origin is quad-word aligned
	x_origin &= ~3;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	unsigned pitch = BMPPITCH;
	uint8_t * row = bvram + x_origin + y_origin * pitch;
	if( hist_max == 0 )
		hist_max = 1;

	int i, y;

	// vertical line up to the hist size
	for( y=WAVEFORM_HEIGHT*WAVEFORM_FACTOR-1 ; y>0 ; y-- )
	{
		uint32_t pixel = 0;

		for( i=0 ; i<WAVEFORM_WIDTH*WAVEFORM_FACTOR ; i++ )
		{

			uint32_t count = waveform[ i / WAVEFORM_FACTOR ][ y / WAVEFORM_FACTOR ];
			// Scale to a grayscale
			count = (count * 42) / 128;
			if( count > 42 )
				count = 0x0F;
			else
			if( count >  0 )
				count += 0x26;
			else
			// Draw a series of colored scales
			if( y == (WAVEFORM_HEIGHT*WAVEFORM_FACTOR*1)/4 )
				count = COLOR_BLUE;
			else
			if( y == (WAVEFORM_HEIGHT*WAVEFORM_FACTOR*2)/4 )
				count = 0xE; // pink
			else
			if( y == (WAVEFORM_HEIGHT*WAVEFORM_FACTOR*3)/4 )
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
		}

		row += pitch;
	}
}


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

int tic()
{
	struct tm now;
	LoadCalendarFromRTC(&now);
	return now.tm_sec + now.tm_min * 60 + now.tm_hour * 3600 + now.tm_mday * 3600 * 24;
}

#if CONFIG_DEBUGMSG
void card_benchmark_wr(int bufsize, int K, int N)
{
	FIO_RemoveFile(CARD_DRIVE "bench.tmp");
	msleep(1000);
	int n = 0x10000000 / bufsize;
	{
		FILE* f = FIO_CreateFile(CARD_DRIVE "bench.tmp");
		int t0 = tic();
		int i;
		for (i = 0; i < n; i++)
		{
			uint32_t start = UNCACHEABLE(i * bufsize);
			bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Writing: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
			FIO_WriteFile( f, (const void *) start, bufsize );
		}
		FIO_CloseFile(f);
		int t1 = tic();
		int speed = 2560 / (t1 - t0);
		console_printf("Write speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
	}
	SW1(1,100);
	SW1(0,100);
	msleep(1000);
	if (bufsize > 1024*1024) console_printf("read test skipped: buffer=%d\n", bufsize);
	else
	{
		void* buf = AllocateMemory(bufsize);
		if (buf)
		{
			FILE* f = FIO_Open(CARD_DRIVE "bench.tmp", O_RDONLY | O_SYNC);
			int t0 = tic();
			int i;
			for (i = 0; i < n; i++)
			{
				bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Reading: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
				FIO_ReadFile(f, UNCACHEABLE(buf), bufsize );
			}
			FIO_CloseFile(f);
			FreeMemory(buf);
			int t1 = tic();
			int speed = 2560 / (t1 - t0);
			console_printf("Read speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
		}
		else
		{
			console_printf("malloc error: buffer=%d\n", bufsize);
		}
	}

	FIO_RemoveFile(CARD_DRIVE "bench.tmp");
	msleep(1000);
	SW1(1,100);
	SW1(0,100);
}

void card_benchmark()
{
	console_printf("Card benchmark starting...\n");
	card_benchmark_wr(16384, 1, 3);
	card_benchmark_wr(131072, 2, 3);
	card_benchmark_wr(16777216, 3, 3);
	console_printf("Card benchmark done.\n");
	console_show();
}

int card_benchmark_start = 0;
void card_benchmark_schedule()
{
	gui_stop_menu();
	card_benchmark_start = 1;
}
#endif

static void dump_vram()
{
	dump_big_seg(0, CARD_DRIVE "0.bin");
	//dump_big_seg(4, CARD_DRIVE "4.bin");
	dump_seg(0x1000, 0x100000, CARD_DRIVE "ram.bin");
/*	dump_seg(0x40000000, 0x1000000, CARD_DRIVE "0.bin");
	dump_seg(0x41000000, 0x1000000, CARD_DRIVE "1.bin");
	dump_seg(0x42000000, 0x1000000, CARD_DRIVE "2.bin");
	dump_seg(0x43000000, 0x1000000, CARD_DRIVE "3.bin");
	dump_seg(0x44000000, 0x1000000, CARD_DRIVE "4.bin");
	dump_seg(0x45000000, 0x1000000, CARD_DRIVE "5.bin");
	dump_seg(0x46000000, 0x1000000, CARD_DRIVE "6.bin");
	dump_seg(0x47000000, 0x1000000, CARD_DRIVE "7.bin");
	dump_seg(0x48000000, 0x1000000, CARD_DRIVE "8.bin");
	dump_seg(0x49000000, 0x1000000, CARD_DRIVE "9.bin");
	dump_seg(0x4A000000, 0x1000000, CARD_DRIVE "A.bin");
	dump_seg(0x4B000000, 0x1000000, CARD_DRIVE "B.bin");
	dump_seg(0x4C000000, 0x1000000, CARD_DRIVE "C.bin");
	dump_seg(0x4D000000, 0x1000000, CARD_DRIVE "D.bin");
	dump_seg(0x4E000000, 0x1000000, CARD_DRIVE "E.bin");
	dump_seg(0x4F000000, 0x1000000, CARD_DRIVE "F.bin");
	dump_seg(0x50000000, 0x1000000, CARD_DRIVE "10.bin");
	dump_seg(0x51000000, 0x1000000, CARD_DRIVE "11.bin");
	dump_seg(0x52000000, 0x1000000, CARD_DRIVE "12.bin");
	dump_seg(0x53000000, 0x1000000, CARD_DRIVE "13.bin");
	dump_seg(0x54000000, 0x1000000, CARD_DRIVE "14.bin");
	dump_seg(0x55000000, 0x1000000, CARD_DRIVE "15.bin");
	dump_seg(0x56000000, 0x1000000, CARD_DRIVE "16.bin");
	dump_seg(0x57000000, 0x1000000, CARD_DRIVE "17.bin");
	dump_seg(0x58000000, 0x1000000, CARD_DRIVE "18.bin");
	dump_seg(0x59000000, 0x1000000, CARD_DRIVE "19.bin");
	dump_seg(0x5A000000, 0x1000000, CARD_DRIVE "1A.bin");
	dump_seg(0x5B000000, 0x1000000, CARD_DRIVE "1B.bin");
	dump_seg(0x5C000000, 0x1000000, CARD_DRIVE "1C.bin");
	dump_seg(0x5D000000, 0x1000000, CARD_DRIVE "1D.bin");
	dump_seg(0x5E000000, 0x1000000, CARD_DRIVE "1E.bin");
	dump_seg(0x5F000000, 0x1000000, CARD_DRIVE "1F.bin");*/
	//~ dump_seg(YUV422_IMAGE_BUFFER, 1920*1080*2, CARD_DRIVE "VRAM.BIN");
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
		waveform = AllocateMemory(WAVEFORM_WIDTH * sizeof(uint32_t*));
		if (!waveform) fail("Waveform malloc failed");
		int i;
		for (i = 0; i < WAVEFORM_WIDTH; i++) {
			waveform[i] = AllocateMemory(WAVEFORM_HEIGHT * sizeof(uint32_t));
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
		bvram_mirror = AllocateMemory(BVRAM_MIRROR_SIZE);
		if (!bvram_mirror) 
		{	
			bmp_printf(FONT_MED, 30, 30, "Failed to allocate BVRAM mirror");
			return;
		}
		bzero32(bvram_mirror, BVRAM_MIRROR_SIZE);
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

static void little_cleanup(void* BP, void* MP)
{
	uint8_t* bp = BP; uint8_t* mp = MP;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
	bp++; mp++;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
	bp++; mp++;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
	bp++; mp++;
	if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
}


int zebra_color_word_row(int c, int y)
{
	if (!c) return 0;
	
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


static int* dirty_pixels = 0;
static int dirty_pixels_num = 0;
//~ static int very_dirty = 0;

int zebra_color_word_row_thick(int c, int y)
{
	//~ return zebra_color_word_row(c,y);
	if (!c) return 0;
	
	uint32_t cw = 0;
	switch(y % 4)
	{
		case 0:
			cw  = c  | c  << 8 | c << 16;
			break;
		case 1:
			cw  = c << 8 | c << 16 | c << 24;
			break;
		case 2:
			cw = c  << 16 | c << 24 | c;
			break;
		case 3:
			cw  = c  << 24 | c | c << 8;
			break;
	}
	return cw;
}


int focus_peaking_debug = 0;

// thresholded edge detection
static void
draw_zebra_and_focus( int Z, int F )
{
	if (lv_dispsize != 1) return;
	//~ if (vram_width > 720) return;

/*	int Zd = should_draw_zoom_overlay();
	static int Zdp = 0;
	if (Zd && !Zdp) clrscr_mirror();
	Zdp = Zd;
	if (Zd) msleep(100); // reduce frame rate when zoom overlay is active
	*/
	
	//~ if (unified_loop == 1) { draw_zebra_and_focus_unified(); return; }
	//~ if (unified_loop == 2 && (ext_monitor_hdmi || ext_monitor_rca || (is_movie_mode() && video_mode_resolution != 0)))
		//~ { draw_zebra_and_focus_unified(); return; }
	
	if (!global_draw) return;
	
	// HD to LV coordinate transform:
	// non-record: 1056 px: 1.46 ratio (yuck!)
	// record: 1720: 2.38 ratio (yuck!)
	
	// How to scan?
	// Scan the HD vram and do ratio conversion only for the 1% pixels displayed

	//~ bvram_mirror_init();

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;
	//~ int BMPPITCH = bmp_pitch();
	int y;

	if (F && focus_peaking)
	{
		// clear previously written pixels
		#define MAX_DIRTY_PIXELS 5000
  		if (!dirty_pixels) dirty_pixels = AllocateMemory(MAX_DIRTY_PIXELS * sizeof(int));
  		if (!dirty_pixels) return;
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

		#ifdef CONFIG_600D
		int rec_off = (is_movie_mode() ? 90 : 0);
		#else
		int rec_off = (recording ? 90 : 0);
		#endif
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
		//~ bmp_printf(FONT_LARGE, 10, 50, "%d ", thr);
		if (1000 * n_over / n_total > (int)focus_peaking_pthr) thr++;
		else thr--;
		
		int thr_min = (lens_info.iso > 1600 ? 15 : 10);
		thr = COERCE(thr, thr_min, 255);
	}
	
	int zd = Z && zebra_draw && (expsim || PLAY_MODE) && (!zebra_nrec || !recording); // when to draw zebras
	if (zd)
	{
		int zlh = zebra_level_hi;
		int zll = zebra_level_lo;

		uint8_t * const lvram = get_yuv422_vram()->vram;
		int lvpitch = hdmi_code == 5 ? 3840 : 1440;
		int lvp_step_x = hdmi_code == 5 ? 4 : 2;
		int lvp_step_y = hdmi_code == 5 ? 2 : 1;
		int lvheight = hdmi_code == 5 ? 540 : 480;
		for( y = 40; y < lvheight - 40; y += 2 )
		{
			uint32_t color_over = zebra_color_word_row(COLOR_RED, y);
			uint32_t color_under = zebra_color_word_row(COLOR_BLUE, y);
			uint32_t color_over_2 = zebra_color_word_row(COLOR_RED, y+1);
			uint32_t color_under_2 = zebra_color_word_row(COLOR_BLUE, y+1);
			
			uint32_t * const v_row = (uint32_t*)( lvram + y * lvpitch * lvp_step_y );          // 2 pixels
			uint32_t * const b_row = (uint32_t*)( bvram + y * BMPPITCH);          // 4 pixels
			uint32_t * const m_row = (uint32_t*)( bvram_mirror + y * BMPPITCH );  // 4 pixels
			
			uint32_t* lvp; // that's a moving pointer through lv vram
			uint32_t* bp;  // through bmp vram
			uint32_t* mp;  // through mirror

			for (lvp = v_row, bp = b_row, mp = m_row ; lvp < v_row + lvpitch/4 ; lvp += lvp_step_x, bp++, mp++)
			{
				#define BP (*bp)
				#define MP (*mp)
				#define BN (*(bp + BMPPITCH/4))
				#define MN (*(mp + BMPPITCH/4))
				if (BP != 0 && BP != MP) { little_cleanup(bp, mp); continue; }
				if (BN != 0 && BN != MN) { little_cleanup(bp + BMPPITCH/4, mp + BMPPITCH/4); continue; }
				
				if (zebra_draw == 2) // rgb
				{
					uint32_t pixel = *lvp;
					uint32_t p1 = (pixel >> 24) & 0xFF;
					uint32_t p2 = (pixel >>  8) & 0xFF;
					int Y = (p1+p2) / 2;
					int8_t U = (pixel >>  0) & 0xFF;
					int8_t V = (pixel >> 16) & 0xFF;
					int R = Y + 1437 * V / 1024;
					int G = Y -  352 * U / 1024 - 731 * V / 1024;
					int B = Y + 1812 * U / 1024;
					
					//~ bmp_printf(FONT_SMALL, 0, 0, "%d %d %d %d   ", Y, R, G, B);

					BP = MP = zebra_rgb_color(Y < zll, R > zlh, G > zlh, B > zlh, y);
					BN = MN = zebra_rgb_color(Y < zll, R > zlh, G > zlh, B > zlh, y+1);
				}
				else
				{
					int p0 = (*lvp) >> 8 & 0xFF;
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
				}
					
				#undef MP
				#undef BP
				#undef BN
				#undef MN
			}
		}
	}
}


// clear only zebra, focus assist and whatever else is in BMP VRAM mirror
void
clrscr_mirror( void )
{
	if (!lv) return;
	if (!global_draw) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;

	int y;
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
draw_false_downsampled( void )
{
	//~ if (vram_width > 720) return;
	if (!PLAY_MODE)
	{
		if (!expsim) return;
	}
	//~ bvram_mirror_init();
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;

	int y;
	uint8_t * const lvram = get_yuv422_vram()->vram;
	int lvpitch = YUV422_LV_PITCH;
	uint8_t* fc = false_colour[falsecolor_palette];
	for( y = 40; y < 440; y += 2 )
	{
		uint32_t * const v_row = (uint32_t*)( lvram + y * lvpitch );        // 2 pixel
		uint16_t * const b_row = (uint16_t*)( bvram + y * BMPPITCH);          // 2 pixel
		uint16_t * const m_row = (uint16_t*)( bvram_mirror + y * BMPPITCH );  // 2 pixel
		
		uint8_t* lvp; // that's a moving pointer through lv vram
		uint16_t* bp;  // through bmp vram
		uint16_t* mp;  // through mirror
		
		for (lvp = ((uint8_t*)v_row)+1, bp = b_row, mp = m_row; lvp < (uint8_t*)(v_row + 720/2) ; lvp += 4, bp++, mp++)
		{
			#define BP (*bp)
			#define MP (*mp)
			#define BN (*(bp + BMPPITCH/2))
			#define MN (*(mp + BMPPITCH/2))
			
			if (BP != 0 && BP != MP) { little_cleanup(bp, mp); continue; }
			if (BN != 0 && BN != MN) { little_cleanup(bp + BMPPITCH/2, mp + BMPPITCH/2); continue; }
			
			int c = fc[*lvp]; c |= (c << 8);
			MP = BP = c;
			MN = BN = c;

			#undef BP
			#undef MP
			#undef BN
			#undef MN
		}
	}
}

void
highlight_luma_range(int lo, int hi, int color1, int color2)
{
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	if (!bvram_mirror) return;
	int y;
	uint8_t * const lvram = get_yuv422_vram()->vram;
	int lvpitch = YUV422_LV_PITCH;
	for( y = 0; y < 480; y += 2 )
	{
		uint32_t * const v_row = (uint32_t*)( lvram + y * lvpitch );        // 2 pixel
		uint16_t * const b_row = (uint16_t*)( bvram + y * BMPPITCH);          // 2 pixel
		
		uint8_t* lvp; // that's a moving pointer through lv vram
		uint16_t* bp;  // through bmp vram
		
		for (lvp = ((uint8_t*)v_row)+1, bp = b_row; lvp < (uint8_t*)(v_row + 720/2) ; lvp += 4, bp++)
		{
			int x = ((int)lvp) / 2;
			int color = (y/2 - x/2) % 2 ? color1 | color1 << 8 : color2 | color2 << 8;
			#define BP (*bp)
			#define BN (*(bp + BMPPITCH/2))
			int pix = (*lvp + *(lvp+2))/2;
			int c = pix >= lo && *lvp <= hi ? color : 0;
			BN = BP = c;
			#undef BP
			#undef BN
		}
	}
}

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
	if (!global_draw && lv) bmp_fill(0, 0, 0, 720, 480);
}

#define MAX_CROP_NAME_LEN 15
#define MAX_CROPMARKS 9
int num_cropmarks = 0;
char cropmark_names[MAX_CROPMARKS][MAX_CROP_NAME_LEN];

// Cropmark sorting code contributed by Nathan Rosenquist
void sort_cropmarks()
{
	int i = 0;
	int j = 0;
	
	char aux[MAX_CROP_NAME_LEN];
	
	for (i=0; i<num_cropmarks; i++)
	{
		for (j=i+1; j<num_cropmarks; j++)
		{
			if (strcmp(cropmark_names[i], cropmark_names[j]) > 0)
			{
				strcpy(aux, cropmark_names[i]);
				strcpy(cropmark_names[i], cropmark_names[j]);
				strcpy(cropmark_names[j], aux);
			}
		}
	}
}

int is_valid_cropmark_filename(char* filename)
{
	int n = strlen(filename);
	if ((n > 4) && (streq(filename + n - 4, ".BMP") || streq(filename + n - 4, ".bmp")) && (filename[0] != '.') && (filename[0] != '_'))
		return 1;
	return 0;
}

static void find_cropmarks()
{
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "CROPMKS/", &file );
	if( IS_ERROR(dirent) )
	{
		NotifyBox(2000, "CROPMKS dir missing" );
		msleep(100);
		NotifyBox(2000, "Please copy all ML files!" );
		return;
	}
	int k = 0;
	do {
		if (is_valid_cropmark_filename(file.name))
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
	FIO_CleanupAfterFindNext_maybe(dirent);
	num_cropmarks = k;
	sort_cropmarks();
}
static void reload_cropmark(int i)
{
	static int old_i = -1;
	if (i == old_i) return; 
	old_i = i;
	//~ bmp_printf(FONT_LARGE, 0, 100, "reload crop: %d", i);

	if (cropmarks)
	{
		FreeMemory(cropmarks);
		cropmarks = 0;
	}
	
	i = COERCE(i, 0, num_cropmarks);
	if (i)
	{
		char bmpname[100];
		snprintf(bmpname, sizeof(bmpname), CARD_DRIVE "CROPMKS/%s", cropmark_names[i-1]);
		cropmarks = bmp_load(bmpname,1);
		if (!cropmarks) bmp_printf(FONT_LARGE, 0, 50, "LOAD ERROR %d:%s   ", i, bmpname);
	}
}

static void
crop_toggle( int sign )
{
	crop_draw = mod(crop_draw + sign, num_cropmarks + 1);  // 0 = off, 1..num_cropmarks = cropmarks
	reload_cropmark(crop_draw);
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
zebra_draw_display( void * priv, int x, int y, int selected )
{
	unsigned z = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zebras      : %s, %d..%d",
		z == 1 ? "Luma" : (z == 2 ? "RGB" : "OFF"),
		zebra_level_lo, zebra_level_hi
	);
	menu_draw_icon(x, y, MNI_BOOL_GDR(z), 0);
}

static void
falsecolor_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"False Color : %s",
		falsecolor_draw ? "ON" : "OFF"
	);
	int i;
	for (i = 0; i < 256; i++)
	{
		draw_line(x + 364 + i, y + 2, x + 364 + i, y + font_large.height - 2, false_colour[falsecolor_palette][i]);
	}
	menu_draw_icon(x, y, MNI_BOOL_GDR(falsecolor_draw), 0);
}

static void
falsecolor_palette_toggle(void* priv)
{
	falsecolor_palette = mod(falsecolor_palette+1, COUNT(false_colour));
}
/*
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
}*/

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
	menu_draw_icon(x, y, MNI_BOOL_GDR(f), 0);
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
	//~ extern int retry_count;
	int index = crop_draw;
	index = COERCE(index, 0, num_cropmarks);
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Cropmks(%d/%d): %s%s",
		 index, num_cropmarks,
		 index  ? cropmark_names[index-1] : "OFF",
		 (cropmarks || !index) ? "" : "!" // ! means error
	);
	//~ int h = font_large.height;
	//~ int w = h * 720 / 480;
	//~ bmp_draw_scaled_ex(cropmarks, x + 572, y, w, h, 0, 0);
	if (index && cropmark_movieonly && !is_movie_mode())
		menu_draw_icon(x, y, MNI_WARNING, 0);
	menu_draw_icon(x, y, MNI_BOOL_GDR(index), 0);
}

/*
static void
focus_graph_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Focus Graph : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}*/

/*
static void
edge_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Edgedetect  : %s",
		*(unsigned*) priv ? "ON " : "OFF"
	);
}*/

static void
hist_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Histo/Wavefm: %s/%s",
		hist_draw == 1 ? "Luma" : hist_draw == 2 ? "RGB" : "OFF",
		//~ hist_draw ? "RGB" : "OFF",
		waveform_draw == 1 ? "Small" : waveform_draw == 2 ? "Large" : waveform_draw == 3 ? "FullScreen" : "OFF"
	);
	//~ bmp_printf(FONT_MED, x + 460, y+5, "[SET/Q]");
	menu_draw_icon(x, y, MNI_BOOL_GDR(hist_draw || waveform_draw), 0);
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
	if (disp_profiles_0)
		bmp_printf(FONT(FONT_LARGE, 55, COLOR_BLACK), x + 540, y, "DISP %d", get_disp_mode());
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
	menu_draw_icon(x, y, MNI_BOOL_GDR(*(unsigned*) priv), 0);
}
static void 
waveform_toggle(void* priv)
{
	waveform_draw = mod(waveform_draw+1, 4);
	bmp_fill(0, 360, 240-50, 360, 240);
}


static void
clearscreen_display(
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
		"ClearScreen : %s",
		//~ mode ? "ON (HalfShutter)" : "OFF"
		mode == 0 ? "OFF" : 
		mode == 1 ? "HalfShutter" : 
		mode == 2 ? "WhenIdle" : "err"
	);
}

static void
zoom_overlay_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Magic Zoom  : %s%s%s",
		zoom_overlay_mode == 0 ? "OFF" :
		zoom_overlay_mode == 1 ? "Zrec, " :
		zoom_overlay_mode == 2 ? "Zr+F, " :
		zoom_overlay_mode == 3 ? "(+), " : "ALW, ",

		zoom_overlay_mode == 0 ? "" :
			zoom_overlay_size == 0 ? "Small, " :
			zoom_overlay_size == 1 ? "Med, " :
			zoom_overlay_size == 2 ? "Large, " :
			zoom_overlay_size == 3 ? "SmallX2, " :
			zoom_overlay_size == 4 ? "MedX2, " :
			zoom_overlay_size == 5 ? "LargeX2, " :  "720x480, ",
		zoom_overlay_mode == 0 ? "" :
			zoom_overlay_pos == 0 ? "AFF" :
			zoom_overlay_pos == 1 ? "NW" :
			zoom_overlay_pos == 2 ? "NE" :
			zoom_overlay_pos == 3 ? "SE" :
			zoom_overlay_pos == 4 ? "SW" : "err"
	);

	if (zoom_overlay_mode && !get_zoom_overlay_mode()) // MZ enabled, but for some reason it doesn't work in current mode
		menu_draw_icon(x, y, MNI_WARNING, 0);
	else
		menu_draw_icon(x, y, MNI_BOOL_GDR(zoom_overlay_mode), 0);
}

/*
static void
split_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Split Screen: %s%s",
		zoom_overlay_split ? "ON" : "OFF",
		zoom_overlay_split && zoom_overlay_split_zerocross ? ", zerocross" : ""
	);
	menu_draw_icon(x, y, MNI_BOOL_GDR(zoom_overlay_split), 0);
}

static void split_zerocross_toggle(void* priv)
{
	zoom_overlay_split_zerocross = !zoom_overlay_split_zerocross;
}*/

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
		"Spotmeter   : %s",
		spotmeter_draw == 0 ? "OFF" : (spotmeter_formula == 0 ? "Percent" : spotmeter_formula == 1 ? "IRE -1..101" : "IRE 0..108")
	);
	menu_draw_icon(x, y, MNI_BOOL_GDR(spotmeter_draw), 0);
}

static void 
spotmeter_formula_toggle(void* priv)
{
	spotmeter_formula = mod(spotmeter_formula + 1, 3);
}



void get_spot_yuv(int dx, int* Y, int* U, int* V)
{
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return;
	const uint16_t*		vr = (void*) YUV422_LV_BUFFER_DMA_ADDR;
	const unsigned		width = vram->width;
	//~ const unsigned		pitch = vram->pitch;
	const unsigned		height = vram->height;
	unsigned		x, y;

	bmp_draw_rect(COLOR_WHITE, width/2 - dx, height/2 - dx, 2*dx, 2*dx);
	
	unsigned sy = 0;
	int32_t su = 0, sv = 0; // Y is unsigned, U and V are signed
	// Sum the values around the center
	for( y = height/2 - dx ; y <= height/2 + dx ; y++ )
	{
		for( x = width/2 - dx ; x <= width/2 + dx ; x++ )
		{
			uint16_t p = vr[ x + y * width ];
			sy += (p >> 8) & 0xFF;
			if (x % 2) sv += (int8_t)(p & 0x00FF); else su += (int8_t)(p & 0x00FF);
		}
	}

	sy /= (2 * dx + 1) * (2 * dx + 1);
	su /= (dx + 1) * (2 * dx + 1);
	sv /= (dx + 1) * (2 * dx + 1);

	*Y = sy;
	*U = su;
	*V = sv;
}

int get_spot_motion(int dx, int draw)
{
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return 0;
	const uint16_t*		vr1 = (void*)YUV422_LV_BUFFER_DMA_ADDR;
	const uint16_t*		vr2 = (void*)get_fastrefresh_422_buf();
	uint8_t * const		bm = bmp_vram();
	if (!bm) return 0;
	const unsigned		width = vram->width;
	//~ const unsigned		pitch = vram->pitch;
	const unsigned		height = vram->height;
	unsigned		x, y;

	draw_line(width/2 - dx, height/2 - dx, width/2 + dx, height/2 - dx, COLOR_WHITE);
	draw_line(width/2 + dx, height/2 - dx, width/2 + dx, height/2 + dx, COLOR_WHITE);
	draw_line(width/2 + dx, height/2 + dx, width/2 - dx, height/2 + dx, COLOR_WHITE);
	draw_line(width/2 - dx, height/2 + dx, width/2 - dx, height/2 - dx, COLOR_WHITE);
	
	unsigned D = 0;
	for( y = height/2 - dx ; y <= height/2 + dx ; y++ )
	{
		for( x = width/2 - dx ; x <= width/2 + dx ; x++ )
		{
			int p1 = (vr1[ x + y * width ] >> 8) & 0xFF;
			int p2 = (vr2[ x + y * width ] >> 8) & 0xFF;
			int dif = ABS(p1 - p2);
			D += dif;
			if (draw) bm[x + y * BMPPITCH] = false_colour[4][dif & 0xFF];
		}
	}
	
	D = D * 2;
	D /= (2 * dx + 1) * (2 * dx + 1);
	return D;
}

int get_spot_focus(int dx)
{
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return 0;
	const uint32_t*		vr = (uint32_t*) vram->vram; // 2px
	const unsigned		width = vram->width;
	//~ const unsigned		pitch = vram->pitch;
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

void spotmeter_step()
{
    //~ if (!lv) return;
	struct vram_info *	vram = get_yuv422_vram();

	if( !vram->vram )
		return;
	
	const uint16_t*		vr = (uint16_t*) vram->vram;
	const unsigned		width = vram->width;
	//~ const unsigned		pitch = vram->pitch;
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
	if (scaled < 50 || falsecolor_draw) fg = COLOR_WHITE;
	int bg = falsecolor_draw ? COLOR_BG : 0;

	int xc = (hdmi_code == 5) ? 480 : 360;
	int yc = (hdmi_code == 5) ? 270 : 240;
	bmp_draw_rect(fg, xc - dx, yc - dx, 2*dx+1, 2*dx+1);
	yc += dx + 20;
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


void zoom_overlay_main_toggle(void* priv)
{
	zoom_overlay_mode = mod(zoom_overlay_mode + 1, 5);
}

void zoom_overlay_size_toggle(void* priv)
{
	zoom_overlay_size = mod(zoom_overlay_size + 1, 5);
}

static void
disp_profiles_0_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"DISP profiles  : %d", 
		disp_profiles_0 + 1
	);
}


static void
transparent_overlay_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (transparent_overlay && (transparent_overlay_offx || transparent_overlay_offy))
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Ghost Image : ON, dx=%d, dy=%d", 
			transparent_overlay_offx, 
			transparent_overlay_offy
		);
	else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Ghost Image : %s", 
			transparent_overlay ? "ON" : "OFF"
		);
	menu_draw_icon(x, y, MNI_BOOL_GDR(transparent_overlay), 0);
	transparent_overlay_hidden = 0;
}

void transparent_overlay_offset(int dx, int dy)
{
	transparent_overlay_offx = COERCE((int)transparent_overlay_offx + dx, -650, 650);
	transparent_overlay_offy = COERCE((int)transparent_overlay_offy + dy, -400, 400);
	transparent_overlay_hidden = 0;
	BMP_LOCK( show_overlay(); )
}

void transparent_overlay_center_or_toggle()
{
	if (transparent_overlay_offx || transparent_overlay_offy) // if off-center, just center it
	{
		transparent_overlay_offset_clear(0);
		transparent_overlay_offset(0, 0);
	}
	else // if centered, hide it or show it back
	{
		transparent_overlay_hidden = !transparent_overlay_hidden;
		if (!transparent_overlay_hidden) BMP_LOCK( show_overlay(); )
		else redraw();
	}
}

void transparent_overlay_offset_clear(void* priv)
{
	transparent_overlay_offx = transparent_overlay_offy = 0;
}

int handle_transparent_overlay(struct event * event)
{
	if (transparent_overlay && event->param == BGMT_LV && (gui_state == GUISTATE_QR || PLAY_MODE))
	{
		schedule_transparent_overlay();
		return 0;
	}

	if (transparent_overlay && lv && gui_state == GUISTATE_IDLE && !gui_menu_shown())
	{
		if (event->param == BGMT_PRESS_UP)
		{
			transparent_overlay_offset(0, -40);
			return 0;
		}
		if (event->param == BGMT_PRESS_DOWN)
		{
			transparent_overlay_offset(0, 40);
			return 0;
		}
		if (event->param == BGMT_PRESS_LEFT)
		{
			transparent_overlay_offset(-40, 0);
			return 0;
		}
		if (event->param == BGMT_PRESS_RIGHT)
		{
			transparent_overlay_offset(40, 0);
			return 0;
		}
		if (event->param == BGMT_PRESS_SET)
		{
			transparent_overlay_center_or_toggle();
			return 0;
		}
	}
	return 1;
}

char* idle_time_format(int t)
{
	static char msg[50];
	if (t) snprintf(msg, sizeof(msg), "after %d%s", t < 60 ? t : t/60, t < 60 ? "sec" : "min");
	else snprintf(msg, sizeof(msg), "OFF");
	return msg;
}

static void
idle_display_dim_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Dim display        : %s",
		idle_time_format(*(int*)priv)
	);
}

static void
idle_display_turn_off_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Turn off LCD and LV: %s",
		idle_time_format(*(int*)priv)
	);
}

static void
idle_rec_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Save power when REC: %s",
		idle_rec ? "ON" : "OFF"
	);
}

static void
idle_display_global_draw_off_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Turn off GlobalDraw: %s",
		idle_time_format(*(int*)priv)
	);
}

int timeout_values[] = {0, 5, 10, 20, 30, 60, 120, 300, 600, 900};

int current_timeout_index(int t)
{
	int i;
	for (i = 0; i < COUNT(timeout_values); i++)
		if (t == timeout_values[i]) return i;
	return 0;
}

void idle_timeout_toggle(void* priv, int sign)
{
	int* t = (int*)priv;
	int i = current_timeout_index(*t);
	i = mod(i + sign, COUNT(timeout_values));
	*(int*)priv = timeout_values[i];
}

void idle_timeout_toggle_forward(void* priv) { idle_timeout_toggle(priv, 1); }
void idle_timeout_toggle_reverse(void* priv) { idle_timeout_toggle(priv, -1); }

CONFIG_INT("defish.preview", defish_preview, 0);
static void
defish_preview_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Live Defish : %s",
		defish_preview ? "ON" : "OFF"
	);
}


struct menu_entry zebra_menus[] = {
	{
		.name = "Global Draw",
		.priv		= &global_draw,
		.select		= global_draw_toggle,
		.display	= global_draw_display,
		.help = "Enable/disable ML overlay graphics (zebra, cropmarks...)"

	},
	{
		.name = "Zebras",
		.priv		= &zebra_draw,
		.select		= menu_ternary_toggle_reverse,
		.select_reverse = zebra_lo_toggle, 
		.select_auto = zebra_hi_toggle,
		.display	= zebra_draw_display,
		.help = "Zebra stripes: show overexposed or underexposed areas."
	},
	{
		.name = "Focus Peak",
		.priv			= &focus_peaking,
		.display		= focus_peaking_display,
		.select			= menu_ternary_toggle,
		.select_reverse = focus_peaking_adjust_color, 
		.select_auto    = focus_peaking_adjust_thr,
		.help = "Show tiny dots on focused edges. Params: method,thr,color."
	},
	{
		.name = "Magic Zoom",
		.priv = &zoom_overlay_pos,
		.display = zoom_overlay_display,
		.select = zoom_overlay_main_toggle,
		.select_reverse = zoom_overlay_size_toggle,
		.select_auto = menu_quinternary_toggle,
		.help = "Zoom box for checking focus. Can be used while recording."
	},
	/*{
		.name = "Split Screen",
		.priv = &zoom_overlay_split,
		.display = split_display, 
		.select = menu_binary_toggle,
		.select_auto = split_zerocross_toggle,
		.help = "Magic Zoom will be split when image is out of focus. [Q]:ZC"
	},*/
	{
		.name = "Cropmks(x/n)",
		.priv = &crop_draw,
		.display	= crop_display,
		.select		= crop_toggle_forward,
		.select_reverse		= crop_toggle_reverse,
		.help = "Cropmarks for framing. Usually shown only in Movie mode."
	},
	{
		.name = "Ghost image",
		.priv = &transparent_overlay, 
		.display = transparent_overlay_display, 
		.select = menu_binary_toggle,
		.select_auto = transparent_overlay_offset_clear,
		.help = "Overlay any image in LiveView. In PLAY mode, press LV btn."
	},
	{
		.name = "Live Defish",
		.priv = &defish_preview, 
		.display = defish_preview_display, 
		.select = menu_binary_toggle,
		.help = "Preview rectilinear image from Samyang 8mm fisheye (gray)."
	},
	{
		.name = "Spotmeter",
		.priv			= &spotmeter_draw,
		.select			= menu_binary_toggle,
		.select_auto	= spotmeter_formula_toggle,
		.display		= spotmeter_menu_display,
		.help = "Measure brightness in the frame center. [Q]: Percent/IRE."
	},
	{
		.name = "False color",
		.priv		= &falsecolor_draw,
		.display	= falsecolor_display,
		.select		= menu_binary_toggle,
		.select_auto = falsecolor_palette_toggle,
		.help = "Shows brightness level as color-coded. [Q]: change palette."
	},
	{
		.name = "Histo/Wavefm",
		.priv		= &hist_draw,
		.select		= menu_ternary_toggle_reverse,
		.select_auto = waveform_toggle,
		.display	= hist_display,
		.help = "Histogram [SET] and Waveform [Q] for evaluating exposure."
	},
	{
		.name = "ClearScreen",
		.priv			= &clearscreen,
		.display		= clearscreen_display,
		.select			= menu_ternary_toggle,
		.select_reverse	= menu_ternary_toggle_reverse,
		.help = "Clear bitmap overlays from LiveView display."
	},
	/*{
		.priv			= &focus_graph,
		.display		= focus_graph_display,
		.select			= menu_binary_toggle,
	},*/
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
	/*	{
		 .priv = "[debug] dump vram", 
		 .display = menu_print, 
		 .select = dump_vram,
	 }*/
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

struct menu_entry livev_dbg_menus[] = {
#if CONFIG_DEBUGMSG
	{
		.priv = "Card Benchmark",
		.select = card_benchmark_schedule,
		.display = menu_print,
	},
#endif
	/*
	{
		.priv = "Dump RAM",
		.display = menu_print, 
		.select = dump_vram,
	},
	{
		.priv		= &unified_loop,
		.select		= menu_ternary_toggle,
		.display	= unified_loop_display,
		.help = "Unique loop for zebra and FP. Used with HDMI and 720p."
	},
	{
		.priv		= &zebra_density,
		.select		= menu_ternary_toggle,
		.display	= zebra_mode_display,
	},
	{
		.priv		= &use_hd_vram,
		.select		= menu_binary_toggle,
		.display	= use_hd_vram_display,
	},
	{
		.priv = &focus_peaking_debug,
		.select = menu_binary_toggle, 
		.display = focus_debug_display,
	}*/
};
struct menu_entry powersave_menus[] = {
	{
		.name = "Dim display",
		.priv			= &idle_display_dim_after,
		.display		= idle_display_dim_print,
		.select			= idle_timeout_toggle_forward,
		.select_reverse	= idle_timeout_toggle_reverse,
		.help = "Dim LCD display in LiveView when idle, to save power."
	},
	{
		.name = "Turn off LCD and LV",
		.priv			= &idle_display_turn_off_after,
		.display		= idle_display_turn_off_print,
		.select			= idle_timeout_toggle_forward,
		.select_reverse	= idle_timeout_toggle_reverse,
		.help = "Turn off display and pause LiveView when idle and not REC."
	},
	{
		.name = "Turn off GlobalDraw",
		.priv			= &idle_display_global_draw_off_after,
		.display		= idle_display_global_draw_off_print,
		.select			= idle_timeout_toggle_forward,
		.select_reverse	= idle_timeout_toggle_reverse,
		.help = "Turn off GlobalDraw when idle, to save some CPU cycles."
	},
	{
		.name = "Save power when REC",
		.priv			= &idle_rec,
		.display		= idle_rec_print,
		.select			= menu_binary_toggle,
		.help = "If enabled, camera will save power during recording."
	},
};

struct menu_entry livev_cfg_menus[] = {
	{
		.name = "DISP presets",
		.priv		= &disp_profiles_0,
		.select		= menu_quaternary_toggle,
		.select_reverse	= menu_quaternary_toggle_reverse,
		.display	= disp_profiles_0_display,
		.help = "No. of LiveV disp. presets. Switch w Metering or ISO+DISP."
	},
};


PROP_HANDLER(PROP_MVR_REC_START)
{
	if (buf[0] != 1) redraw_after(2000);
	return prop_cleanup( token, property );
}

void 
cropmark_draw()
{
	ChangeColorPaletteLV(2);
	if (!get_global_draw()) return;
	if (transparent_overlay && !transparent_overlay_hidden) show_overlay();
	if (cropmark_movieonly && !is_movie_mode()) return;
	reload_cropmark(crop_draw); // reloads only when changed
	clrscr_mirror();
	//~ bmp_printf(FONT_MED, 0, 0, "%x %x %x %x ", os.x0, os.y0, os.x_ex, os.y_ex);
	bmp_draw_scaled_ex(cropmarks, os.x0, os.y0, os.x_ex, os.y_ex, bvram_mirror, 0);
}
void
cropmark_redraw()
{
	BMP_LOCK( cropmark_draw(); )
	zoom_overlay_dirty = 1;
}

// those functions will do nothing if called multiple times (it's safe to do this)
// they might cause ERR80 if called while taking a picture

int is_safe_to_mess_with_the_display(int timeout_ms)
{
	int k = 0;
	while (lens_info.job_state >= 10 || tft_status || recording == 1)
	{
		k++;
		if (k * 100 > timeout_ms) return 0;
		msleep(100);
	}
	return 1;
}

int _bmp_cleared = 0;
void bmp_on()
{
	//~ return;
	//~ if (!is_safe_to_mess_with_the_display(500)) return;
	if (_bmp_cleared) 
	{// BMP_LOCK(GMT_LOCK( if (is_safe_to_mess_with_the_display(0)) {call("MuteOff"); _bmp_cleared = 0;}))
		cli_save();
		if (tft_status == 0 && lv)
		{
			MuteOff_0();
		}
		sei_restore();
		_bmp_cleared = 0;
	}
}
void bmp_on_force()
{
	_bmp_cleared = 1;
	bmp_on();
}
void bmp_off()
{
	//~ return;
	//~ clrscr();
	//~ if (!is_safe_to_mess_with_the_display(500)) return;
	if (!_bmp_cleared) //{ BMP_LOCK(GMT_LOCK( if (is_safe_to_mess_with_the_display(0)) { call("MuteOn")); ) }}
	{
		cli_save();
		if (tft_status == 0 && lv)
		{
			_bmp_cleared = 1;
			MuteOn_0();
		}
		sei_restore();
	}
}
int bmp_is_on() { return !_bmp_cleared; }

/*
int _lvimage_cleared = 0;
void lvimage_on()
{
	if (!is_safe_to_mess_with_the_display(500)) return;
	if (!_lvimage_cleared) call("MuteOffImage");
	_lvimage_cleared = 1;
}
void lvimage_off()
{
	if (!is_safe_to_mess_with_the_display(500)) return;
	if (_lvimage_cleared) GMT_LOCK( call("MuteOnImage"); )
	_lvimage_cleared = 0;
}*/

void zoom_overlay_toggle()
{
	zoom_overlay = !zoom_overlay;
	if (!zoom_overlay)
	{
		zoom_overlay_countdown = 0;
		redraw_after(500);
	}
}

int handle_zoom_overlay(struct event * event)
{
	// zoom in when recording => enable Magic Zoom 
	if (get_zoom_overlay_mode() && recording == 2 && event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
	{
		zoom_overlay_toggle();
		return 0;
	}

	// if magic zoom is enabled, Zoom In should always disable it 
	if (lv && get_zoom_overlay() && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
	{
		zoom_overlay_toggle();
		return 0;
	}
	
	if (lv && get_zoom_overlay_mode() && lv_dispsize == 1 && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
	{
		// magic zoom toggled by sensor+zoom in (modes Zr and Zr+F)
		if (get_zoom_overlay_mode() < 3 && get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED)
		{
			zoom_overlay_toggle();
			return 0;
		}
		// (*): magic zoom toggled by zoom in, normal zoom by sensor+zoom in
		else if (get_zoom_overlay_mode() == 3 && !get_halfshutter_pressed() && !(get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED))
		{
			zoom_overlay_toggle();
			return 0;
		}
	}
	
	// move AF frame when recording
	if (recording && get_zoom_overlay_mode() && liveview_display_idle())
	{
		if (event->param == BGMT_PRESS_LEFT)
			{ move_lv_afframe(-200, 0); return 0; }
		if (event->param == BGMT_PRESS_RIGHT)
			{ move_lv_afframe(200, 0); return 0; }
		if (event->param == BGMT_PRESS_UP)
			{ move_lv_afframe(0, -200); return 0; }
		if (event->param == BGMT_PRESS_DOWN)
			{ move_lv_afframe(0, 200); return 0; }
		if (event->param == BGMT_PRESS_SET)
			{ center_lv_afframe(); return 0; }
	}

	return 1;
}
//~ void zoom_overlay_enable()
//~ {
	//~ zoom_overlay = 1;
//~ }

void zoom_overlay_disable()
{
	zoom_overlay = 0;
	zoom_overlay_countdown = 0;
}

void zoom_overlay_set_countdown(int x)
{
	zoom_overlay_countdown = x;
}

void yuvcpy_x2(uint32_t* dst, uint32_t* src, int num_pix)
{
	dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
	src = (void*)((unsigned int)src & 0xFFFFFFFC);
	uint32_t* last_s = src + (num_pix>>1);
	for (; src < last_s; src++, dst += 2)
	{
		*(dst) = ((*src) & 0x00FFFFFF) | (((*src) & 0x0000FF00) << 16);
		*(dst+1) = ((*src) & 0xFFFF00FF) | (((*src) & 0xFF000000) >> 16);
	}
}

void draw_zoom_overlay(int dirty)
{	
	//~ if (vram_width > 720) return;
	if (!lv) return;
	if (!get_global_draw()) return;
	//~ if (gui_menu_shown()) return;
	if (!bmp_is_on()) return;
	if (lv_dispsize != 1) return;
	//~ if (get_halfshutter_pressed() && clearscreen != 2) return;
	if (recording == 1) return;
	
	struct vram_info *	lv = get_yuv422_vram();
	struct vram_info *	hd = get_yuv422_hd_vram();
	
	//~ lv->width = 1920;

	if( !lv->vram )	return;
	if( !hd->vram )	return;
	if( !bmp_vram()) return;

	uint16_t*		lvr = (uint16_t*) lv->vram;
	uint16_t*		hdr = (uint16_t*) hd->vram;
	
	if (!lvr) return;

	int hx0,hy0; 
	get_afframe_pos(hd->width, hd->height, &hx0, &hy0);
	
	int W = 240;
	int H = 240;
	
	switch(zoom_overlay_size)
	{
		case 0:
		case 3:
			W = 150;
			H = 150;
			break;
		case 1:
		case 4:
			W = 250;
			H = 200;
			break;
		case 2:
		case 5:
			W = 500;
			H = 350;
			break;
		case 6:
			W = 720;
			H = 480;
			break;
	}
	
	int x2 = (zoom_overlay_size > 2) ? 1 : 0;

	int x0,y0; 
	int xaf,yaf;
	get_afframe_pos(lv->width, lv->height, &xaf, &yaf);

	switch(zoom_overlay_pos)
	{
		case 0: // AFF
			x0 = xaf;
			y0 = yaf;
			break;
		case 1: // NW
			x0 = W/2 + 50;
			y0 = H/2 + 50;
			break;
		case 2: // NE
			x0 = 720 - W/2 - 50;
			y0 = H/2 + 50;
			break;
		case 3: // SE
			x0 = 720 - W/2 - 50;
			y0 = 480 - H/2 - 50;
			break;
		case 4: // SV
			x0 = W/2 + 50;
			y0 = 480 - H/2 - 50;
			break;
		default:
			return;
	}
	
	/*if (zoom_overlay_size == 6)
	{
		x0 = 360;
		y0 = 240;
		x2 = 0;
	}*/

	if (zoom_overlay_pos)
	{
		int w = W * lv->width / hd->width;
		int h = H * lv->width / hd->width;
		if (x2)
		{
			w >>= 1;
			h >>= 1;
		}
		memset(lvr + COERCE(xaf - (w>>1), 0, 720-w) + COERCE(yaf - (h>>1) - 1, 0, 480) * lv->width, 0,    w<<1);
		memset(lvr + COERCE(xaf - (w>>1), 0, 720-w) + COERCE(yaf - (h>>1) - 2, 0, 480) * lv->width, 0xFF, w<<1);
		memset(lvr + COERCE(xaf - (w>>1), 0, 720-w) + COERCE(yaf + (h>>1) + 1, 0, 480) * lv->width, 0xFF, w<<1);
		memset(lvr + COERCE(xaf - (w>>1), 0, 720-w) + COERCE(yaf + (h>>1) + 2, 0, 480) * lv->width, 0,    w<<1);
	}

	//~ draw_circle(x0,y0,45,COLOR_WHITE);
	int y;
	int x0c = COERCE(x0 - (W>>1), 0, 720-W);
	int y0c = COERCE(y0 - (H>>1), 0, 480-H);

	extern int focus_value;
	int rawoff = COERCE(80 - focus_value, 0, 100) >> 2;
	
	// reverse the sign of split when perfect focus is achieved
	static int rev = 0;
	static int poff = 0;
	if (rawoff != 0 && poff == 0) rev = !rev;
	poff = rawoff;
	if (!zoom_overlay_split_zerocross) rev = 0;

	if (x2)
	{
		uint16_t* d = lvr + x0c + (y0c + 2) * lv->width;
		uint16_t* s = hdr + (hy0 - (H>>2)) * hd->width + (hx0 - (W>>2));
		for (y = 2; y < H-2; y++)
		{
			int off = zoom_overlay_split ? (y < H/2 ? rawoff : -rawoff) : 0;
			if (rev) off = -off;
			yuvcpy_x2((uint32_t*)d, (uint32_t*)(s + off), W>>1);
			d += lv->width;
			if (y & 1) s += hd->width;
		}
	}
	else
	{
		uint16_t* d = lvr + x0c + (y0c + 2) * lv->width;
		uint16_t* s = hdr + (hy0 - (H>>1)) * hd->width + (hx0 - (W>>1));
		for (y = 2; y < H-2; y++)
		{
			int off = zoom_overlay_split ? (y < H/2 ? rawoff : -rawoff) : 0;
			if (rev) off = -off;
			memcpy(d, s + off, W<<1);
			d += lv->width;
			s += hd->width;
		}
	}

	memset(lvr + x0c + COERCE(0   + y0c, 0, 720) * lv->width, rawoff ? 0    : 0x80, W<<1);
	memset(lvr + x0c + COERCE(1   + y0c, 0, 720) * lv->width, rawoff ? 0xFF : 0x80, W<<1);
	memset(lvr + x0c + COERCE(H-1 + y0c, 0, 720) * lv->width, rawoff ? 0xFF : 0x80, W<<1);
	memset(lvr + x0c + COERCE(H   + y0c, 0, 720) * lv->width, rawoff ? 0    : 0x80, W<<1);
	if (dirty) bmp_fill(0, x0c, y0c + 2, W, H - 4);
	//~ bmp_fill(rawoff ? COLOR_BLACK : COLOR_GREEN1, x0c, y0c, W, 1);
	//~ bmp_fill(rawoff ? COLOR_WHITE : COLOR_GREEN2, x0c+1, y0c, W, 1);
	//~ bmp_fill(rawoff ? COLOR_WHITE : COLOR_GREEN2, x0c, y0c + H - 1, W, 1);
	//~ bmp_fill(rawoff ? COLOR_BLACK : COLOR_GREEN1, x0c, y0c + H, W, 1);
}

//~ int zebra_paused = 0;
//~ void zebra_pause() { zebra_paused = 1; }
//~ void zebra_resume() { zebra_paused = 0; }

int liveview_display_idle()
{
	return
		lv && 
		!menu_active_and_not_hidden() &&
		gui_state == GUISTATE_IDLE && 
		CURRENT_DIALOG_MAYBE <= 3 && 
		#ifdef CURRENT_DIALOG_MAYBE_2
		CURRENT_DIALOG_MAYBE_2 <= 3 &&
		#endif
		lv_dispsize == 1 &&
		lens_info.job_state < 10 &&
		!mirror_down &&
		//~ !zebra_paused &&
		!(clearscreen == 1 && get_halfshutter_pressed());
}
// when it's safe to draw zebras and other on-screen stuff
int zebra_should_run()
{
	return liveview_display_idle() && get_global_draw();
}

void zebra_sleep_when_tired()
{
	if (!zebra_should_run())
	{
		while (clearscreen == 1 && get_halfshutter_pressed()) msleep(100);
		if (zebra_should_run()) return;
		//~ if (!gui_menu_shown()) ChangeColorPaletteLV(4);
		if (lv && !gui_menu_shown()) redraw();
		while (!zebra_should_run()) msleep(100);
		ChangeColorPaletteLV(2);
		crop_set_dirty(5);
		//~ if (lv && !gui_menu_shown()) redraw();
	}
}

void clear_this_message_not_available_in_movie_mode()
{
	static int fp = -1;
	int f = FLASH_BTN_MOVIE_MODE;
	if (f == fp) return; // clear the message only once
	fp = f;
	if (!f) return;
	
	bmp_fill(0, 0, 330, 720, 480-330);
	msleep(50);
	bmp_fill(0, 0, 330, 720, 480-330);
}

void draw_livev_for_playback()
{
	clrscr();
	if (!get_global_draw()) return;
	
BMP_LOCK(
	cropmark_redraw();

	if (spotmeter_draw)
		spotmeter_step();

	if (falsecolor_draw) 
	{
		draw_false_downsampled();
	}
	else if (defish_preview)
	{
		defish_draw();
	}
	else
	{
		draw_zebra_and_focus(1,0);
	}
	
	draw_histogram_and_waveform();
)
}

void draw_histogram_and_waveform()
{
	if (!get_global_draw()) return;
	if (hist_draw || waveform_draw)
	{
		struct vram_info * vram = get_yuv422_vram();
		hist_build(vram->vram, vram->width, vram->pitch);
	}
	
	if (menu_active_and_not_hidden()) return; // hack: not to draw histo over menu
	if (!get_global_draw()) return;
	
	if( hist_draw)
		hist_draw_image( os.x_max - hist_width, os.y0 + 100, -1);

	if (menu_active_and_not_hidden()) return;
	if (!get_global_draw()) return;
		
	if( waveform_draw)
		waveform_draw_image( os.x_max - WAVEFORM_WIDTH*WAVEFORM_FACTOR, os.y_max - WAVEFORM_HEIGHT*WAVEFORM_FACTOR - WAVEFORM_OFFSET );
}
/*
//this function is a mess... but seems to work
static void
zebra_task( void )
{
	DebugMsg( DM_MAGIC, 3, "Starting zebra_task");
	
	msleep(1000);
	
	find_cropmarks();
	int k;

	while(1)
	{
		k++;
		msleep(10); // safety msleep :)
		if (recording) msleep(100);
		
		if (lv && disp_mode_change_request)
		{
			disp_mode_change_request = 0;
			do_disp_mode_change();
		}
		
		zebra_sleep_when_tired();
		
		if (get_global_draw())
		{
			draw_livev_stuff(k);
		}
	}
}


TASK_CREATE( "zebra_task", zebra_task, 0, 0x1f, 0x1000 ); */

int idle_countdown_display_dim = 50;
int idle_countdown_display_off = 50;
int idle_countdown_globaldraw = 50;
int idle_countdown_clrscr = 50;
int idle_countdown_killflicker = 5;
int idle_countdown_display_dim_prev = 50;
int idle_countdown_display_off_prev = 50;
int idle_countdown_globaldraw_prev = 50;
int idle_countdown_clrscr_prev = 50;
int idle_countdown_killflicker_prev = 5;

void idle_wakeup_reset_counters(int reason) // called from handle_buttons
{
#if CONFIG_DEBUGMSG
	NotifyBox(1000, "wakeup: %x   ", reason);
#endif

	// those are for powersaving
	idle_countdown_display_off = MAX((int)idle_display_turn_off_after * 10, idle_countdown_display_off);
	idle_countdown_display_dim = MAX((int)idle_display_dim_after * 10, idle_countdown_display_dim);
	idle_countdown_globaldraw = MAX((int)idle_display_global_draw_off_after * 10, idle_countdown_display_dim);

	if (reason == -2345) // disable powersave during recording 
		return;

	// those are not for powersaving
	idle_countdown_clrscr = 30;
	
	if (reason == -10 || reason == -11) // focus event (todo: should define constants for those)
		return;
	
	idle_countdown_killflicker = 5;
}

// called at 10 Hz
void update_idle_countdown(int* countdown)
{
	//~ bmp_printf(FONT_MED, 200, 200, "%d  ", *countdown);
	if (!get_halfshutter_pressed() && liveview_display_idle())
	{
		if (*countdown)
			(*countdown)--;
	}
	else
	{
		idle_wakeup_reset_counters(-100); // will reset all idle countdowns
	}
	
	if (get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED)
		idle_wakeup_reset_counters(-1);
}

void idle_action_do(int* countdown, int* prev_countdown, void(*action_on)(void), void(*action_off)(void))
{
	update_idle_countdown(countdown);
	int c = *countdown; // *countdown may be changed by "wakeup" => race condition
	//~ bmp_printf(FONT_MED, 100, 200, "%d->%d ", *prev_countdown, c);
	if (*prev_countdown && !c)
	{
		//~ bmp_printf(FONT_MED, 100, 200, "action  "); msleep(500);
		action_on();
		//~ msleep(500);
		//~ bmp_printf(FONT_MED, 100, 200, "        ");
	}
	else if (!*prev_countdown && c)
	{
		//~ bmp_printf(FONT_MED, 100, 200, "unaction"); msleep(500);
		action_off();
		//~ msleep(500);
		//~ bmp_printf(FONT_MED, 100, 200, "        ");
	}
	*prev_countdown = c;
}

void PauseLiveView()
{
	if (lv && !lv_paused)
	{
		int x = 1;
		prop_request_change(PROP_LV_ACTION, &x, 4);
		msleep(100);
		clrscr();
		lv_paused = 1;
		lv = 1;
	}
}

void ResumeLiveView()
{
	if (lv && lv_paused)
	{
		lv = 0;
		int x = 0;
		prop_request_change(PROP_LV_ACTION, &x, 4);
		while (!lv) msleep(100);
	}
	lv_paused = 0;
}

void idle_display_off()
{
	extern int motion_detect;

	wait_till_next_second();

	if (motion_detect || recording)
	{
		NotifyBox(1000, "DISPLAY OFF");
	}
	else
	{
		NotifyBox(1000, "DISPLAY AND SENSOR OFF");
	}

	for (int i = 0; i < 30; i++)
	{
		if (idle_countdown_display_off) { NotifyBoxHide(); return; }
		msleep(100);
	}
	if (!(motion_detect || recording)) PauseLiveView();
	display_off_force();
	msleep(100);
	idle_countdown_display_off = 0;
}
void idle_display_on()
{
	//~ card_led_blink(5, 50, 50);
	ResumeLiveView();
	display_on_force();
	redraw();
}

void idle_bmp_off()
{
	bmp_off();
}
void idle_bmp_on()
{
	bmp_on();
}

int old_backlight_level = 0;
void idle_display_dim()
{
	old_backlight_level = backlight_level;
	set_backlight_level(1);
}
void idle_display_undim()
{
	if (old_backlight_level) set_backlight_level(old_backlight_level);
	old_backlight_level = 0;
}

void idle_globaldraw_dis()
{
	idle_globaldraw_disable = 1;
}
void idle_globaldraw_en()
{
	idle_globaldraw_disable = 0;
}

void idle_kill_flicker()
{
	idle_globaldraw_disable = 0;
	kill_flicker();
}
void idle_stop_killing_flicker()
{
	#ifdef CONFIG_KILL_FLICKER
	idle_globaldraw_disable = 1;
	#endif
	stop_killing_flicker();
}


static void
clearscreen_task( void* unused )
{
	idle_wakeup_reset_counters(0);
	#ifdef CONFIG_KILL_FLICKER
	idle_stop_killing_flicker();
	#endif

	int k = 0;
	for (;;k++)
	{
clearscreen_loop:
		//~ msleep(100);
		if (lens_info.job_state == 0) // unsafe otherwise?
		{
			call("DisablePowerSave");
			call("EnablePowerSave");
		}
		msleep(100);
		//~ card_led_blink(1,10,90);
		
		//~ bmp_printf(FONT_MED, 100, 100, "%d %d %d", idle_countdown_display_dim, idle_countdown_display_off, idle_countdown_globaldraw);

		if (k % 50 == 0 && (tft_status || !display_is_on()) && lens_info.job_state == 0)
			card_led_blink(1, 50, 50);

		if (!lv) continue;
		
		/*if (k % 10 == 0)
		{
			bmp_printf(FONT_MED, 50, 50, "%d fps ", fps_ticks);
			fps_ticks = 0;
		}*/

		// clear overlays on shutter halfpress
		if (clearscreen == 1 && get_halfshutter_pressed() && !gui_menu_shown())
		{
			BMP_LOCK( clrscr_mirror(); )
			int i;
			for (i = 0; i < (int)clearscreen_delay/10; i++)
			{
				lens_display_set_dirty();
				msleep(10);
				if (!get_halfshutter_pressed() || dofpreview)
					goto clearscreen_loop;
			}
			bmp_off();
			while (get_halfshutter_pressed()) msleep(100);
			bmp_on();
		}
		//~ else if (clearscreen == 2)  // always clear overlays
		//~ {
			//~ idle_action_do(&idle_countdown_display_clear, bmp_off, bmp_on);
		//~ }

		if (recording && !idle_rec) // don't go to powersave when recording
			idle_wakeup_reset_counters(-2345);
		
		if (idle_display_dim_after)
			idle_action_do(&idle_countdown_display_dim, &idle_countdown_display_dim_prev, idle_display_dim, idle_display_undim);

		if (idle_display_turn_off_after)
			idle_action_do(&idle_countdown_display_off, &idle_countdown_display_off_prev, idle_display_off, idle_display_on);

		if (idle_display_global_draw_off_after)
			idle_action_do(&idle_countdown_globaldraw, &idle_countdown_globaldraw_prev, idle_globaldraw_dis, idle_globaldraw_en);

		if (clearscreen == 2) // clear overlay when idle
			idle_action_do(&idle_countdown_clrscr, &idle_countdown_clrscr_prev, idle_bmp_off, idle_bmp_on);
		
		#ifdef CONFIG_KILL_FLICKER
		if (recording)
			idle_countdown_killflicker = 5; // no flicker problems during recording

		if (global_draw && !gui_menu_shown())
			idle_action_do(&idle_countdown_killflicker, &idle_countdown_killflicker_prev, idle_kill_flicker, idle_stop_killing_flicker);
		#endif

		// since this task runs at 10Hz, I prefer cropmark redrawing here
		if (crop_dirty)
		{
			crop_dirty--;
			if (crop_dirty == 0)
				crop_redraw_flag = 1;
		}
	}
}

TASK_CREATE( "cls_task", clearscreen_task, 0, 0x1a, 0x1000 );

CONFIG_INT("disable.redraw", disable_redraw, 0);
CONFIG_INT("display.dont.mirror", display_dont_mirror, 1);

// this should be synchronized with
// * graphics code (like zebra); otherwise zebras will remain frozen on screen
// * gui_main_task (to make sure Canon won't call redraw in parallel => crash)
void redraw_do()
{
BMP_LOCK (

#ifndef CONFIG_50D
	if (display_dont_mirror && display_dont_mirror_dirty)
	{
		if (lcd_position == 1) NormalDisplay();
		display_dont_mirror_dirty = 0;
	}
#endif

	if (disable_redraw) 
	{
		clrscr(); // safest possible redraw method :)
	}
	else
	{
		struct gui_task * current = gui_task_list.current;
		struct dialog * dialog = current->priv;
		if (dialog && MEM(dialog->type) == DLG_SIGNATURE && !flicker_being_killed()) // if dialog seems valid
		{
			dialog_redraw(dialog); // try to redraw (this has semaphores for winsys)
		}
		else
			clrscr(); // out of luck, fallback
	}
)
	// ask other stuff to redraw
	afframe_set_dirty();
	crop_set_dirty(2);
	menu_set_dirty();
	zoom_overlay_dirty = 1;
}

void redraw()
{
	fake_simple_button(MLEV_REDRAW);
}

void test_fps(int* x)
{
	int x0 = 0;
	int F = 0;
	//~ int f = 0;
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


int should_draw_zoom_overlay()
{
	if (zoom_overlay_mode == 4 && zebra_should_run() && get_global_draw()) return 1;
	if (zebra_should_run() && get_global_draw() && zoom_overlay_mode && (zoom_overlay || zoom_overlay_countdown)) return 1;
	if (lv && get_halfshutter_pressed() && get_global_draw() && zoom_overlay_mode && (zoom_overlay || zoom_overlay_countdown)) return 1;
	return 0;
}


void false_color_toggle()
{
	falsecolor_draw = !falsecolor_draw;
	if (falsecolor_draw) zoom_overlay_disable();
}

int transparent_overlay_flag = 0;
void schedule_transparent_overlay()
{
	transparent_overlay_flag = 1;
}

volatile int lens_display_dirty = 0;
void lens_display_set_dirty() { lens_display_dirty = 1; }

// Items which need a high FPS
// Magic Zoom, Focus Peaking, zebra*, spotmeter*, false color*
// * = not really high FPS, but still fluent
 static void
livev_hipriority_task( void* unused )
{
	msleep(1000);
	find_cropmarks();

	int k = 0;
	for (;;k++)
 	{
		msleep(10);
		//~ vsync(&YUV422_LV_BUFFER_DMA_ADDR);
		fps_ticks++;

		while (is_mvr_buffer_almost_full())
			msleep(100);
		
		get_422_hd_idle_buf(); // just to keep it up-to-date
		
		zebra_sleep_when_tired();
		

		if (should_draw_zoom_overlay())
		{
			guess_fastrefresh_direction();
			if (zoom_overlay_dirty) clrscr_mirror(); 
			BMP_LOCK( if (lv) draw_zoom_overlay(zoom_overlay_dirty); )
			zoom_overlay_dirty = 0;
		}
		else
		{
			zoom_overlay_dirty = 1;
			if (falsecolor_draw)
			{
				if (k % 2 == 0)
					BMP_LOCK( if (lv) draw_false_downsampled(); )
			}
			else if (defish_preview)
			{
				if (k % 4 == 0)
					BMP_LOCK( if (lv) defish_draw(); )
			}
			else
			{
				BMP_LOCK( if (lv) draw_zebra_and_focus(k % 2 == 0, 1); )
			}
			//~ msleep(20);
		}
		
		if (spotmeter_draw && k % 4 == 0)
			BMP_LOCK( if (lv) spotmeter_step(); )

		if (zoom_overlay_countdown)
		{
			zoom_overlay_countdown--;
			crop_set_dirty(5);
		}
		
		if (lens_display_dirty && k % 5 == 0)
		{
			BMP_LOCK( update_lens_display(); );
			lens_display_dirty = 0;
		}
		
		if (LV_BOTTOM_BAR_DISPLAYED || get_halfshutter_pressed())
			crop_set_dirty(5);

#if CONFIG_DEBUGMSG
		if (card_benchmark_start)
		{
			card_benchmark();
			card_benchmark_start = 0;
		}
#endif
	}
}

static void loprio_sleep()
{
	msleep(10);
	while (is_mvr_buffer_almost_full()) msleep(100);
}

static void black_bars()
{
	if (!get_global_draw()) return;
	int i,j;
	for (i = 0; i < os.y0 + os.y_ex; i++)
	{
		if (i < os.y0 + 50 || i > os.y0 + os.y_ex - 50)
		{
			int newcolor = (i < os.y0 + 35 || i > os.y0 + os.y_ex - 37) ? COLOR_BLACK : COLOR_BG;
			for (j = os.x0; j < os.x_ex; j++)
				if (bmp_getpixel(j,i) == COLOR_BG)
					bmp_putpixel(j,i,newcolor);
		}
	}
}

// Items which do not need a high FPS, but are CPU intensive
// histogram, waveform...
static void
livev_lopriority_task( void* unused )
{
	while(1)
	{
		#ifdef CONFIG_550D
		black_bars();
		#endif
		
		if (transparent_overlay_flag)
		{
			transparent_overlay_from_play();
			transparent_overlay_flag = 0;
		}

		// here, redrawing cropmarks does not block fast zoom
		extern int cropmarks_play; // from tweak.c
		if (cropmarks_play && PLAY_MODE)
		{
			cropmark_redraw();
			msleep(2000);
		}

		loprio_sleep();
		if (!zebra_should_run())
			continue;

		loprio_sleep();

		draw_histogram_and_waveform();
		
		if (crop_redraw_flag)
		{
			cropmark_redraw();
			crop_redraw_flag = 0;
		}
	}
}

#if defined(CONFIG_600D) || defined(CONFIG_50D)
#define HIPRIORITY_TASK_PRIO 0x19
#else
#define HIPRIORITY_TASK_PRIO 0x1a
#endif

TASK_CREATE( "livev_hiprio_task", livev_hipriority_task, 0, HIPRIORITY_TASK_PRIO, 0x1000 );
TASK_CREATE( "livev_loprio_task", livev_lopriority_task, 0, 0x1f, 0x1000 );

/*static CONFIG_INT("picstyle.disppreset", picstyle_disppreset_enabled, 0);
static unsigned int picstyle_disppreset = 0;
PROP_HANDLER(PROP_PICTURE_STYLE)
{
	update_disp_mode_bits_from_params();
	return prop_cleanup(token, property);
}*/

int unused = 0;
unsigned int * disp_mode_params[] = {&crop_draw, &zebra_draw, &hist_draw, &waveform_draw, &falsecolor_draw, &spotmeter_draw, &clearscreen, &focus_peaking, &zoom_overlay_split, &global_draw, &zoom_overlay_mode, &transparent_overlay};
int disp_mode_bits[] =              {4,          2,           2,          2,              2,                2,               2,             2,             1,                   1,            2,                   2};

void update_disp_mode_bits_from_params()
{
	//~ picstyle_disppreset = lens_info.picstyle;
	
	int i;
	int off = 0;
	uint32_t bits = 0;
	for (i = 0; i < COUNT(disp_mode_params); i++)
	{
		int b = disp_mode_bits[i];
		bits = bits | (((*(disp_mode_params[i])) & ((1 << b) - 1)) << off);
		off += b;
	}
	
	if (disp_mode == 1) disp_mode_a = bits;
	else if (disp_mode == 2) disp_mode_b = bits;
	else if (disp_mode == 3) disp_mode_c = bits;
	else disp_mode_x = bits;
	
	unused++;
	//~ bmp_printf(FONT_MED, 0, 50, "mode: %d", disp_mode);
	//~ bmp_printf(FONT_MED, 0, 50, "a=%8x b=%8x c=%8x x=%8x", disp_mode_a, disp_mode_b, disp_mode_c, disp_mode_x);
}

int update_disp_mode_params_from_bits()
{
	uint32_t bits = disp_mode == 1 ? disp_mode_a : 
	                disp_mode == 2 ? disp_mode_b :
	                disp_mode == 3 ? disp_mode_c : disp_mode_x;
	
	static uint32_t old_bits = 0xffffffff;
	if (bits == old_bits) return 0;
	old_bits = bits;
	
	int i;
	int off = 0;
	for (i = 0; i < COUNT(disp_mode_bits); i++)
	{
		int b = disp_mode_bits[i];
		//~ bmp_printf(FONT_LARGE, 50, 50, "%d) %x -> %x", i, disp_mode_params[i], (bits >> off) & ((1 << b) - 1));
		//~ bmp_printf(FONT_MED, 50, 100, "%x %x %x %x", &clearscreen, &focus_peaking, &zoom_overlay_split, &global_draw);
		//~ msleep(5000);
		*(disp_mode_params[i]) = (bits >> off) & ((1 << b) - 1);
		off += b;
	}
	
	/*if (picstyle_disppreset_enabled && picstyle_disppreset)
	{
		int p = get_prop_picstyle_from_index(picstyle_disppreset);
		//~ bmp_printf(FONT_LARGE, 50, 50, "picsty %x ", p);
		//~ msleep(1000);
		if (p) prop_request_change(PROP_PICTURE_STYLE, &p, 4);
	}*/
	
	//~ bmp_on();
	return 1;
}

int get_disp_mode() { return disp_mode; }

int toggle_disp_mode()
{
	idle_wakeup_reset_counters(-3);
	disp_mode = mod(disp_mode + 1, disp_profiles_0 + 1);
	BMP_LOCK( do_disp_mode_change(); )
	menu_set_dirty();
	return disp_mode == 0;
}
void do_disp_mode_change()
{
	if (gui_menu_shown()) 
	{ 
		update_disp_mode_params_from_bits(); 
		return; 
	}
	
	display_on();
	bmp_on();
	redraw();
	NotifyBox(1000, "Display preset: %d", disp_mode);
	update_disp_mode_params_from_bits();
	//~ draw_ml_topbar();
	crop_set_dirty(10);
}

int livev_playback = 0;

void livev_playback_toggle()
{
	livev_playback = !livev_playback;
	if (livev_playback)
	{
		draw_livev_for_playback();
	}
	else
	{
		redraw();
	}
}
void livev_playback_reset()
{
	livev_playback = 0;
}

int handle_livev_playback(struct event * event, int button)
{
	// enable LiveV stuff in Play mode
	if (PLAY_MODE)
	{
		if (event->param == button)
		{
			livev_playback_toggle();
			return 0;
		}
		else
			livev_playback_reset();
	}
	return 1;
}


static void zebra_init_menus()
{
    menu_add( "LiveV", zebra_menus, COUNT(zebra_menus) );
    //~ menu_add( "Debug", dbg_menus, COUNT(dbg_menus) );
    //~ menu_add( "Movie", movie_menus, COUNT(movie_menus) );
    //~ menu_add( "Config", cfg_menus, COUNT(cfg_menus) );
	menu_add( "Power", powersave_menus, COUNT(powersave_menus) );
}

INIT_FUNC(__FILE__, zebra_init_menus);




void make_overlay()
{
	//~ bvram_mirror_init();
	clrscr();

	bmp_printf(FONT_MED, 0, 0, "Saving overlay...");

	struct vram_info * vram = get_yuv422_vram();
	uint8_t * const lvram = vram->vram;
	int lvpitch = YUV422_LV_PITCH;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	#define BMPPITCH 960

	int y;
	for (y = 0; y < vram->height; y++)
	{
		//~ int k;
		uint16_t * const v_row = (uint16_t*)( lvram + y * lvpitch );        // 1 pixel
		uint8_t * const b_row = (uint8_t*)( bvram + y * BMPPITCH);          // 1 pixel
		uint8_t * const m_row = (uint8_t*)( bvram_mirror + y * BMPPITCH);   // 1 pixel
		uint16_t* lvp; // that's a moving pointer through lv vram
		uint8_t* bp;  // through bmp vram
		uint8_t* mp;  //through bmp vram mirror
		for (lvp = v_row, bp = b_row, mp = m_row; lvp < v_row + 720 ; lvp++, bp++, mp++)
			if ((y + (int)bp) % 2)
				*bp = *mp = ((*lvp) * 41 >> 16) + 38;
	}
	FIO_RemoveFile(CARD_DRIVE "overlay.dat");
	FILE* f = FIO_CreateFile(CARD_DRIVE "overlay.dat");
	FIO_WriteFile( f, (const void *) UNCACHEABLE(bvram_mirror), BVRAM_MIRROR_SIZE);
	FIO_CloseFile(f);
	bmp_printf(FONT_MED, 0, 0, "Overlay saved.  ");

	msleep(1000);
}

void show_overlay()
{
	//~ bvram_mirror_init();
	struct vram_info * vram = get_yuv422_vram();
	//~ uint8_t * const lvram = vram->vram;
	//~ int lvpitch = YUV422_LV_PITCH;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	#define BMPPITCH 960
	
	clrscr();

	FILE* f = FIO_Open(CARD_DRIVE "overlay.dat", O_RDONLY | O_SYNC);
	if (f == INVALID_PTR) return;
	FIO_ReadFile(f, UNCACHEABLE(bvram_mirror), BVRAM_MIRROR_SIZE );
	FIO_CloseFile(f);

	int y;
	for (y = 0; y < vram->height; y++)
	{
		//~ int k;
		//~ uint16_t * const v_row = (uint16_t*)( lvram + y * lvpitch );        // 1 pixel
		uint8_t * const b_row = (uint8_t*)( bvram + y * BMPPITCH);          // 1 pixel
		uint8_t * const m_row = (uint8_t*)( bvram_mirror + (y - (int)transparent_overlay_offy) * BMPPITCH);   // 1 pixel
		uint8_t* bp;  // through bmp vram
		uint8_t* mp;  //through bmp vram mirror
		if (y - (int)transparent_overlay_offy < 0 || y - (int)transparent_overlay_offy > 480) continue;
		//~ int offm = 0;
		//~ int offb = 0;
		//~ if (transparent_overlay == 2) offm = 720/2;
		//~ if (transparent_overlay == 3) offb = 720/2;
		for (bp = b_row, mp = m_row - (int)transparent_overlay_offx; bp < b_row + 720 ; bp++, mp++)
			if (((y + (int)bp) % 2) && mp > m_row && mp < m_row + 720)
				*bp = *mp;
	}
	
	bzero32(bvram_mirror, BVRAM_MIRROR_SIZE);
}

void bmp_zoom(int x0, int y0, int numx, int denx, int numy, int deny)
{
	uint8_t * bvram = bmp_vram();
	if (!bvram) return;
	#define BMPPITCH 960
	memcpy(bvram_mirror, bvram, BVRAM_MIRROR_SIZE);
	int i,j;
	for (i = 0; i < 540; i++)
	{
		for (j = 0; j < 960; j++)
		{
			int is = (i - y0) * deny / numy + y0;
			int js = (j - x0) * denx / numx + x0;
			bvram[i * BMPPITCH + j] = (is >= 0 && js >= 0 && is < 540 && js < 960) ? bvram_mirror[is * BMPPITCH + js] : 0;
			
		}
	}
	bzero32(bvram_mirror, BVRAM_MIRROR_SIZE);
}

void transparent_overlay_from_play()
{
	if (!PLAY_MODE) { fake_simple_button(BGMT_PLAY); msleep(1000); }
	make_overlay();
	get_out_of_play_mode(500);
	msleep(500);
	if (!lv) { force_liveview(); msleep(500); }
	msleep(1000);
	BMP_LOCK( show_overlay(); )
	//~ transparent_overlay = 1;
}

INIT_FUNC("bvram_mirror_init", bvram_mirror_init);

//~ CONFIG_STR("defish.lut", defish_lut_file, CARD_DRIVE "recti.lut");
#define defish_lut_file CARD_DRIVE "rectilin.lut"

uint8_t* defish_lut = INVALID_PTR;

void defish_draw()
{
	if (defish_lut == INVALID_PTR)
	{
		int size = 0;
		defish_lut = (uint8_t*)read_entire_file(defish_lut_file, &size);
	}
	if (defish_lut == NULL)
	{
		bmp_printf(FONT_MED, 50, 50, "%s not loaded", defish_lut_file);
		return;
	}
	int i,j;
	struct vram_info * vram = get_yuv422_vram();
	uint8_t * const lvram = vram->vram;
	int lvpitch = YUV422_LV_PITCH;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;

	//~ int y;
	for (i = 0; i < 240; i++)
	{
		for (j = 0; j < 360; j++)
		{
			static const int off_i[] = {0,0,479,479};
			static const int off_j[] = {0,719,0,719};
			int id = defish_lut[(i * 360 + j) * 2 + 1];
			int jd = defish_lut[(i * 360 + j) * 2] * 360 / 255;
			int k;
			for (k = 0; k < 4; k++)
			{
				int I = (off_i[k] ? off_i[k] - i : i);
				int J = (off_j[k] ? off_j[k] - j : j);
				int Id = (off_i[k] ? off_i[k] - id : id);
				int Jd = (off_j[k] ? off_j[k] - jd : jd);
				int lv_pixel = lvram[Id * lvpitch + Jd * 2 + 1];
				uint8_t* bp = &(bvram[I * BMPPITCH + J]);
				uint8_t* mp = &(bvram_mirror[I * BMPPITCH + J]);
				if (*bp != 0 && *bp != *mp) continue;
				*bp = *mp = (lv_pixel * 41 >> 8) + 38;
			}
		}
	}
}

PROP_HANDLER(PROP_LV_ACTION)
{
	zoom_overlay_countdown = 0;
	idle_display_undim(); // restore LCD brightness, especially for shutdown
	idle_wakeup_reset_counters(-4);
	#ifdef CONFIG_KILL_FLICKER
	idle_globaldraw_disable = 1;
	#else
	idle_globaldraw_disable = 0;
	#endif
	return prop_cleanup( token, property );
}

void yuv_resize(uint32_t* src, int src_w, int src_h, uint32_t* dst, int dst_w, int dst_h)
{
	int i,j;
	for (i = 0; i < dst_h; i++)
	{
		for (j = 0; j < dst_w/2; j++)
		{
			dst[i * dst_w/2 + j] = src[(i*src_h/dst_h) * src_w/2 + (j*src_w/dst_w)];
		}
	}
}

void yuv_halfcopy(uint32_t* dst, uint32_t* src, int w, int h, int top_half)
{
	int i,j;
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w/2; j++)
		{
			int sign = j - i * w/h/2;
			if ((top_half && sign > 0) || (!top_half && sign <= 0))
			{
				dst[i * w/2 + j] = src[i * w/2 + j];
			}
		}
	}
}

void play_422(char* filename)
{
    //~ bmp_printf(FONT_LARGE, 0, 0, filename);
	//~ return;
	clrscr();
	
	unsigned size;
	if( FIO_GetFileSize( filename, &size ) != 0 ) return;
	uint32_t * buf = (uint32_t*)YUV422_HD_BUFFER_2;
	struct vram_info * vram = get_yuv422_vram();

	bmp_printf(FONT_LARGE, 0, 0, "%s ", filename+17);
    bmp_printf(FONT_LARGE, 500, 0, "%d", size);

	int w,h;
    if      (size == 1056*704*2) { w = 1056; h = 704; } // photo mode
    else if (size == 1720*974*2) { w = 1720; h = 974; } // fullhd 550d, 60d
    else if (size == 1720*974*2) { w = 1680; h = 945; } // fullhd 600d
    else if (size == 1728*972*2) { w = 1728; h = 972; } // fullhd 3x 600d
    else if (size == 580*580*2)  { w = 580 ; h = 580; }
    else if (size == 1280*580*2) { w = 1280; h = 580; } // 720p 550d, 60d
    else if (size == 1280*560*2) { w = 1280; h = 560; } // 720p 600d
    else if (size == 640*480*2)  { w = 640 ; h = 480; }
    else if (size == 1024*680*2) { w = 1024; h = 680; } // zoom mode (5x, 10x)
    else if (size == 512*340*2)  { w = 512;  h = 340; }
    else if (size == 720*480*2)  { w = 720;  h = 480; } // LiveView buffer
    else
    {
		bmp_printf(FONT_LARGE, 0, 50, "Cannot preview this picture.");
		bzero32(vram->vram, vram->width * vram->height * 2);
		return;
	}
    
    bmp_printf(FONT_LARGE, 500, 0, " %dx%d ", w, h);
	if (PLAY_MODE) bmp_printf(FONT_LARGE, 0, 480 - font_large.height, "Do not press Delete!");

	size_t rc = read_file( filename, buf, size );
	if( rc != size ) return;

	yuv_resize(buf, w, h, (uint32_t*)vram->vram, vram->width, vram->height);
}

/*char* get_next_422()
{
	static struct fio_file file;
	static int first = 1;
	static struct fio_dirent * dirent = 0;
	if (first)
	{
		dirent = FIO_FindFirstEx( CARD_DRIVE "DCIM/100CANON/", &file );
		if( IS_ERROR(dirent) )
		{
			bmp_printf( FONT_LARGE, 40, 40, "dir err" );
			return 0;
		}
		first = 0;
	}
	while(FIO_FindNextEx( dirent, &file ) == 0)
	{
		//~ msleep(1000);
		int n = strlen(file.name);
		if ((n > 4) && (streq(file.name + n - 4, ".422")))
		{
			bmp_printf(FONT_LARGE, 0, 0, "%s ", file.name);
			return file.name;
		}
	}
	FIO_CleanupAfterFindNext_maybe(dirent);
	first = 1;
	dirent = 0;
	return 0;
}*/
