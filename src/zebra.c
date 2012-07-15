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


#define DIGIC_ZEBRA_REGISTER 0xC0F140cc

//~ #if 1
//~ #define CONFIG_KILL_FLICKER // this will block all Canon drawing routines when the camera is idle 
#if defined(CONFIG_50D)// || defined(CONFIG_60D)
#define CONFIG_KILL_FLICKER // this will block all Canon drawing routines when the camera is idle 

extern int kill_canon_gui_mode;
#endif                      // but it will display ML graphics

static void waveform_init();
//~ static void histo_init();
static void do_disp_mode_change();
static void show_overlay();
static void transparent_overlay_from_play();
static void transparent_overlay_offset_clear(void* priv, int delta);
//~ static void draw_histogram_and_waveform();
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
void cropmark_clear_cache();
void draw_histogram_and_waveform(int);
void update_disp_mode_bits_from_params();
//~ void uyvy2yrgb(uint32_t , int* , int* , int* , int* );
int toggle_disp_mode();
void toggle_disp_mode_menu(void *priv, int delta);



// precompute some parts of YUV to RGB computations
static int yuv2rgb_RV[256];
static int yuv2rgb_GU[256];
static int yuv2rgb_GV[256];
static int yuv2rgb_BU[256];

static void precompute_yuv2rgb()
{
    /*
    *R = *Y + 1437 * V / 1024;
    *G = *Y -  352 * U / 1024 - 731 * V / 1024;
    *B = *Y + 1812 * U / 1024;
    */

    for (int u = 0; u < 256; u++)
    {
        int8_t U = u;
        yuv2rgb_GU[u] = -352 * U / 1024;
        yuv2rgb_BU[u] = 1812 * U / 1024;
    }

    for (int v = 0; v < 256; v++)
    {
        int8_t V = v;
        yuv2rgb_RV[v] = 1437 * V / 1024;
        yuv2rgb_GV[v] = -731 * V / 1024;
    }
}

/*inline void uyvy2yrgb(uint32_t uyvy, int* Y, int* R, int* G, int* B)
{
    uint32_t y1 = (uyvy >> 24) & 0xFF;
    uint32_t y2 = (uyvy >>  8) & 0xFF;
    *Y = (y1+y2) / 2;
    uint8_t u = (uyvy >>  0) & 0xFF;
    uint8_t v = (uyvy >> 16) & 0xFF;
    *R = MIN(*Y + yuv2rgb_RV[v], 255);
    *G = MIN(*Y + yuv2rgb_GU[u] + yuv2rgb_GV[v], 255);
    *B = MIN(*Y + yuv2rgb_BU[u], 255);
} */

#define UYVY_GET_AVG_Y(uyvy) (((((uyvy) >> 24) & 0xFF) + (((uyvy) >> 8) & 0xFF)) >> 1)
#define UYVY_GET_U(uyvy) (((uyvy)       ) & 0xFF)
#define UYVY_GET_V(uyvy) (((uyvy) >>  16) & 0xFF)
#define COMPUTE_UYVY2YRGB(uyvy, Y, R, G, B) \
{ \
    Y = UYVY_GET_AVG_Y(uyvy); \
    R = COERCE(Y + yuv2rgb_RV[UYVY_GET_V(uyvy)], 0, 255); \
    G = COERCE(Y + yuv2rgb_GU[UYVY_GET_U(uyvy)] + yuv2rgb_GV[UYVY_GET_V(uyvy)], 0, 255); \
    B = COERCE(Y + yuv2rgb_BU[UYVY_GET_U(uyvy)], 0, 255); \
} \


int is_zoom_mode_so_no_zebras() 
{ 
    if (!lv) return 0;
    if (lv_dispsize == 1) return 0;
    
    return 1;
}

// true if LV image reflects accurate luma of the final picture / video
int lv_luma_is_accurate()
{
    if (is_movie_mode()) return 1;
    
    extern int digic_iso_gain_photo;
    return expsim && digic_iso_gain_photo == 1024;
}

int show_lv_fps = 0; // for debugging


//~ static struct bmp_file_t * cropmarks_array[3] = {0};
static struct bmp_file_t * cropmarks = 0;
static int _bmp_muted = false;
static int _bmp_unmuted = false;
static int bmp_is_on() { return !_bmp_muted; }
void bmp_on();
void bmp_off();

#define hist_height         54
#define HIST_WIDTH          128
#define WAVEFORM_WIDTH 180
#define WAVEFORM_HEIGHT 120
#define WAVEFORM_FACTOR (1 << waveform_size) // 1, 2 or 4
#define WAVEFORM_OFFSET (waveform_size <= 1 ? 80 : 0)

#define WAVEFORM_FULLSCREEN (waveform_draw && waveform_size == 2)

#define BVRAM_MIRROR_SIZE (BMPPITCH*540)

CONFIG_INT("lv.disp.profiles", disp_profiles_0, 0);

static CONFIG_INT("disp.mode", disp_mode, 0);
static CONFIG_INT("disp.mode.a", disp_mode_a, 1);
static CONFIG_INT("disp.mode.b", disp_mode_b, 1);
static CONFIG_INT("disp.mode.c", disp_mode_c, 1);
static CONFIG_INT("disp.mode.x", disp_mode_x, 1);

       CONFIG_INT( "transparent.overlay", transparent_overlay, 0);
static CONFIG_INT( "transparent.overlay.x", transparent_overlay_offx, 0);
static CONFIG_INT( "transparent.overlay.y", transparent_overlay_offy, 0);
int transparent_overlay_hidden = 0;

static CONFIG_INT( "global.draw",   global_draw, 3 );

#define ZEBRAS_IN_QUICKREVIEW (global_draw > 1)
#define ZEBRAS_IN_LIVEVIEW (global_draw & 1)

static CONFIG_INT( "zebra.draw",    zebra_draw, 0 );
static CONFIG_INT( "zebra.colorspace",    zebra_colorspace,   2 );// luma/rgb/lumafast
static CONFIG_INT( "zebra.thr.hi",    zebra_level_hi, 99 );
static CONFIG_INT( "zebra.thr.lo",    zebra_level_lo, 0 );
       CONFIG_INT( "zebra.rec", zebra_rec,  1 );
static CONFIG_INT( "crop.enable",   crop_enabled,   0 ); // index of crop file
static CONFIG_INT( "crop.index",    crop_index, 0 ); // index of crop file
       CONFIG_INT( "crop.movieonly", cropmark_movieonly, 0);
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
static CONFIG_INT( "zoom.overlay.x", zoom_overlay_x, 1);
static CONFIG_INT( "zoom.overlay.pos", zoom_overlay_pos, 1);
static CONFIG_INT( "zoom.overlay.split", zoom_overlay_split, 0);
//~ static CONFIG_INT( "zoom.overlay.lut", zoom_overlay_lut, 0);

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

int should_draw_zoom_overlay()
{
    if (!lv) return 0;
    if (!zoom_overlay_enabled) return 0;
    if (!zebra_should_run()) return 0;
    if (ext_monitor_rca) return 0;
    if (zoom_overlay_trigger_mode == 4) return true;
    if (zoom_overlay_triggered_by_zoom_btn || zoom_overlay_triggered_by_focus_ring_countdown) return true;
    return false;
}

static CONFIG_INT( "focus.peaking", focus_peaking, 0);
static CONFIG_INT( "focus.peaking.method", focus_peaking_method, 1);
static CONFIG_INT( "focus.peaking.thr", focus_peaking_pthr, 5); // 1%
static CONFIG_INT( "focus.peaking.color", focus_peaking_color, 7); // R,G,B,C,M,Y,cc1,cc2
CONFIG_INT( "focus.peaking.grayscale", focus_peaking_grayscale, 0); // R,G,B,C,M,Y,cc1,cc2

//~ static CONFIG_INT( "focus.graph", focus_graph, 0);

//~ static CONFIG_INT( "edge.draw", edge_draw,  0 );
static CONFIG_INT( "hist.draw", hist_draw,  1 );
static CONFIG_INT( "hist.colorspace",   hist_colorspace,    1 );
static CONFIG_INT( "hist.warn", hist_warn,  3 );
static CONFIG_INT( "hist.log",  hist_log,   1 );
//~ static CONFIG_INT( "hist.x",        hist_x,     720 - HIST_WIDTH - 4 );
//~ static CONFIG_INT( "hist.y",        hist_y,     100 );
static CONFIG_INT( "waveform.draw", waveform_draw,
#ifdef CONFIG_5D2
1
#else
0
#endif
 );
static CONFIG_INT( "waveform.size", waveform_size,  0 );
//~ static CONFIG_INT( "waveform.x",    waveform_x, 720 - WAVEFORM_WIDTH );
//~ static CONFIG_INT( "waveform.y",    waveform_y, 480 - 50 - WAVEFORM_WIDTH );
static CONFIG_INT( "waveform.bg",   waveform_bg,    COLOR_ALMOST_BLACK ); // solid black


static CONFIG_INT( "vectorscope.draw", vectorscope_draw, 0);

/* runtime-configurable size */
uint32_t vectorscope_width = 256;
uint32_t vectorscope_height = 256;
/* 128 is also a good choice, but 256 is max. U and V are using that resolution */
#define VECTORSCOPE_WIDTH_MAX 256
#define VECTORSCOPE_HEIGHT_MAX 256


       CONFIG_INT( "clear.preview", clearscreen_enabled, 0);
static CONFIG_INT( "clear.preview.delay", clearscreen_delay, 1000); // ms
       CONFIG_INT( "clear.preview.mode", clearscreen_mode, 0); // 2 is always

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
    
    if (lv && ZEBRAS_IN_LIVEVIEW)
    {
        return 
            lv_disp_mode == 0 &&
            !idle_globaldraw_disable && 
            bmp_is_on() &&
            DISPLAY_IS_ON && 
            recording != 1 && 
            #ifdef CONFIG_KILL_FLICKER
            !(lv && kill_canon_gui_mode && !canon_gui_front_buffer_disabled() && !gui_menu_shown()) &&
            #endif
            !LV_PAUSED && 
            lens_info.job_state <= 10;
    }
    
    if (!lv && ZEBRAS_IN_QUICKREVIEW)
    {
        return DISPLAY_IS_ON;
    }
        
    return 0;
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
static uint32_t hist_r[HIST_WIDTH];
static uint32_t hist_g[HIST_WIDTH];
static uint32_t hist_b[HIST_WIDTH];

/** Maximum value in the histogram so that at least one entry fills
 * the box */
static uint32_t hist_max;

/** total number of pixels analyzed by histogram */
static uint32_t hist_total_px;

static uint8_t *vectorscope = NULL;

/* helper to draw <count> pixels at given position. no wrap checks when <count> is greater 1 */
static void 
vectorscope_putpixel(uint8_t *bmp_buf, int x_pos, int y_pos, uint8_t color, uint8_t count)
{
    int pos = x_pos + y_pos * vectorscope_width;

    while(count--)
    {
        bmp_buf[pos++] = 255 - color;
    }
}

/* another helper that draws a color dot at given position.
   <xc> and <yc> specify the center of our scope graphic.
   <frac_x> and <frac_y> are in 1/2048th units and specify the relative dot position.
 */
static void 
vectorscope_putblock(uint8_t *bmp_buf, int xc, int yc, uint8_t color, int32_t frac_x, int32_t frac_y)
{
    int x_pos = xc + ((int32_t)vectorscope_width * frac_x) / 4096;
    int y_pos = yc + (-(int32_t)vectorscope_height * frac_y) / 4096;

    vectorscope_putpixel(bmp_buf, x_pos + 0, y_pos - 4, color, 1);
    vectorscope_putpixel(bmp_buf, x_pos + 0, y_pos + 4, color, 1);

    vectorscope_putpixel(bmp_buf, x_pos - 3, y_pos - 3, color, 7);
    vectorscope_putpixel(bmp_buf, x_pos - 3, y_pos - 2, color, 7);
    vectorscope_putpixel(bmp_buf, x_pos - 3, y_pos - 1, color, 7);
    vectorscope_putpixel(bmp_buf, x_pos - 4, y_pos + 0, color, 9);
    vectorscope_putpixel(bmp_buf, x_pos - 3, y_pos + 1, color, 7);
    vectorscope_putpixel(bmp_buf, x_pos - 3, y_pos + 2, color, 7);
    vectorscope_putpixel(bmp_buf, x_pos - 3, y_pos + 3, color, 7);
}

/* draws the overlay: circle with color dots. */
void vectorscope_paint(uint8_t *bmp_buf, uint32_t x_origin, uint32_t y_origin)
{    
    //int r = vectorscope_height/2 - 1;
    int xc = x_origin + vectorscope_width/2;
    int yc = y_origin + vectorscope_height/2;

    /* red block at U=-14.7% V=61.5% => U=-304/2048th V=1259/2048th */
    vectorscope_putblock(bmp_buf, xc, yc, 8, -302, 1259);
    /* green block */
    vectorscope_putblock(bmp_buf, xc, yc, 7, -593, -1055);
    /* blue block */
    vectorscope_putblock(bmp_buf, xc, yc, 9, 895, -204);
    /* cyan block */
    vectorscope_putblock(bmp_buf, xc, yc, 5, 301, -1259);
    /* magenta block */
    vectorscope_putblock(bmp_buf, xc, yc, 14, 592, 1055);
    /* yellow block */
    vectorscope_putblock(bmp_buf, xc, yc, 15, -893, 204);
}

void
vectorscope_clear()
{
    if(vectorscope != NULL)
    {
        bzero32(vectorscope, vectorscope_width * vectorscope_height * sizeof(uint8_t));
    }
}

void
vectorscope_init()
{
    if(vectorscope == NULL)
    {
        vectorscope = AllocateMemory(VECTORSCOPE_WIDTH_MAX * VECTORSCOPE_HEIGHT_MAX * sizeof(uint8_t));
        vectorscope_clear();
    }
}

static inline void
vectorscope_addpixel(uint8_t y, int8_t u, int8_t v)
{
    if(vectorscope == NULL)
    {
        return;
    }
    
    int32_t V = -v;
    int32_t U = u;

    
    /* convert YUV to vectorscope position */
    V *= vectorscope_height;
    V >>= 8;
    V += vectorscope_height >> 1;

    U *= vectorscope_width;
    U >>= 8;
    U += vectorscope_width >> 1;

    uint16_t pos = U + V * vectorscope_width;

    /* increase luminance at this position. when reaching 4*0x2A, we are at maximum. */
    if(vectorscope[pos] < (0x2A << 2))
    {
        vectorscope[pos]++;
    }
}

/* memcpy the second part of vectorscope buffer. uses only few resources */
static void
vectorscope_draw_image(uint32_t x_origin, uint32_t y_origin)
{    
    if(vectorscope == NULL)
    {
        return;
    }

    uint8_t * const bvram = bmp_vram();
    if (!bvram)
    {
        return;
    }

    vectorscope_paint(vectorscope, 0, 0);

    for(uint32_t y = 0; y < vectorscope_height; y++)
    {
        #ifdef CONFIG_4_3_SCREEN
        uint8_t *bmp_buf = &(bvram[BM(x_origin, y_origin + (EXT_MONITOR_CONNECTED ? y : y*8/9))]);
        #else
        uint8_t *bmp_buf = &(bvram[BM(x_origin, y_origin+y)]);
        #endif

        for(uint32_t x = 0; x < vectorscope_width; x++)
        {
            uint8_t brightness = vectorscope[x + y*vectorscope_width];

            int xc = x - vectorscope_height/2;
            int yc = y - vectorscope_height/2;
            int r = vectorscope_height/2 - 1;
            int inside_circle = xc*xc + yc*yc < (r-1)*(r-1);
            int on_circle = !inside_circle && xc*xc + yc*yc <= (r+1)*(r+1);
            // kdenlive vectorscope:
            // center: 175,180
            // I: 83,38   => dx=-92, dy=142
            // Q: 320,87  => dx=145, dy=93
            // let's say 660/1024 is a good approximation of the slope
            
            // wikipedia image:
            // center: 318, 294
            // I: 171, 68  => 147,226
            // Q: 545, 147 => 227,147
            // => 663/1024 is a better approximation
            
            int on_axis = (x==vectorscope_width/2) || (y==vectorscope_height/2) || (inside_circle && (xc==yc*663/1024 || -xc*663/1024==yc));

            if (on_circle || (on_axis && brightness==0))
            {
                #ifdef CONFIG_4_3_SCREEN
                bmp_buf[x] = 60;
                #else
                bmp_buf[x] = COLOR_BLACK;
                #endif
            }
            else if (inside_circle)
            {
                /* paint (semi)transparent when no pixels in this color range */
                if (brightness == 0)
                {
                    #ifdef CONFIG_4_3_SCREEN
                    bmp_buf[x] = COLOR_WHITE; // semitransparent looks bad
                    #else
                    bmp_buf[x] = (x+y)%2 ? COLOR_WHITE : 0;
                    #endif
                }
                else if (brightness > 0x26 + 0x2A * 4)
                {
                    /* some fake fixed color, for overlays */
                    bmp_buf[x] = 255 - brightness;
                }
                else
                {
                    /* 0x26 is the palette color for black plus max 0x2A until white */
                    bmp_buf[x] = 0x26 + (brightness >> 2);
                }
            }
        }
    }
}


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
        hist_r[x] = 0;
        hist_g[x] = 0;
        hist_b[x] = 0;
    }

    if (waveform_draw)
    {
        waveform_init();
    }
    
    if (vectorscope_draw)
    {
        vectorscope_init();
        vectorscope_clear();
    }

    for( y = os.y0 + os.off_169; y < os.y_max - os.off_169; y += 2 )
    {
        for( x = os.x0 ; x < os.x_max ; x += 2 )
        {
            uint32_t pixel = buf[BM2LV(x,y)/4];
            int Y;
            if (hist_colorspace == 1 && !ext_monitor_rca) // rgb
            {
                int R, G, B;
                //~ uyvy2yrgb(pixel, &Y, &R, &G, &B);
                COMPUTE_UYVY2YRGB(pixel, Y, R, G, B);
                // YRGB range: 0-255
                uint32_t R_level = R * HIST_WIDTH / 256;
                uint32_t G_level = G * HIST_WIDTH / 256;
                uint32_t B_level = B * HIST_WIDTH / 256;
                hist_r[R_level]++;
                hist_g[G_level]++;
                hist_b[B_level]++;
            }
            else // luma
            {
                uint32_t p1 = ((pixel >> 16) & 0xFF00) >> 8;
                uint32_t p2 = ((pixel >>  0) & 0xFF00) >> 8;
                Y = (p1+p2) / 2; 
            }

            hist_total_px++;
            uint32_t hist_level = Y * HIST_WIDTH / 256;

            // Ignore the 0 bin.  It generates too much noise
            unsigned count = ++ (hist[ hist_level ]);
            if( hist_level && count > hist_max )
                hist_max = count;

            // Update the waveform plot
            if (waveform_draw) 
            {
                uint8_t* w = &WAVEFORM(((x-os.x0) * WAVEFORM_WIDTH) / os.x_ex, (Y * WAVEFORM_HEIGHT) / 256);
                if ((*w) < 250) (*w)++;
            }

            if (vectorscope_draw)
            {
                int8_t U = (pixel >>  0) & 0xFF;
                int8_t V = (pixel >> 16) & 0xFF;
                vectorscope_addpixel(Y, U, V);
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
            
            int Y, R, G, B;
            //~ uyvy2yrgb(pixel, &Y, &R, &G, &B);
            COMPUTE_UYVY2YRGB(pixel, Y, R, G, B);
            
            int M = MAX(MAX(R,G),B);
            if (pixel && Y < thr_lo) (*under)++; // try to ignore black bars
            if (M > thr_hi) (*over)++;
            total++;
        }
    }
    return total;
}

static int hist_rgb_color(int y, int sizeR, int sizeG, int sizeB)
{
    switch ((y > sizeR ? 0 : 1) |
            (y > sizeG ? 0 : 2) |
            (y > sizeB ? 0 : 4))
    {
        case 0b000: return COLOR_ALMOST_BLACK; // almost black
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
    if (!PLAY_OR_QR_MODE)
    {
        if (!lv_luma_is_accurate()) return;
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
        highlight_level = highlight_level * HIST_WIDTH / 256;

    int log_max = log_length(hist_max);
    
    for( i=0 ; i < HIST_WIDTH ; i++ )
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
            else if (hist_colorspace == 1 && !ext_monitor_rca) // RGB
                *col = hist_rgb_color(y, sizeR, sizeG, sizeB);
            else
                *col = y > size ? COLOR_BG : (falsecolor_draw ? false_colour[falsecolor_palette][(i * 256 / HIST_WIDTH) & 0xFF]: COLOR_WHITE);
        }
        
        if (hist_warn && i == HIST_WIDTH - 1
            && !should_draw_zoom_overlay()) // magic zoom borders will be "overexposed" => will cause warning
        {
            unsigned int thr = hist_total_px / (
                hist_warn == 1 ? 100000 : // 0.001%
                hist_warn == 2 ? 10000  : // 0.01%
                hist_warn == 3 ? 1000   : // 0.01%
                                 100);    // 1%
            int yw = y_origin + 10 - 16 + (hist_log ? hist_height - 20 : 0);
            if (hist_colorspace == 1 && !ext_monitor_rca) // RGB
            {
                if (hist_r[i] + hist_r[i-1] + hist_r[i-2] > thr) dot(x_origin + HIST_WIDTH/2 - 20 - 16, yw, COLOR_RED   , 7);
                if (hist_g[i] + hist_g[i-1] + hist_g[i-2] > thr) dot(x_origin + HIST_WIDTH/2      - 16, yw, COLOR_GREEN1, 7);
                if (hist_b[i] + hist_b[i-1] + hist_b[i-2] > thr) dot(x_origin + HIST_WIDTH/2 + 20 - 16, yw, COLOR_LIGHTBLUE  , 7);
            }
            else
            {
                if (hist[i] + hist[i-1] + hist[i-2] > thr) dot(x_origin + HIST_WIDTH/2 - 16, yw, COLOR_RED, 7);
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

    uint8_t * const bvram = bmp_vram();
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
            uint8_t * row = bvram + x_origin + (y_origin + y * height / WAVEFORM_HEIGHT + k) * pitch;
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
                    count = waveform_bg; // transparent

                pixel |= (count << ((i & 3)<<3));

                if( (i & 3) != 3 )
                    continue;

                // Draw the pixel, rounding down to the nearest
                // quad word write (and then nop to avoid err70).
                *(uint32_t*)( row + (i & ~3) ) = pixel;
                //~ asm( "nop" );
                //~ asm( "nop" );
                //~ asm( "nop" );
                //~ asm( "nop" );
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

#if CONFIG_DEBUGMSG
void card_benchmark_wr(int bufsize, int K, int N)
{
    FIO_RemoveFile(CARD_DRIVE "ML/LOGS/bench.tmp");
    msleep(1000);
    int n = 0x10000000 / bufsize;
    {
        FILE* f = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/bench.tmp");
        int t0 = tic();
        int i;
        for (i = 0; i < n; i++)
        {
            uint32_t start = 0x40000000;
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
        void* buf = alloc_dma_memory(bufsize);
        if (buf)
        {
            FILE* f = FIO_Open(CARD_DRIVE "ML/LOGS/bench.tmp", O_RDONLY | O_SYNC);
            int t0 = tic();
            int i;
            for (i = 0; i < n; i++)
            {
                bmp_printf(FONT_LARGE, 0, 0, "[%d/%d] Reading: %d/100 (buf=%dK)... ", K, N, i * 100 / n, bufsize/1024);
                FIO_ReadFile(f, UNCACHEABLE(buf), bufsize );
            }
            FIO_CloseFile(f);
            free_dma_memory(buf);
            int t1 = tic();
            int speed = 2560 / (t1 - t0);
            console_printf("Read speed (buffer=%dk):\t %d.%d MB/s\n", bufsize/1024, speed/10, speed % 10);
        }
        else
        {
            console_printf("malloc error: buffer=%d\n", bufsize);
        }
    }

    FIO_RemoveFile(CARD_DRIVE "ML/LOGS/bench.tmp");
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
    dump_big_seg(4, CARD_DRIVE "ML/LOGS/4.bin");
    dump_big_seg(4, CARD_DRIVE "ML/LOGS/4-1.bin");
    //dump_seg(0x1000, 0x100000, CARD_DRIVE "ML/LOGS/ram.bin");
    //~ dump_seg(YUV422_IMAGE_BUFFER, 1920*1080*2, CARD_DRIVE "ML/LOGS/VRAM.BIN");
}

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


static inline int zebra_color_word_row(int c, int y)
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

#define MAX_DIRTY_PIXELS 5000

int focus_peaking_debug = 0;

static int zebra_digic_dirty = 0;

void draw_zebras( int Z )
{
    uint8_t * const bvram = bmp_vram_real();
    int zd = Z && zebra_draw && (lv_luma_is_accurate() || PLAY_OR_QR_MODE) && (zebra_rec || !recording); // when to draw zebras
    if (zd)
    {
        int zlh = zebra_level_hi * 255 / 100 - 1;
        int zll = zebra_level_lo * 255 / 100;

        if (zebra_colorspace == 2 && lv) // use regular zebras in photo mode
        {
            zebra_digic_dirty = 1;
            
            // if both zebras are enabled, alternate them (can't display both at the same time)
            // if only one is enabled, show them both
            
            int parity = (get_seconds_clock() / 2) % 2;
            
            int ov = (zebra_level_hi <= 100 && (zebra_level_lo ==   0 || parity == 0));
            int un = (zebra_level_lo  >   0 && (zebra_level_hi  > 100 || parity == 1));
            
            if (ov)
                EngDrvOut(DIGIC_ZEBRA_REGISTER, 0xC000 + zlh);
            else if (un)
                EngDrvOut(DIGIC_ZEBRA_REGISTER, 0x1d000 + zll);
            return;
        }
        
        uint8_t * lvram = get_yuv422_vram()->vram;
        lvram = (void*)YUV422_LV_BUFFER_DMA_ADDR; // this one is not updating right now, but it's a bit behind

        // draw zebra in 16:9 frame
        // y is in BM coords
        for(int y = os.y0 + os.off_169; y < os.y_max - os.off_169; y += 2 )
        {
            #define color_over           zebra_color_word_row(COLOR_RED,  y)
            #define color_under          zebra_color_word_row(COLOR_BLUE, y)
            #define color_over_2         zebra_color_word_row(COLOR_RED,  y+1)
            #define color_under_2        zebra_color_word_row(COLOR_BLUE, y+1)
            
            #define color_rgb_under      zebra_rgb_color(1, 0, 0, 0, y)
            #define color_rgb_under_2    zebra_rgb_color(1, 0, 0, 0, y+1)
            
            #define color_rgb_clipR      zebra_rgb_color(0, 1, 0, 0, y)
            #define color_rgb_clipR_2    zebra_rgb_color(0, 1, 0, 0, y+1)
            #define color_rgb_clipG      zebra_rgb_color(0, 0, 1, 0, y)
            #define color_rgb_clipG_2    zebra_rgb_color(0, 0, 1, 0, y+1)
            #define color_rgb_clipB      zebra_rgb_color(0, 0, 0, 1, y)
            #define color_rgb_clipB_2    zebra_rgb_color(0, 0, 0, 1, y+1)
            
            #define color_rgb_clipRG     zebra_rgb_color(0, 1, 1, 0, y)
            #define color_rgb_clipRG_2   zebra_rgb_color(0, 1, 1, 0, y+1)
            #define color_rgb_clipGB     zebra_rgb_color(0, 0, 1, 1, y)
            #define color_rgb_clipGB_2   zebra_rgb_color(0, 0, 1, 1, y+1)
            #define color_rgb_clipRB     zebra_rgb_color(0, 1, 0, 1, y)
            #define color_rgb_clipRB_2   zebra_rgb_color(0, 1, 0, 1, y+1)
            
            #define color_rgb_clipRGB    zebra_rgb_color(0, 1, 1, 1, y)
            #define color_rgb_clipRGB_2  zebra_rgb_color(0, 1, 1, 1, y+1)

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
                
                if (zebra_colorspace == 1 && !ext_monitor_rca) // rgb
                {
                    int Y, R, G, B;
                    //~ uyvy2yrgb(*lvp, &Y, &R, &G, &B);
                    COMPUTE_UYVY2YRGB(*lvp, Y, R, G, B);

                    if(unlikely(Y < zll)) // underexposed
                    {
                        BP = MP = color_rgb_under;
                        BN = MN = color_rgb_under_2;
                    }
                    else
                    {
                        //~ BP = MP = zebra_rgb_color(Y < zll, R > zlh, G > zlh, B > zlh, y);
                        //~ BN = MN = zebra_rgb_color(Y < zll, R > zlh, G > zlh, B > zlh, y+1);
                        
                        if (unlikely(R > zlh)) // R clipped
                        {
                            if (unlikely(G > zlh)) // RG clipped
                            {
                                if (B > zlh) // RGB clipped (all of them)
                                {
                                    BP = MP = color_rgb_clipRGB;
                                    BN = MN = color_rgb_clipRGB_2;
                                }
                                else // only R and G clipped
                                {
                                    BP = MP = color_rgb_clipRG;
                                    BN = MN = color_rgb_clipRG_2;
                                }
                            }
                            else // R clipped, G not clipped
                            {
                                if (unlikely(B > zlh)) // only R and B clipped
                                {
                                    BP = MP = color_rgb_clipRB;
                                    BN = MN = color_rgb_clipRB_2;
                                }
                                else // only R clipped
                                {
                                    BP = MP = color_rgb_clipR;
                                    BN = MN = color_rgb_clipR_2;
                                }
                            }
                        }
                        else // R not clipped
                        {
                            if (unlikely(G > zlh)) // R not clipped, G clipped
                            {
                                if (unlikely(B > zlh)) // only G and B clipped
                                {
                                    BP = MP = color_rgb_clipGB;
                                    BN = MN = color_rgb_clipGB_2;
                                }
                                else // only G clipped
                                {
                                    BP = MP = color_rgb_clipG;
                                    BN = MN = color_rgb_clipG_2;
                                }
                            }
                            else // R not clipped, G not clipped
                            {
                                if (unlikely(B > zlh)) // only B clipped
                                {
                                    BP = MP = color_rgb_clipB;
                                    BN = MN = color_rgb_clipB_2;
                                }
                                else // nothing clipped
                                {
                                    BN = MN = BP = MP = 0;
                                }
                            }
                        }
                    }
                }
                else // luma
                {
                    int p0 = (*lvp) >> 8 & 0xFF;
                    if (unlikely(p0 > zlh))
                    {
                        BP = MP = color_over;
                        BN = MN = color_over_2;
                    }
                    else if (unlikely(p0 < zll))
                    {
                        BP = MP = color_under;
                        BN = MN = color_under_2;
                    }
                    else
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

void focus_found_pixel(int x, int y, int e, int thr, uint8_t * const bvram)
{    
    int color = get_focus_color(thr, e);
    //~ int color = COLOR_RED;
    color = (color << 8) | color;   
    
    uint16_t * const b_row = (uint16_t*)( bvram + BM_R(y) );   // 2 pixels
    uint16_t * const m_row = (uint16_t*)( bvram_mirror + BM_R(y) );   // 2 pixels
    
    uint16_t pixel = b_row[x/2];
    uint16_t mirror = m_row[x/2];
    uint16_t pixel2 = b_row[x/2 + BMPPITCH/2];
    uint16_t mirror2 = m_row[x/2 + BMPPITCH/2];
    if (mirror  & 0x8080) 
        return;
    if (mirror2 & 0x8080)
        return;
    if (pixel  != 0 && pixel  != mirror )
        return;
    if (pixel2 != 0 && pixel2 != mirror2)
        return;

    b_row[x/2] = b_row[x/2 + BMPPITCH/2] = 
    m_row[x/2] = m_row[x/2 + BMPPITCH/2] = color;
    
    if (dirty_pixels_num < MAX_DIRTY_PIXELS)
    {
        dirty_pixels[dirty_pixels_num++] = (void*)&b_row[x/2] - (void*)bvram;
    }
}


// returns how the focus peaking threshold changed
static int
draw_zebra_and_focus( int Z, int F )
{
    if (unlikely(!get_global_draw())) return 0;

    uint8_t * const bvram = bmp_vram_real();
    if (unlikely(!bvram)) return 0;
    if (unlikely(!bvram_mirror)) return 0;
    
    draw_zebras(Z);

    static int thr = 50;
    static int thr_increment = 1;
    static int prev_thr = 50;
    static int thr_delta = 0;

    if (F && focus_peaking)
    {
        // clear previously written pixels
        if (unlikely(!dirty_pixels)) dirty_pixels = AllocateMemory(MAX_DIRTY_PIXELS * sizeof(int));
        if (unlikely(!dirty_pixels)) return -1;
        int i;
        for (i = 0; i < dirty_pixels_num; i++)
        {
            #define B1 *(uint16_t*)(bvram + dirty_pixels[i])
            #define B2 *(uint16_t*)(bvram + dirty_pixels[i] + BMPPITCH)
            #define M1 *(uint16_t*)(bvram_mirror + dirty_pixels[i])
            #define M2 *(uint16_t*)(bvram_mirror + dirty_pixels[i] + BMPPITCH)
            if (unlikely((B1 == 0 || B1 == M1)) && unlikely((B2 == 0 || B2 == M2)))
                B1 = B2 = M1 = M2 = 0;
            #undef B1
            #undef B2
            #undef M1
            #undef M2
        }
        dirty_pixels_num = 0;
        
        struct vram_info *hd_vram = get_yuv422_hd_vram();
        uint32_t hdvram = (uint32_t)UNCACHEABLE(hd_vram->vram);
        
        int yStart = os.y0 + os.off_169 + 8;
        int yEnd = os.y_max - os.off_169 - 8;
        int xStart = os.x0 + 8;
        int xEnd = os.x_max - 8;
        int n_over = 0;
        int n_total = ((yEnd - yStart) * (xEnd - xStart)) / 4;
        
        const uint8_t* p8; // that's a moving pointer
        
        zebra_update_lut();
        
        if(focus_peaking_method == 0)
        {
            for(int y = yStart; y < yEnd; y += 2)
            {
                uint32_t hd_row = hdvram + BM2HD_R(y);
                
                for (int x = xStart; x < xEnd; x += 2)
                {
                    p8 = (uint8_t *)(hd_row + bm_hd_x_cache[x - BMP_W_MINUS]);
                    
                    int p_cc = (int)(*p8);
                    int p_rc = (int)(*(p8 + 2));
                    int p_cd = (int)(*(p8 + vram_hd.pitch));
                    
                    int e_dx = ABS(p_rc - p_cc);
                    int e_dy = ABS(p_cd - p_cc);
                    
                    int e = MAX(e_dx, e_dy);
                    
                    /* executed for 1% of pixels */
                    if (unlikely(e >= thr))
                    {
                        n_over++;
                        if (n_over > MAX_DIRTY_PIXELS) // threshold too low, abort
                        {
                            break;
                        }

                        focus_found_pixel(x, y, e, thr, bvram);
                    }
                }
            }
        }
        else
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
                     
                    
                    int result = ((int)(*p8) * 4) - (int)(*(p8 + 2));
                    result -= (int)(*(p8 - 2));
                    result -= (int)(*(p8 + vram_hd.pitch));
                    result -= (int)(*(p8 - vram_hd.pitch));
                    
                    int e = ABS(result);
                    
                    /* executed for 1% of pixels */
                    if (unlikely(e >= thr))
                    {
                        n_over++;
                        if (n_over > MAX_DIRTY_PIXELS) // threshold too low, abort
                        {
                            break;
                        }

                        focus_found_pixel(x, y, e, thr, bvram);
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

        int thr_min = (lens_info.iso > 1600 ? 15 : 10);
        thr = COERCE(thr, thr_min, 255);


        thr_delta = thr - prev_thr;
        prev_thr = thr;

        if (n_over > MAX_DIRTY_PIXELS)
            return thr_delta;
    }

    return thr_delta;
}

void guess_focus_peaking_threshold()
{
    if (!focus_peaking) return;
    int prev_thr_delta = 1234;
    for (int i = 0; i < 50; i++)
    {
        int thr_delta = draw_zebra_and_focus(0,1);
        //~ bmp_printf(FONT_LARGE, 0, 0, "%x ", thr_delta); msleep(1000);
        if (!thr_delta) break;
        if (prev_thr_delta != 1234 && SGN(thr_delta) != SGN(prev_thr_delta)) break;
        prev_thr_delta = thr_delta;
    }
}


// clear only zebra, focus assist and whatever else is in BMP VRAM mirror
void
clrscr_mirror( void )
{
    if (!lv && !PLAY_OR_QR_MODE) return;
    if (!get_global_draw()) return;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    if (!bvram_mirror) return;

    int x, y;
    for( y = os.y0; y < os.y_max; y++ )
    {
        for( x = os.x0; x < os.x_max; x += 4 )
        {
            uint32_t* bp = (uint32_t*)bvram        + BM(x,y)/4;
            uint32_t* mp = (uint32_t*)bvram_mirror + BM(x,y)/4;
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
    struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "ML/CROPMKS/", &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "ML/CROPMKS dir missing\n"
                        "Please copy all ML files!" );
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
static void reload_cropmark()
{
    int i = crop_index;
    static int old_i = -1;
    if (i == old_i) return; 
    old_i = i;
    //~ bmp_printf(FONT_LARGE, 0, 100, "reload crop: %d", i);

    if (cropmarks)
    {
        void* old_crop = cropmarks;
        cropmarks = 0;
        FreeMemory(old_crop);
    }

    cropmark_clear_cache();
    
    if (!num_cropmarks) return;
    i = COERCE(i, 0, num_cropmarks-1);
    char bmpname[100];
    snprintf(bmpname, sizeof(bmpname), CARD_DRIVE "ML/CROPMKS/%s", cropmark_names[i]);
    cropmarks = bmp_load(bmpname,1);
    if (!cropmarks) bmp_printf(FONT_LARGE, 0, 50, "LOAD ERROR %d:%s   ", i, bmpname);
}

static void
crop_toggle( void* priv, int sign )
{
    crop_index = mod(crop_index + sign, num_cropmarks);
    //~ reload_cropmark(crop_index);
    crop_set_dirty(10);
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
            "%s, ",
            zebra_colorspace == 0 ? "Luma" :
            zebra_colorspace == 1 ? "RGB" : "LumaFast"
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
            "Focus Peak  : %s,%d.%d,%s%s",
            focus_peaking_method == 0 ? "D1xy" :
            focus_peaking_method == 1 ? "D2xy" : "Nyq.H",
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
         crop_enabled ? (num_cropmarks ? cropmark_names[index] : "N/A") : "OFF"
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
    index = COERCE(index, 0, num_cropmarks-1);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Bitmap (%d/%d)  : %s",
         index+1, num_cropmarks,
         num_cropmarks ? cropmark_names[index] : "N/A"
    );
    int h = 150;
    int w = h * 720 / 480;
    int xc = x + 315;
    int yc = y + font_large.height * 3 + 10;
    BMP_LOCK( reload_cropmark(); )
    //~ task_create("crop_reload", 0x1a, 0x1000, reload_cropmark, 0); // reloads only when needed - will be applied at next redraw though
    //~ reload_cropmark(crop_index);
    //~ ASSERT(cropmarks);
    bmp_fill(0, xc, yc, w, h);
    BMP_LOCK( bmp_draw_scaled_ex(cropmarks, xc, yc, w, h, 0); )
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
        global_draw == 0 ? "OFF" :
        global_draw == 1 ? "LiveView" :
        global_draw == 2 ? "QuickReview" :
        global_draw == 3 ? "ON, all modes" : ""
    );
    if (disp_profiles_0)
    {
        bmp_printf(FONT(FONT_LARGE, selected ? COLOR_WHITE : 55, COLOR_BLACK), x + 560, y, "DISP %d", get_disp_mode());
        if (selected) bmp_printf(FONT(FONT_MED, COLOR_CYAN, COLOR_BLACK), 720 - font_med.width * strlen(Q_BTN_NAME), y + font_large.height, Q_BTN_NAME);
    }
    if (lv && lv_disp_mode && ZEBRAS_IN_LIVEVIEW)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t)"Press " INFO_BTN_NAME " (outside ML menu) to turn Canon displays off.");
    if (global_draw && lv && !ZEBRAS_IN_LIVEVIEW)
        menu_draw_icon(x, y, MNI_WARNING, 0);
    if (global_draw && !lv && !ZEBRAS_IN_QUICKREVIEW)
        menu_draw_icon(x, y, MNI_WARNING, 0);
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
vectorscope_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Vectorscope : %s",
        *(unsigned*) priv ? "ON " : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(*(unsigned*) priv));
}



void
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
        "Clear overlays : %s",
        //~ mode ? "ON (HalfShutter)" : "OFF"
        mode == 0 ? "OFF" : 
        mode == 1 ? "HalfShutter/DofP" : 
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
            zoom_overlay_x == 0 ? "1:1" :
            zoom_overlay_x == 1 ? "2:1" :
            zoom_overlay_x == 2 ? "3:1" :
            zoom_overlay_x == 3 ? "4:1" : "err",

        zoom_overlay_trigger_mode == 0 ? "" :
            zoom_overlay_split == 0 ? "" :
            zoom_overlay_split == 1 ? ",Ss" :
            zoom_overlay_split == 2 ? ",Sz" : "err"

    );

    if (ext_monitor_rca)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Magic Zoom does not work with SD monitors");
    else if (is_movie_mode() && video_mode_fps > 30)
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
        spotmeter_formula == 2 ? "IRE -1..101" :
        spotmeter_formula == 3 ? "IRE 0..108" :
        spotmeter_formula == 4 ? "RGB" :
        spotmeter_formula == 5 ? "HSL" :
        /*spotmeter_formula == 6*/"HSV",
        spotmeter_draw && spotmeter_position ? ", AFF" : ""
    );
    menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(spotmeter_draw));
}

void get_spot_yuv_ex(int size_dxb, int dx, int dy, int* Y, int* U, int* V)
{
    struct vram_info *  vram = get_yuv422_vram();

    if( !vram->vram )
        return;
    const uint16_t*     vr = (void*) YUV422_LV_BUFFER_DMA_ADDR;
    const unsigned      width = vram->width;
    //~ const unsigned      pitch = vram->pitch;
    //~ const unsigned      height = vram->height;
    int                 x, y;

    int xcb = os.x0 + os.x_ex/2 + dx;
    int ycb = os.y0 + os.y_ex/2 + dy;
    int xcl = BM2LV_X(xcb);
    int ycl = BM2LV_Y(ycb);
    int dxl = BM2LV_DX(size_dxb);

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
}

void get_spot_yuv(int dxb, int* Y, int* U, int* V)
{
    get_spot_yuv_ex(dxb, 0, 0, Y, U, V);
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
    int ycl = BM2LV_Y(ycb);
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
    int ycl = BM2LV_Y(ycb);
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
    if (gui_menu_shown()) return;
    if (!get_global_draw()) return;
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
    
    if (spotmeter_position == 1) // AF frame
    {
        get_afframe_pos(os.x_ex, os.y_ex, &xcb, &ycb);
        xcb += os.x0;
        ycb += os.y0;
        xcb = COERCE(xcb, os.x0 + 50, os.x_max - 50);
        ycb = COERCE(ycb, os.y0 + 50, os.y_max - 50);
    }
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

    // protect the surroundings from zebras
    uint32_t* M = (uint32_t*)get_bvram_mirror();
    uint32_t* B = (uint32_t*)bmp_vram();

    int dx = spotmeter_formula <= 3 ? 26 : 52;
    int y0 = arrow_keys_shortcuts_active() ? (int)(36 - font_med.height) : (int)(-13);
    for( y = (ycb&~1) + y0 ; y <= (ycb&~1) + 36 ; y++ )
    {
        for( x = xcb - dx ; x <= xcb + dx ; x+=4 )
        {
            uint8_t* m = (uint8_t*)(&(M[BM(x,y)/4])); //32bit to 8bit 
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

    if (!arrow_keys_shortcuts_active())
    {
        bmp_draw_rect(COLOR_WHITE, xcb - dxb, ycb - dxb, 2*dxb+1, 2*dxb+1);
        bmp_draw_rect(COLOR_BLACK, xcb - dxb + 1, ycb - dxb + 1, 2*dxb+1-2, 2*dxb+1-2);
    }
    ycb += dxb + 20;
    ycb -= font_med.height/2;
    xcb -= 2 * font_med.width;

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
            xcb + font_med.width*4, ycb,
            "IRE\n%s",
            spotmeter_formula == 2 ? "-1..101" : "0..108"
        );
    }
    else
    {
        int R = COERCE(sy + 1437 * sv / 1024, 0, 255);
        int G = COERCE(sy -  352 * su / 1024 - 731 * sv / 1024, 0, 255);
        int B = COERCE(sy + 1812 * su / 1024, 0, 255);
        xcb -= font_med.width * 3/2;
        bmp_printf(
            fnt,
            xcb, ycb, 
            "#%02x%02x%02x",
            R,G,B
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
        "LV display presets  : %d", 
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
    redraw();
    //~ BMP_LOCK( show_overlay(); )
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
        redraw();
        //~ if (!transparent_overlay_hidden) BMP_LOCK( show_overlay(); )
        //~ else redraw();
    }
}

static void transparent_overlay_offset_clear(void* priv, int delta)
{
    transparent_overlay_offx = transparent_overlay_offy = 0;
}

int handle_transparent_overlay(struct event * event)
{
    if (transparent_overlay && event->param == BGMT_LV && PLAY_OR_QR_MODE)
    {
        schedule_transparent_overlay();
        return 0;
    }

    if (!get_global_draw()) return 1;

    if (transparent_overlay && liveview_display_idle() && !gui_menu_shown())
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
        #if defined(CONFIG_5D2) || defined(CONFIG_50D)
        if (event->param == BGMT_JOY_CENTER)
        #else
        if (event->param == BGMT_PRESS_SET)
        #endif
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

static PROP_INT(PROP_LCD_BRIGHTNESS_MODE, lcd_brightness_mode);

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

    #ifdef CONFIG_5D2
    if (*(int*)priv)
    {
        int backlight_mode = lcd_brightness_mode;
        if (backlight_mode == 0) // can't restore brightness properly in auto mode
        {
            menu_draw_icon(x,y, MNI_WARNING, (intptr_t) "LCD brightness is auto in Canon menu. It won't work.");
            return;
        }
    }
    #endif
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
static CONFIG_INT("defish.projection", defish_projection, 0);
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
        defish_preview ? (defish_projection ? "Panini" : "Rectilinear") : "OFF"
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
        "Level Indicator: %s",
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
        .max = 3,
        //~ .select     = menu_binary_toggle,
        .select_Q   = toggle_disp_mode_menu,
        .display    = global_draw_display,
        .icon_type = IT_BOOL,
        .help = "Enable/disable ML overlay graphics (zebra, cropmarks...)",
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
                .name = "Color space",
                .priv = &zebra_colorspace, 
                .max = 2,
                .choices = (const char *[]) {"Luma", "RGB", "Luma Fast"},
                .icon_type = IT_NAMED_COLOR,
                .help = "Luma: red/blue. RGB: color is reverse of clipped channel.",
            },
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
        .submenu_width = 650,
        //.essential = FOR_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Method",
                .priv = &focus_peaking_method, 
                .max = 1,
                .choices = (const char *[]) {"1st deriv.", "2nd deriv.", "Nyquist H"},
                .help = "Contrast detection method.",
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
            {
                .name = "Grayscale img.", 
                .priv = &focus_peaking_grayscale,
                .max = 1,
                .help = "Display LiveView image in grayscale.",
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
        .name = "Magic Zoom",
        .priv = &zoom_overlay_enabled,
        .display = zoom_overlay_display,
        .min = 0,
        .max = 1,
        .help = "Zoom box for checking focus. Can be used while recording.",
        .submenu_width = 650,
        //.essential = FOR_LIVEVIEW,
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
                .choices = (const char *[]) {"Focus box", "NorthWest", "NorthEast", "SouthEast", "SouthWest"},
                .icon_type = IT_DICE,
                .help = "Position of zoom box (fixed or linked to focus box).",
            },
            {
                .name = "Magnification", 
                .priv = &zoom_overlay_x,
                .max = 2,
                .choices = (const char *[]) {"1:1", "2:1", "3:1", "4:1"},
                .icon_type = IT_SIZE,
                .help = "Magnification: 2:1 doubles the pixels.",
            },
            #if !defined(CONFIG_50D) && !defined(CONFIG_500D)
            {
                .name = "Focus confirm", 
                .priv = &zoom_overlay_split,
                .max = 2,
                .choices = (const char *[]) {"Green Bars", "SplitScreen", "SS ZeroCross"},
                .icon_type = IT_DICE,
                .help = "How to show focus confirmation (green bars / split screen).",
            },
            #endif
            /*{
                .name = "Look-up Table", 
                .priv = &zoom_overlay_lut,
                .max = 1,
                .choices = (const char *[]) {"OFF", "CineStyle"},
                .help = "LUT for increasing contrast in the zoom box.",
            },*/
            MENU_EOL
        },
    },
    {
        .name = "Cropmarks",
        .priv = &crop_enabled,
        .display    = crop_display,
        .select     = menu_binary_toggle,
        .help = "Cropmarks or custom grids for framing.",
        //.essential = FOR_LIVEVIEW,
        .submenu_width = 650,
        .submenu_height = 270,
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
        //.essential = FOR_PLAYBACK,
    },
    {
        .name = "Defishing",
        .priv = &defish_preview, 
        .display = defish_preview_display, 
        .select = menu_binary_toggle,
        .help = "Preview straightened images from Samyang 8mm fisheye.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Projection",
                .priv = &defish_projection, 
                .max = 1,
                .choices = (const char *[]) {"Rectilinear", "Panini"},
                .icon_type = IT_DICE,
                .help = "Projection used for defishing (Rectilinear or Panini).",
            },
            MENU_EOL
        }
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
                .max = 4,
                .choices = (const char *[]) {"Percent", "0..255", "IRE -1..101", "IRE 0..108", "RGB (HTML)"},
                .icon_type = IT_DICE,
                .help = "Measurement unit for brightness level(s).",
            },
            {
                .name = "Position",
                .priv = &spotmeter_position, 
                .max = 1,
                .choices = (const char *[]) {"Center", "Focus box"},
                .icon_type = IT_DICE,
                .help = "Spotmeter position: center or linked to focus box.",
            },
            MENU_EOL
        }
    },
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
        //.essential = FOR_LIVEVIEW | FOR_PLAYBACK,
    },
    {
        .name = "Vectorscope",
        .display = vectorscope_display,
        .priv       = &vectorscope_draw,
        .max = 1,
        .help = "Shows color distribution as U-V plot. For grading & WB.",
        //.essential = FOR_LIVEVIEW,
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

struct menu_entry level_indic_menus[] = {
    #ifdef CONFIG_60D
    {
        .name = "Level Indicator", 
        .priv = &electronic_level, 
        .select = menu_binary_toggle, 
        .display = electronic_level_display,
        .help = "Electronic level indicator in 0.5 degree steps.",
        //.essential = FOR_LIVEVIEW,
    },
    #endif
};
struct menu_entry livev_dbg_menus[] = {
    {
        .name = "Show LiveV FPS",
        .priv = &show_lv_fps, 
        .max = 1,
        .help = "Show the frame rate of LiveV loop (zebras, peaking)"
    },
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
void batt_display(
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

#if defined(CONFIG_550D) || defined(CONFIG_500D)
CONFIG_INT("lcdsensor.wakeup", lcd_sensor_wakeup, 1);
#else
int lcd_sensor_wakeup = 0;
CONFIG_INT("lcdsensor.wakeup", lcd_sensor_wakeup_unused, 1);
#endif

struct menu_entry powersave_menus[] = {
{
    .name = "Powersave in LiveView...",
    .select = menu_open_submenu,
    .submenu_width = 715,
    .help = "Options for reducing power consumption during idle times.",
    .children =  (struct menu_entry[]) {
        {
            .name = "Enable power saving",
            .priv           = &idle_rec,
            .max = 2,
            .choices = (const char *[]) {"on Standby", "on Recording", "on STBY+REC"},
            .help = "If enabled, powersave (see above) works when recording too."
        },
        #if defined(CONFIG_550D) || defined(CONFIG_500D)
        {
            .name = "Use LCD sensor     ",
            .priv           = &lcd_sensor_wakeup,
            .max = 1,
            .help = "With the LCD sensor you may wakeup or force powersave mode."
        },
        #endif
        {
            .name = "Dim display",
            .priv           = &idle_display_dim_after,
            .display        = idle_display_dim_print,
            .select         = idle_timeout_toggle,
            .help = "Dim LCD display in LiveView when idle, to save power.",
            //~ .edit_mode = EM_MANY_VALUES,
        },
        {
            .name = "Turn off LCD",
            .priv           = &idle_display_turn_off_after,
            .display        = idle_display_turn_off_print,
            .select         = idle_timeout_toggle,
            .help = "Turn off display and pause LiveView when idle and not REC.",
            //~ .edit_mode = EM_MANY_VALUES,
        },
        {
            .name = "Turn off GlobalDraw",
            .priv           = &idle_display_global_draw_off_after,
            .display        = idle_display_global_draw_off_print,
            .select         = idle_timeout_toggle,
            .help = "Turn off GlobalDraw when idle, to save some CPU cycles.",
            //~ .edit_mode = EM_MANY_VALUES,
        },
        #if defined(CONFIG_60D) || defined(CONFIG_5D2)
        {
            .name = "Battery remaining",
            .display = batt_display,
            .help = "Battery remaining. Wait for 2%% discharge before reading.",
            //~ //.essential = FOR_MOVIE | FOR_PHOTO,
        },
        #endif
        MENU_EOL
    },
}
};

struct menu_entry livev_cfg_menus[] = {
    {
        .name = "LV Display Presets",
        .priv       = &disp_profiles_0,
        .select     = menu_quaternary_toggle,
        .display    = disp_profiles_0_display,
        .help = "Num. of LV display presets. Switch with " INFO_BTN_NAME " or from LiveV.",
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
    get_yuv422_vram();
    ASSERT(B);
    ASSERT(M);
    
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

void copy_zebras_from_mirror()
{
    uint32_t* B = (uint32_t*)bmp_vram();
    uint32_t* M = (uint32_t*)get_bvram_mirror();
    ASSERT(B);
    ASSERT(M);
    get_yuv422_vram();
    for (int i = os.y0; i < os.y_max; i++)
    {
        for (int j = os.x0; j < os.x_max; j+=4)
        {
            uint32_t p = B[BM(j,i)/4];
            uint32_t m = M[BM(j,i)/4];
            if (p != 0) continue;
            B[BM(j,i)/4] = m & ~0x80808080;
        }
    }
}

void clear_zebras_from_mirror()
{
    uint8_t* M = (uint8_t*)get_bvram_mirror();
    get_yuv422_vram();
    for (int i = os.y0; i < os.y_max; i++)
    {
        for (int j = os.x0; j < os.x_max; j++)
        {
            uint8_t m = M[BM(j,i)];
            if (m & 0x80) continue;
            M[BM(j,i)] = 0;
        }
    }
}

void cropmark_clear_cache()
{
    BMP_LOCK(
        clrscr_mirror();
        bvram_mirror_clear();
        default_movie_cropmarks();
    )
}

static void 
cropmark_draw()
{
    if (!get_global_draw()) return;

    get_yuv422_vram(); // just to refresh VRAM params
    clear_lv_affframe_if_dirty();

    if (transparent_overlay && !transparent_overlay_hidden && !PLAY_MODE)
    {
        show_overlay();
        zoom_overlay_dirty = 1;
        cropmark_cache_dirty = 1;
    }
    crop_dirty = 0;

    BMP_LOCK( reload_cropmark()); // reloads only when changed

    // this is very fast
    if (cropmark_cache_is_valid())
    {
        clrscr_mirror();
        cropmark_draw_from_cache();
        //~ bmp_printf(FONT_MED, 50, 50, "crop cached");
        //~ info_led_blink(5, 10, 10);
        goto end;
    }

    if (
            (!crop_enabled) ||
            (cropmark_movieonly && !is_movie_mode() && !PLAY_OR_QR_MODE)
       )
    {
        // Cropmarks disabled (or not shown in this mode)
        // Generate and draw default cropmarks
        cropmark_clear_cache();
        cropmark_draw_from_cache();
        //~ info_led_blink(5,50,50);
        goto end;
    }
    
    if (cropmarks) 
    {
        // Cropmarks enabled, but cache is not valid
        if (!lv) msleep(500); // let the bitmap buffer settle, otherwise ML may see black image and not draw anything (or draw half of cropmark)
        clrscr_mirror(); // clean any remaining zebras / peaking
        bmp_draw_scaled_ex(cropmarks, os.x0, os.y0, os.x_ex, os.y_ex, bvram_mirror);
        //~ info_led_blink(5,50,50);
        //~ bmp_printf(FONT_MED, 50, 50, "crop regen");
        goto end;
    }

end:
    cropmark_cache_dirty = 0;
    zoom_overlay_dirty = 1;
    crop_dirty = 0;
}

int cropmark_cache_is_valid()
{
    int ans = 1;
    if (cropmark_cache_dirty) return 0;
    
    get_yuv422_vram(); // update VRAM params if needed

    // check if cropmark cache is still valid
    int sig = 
        crop_index * 13579 + crop_enabled * 14567 +
        os.x0*811 + os.y0*467 + os.x_ex*571 + os.y_ex*487 + (is_movie_mode() ? 113 : 0) + lv;

    static int prev_sig = 0;
    if (prev_sig != sig)
    {
        cropmark_cache_dirty = 1;
        ans = 0;
    }
    prev_sig = sig;
    return ans;
}

static void
cropmark_redraw()
{
    if (!zebra_should_run() && !PLAY_OR_QR_MODE) return;
    if (!cropmark_cache_is_valid())
        cropmark_clear_cache();
    BMP_LOCK( cropmark_draw(); )
}


// those functions will do nothing if called multiple times (it's safe to do this)
// they might cause ERR80 if called while taking a picture

int is_safe_to_mess_with_the_display(int timeout_ms)
{
    int k = 0;
    while (lens_info.job_state >= 10 || !DISPLAY_IS_ON || recording == 1)
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
    if (!_bmp_unmuted) 
    {// BMP_LOCK(GMT_LOCK( if (is_safe_to_mess_with_the_display(0)) {call("MuteOff"); _bmp_muted = 0;}))
    #if defined(CONFIG_500D) || defined(CONFIG_50D)// || defined(CONFIG_5D2)
        canon_gui_enable_front_buffer(1);
        _bmp_muted = false; _bmp_unmuted = true;
    #else
        BMP_LOCK(
            int f = cli_save();
            if (DISPLAY_IS_ON)
            {
                MuteOff_0();
                _bmp_muted = false; _bmp_unmuted = true;
            }
            sei_restore(f);
        )
    #endif
    }
}
void bmp_on_force()
{
    _bmp_muted = true; _bmp_unmuted = false;
    bmp_on();
}
void bmp_off()
{
    //~ return;
    //~ clrscr();
    //~ if (!is_safe_to_mess_with_the_display(500)) return;
    if (!_bmp_muted) //{ BMP_LOCK(GMT_LOCK( if (is_safe_to_mess_with_the_display(0)) { call("MuteOn")); ) }}
    {
    #if defined(CONFIG_500D) || defined(CONFIG_50D)// || defined(CONFIG_5D2)
        _bmp_muted = true; _bmp_unmuted = false;
        canon_gui_disable_front_buffer();
        clrscr();
    #else
        BMP_LOCK(
            int f = cli_save();
            if (DISPLAY_IS_ON)
            {
                _bmp_muted = true; _bmp_unmuted = false;
                MuteOn_0();
            }
            sei_restore(f);
        )
    #endif
    }
}

void bmp_mute_flag_reset()
{
    _bmp_muted = 0;
    _bmp_unmuted = 0;
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
    if (!get_global_draw()) return 1;
    #ifdef CONFIG_600D
    if (get_disp_pressed()) return 1;
    #endif

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


/*static const unsigned char TechnicolLUT[256] = {
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,3,3,3,4,4,5,5,5,6,6,7,8,8,9,9,10,10,11,12,12,13,14,15,15,16,17,18,18,19,20,21,22,23,24,25,26,26,27,28,29,30,32,33,34,35,36,37,38,39,40,41,43,44,45,46,47,49,50,51,52,54,55,56,58,59,60,62,63,64,66,67,68,70,71,73,74,75,77,78,80,81,83,84,86,87,89,90,92,93,95,96,98,99,101,102,104,105,107,109,110,112,113,115,116,118,119,121,123,124,126,127,129,130,132,134,135,137,138,140,141,143,145,146,148,149,151,152,154,155,157,158,160,161,163,164,166,167,169,170,172,173,175,176,178,179,181,182,183,185,186,188,189,190,192,193,194,196,197,198,200,201,202,204,205,206,207,209,210,211,212,213,214,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,235,236,237,238,239,239,240,241,242,242,243,244,244,245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,252,253,253,253,253,254,254,254,254,255,255,255,255,255,255,255,255,255,255 
};*/

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

static void yuvcpy_x3(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
    src = (void*)((unsigned int)src & 0xFFFFFFFC);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 3)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        *(dst)   = chroma | (luma1 << 8) | (luma1 << 24);
        *(dst+1) = chroma | (luma1 << 8) | (luma2 << 24);
        *(dst+2) = chroma | (luma2 << 8) | (luma2 << 24);
    }
}

/*
static void yuvcpy_x3_lut(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
    src = (void*)((unsigned int)src & 0xFFFFFFFC);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 3)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        luma1 = TechnicolLUT[luma1];
        luma2 = TechnicolLUT[luma2];
        *(dst)   = chroma | (luma1 << 8) | (luma1 << 24);
        *(dst+1) = chroma | (luma1 << 8) | (luma2 << 24);
        *(dst+2) = chroma | (luma2 << 8) | (luma2 << 24);
    }
}
*/

/*static void yuvcpy_x4(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = (void*)((unsigned int)dst & 0xFFFFFFFC);
    src = (void*)((unsigned int)src & 0xFFFFFFFC);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 4)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        *(dst) = *(dst+1) = chroma | (luma1 << 8) | (luma1 << 24);
        *(dst+2) = *(dst+3) = chroma | (luma2 << 8) | (luma2 << 24);
    }
}*/
/*
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
}*/
/*
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
}*/

static void yuvcpy_main(uint32_t* dst, uint32_t* src, int num_pix, int X, int lut)
{
    if (X==1)
    {
        //~ if (lut) yuvcpy_lut(dst, src, num_pix);
        //~ else 
        memcpy(dst, src, num_pix*2);
    }
    else if (X==2)
    {
        //~ if (lut) yuvcpy_x2_lut(dst, src, num_pix/2);
        //~ else 
        yuvcpy_x2(dst, src, num_pix/2);
    }
    else if (X==3)
    {
        //~ if (lut) yuvcpy_x3_lut(dst, src, num_pix/3);
        //~ else 
        yuvcpy_x3(dst, src, num_pix/3);
    }
    //~ else if (X==4)
    //~ {
        //~ if (lut) yuvcpy_x4(dst, src, num_pix/4);
        //~ else yuvcpy_x4(dst, src, num_pix/2);
    //~ }
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
    
    #if defined(CONFIG_50D) || defined(CONFIG_500D)
    zoom_overlay_split = 0; // 50D doesn't report focus
    #endif
    
    struct vram_info *  lv = get_yuv422_vram();
    lv->vram = (void*)get_fastrefresh_422_buf();
    struct vram_info *  hd = get_yuv422_hd_vram();
    
    //~ lv->width = 1920;

    if( !lv->vram ) return;
    if( !hd->vram ) return;
    if( !bmp_vram()) return;

    uint16_t*       lvr = (uint16_t*) lv->vram;
    uint16_t*       hdr = (uint16_t*) hd->vram;

    //~ lvr = get_lcd_422_buf();
    
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
    
    //~ int x2 = zoom_overlay_x2;
    int X = zoom_overlay_x + 1;

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
    //~ bmp_printf(FONT_LARGE, 50, 50, "%d,%d %d,%d", W, H, aff_x0_lv);

    if (zoom_overlay_pos)
    {
        int w = W * lv->width / hd->width;
        int h = H * lv->width / hd->width;

        w /= X;
        h /= X;
        if (video_mode_fps <= 30 || !is_movie_mode())
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

    uint16_t* d = lvr + x0c + (y0c + 2) * lv->width;
    uint16_t* s = hdr + (aff_y0_hd - (H/2/X)) * hd->width + (aff_x0_hd - (W/2/X));
    for (y = 2; y < H-2; y++)
    {
        int off = zoom_overlay_split ? (y < H/2 ? rawoff : -rawoff) : 0;
        if (rev) off = -off;
        yuvcpy_main((uint32_t*)d, (uint32_t*)(s + off), W, X, 0 /*zoom_overlay_lut*/);
        d += lv->width;
        if (y%X==0) s += hd->width;
    }

    if (video_mode_fps <= 30 || !is_movie_mode())
    {
        memset(lvr + x0c + COERCE(0   + y0c, 0, 720) * lv->width, rawoff ? 0    : 0x80, W<<1);
        memset(lvr + x0c + COERCE(1   + y0c, 0, 720) * lv->width, rawoff ? 0xFF : 0x80, W<<1);
        memset(lvr + x0c + COERCE(H-1 + y0c, 0, 720) * lv->width, rawoff ? 0xFF : 0x80, W<<1);
        memset(lvr + x0c + COERCE(H   + y0c, 0, 720) * lv->width, rawoff ? 0    : 0x80, W<<1);
    }
    if (dirty) bmp_fill(0, LV2BM_X(x0c), LV2BM_Y(y0c), LV2BM_DX(W), LV2BM_DY(H));
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
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    extern thunk LiveViewApp_handler;
    extern uintptr_t new_LiveViewApp_handler;
    //~ extern thunk test_minimal_handler;

/*
    if (dialog->handler == (dialog_handler_t) &test_minimal_handler)
    { // ML is clearing the screen with a fake dialog, let's see what's underneath
        current = current->next;
        dialog = current->priv;
    }
    */

    return
        LV_NON_PAUSED && 
        DISPLAY_IS_ON &&
        !menu_active_and_not_hidden() && 
        ( gui_menu_shown() || // force LiveView when menu is active, but hidden
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
        !is_zoom_mode_so_no_zebras() &&
        !(clearscreen == 1 && (get_halfshutter_pressed() || dofpreview)) &&
        !WAVEFORM_FULLSCREEN;
}

int livev_for_playback_running = 0;
void draw_livev_for_playback()
{
    if (!PLAY_MODE && !QR_MODE) return;

    livev_for_playback_running = 1;
    get_yuv422_vram(); // just to refresh VRAM params
    
BMP_LOCK(

    bvram_mirror_clear(); // may be filled with liveview cropmark / masking info, not needed in play mode
    clrscr();

    // don't draw cropmarks in QR mode (buggy on 4:3 screens)
    if (!QR_MODE) cropmark_redraw();

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
        guess_focus_peaking_threshold();
        draw_zebra_and_focus(1,1);
    }
    
    draw_histogram_and_waveform(1);

    bvram_mirror_clear(); // may remain filled with playback zebras 
)
    livev_for_playback_running = 0;
}

int should_draw_bottom_graphs()
{
    if (!lv) return 0;
    if (gui_menu_shown()) return 0;
    int screen_layout = get_screen_layout();
    if (screen_layout == SCREENLAYOUT_4_3 && lv_disp_mode == 0) return 1;
    return 0;
}

void draw_histogram_and_waveform(int allow_play)
{
    if (menu_active_and_not_hidden()) return;
    if (!get_global_draw()) return;
    
    get_yuv422_vram();


    if (hist_draw || waveform_draw || vectorscope_draw)
    {
        hist_build();
    }
    
    //~ if (menu_active_and_not_hidden()) return; // hack: not to draw histo over menu
    if (!get_global_draw()) return;
    if (!liveview_display_idle() && !(PLAY_OR_QR_MODE && allow_play)) return;
    if (is_zoom_mode_so_no_zebras()) return;

//    int screen_layout = get_screen_layout();

    if( hist_draw && !WAVEFORM_FULLSCREEN)
    {
        if (should_draw_bottom_graphs())
            BMP_LOCK( hist_draw_image( os.x0 + 50,  480 - hist_height - 1, -1); )
        else
            BMP_LOCK( hist_draw_image( os.x_max - HIST_WIDTH - 5, os.y0 + 100, -1); )
    }

    if (should_draw_zoom_overlay()) return;

    //~ if (menu_active_and_not_hidden()) return;
    if (!get_global_draw()) return;
    if (!liveview_display_idle() && !(PLAY_OR_QR_MODE && allow_play)) return;
    if (is_zoom_mode_so_no_zebras()) return;
        
    if( waveform_draw)
    {
        if (should_draw_bottom_graphs() && WAVEFORM_FACTOR == 1)
            BMP_LOCK( waveform_draw_image( os.x0 + 250,  480 - 54, 54); )
        else
            BMP_LOCK( waveform_draw_image( os.x_max - WAVEFORM_WIDTH*WAVEFORM_FACTOR - (WAVEFORM_FULLSCREEN ? 0 : 4), os.y_max - WAVEFORM_HEIGHT*WAVEFORM_FACTOR - WAVEFORM_OFFSET, WAVEFORM_HEIGHT*WAVEFORM_FACTOR ); )
    }
    
    if(vectorscope_draw)
    {
        /* make sure memory address of bvram will be 4 byte aligned */
        BMP_LOCK( vectorscope_draw_image(os.x0 + 32, 64); )
    }
}

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

void idle_force_powersave_in_1s()
{
    idle_countdown_display_off = 10;
    idle_countdown_display_dim = 10;
    idle_countdown_globaldraw  = 10;
}

void idle_wakeup_reset_counters(int reason) // called from handle_buttons
{
    if (ml_shutdown_requested) return;
    
#if 0
    NotifyBox(2000, "wakeup: %d   ", reason);
#endif

    //~ bmp_printf(FONT_LARGE, 50, 50, "wakeup: %d   ", reason);
    
    if (lv && reason == GMT_OLC_INFO_CHANGED) return;
    
    // when sensor is covered, timeout changes to 3 seconds
    int sensor_status = lcd_sensor_wakeup && display_sensor && DISPLAY_SENSOR_POWERED;

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
    if ((liveview_display_idle() && !get_halfshutter_pressed() && !gui_menu_shown()) || !DISPLAY_IS_ON)
    {
        if (*countdown)
            (*countdown)--;
    }
    else
    {
        idle_wakeup_reset_counters(-100); // will reset all idle countdowns
    }
    
    int sensor_status = lcd_sensor_wakeup && display_sensor && DISPLAY_SENSOR_POWERED;
    static int prev_sensor_status = 0;

    if (sensor_status != prev_sensor_status)
        idle_wakeup_reset_counters(-1);
    
    prev_sensor_status = sensor_status;
}

static void idle_action_do(int* countdown, int* prev_countdown, void(*action_on)(void), void(*action_off)(void))
{
    if (ml_shutdown_requested) return;
    
    update_idle_countdown(countdown);
    int c = *countdown; // *countdown may be changed by "wakeup" => race condition
    //~ bmp_printf(FONT_MED, 100, 200, "%d->%d ", *prev_countdown, c);
    if (*prev_countdown && !c)
    {
        info_led_blink(1, 50, 50);
        //~ bmp_printf(FONT_MED, 100, 200, "action  "); msleep(500);
        action_on();
        //~ msleep(500);
        //~ bmp_printf(FONT_MED, 100, 200, "        ");
    }
    else if (!*prev_countdown && c)
    {
        info_led_blink(1, 50, 50);
        //~ bmp_printf(FONT_MED, 100, 200, "unaction"); msleep(500);
        action_off();
        //~ msleep(500);
        //~ bmp_printf(FONT_MED, 100, 200, "        ");
    }
    *prev_countdown = c;
}

int lv_zoom_before_pause = 0;
void PauseLiveView() // this should not include "display off" command
{
    if (ml_shutdown_requested) return;
    if (sensor_cleaning) return;
    if (PLAY_MODE) return;
    if (MENU_MODE) return;
    if (LV_NON_PAUSED)
    {
        //~ ASSERT(DISPLAY_IS_ON);
        int x = 1;
        //~ while (get_halfshutter_pressed()) msleep(MIN_MSLEEP);
        BMP_LOCK(
            lv_zoom_before_pause = lv_dispsize;
            prop_request_change(PROP_LV_ACTION, &x, 4);
            msleep(100);
            clrscr();
            lv_paused = 1;
            lv = 1;
        )
        ASSERT(LV_PAUSED);
    }
}

// returns 1 if it did wakeup
int ResumeLiveView()
{
    if (ml_shutdown_requested) return 0;
    if (sensor_cleaning) return 0;
    if (PLAY_MODE) return 0;
    if (MENU_MODE) return 0;
    int ans = 0;
    if (LV_PAUSED)
    {
        lv = 0;
        int x = 0;
        //~ while (get_halfshutter_pressed()) msleep(MIN_MSLEEP);
        BMP_LOCK(
            prop_request_change(PROP_LV_ACTION, &x, 4);
            while (!lv) msleep(100);
            while (!DISPLAY_IS_ON) msleep(100);
        )
        set_lv_zoom(lv_zoom_before_pause);
        msleep(100);
        ASSERT(LV_NON_PAUSED);
        ans = 1;
        //~ ASSERT(DISPLAY_IS_ON);
    }
    lv_paused = 0;
    return ans;
}

static void idle_display_off()
{
    extern int motion_detect;
    
    if (!is_intervalometer_running())
    {
        wait_till_next_second();

        if (motion_detect || recording)
        {
            NotifyBox(3000, "DISPLAY OFF...");
        }
        else
        {
            NotifyBox(3000, "DISPLAY AND SENSOR OFF...");
        }

        if (!(lcd_sensor_wakeup && display_sensor && DISPLAY_SENSOR_POWERED))
        {
            for (int i = 0; i < 30; i++)
            {
                if (idle_countdown_display_off) { NotifyBoxHide(); return; }
                msleep(100);
            }
        }
    }
    if (!(motion_detect || recording)) PauseLiveView();
    display_off();
    msleep(100);
    idle_countdown_display_off = 0;
    ASSERT(!(recording && LV_PAUSED));
    ASSERT(!DISPLAY_IS_ON);
}
static void idle_display_on()
{
    //~ card_led_blink(5, 50, 50);
    ResumeLiveView();
    display_on();
    redraw();
    //~ ASSERT(DISPLAY_IS_ON); // it will take a short time until display will turn on
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
    ASSERT(lv);
    #ifdef CONFIG_5D2
    int backlight_mode = lcd_brightness_mode;
    if (backlight_mode == 0) // can't restore brightness properly in auto mode
    {
        NotifyBox(2000, "LCD brightness is automatic.\n"
                        "ML will not dim the display.");
        return;
    }
    #endif

    old_backlight_level = backlight_level;
    set_backlight_level(1);
}
static void idle_display_undim()
{
    if (old_backlight_level)
    {
        set_backlight_level(old_backlight_level);
        old_backlight_level = 0;
    }
}

void idle_globaldraw_dis()
{
    //~ ASSERT(lv);
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
        get_yuv422_vram();
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
        canon_gui_enable_front_buffer(0);
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

    TASK_LOOP
    {
clearscreen_loop:
        //~ msleep(100);
        //~ if (lens_info.job_state == 0 && !DISPLAY_IS_ON) // unsafe when taking pics, not needed with display on
        //~ {
            //~ call("DisablePowerSave");
            //~ call("EnablePowerSave");
        //~ }
        msleep(100);
        //~ card_led_blink(1,10,90);
        
        //~ bmp_printf(FONT_MED, 100, 100, "%d %d %d", idle_countdown_display_dim, idle_countdown_display_off, idle_countdown_globaldraw);

        // Here we're blinking the info LED approximately once every five
        // seconds to show the user that their camera is still on and has
        // not dropped into standby mode.  But it's distracting to blink
        // it every five seconds, and if the user pushed a button recently
        // then they already _know_ that their camera is still on, so
        // let's only do it if the camera's buttons have been idle for at
        // least 30 seconds.
        if (k % 50 == 0 && !DISPLAY_IS_ON && lens_info.job_state == 0 && !recording && !get_halfshutter_pressed() && !is_intervalometer_running())
            if ((get_seconds_clock() - get_last_time_active()) > 30)
                info_led_blink(1, 10, 10);

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
                int idle = liveview_display_idle() && lv_disp_mode == 0;
                if (idle)
                {
                    if (!canon_gui_front_buffer_disabled())
                        idle_kill_flicker();
                }
                else
                {
                    if (canon_gui_front_buffer_disabled())
                        idle_stop_killing_flicker();
                }
                static int prev_idle = 0;
                if (!idle && prev_idle != idle) redraw();
                prev_idle = idle;
            }
        }
        #endif
        
        if (clearscreen == 3)
        {
            if (liveview_display_idle() && !gui_menu_shown())
            {
                bmp_off();
            }
            else
            {
                bmp_on();
            }
        }
        
        if (k % 100 == 0)
        {
            if (show_lv_fps) bmp_printf(FONT_MED, 50, 50, "%d.%d fps ", fps_ticks/10, fps_ticks%10);
            fps_ticks = 0;
        }

        // clear overlays on shutter halfpress
        if (clearscreen == 1 && (get_halfshutter_pressed() || dofpreview) && !gui_menu_shown())
        {
            BMP_LOCK( clrscr_mirror(); )
            int i;
            for (i = 0; i < (int)clearscreen_delay/20; i++)
            {
                if (i % 10 == 0 && liveview_display_idle()) BMP_LOCK( update_lens_display(); )
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

        if (recording && idle_rec == 0) // don't go to powersave when recording
            idle_wakeup_reset_counters(-2345);

        if (!recording && idle_rec == 1) // don't go to powersave when not recording
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
            if (global_draw && !gui_menu_shown() && lv_disp_mode == 0)
                idle_action_do(&idle_countdown_killflicker, &idle_countdown_killflicker_prev, idle_kill_flicker, idle_stop_killing_flicker);
        }
        #endif

        // since this task runs at 10Hz, I prefer cropmark redrawing here

        if (crop_dirty && lv && zebra_should_run())
        {
            crop_dirty--;
            
            //~ bmp_printf(FONT_MED, 50, 100, "crop: cache=%d dirty=%d ", cropmark_cache_is_valid(), crop_dirty);

            // if cropmarks are disabled, we will still draw default cropmarks (fast)
            if (!(crop_enabled && cropmark_movieonly && !is_movie_mode())) 
                crop_dirty = MIN(crop_dirty, 4);
            
            // if cropmarks are cached, we can redraw them fast
            if (cropmark_cache_is_valid() && !should_draw_zoom_overlay() && !get_halfshutter_pressed())
                crop_dirty = MIN(crop_dirty, 4);
                
            if (crop_dirty == 0)
                cropmark_redraw();
        }
    }
}

TASK_CREATE( "cls_task", clearscreen_task, 0, 0x1a, 0x2000 );

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

#if defined(CONFIG_60D) || defined(CONFIG_600D)
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

    //~ if (cropmark_cache_is_valid()) cropmark_redraw();
    //~ else 
    crop_set_dirty(cropmark_cache_is_valid() ? 2 : 10);
    
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
void lens_display_set_dirty() 
{ 
    lens_display_dirty = 4; 
    if (menu_active_but_hidden()) // in this case, menu will display bottom bar, force a redraw
        menu_set_dirty(); 
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

int is_focus_peaking_enabled()
{
    return
        focus_peaking &&
        (lv || (QR_MODE && ZEBRAS_IN_QUICKREVIEW))
        && get_global_draw()
        && !should_draw_zoom_overlay()
    ;
}

static void digic_zebra_cleanup()
{
    if (!DISPLAY_IS_ON) return;
    EngDrvOut(DIGIC_ZEBRA_REGISTER, 0); 
    zebra_digic_dirty = 0;
}


// Items which need a high FPS
// Magic Zoom, Focus Peaking, zebra*, spotmeter*, false color*
// * = not really high FPS, but still fluent
 static void
livev_hipriority_task( void* unused )
{
    msleep(1000);
    find_cropmarks();
    update_disp_mode_bits_from_params();

    TASK_LOOP
    {
        //~ vsync(&YUV422_LV_BUFFER_DMA_ADDR);
        fps_ticks++;

        while (is_mvr_buffer_almost_full())
        {
            msleep(100);
        }
        
        get_422_hd_idle_buf(); // just to keep it up-to-date
        
        int zd = zebra_draw && (lv_luma_is_accurate() || PLAY_OR_QR_MODE) && (zebra_rec || !recording); // when to draw zebras (should match the one from draw_zebra_and_focus)
        if (zebra_digic_dirty && !zd) digic_zebra_cleanup();
        
        if (!zebra_should_run())
        {
            while (clearscreen == 1 && (get_halfshutter_pressed() || dofpreview)) msleep(100);
            if (!zebra_should_run())
            {
                if (zebra_digic_dirty) digic_zebra_cleanup();
                if (lv && !gui_menu_shown()) redraw();
                #ifdef CONFIG_60D
                disable_electronic_level();
                #endif
                while (!zebra_should_run()) 
                {
                    msleep(100);
                }
                vram_params_set_dirty();
                zoom_overlay_triggered_by_focus_ring_countdown = 0;
                msleep(500);
            }
        }
        #if 0
        draw_cropmark_area(); // just for debugging
        struct vram_info * lv = get_yuv422_vram();
        struct vram_info * hd = get_yuv422_hd_vram();
        bmp_printf(FONT_MED, 100, 100, "ext:%d%d%d \nlv:%x %dx%d \nhd:%x %dx%d ", ext_monitor_rca, ext_monitor_hdmi, hdmi_code, lv->vram, lv->width, lv->height, hd->vram, hd->width, hd->height);
        #endif

        //~ lv_vsync();
        guess_fastrefresh_direction();

        if (should_draw_zoom_overlay())
        {
            msleep(k % 50 == 0 ? MIN_MSLEEP : 10);
            if (zoom_overlay_dirty) BMP_LOCK( clrscr_mirror(); )
            BMP_LOCK( if (lv) draw_zoom_overlay(zoom_overlay_dirty); )
            zoom_overlay_dirty = 0;
            //~ crop_set_dirty(10); // don't draw cropmarks while magic zoom is active
            // but redraw them after MZ is turned off
        }
        else
        {
            if (!zoom_overlay_dirty) { crop_set_dirty(5); msleep(700); } // redraw cropmarks after MZ is turned off
            
            msleep(MIN_MSLEEP);
            zoom_overlay_dirty = 1;
            if (falsecolor_draw)
            {
                if (k % 4 == 0)
                    BMP_LOCK( if (lv) draw_false_downsampled(); )
            }
            else if (defish_preview)
            {
                if (k % 2 == 0)
                    BMP_LOCK( if (lv) defish_draw(); )
            }
            else
            {
                #ifdef CONFIG_5D3
                BMP_LOCK( if (lv) draw_zebra_and_focus(focus_peaking==0 || k % 2 == 1, 1) ) // DIGIC 5 has more CPU power
                #else
                // luma zebras are fast
                // also, if peaking is off, zebra can be faster
                BMP_LOCK( if (lv) draw_zebra_and_focus(k % (focus_peaking ? 4 : 2) == 0, k % 2 == 1); )
                #endif
            }
            if (MIN_MSLEEP <= 10) msleep(MIN_MSLEEP);
        }

        int s = get_seconds_clock();
        static int prev_s = 0;
        if (spotmeter_draw && s != prev_s)
            BMP_LOCK( if (lv) spotmeter_step(); )
        prev_s = s;

        #ifdef CONFIG_60D
        if (electronic_level && k % 8 == 5)
            BMP_LOCK( if (lv) show_electronic_level(); )
        #endif

        if (k % 8 == 7) rec_notify_continuous(0);
        
        if (zoom_overlay_triggered_by_focus_ring_countdown)
        {
            zoom_overlay_triggered_by_focus_ring_countdown--;
        }
        
        //~ if ((lv_disp_mode == 0 && LV_BOTTOM_BAR_DISPLAYED) || get_halfshutter_pressed())
            //~ crop_set_dirty(20);
        
        //~ if (lens_display_dirty)
        
        int m = 100;
        if (lens_display_dirty) m = 10;
        if (should_draw_zoom_overlay()) m = 100;
        
        int kmm = k % m;
        if (!gui_menu_shown()) // don't update everything in one step, to reduce magic zoom flicker
        {
            #if defined(CONFIG_550D) || defined(CONFIG_5D2)
            if (kmm == 0)
                BMP_LOCK( if (lv) black_bars(); )
            #endif

            if (kmm == 2)
            {
                BMP_LOCK( if (lv) update_lens_display(1,0); );
                if (lens_display_dirty) lens_display_dirty--;
            }

            if (kmm == 8)
            {
                BMP_LOCK( if (lv) update_lens_display(0,1); );
                if (lens_display_dirty) lens_display_dirty--;
            }

            if (kmm == 5)
                if (lv) movie_indicators_show();
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
    msleep(100);
    while (is_mvr_buffer_almost_full()) msleep(100);
}

static void black_bars()
{
    if (!get_global_draw()) return;
    if (!is_movie_mode()) return;
    int i,j;
    uint8_t * const bvram = bmp_vram();
    get_yuv422_vram();
    ASSERT(bvram);
    for (i = os.y0; i < MIN(os.y_max+1, BMP_H_PLUS); i++)
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
    #ifndef CONFIG_50D
    if (expsim != 2) return;
    #endif
    int i,j;
    uint8_t * const bvram_mirror = get_bvram_mirror();
    get_yuv422_vram();
    ASSERT(bvram_mirror);
    for (i = os.y0; i < MIN(os.y_max+1, BMP_H_PLUS); i++)
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
    if (!is_movie_mode()) return;
    get_yuv422_vram();
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
    msleep(500);
    TASK_LOOP
    {
        if (transparent_overlay_flag)
        {
            transparent_overlay_from_play();
            transparent_overlay_flag = 0;
        }

        // here, redrawing cropmarks does not block fast zoom
        if (cropmarks_play && PLAY_MODE && DISPLAY_IS_ON)
        {
            //~ beep();
            msleep(500);
            cropmark_redraw();
        }

        static int qr_zebras_drawn = 0; // zebras in QR should only be drawn once
        extern int hdr_enabled;
        if (ZEBRAS_IN_QUICKREVIEW && QR_MODE && get_global_draw() && !hdr_enabled)
        {
            if (!qr_zebras_drawn)
            {
                msleep(500);
                draw_livev_for_playback();
                if (cropmarks_play) cropmark_redraw();
                qr_zebras_drawn = 1;
            }
        }
        else qr_zebras_drawn = 0;

        loprio_sleep();
        if (!zebra_should_run())
        {
            if (WAVEFORM_FULLSCREEN && liveview_display_idle() && get_global_draw() && !is_zoom_mode_so_no_zebras() && !gui_menu_shown())
            {
                if (get_halfshutter_pressed()) clrscr();
                else draw_histogram_and_waveform(0);
            }
            continue;
        }

        loprio_sleep();

        if (!gui_menu_shown())
        {
            draw_histogram_and_waveform(0);
        }
    }
}

#define HIPRIORITY_TASK_PRIO 0x18

TASK_CREATE( "livev_hiprio_task", livev_hipriority_task, 0, HIPRIORITY_TASK_PRIO, 0x4000 );
TASK_CREATE( "livev_loprio_task", livev_lopriority_task, 0, 0x1f, 0x4000 );

// these may be out of order for config compatibility
void update_disp_mode_bits_from_params()
{
//~ BMP_LOCK(
    uint32_t bits =
        (global_draw & 1      ? 1<<0 : 0) |
        (zebra_draw           ? 1<<1 : 0) |
        (hist_draw            ? 1<<2 : 0) |
        (crop_enabled         ? 1<<3 : 0) |
        (waveform_draw        ? 1<<4 : 0) |
        (falsecolor_draw      ? 1<<5 : 0) |
        (spotmeter_draw       ? 1<<6 : 0) |
        (global_draw & 2      ? 1<<7 : 0) |
        (focus_peaking        ? 1<<8 : 0) |
        (zoom_overlay_enabled ? 1<<9 : 0) |
        (transparent_overlay  ? 1<<10: 0) |
        //~ (electronic_level     ? 1<<11: 0) |
        (defish_preview       ? 1<<12: 0) |
        (vectorscope_draw     ? 1<<13: 0) |
        0;
        
    if (disp_mode == 1) disp_mode_a = bits;
    else if (disp_mode == 2) disp_mode_b = bits;
    else if (disp_mode == 3) disp_mode_c = bits;
    else disp_mode_x = bits;
//~ )
}

void update_disp_mode_params_from_bits()
{
//~ BMP_LOCK(
    uint32_t bits = disp_mode == 1 ? disp_mode_a : 
                    disp_mode == 2 ? disp_mode_b :
                    disp_mode == 3 ? disp_mode_c : disp_mode_x;

    int global_draw_0    = bits & (1<<0) ? 1 : 0;
    zebra_draw           = bits & (1<<1) ? 1 : 0;
    hist_draw            = bits & (1<<2) ? 1 : 0;
    crop_enabled         = bits & (1<<3) ? 1 : 0;
    waveform_draw        = bits & (1<<4) ? 1 : 0;
    falsecolor_draw      = bits & (1<<5) ? 1 : 0;
    spotmeter_draw       = bits & (1<<6) ? 1 : 0;
    int global_draw_1    = bits & (1<<7) ? 1 : 0;
    focus_peaking        = bits & (1<<8) ? 1 : 0;
    zoom_overlay_enabled = bits & (1<<9) ? 1 : 0;
    transparent_overlay  = bits & (1<<10)? 1 : 0;
    //~ electronic_level     = bits & (1<<11)? 1 : 0;
    defish_preview       = bits & (1<<12)? 1 : 0;
    vectorscope_draw     = bits & (1<<13)? 1 : 0;
    global_draw = global_draw_0 + global_draw_1 * 2;
//~ end:
//~ )
}

int get_disp_mode() { return disp_mode; }

void toggle_disp_mode_menu(void *priv, int delta) {
	toggle_disp_mode();
}

int toggle_disp_mode()
{
    update_disp_mode_bits_from_params();
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
    
    ResumeLiveView();
    display_on();
    bmp_on();
    clrscr();
    idle_globaldraw_dis();
    //~ redraw();
    bmp_printf(SHADOW_FONT(FONT_LARGE), 50, 50, "Display preset: %d", disp_mode);
    msleep(250);
    idle_globaldraw_en();
    update_disp_mode_params_from_bits();
    redraw();
}

int handle_disp_preset_key(struct event * event)
{
    if (!disp_profiles_0) return 1;
    if (!lv) return 1;
    if (IS_FAKE(event)) return 1;
    if (gui_menu_shown()) return 1;
    if (event->param == BGMT_INFO)
    {
        toggle_disp_mode();
        return 0;
    }
    return 1;
}

int livev_playback = 0;

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
        #ifdef CONFIG_4_3_SCREEN
        clrscr(); // old cameras don't refresh the entire screen
        #endif
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


static void zebra_init()
{
    precompute_yuv2rgb();
#ifndef CONFIG_5DC
    menu_add( "Overlay", zebra_menus, COUNT(zebra_menus) );
#endif
    //~ menu_add( "Debug", livev_dbg_menus, COUNT(livev_dbg_menus) );
    //~ menu_add( "Movie", movie_menus, COUNT(movie_menus) );
    //~ menu_add( "Config", cfg_menus, COUNT(cfg_menus) );
    menu_add( "Prefs", powersave_menus, COUNT(powersave_menus) );
    menu_add( "Display", level_indic_menus, COUNT(level_indic_menus) );
}

INIT_FUNC(__FILE__, zebra_init);




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
    FILE* f = FIO_CreateFileEx(CARD_DRIVE "ML/DATA/overlay.dat");
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
    get_yuv422_vram();
    uint8_t * const bvram = bmp_vram_real();
    if (!bvram) return;
    
    clrscr();

    FILE* f = FIO_Open(CARD_DRIVE "ML/DATA/overlay.dat", O_RDONLY | O_SYNC);
    if (f == INVALID_PTR) return;
    FIO_ReadFile(f, bvram_mirror, 960*480 );
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
    ASSERT(src);
    ASSERT(dst);
    if (!dst) return;
    int i,j;
    for (i = BMP_H_MINUS; i < BMP_H_PLUS; i++)
    {
        for (j = BMP_W_MINUS; j < BMP_W_PLUS; j++)
        {
            int is = (i - y0) * deny / 128 + y0;
            int js = (j - x0) * denx / 128 + x0;
            dst[BM(j,i)] = (is >= 0 && js >= 0 && is < 480 && js < 720) // this is only used for menu
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

//~ CONFIG_STR("defish.lut", defish_lut_file, CARD_DRIVE "ML/SETTINGS/recti.lut");
#if defined(CONFIG_5D2) || defined(CONFIG_5D3) || defined(CONFIG_5DC) // fullframe
#define defish_lut_file_rectilin CARD_DRIVE "ML/DATA/ff8r.lut"
#define defish_lut_file_panini CARD_DRIVE "ML/DATA/ff8p.lut"
#else
#define defish_lut_file_rectilin CARD_DRIVE "ML/DATA/apsc8r.lut"
#define defish_lut_file_panini CARD_DRIVE "ML/DATA/apsc8p.lut"
#endif

static uint8_t* defish_lut = INVALID_PTR;
static int defish_projection_loaded = -1;

static void defish_lut_load()
{
    char* defish_lut_file = defish_projection ? defish_lut_file_panini : defish_lut_file_rectilin;
    if ((int)defish_projection != defish_projection_loaded)
    {
        if (defish_lut && defish_lut != INVALID_PTR) free_dma_memory(defish_lut);
        
        int size = 0;
        defish_lut = (uint8_t*) read_entire_file(defish_lut_file, &size);
        defish_projection_loaded = defish_projection;
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
                if ((*mp & 0x80808080)) continue;
                int c = (lv_pixel * 41 >> 8) + 38;
                c = c | (c << 8);
                c = c | (c << 16);
                *bp = *mp = *(bp + BMPPITCH/4) = *(mp + BMPPITCH/4) = c;
            }
        }
    }
}


static uint32_t get_yuv_pixel(uint32_t* buf, int pixoff)
{
    uint32_t* src = &buf[pixoff / 2];
    
    uint32_t chroma = (*src)  & 0x00FF00FF;
    uint32_t luma1 = (*src >>  8) & 0xFF;
    uint32_t luma2 = (*src >> 24) & 0xFF;
    uint32_t luma = pixoff % 2 ? luma2 : luma1;
    return (chroma | (luma << 8) | (luma << 24));
}

/* some sort of bilinear interpolation, doesn't seem to be correct
 * also too slow for real use
static uint32_t get_yuv_pixel_averaged(uint32_t* buf, float i, float j)
{
    int ilo = (int)floorf(i); int ihi = (int)ceilf(i);
    int jlo = (int)floorf(j); int jhi = (int)ceilf(j);
    
    float k1 = i - ilo;
    float k2 = j - jlo;
    float w1 = (1-k1) * (1-k2);
    float w2 = k1 * k2;
    float w3 = k1 * (1-k2);
    float w4 = (1-k1) * k2;
    
    uint32_t ll = get_yuv_pixel(buf, LV(jlo,ilo)/2);
    uint32_t hh = get_yuv_pixel(buf, LV(jhi,ihi)/2);
    uint32_t hl = get_yuv_pixel(buf, LV(jhi,ilo)/2);
    uint32_t lh = get_yuv_pixel(buf, LV(jlo,ihi)/2);
    
    uint32_t luma1 = (((ll >>  8) & 0xFF) * w1 + ((hh >>  8) & 0xFF) * w2 + ((hl >>  8) & 0xFF) * w3 + ((lh >>  8) & 0xFF) * w4) / (w1+w2+w3+w4);
    uint32_t luma2 = (((ll >> 24) & 0xFF) * w1 + ((hh >> 24) & 0xFF) * w2 + ((hl >> 24) & 0xFF) * w3 + ((lh >> 24) & 0xFF) * w4) / (w1+w2+w3+w4);

    uint32_t u = (int)((float)((int8_t)((ll >>  0) & 0xFF)) * w1 + (float)((int8_t)((hh >>  0) & 0xFF)) * w2 + (float)((int8_t)((hl >>  0) & 0xFF)) * w3 + ((float)((int8_t)((lh >>  0) & 0xFF)) * w4) / (w1+w2+w3+w4));
    uint32_t v = (int)((float)((int8_t)((ll >> 16) & 0xFF)) * w1 + (float)((int8_t)((hh >> 16) & 0xFF)) * w2 + (float)((int8_t)((hl >> 16) & 0xFF)) * w3 + ((float)((int8_t)((lh >> 16) & 0xFF)) * w4) / (w1+w2+w3+w4));
    
    return (u & 0xFF) | ((v & 0xFF) << 16) | ((luma1 & 0xFF) << 8) | ((luma2 & 0xFF) << 24);
}
*/
int defish_get_averaged_coord(uint8_t* lut, int i, int j, int num, int den)
{
    int acc = 0;
    for (int di = -2; di <= 2; di++)
    {
        for (int dj = -2; dj <= 2; dj++)
        {
            int newi = COERCE(i+di, 0, 239);
            int newj = COERCE(j+dj, 0, 359);
            acc += lut[(newi * 360 + newj) * 2];
        }
    }
    return acc * num / 25 / den;
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
    
    if (!PLAY_OR_QR_MODE || !DISPLAY_IS_ON) return;

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

            //~ int id = defish_lut[(i * 360 + j) * 2 + 1];
            //~ int jd = defish_lut[(i * 360 + j) * 2] * 360 / 255;
            
            // this reduces the quantization error from the LUT 
            int id = defish_get_averaged_coord(defish_lut + 1, i, j, 1, 1);
            int jd = defish_get_averaged_coord(defish_lut, i, j, 360, 255);
            
            int k;
            for (k = 0; k < 4; k++)
            {
                int Y = (off_i[k] ? N2LV_Y(off_i[k]) - y + BM2LV_Y(os.y0) - 1 : y);
                int X = (off_j[k] ? N2LV_X(off_j[k]) - x + BM2LV_X(os.x0) : x);
                int Id = (off_i[k] ? off_i[k] - id : id);
                int Jd = (off_j[k] ? off_j[k] - jd : jd);
                
                //~ lvram[LV(X,Y)/4] = aux_buf[N2LV(Jd,Id)/4];

                // Rather than copying an entire uyvy pair, copy only one pixel (and overwrite luma for both pixels in the bin)
                // => slightly better image quality
                
                // Actually, IQ is far lower than what Nona does with proper interpolation
                // but this is enough for preview purposes
                
                
                //~ uint32_t new_color = get_yuv_pixel_averaged(aux_buf, Id, Jd);

                int pixoff_src = N2LV(Jd,Id) / 2;
                uint32_t new_color = get_yuv_pixel(aux_buf, pixoff_src);

                int pixoff_dst = LV(X,Y) / 2;
                uint32_t* dst = &lvram[pixoff_dst / 2];
                uint32_t mask = (pixoff_dst % 2 ? 0xffFF00FF : 0x00FFffFF);
                *(dst) = (new_color & mask) | (*(dst) & ~mask);
            }
        }
        if (!PLAY_OR_QR_MODE || !DISPLAY_IS_ON) return;
        if ((void*)get_yuv422_vram()->vram != (void*)lvram) break; // user moved to a new image?
    }
}

PROP_HANDLER(PROP_LV_ACTION)
{
    zoom_overlay_triggered_by_focus_ring_countdown = 0;
    idle_display_undim(); // restore LCD brightness, especially for shutdown
    //~ idle_wakeup_reset_counters(-4);
    idle_globaldraw_disable = 0;
    if (buf[0] == 0) lv_paused = 0;
    bv_auto_update();
    zoom_sharpen_step();
    zoom_auto_exposure_step();
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
    // auto-generated code from 422-jpg.py
         if (size == 1120 *  746 * 2) { w = 1120; h =  746; } 
    else if (size == 1872 * 1080 * 2) { w = 1872; h = 1080; } 
    else if (size == 1024 *  680 * 2) { w = 1024; h =  680; } 
    else if (size == 1560 *  884 * 2) { w = 1560; h =  884; } 
    else if (size ==  944 *  632 * 2) { w =  944; h =  632; } 
    else if (size ==  928 *  616 * 2) { w =  928; h =  616; } 
    else if (size == 1576 * 1048 * 2) { w = 1576; h = 1048; } 
    else if (size == 1576 *  632 * 2) { w = 1576; h =  632; } 
    else if (size ==  720 *  480 * 2) { w =  720; h =  480; } 
    else if (size == 1056 *  704 * 2) { w = 1056; h =  704; } 
    else if (size == 1720 *  974 * 2) { w = 1720; h =  974; } 
    else if (size == 1280 *  580 * 2) { w = 1280; h =  580; } 
    else if (size ==  640 *  480 * 2) { w =  640; h =  480; } 
    else if (size == 1024 *  680 * 2) { w = 1024; h =  680; } 
    else if (size == 1056 *  756 * 2) { w = 1056; h =  756; } 
    else if (size == 1728 *  972 * 2) { w = 1728; h =  972; } 
    else if (size == 1680 *  945 * 2) { w = 1680; h =  945; } 
    else if (size == 1280 *  560 * 2) { w = 1280; h =  560; } 
    else if (size == 1152 *  768 * 2) { w = 1152; h =  768; } 
    else if (size == 1904 * 1270 * 2) { w = 1904; h = 1270; } 
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

void peaking_benchmark()
{
    fake_simple_button(BGMT_PLAY);
    msleep(1000);
    int a = get_seconds_clock();
    for (int i = 0; i < 1000; i++)
    {
        draw_zebra_and_focus(0,1);
    }
    int b = get_seconds_clock();
    NotifyBox(10000, "%d ", b-a);
    beep();
}
