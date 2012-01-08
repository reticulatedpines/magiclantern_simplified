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

#include "zebra.h"
#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "gui.h"
#include "lens.h"

//~ #if 1
//~ #define CONFIG_KILL_FLICKER // this will block all Canon drawing routines when the camera is idle 
#if defined(CONFIG_50D)// || defined(CONFIG_60D)
#define CONFIG_KILL_FLICKER // this will block all Canon drawing routines when the camera is idle 

extern int kill_canon_gui_mode;
#endif                      // but it will display ML graphics

int lv_paused = 0;

static void waveform_init();
static void histo_init();
static void do_disp_mode_change();
static void show_overlay();
static void transparent_overlay_from_play();
static void transparent_overlay_offset_clear(void* priv, int delta);
static void draw_histogram_and_waveform();
static void schedule_transparent_overlay();
static void defish_draw();
static int zebra_color_word_row(int c, int y);
static void spotmeter_step();


static void default_movie_cropmarks();
static void black_bars_16x9();
static void black_bars();
static void defish_draw_play();

extern unsigned int log_length(int x);
extern void zoom_sharpen_step();
extern void bv_auto_update();

void lens_display_set_dirty();

//~ static struct bmp_file_t * cropmarks_array[3] = {0};
static struct bmp_file_t * cropmarks = 0;
static bool _bmp_cleared = false;
static bool bmp_is_on() { return !_bmp_cleared; }
void bmp_on();
void bmp_off();

#define hist_height         64
#define hist_width          128
#define WAVEFORM_WIDTH 180
#define WAVEFORM_HEIGHT 120
#define WAVEFORM_FACTOR (1 << waveform_size) // 1, 2 or 4
#define WAVEFORM_OFFSET (waveform_size <= 1 ? 60 : 0)

#define BVRAM_MIRROR_SIZE (BMPPITCH*540)

CONFIG_INT("lv.disp.profiles", disp_profiles_0, 0);

static CONFIG_INT("disp.mode", disp_mode, 0);
static CONFIG_INT("disp.mode.aaa", disp_mode_a, 0x285041);
static CONFIG_INT("disp.mode.bbb", disp_mode_b, 0x295001);
static CONFIG_INT("disp.mode.ccc", disp_mode_c,  0x88090);
static CONFIG_INT("disp.mode.xxx", disp_mode_x, 0x2c5051);

       CONFIG_INT( "transparent.overlay", transparent_overlay, 0);
static CONFIG_INT( "transparent.overlay.x", transparent_overlay_offx, 0);
static CONFIG_INT( "transparent.overlay.y", transparent_overlay_offy, 0);
int transparent_overlay_hidden = 0;

static CONFIG_INT( "global.draw",   global_draw, 1 );
static CONFIG_INT( "zebra.draw",    zebra_draw, 0 );
static CONFIG_INT( "zebra.mode",    zebra_colorspace,   1 );// luma/rgb
static CONFIG_INT( "zebra.level.hi",    zebra_level_hi, 95 );
static CONFIG_INT( "zebra.level.lo",    zebra_level_lo, 5 );
       CONFIG_INT( "zebra.rec", zebra_rec,  1 );
static CONFIG_INT( "crop.enable",   crop_enabled,   0 ); // index of crop file
static CONFIG_INT( "crop.index",    crop_index, 0 ); // index of crop file
       CONFIG_INT( "crop.movieonly", cropmark_movieonly, 1);
static CONFIG_INT("crop.playback", cropmarks_play, 0);
static CONFIG_INT( "falsecolor.draw", falsecolor_draw, 0);
static CONFIG_INT( "falsecolor.palette", falsecolor_palette, 0);

#define MZ_ZOOM_WHILE_RECORDING 1
#define MZ_ZOOMREC_N_FOCUS_RING 2
#define MZ_TAKEOVER_ZOOM_IN_BTN 3
#define MZ_ALWAYS_ON            4
static CONFIG_INT( "zoom.overlay", zoom_overlay_enabled, 0);
static CONFIG_INT( "zoom.overlay.trig", zoom_overlay_trigger_mode, MZ_TAKEOVER_ZOOM_IN_BTN);
static CONFIG_INT( "zoom.overlay.size", zoom_overlay_size, 1);
static CONFIG_INT( "zoom.overlay.x2", zoom_overlay_x2, 1);
static CONFIG_INT( "zoom.overlay.pos", zoom_overlay_pos, 1);
static CONFIG_INT( "zoom.overlay.split", zoom_overlay_split, 0);
static CONFIG_INT( "zoom.overlay.lut", zoom_overlay_lut, 0);

//~ static CONFIG_INT( "zoom.overlay.split.zerocross", zoom_overlay_split_zerocross, 1);
int get_zoom_overlay_trigger_mode() 
{ 
    if (!get_global_draw()) return 0;
    if (!zoom_overlay_enabled) return 0;
    return zoom_overlay_trigger_mode;
}

int get_zoom_overlay_trigger_by_focus_ring()
{
    int z = get_zoom_overlay_trigger_mode();
    #ifdef CONFIG_5D2
    return z == 2 || z == 3;
    #else
    return z == 2;
    #endif
}

int get_zoom_overlay_trigger_by_halfshutter()
{
    #ifdef CONFIG_5D2
    int z = get_zoom_overlay_trigger_mode();
    return z == 1 || z == 3;
    #else
    return 0;
    #endif
}

int zoom_overlay_triggered_by_zoom_btn = 0;
int zoom_overlay_triggered_by_focus_ring_countdown = 0;
int is_zoom_overlay_triggered_by_zoom_btn() 
{ 
    if (!get_global_draw()) return 0;
    return zoom_overlay_triggered_by_zoom_btn;
}

int zoom_overlay_dirty = 0;

bool should_draw_zoom_overlay()
{
    if (!zoom_overlay_enabled) return 0;
    if (!zebra_should_run()) return 0;
    if (zoom_overlay_trigger_mode == 4) return true;
    if (zoom_overlay_triggered_by_zoom_btn || zoom_overlay_triggered_by_focus_ring_countdown) return true;
    return false;
}

static CONFIG_INT( "focus.peaking", focus_peaking, 0);
static CONFIG_INT( "focus.peaking.method", focus_peaking_method, 0);
static CONFIG_INT( "focus.peaking.thr", focus_peaking_pthr, 10); // 1%
static CONFIG_INT( "focus.peaking.color", focus_peaking_color, 7); // R,G,B,C,M,Y,cc1,cc2

//~ static CONFIG_INT( "focus.graph", focus_graph, 0);
//~ int get_crop_black_border() { return crop_black_border; }

//~ static CONFIG_INT( "edge.draw", edge_draw,  0 );
static CONFIG_INT( "hist.draw", hist_draw,  0 );
static CONFIG_INT( "hist.colorspace",   hist_colorspace,    1 );
static CONFIG_INT( "hist.warn", hist_warn,  2 );
static CONFIG_INT( "hist.log",  hist_log,   0 );
//~ static CONFIG_INT( "hist.x",        hist_x,     720 - hist_width - 4 );
//~ static CONFIG_INT( "hist.y",        hist_y,     100 );
static CONFIG_INT( "waveform.draw", waveform_draw,  0 );
static CONFIG_INT( "waveform.size", waveform_size,  0 );
//~ static CONFIG_INT( "waveform.x",    waveform_x, 720 - WAVEFORM_WIDTH );
//~ static CONFIG_INT( "waveform.y",    waveform_y, 480 - 50 - WAVEFORM_WIDTH );
static CONFIG_INT( "waveform.bg",   waveform_bg,    0x26 ); // solid black

static CONFIG_INT( "clear.preview", clearscreen_enabled, 0);
static CONFIG_INT( "clear.preview.delay", clearscreen_delay, 1000); // ms
static CONFIG_INT( "clear.preview.mode", clearscreen_mode, 0); // 2 is always

// keep old program logic
#define clearscreen (clearscreen_enabled ? clearscreen_mode+1 : 0)

static CONFIG_INT( "spotmeter.size",        spotmeter_size, 5 );
static CONFIG_INT( "spotmeter.draw",        spotmeter_draw, 1 );
static CONFIG_INT( "spotmeter.formula",     spotmeter_formula, 0 ); // 0 percent, 1 IRE AJ, 2 IRE Piers
static CONFIG_INT( "spotmeter.position",        spotmeter_position, 1 ); // fixed / attached to AF frame

//~ static CONFIG_INT( "unified.loop", unified_loop, 2); // temporary; on/off/auto
//~ static CONFIG_INT( "zebra.density", zebra_density, 0); 
//~ static CONFIG_INT( "hd.vram", use_hd_vram, 0); 

CONFIG_INT("idle.display.turn_off.after", idle_display_turn_off_after, 0); // this also enables power saving for intervalometer
static CONFIG_INT("idle.display.dim.after", idle_display_dim_after, 0);
static CONFIG_INT("idle.display.gdraw_off.after", idle_display_global_draw_off_after, 0);
static CONFIG_INT("idle.rec", idle_rec, 0);


int cropmark_cache_valid = 0;
int crop_redraw_flag = 0; // redraw cropmarks now
int crop_dirty = 0;       // redraw cropmarks after some time (unit: 0.1s)
int clearscreen_countdown = 20;

//~ int recording = 0;

static uint8_t false_colour[][256] = {
    {0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
    {0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x26, 0x26, 0x27, 0x27, 0x27, 0x27, 0x27, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x29, 0x29, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x31, 0x31, 0x31, 0x31, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x33, 0x33, 0x33, 0x33, 0x33, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x35, 0x35, 0x35, 0x35, 0x35, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x37, 0x37, 0x37, 0x37, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x39, 0x39, 0x39, 0x39, 0x39, 0x3A, 0x3A, 0x3A, 0x3A, 0x3A, 0x3A, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x43, 0x43, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x45, 0x45, 0x45, 0x45, 0x45, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x47, 0x47, 0x47, 0x47, 0x47, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
    {0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
    {0x0E, 0x0E, 0x0E, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02},
    {0x0E, 0x26, 0x30, 0x26, 0x30, 0x26, 0x30, 0x26, 0x30, 0x27, 0x31, 0x27, 0x31, 0x27, 0x31, 0x27, 0x31, 0x28, 0x32, 0x28, 0x32, 0x28, 0x32, 0x28, 0x32, 0x29, 0x33, 0x29, 0x33, 0x29, 0x33, 0x29, 0x33, 0x2a, 0x34, 0x2a, 0x34, 0x2a, 0x34, 0x2a, 0x34, 0x2a, 0x35, 0x2b, 0x35, 0x2b, 0x35, 0x2b, 0x35, 0x2b, 0x36, 0x2c, 0x36, 0x2c, 0x36, 0x2c, 0x36, 0x2c, 0x37, 0x2d, 0x37, 0x2d, 0x37, 0x2d, 0x37, 0x2d, 0x38, 0x2e, 0x38, 0x2e, 0x38, 0x2e, 0x38, 0x2e, 0x38, 0x2f, 0x39, 0x2f, 0x39, 0x2f, 0x39, 0x2f, 0x39, 0x30, 0x3a, 0x30, 0x3a, 0x30, 0x3a, 0x30, 0x3a, 0x31, 0x3b, 0x31, 0x3b, 0x31, 0x3b, 0x31, 0x3b, 0x32, 0x3c, 0x32, 0x3c, 0x32, 0x3c, 0x32, 0x3c, 0x33, 0x3d, 0x33, 0x3d, 0x33, 0x3d, 0x33, 0x3d, 0x33, 0x3e, 0x34, 0x3e, 0x34, 0x3e, 0x34, 0x3e, 0x34, 0x3f, 0x35, 0x3f, 0x35, 0x3f, 0x35, 0x3f, 0x35, 0x40, 0x36, 0x40, 0x36, 0x40, 0x36, 0x40, 0x36, 0x41, 0x37, 0x41, 0x37, 0x41, 0x37, 0x41, 0x37, 0x41, 0x38, 0x42, 0x38, 0x42, 0x38, 0x42, 0x38, 0x42, 0x39, 0x43, 0x39, 0x43, 0x39, 0x43, 0x39, 0x43, 0x3a, 0x44, 0x3a, 0x44, 0x3a, 0x44, 0x3a, 0x44, 0x3b, 0x45, 0x3b, 0x45, 0x3b, 0x45, 0x3b, 0x45, 0x3c, 0x46, 0x3c, 0x46, 0x3c, 0x46, 0x3c, 0x46, 0x3c, 0x47, 0x3d, 0x47, 0x3d, 0x47, 0x3d, 0x47, 0x3d, 0x48, 0x3e, 0x48, 0x3e, 0x48, 0x3e, 0x48, 0x3e, 0x49, 0x3f, 0x49, 0x3f, 0x49, 0x3f, 0x49, 0x3f, 0x4a, 0x40, 0x4a, 0x40, 0x4a, 0x40, 0x4a, 0x40, 0x4a, 0x41, 0x4b, 0x41, 0x4b, 0x41, 0x4b, 0x41, 0x4b, 0x42, 0x4c, 0x42, 0x4c, 0x42, 0x4c, 0x42, 0x4c, 0x43, 0x4d, 0x43, 0x4d, 0x43, 0x4d, 0x43, 0x4d, 0x44, 0x4e, 0x44, 0x4e, 0x44, 0x4e, 0x44, 0x4e, 0x08},
    {0x26, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 0x30, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x35, 0x36, 0x36, 0x37, 0x37, 0x38, 0x38, 0x38, 0x39, 0x39, 0x3A, 0x3A, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x3A, 0x3A, 0x3B, 0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3D, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x41, 0x41, 0x42, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x47, 0x48, 0x48, 0x49, 0x49, 0x49, 0x4A, 0x4A, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4D, 0x4D, 0x4E, 0x4E, 0x4F},
};

void crop_set_dirty(int value)
{
    crop_dirty = MAX(crop_dirty, value);
}

PROP_HANDLER(PROP_HOUTPUT_TYPE)
{
    extern int ml_started;
    if (ml_started) redraw_after(200);
    return prop_cleanup(token, property);
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

static int idle_globaldraw_disable = 0;

int get_global_draw() // menu setting, or off if 
{
    extern int ml_started;
    if (!ml_started) return 0;
    
    if (PLAY_MODE) return 1; // exception, always draw stuff in play mode
    
    return global_draw &&
        !idle_globaldraw_disable && 
        !sensor_cleaning && 
        bmp_is_on() &&
        tft_status == 0 && 
        recording != 1 && 
        #ifdef CONFIG_KILL_FLICKER
        !(lv && kill_canon_gui_mode && !canon_gui_front_buffer_disabled() && !gui_menu_shown()) &&
        #endif
        !lv_paused && 
        lens_info.job_state <= 10 &&
        !(recording && !lv);
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

/** total number of pixels analyzed by histogram */
static uint32_t hist_total_px;


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
hist_build()
{
    struct vram_info * lv = get_yuv422_vram();

    uint32_t *  v_row = (uint32_t*) lv->vram;
    int x,y;

    histo_init();
    if (!hist) return;
    if (!hist_r) return;
    if (!hist_g) return;
    if (!hist_b) return;

    hist_max = 0;
    hist_total_px = 0;
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

    for( y = os.y0 ; y < os.y_max; y += 2, v_row += (lv->pitch/2) )
    {
        for( x = os.x0 ; x < os.x_max ; x += 2 )
        {
            // Average each of the two pixels
            uint32_t pixel = v_row[x/2];
            uint32_t p1 = (pixel >> 16) & 0xFF00;
            uint32_t p2 = (pixel >>  0) & 0xFF00;
            int Y = ((p1+p2) / 2) >> 8;

            hist_total_px++;
            
            if (hist_colorspace == 1) // rgb
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
            if (waveform_draw) waveform[ COERCE(((x-os.x0) * WAVEFORM_WIDTH) / os.x_max, 0, WAVEFORM_WIDTH-1)][ COERCE((Y * WAVEFORM_HEIGHT) / 0xFF, 0, WAVEFORM_HEIGHT-1) ]++;
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
    struct vram_info * lv = get_yuv422_vram();
    if (!lv) return;

    *under = 0;
    *over = 0;
    void* vram = lv->vram;
    uint32_t *  v_row = (uint32_t*) vram;
    int x,y;
    for( y = os.y0 ; y < os.y_max; y += 2, v_row += (lv->pitch/2) )
    {
        for( x = os.x0 ; x < os.y_max ; x += 4 )
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
    unsigned        x_origin,
    unsigned        y_origin,
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

    int log_max = log_length(hist_max);
    
    for( i=0 ; i < hist_width ; i++ )
    {
        // Scale by the maximum bin value
        const uint32_t size  = hist_log ? log_length(hist[i])   * hist_height / log_max : (hist[i]   * hist_height) / hist_max;
        const uint32_t sizeR = hist_log ? log_length(hist_r[i]) * hist_height / log_max : (hist_r[i] * hist_height) / hist_max;
        const uint32_t sizeG = hist_log ? log_length(hist_g[i]) * hist_height / log_max : (hist_g[i] * hist_height) / hist_max;
        const uint32_t sizeB = hist_log ? log_length(hist_b[i]) * hist_height / log_max : (hist_b[i] * hist_height) / hist_max;

        uint8_t * col = row + i;
        // vertical line up to the hist size
        for( y=hist_height ; y>0 ; y-- , col += BMPPITCH )
        {
            if (highlight_level >= 0)
            {
                int hilight = ABS(i-highlight_level) <= 1;
                *col = y > size + hilight ? COLOR_BG : (hilight ? COLOR_RED : COLOR_WHITE);
            }
            else if (hist_colorspace == 1) // RGB
                *col = hist_rgb_color(y, sizeR, sizeG, sizeB);
            else
                *col = y > size ? COLOR_BG : (falsecolor_draw ? false_colour[falsecolor_palette][(i * 256 / hist_width) & 0xFF]: COLOR_WHITE);
        }
        
        if (hist_warn && i == hist_width - 1
            && !should_draw_zoom_overlay()) // magic zoom borders will be "overexposed" => will cause warning
        {
            unsigned int thr = hist_total_px / (
                hist_warn == 1 ? 100000 : // 0.001%
                hist_warn == 2 ? 10000  : // 0.01%
                hist_warn == 3 ? 1000   : // 0.01%
                                 100);    // 1%
            int yw = y_origin + 10 - 16 + (hist_log ? hist_height - 20 : 0);
            if (hist_colorspace == 1) // RGB
            {
                if (hist_r[i] > thr) dot(x_origin + hist_width/2 - 20 - 16, yw, COLOR_RED   , 7);
                if (hist_g[i] > thr) dot(x_origin + hist_width/2      - 16, yw, COLOR_GREEN1, 7);
                if (hist_b[i] > thr) dot(x_origin + hist_width/2 + 20 - 16, yw, COLOR_LIGHTBLUE  , 7);
            }
            else
            {
                if (hist[i] > thr) dot(x_origin + hist_width/2 - 16, yw, COLOR_RED, 7);
            }
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
    unsigned        x_origin,
    unsigned        y_origin
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
            asm( "nop" );
            asm( "nop" );
            asm( "nop" );
            asm( "nop" );
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
    menu_stop();
    card_benchmark_start = 1;
}
#endif

static void dump_vram()
{
    dump_big_seg(4, CARD_DRIVE "4.bin");
    dump_big_seg(4, CARD_DRIVE "4-1.bin");
    //dump_seg(0x1000, 0x100000, CARD_DRIVE "ram.bin");
    //~ dump_seg(YUV422_IMAGE_BUFFER, 1920*1080*2, CARD_DRIVE "VRAM.BIN");
}

static uint8_t* bvram_mirror = 0;
uint8_t* get_bvram_mirror() { return bvram_mirror; }
//~ #define bvram_mirror bmp_vram_idle()

int fps_ticks = 0;

/*void fail(char* msg)
{
    bmp_printf(FONT_LARGE, 30, 100, msg);
    while(1) msleep(1);
}*/
static void waveform_init()
{
    if (!waveform)
    {
        waveform = AllocateMemory(WAVEFORM_WIDTH * sizeof(uint32_t*));
        //~ if (!waveform) fail("Waveform malloc failed");
        int i;
        for (i = 0; i < WAVEFORM_WIDTH; i++) {
            waveform[i] = AllocateMemory(WAVEFORM_HEIGHT * sizeof(uint32_t));
            //~ if (!waveform[i]) fail("Waveform malloc failed");
        }
    }
}

static void histo_init()
{
    if (!hist) hist = AllocateMemory(hist_width * sizeof(uint32_t*));
    //~ if (!hist) fail("Hist malloc failed");

    if (!hist_r) hist_r = AllocateMemory(hist_width * sizeof(uint32_t*));
    //~ if (!hist_r) fail("HistR malloc failed");

    if (!hist_g) hist_g = AllocateMemory(hist_width * sizeof(uint32_t*));
    //~ if (!hist_g) fail("HistG malloc failed");

    if (!hist_b) hist_b = AllocateMemory(hist_width * sizeof(uint32_t*));
    //~ if (!hist_b) fail("HistB malloc failed");
}

void bvram_mirror_clear()
{
    BMP_LOCK( bzero32(bvram_mirror, BVRAM_MIRROR_SIZE); )
    cropmark_cache_valid = 0;
}
void bvram_mirror_init()
{
    
    if (!bvram_mirror)
    {
        bvram_mirror = AllocateMemory(BVRAM_MIRROR_SIZE);
        if (!bvram_mirror) 
        {   
            //~ bmp_printf(FONT_MED, 30, 30, "Failed to allocate BVRAM mirror");
            return;
        }
        bvram_mirror_clear();
    }
}

static int get_focus_color(int thr, int d)
{
    return
        focus_peaking_color == 0 ? COLOR_RED :
        focus_peaking_color == 1 ? 7 :
        focus_peaking_color == 2 ? COLOR_BLUE :
        focus_peaking_color == 3 ? 5 :
        focus_peaking_color == 4 ? 14 :
        focus_peaking_color == 5 ? 15 :
        focus_peaking_color == 6 ?  (thr > 50 ? COLOR_RED :
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


static int zebra_color_word_row(int c, int y)
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

static int zebra_color_word_row_thick(int c, int y)
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

/*  int Zd = should_draw_zoom_overlay();
    static int Zdp = 0;
    if (Zd && !Zdp) clrscr_mirror();
    Zdp = Zd;
    if (Zd) msleep(100); // reduce frame rate when zoom overlay is active
    */
    
    //~ if (unified_loop == 1) { draw_zebra_and_focus_unified(); return; }
    //~ if (unified_loop == 2 && (ext_monitor_hdmi || ext_monitor_rca || (is_movie_mode() && video_mode_resolution != 0)))
        //~ { draw_zebra_and_focus_unified(); return; }
    
    if (!get_global_draw()) return;
    
    // HD to LV coordinate transform:
    // non-record: 1056 px: 1.46 ratio (yuck!)
    // record: 1720: 2.38 ratio (yuck!)
    
    // How to scan?
    // Scan the HD vram and do ratio conversion only for the 1% pixels displayed

    //~ bvram_mirror_init();

    uint8_t * const bvram = bmp_vram_real();
    if (!bvram) return;
    if (!bvram_mirror) return;
    //~ int BMPPITCH = bmp_pitch();
    //~ int y;

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
        
        struct vram_info * hd_vram = get_yuv422_hd_vram();
        uint32_t * const hdvram = UNCACHEABLE(hd_vram->vram);
        
        static int thr = 50;
        
        int n_over = 0;
        int n_total = 0;

        // look in the HD buffer
        for(int y = os.y0 + os.off_169 + 8; y < os.y_max - os.off_169 - 8; y += 2 )
        {
            uint16_t * const hd_row = (uint16_t *)(hdvram + BM2HD_R(y) / 4); // 2 pixels
            
            uint32_t* hdp; // that's a moving pointer
            for (int x = os.x0 + 8; x < os.x_max - 8; x += 2)
            {
                hdp = (uint32_t *)(hd_row + BM2HD_X(x));
                //~ hdp = hdvram + BM2HD(x,y)/4;
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
                
                /** simple Laplacian filter
                 *     -1
                 *  -1  4 -1
                 *     -1
                 * 
                 * Big endian:
                 *  uyvy uyvy uyvy
                 *  uyvy uYvy uyvy
                 *  uyvy uyvy uyvy
                 */
                #define P (vram_hd.pitch/4)
                #define p_cc ((int32_t)((*(hdp  )) >>  8) & 0xFF)
                #define p_rc ((int32_t)((*(hdp  )) >> 24) & 0xFF)
                #define p_lc ((int32_t)((*(hdp-1)) >> 24) & 0xFF)
                #define p_cu ((int32_t)((*(hdp-P)) >>  8) & 0xFF)
                #define p_cd ((int32_t)((*(hdp+P)) >>  8) & 0xFF)
                
                #define e_laplacian_x  ABS(p_cc * 2 - p_lc - p_rc)
                #define e_laplacian_xy ABS(p_cc * 4 - p_lc - p_rc - p_cu - p_cd)
                #define e_dx           ABS(p_rc - p_cc)
                #define e_dy           ABS(p_cd - p_cc)

                int e = focus_peaking_method == 0 ? MAX(e_dx, e_dy) : e_laplacian_xy ;
                #undef a
                #undef b
                #undef c
                #undef d
                #undef z
                #undef P
                
                /*if (focus_peaking_debug)
                {
                    int c = (COERCE(e, 0, thr*2) * 41 / thr / 2) + 38;
                    bvram[BM(x,y)] = c | (c << 8);
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
                    
                    uint16_t * const b_row = (uint16_t*)( bvram + BM_R(y) );   // 2 pixels
                    uint16_t * const m_row = (uint16_t*)( bvram_mirror + BM_R(y) );   // 2 pixels
                    
                    uint16_t pixel = b_row[x/2];
                    uint16_t mirror = m_row[x/2];
                    uint16_t pixel2 = b_row[x/2 + BMPPITCH/2];
                    uint16_t mirror2 = m_row[x/2 + BMPPITCH/2];
                    if (mirror  & 0x8080) continue;
                    if (mirror2 & 0x8080) continue;
                    if (pixel  != 0 && pixel  != mirror ) continue;
                    if (pixel2 != 0 && pixel2 != mirror2) continue;

                    b_row[x/2] = b_row[x/2 + BMPPITCH/2] = 
                    m_row[x/2] = m_row[x/2 + BMPPITCH/2] = color;
                    if (dirty_pixels_num < MAX_DIRTY_PIXELS)
                    {
                        dirty_pixels[dirty_pixels_num++] = BM(x,y);
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
    
    int zd = Z && zebra_draw && (expsim || PLAY_MODE) && (zebra_rec || !recording); // when to draw zebras
    if (zd)
    {
        int zlh = zebra_level_hi * 255 / 100;
        int zll = zebra_level_lo * 255 / 100;

        uint8_t * const lvram = get_yuv422_vram()->vram;
        
        // draw zebra in 16:9 frame
        // y is in BM coords
        for(int y = os.y0 + os.off_169; y < os.y_max - os.off_169; y += 2 )
        {
            uint32_t color_over = zebra_color_word_row(COLOR_RED, y);
            uint32_t color_under = zebra_color_word_row(COLOR_BLUE, y);
            uint32_t color_over_2 = zebra_color_word_row(COLOR_RED, y+1);
            uint32_t color_under_2 = zebra_color_word_row(COLOR_BLUE, y+1);
            
            uint32_t * const v_row = (uint32_t*)( lvram        + BM2LV_R(y)    );  // 2 pixels
            uint32_t * const b_row = (uint32_t*)( bvram        + BM_R(y)       );  // 4 pixels
            uint32_t * const m_row = (uint32_t*)( bvram_mirror + BM_R(y)       );  // 4 pixels
            
            uint32_t* lvp; // that's a moving pointer through lv vram
            uint32_t* bp;  // through bmp vram
            uint32_t* mp;  // through mirror

            for (int x = os.x0; x < os.x_max; x += 4)
            {
                lvp = v_row + BM2LV_X(x)/2;
                bp = b_row + x/4;
                mp = m_row + x/4;
                #define BP (*bp)
                #define MP (*mp)
                #define BN (*(bp + BMPPITCH/4))
                #define MN (*(mp + BMPPITCH/4))
                if (BP != 0 && BP != MP) { little_cleanup(bp, mp); continue; }
                if (BN != 0 && BN != MN) { little_cleanup(bp + BMPPITCH/4, mp + BMPPITCH/4); continue; }
                if ((MP & 0x80808080) || (MN & 0x80808080)) continue;
                
                if (zebra_colorspace == 1) // rgb
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
                    R = MIN(R, 255);
                    G = MIN(G, 255);
                    B = MIN(B, 255);
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
    if (!get_global_draw()) return;

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

static void
draw_false_downsampled( void )
{
    //~ if (vram_width > 720) return;
    if (!PLAY_MODE)
    {
        if (!expsim) return;
    }
    
    // exception: green screen palette is not fixed
    if (falsecolor_palette == 5)
    {
        aj_green_screen();
        return;
    }

    
    //~ bvram_mirror_init();
    uint8_t * const bvram = bmp_vram_real();
    if (!bvram) return;
    if (!bvram_mirror) return;

    uint8_t * const lvram = get_yuv422_vram()->vram;
    uint8_t* fc = false_colour[falsecolor_palette];

    for(int y = os.y0 + os.off_169; y < os.y_max - os.off_169; y += 2 )
    {
        uint32_t * const v_row = (uint32_t*)( lvram        + BM2LV_R(y)    );  // 2 pixels
        uint16_t * const b_row = (uint16_t*)( bvram        + BM_R(y)       );  // 2 pixels
        uint16_t * const m_row = (uint16_t*)( bvram_mirror + BM_R(y)       );  // 2 pixels
        
        uint8_t* lvp; // that's a moving pointer through lv vram
        uint16_t* bp;  // through bmp vram
        uint16_t* mp;  // through mirror
        
        for (int x = os.x0; x < os.x_max; x += 2)
        {
            lvp = (uint8_t *)(v_row + BM2LV_X(x)/2); lvp++;
            bp = b_row + x/2;
            mp = m_row + x/2;
            
            #define BP (*bp)
            #define MP (*mp)
            #define BN (*(bp + BMPPITCH/2))
            #define MN (*(mp + BMPPITCH/2))
            
            if (BP != 0 && BP != MP) { little_cleanup(bp, mp); continue; }
            if (BN != 0 && BN != MN) { little_cleanup(bp + BMPPITCH/2, mp + BMPPITCH/2); continue; }
            if ((MP & 0x80808080) || (MN & 0x80808080)) continue;
            
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
    int lvpitch = get_yuv422_vram()->pitch;
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

#define MAX_CROP_NAME_LEN 15
#define MAX_CROPMARKS 9
int num_cropmarks = 0;
static char cropmark_names[MAX_CROPMARKS][MAX_CROP_NAME_LEN];

// Cropmark sorting code contributed by Nathan Rosenquist
static void sort_cropmarks()
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
                NotifyBox(2000, "TOO MANY CROPMARKS (max=%d)", MAX_CROPMARKS);
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
        cropmark_clear_cache();
    }
    
    i = COERCE(i, 0, num_cropmarks-1);
    char bmpname[100];
    snprintf(bmpname, sizeof(bmpname), CARD_DRIVE "CROPMKS/%s", cropmark_names[i]);
    cropmarks = bmp_load(bmpname,1);
    if (!cropmarks) bmp_printf(FONT_LARGE, 0, 50, "LOAD ERROR %d:%s   ", i, bmpname);
    cropmark_cache_valid = 0;
}

static void
crop_toggle( void* priv, int sign )
{
    crop_index = mod(crop_index + sign, num_cropmarks);
    reload_cropmark(crop_index);
    crop_set_dirty(10);
}

static void
zebra_draw_display( void * priv, int x, int y, int selected )
{
    unsigned z = *(unsigned*) priv;
    if (!z)
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Zebras      : OFF"
        );
    else
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Zebras      : %s, %d..%d%%",
            zebra_colorspace ? "RGB" : "Luma",
            zebra_level_lo, zebra_level_hi
        );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(z));
}

static void
zebra_level_display( void * priv, int x, int y, int selected )
{
    unsigned level = *(unsigned*) priv;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "%s : %d%% (%d)",
        priv == &zebra_level_lo ? "Underexposure" : 
                                  "Overexposure ",
        level, 0, 
        (level * 255 + 50) / 100
    );
}
static void
zebra_toggle( void* priv, int sign )
{
    menu_ternary_toggle(priv, -sign);
}

static char* falsecolor_palette_name()
{
    return
        falsecolor_palette == 0 ? "Marshall" :
        falsecolor_palette == 1 ? "SmallHD" :
        falsecolor_palette == 2 ? "50-55%" :
        falsecolor_palette == 3 ? "67-72%" :
        falsecolor_palette == 4 ? "Banding detection" :
        falsecolor_palette == 5 ? "GreenScreen" : "Unk";
}

static void falsecolor_palette_preview(int x, int y)
{
    for (int i = 0; i < 256; i++)
    {
        draw_line(x + 419 + i, y, x + 419 + i, y + font_large.height, false_colour[falsecolor_palette][i]);
    }
}

static void
falsecolor_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "False Color : %s",
        falsecolor_draw ? falsecolor_palette_name() : "OFF"
    );
    if (falsecolor_draw)
        falsecolor_palette_preview(x, y);
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(falsecolor_draw));
}

static void
falsecolor_display_palette( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Palette : %s",
        falsecolor_palette_name()
    );
    falsecolor_palette_preview(x - 420, y + font_large.height + 10);
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
            focus_peaking_method == 0 ? "D1xy" : "D2xy",
            focus_peaking_pthr / 10, focus_peaking_pthr % 10, 
            focus_peaking_color == 0 ? "R" :
            focus_peaking_color == 1 ? "G" :
            focus_peaking_color == 2 ? "B" :
            focus_peaking_color == 3 ? "C" :
            focus_peaking_color == 4 ? "M" :
            focus_peaking_color == 5 ? "Y" :
            focus_peaking_color == 6 ? "global" :
            focus_peaking_color == 7 ? "local" : "err"
        );
    else
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Focus Peak  : OFF"
        );
    menu_draw_icon(x, y, MNI_BOOL_GDR(f));
}

static void focus_peaking_adjust_thr(void* priv, int delta)
{
    focus_peaking_pthr = (int)focus_peaking_pthr + (focus_peaking_pthr < 10 ? 1 : 5) * delta;
    if ((int)focus_peaking_pthr > 50) focus_peaking_pthr = 1;
    if ((int)focus_peaking_pthr <= 0) focus_peaking_pthr = 50;
}

static void
crop_display( void * priv, int x, int y, int selected )
{
    //~ extern int retry_count;
    int index = crop_index;
    index = COERCE(index, 0, num_cropmarks-1);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Cropmarks   : %s",
         crop_enabled  ? cropmark_names[index] : "OFF"
    );
    if (crop_enabled && cropmark_movieonly && !is_movie_mode())
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Cropmarks are only displayed in movie mode");
    menu_draw_icon(x, y, MNI_BOOL_GDR(crop_enabled));
}

static void
crop_display_submenu( void * priv, int x, int y, int selected )
{
    //~ extern int retry_count;
    int index = crop_index;
    index = COERCE(index, 0, num_cropmarks);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bitmap (%d/%d)  : %s",
         index+1, num_cropmarks,
         cropmark_names[index]
    );
    int h = 150;
    int w = h * 720 / 480;
    int xc = x + 315;
    int yc = y + font_large.height * 3 + 10;
    reload_cropmark(crop_index);
    bmp_fill(0, xc, yc, w, h);
    bmp_draw_scaled_ex(cropmarks, xc, yc, w, h, 0);
    bmp_draw_rect(COLOR_WHITE, xc, yc, w, h);
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

/*static void
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
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(hist_draw || waveform_draw));
}*/

static void
hist_print( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Histogram   : %s%s%s",
        hist_draw == 0 ? "OFF" : hist_colorspace == 0 ? "Luma" : "RGB",
        hist_draw == 0 ? "" : hist_log ? ",Log" : ",Lin",
        hist_draw && hist_warn ? ",clip warn" : ""
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(hist_draw));
}

static void
hist_warn_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Clip warning  : %s",
        hist_warn == 0 ? "OFF" :
        hist_warn == 1 ? "0.001% px" :
        hist_warn == 2 ? "0.01% px" :
        hist_warn == 3 ? "0.1% px" : "1% px"
    );
    menu_draw_icon(x, y, MNI_BOOL(hist_warn), 0);
}

static void
waveform_print( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Waveform    : %s",
        waveform_draw == 0 ? "OFF" : 
        waveform_size == 0 ? "Small" : 
        waveform_size == 1 ? "Large" : 
        waveform_size == 2 ? "FullScreen" : "err"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(waveform_draw));
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
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(*(unsigned*) priv));
}


static void
clearscreen_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int mode = clearscreen;
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "ClearScreen : %s",
        //~ mode ? "ON (HalfShutter)" : "OFF"
        mode == 0 ? "OFF" : 
        mode == 1 ? "HalfShutter/DOF" : 
        mode == 2 ? "WhenIdle" : "Always"
    );
}

static void
zoom_overlay_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    zoom_overlay_size = mod(zoom_overlay_size, 3);

    if (!zoom_overlay_enabled)
    {
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Magic Zoom  : OFF");
        return;
    }

    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Magic Zoom  : %s%s%s%s%s",
        zoom_overlay_trigger_mode == 0 ? "err" :
#ifdef CONFIG_5D2
        zoom_overlay_trigger_mode == 1 ? "HalfS," :
        zoom_overlay_trigger_mode == 2 ? "Focus," :
        zoom_overlay_trigger_mode == 3 ? "F+HS," : "ALW,",
#else
        zoom_overlay_trigger_mode == 1 ? "Zrec," :
        zoom_overlay_trigger_mode == 2 ? "F+Zr," :
        zoom_overlay_trigger_mode == 3 ? "(+)," : "ALW,",
#endif

        zoom_overlay_trigger_mode == 0 ? "" :
            zoom_overlay_size == 0 ? "Small," :
            zoom_overlay_size == 1 ? "Med," :
            zoom_overlay_size == 2 ? "Large," : "err",

        zoom_overlay_trigger_mode == 0 ? "" :
            zoom_overlay_pos == 0 ? "AFF," :
            zoom_overlay_pos == 1 ? "NW," :
            zoom_overlay_pos == 2 ? "NE," :
            zoom_overlay_pos == 3 ? "SE," :
            zoom_overlay_pos == 4 ? "SW," : "err",

        zoom_overlay_trigger_mode == 0 ? "" :
            zoom_overlay_x2 == 0 ? "1:1" : "2:1",

        zoom_overlay_trigger_mode == 0 ? "" :
            zoom_overlay_split == 0 ? "" :
            zoom_overlay_split == 1 ? ",Ss" :
            zoom_overlay_split == 2 ? ",Sz" : "err"

    );

    if (ext_monitor_rca)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Magic Zoom does not work with SD monitors");
    else if (is_movie_mode() && video_mode_resolution)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Magic Zoom does not work well in current video mode");
    else if (zoom_overlay_trigger_mode && !get_zoom_overlay_trigger_mode() && get_global_draw()) // MZ enabled, but for some reason it doesn't work in current mode
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Magic Zoom is not available in this mode");
    else
        menu_draw_icon(x, y, MNI_BOOL_GDR(zoom_overlay_trigger_mode));
}


static void
spotmeter_menu_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{

    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Spotmeter   : %s%s",
        spotmeter_draw == 0    ? "OFF" : 
        spotmeter_formula == 0 ? "Percent" :
        spotmeter_formula == 1 ? "0..255" :
        spotmeter_formula == 2 ? "IRE -1..101" 
                               : "IRE 0..108",
        spotmeter_draw && spotmeter_position ? ", AFF" : ""
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(spotmeter_draw));
}

void get_spot_yuv(int dxb, int* Y, int* U, int* V)
{
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return;
    const uint16_t*     vr = (void*) YUV422_LV_BUFFER_DMA_ADDR;
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    int                 x, y;

    int xcb = os.x0 + os.x_ex/2;
    int ycb = os.y0 + os.y_ex/2;
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_X(ycb);
    int dxl = BM2LV_DX(dxb);

    bmp_draw_rect(COLOR_WHITE, xcb - dxb, ycb - dxb, 2*dxb, 2*dxb);
    
    unsigned sy = 0;
    int32_t su = 0, sv = 0; // Y is unsigned, U and V are signed
    // Sum the values around the center
    for( y = ycl - dxl ; y <= ycl + dxl ; y++ )
    {
        for( x = xcl - dxl ; x <= xcl + dxl ; x++ )
        {
            uint16_t p = vr[ x + y * width ];
            sy += (p >> 8) & 0xFF;
            if (x % 2) sv += (int8_t)(p & 0x00FF); else su += (int8_t)(p & 0x00FF);
        }
    }

    sy /= (2 * dxl + 1) * (2 * dxl + 1);
    su /= (dxl + 1) * (2 * dxl + 1);
    sv /= (dxl + 1) * (2 * dxl + 1);

    *Y = sy;
    *U = su;
    *V = sv;
}

int get_spot_motion(int dxb, int draw)
{
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return 0;
    const uint16_t*     vr1 = (void*)YUV422_LV_BUFFER_DMA_ADDR;
    const uint16_t*     vr2 = (void*)get_fastrefresh_422_buf();
    uint8_t * const     bm = bmp_vram();
    if (!bm) return 0;
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    int                 x, y;

    int xcb = os.x0 + os.x_ex/2;
    int ycb = os.y0 + os.y_ex/2;
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_X(ycb);
    int dxl = BM2LV_DX(dxb);

    draw_line(xcb - dxb, ycb - dxb, xcb + dxb, ycb - dxb, COLOR_WHITE);
    draw_line(xcb + dxb, ycb - dxb, xcb + dxb, ycb + dxb, COLOR_WHITE);
    draw_line(xcb + dxb, ycb + dxb, xcb - dxb, ycb + dxb, COLOR_WHITE);
    draw_line(xcb - dxb, ycb + dxb, xcb - dxb, ycb - dxb, COLOR_WHITE);
    
    unsigned D = 0;
    for( y = ycl - dxl ; y <= ycl + dxl ; y++ )
    {
        for( x = xcl - dxl ; x <= xcl + dxl ; x++ )
        {
            int p1 = (vr1[ x + y * width ] >> 8) & 0xFF;
            int p2 = (vr2[ x + y * width ] >> 8) & 0xFF;
            int dif = ABS(p1 - p2);
            D += dif;
            if (draw) bm[x + y * BMPPITCH] = false_colour[4][dif & 0xFF];
        }
    }
    
    D = D * 2;
    D /= (2 * dxl + 1) * (2 * dxl + 1);
    return D;
}

int get_spot_focus(int dxb)
{
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return 0;
    const uint32_t*     vr = (uint32_t*) vram->vram; // 2px
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    int                 x, y;
    
    unsigned sf = 0;
    unsigned br = 0;

    int xcb = os.x0 + os.x_ex/2;
    int ycb = os.y0 + os.y_ex/2;
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_X(ycb);
    int dxl = BM2LV_DX(dxb);
    
    // Sum the absolute difference of values around the center
    for( y = ycl - dxl ; y <= ycl + dxl ; y++ )
    {
        for( x = xcl - dxl ; x <= xcl + dxl ; x++ )
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

static void spotmeter_step()
{
    //~ if (!lv) return;
    if (!PLAY_MODE)
    {
        if (!expsim) return;
    }
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return;
    
    const uint16_t*     vr = (uint16_t*) vram->vram;
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    const unsigned      dxb = spotmeter_size;
    unsigned        sum = 0;
    int                 x, y;

    int xcb = os.x0 + os.x_ex/2;
    int ycb = os.y0 + os.y_ex/2;
    
    if (spotmeter_position == 1) // AF frame
    {
        get_afframe_pos(os.x_ex, os.y_ex, &xcb, &ycb);
        xcb += os.x0;
        ycb += os.y0;
    }
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_X(ycb);
    int dxl = BM2LV_DX(dxb);
    
    // Sum the values around the center
    for( y = ycl - dxl ; y <= ycl + dxl ; y++ )
    {
        for( x = xcl - dxl ; x <= xcl + dxl ; x++ )
            sum += (vr[ x + y * width]) & 0xFF00;
    }

    sum /= (2 * dxl + 1) * (2 * dxl + 1);

    // Scale to 100%
    const unsigned      scaled = (100 * sum) / 0xFF00;
    
    // spotmeter color: 
    // black on transparent, if brightness > 60%
    // white on transparent, if brightness < 50%
    // previous value otherwise
    
    // if false color is active, draw white on semi-transparent gray

    // protect the surroundings from zebras
    uint32_t* M = get_bvram_mirror();
    uint32_t* B = bmp_vram();

    for( y = (ycb&~1) - 13 ; y <= (ycb&~1) + 36 ; y++ )
    {
        for( x = xcb - 26 ; x <= xcb + 26 ; x+=4 )
        {
            uint8_t* m = &(M[BM(x,y)/4]);
            if (!(*m & 0x80)) *m = 0x80;
            m++;
            if (!(*m & 0x80)) *m = 0x80;
            m++;
            if (!(*m & 0x80)) *m = 0x80;
            m++;
            if (!(*m & 0x80)) *m = 0x80;
            B[BM(x,y)/4] = 0;
        }
    }
    
    static int fg = 0;
    if (scaled > 60) fg = COLOR_BLACK;
    if (scaled < 50 || falsecolor_draw) fg = COLOR_WHITE;
    int bg = fg == COLOR_BLACK ? COLOR_WHITE : COLOR_BLACK;
    int fnt = FONT(SHADOW_FONT(FONT_MED), fg, bg);
    int fnts = FONT(SHADOW_FONT(FONT_SMALL), fg, bg);

    bmp_draw_rect(COLOR_WHITE, xcb - dxb, ycb - dxb, 2*dxb+1, 2*dxb+1);
    bmp_draw_rect(COLOR_BLACK, xcb - dxb + 1, ycb - dxb + 1, 2*dxb+1-2, 2*dxb+1-2);
    ycb += dxb + 20;
    ycb -= font_med.height/2;
    xcb -= 2 * font_med.width;

    if (spotmeter_formula <= 1)
    {
        bmp_printf(
            fnt,
            xcb, ycb, 
            "%3d%s",
            spotmeter_formula == 0 ? scaled : sum >> 8,
            spotmeter_formula == 0 ? "%" : ""
        );
    }
    else
    {
        int ire_aj = (((int)sum >> 8) - 2) * 102 / 253 - 1; // formula from AJ: (2...255) -> (-1...101)
        int ire_piers = ((int)sum >> 8) * 108/255;           // formula from Piers: (0...255) -> (0...108)
        int ire = (spotmeter_formula == 2) ? ire_aj : ire_piers;
        
        bmp_printf(
            fnt,
            xcb, ycb, 
            "%s%3d", // why does %4d display garbage?!
            ire < 0 ? "-" : " ",
            ire < 0 ? -ire : ire
        );
        bmp_printf(
            fnts,
            xcb + font_med.width*4, ycb,
            "IRE\n%s",
            spotmeter_formula == 2 ? "-1..101" : "0..108"
        );
    }
}


static void
disp_profiles_0_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "DISP profiles    : %d", 
        disp_profiles_0 + 1
    );
}


static void
transparent_overlay_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
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
    menu_draw_icon(x, y, MNI_BOOL_GDR(transparent_overlay));
    transparent_overlay_hidden = 0;
}

static void transparent_overlay_offset(int dx, int dy)
{
    transparent_overlay_offx = COERCE((int)transparent_overlay_offx + dx, -650, 650);
    transparent_overlay_offy = COERCE((int)transparent_overlay_offy + dy, -400, 400);
    transparent_overlay_hidden = 0;
    BMP_LOCK( show_overlay(); )
}

static void transparent_overlay_center_or_toggle()
{
    if (transparent_overlay_offx || transparent_overlay_offy) // if off-center, just center it
    {
        transparent_overlay_offset_clear(0, 0);
        transparent_overlay_offset(0, 0);
    }
    else // if centered, hide it or show it back
    {
        transparent_overlay_hidden = !transparent_overlay_hidden;
        if (!transparent_overlay_hidden) BMP_LOCK( show_overlay(); )
        else redraw();
    }
}

static void transparent_overlay_offset_clear(void* priv, int delta)
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

static char* idle_time_format(int t)
{
    static char msg[50];
    if (t) snprintf(msg, sizeof(msg), "after %d%s", t < 60 ? t : t/60, t < 60 ? "sec" : "min");
    else snprintf(msg, sizeof(msg), "OFF");
    return msg;
}

static void
idle_display_dim_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
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
    void *          priv,
    int         x,
    int         y,
    int         selected
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
    void *          priv,
    int         x,
    int         y,
    int         selected
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
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Turn off GlobalDraw: %s",
        idle_time_format(*(int*)priv)
    );
}

static int timeout_values[] = {0, 5, 10, 20, 30, 60, 120, 300, 600, 900};

static int current_timeout_index(int t)
{
    int i;
    for (i = 0; i < COUNT(timeout_values); i++)
        if (t == timeout_values[i]) return i;
    return 0;
}

static void idle_timeout_toggle(void* priv, int sign)
{
    int* t = (int*)priv;
    int i = current_timeout_index(*t);
    i = mod(i + sign, COUNT(timeout_values));
    *(int*)priv = timeout_values[i];
}

static CONFIG_INT("defish.preview", defish_preview, 0);
static void
defish_preview_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Defishing   : %s",
        defish_preview ? "ON" : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR(defish_preview));
}


CONFIG_INT("electronic.level", electronic_level, 0);
static void
electronic_level_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Level Indic.: %s",
        electronic_level ? "ON" : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR(electronic_level));
}

static void clearscreen_now()
{
    gui_stop_menu();
    bmp_on();
    bmp_off();
}

struct menu_entry zebra_menus[] = {
    {
        .name = "Global Draw",
        .priv       = &global_draw,
        .select     = menu_binary_toggle,
        .display    = global_draw_display,
        .help = "Enable/disable ML overlay graphics (zebra, cropmarks...)",
        .essential = FOR_LIVEVIEW,
    },
    {
        .name = "Zebras",
        .priv       = &zebra_draw,
        .select     = menu_binary_toggle,
        .display    = zebra_draw_display,
        .help = "Zebra stripes: show overexposed or underexposed areas.",
        .essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
            {
                .name = "Color space",
                .priv = &zebra_colorspace, 
                .max = 1,
                .choices = (const char *[]) {"Luma", "RGB"},
                .icon_type = IT_NAMED_COLOR,
                .help = "Luma: red/blue. RGB: color is reverse of clipped channel.",
            },
            {
                .name = "Underexposure",
                .priv = &zebra_level_lo, 
                .min = 0,
                .max = 20,
                .display = zebra_level_display,
                .help = "Underexposure threshold (0=disable).",
            },
            {
                .name = "Overexposure", 
                .priv = &zebra_level_hi,
                .min = 80,
                .max = 100,
                .display = zebra_level_display,
                .help = "Overexposure threshold (100=disable).",
            },
            {
                .name = "When recording", 
                .priv = &zebra_rec,
                .max = 1,
                .choices = (const char *[]) {"Hide", "Show"},
                .help = "You can hide zebras when recording.",
                .icon_type = IT_DISABLE_SOME_FEATURE_NEG,
            },
            MENU_EOL
        },
    },
    {
        .name = "Focus Peak",
        .priv           = &focus_peaking,
        .display        = focus_peaking_display,
        .select         = menu_binary_toggle,
        .help = "Show tiny dots on focused edges.",
        .essential = FOR_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Method",
                .priv = &focus_peaking_method, 
                .max = 1,
                .choices = (const char *[]) {"1st deriv.", "2nd deriv."},
                .help = "Edge detection method.",
            },
            {
                .name = "Threshold", 
                .priv = &focus_peaking_pthr,
                .select = focus_peaking_adjust_thr,
                .help = "How many pixels are considered in focus (percentage).",
                .unit = UNIT_PERCENT_x10
            },
            {
                .name = "Color", 
                .priv = &focus_peaking_color,
                .max = 7,
                .choices = (const char *[]) {"Red", "Green", "Blue", "Cyan", "Magenta", "Yellow", "Global Focus", "Local Focus"},
                .help = "Focus peaking color (fixed or color coding).",
                .icon_type = IT_NAMED_COLOR,
            },
            MENU_EOL
        },
    },
    {
        .name = "Magic Zoom",
        .priv = &zoom_overlay_enabled,
        .display = zoom_overlay_display,
        .min = 0,
        .max = 1,
        .help = "Zoom box for checking focus. Can be used while recording.",
        .essential = FOR_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger mode",
                .priv = &zoom_overlay_trigger_mode, 
                .min = 1,
                .max = 4,
                #ifdef CONFIG_5D2
                .choices = (const char *[]) {"OFF", "HalfShutter", "Focus Ring", "FocusR+HalfS", "Always On"},
                .help = "Trigger MZ by focus ring or half-shutter.",
                #else
                .choices = (const char *[]) {"OFF", "Zoom.REC", "Focus+ZREC", "ZoomIn (+)", "Always On"},
                .help = "Zoom when recording / trigger from focus ring / Zoom button",
                #endif
            },
            {
                .name = "Size", 
                .priv = &zoom_overlay_size,
                .max = 2,
                .choices = (const char *[]) {"Small", "Medium", "Large"},
                .icon_type = IT_SIZE,
                .help = "Size of zoom box (small / medium / large).",
            },
            {
                .name = "Position", 
                .priv = &zoom_overlay_pos,
                .max = 4,
                .choices = (const char *[]) {"AF Frame", "NorthWest", "NorthEast", "SouthEast", "SouthWest"},
                .icon_type = IT_DICE,
                .help = "Position of zoom box (fixed or linked to AF frame).",
            },
            {
                .name = "Magnification", 
                .priv = &zoom_overlay_x2,
                .max = 1,
                .choices = (const char *[]) {"1:1", "2:1"},
                .icon_type = IT_SIZE,
                .help = "Magnification: 2:1 doubles the pixels.",
            },
            #ifndef CONFIG_50D
            {
                .name = "Focus confirm", 
                .priv = &zoom_overlay_split,
                .max = 2,
                .choices = (const char *[]) {"Green Bars", "SplitScreen", "SS ZeroCross"},
                .icon_type = IT_DICE,
                .help = "How to show focus confirmation (green bars / split screen).",
            },
            #endif
            {
                .name = "Look-up Table", 
                .priv = &zoom_overlay_lut,
                .max = 1,
                .choices = (const char *[]) {"OFF", "CineStyle"},
                .help = "LUT for increasing contrast in the zoom box.",
            },
            MENU_EOL
        },
    },
    {
        .name = "Cropmarks",
        .priv = &crop_enabled,
        .display    = crop_display,
        .select     = menu_binary_toggle,
        .help = "Cropmarks or custom grids for framing.",
        .essential = FOR_MOVIE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Bitmap",
                .priv = &crop_index, 
                .select = crop_toggle,
                .display    = crop_display_submenu,
                .icon_type = IT_ALWAYS_ON,
                .help = "You can draw your own cropmarks in Paint.",
            },
            {
                .name = "Show in photo mode",
                .priv = &cropmark_movieonly, 
                .max = 1,
                .choices = (const char *[]) {"ON", "OFF"},
                .help = "Cropmarks are mostly used in movie mode.",
            },
            {
                .name = "Show in PLAY mode ",
                .priv = &cropmarks_play, 
                .max = 1,
                .help = "You may also have cropmarks in Playback mode.",
            },
            MENU_EOL
        },
    },
    {
        .name = "Ghost image",
        .priv = &transparent_overlay, 
        .display = transparent_overlay_display, 
        .select = menu_binary_toggle,
        .help = "Overlay any image in LiveView. In PLAY mode, press LV btn.",
        .essential = FOR_PLAYBACK,
    },
    #ifndef CONFIG_5D2
    {
        .name = "Defishing",
        .priv = &defish_preview, 
        .display = defish_preview_display, 
        .select = menu_binary_toggle,
        .help = "Preview rectilinear image from Samyang 8mm fisheye.",
        .essential = FOR_PLAYBACK,
    },
    #endif
    {
        .name = "Spotmeter",
        .priv           = &spotmeter_draw,
        .select         = menu_binary_toggle,
        .display        = spotmeter_menu_display,
        .help = "Exposure aid: display brightness from a small spot.",
        .essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
            {
                .name = "Position",
                .priv = &spotmeter_position, 
                .max = 1,
                .choices = (const char *[]) {"Center", "AF Frame"},
                .icon_type = IT_DICE,
                .help = "Spotmeter position: center or attached to AF frame.",
            },
            {
                .name = "Unit",
                .priv = &spotmeter_formula, 
                .max = 3,
                .choices = (const char *[]) {"Percent", "0..255", "IRE -1..101", "IRE 0..108"},
                .icon_type = IT_DICE,
                .help = "Measurement unit for brightness level.",
            },
            MENU_EOL
        }
    },
    {
        .name = "False color",
        .priv       = &falsecolor_draw,
        .display    = falsecolor_display,
        .select     = menu_binary_toggle,
        .help = "Exposure aid: each brightness level is color-coded.",
        .essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
            {
                .name = "Palette",
                .priv = &falsecolor_palette, 
                .max = COUNT(false_colour)-1,
                .icon_type = IT_DICE,
                .display = falsecolor_display_palette,
                .help = "False color palettes for exposure, banding, green screen...",
            },
            MENU_EOL
        }
    },
/*  {
        .name = "Histo/Wavefm",
        .priv       = &hist_draw,
        .select     = zebra_toggle,
        .select_auto = waveform_toggle,
        .display    = hist_display,
        .help = "Histogram [SET] and Waveform [Q] for evaluating exposure.",
        .essential = FOR_LIVEVIEW | FOR_PLAYBACK,
    },
    */
    {
        .name = "Histogram",
        .priv       = &hist_draw,
        .max = 1,
        .display = hist_print,
        .help = "Exposure aid: shows the distribution of brightness levels.",
        .essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
            {
                .name = "Color space",
                .priv = &hist_colorspace, 
                .max = 1,
                .choices = (const char *[]) {"Luma", "RGB"},
                .icon_type = IT_NAMED_COLOR,
                .help = "Color space for histogram: Luma channel (YUV) / RGB.",
            },
            {
                .name = "Scaling",
                .priv = &hist_log, 
                .max = 1,
                .choices = (const char *[]) {"Linear", "Logarithmic"},
                .help = "Linear or logarithmic histogram.",
                .icon_type = IT_DICE,
            },
            {
                .name = "Clip warning",
                .priv = &hist_warn, 
                .max = 4,
                .display = hist_warn_display,
                .help = "Display warning dots when one color channel is clipped.",
            },
            MENU_EOL
        },
    },
    {
        .name = "Waveform",
        .priv       = &waveform_draw,
        .display = waveform_print,
        .max = 1,
        .help = "Exposure aid: useful for checking overall brightness.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Size",
                .priv = &waveform_size, 
                .max = 2,
                .choices = (const char *[]) {"Small", "Large", "FullScreen"},
                .icon_type = IT_SIZE,
                .help = "Waveform size: Small / Large / FullScreen.",
            },
            MENU_EOL
        },
    },
    #ifdef CONFIG_60D
    {
        .name = "Level Indic.", 
        .priv = &electronic_level, 
        .select = menu_binary_toggle, 
        .display = electronic_level_display,
        .help = "Electronic level indicator in 0.5 degree steps.",
        .essential = FOR_LIVEVIEW,
    },
    #endif
    {
        .name = "ClearScreen",
        .priv           = &clearscreen_enabled,
        .display        = clearscreen_display,
        .select         = menu_binary_toggle,
        .help = "Clear bitmap overlays from LiveView display.",
        .essential = FOR_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Mode",
                .priv = &clearscreen_mode, 
                .min = 0,
                .max = 2,
                .choices = (const char *[]) {"HalfShutter", "WhenIdle", "Always"},
                .icon_type = IT_DICE,
                .help = "Clear screen when you hold shutter halfway or when idle.",
            },
            MENU_EOL
        },
    },
    /*{
        .priv           = &focus_graph,
        .display        = focus_graph_display,
        .select         = menu_binary_toggle,
    },*/
    //~ {
        //~ .display        = crop_off_display,
        //~ .select         = crop_off_toggle,
        //~ .select_reverse = crop_off_toggle_rev, 
    //~ },
    
    //~ {
        //~ .priv = "[debug] HDMI test", 
        //~ .display = menu_print, 
        //~ .select = hdmi_test_toggle,
    //~ }
    //~ {
        //~ .priv       = &edge_draw,
        //~ .select     = menu_binary_toggle,
        //~ .display    = edge_display,
    //~ },
    //~ {
        //~ .priv       = &waveform_draw,
        //~ .select     = menu_binary_toggle,
        //~ .display    = waveform_display,
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
        .priv       = &unified_loop,
        .select     = menu_ternary_toggle,
        .display    = unified_loop_display,
        .help = "Unique loop for zebra and FP. Used with HDMI and 720p."
    },
    {
        .priv       = &zebra_density,
        .select     = menu_ternary_toggle,
        .display    = zebra_mode_display,
    },
    {
        .priv       = &use_hd_vram,
        .select     = menu_binary_toggle,
        .display    = use_hd_vram_display,
    },
    {
        .priv = &focus_peaking_debug,
        .select = menu_binary_toggle, 
        .display = focus_debug_display,
    }*/
};

#if defined(CONFIG_60D) || defined(CONFIG_5D2)
static void batt_display(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int l = GetBatteryLevel();
    int r = GetBatteryTimeRemaining();
    int d = GetBatteryDrainRate();
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Battery level: %d%%, %dh%02dm, %d%%/h",
        l, 0, 
        r / 3600, (r % 3600) / 60,
        d, 0
    );
    menu_draw_icon(x, y, MNI_ON, 0);
}
#endif

struct menu_entry powersave_menus[] = {
    {
        .name = "Dim display",
        .priv           = &idle_display_dim_after,
        .display        = idle_display_dim_print,
        .select         = idle_timeout_toggle,
        .help = "Dim LCD display in LiveView when idle, to save power."
    },
    {
        .name = "Turn off LCD",
        .priv           = &idle_display_turn_off_after,
        .display        = idle_display_turn_off_print,
        .select         = idle_timeout_toggle,
        .help = "Turn off display and pause LiveView when idle and not REC."
    },
    {
        .name = "Turn off GlobalDraw",
        .priv           = &idle_display_global_draw_off_after,
        .display        = idle_display_global_draw_off_print,
        .select         = idle_timeout_toggle,
        .help = "Turn off GlobalDraw when idle, to save some CPU cycles."
    },
    {
        .name = "Save power when REC",
        .priv           = &idle_rec,
        .display        = idle_rec_print,
        .select         = menu_binary_toggle,
        .help = "If enabled, powersave (see above) works when recording too."
    },
    #if defined(CONFIG_60D) || defined(CONFIG_5D2)
    {
        .name = "Battery remaining",
        .display = batt_display,
        .help = "Battery remaining. Wait for 2%% discharge before reading.",
        //~ .essential = FOR_MOVIE | FOR_PHOTO,
    },
    #endif
};

struct menu_entry livev_cfg_menus[] = {
    {
        .name = "DISP presets",
        .priv       = &disp_profiles_0,
        .select     = menu_quaternary_toggle,
        .display    = disp_profiles_0_display,
        .help = "No.of LiveV display presets. Switch with "
                #ifdef CONFIG_550D
                "ISO+Disp or Flash."
                #endif
                #ifdef CONFIG_500D
                "ISO+Disp."
                #endif
                #ifdef CONFIG_600D
                "ISO+Info or Disp."
                #endif
                #ifdef CONFIG_60D
                "Metering button."
                #endif
                #ifdef CONFIG_5D2
                "PicStyle button."
                #endif
    },
};


/*PROP_HANDLER(PROP_MVR_REC_START)
{
    if (buf[0] != 1) redraw_after(2000);
    return prop_cleanup( token, property );
}*/

void cropmark_draw_from_cache()
{
    uint8_t* B = bmp_vram();
    uint8_t* M = get_bvram_mirror();
    
    for (int i = os.y0; i < os.y_max; i++)
    {
        for (int j = os.x0; j < os.x_max; j++)
        {
            uint8_t p = B[BM(j,i)];
            uint8_t m = M[BM(j,i)];
            if (!(m & 0x80)) continue;
            if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue;
            B[BM(j,i)] = m & ~0x80;
        }
    }
}

void cropmark_clear_cache()
{
    if (cropmark_cache_valid)
    {
        clrscr_mirror();
        bvram_mirror_clear();
        cropmark_cache_valid = 0;
    }
}

static void 
cropmark_draw()
{
    //~ ChangeColorPaletteLV(2);
    if (!get_global_draw()) return;

    get_yuv422_vram(); // just to refresh VRAM params
    clear_lv_affframe_if_dirty();

    default_movie_cropmarks();

    if (transparent_overlay && !transparent_overlay_hidden)
    {
        show_overlay();
        zoom_overlay_dirty = 1;
        cropmark_cache_valid = 0;
    }
    crop_dirty = 0;

    if (
            (!crop_enabled) ||
            (cropmark_movieonly && !is_movie_mode() && !PLAY_MODE)
       )
    {
        cropmark_clear_cache();
        return;
    }
    
    reload_cropmark(crop_index); // reloads only when changed
    if (cropmarks) 
    {
        clrscr_mirror();
        //~ bmp_printf(FONT_MED, 0, 0, "%x %x %x %x %d", os.x0, os.y0, os.x_ex, os.y_ex, PLAY_MODE);

        if (cropmark_cache_valid)
        {
            cropmark_draw_from_cache();
            //~ bmp_printf(FONT_MED, 50, 50, "crop cached");
        }
        else
        {
            bmp_draw_scaled_ex(cropmarks, os.x0, os.y0, os.x_ex, os.y_ex, bvram_mirror);
            cropmark_cache_valid = 1;
            //~ bmp_printf(FONT_MED, 50, 50, "crop regen");
        }
        zoom_overlay_dirty = 1;
    }
    crop_dirty = 0;
}

static void
cropmark_cache_check()
{
    if (!cropmark_cache_valid) return;

    get_yuv422_vram(); // update VRAM params if needed

    // check if cropmark cache is still valid
    int sig = os.x0*811 + os.y0*467 + os.x_ex*571 + os.y_ex*487 + (is_movie_mode() ? 113 : 0);
    static int prev_sig = 0;
    if (prev_sig != sig)
    {
        //~ NotifyBox(2000, "cropmark refresh");
        cropmark_clear_cache();
    }
    //~ bmp_printf(FONT_LARGE, 0, 0, "crop sig: %x ", sig);
    prev_sig = sig;
}

static void
cropmark_redraw()
{
    if (!zebra_should_run() && !PLAY_MODE) return;
    cropmark_cache_check();
    BMP_LOCK( cropmark_draw(); )
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

void bmp_on()
{
    //~ return;
    //~ if (!is_safe_to_mess_with_the_display(500)) return;
    if (_bmp_cleared) 
    {// BMP_LOCK(GMT_LOCK( if (is_safe_to_mess_with_the_display(0)) {call("MuteOff"); _bmp_cleared = 0;}))
    #if defined(CONFIG_500D) || defined(CONFIG_50D)// || defined(CONFIG_5D2)
        canon_gui_enable_front_buffer(1);
    #else
        BMP_LOCK(
            cli_save();
            if (tft_status == 0 && lv && !lv_paused)
            {
                MuteOff_0();
            }
            sei_restore();
        )
    #endif
        _bmp_cleared = false;
    }
}
void bmp_on_force()
{
    _bmp_cleared = true;
    bmp_on();
}
void bmp_off()
{
    //~ return;
    //~ clrscr();
    //~ if (!is_safe_to_mess_with_the_display(500)) return;
    if (!_bmp_cleared) //{ BMP_LOCK(GMT_LOCK( if (is_safe_to_mess_with_the_display(0)) { call("MuteOn")); ) }}
    {
    #if defined(CONFIG_500D) || defined(CONFIG_50D)// || defined(CONFIG_5D2)
        _bmp_cleared = true;
        canon_gui_disable_front_buffer();
        clrscr();
    #else
        BMP_LOCK(
            cli_save();
            if (tft_status == 0 && lv && !lv_paused)
            {
                _bmp_cleared = true;
                MuteOn_0();
            }
            sei_restore();
        )
    #endif
    }
}

void bmp_mute_flag_reset()
{
    _bmp_cleared = 0;
}

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
    zoom_overlay_triggered_by_zoom_btn = !zoom_overlay_triggered_by_zoom_btn;
    if (!zoom_overlay_triggered_by_zoom_btn)
    {
        zoom_overlay_triggered_by_focus_ring_countdown = 0;
        //~ crop_set_dirty(10);
        //~ redraw_after(500);
    }
}

int handle_zoom_overlay(struct event * event)
{
    if (gui_menu_shown()) return 1;
    if (!lv) return 1;

#ifdef CONFIG_5D2
    if (event->param == BGMT_PRESS_HALFSHUTTER && get_zoom_overlay_trigger_by_halfshutter())
        zoom_overlay_toggle();
    if (is_zoom_overlay_triggered_by_zoom_btn() && !get_zoom_overlay_trigger_by_halfshutter())
        zoom_overlay_toggle();
#else
    // zoom in when recording => enable Magic Zoom 
    if (get_zoom_overlay_trigger_mode() && recording == 2 && MVR_FRAME_NUMBER > 50 && event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
    {
        zoom_overlay_toggle();
        return 0;
    }

    // if magic zoom is enabled, Zoom In should always disable it 
    if (is_zoom_overlay_triggered_by_zoom_btn() && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
    {
        zoom_overlay_toggle();
        return 0;
    }
    
    if (get_zoom_overlay_trigger_mode() && lv_dispsize == 1 && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
    {
        // magic zoom toggled by sensor+zoom in (modes Zr and Zr+F)
        if (get_zoom_overlay_trigger_mode() < 3 && get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED)
        {
            zoom_overlay_toggle();
            return 0;
        }
        // (*): magic zoom toggled by zoom in, normal zoom by sensor+zoom in
        else if (get_zoom_overlay_trigger_mode() == MZ_TAKEOVER_ZOOM_IN_BTN && !get_halfshutter_pressed() && !(get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED))
        {
            zoom_overlay_toggle();
            return 0;
        }
    }
#endif

    // move AF frame when recording
    if (recording && liveview_display_idle() && is_manual_focus())
    {
        if (event->param == BGMT_PRESS_LEFT)
            { move_lv_afframe(-200, 0); return 0; }
        if (event->param == BGMT_PRESS_RIGHT)
            { move_lv_afframe(200, 0); return 0; }
        if (event->param == BGMT_PRESS_UP)
            { move_lv_afframe(0, -200); return 0; }
        if (event->param == BGMT_PRESS_DOWN)
            { move_lv_afframe(0, 200); return 0; }
        #if !defined(CONFIG_50D) && !defined(CONFIG_500D) && !defined(CONFIG_5D2)
        if (event->param == BGMT_PRESS_SET)
            { center_lv_afframe(); return 0; }
        #endif
    }

    return 1;
}
//~ void zoom_overlay_enable()
//~ {
    //~ zoom_overlay_triggered_by_zoom_btn = 1;
//~ }

void zoom_overlay_disable()
{
    zoom_overlay_triggered_by_zoom_btn = 0;
    zoom_overlay_triggered_by_focus_ring_countdown = 0;
}

void zoom_overlay_set_countdown(int x)
{
    zoom_overlay_triggered_by_focus_ring_countdown = x;
}

static void yuvcpy_x2(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
    src = (void*)((unsigned int)src & 0xFFFFFFFC);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 2)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        *(dst) = chroma | (luma1 << 8) | (luma1 << 24);
        *(dst+1) = chroma | (luma2 << 8) | (luma2 << 24);
    }
}

static const unsigned char TechnicolLUT[256] = {
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,3,3,3,4,4,5,5,5,6,6,7,8,8,9,9,10,10,11,12,12,13,14,15,15,16,17,18,18,19,20,21,22,23,24,25,26,26,27,28,29,30,32,33,34,35,36,37,38,39,40,41,43,44,45,46,47,49,50,51,52,54,55,56,58,59,60,62,63,64,66,67,68,70,71,73,74,75,77,78,80,81,83,84,86,87,89,90,92,93,95,96,98,99,101,102,104,105,107,109,110,112,113,115,116,118,119,121,123,124,126,127,129,130,132,134,135,137,138,140,141,143,145,146,148,149,151,152,154,155,157,158,160,161,163,164,166,167,169,170,172,173,175,176,178,179,181,182,183,185,186,188,189,190,192,193,194,196,197,198,200,201,202,204,205,206,207,209,210,211,212,213,214,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,235,236,237,238,239,239,240,241,242,242,243,244,244,245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,252,253,253,253,253,254,254,254,254,255,255,255,255,255,255,255,255,255,255 
};

static void yuvcpy_lut(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
    src = (void*)((unsigned int)src & 0xFFFFFFFC);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst++)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        *(dst) = chroma | ((uint32_t)(TechnicolLUT[luma1]) << 8) | ((uint32_t)(TechnicolLUT[luma2]) << 24);
    }
}

static void yuvcpy_x2_lut(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
    src = (void*)((unsigned int)src & 0xFFFFFFFC);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 2)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        luma1 = TechnicolLUT[luma1];
        luma2 = TechnicolLUT[luma2];
        *(dst) = chroma | (luma1 << 8) | (luma1 << 24);
        *(dst+1) = chroma | (luma2 << 8) | (luma2 << 24);
    }
}


static void draw_zoom_overlay(int dirty)
{   
    //~ if (vram_width > 720) return;
    if (!lv) return;
    if (!get_global_draw()) return;
    //~ if (gui_menu_shown()) return;
    if (!bmp_is_on()) return;
    if (lv_dispsize != 1) return;
    //~ if (get_halfshutter_pressed() && clearscreen != 2) return;
    if (recording == 1) return;
    
    #ifdef CONFIG_50D
    zoom_overlay_split = 0; // 50D doesn't report focus
    #endif
    
    struct vram_info *  lv = get_yuv422_vram();
    struct vram_info *  hd = get_yuv422_hd_vram();
    
    //~ lv->width = 1920;

    if( !lv->vram ) return;
    if( !hd->vram ) return;
    if( !bmp_vram()) return;

    uint16_t*       lvr = (uint16_t*) lv->vram;
    uint16_t*       hdr = (uint16_t*) hd->vram;
    
    if (!lvr) return;

    // center of AF frame
    int aff_x0_lv, aff_y0_lv; 
    get_afframe_pos(720, 480, &aff_x0_lv, &aff_y0_lv);
    aff_x0_lv = N2LV_X(aff_x0_lv);
    aff_y0_lv = N2LV_Y(aff_y0_lv);
    
    int aff_x0_hd = LV2HD_X(aff_x0_lv);
    int aff_y0_hd = LV2HD_Y(aff_y0_lv);
    
    //~ int aff_x0_bm = LV2BM_X(aff_x0_lv);
    //~ int aff_y0_bm = LV2BM_Y(aff_y0_lv);
    
    int W = os.x_ex / 3;
    int H = os.y_ex / 2;
    
    switch(zoom_overlay_size)
    {
        case 0:
        case 3:
            W = os.x_ex / 5;
            H = os.y_ex / 4;
            break;
        case 1:
        case 4:
            W = os.x_ex / 3;
            H = os.y_ex * 2/5;
            break;
        case 2:
        case 5:
            W = os.x_ex/2;
            H = os.y_ex/2;
            break;
        case 6:
            W = 720;
            H = 480;
            break;
    }
    
    int x2 = zoom_overlay_x2;

    int zb_x0_lv, zb_y0_lv; // center of zoom box

    switch(zoom_overlay_pos)
    {
        case 0: // AFF
            zb_x0_lv = aff_x0_lv;
            zb_y0_lv = aff_y0_lv;
            break;
        case 1: // NW
            zb_x0_lv = W/2 + 50;
            zb_y0_lv = H/2 + 50;
            break;
        case 2: // NE
            zb_x0_lv = BM2LV_X(os.x_max) - W/2 - 50;
            zb_y0_lv = H/2 + 50;
            break;
        case 3: // SE
            zb_x0_lv = BM2LV_X(os.x_max) - W/2 - 50;
            zb_y0_lv = BM2LV_Y(os.y_max) - H/2 - 50;
            break;
        case 4: // SV
            zb_x0_lv = W/2 + 50;
            zb_y0_lv = BM2LV_Y(os.y_max) - H/2 - 50;
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

    //~ bmp_printf(FONT_LARGE, 50, 50, "%d,%d %d,%d", W, H, aff_x0_lv);

    if (zoom_overlay_pos)
    {
        int w = W * lv->width / hd->width;
        int h = H * lv->width / hd->width;
        if (x2)
        {
            w >>= 1;
            h >>= 1;
        }
        if (video_mode_resolution == 0 || !is_movie_mode())
        {
            memset(lvr + COERCE(aff_x0_lv - (w>>1), 0, 720-w) + COERCE(aff_y0_lv - (h>>1) - 1, 0, lv->height) * lv->width, 0,    w<<1);
            memset(lvr + COERCE(aff_x0_lv - (w>>1), 0, 720-w) + COERCE(aff_y0_lv - (h>>1) - 2, 0, lv->height) * lv->width, 0xFF, w<<1);
            memset(lvr + COERCE(aff_x0_lv - (w>>1), 0, 720-w) + COERCE(aff_y0_lv + (h>>1) + 1, 0, lv->height) * lv->width, 0xFF, w<<1);
            memset(lvr + COERCE(aff_x0_lv - (w>>1), 0, 720-w) + COERCE(aff_y0_lv + (h>>1) + 2, 0, lv->height) * lv->width, 0,    w<<1);
        }
    }

    //~ draw_circle(x0,y0,45,COLOR_WHITE);
    int y;
    int x0c = COERCE(zb_x0_lv - (W>>1), 0, lv->width-W);
    int y0c = COERCE(zb_y0_lv - (H>>1), 0, lv->height-H);

    extern int focus_value;
    extern int focus_min_value;
    //~ bmp_printf(FONT_MED, 300, 100, "%d %d ", focus_value, focus_min_value);
    int rawoff = COERCE(80 - focus_value, 0, 100) >> 2;
    if (focus_min_value > 60) rawoff = 1; // out of focus?
    
    // reverse the sign of split when perfect focus is achieved
    static int rev = 0;
    static int poff = 0;
    if (rawoff != 0 && poff == 0) rev = !rev;
    poff = rawoff;
    if (zoom_overlay_split == 1 /* non zerocross */) rev = 0;

    if (x2)
    {
        uint16_t* d = lvr + x0c + (y0c + 2) * lv->width;
        uint16_t* s = hdr + (aff_y0_hd - (H>>2)) * hd->width + (aff_x0_hd - (W>>2));
        for (y = 2; y < H-2; y++)
        {
            int off = zoom_overlay_split ? (y < H/2 ? rawoff : -rawoff) : 0;
            if (rev) off = -off;
            if (zoom_overlay_lut) yuvcpy_x2_lut((uint32_t*)d, (uint32_t*)(s + off), W>>1);
            else yuvcpy_x2((uint32_t*)d, (uint32_t*)(s + off), W>>1);
            d += lv->width;
            if (y & 1) s += hd->width;
        }
    }
    else
    {
        uint16_t* d = lvr + x0c + (y0c + 2) * lv->width;
        uint16_t* s = hdr + (aff_y0_hd - (H>>1)) * hd->width + (aff_x0_hd - (W>>1));
        for (y = 2; y < H-2; y++)
        {
            int off = zoom_overlay_split ? (y < H/2 ? rawoff : -rawoff) : 0;
            if (rev) off = -off;
            if (zoom_overlay_lut) yuvcpy_lut((uint32_t *)d, (uint32_t *)(s + off), W);
            else memcpy(d, s + off, W<<1);
            d += lv->width;
            s += hd->width;
        }
    }

    if (video_mode_resolution == 0 || !is_movie_mode())
    {
        memset(lvr + x0c + COERCE(0   + y0c, 0, 720) * lv->width, rawoff ? 0    : 0x80, W<<1);
        memset(lvr + x0c + COERCE(1   + y0c, 0, 720) * lv->width, rawoff ? 0xFF : 0x80, W<<1);
        memset(lvr + x0c + COERCE(H-1 + y0c, 0, 720) * lv->width, rawoff ? 0xFF : 0x80, W<<1);
        memset(lvr + x0c + COERCE(H   + y0c, 0, 720) * lv->width, rawoff ? 0    : 0x80, W<<1);
    }
    if (dirty) bmp_fill(0, LV2BM_X(x0c), LV2BM_Y(y0c) + 2, LV2BM_DX(W), LV2BM_DY(H) - 4);
    //~ bmp_fill(rawoff ? COLOR_BLACK : COLOR_GREEN1, x0c, y0c, W, 1);
    //~ bmp_fill(rawoff ? COLOR_WHITE : COLOR_GREEN2, x0c+1, y0c, W, 1);
    //~ bmp_fill(rawoff ? COLOR_WHITE : COLOR_GREEN2, x0c, y0c + H - 1, W, 1);
    //~ bmp_fill(rawoff ? COLOR_BLACK : COLOR_GREEN1, x0c, y0c + H, W, 1);
}

//~ int zebra_paused = 0;
//~ void zebra_pause() { zebra_paused = 1; }
//~ void zebra_resume() { zebra_paused = 0; }

bool liveview_display_idle()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    extern thunk LiveViewApp_handler;
    extern uintptr_t new_LiveViewApp_handler;
    extern thunk test_minimal_handler;

    if (dialog->handler == (dialog_handler_t) &test_minimal_handler)
    { // ML is clearing the screen with a fake dialog, let's see what's underneath
        current = current->next;
        dialog = current->priv;
    }

    return
        lv && 
        tft_status == 0 &&
        !menu_active_and_not_hidden() && 
        (gui_menu_shown() || // force LiveView when menu is active, but hidden
            ( gui_state == GUISTATE_IDLE && 
              (dialog->handler == (dialog_handler_t) &LiveViewApp_handler || dialog->handler == (dialog_handler_t) new_LiveViewApp_handler) &&
            CURRENT_DIALOG_MAYBE <= 3 && 
            #ifdef CURRENT_DIALOG_MAYBE_2
            CURRENT_DIALOG_MAYBE_2 <= 3 &&
            #endif
            lens_info.job_state < 10 &&
            !mirror_down )
        );
        //~ !zebra_paused &&
        //~ !(clearscreen == 1 && (get_halfshutter_pressed() || dofpreview));
}


// when it's safe to draw zebras and other on-screen stuff
int zebra_should_run()
{
    return liveview_display_idle() && get_global_draw() &&
        lv_dispsize == 1 &&
        !(clearscreen == 1 && (get_halfshutter_pressed() || dofpreview));
}

static void zebra_sleep_when_tired()
{
    if (!zebra_should_run())
    {
        while (clearscreen == 1 && (get_halfshutter_pressed() || dofpreview)) msleep(100);
        if (zebra_should_run()) return;
        //~ if (!gui_menu_shown()) ChangeColorPaletteLV(4);
        if (lv && !gui_menu_shown()) redraw();
#ifdef CONFIG_60D
        disable_electronic_level();
#endif
        while (!zebra_should_run()) msleep(100);
        //~ ChangeColorPaletteLV(2);
        if (!gui_menu_shown()) crop_set_dirty(5);
        vram_params_set_dirty();

        default_movie_cropmarks();
        //~ if (lv && !gui_menu_shown()) redraw();
    }
}

void draw_livev_for_playback()
{
    get_yuv422_vram(); // just to refresh VRAM params
    clrscr();
    
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
        defish_draw_play();
    }
    else
    {
        draw_zebra_and_focus(1,0);
    }
    
    draw_histogram_and_waveform();
)
}

static void draw_histogram_and_waveform()
{
    if (menu_active_and_not_hidden()) return;
    if (!get_global_draw()) return;
    
    if (hist_draw || waveform_draw)
    {
        hist_build();
    }
    
    if (menu_active_and_not_hidden()) return; // hack: not to draw histo over menu
    if (!get_global_draw()) return;
    
    if( hist_draw)
        BMP_LOCK( hist_draw_image( os.x_max - hist_width - 5, os.y0 + 100, -1); )

    if (menu_active_and_not_hidden()) return;
    if (!get_global_draw()) return;
        
    if( waveform_draw)
        BMP_LOCK( waveform_draw_image( os.x_max - WAVEFORM_WIDTH*WAVEFORM_FACTOR, os.y_max - WAVEFORM_HEIGHT*WAVEFORM_FACTOR - WAVEFORM_OFFSET ); )
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
        msleep(MIN_MSLEEP); // safety msleep :)
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

static int idle_countdown_display_dim = 50;
static int idle_countdown_display_off = 50;
static int idle_countdown_globaldraw = 50;
static int idle_countdown_clrscr = 50;
static int idle_countdown_display_dim_prev = 50;
static int idle_countdown_display_off_prev = 50;
static int idle_countdown_globaldraw_prev = 50;
static int idle_countdown_clrscr_prev = 50;

#ifdef CONFIG_KILL_FLICKER
static int idle_countdown_killflicker = 5;
static int idle_countdown_killflicker_prev = 5;
#endif

void idle_wakeup_reset_counters(int reason) // called from handle_buttons
{
#if 0
    NotifyBox(2000, "wakeup: %d   ", reason);
#endif

    //~ bmp_printf(FONT_LARGE, 50, 50, "wakeup: %d   ", reason);
    
    if (lv && !lv_paused && reason == GMT_OLC_INFO_CHANGED) return;

    // when sensor is covered, timeout changes to 3 seconds
    int sensor_status = get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED;

    // those are for powersaving
    idle_countdown_display_off = sensor_status ? 25 : idle_display_turn_off_after * 10;
    idle_countdown_display_dim = sensor_status ? 25 : idle_display_dim_after * 10;
    idle_countdown_globaldraw  = sensor_status ? 25 : idle_display_global_draw_off_after * 10;

    if (reason == -2345) // disable powersave during recording 
        return;

    // those are not for powersaving
    idle_countdown_clrscr = 30;
    
    if (reason == -10 || reason == -11) // focus event (todo: should define constants for those)
        return;
    
#ifdef CONFIG_KILL_FLICKER
    idle_countdown_killflicker = 10;
#endif
}

// called at 10 Hz
static void update_idle_countdown(int* countdown)
{
    //~ bmp_printf(FONT_MED, 200, 200, "%d  ", *countdown);
    if ((liveview_display_idle() && !get_halfshutter_pressed()) || lv_paused)
    {
        if (*countdown)
            (*countdown)--;
    }
    else
    {
        idle_wakeup_reset_counters(-100); // will reset all idle countdowns
    }
    
    int sensor_status = get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED;
    static int prev_sensor_status = 0;

    if (sensor_status != prev_sensor_status)
        idle_wakeup_reset_counters(-1);
    
    prev_sensor_status = sensor_status;
}

static void idle_action_do(int* countdown, int* prev_countdown, void(*action_on)(void), void(*action_off)(void))
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
        //~ while (get_halfshutter_pressed()) msleep(MIN_MSLEEP);
        BMP_LOCK(
            prop_request_change(PROP_LV_ACTION, &x, 4);
            msleep(100);
            clrscr();
            lv_paused = 1;
            lv = 1;
        )
    }
}

void ResumeLiveView()
{
    if (lv && lv_paused)
    {
        lv = 0;
        int x = 0;
        //~ while (get_halfshutter_pressed()) msleep(MIN_MSLEEP);
        BMP_LOCK(
            prop_request_change(PROP_LV_ACTION, &x, 4);
            while (!lv) msleep(100);
        )
        msleep(300);
    }
    lv_paused = 0;
}

static void idle_display_off()
{
    extern int motion_detect;

    wait_till_next_second();

    if (motion_detect || recording)
    {
        NotifyBox(3000, "DISPLAY OFF");
    }
    else
    {
        NotifyBox(3000, "DISPLAY AND SENSOR OFF");
    }

    if (!(get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED))
    {
        for (int i = 0; i < 30; i++)
        {
            if (idle_countdown_display_off) { NotifyBoxHide(); return; }
            msleep(100);
        }
    }
    if (!(motion_detect || recording)) PauseLiveView();
    display_off_force();
    msleep(100);
    idle_countdown_display_off = 0;
}
static void idle_display_on()
{
    //~ card_led_blink(5, 50, 50);
    ResumeLiveView();
    display_on_force();
    redraw();
}

static void idle_bmp_off()
{
    bmp_off();
}
static void idle_bmp_on()
{
    bmp_on();
}

static int old_backlight_level = 0;
static void idle_display_dim()
{
    old_backlight_level = backlight_level;
    set_backlight_level(1);
}
static void idle_display_undim()
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

/*void clear_liveview_area()
{
    if (is_movie_mode())
        bmp_fill(0, os.x0, os.y0 + os.off_169, os.x_ex, os.y_ex - os.off_169 * 2 + 2);
    else
        bmp_fill(0, os.x0, os.y0, os.x_ex, os.y_ex);
}*/

static void idle_kill_flicker()
{
    if (!canon_gui_front_buffer_disabled())
    {
        canon_gui_disable_front_buffer();
        clrscr();
        if (is_movie_mode())
        {
            black_bars_16x9();
            if (recording)
                maru(os.x_max - 28, os.y0 + 12, COLOR_RED);
        }
    }
}
static void idle_stop_killing_flicker()
{
    if (canon_gui_front_buffer_disabled())
    {
        canon_gui_enable_front_buffer(1);
    }
}

static PROP_INT(PROP_LOGICAL_CONNECT, logical_connect); // EOS utility?

static void
clearscreen_task( void* unused )
{
    idle_wakeup_reset_counters(0);
    //~ #ifdef CONFIG_KILL_FLICKER
    //~ idle_stop_killing_flicker();
    //~ #endif

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
            info_led_blink(1, 20, 20);

        if (!lv) continue;


        //~ if (lv_disp_mode == 0 && LV_BOTTOM_BAR_DISPLAYED && !get_halfshutter_pressed())
            //~ canon_gui_disable_front_buffer();
        //~ else canon_gui_enable_front_buffer(0);

        // especially for 50D
        #ifdef CONFIG_KILL_FLICKER
        if (kill_canon_gui_mode == 1)
        {
            if (global_draw && !gui_menu_shown())
            {
                if (liveview_display_idle())
                {
                    if (!canon_gui_front_buffer_disabled())
                        idle_kill_flicker();
                }
                else
                {
                    if (canon_gui_front_buffer_disabled())
                        idle_stop_killing_flicker();
                }
            }
        }
        #endif
        
        if (clearscreen == 3)
        {
            if (liveview_display_idle())
            {
                bmp_off();
            }
            else
            {
                bmp_on();
            }
        }
        
        /*if (k % 10 == 0)
        {
            bmp_printf(FONT_MED, 50, 50, "%d fps ", fps_ticks);
            fps_ticks = 0;
        }*/

        // clear overlays on shutter halfpress
        if (clearscreen == 1 && (get_halfshutter_pressed() || dofpreview) && !gui_menu_shown())
        {
            #ifdef CONFIG_KILL_FLICKER
            idle_stop_killing_flicker();
            #endif

            BMP_LOCK( clrscr_mirror(); )
            int i;
            for (i = 0; i < (int)clearscreen_delay/20; i++)
            {
                if (i % 10 == 0 && zebra_should_run()) BMP_LOCK( update_lens_display(); )
                msleep(20);
                if (!(get_halfshutter_pressed() || dofpreview))
                    goto clearscreen_loop;
            }
            bmp_off();
            while ((get_halfshutter_pressed() || dofpreview)) msleep(100);
            bmp_on();
        }
        //~ else if (clearscreen == 2)  // always clear overlays
        //~ {
            //~ idle_action_do(&idle_countdown_display_clear, bmp_off, bmp_on);
        //~ }

        if (recording && !idle_rec) // don't go to powersave when recording
            idle_wakeup_reset_counters(-2345);
        
        if (logical_connect)
            idle_wakeup_reset_counters(-305); // EOS utility
        
        if (idle_display_dim_after)
            idle_action_do(&idle_countdown_display_dim, &idle_countdown_display_dim_prev, idle_display_dim, idle_display_undim);

        if (idle_display_turn_off_after)
            idle_action_do(&idle_countdown_display_off, &idle_countdown_display_off_prev, idle_display_off, idle_display_on);

        if (idle_display_global_draw_off_after)
            idle_action_do(&idle_countdown_globaldraw, &idle_countdown_globaldraw_prev, idle_globaldraw_dis, idle_globaldraw_en);

        if (clearscreen == 2) // clear overlay when idle
            idle_action_do(&idle_countdown_clrscr, &idle_countdown_clrscr_prev, idle_bmp_off, idle_bmp_on);
        
        #ifdef CONFIG_KILL_FLICKER
        if (kill_canon_gui_mode == 2) // LV transparent menus and key presses
        {
            if (global_draw && !gui_menu_shown())
                idle_action_do(&idle_countdown_killflicker, &idle_countdown_killflicker_prev, idle_kill_flicker, idle_stop_killing_flicker);
        }
        #endif

        // since this task runs at 10Hz, I prefer cropmark redrawing here
        if (crop_dirty)
        {
            crop_dirty--;

            // if cropmarks are cached, we can redraw them faster

            cropmark_cache_check();
            
            if (transparent_overlay) cropmark_cache_valid = 0;
            
            if (cropmark_cache_valid && !should_draw_zoom_overlay() && !get_halfshutter_pressed())
                crop_dirty = MIN(crop_dirty, 2);

            if (!crop_enabled) crop_dirty = 0;
                
            if (crop_dirty == 0)
                crop_redraw_flag = 1;
        }
    }
}

TASK_CREATE( "cls_task", clearscreen_task, 0, 0x1a, 0x1000 );

//~ CONFIG_INT("disable.redraw", disable_redraw, 0);
CONFIG_INT("display.dont.mirror", display_dont_mirror, 1);

// this should be synchronized with
// * graphics code (like zebra); otherwise zebras will remain frozen on screen
// * gui_main_task (to make sure Canon won't call redraw in parallel => crash)
void redraw_do()
{
    extern int ml_started;
    if (!ml_started) return;
    
BMP_LOCK (

#if !defined(CONFIG_50D) && !defined(CONFIG_500D) && !defined(CONFIG_5D2)
    if (display_dont_mirror && display_dont_mirror_dirty)
    {
        if (lcd_position == 1) NormalDisplay();
        display_dont_mirror_dirty = 0;
    }
#endif

    //~ if (disable_redraw) 
    //~ {
        //~ clrscr(); // safest possible redraw method :)
    //~ }
    //~ else
    {
        struct gui_task * current = gui_task_list.current;
        struct dialog * dialog = current->priv;
        if (dialog && MEM(dialog->type) == DLG_SIGNATURE) // if dialog seems valid
        {
            #ifdef CONFIG_KILL_FLICKER
            // to redraw, we need access to front buffer
            int d = canon_gui_front_buffer_disabled();
            canon_gui_enable_front_buffer(0);
            #endif
            
            dialog_redraw(dialog); // try to redraw (this has semaphores for winsys)
            
            #ifdef CONFIG_KILL_FLICKER
            // restore things back
            if (d) idle_kill_flicker();
            #endif
        }
        else
        {
            clrscr(); // out of luck, fallback
        }
    }
)

    // ask other stuff to redraw
    afframe_set_dirty();

    if (cropmark_cache_valid) cropmark_redraw();
    else crop_set_dirty(10);
    
    menu_set_dirty();
    zoom_overlay_dirty = 1;
}

void redraw()
{
    fake_simple_button(MLEV_REDRAW);
}

/*void test_fps(int* x)
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
*/


static void false_color_toggle()
{
    falsecolor_draw = !falsecolor_draw;
    if (falsecolor_draw) zoom_overlay_disable();
}

static int transparent_overlay_flag = 0;
void schedule_transparent_overlay()
{
    transparent_overlay_flag = 1;
}

volatile int lens_display_dirty = 0;
void lens_display_set_dirty() { lens_display_dirty = 1; }

void draw_cropmark_area()
{
    bmp_draw_rect(COLOR_BLUE, os.x0, os.y0, os.x_ex, os.y_ex);
    draw_line(os.x0, os.y0, os.x_max, os.y_max, COLOR_BLUE);
    draw_line(os.x0, os.y_max, os.x_max, os.y0, COLOR_BLUE);
    
    bmp_draw_rect(COLOR_RED, HD2BM_X(0), HD2BM_Y(0), HD2BM_DX(vram_hd.width), HD2BM_DY(vram_hd.height));
    draw_line(HD2BM_X(0), HD2BM_Y(0), HD2BM_X(vram_hd.width), HD2BM_Y(vram_hd.height), COLOR_RED);
    draw_line(HD2BM_X(0), HD2BM_Y(vram_hd.height), HD2BM_X(vram_hd.width), HD2BM_Y(0), COLOR_RED);
}


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
        //~ vsync(&YUV422_LV_BUFFER_DMA_ADDR);
        fps_ticks++;

        while (is_mvr_buffer_almost_full())
            msleep(100);
        
        get_422_hd_idle_buf(); // just to keep it up-to-date
        
        zebra_sleep_when_tired();

        #if 0
        draw_cropmark_area(); // just for debugging
        #endif

        if (should_draw_zoom_overlay())
        {
            msleep(k % 50 == 0 ? MIN_MSLEEP : 10);
            guess_fastrefresh_direction();
            if (zoom_overlay_dirty) BMP_LOCK( clrscr_mirror(); )
            BMP_LOCK( if (lv) draw_zoom_overlay(zoom_overlay_dirty); )
            zoom_overlay_dirty = 0;
            //~ crop_set_dirty(10); // don't draw cropmarks while magic zoom is active
            // but redraw them after MZ is turned off
        }
        else
        {
            if (!zoom_overlay_dirty) crop_set_dirty(5);
            
            msleep(MIN_MSLEEP);
            zoom_overlay_dirty = 1;
            if (falsecolor_draw)
            {
                if (k % 2 == 0)
                    BMP_LOCK( if (lv) draw_false_downsampled(); )
            }
            else if (defish_preview)
            {
                if (k % 2 == 0)
                    BMP_LOCK( if (lv) defish_draw(); )
            }
            else
            {
                BMP_LOCK( if (lv) draw_zebra_and_focus(k % 4 == 1, k % 2 == 0); )
            }
            if (MIN_MSLEEP <= 10) msleep(MIN_MSLEEP);
        }

        
        if (spotmeter_draw && k % 8 == 3)
            BMP_LOCK( if (lv) spotmeter_step(); )

        #ifdef CONFIG_60D
        if (electronic_level && k % 8 == 5)
            BMP_LOCK( show_electronic_level(); )
        #endif

        if (k % 8 == 7) rec_notify_continuous(0);
        
        if (zoom_overlay_triggered_by_focus_ring_countdown)
        {
            zoom_overlay_triggered_by_focus_ring_countdown--;
        }
        
        //~ if ((lv_disp_mode == 0 && LV_BOTTOM_BAR_DISPLAYED) || get_halfshutter_pressed())
            //~ crop_set_dirty(20);
        
        //~ if (lens_display_dirty)
        if ((k % 50 == 0 || (lens_display_dirty && k % 10 == 0)) && !gui_menu_shown())
        //~ if (k % 5 == 0 && !gui_menu_shown())
        {
            //~ #ifdef CONFIG_KILL_FLICKER
            //~ if (lv && is_movie_mode() && !crop_draw) BMP_LOCK( bars_16x9_50D(); )
            //~ #endif

            #if defined(CONFIG_550D) || defined(CONFIG_5D2)
            BMP_LOCK( black_bars(); )
            #endif

            BMP_LOCK( update_lens_display(); );
            lens_display_dirty = 0;

            movie_indicators_show();
        }

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
    msleep(20);
    while (is_mvr_buffer_almost_full()) msleep(100);
}

static void black_bars()
{
    if (!get_global_draw()) return;
    if (!is_movie_mode()) return;
    int i,j;
    uint8_t * const bvram = bmp_vram();
    uint8_t * const bvram_mirror = get_bvram_mirror();
    for (i = os.y0; i < MIN(os.y_max+1, BMP_HEIGHT); i++)
    {
        if (i < os.y0 + os.off_169 || i > os.y_max - os.off_169)
        {
            int newcolor = (i < os.y0 + os.off_169 - 2 || i > os.y_max - os.off_169 + 2) ? COLOR_BLACK : COLOR_BG;
            for (j = os.x0; j < os.x_max; j++)
            {
                if (bvram[BM(j,i)] == COLOR_BG)
                    bvram[BM(j,i)] = newcolor;
            }
        }
    }
}

static void default_movie_cropmarks()
{
    if (!get_global_draw()) return;
    if (!is_movie_mode()) return;
    int i,j;
    uint8_t * const bvram = bmp_vram();
    uint8_t * const bvram_mirror = get_bvram_mirror();
    for (i = os.y0; i < MIN(os.y_max+1, BMP_HEIGHT); i++)
    {
        if (i < os.y0 + os.off_169 || i > os.y_max - os.off_169)
        {
            int newcolor = (i < os.y0 + os.off_169 - 2 || i > os.y_max - os.off_169 + 2) ? COLOR_BLACK : COLOR_BG;
            for (j = os.x0; j < os.x_max; j++)
            {
                bvram_mirror[BM(j,i)] = newcolor | 0x80;
            }
        }
    }
}

static void black_bars_16x9()
{
#ifdef CONFIG_KILL_FLICKER
    if (!get_global_draw()) return;
    if (video_mode_resolution > 1)
    {
        int off_43 = (os.x_ex - os.x_ex * 8/9) / 2;
        bmp_fill(COLOR_BLACK, os.x0, os.y0, off_43, os.y_ex);
        bmp_fill(COLOR_BLACK, os.x_max - off_43, os.y0, off_43, os.y_ex);
    }
    else
    {
        bmp_fill(COLOR_BLACK, os.x0, os.y0, os.x_ex, os.off_169);
        bmp_fill(COLOR_BLACK, os.x0, os.y_max - os.off_169, os.x_ex, os.off_169);
    }
#endif
}


// Items which do not need a high FPS, but are CPU intensive
// histogram, waveform...
static void
livev_lopriority_task( void* unused )
{
    msleep(2000);
    while(1)
    {
        if (transparent_overlay_flag)
        {
            transparent_overlay_from_play();
            transparent_overlay_flag = 0;
        }

        // here, redrawing cropmarks does not block fast zoom
        if (cropmarks_play && PLAY_MODE)
        {
            cropmark_redraw();
            msleep(300);
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
        
        /*if (menu_upside_down && get_halfshutter_pressed())
        {
            idle_globaldraw_dis();
            BMP_LOCK(
                clrscr_mirror();
                bmp_idle_copy(0);
            )
            kill_flicker();
            msleep(100);
            bmp_flip(bmp_vram_real(), bmp_vram_idle());

            while (get_halfshutter_pressed()) msleep(100);
            idle_globaldraw_en();
            stop_killing_flicker();
        }*/

    }
}

#define HIPRIORITY_TASK_PRIO 0x18

TASK_CREATE( "livev_hiprio_task", livev_hipriority_task, 0, HIPRIORITY_TASK_PRIO, 0x1000 );
TASK_CREATE( "livev_loprio_task", livev_lopriority_task, 0, 0x1f, 0x1000 );

/*static CONFIG_INT("picstyle.disppreset", picstyle_disppreset_enabled, 0);
static unsigned int picstyle_disppreset = 0;
PROP_HANDLER(PROP_PICTURE_STYLE)
{
    update_disp_mode_bits_from_params();
    return prop_cleanup(token, property);
}*/

static int unused = 0;
static unsigned int * disp_mode_params[] = {&crop_enabled, &zebra_draw, &hist_draw, &waveform_draw, &falsecolor_draw, &spotmeter_draw, &clearscreen_enabled, &focus_peaking, &zoom_overlay_split, &global_draw, &zoom_overlay_enabled, &transparent_overlay, &electronic_level, &defish_preview};
static int disp_mode_bits[] =              {4,          2,           2,          2,              2,                2,               2,                       2,             1,                   1,            3,                     2,                    1,                 1};

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
    //~ menu_set_dirty();
    return disp_mode == 0;
}
static void do_disp_mode_change()
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

static void livev_playback_toggle()
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
static void livev_playback_reset()
{
    livev_playback = 0;
}

int handle_livev_playback(struct event * event, int button)
{
    // enable LiveV stuff in Play mode
    if (PLAY_MODE && !gui_menu_shown())
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




static void make_overlay()
{
    draw_cropmark_area();
    msleep(1000);
    //~ bvram_mirror_init();
    clrscr();

    bmp_printf(FONT_MED, 0, 0, "Saving overlay...");

    struct vram_info * vram = get_yuv422_vram();
    uint8_t * const lvram = vram->vram;
    //~ int lvpitch = YUV422_LV_PITCH;
    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    #define BMPPITCH 960

    // difficulty: in play mode, image buffer may have different size/position than in LiveView
    // => normalized xn and yn will fix this
    for (int yn = 0; yn < 480; yn++)
    {
        int y = N2BM_Y(yn);
        //~ int k;
        uint16_t * const v_row = (uint16_t*)( lvram        + BM2LV_R(y)); // 1 pixel
        uint8_t  * const b_row = (uint8_t*) ( bvram        + BM_R(y));    // 1 pixel
        uint8_t  * const m_row = (uint8_t*) ( bvram_mirror + BM_R(yn));    // 1 pixel
        uint16_t* lvp; // that's a moving pointer through lv vram
        uint8_t* bp;   // through bmp vram
        uint8_t* mp;   // through bmp vram mirror
        for (int xn = 0; xn < 720; xn++)
        {
            int x = N2BM_X(xn);
            lvp = v_row + BM2LV_X(x);
            bp = b_row + x;
            mp = m_row + xn;
            *bp = *mp = ((*lvp) * 41 >> 16) + 38;
        }
    }
    FIO_RemoveFile(CARD_DRIVE "overlay.dat");
    FILE* f = FIO_CreateFile(CARD_DRIVE "overlay.dat");
    FIO_WriteFile( f, (const void *) UNCACHEABLE(bvram_mirror), BVRAM_MIRROR_SIZE);
    FIO_CloseFile(f);
    bmp_printf(FONT_MED, 0, 0, "Overlay saved.  ");

    msleep(1000);
}

static void show_overlay()
{
    //~ bvram_mirror_init();
    //~ struct vram_info * vram = get_yuv422_vram();
    //~ uint8_t * const lvram = vram->vram;
    //~ int lvpitch = YUV422_LV_PITCH;
    uint8_t * const bvram = bmp_vram_real();
    if (!bvram) return;
    #define BMPPITCH 960
    
    clrscr();

    FILE* f = FIO_Open(CARD_DRIVE "overlay.dat", O_RDONLY | O_SYNC);
    if (f == INVALID_PTR) return;
    FIO_ReadFile(f, UNCACHEABLE(bvram_mirror), BVRAM_MIRROR_SIZE );
    FIO_CloseFile(f);

    for (int y = os.y0; y < os.y_max; y++)
    {
        int yn = BM2N_Y(y);
        int ym = yn - (int)transparent_overlay_offy; // normalized with offset applied
        //~ int k;
        //~ uint16_t * const v_row = (uint16_t*)( lvram + y * lvpitch );        // 1 pixel
        uint8_t * const b_row = (uint8_t*)( bvram + y * BMPPITCH);          // 1 pixel
        uint8_t * const m_row = (uint8_t*)( bvram_mirror + ym * BMPPITCH);   // 1 pixel
        uint8_t* bp;  // through bmp vram
        uint8_t* mp;  //through bmp vram mirror
        if (ym < 0 || ym > 480) continue;
        //~ int offm = 0;
        //~ int offb = 0;
        //~ if (transparent_overlay == 2) offm = 720/2;
        //~ if (transparent_overlay == 3) offb = 720/2;
        for (int x = os.x0; x < os.x_max; x++)
        {
            int xn = BM2N_X(x);
            int xm = xn - (int)transparent_overlay_offx;
            bp = b_row + x;
            mp = m_row + xm;
            if (((x+y) % 2) && xm >= 0 && xm <= 720)
                *bp = *mp;
        }
    }
    
    bvram_mirror_clear();
    afframe_clr_dirty();
}

void bmp_zoom(uint8_t* dst, uint8_t* src, int x0, int y0, int denx, int deny)
{
    if (!dst) return;
    int i,j;
    for (i = 0; i < vram_bm.height; i++)
    {
        for (j = 0; j < vram_bm.width; j++)
        {
            int is = (i - y0) * deny / 128 + y0;
            int js = (j - x0) * denx / 128 + x0;
            dst[BM(j,i)] = (is >= 0 && js >= 0 && is < vram_bm.height && js < vram_bm.width) 
                ? src[BM(js,is)] : 0;
        }
    }
}

static void transparent_overlay_from_play()
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

//~ CONFIG_STR("defish.lut", defish_lut_file, CARD_DRIVE "recti.lut");
#define defish_lut_file CARD_DRIVE "rectilin.lut"

static uint8_t* defish_lut = INVALID_PTR;

static void defish_lut_load()
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
}

static void defish_draw()
{
    defish_lut_load();
    struct vram_info * vram = get_yuv422_vram();
    uint8_t * const lvram = vram->vram;
    uint8_t * const bvram = bmp_vram_real();
    if (!bvram) return;

    for (int y = os.y0 + (is_movie_mode() ? os.off_169 : 0); y < os.y0 + os.y_ex/2; y += 2)
    {
        for (int x = os.x0; x < os.x0 + os.x_ex/2; x += 4)
        {
            // i,j are normalized values: [0,0 ... 720x480)
            int j = BM2N_X(x);
            int i = BM2N_Y(y);

            static int off_i[] = {0,  0,479,479};
            static int off_j[] = {0,719,  0,719};

            int id = defish_lut[(i * 360 + j) * 2 + 1];
            int jd = defish_lut[(i * 360 + j) * 2] * 360 / 255;
            int k;
            for (k = 0; k < 4; k++)
            {
                int Y = (off_i[k] ? N2BM_Y(off_i[k]) - y + os.y0 - 1 : y);
                int X = (off_j[k] ? N2BM_X(off_j[k]) - x + os.x0 : x);
                int Id = (off_i[k] ? off_i[k] - id : id);
                int Jd = (off_j[k] ? off_j[k] - jd : jd);
                int lv_pixel = lvram[N2LV(Jd&~1,Id&~1) + 1];
                uint32_t* bp = (uint32_t *)&(bvram[BM(X,Y)]);
                uint32_t* mp = (uint32_t *)&(bvram_mirror[BM(X,Y)]);
                if (*bp != 0 && *bp != *mp) continue;
                int c = (lv_pixel * 41 >> 8) + 38;
                c = c | (c << 8);
                c = c | (c << 16);
                *bp = *mp = *(bp + BMPPITCH/4) = *(mp + BMPPITCH/4) = c;
            }
        }
    }
}

static void defish_draw_play()
{
    defish_lut_load();
    struct vram_info * vram = get_yuv422_vram();

    uint32_t * lvram = (uint32_t *)vram->vram;
    uint32_t * aux_buf = (void*)YUV422_HD_BUFFER_2;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    int w = vram->width;
    int h = vram->height;
    int buf_size = w * h * 2;

    memcpy(aux_buf, lvram, buf_size);

    for (int y = BM2LV_Y(os.y0); y < BM2LV_Y(os.y0 + os.y_ex/2); y++)
    {
        for (int x = BM2LV_X(os.x0); x < BM2LV_X(os.x0 + os.x_ex/2); x++)
        {
            // i,j are normalized values: [0,0 ... 720x480)
            int j = LV2N_X(x);
            int i = LV2N_Y(y);

            static int off_i[] = {0,  0,479,479};
            static int off_j[] = {0,719,  0,719};

            int id = defish_lut[(i * 360 + j) * 2 + 1];
            int jd = defish_lut[(i * 360 + j) * 2] * 360 / 255;
            int k;
            for (k = 0; k < 4; k++)
            {
                int Y = (off_i[k] ? N2LV_Y(off_i[k]) - y + BM2LV_Y(os.y0) - 1 : y);
                int X = (off_j[k] ? N2LV_X(off_j[k]) - x + BM2LV_X(os.x0) : x);
                int Id = (off_i[k] ? off_i[k] - id : id);
                int Jd = (off_j[k] ? off_j[k] - jd : jd);
                
                lvram[LV(X,Y)/4] = aux_buf[N2LV(Jd,Id)/4];
            }
        }
    }
}

PROP_HANDLER(PROP_LV_ACTION)
{
    zoom_overlay_triggered_by_focus_ring_countdown = 0;
    idle_display_undim(); // restore LCD brightness, especially for shutdown
    //~ idle_wakeup_reset_counters(-4);
    idle_globaldraw_disable = 0;
    lv_paused = 0;
    bv_auto_update();
    zoom_sharpen_step();
    return prop_cleanup( token, property );
}

static void yuv_resize(uint32_t* src, int src_w, int src_h, uint32_t* dst, int dst_w, int dst_h)
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
    else if (size == 1680*974*2) { w = 1680; h = 974; } // fullhd 600d
    else if (size == 1728*972*2) { w = 1728; h = 972; } // fullhd 3x 600d
    else if (size == 580*580*2)  { w = 580 ; h = 580; }
    else if (size == 1280*580*2) { w = 1280; h = 580; } // 720p 550d, 60d
    else if (size == 1280*560*2) { w = 1280; h = 560; } // 720p 600d
    else if (size == 640*480*2)  { w = 640 ; h = 480; }
    else if (size == 1024*680*2) { w = 1024; h = 680; } // zoom mode (5x, 10x)
    else if (size == 512*340*2)  { w = 512;  h = 340; }
    else if (size == 720*480*2)  { w = 720;  h = 480; } // LiveView buffer
    else if (size == 928*616*2)  { w = 928;  h = 616; } // 500d LV HD buffer dimensions.
    else if (size == 720*424*2)  { w = 720;  h = 424; } // 500d LV buffer in movie mode.
    else if (size == 1576*632*2) { w = 1576; h = 632; } // 500d 720p recording lv buffer dimensions.
    else if (size == 1576*1048*2){ w = 1576; h = 1048;} // 500d HD buffer dimensions in 1080p/720p mode and LV mode.
    else
    {
        bmp_printf(FONT_LARGE, 0, 50, "Cannot preview this picture.");
        bzero32(vram->vram, vram->width * vram->height * 2);
        return;
    }
    
    bmp_printf(FONT_LARGE, 500, 0, " %dx%d ", w, h);
    if (PLAY_MODE) bmp_printf(FONT_LARGE, 0, os.y_max - font_large.height, "Do not press Delete!");

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
