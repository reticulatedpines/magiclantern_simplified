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
#include "math.h"

static void waveform_init();
//~ static void histo_init();
static void do_disp_mode_change();
static void show_overlay();
static void transparent_overlay_from_play();
static void transparent_overlay_offset_clear(void* priv, int delta);
//~ static void draw_histogram_and_waveform();
static void schedule_transparent_overlay();
//~ static void defish_draw();
//~ static void defish_draw_lv_color();
static int zebra_color_word_row(int c, int y);
static void spotmeter_step();


static void cropmark_cache_update_signature();
static int cropmark_cache_is_valid();
static void default_movie_cropmarks();
static void black_bars_16x9();
static void black_bars();
//~ static void defish_draw_play();

extern unsigned int log_length(int x);
extern void zoom_sharpen_step();
extern void bv_auto_update();

void lens_display_set_dirty(){};
void cropmark_clear_cache();
void draw_histogram_and_waveform(int);
void update_disp_mode_bits_from_params();
//~ void uyvy2yrgb(uint32_t , int* , int* , int* , int* );
int toggle_disp_mode();
void toggle_disp_mode_menu(void *priv, int delta);

int get_zoom_overlay_trigger_mode() { return 0; }
void zoom_overlay_set_countdown(){}
int lv_luma_is_accurate() { return 1; }
int should_draw_bottom_graphs() { return 0; }
void bmp_mute_flag_reset(){}
void PauseLiveView(){};
void ResumeLiveView(){};
void play_422(){};
int get_zoom_overlay_trigger_by_focus_ring(){ return 0; }
int get_disp_mode() { return 0; }
int is_focus_peaking_enabled() { return 0; }
void bmp_off(){};
void bmp_on(){};
void clear_zebras_from_mirror(){};
void copy_zebras_from_mirror(){};
void cropmark_clear_cache(){};
void bmp_zoom(){};
void update_disp_mode_bits_from_params(){};
int disp_profiles_0 = 0;

// color info unknown in 5Dc (it's not yuv422)
void yuv2rgb(int Y, int U, int V, int* R, int* G, int* B)
{
    R = G = B = 0;
}

static int bmp_is_on() { return 1; }

#define hist_height         54
#define HIST_WIDTH          128
#define WAVEFORM_WIDTH 180
#define WAVEFORM_HEIGHT 120
#define WAVEFORM_FACTOR (1 << waveform_size) // 1, 2 or 4
#define WAVEFORM_OFFSET (waveform_size <= 1 ? 80 : 0)

#define WAVEFORM_FULLSCREEN (waveform_draw && waveform_size == 2)

#define BVRAM_MIRROR_SIZE (BMPPITCH*540)

//~ static CONFIG_INT( "global.draw",   global_draw, 1 );
#define global_draw 1
static CONFIG_INT( "global.draw.mode",   global_draw_mode, 0 );

PROP_HANDLER(PROP_GUI_STATE)
{
    if (global_draw_mode && buf[0] == GUISTATE_QR)
    {
        fake_simple_button(BGMT_PRESS_DIRECT_PRINT);
    }
}

static CONFIG_INT( "zebra.draw",    zebra_draw, 1 );
static CONFIG_INT( "zebra.thr.hi",    zebra_level_hi, 99 );
static CONFIG_INT( "zebra.thr.lo",    zebra_level_lo, 1 );
       CONFIG_INT( "zebra.rec", zebra_rec,  1 );
static CONFIG_INT( "falsecolor.draw", falsecolor_draw, 0);
static CONFIG_INT( "falsecolor.palette", falsecolor_palette, 0);

int should_draw_zoom_overlay()
{
    return false;
}

int digic_zoom_overlay_enabled()
{
    return 0;
}

int nondigic_zoom_overlay_enabled()
{
    return 0;
}

static CONFIG_INT( "focus.peaking", focus_peaking, 1);
//~ static CONFIG_INT( "focus.peaking.method", focus_peaking_method, 1);
static CONFIG_INT( "focus.peaking.filter.edges", focus_peaking_filter_edges, 1); // prefer texture details rather than strong edges
static CONFIG_INT( "focus.peaking.disp", focus_peaking_disp, 0); // display as dots or blended
static CONFIG_INT( "focus.peaking.thr", focus_peaking_pthr, 5); // 1%
static CONFIG_INT( "focus.peaking.color", focus_peaking_color, 7); // R,G,B,C,M,Y,cc1,cc2
CONFIG_INT( "focus.peaking.grayscale", focus_peaking_grayscale, 0); // R,G,B,C,M,Y,cc1,cc2

int focus_peaking_as_display_filter() { return lv && focus_peaking && focus_peaking_disp; }

//~ static CONFIG_INT( "focus.graph", focus_graph, 0);

//~ static CONFIG_INT( "edge.draw", edge_draw,  0 );
static CONFIG_INT( "hist.draw", hist_draw,  1 );
static CONFIG_INT( "hist.warn", hist_warn,  5 );
static CONFIG_INT( "hist.log",  hist_log,   1 );
//~ static CONFIG_INT( "hist.x",        hist_x,     720 - HIST_WIDTH - 4 );
//~ static CONFIG_INT( "hist.y",        hist_y,     100 );
static CONFIG_INT( "waveform.draw", waveform_draw, 0);
static CONFIG_INT( "waveform.size", waveform_size,  0 );
//~ static CONFIG_INT( "waveform.x",    waveform_x, 720 - WAVEFORM_WIDTH );
//~ static CONFIG_INT( "waveform.y",    waveform_y, 480 - 50 - WAVEFORM_WIDTH );
static CONFIG_INT( "waveform.bg",   waveform_bg,    COLOR_ALMOST_BLACK ); // solid black


       CONFIG_INT( "clear.preview", clearscreen_enabled, 0);
static CONFIG_INT( "clear.preview.delay", clearscreen_delay, 1000); // ms
       CONFIG_INT( "clear.preview.mode", clearscreen_mode, 0); // 2 is always

// keep old program logic
#define clearscreen (clearscreen_enabled ? clearscreen_mode+1 : 0)

static CONFIG_INT( "spotmeter.size",        spotmeter_size, 5 );
static CONFIG_INT( "spotmeter.draw",        spotmeter_draw, 0 );
static CONFIG_INT( "spotmeter.formula",     spotmeter_formula, 0 ); // 0 percent, 1 IRE AJ, 2 IRE Piers
static CONFIG_INT( "spotmeter.position",        spotmeter_position, 1 ); // fixed / attached to AF frame

//~ static CONFIG_INT( "unified.loop", unified_loop, 2); // temporary; on/off/auto
//~ static CONFIG_INT( "zebra.density", zebra_density, 0); 
//~ static CONFIG_INT( "hd.vram", use_hd_vram, 0); 

CONFIG_INT("idle.display.turn_off.after", idle_display_turn_off_after, 0); // this also enables power saving for intervalometer
static CONFIG_INT("idle.display.dim.after", idle_display_dim_after, 0);
static CONFIG_INT("idle.display.gdraw_off.after", idle_display_global_draw_off_after, 0);
static CONFIG_INT("idle.rec", idle_rec, 0);
static CONFIG_INT("idle.shortcut.key", idle_shortcut_key, 0);
CONFIG_INT("idle.blink", idle_blink, 1);

/**
 * Normal BMP VRAM has its origin in 720x480 center crop
 * But on HDMI you are allowed to go back 120x30 pixels (BMP_W_MINUS x BMP_H_MINUS).
 * 
 * For mirror VRAM we'll keep the same addressing mode:
 * allocate full size (960x540) and use the pointer to 720x480 center crop.
 */


static uint8_t* bvram_mirror_start = 0;
static uint8_t* bvram_mirror = 0;
uint8_t* get_bvram_mirror() { return bvram_mirror; }
//~ #define bvram_mirror bmp_vram_idle()


int cropmark_cache_dirty = 1;
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
    if (ml_started) redraw();
}

#if defined(CONFIG_60D) || defined(CONFIG_600D)
volatile int lcd_position = 0;
volatile int display_dont_mirror_dirty;
PROP_HANDLER(PROP_LCD_POSITION)
{
    if (lcd_position != (int)buf[0]) display_dont_mirror_dirty = 1;
    lcd_position = buf[0];
    redraw_after(100);
}
#endif

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
    if (!global_draw) return 0;
    
    if (PLAY_MODE) return 1; // exception, always draw stuff in play mode
    
    return DISPLAY_IS_ON;
}

int get_global_draw_setting() // whatever is set in menu
{
    return global_draw;
}

/** Store the waveform data for each of the WAVEFORM_WIDTH bins with
 * 128 levels
 */
static uint8_t* waveform = 0;
#define WAVEFORM_UNSAFE(x,y) (waveform[(x) + (y) * WAVEFORM_WIDTH])
#define WAVEFORM(x,y) (waveform[COERCE((x), 0, WAVEFORM_WIDTH-1) + COERCE((y), 0, WAVEFORM_HEIGHT-1) * WAVEFORM_WIDTH])

/** Store the histogram data for each of the "HIST_WIDTH" bins */
static uint32_t hist[HIST_WIDTH];

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
    uint32_t* buf = (uint32_t*)lv->vram;

    int x,y;
    
    hist_max = 0;
    hist_total_px = 0;
    for( x=0 ; x<HIST_WIDTH ; x++ )
    {
        hist[x] = 0;
    }

    if (waveform_draw)
    {
        waveform_init();
    }
    
    for( y = os.y0 + os.off_169; y < os.y_max - os.off_169; y += 2 )
    {
        for( x = os.x0 ; x < os.x_max ; x += 2 )
        {
            uint32_t pixel = buf[BM2LV(x,y)/4];
            int Y;
            {
                uint32_t p1 = ((pixel >> 16) & 0xFF00) >> 8;
                uint32_t p2 = ((pixel >>  0) & 0xFF00) >> 8;
                Y = (p1+p2) / 2; 
            }

            hist_total_px++;
            uint32_t hist_level = Y * HIST_WIDTH / 256;

            // Ignore the 0 bin.  It generates too much noise
            unsigned count = ++ (hist[ hist_level & 0x7F]);
            if( hist_level && count > hist_max )
                hist_max = count;

            // Update the waveform plot
            if (waveform_draw) 
            {
                uint8_t* w = &WAVEFORM(((x-os.x0) * WAVEFORM_WIDTH) / os.x_ex, (Y * WAVEFORM_HEIGHT) / 256);
                if ((*w) < 250) (*w)++;
            }

        }
    }
}

int hist_get_percentile_level(int percentile)
{
    int total = 0;
    int i;
    for( i=0 ; i < HIST_WIDTH ; i++ )
        total += hist[i];
    
    int thr = total * percentile / 100;  // 50% => median
    int n = 0;
    for( i=0 ; i < HIST_WIDTH ; i++ )
    {
        n += hist[i];
        if (n >= thr)
            return i * 255 / HIST_WIDTH;
    }
    return -1; // invalid argument?
}

#define UYVY_GET_AVG_Y(uyvy) (((((uyvy) >> 24) & 0xFF) + (((uyvy) >> 8) & 0xFF)) >> 1)

int get_under_and_over_exposure(int thr_lo, int thr_hi, int* under, int* over)
{
    *under = -1;
    *over = -1;
    struct vram_info * lv = get_yuv422_vram();
    if (!lv) return -1;

    *under = 0;
    *over = 0;
    int total = 0;
    void* vram = lv->vram;
    int x,y;
    for( y = os.y0 ; y < os.y_max; y ++ )
    {
        uint32_t * const v_row = (uint32_t*)( vram + BM2LV_R(y) );
        for( x = os.x0 ; x < os.x_max ; x += 2 )
        {
            uint32_t pixel = v_row[x/2];
            
            int Y = UYVY_GET_AVG_Y(pixel);
            if (pixel && Y < thr_lo) (*under)++; // try to ignore black bars
            if (Y > thr_hi) (*over)++;
            total++;
        }
    }
    return total;
}

#define ZEBRA_COLOR_WORD_SOLID(x) ( (x) | (x)<<8 | (x)<<16 | (x)<<24 )

static void hist_dot(int x, int y, int fg_color, int bg_color, int radius, int label)
{
    for (int r = 0; r < radius; r++)
    {
        draw_circle(x, y, r, fg_color);
        draw_circle(x + 1, y, r, fg_color);
    }
    draw_circle(x, y, radius, bg_color);
    
    if (label)
    {
        char msg[5];
        snprintf(msg, sizeof(msg), "%d", label);
        bmp_printf(
            SHADOW_FONT(FONT(FONT_MED, COLOR_BLACK, COLOR_WHITE)), 
            x - font_med.width * strlen(msg) / 2 + 1 + 30,
            y - font_med.height/2,
            msg);
    }
}

static int hist_dot_radius(int over, int hist_total_px)
{
    if (hist_warn < 4) return 7; // fixed radius for these modes
    
    // overexposures stronger than 1% are displayed at max radius (10)
    int p = 100 * over / hist_total_px;
    if (p > 1) return 10;
    
    // for smaller overexposure percentages, use dot radius to suggest the amount
    unsigned p1000 = 100 * 1000 * over / hist_total_px;
    int plog = p1000 ? (int)log2f(p1000) : 0;
    return MIN(plog, 10);
}

static int hist_dot_label(int over, int hist_total_px)
{
    int p = 100 * over / hist_total_px;
    return hist_warn < 4 ? 0 : p;
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
    if (!PLAY_OR_QR_MODE)
    {
        if (!lv_luma_is_accurate()) return;
    }
    uint8_t * const bvram = bmp_vram(); // okay
    if (!bvram) return;

    // Align the x origin, just in case
    x_origin &= ~3;

    if( hist_max == 0 )
        hist_max = 1;

    unsigned i, y;
    
    if (highlight_level >= 0) 
        highlight_level = highlight_level * HIST_WIDTH / 256;

    int log_max = log_length(hist_max);
    
    for( i=0 ; i < HIST_WIDTH ; i++ )
    {
        // Scale by the maximum bin value
        const uint32_t size  = hist_log ? log_length(hist[i])   * hist_height / log_max : (hist[i]   * hist_height) / hist_max;

        uint8_t color;
        uint8_t * col = &color;
        // vertical line up to the hist size
        for( y=hist_height ; y>0 ; y-- )
        {
            if (highlight_level >= 0)
            {
                int hilight = ABS(i-highlight_level) <= 1;
                *col = y > size + hilight ? COLOR_BG : (hilight ? COLOR_RED : COLOR_WHITE);
            }
            else
                *col = y > size ? COLOR_BG : (falsecolor_draw ? false_colour[falsecolor_palette][(i * 256 / HIST_WIDTH) & 0xFF]: COLOR_WHITE);
            
            bmp_putpixel_fast(bvram, x_origin + i, y_origin + hist_height - y, color);
        }
        
        if (hist_warn && i == HIST_WIDTH - 1
            && !nondigic_zoom_overlay_enabled()) // magic zoom borders will be "overexposed" => will cause warning
        {
            unsigned int thr = hist_total_px / (
                hist_warn == 1 ? 100000 : // 0.001%
                hist_warn == 2 ? 10000  : // 0.01%
                hist_warn == 3 ? 1000   : // 0.1%
                hist_warn == 4 ? 100    : // 1%
                                 100000); // start at 0.0001 with a tiny dot
            thr = MAX(thr, 1);
            int yw = y_origin + 12 + (hist_log ? hist_height - 24 : 0);
            int bg = (hist_log ? COLOR_WHITE : COLOR_BLACK);
            {
                unsigned int over = hist[i] + hist[i-1] + hist[i-2];
                if (over > thr) hist_dot(x_origin + HIST_WIDTH/2, yw, COLOR_RED, bg, hist_dot_radius(over, hist_total_px), hist_dot_label(over, hist_total_px));
            }
        }
    }
    bmp_draw_rect(60, x_origin-1, y_origin-1, HIST_WIDTH+1, hist_height+1);
}

void hist_highlight(int level)
{
    get_yuv422_vram();
    hist_draw_image( os.x_max - HIST_WIDTH, os.y0 + 100, level );
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
    unsigned        y_origin,
    unsigned        height
)
{
    if (!PLAY_OR_QR_MODE)
    {
        if (!lv_luma_is_accurate()) return;
    }

    // Ensure that x_origin is quad-word aligned
    x_origin &= ~3;

    uint8_t * const bvram = bmp_vram(); // okay
    if (!bvram) return;
    unsigned pitch = BMPPITCH;
    if( hist_max == 0 )
        hist_max = 1;

    int i, y;

    // vertical line up to the hist size
    for (int k = 0; k < WAVEFORM_FACTOR; k++)
    {
        for( y=WAVEFORM_HEIGHT-1 ; y>=0 ; y-- )
        {
            int ys = y * height / WAVEFORM_HEIGHT + k;
            //int y_next = (y-1) * height / WAVEFORM_HEIGHT;
            uint32_t pixel = 0;
            int w = WAVEFORM_WIDTH*WAVEFORM_FACTOR;
            for( i=0 ; i<w; i++ )
            {
                uint32_t count = WAVEFORM_UNSAFE( i / WAVEFORM_FACTOR, WAVEFORM_HEIGHT - y - 1);
                if (height < WAVEFORM_HEIGHT)
                { // smooth it a bit to reduce aliasing; not perfect, but works.. sort of
                    count += WAVEFORM_UNSAFE( i / WAVEFORM_FACTOR, WAVEFORM_HEIGHT - y - 1);
                    //~ count /= 2;
                }
                // Scale to a grayscale
                count = (count * 42) / 128;
                if( count > 42 - 5 )
                    count = COLOR_RED;
                else
                if( count >  0 )
                    count += 38 + 5;
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
                    count = COLOR_BLACK; // transparent

                pixel |= (count << ((i & 3)<<3));

                if( (i & 3) != 3 )
                    continue;

                bmp_putpixel_fast(bvram, x_origin + i, y_origin + ys, pixel);
                bmp_putpixel_fast(bvram, x_origin + i + 1, y_origin + ys, pixel>>8);
                bmp_putpixel_fast(bvram, x_origin + i + 2, y_origin + ys, pixel>>16);
                bmp_putpixel_fast(bvram, x_origin + i + 3, y_origin + ys, pixel>>24);

                pixel = 0;
            }
        }
        bmp_draw_rect(60, x_origin-1, y_origin-1, WAVEFORM_WIDTH*WAVEFORM_FACTOR+1, height+1);
    }
}


static FILE * g_aj_logfile = INVALID_PTR;
unsigned int aj_create_log_file( char * name)
{
   g_aj_logfile = FIO_CreateFileEx( name );
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

/*static void dump_vram()
{
    dump_big_seg(4, CARD_DRIVE "ML/LOGS/4.bin");
    dump_big_seg(4, CARD_DRIVE "ML/LOGS/4-1.bin");
    //dump_seg(0x1000, 0x100000, CARD_DRIVE "ML/LOGS/ram.bin");
    //~ dump_seg(YUV422_IMAGE_BUFFER, 1920*1080*2, CARD_DRIVE "ML/LOGS/VRAM.BIN");
}*/

int fps_ticks = 0;

static void waveform_init()
{
    if (!waveform)
        waveform = AllocateMemory(WAVEFORM_WIDTH * WAVEFORM_HEIGHT);
    bzero32(waveform, WAVEFORM_WIDTH * WAVEFORM_HEIGHT);
}

void bvram_mirror_clear()
{
    ASSERT(bvram_mirror_start);
    BMP_LOCK( bzero32(bvram_mirror_start, BMP_VRAM_SIZE); )
    cropmark_cache_dirty = 1;
}
void bvram_mirror_init()
{
    if (!bvram_mirror_start)
    {
        // shoot_malloc is not that stable
        //~ #if defined(CONFIG_600D) || defined(CONFIG_1100D)
        //~ bvram_mirror_start = (void*)shoot_malloc(BMP_VRAM_SIZE); // there's little memory available in system pool
        //~ #else
        bvram_mirror_start = (void*)UNCACHEABLE(AllocateMemory(BMP_VRAM_SIZE));
        //~ #endif
        if (!bvram_mirror_start) 
        {   
            while(1)
            {
                bmp_printf(FONT_MED, 30, 30, "Failed to allocate BVRAM mirror");
                msleep(100);
            }
        }
        // to keep the same addressing mode as with normal BMP VRAM - origin in 720x480 center crop
        bvram_mirror = bvram_mirror_start + BMP_HDMI_OFFSET;
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

//~ static unsigned int* bm_hd_r_cache = 0;
static unsigned int bm_hd_x_cache[BMP_W_PLUS - BMP_W_MINUS];
static int bm_hd_bm2lv_sx = 0;
static int bm_hd_lv2hd_sx = 0;

void zebra_update_lut()
{
    int rebuild = 0;
        
    if(unlikely(bm_hd_bm2lv_sx != bm2lv.sx))
    {
        bm_hd_bm2lv_sx = bm2lv.sx;
        rebuild = 1;
    }
    if(unlikely(bm_hd_lv2hd_sx != lv2hd.sx))
    {
        bm_hd_lv2hd_sx = lv2hd.sx;
        rebuild = 1;
    }
    
    if(unlikely(rebuild))
    {
        int xStart = os.x0 + 8;
        int xEnd = os.x_max - 8;

        for (int x = xStart; x < xEnd; x += 1)
        {
            bm_hd_x_cache[x - BMP_W_MINUS] = (BM2HD_X(x) * 2) + 1;
        }        
    }
}

/*static int zebra_color_word_row_thick(int c, int y)
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
}*/

static int zebra_digic_dirty = 0;

void draw_zebras( int Z )
{
    uint8_t * const bvram = bmp_vram_real();
    int zd = Z && zebra_draw && (lv_luma_is_accurate() || PLAY_OR_QR_MODE) && (zebra_rec || !recording); // when to draw zebras
    if (zd)
    {
        int zlh = zebra_level_hi * 255 / 100 - 1;
        int zll = zebra_level_lo * 255 / 100;
        
        uint8_t * lvram = get_yuv422_vram()->vram;

        // draw zebra in 16:9 frame
        // y is in BM coords
        for(int y = os.y0 + os.off_169; y < os.y_max - os.off_169; y ++ )
        {
            #define color_over           ZEBRA_COLOR_WORD_SOLID(COLOR_RED)
            #define color_under          ZEBRA_COLOR_WORD_SOLID(COLOR_BLUE)
            
            uint32_t * const v_row = (uint32_t*)( lvram        + BM2LV_R(y)    );  // 2 pixels
            
            uint32_t* lvp; // that's a moving pointer through lv vram
            
            for (int x = os.x0; x < os.x_max; x ++)
            {
                lvp = v_row + BM2LV_X(x)/2;
                int bp = 0;
                #define BP bp
                
                {
                    int p0 = (*lvp) >> 8 & 0xFF;
                    if (unlikely(p0 > zlh))
                    {
                        BP = color_over;
                    }
                    else if (unlikely(p0 < zll))
                    {
                        BP = color_under;
                    }
                    else
                        BP = 0;
                }
                    
                bmp_putpixel_fast(bvram, x, y, BP);
                bmp_putpixel_fast(bvram, x+1, y, BP>>8);

                #undef MP
                #undef BP
                #undef BN
                #undef MN
            }
        }
    }
}

static int peak_scaling[256];

/*
static inline int peak_d1xy(uint8_t* p8)
{
    int p_cc = (int)(*p8);
    int p_rc = (int)(*(p8 + 2));
    int p_cd = (int)(*(p8 + vram_lv.pitch));
    
    int e_dx = ABS(p_rc - p_cc);
    int e_dy = ABS(p_cd - p_cc);
    
    int e = MAX(e_dx, e_dy);
    return peak_scaling[MIN(e, 255)];
}*/

static inline int peak_d2xy(uint8_t* p8)
{
    // approximate second derivative with a Laplacian kernel:
    //     -1
    //  -1  4 -1
    //     -1
    int result = ((int)(*p8) * 4) - (int)(*(p8 + 2));
    result -= (int)(*(p8 - 2));
    result -= (int)(*(p8 + vram_lv.pitch));
    result -= (int)(*(p8 - vram_lv.pitch));

    int e = ABS(result);
    
    if (focus_peaking_filter_edges)
    {
        // filter out strong edges where first derivative is strong
        // as these are usually false positives
        int d1x = ABS((int)(*(p8 + 2)) - (int)(*(p8 - 2)));
        int d1y = ABS((int)(*(p8 + vram_lv.pitch)) - (int)(*(p8 - vram_lv.pitch)));
        int d1 = MAX(d1x, d1y);
        e = MAX(e - ((d1 << focus_peaking_filter_edges) >> 2), 0) * 2;
    }
    return e;
}

static inline int peak_d2xy_hd(const uint8_t* p8)
{
    // approximate second derivative with a Laplacian kernel:
    //     -1
    //  -1  4 -1
    //     -1
    int result = ((int)(*p8) * 4) - (int)(*(p8 + 2));
    result -= (int)(*(p8 - 2));
    result -= (int)(*(p8 + vram_hd.pitch));
    result -= (int)(*(p8 - vram_hd.pitch));

    int e = ABS(result);
    
    if (focus_peaking_filter_edges)
    {
        // filter out strong edges where first derivative is strong
        // as these are usually false positives
        int d1x = ABS((int)(*(p8 + 2)) - (int)(*(p8 - 2)));
        int d1y = ABS((int)(*(p8 + vram_hd.pitch)) - (int)(*(p8 - vram_hd.pitch)));
        int d1 = MAX(d1x, d1y);
        e = MAX(e - ((d1 << focus_peaking_filter_edges) >> 2), 0) * 2;
    }
    return e;
}

// should be more accurate for 5Dc - there is a hi-res buffer in playback mode
static int peak_d2xy_hd_avg2x2(const uint8_t* p8)
{
    int p00 = peak_d2xy(p8);
    int p01 = peak_d2xy(p8 + 4);
    int p10 = peak_d2xy(p8 + vram_hd.pitch);
    int p11 = peak_d2xy(p8 + vram_hd.pitch + 4);
    return (p00 + p01 + p10 + p11) / 4;
}

//~ static inline int peak_blend_solid(uint32_t* s, int e, int thr) { return 0x4C7F4CD5; }
//~ static inline int peak_blend_raw(uint32_t* s, int e) { return (e << 8) | (e << 24); }
static inline int peak_blend_alpha(uint32_t* s, int e)
{
}

void peak_disp_filter()
{
    if (!lv) return;

    uint32_t* src_buf;
    uint32_t* dst_buf;
    display_filter_get_buffers(&src_buf, &dst_buf);

    static int thr = 50;
    static int thr_increment = 1;
    static int thr_delta = 0;
    
    #define FOCUSED_THR 64
    // the percentage selected in menu represents how many pixels are considered in focus
    // let's say above some FOCUSED_THR
    // so, let's scale edge value so that e=thr maps to e=FOCUSED_THR
    for (int i = 0; i < 255; i++)
        peak_scaling[i] = MIN(i * FOCUSED_THR / thr, 255);
    
    int n_over = 0;
    int n_total = 720 * (os.y_max - os.y0) / 2;

    #define PEAK_LOOP for (int i = 720 * (os.y0/2); i < 720 * (os.y_max/2); i++)
    // generic loop:
    //~ for (int i = 720 * (os.y0/2); i < 720 * (os.y_max/2); i++)
    //~ {
        //~ int e = peak_compute((uint8_t*)&src_buf[i] + 1);
        //~ dst_buf[i] = peak_blend(&src_buf[i], e, blend_thr);
        //~ if (unlikely(e > FOCUSED_THR)) n_over++;
    //~ }

    if (focus_peaking_disp == 3) // raw
    {
        PEAK_LOOP
        {
            int e = peak_d2xy((uint8_t*)&src_buf[i] + 1);
            e = MIN(e * 4, 255);
            dst_buf[i] = (e << 8) | (e << 24);
        }
    }
    
    else if (focus_peaking_grayscale)
    {
        if (focus_peaking_disp == 1) 
        {
            PEAK_LOOP
            {
                int e = peak_d2xy((uint8_t*)&src_buf[i] + 1);
                e = peak_scaling[MIN(e, 255)];
                if (likely(e < FOCUSED_THR)) dst_buf[i] = src_buf[i] & 0xFF00FF00;
                else 
                { 
                    dst_buf[i] = 0x4C7F4CD5; // red
                    n_over++;
                }
            }
        }
        else if (focus_peaking_disp == 2) // alpha
        {
            PEAK_LOOP
            {
                int e = peak_d2xy((uint8_t*)&src_buf[i] + 1);
                e = peak_scaling[MIN(e, 255)];
                if (likely(e < 20)) dst_buf[i] = src_buf[i] & 0xFF00FF00;
                else dst_buf[i] = peak_blend_alpha(&src_buf[i], e);
                if (unlikely(e > FOCUSED_THR)) n_over++;
            }
        }
    }
    else // color
    {
        if (focus_peaking_disp == 1) 
        {
            PEAK_LOOP
            {
                int e = peak_d2xy((uint8_t*)&src_buf[i] + 1);
                e = peak_scaling[MIN(e, 255)];
                if (likely(e < FOCUSED_THR)) dst_buf[i] = src_buf[i];
                else 
                { 
                    dst_buf[i] = 0x4C7F4CD5; // red
                    n_over++;
                }
            }
        }
        else if (focus_peaking_disp == 2) // alpha
        {
            PEAK_LOOP
            {
                int e = peak_d2xy((uint8_t*)&src_buf[i] + 1);
                e = peak_scaling[MIN(e, 255)];
                if (likely(e < 20)) dst_buf[i] = src_buf[i];
                else dst_buf[i] = peak_blend_alpha(&src_buf[i], e);
                if (unlikely(e > FOCUSED_THR)) n_over++;
            }
        }
    }

    // update threshold for next iteration
    if (1000 * n_over / n_total > (int)focus_peaking_pthr)
    {
        if (thr_delta > 0) thr_increment++; else thr_increment = 1;
        thr += thr_increment;
    }
    else
    {
        if (thr_delta < 0) thr_increment++; else thr_increment = 1;
        thr -= thr_increment;
    }

    thr_increment = COERCE(thr_increment, -10, 10);
    thr = COERCE(thr, 10, 255);
    
    if (focus_peaking_disp == 3) thr = 64;
}

static void focus_found_pixel(int x, int y, int e, int thr, uint8_t * const bvram)
{    
    int color = get_focus_color(thr, e);
    bmp_putpixel_fast(bvram, x, y, color);
    bmp_putpixel_fast(bvram, x+1, y, color);
    bmp_putpixel_fast(bvram, x, y+1, color);
    bmp_putpixel_fast(bvram, x+1, y+1, color);
}

// returns how the focus peaking threshold changed
static int
draw_zebra_and_focus( int Z, int F)
{
    if (unlikely(!get_global_draw())) return 0;

    uint8_t * const bvram = bmp_vram_real();
    if (unlikely(!bvram)) return 0;
    if (unlikely(!bvram_mirror)) return 0;
    
    draw_zebras(Z);

    if (focus_peaking_as_display_filter()) return 0; // it's drawn from display filters routine

    static int thr = 50;
    static int thr_increment = 1;
    static int prev_thr = 50;
    static int thr_delta = 0;

    if (F && focus_peaking)
    {
        struct vram_info *hd_vram = get_yuv422_hd_vram();
        uint32_t hdvram = (uint32_t)hd_vram->vram;
        
        int yStart = os.y0 + os.off_169 + 8;
        int yEnd = os.y_max - os.off_169 - 8;
        int xStart = os.x0 + 8;
        int xEnd = os.x_max - 8;
        int n_over = 0;
        int n_total = ((yEnd - yStart) * (xEnd - xStart)) / 4;
        
        const uint8_t* p8; // that's a moving pointer
        
        zebra_update_lut();

        {
            for(int y = yStart; y < yEnd; y += 2)
            {
                uint32_t hd_row = hdvram + BM2HD_R(y);
                
                for (int x = xStart; x < xEnd; x += 2)
                {
                    p8 = (uint8_t *)(hd_row + bm_hd_x_cache[x - BMP_W_MINUS]);
                    
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
                     
                    int e = peak_d2xy_hd(p8);
                    
                    /* executed for 1% of pixels */
                    if (unlikely(e >= thr))
                    {
                        n_over++;

                        if (F == 1) focus_found_pixel(x, y, e, thr, bvram);
                    }
                }
            }
        }

        //~ bmp_printf(FONT_LARGE, 10, 50, "%d ", thr);
        
        if (1000 * n_over / n_total > (int)focus_peaking_pthr)
        {
            if (thr_delta > 0) thr_increment++; else thr_increment = 1;
            thr += thr_increment;
        }
        else
        {
            if (thr_delta < 0) thr_increment++; else thr_increment = 1;
            thr -= thr_increment;
        }

        thr_increment = COERCE(thr_increment, -5, 5);
        int thr_min = (lens_info.iso > 1600 ? 15 : 10);
        thr = COERCE(thr, thr_min, 255);


        thr_delta = thr - prev_thr;
        prev_thr = thr;

    }

    return thr_delta;
}

void guess_focus_peaking_threshold()
{
    if (!focus_peaking) return;
    int prev_thr_delta = 1234;
    for (int i = 0; i < 50; i++)
    {
        int thr_delta = draw_zebra_and_focus(0,2); // dummy focus peaking without drawing
        //~ bmp_printf(FONT_LARGE, 0, 0, "%x ", thr_delta); msleep(1000);
        if (!thr_delta) break;
        if (prev_thr_delta != 1234 && SGN(thr_delta) != SGN(prev_thr_delta)) break;
        prev_thr_delta = thr_delta;
    }
}

static void
draw_false_downsampled( void )
{
    return;
    //~ if (vram_width > 720) return;
    //~ if (!PLAY_MODE)
    //~ {
        //~ if (!expsim) return;
    //~ }
    
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

static void
zebra_draw_display( void * priv, int x, int y, int selected )
{
    unsigned z = *(unsigned*) priv;
    
    int over_disabled = (zebra_level_hi > 100);
    int under_disabled = (zebra_level_lo == 0);
    
    char msg[50];
    snprintf(msg, sizeof(msg), "Zebras      : ");
    
    if (!z)
    {
        STR_APPEND(msg, "OFF");
    }
    else
    {
        STR_APPEND(msg,
            "Luma, "
        );
    
        if (over_disabled)
        {
            STR_APPEND(msg, 
                "under %d%%",
                zebra_level_lo
            );
        }
        else if (under_disabled)
        {
            STR_APPEND(msg, 
                "over %d%%",
                zebra_level_hi
            );
        }
        else
        {
            STR_APPEND(msg, 
                "%d..%d%%",
                zebra_level_lo, zebra_level_hi
            );
        }
    }
    bmp_printf(
        MENU_FONT,
        x, y,
        "%s", 
        msg
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(z));
}

static void
zebra_level_display( void * priv, int x, int y, int selected )
{
    unsigned level = *(unsigned*) priv;
    if (level == 0 || level > 100)
    {
            bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "%s : Disabled",
            priv == &zebra_level_lo ? "Underexposure" : 
                                      "Overexposure "
        );
        menu_draw_icon(x, y, MNI_DISABLE, 0);
    }
    else
    {
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
}
/*static void
zebra_toggle( void* priv, int sign )
{
    menu_ternary_toggle(priv, -sign);
}*/

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
        draw_line(x + 419 + i, y, x + 419 + i, y + font_large.height - 2, false_colour[falsecolor_palette][i]);
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
            "Focus Peak  : ON,%d.%d,%s%s",
            focus_peaking_pthr / 10, focus_peaking_pthr % 10, 
            focus_peaking_color == 0 ? "R" :
            focus_peaking_color == 1 ? "G" :
            focus_peaking_color == 2 ? "B" :
            focus_peaking_color == 3 ? "C" :
            focus_peaking_color == 4 ? "M" :
            focus_peaking_color == 5 ? "Y" :
            focus_peaking_color == 6 ? "global" :
            focus_peaking_color == 7 ? "local" : "err",
            focus_peaking_grayscale ? ",Gray" : ""
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
        hist_draw == 0 ? "OFF" : "Luma",
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
        hist_warn == 3 ? "0.1% px" : 
        hist_warn == 4 ? "1% px" :
                         "Gradual"
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
        "Show Overlay: %s",
        global_draw_mode == 0 ? "DirecPrint btn only" : "After taking a pic"
    );
}

/*static void
waveform_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Waveform    : %s",
        *(unsigned*) priv ? "ON " : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(*(unsigned*) priv));
}*/

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
        "Spotmeter   : %s",
        spotmeter_draw == 0    ? "OFF" : 
        spotmeter_formula == 0 ? "Percent" :
        spotmeter_formula == 1 ? "0..255" :
        spotmeter_formula == 2 ? "IRE -1..101" :
        spotmeter_formula == 3 ? "IRE 0..108" :
        spotmeter_formula == 4 ? "RGB" :
        spotmeter_formula == 5 ? "HSL" :
        /*spotmeter_formula == 6*/"HSV"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(spotmeter_draw));
}

// for surface cleaning
int spy_pre_xcb = -1;
int spy_pre_ycb = -1;

void get_spot_yuv_ex(int size_dxb, int dx, int dy, int* Y, int* U, int* V)
{
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return;
    const uint16_t*     vr = (void*) vram->vram;
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    int                 x, y;

    int xcb = os.x0 + os.x_ex/2 + dx;
    int ycb = os.y0 + os.y_ex/2 + dy;
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_Y(ycb);
    int dxl = BM2LV_DX(size_dxb);

	// surface cleaning
	if ( spy_pre_xcb != -1 && spy_pre_ycb != -1  && (spy_pre_xcb != xcb || spy_pre_ycb != ycb) ) {
		bmp_draw_rect(0, spy_pre_xcb - size_dxb, spy_pre_ycb - size_dxb, 2*size_dxb, 2*size_dxb);
	}

    bmp_draw_rect(COLOR_WHITE, xcb - size_dxb, ycb - size_dxb, 2*size_dxb, 2*size_dxb);
    
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

	spy_pre_xcb = xcb;
	spy_pre_ycb = ycb;
}

void get_spot_yuv(int dxb, int* Y, int* U, int* V)
{
    get_spot_yuv_ex(dxb, 0, 0, Y, U, V);
}

int get_spot_motion(int dxb, int xcb, int ycb, int draw)
{
    return 0;
}

static void spotmeter_step()
{
    if (gui_menu_shown()) return;
    if (!get_global_draw()) return;
    if (digic_zoom_overlay_enabled()) return; // incorrect readings
    //~ if (!lv) return;
    if (!PLAY_OR_QR_MODE)
    {
        if (!lv_luma_is_accurate()) return;
    }
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return;
    
    const uint16_t*     vr = (uint16_t*) vram->vram;
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    const unsigned      dxb = spotmeter_size;
    //unsigned        sum = 0;
    int                 x, y;

    int xcb = os.x0 + os.x_ex/2;
    int ycb = os.y0 + os.y_ex/2;
    
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_Y(ycb);
    int dxl = BM2LV_DX(dxb);
    
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

    // Scale to 100%
    const unsigned      scaled = (101 * sy) / 256;
    
    // spotmeter color: 
    // black on transparent, if brightness > 60%
    // white on transparent, if brightness < 50%
    // previous value otherwise
    
    // if false color is active, draw white on semi-transparent gray
    
    static int fg = 0;
    if (scaled > 60) fg = COLOR_BLACK;
    if (scaled < 50 || falsecolor_draw) fg = COLOR_WHITE;
    int bg = fg == COLOR_BLACK ? COLOR_WHITE : COLOR_BLACK;
    int fnt = FONT(SHADOW_FONT(FONT_LARGE), fg, bg);
    int fnts = FONT(SHADOW_FONT(FONT_MED), fg, bg);

    if (!arrow_keys_shortcuts_active())
    {
        bmp_draw_rect(COLOR_WHITE, xcb - dxb, ycb - dxb, 2*dxb+1, 2*dxb+1);
        bmp_draw_rect(COLOR_BLACK, xcb - dxb + 1, ycb - dxb + 1, 2*dxb+1-2, 2*dxb+1-2);
    }
    ycb += dxb + 20;
    ycb -= font_large.height/2;
    xcb -= 2 * font_large.width;

    if (spotmeter_formula <= 1)
    {
        bmp_printf(
            fnt,
            xcb, ycb, 
            "%3d%s",
            spotmeter_formula == 0 ? scaled : sy,
            spotmeter_formula == 0 ? "%" : ""
        );
    }
    else if (spotmeter_formula <= 3)
    {
        int ire_aj = (((int)sy) - 2) * 102 / 253 - 1; // formula from AJ: (2...255) -> (-1...101)
        int ire_piers = ((int)sy) * 108/255;           // formula from Piers: (0...255) -> (0...108)
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
            xcb + font_large.width*4, ycb,
            "IRE\n%s",
            spotmeter_formula == 2 ? "-1..101" : "0..108"
        );
    }
}


int handle_transparent_overlay(struct event * event)
{
    return 1;
}

struct menu_entry zebra_menus[] = {
    {
        .name = "Show Overlay",
        .priv       = &global_draw_mode,
        .max = 1,
        //~ .select     = menu_binary_toggle,
        .display    = global_draw_display,
        .icon_type = IT_DICE,
        .help = "When to display ML overlay graphics (zebra, histogram...)",
        //.essential = FOR_LIVEVIEW,
    },
    {
        .name = "Zebras",
        .priv       = &zebra_draw,
        .select     = menu_binary_toggle,
        .display    = zebra_draw_display,
        .help = "Zebra stripes: show overexposed or underexposed areas.",
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
            {
                .name = "Underexposure",
                .priv = &zebra_level_lo, 
                .min = 0,
                .max = 20,
                .display = zebra_level_display,
                .help = "Underexposure threshold.",
            },
            {
                .name = "Overexposure", 
                .priv = &zebra_level_hi,
                .min = 70,
                .max = 101,
                .display = zebra_level_display,
                .help = "Overexposure threshold.",
            },
            MENU_EOL
        },
    },
    {
        .name = "Focus Peak",
        .priv           = &focus_peaking,
        .display        = focus_peaking_display,
        .select         = menu_binary_toggle,
        .help = "Show which parts of the image are in focus.",
        .submenu_width = 650,
        //.essential = FOR_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Filter bias", 
                .priv = &focus_peaking_filter_edges,
                .max = 2,
                .choices = (const char *[]) {"Strong edges", "Balanced", "Fine details"},
                .help = "Balance fine texture details vs strong high-contrast edges.",
                .icon_type = IT_DICE
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
            /*{
                .priv = &focus_peaking_debug,
                .max = 1,
                .name = "Debug mode",
                .help = "Displays raw contrast image (grayscale).",
            },*/
            MENU_EOL
        },
    },
    {
        .name = "Spotmeter",
        .priv           = &spotmeter_draw,
        .select         = menu_binary_toggle,
        .display        = spotmeter_menu_display,
        .help = "Exposure aid: display brightness from a small spot.",
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
            {
                .name = "Unit",
                .priv = &spotmeter_formula, 
                .max = 1,
                .choices = (const char *[]) {"Percent", "0..255", "IRE -1..101", "IRE 0..108", "RGB (HTML)"},
                .icon_type = IT_DICE,
                .help = "Measurement unit for brightness level(s).",
            },
            MENU_EOL
        }
    },
    #if 0
    {
        .name = "False color",
        .priv       = &falsecolor_draw,
        .display    = falsecolor_display,
        .select     = menu_binary_toggle,
        .submenu_height = 160,
        .help = "Exposure aid: each brightness level is color-coded.",
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
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
    #endif
/*  {
        .name = "Histo/Wavefm",
        .priv       = &hist_draw,
        .select     = zebra_toggle,
        .select_auto = waveform_toggle,
        .display    = hist_display,
        .help = "Histogram [SET] and Waveform [Q] for evaluating exposure.",
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
    },
    */
    {
        .name = "Histogram",
        .priv       = &hist_draw,
        .max = 1,
        .display = hist_print,
        .help = "Exposure aid: shows the distribution of brightness levels.",
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
        .children =  (struct menu_entry[]) {
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
                .max = 5,
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
                .max = 1,
                .choices = (const char *[]) {"Small", "Large"},
                .icon_type = IT_SIZE,
                .help = "Waveform size: Small / Large / FullScreen.",
            },
            MENU_EOL
        },
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
    },
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

int handle_zoom_overlay(struct event * event)
{
    return 1;
}

int liveview_display_idle()
{
    return 0;
}


// when it's safe to draw zebras and other on-screen stuff
int zebra_should_run()
{
    return 0;
}

int livev_for_playback_running = 0;
void draw_livev_for_playback()
{
    if (!PLAY_MODE && !QR_MODE)
    {
        livev_for_playback_running = 0;
        return;
    }

    while (!DISPLAY_IS_ON) msleep(100);

    livev_for_playback_running = 1;
    info_led_on();
    get_yuv422_vram(); // just to refresh VRAM params
    
    extern int defish_preview;
    
//~ BMP_LOCK(
    set_ml_palette();

    bvram_mirror_clear(); // may be filled with liveview cropmark / masking info, not needed in play mode
    clrscr();

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
        guess_focus_peaking_threshold();
        draw_zebra_and_focus(1,1);
    }

    if (spotmeter_draw)
        spotmeter_step();
    
    draw_histogram_and_waveform(1);

    bvram_mirror_clear(); // may remain filled with playback zebras 
//~ )
    livev_for_playback_running = 0;
    info_led_off();
}

void draw_histogram_and_waveform(int allow_play)
{
    if (!get_global_draw()) return;
    
    get_yuv422_vram();

    if (hist_draw || waveform_draw)
    {
        hist_build();
    }

    if( hist_draw )
    {
        BMP_LOCK( hist_draw_image( os.x_max - HIST_WIDTH - 5, os.y0 + 100, -1); )
    }

    if( waveform_draw)
    {
        BMP_LOCK( waveform_draw_image( os.x_max - WAVEFORM_WIDTH*WAVEFORM_FACTOR - (WAVEFORM_FULLSCREEN ? 0 : 4), os.y_max - WAVEFORM_HEIGHT*WAVEFORM_FACTOR - WAVEFORM_OFFSET, WAVEFORM_HEIGHT*WAVEFORM_FACTOR ); )
    }
}

int idle_is_powersave_enabled()
{
    return 0;
}

int idle_is_powersave_active()
{
    return 0;
}

void idle_force_powersave_in_1s()
{
}

void idle_force_powersave_now()
{
}

int handle_powersave_key(struct event * event)
{
    return 1;
}

void idle_wakeup_reset_counters(int reason) // called from handle_buttons
{
}

void redraw_do()
{
    clrscr();
}

void redraw()
{
    clrscr();
}

void draw_cropmark_area()
{
    get_yuv422_vram();
    bmp_draw_rect(COLOR_BLUE, os.x0, os.y0, os.x_ex, os.y_ex);
    draw_line(os.x0, os.y0, os.x_max, os.y_max, COLOR_BLUE);
    draw_line(os.x0, os.y_max, os.x_max, os.y0, COLOR_BLUE);
    
    bmp_draw_rect(COLOR_RED, HD2BM_X(0), HD2BM_Y(0), HD2BM_DX(vram_hd.width), HD2BM_DY(vram_hd.height));
    draw_line(HD2BM_X(0), HD2BM_Y(0), HD2BM_X(vram_hd.width), HD2BM_Y(vram_hd.height), COLOR_RED);
    draw_line(HD2BM_X(0), HD2BM_Y(vram_hd.height), HD2BM_X(vram_hd.width), HD2BM_Y(0), COLOR_RED);
}

int handle_disp_preset_key(struct event * event)
{
    return 1;
}

static int livev_playback = 0;

static void livev_playback_toggle()
{
    if (livev_for_playback_running) return;
    livev_playback = !livev_playback;
    if (livev_playback)
    {
        livev_for_playback_running = 1;
        task_create("lv_playback", 0x1a, 0x4000, draw_livev_for_playback, 0);
    }
    else
    {
        clrscr();
        //~ restore_canon_palette();
    }
}
static void livev_playback_reset()
{
    livev_playback = 0;
}

int handle_livev_playback(struct event * event, int button)
{
    // enable LiveV stuff in Play mode
    if (PLAY_OR_QR_MODE && !gui_menu_shown())
    {
        if (event->param == button)
        {
            livev_playback_toggle();
            return 0;
        }
        else if (event->param != button+1)
        {
            livev_playback_reset();
        }
    }
    return 1;
}


static void zebra_init()
{
    menu_add( "Overlay", zebra_menus, COUNT(zebra_menus) );
}

INIT_FUNC(__FILE__, zebra_init);

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

void peaking_benchmark()
{
    msleep(1000);
    fake_simple_button(BGMT_PLAY);
    msleep(2000);
    int a = get_seconds_clock();
    for (int i = 0; i < 1000; i++)
    {
        draw_zebra_and_focus(0,1);
    }
    int b = get_seconds_clock();
    NotifyBox(10000, "%d seconds => %d fps", b-a, 1000 / (b-a));
    beep();
}
