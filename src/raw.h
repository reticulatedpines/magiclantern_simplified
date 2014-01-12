/** 
 * For decoding 14-bit RAW
 * 
 **/

/*
 * Copyright (C) 2013 Magic Lantern Team
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

/**
* RAW pixels (document mode, as with dcraw -D -o 0):

    01 23 45 67 89 AB ... (raw_info.width-1)
    ab cd ef gh ab cd ...

    v-------------------------- first pixel should be red
0   RG RG RG RG RG RG ...   <-- first line (even)
1   GB GB GB GB GB GB ...   <-- second line (odd)
2   RG RG RG RG RG RG ...
3   GB GB GB GB GB GB ...
...
(raw_info.height-1)
*/

/**
* 14-bit encoding:

hi          lo
aaaaaaaaaaaaaabb
bbbbbbbbbbbbcccc
ccccccccccdddddd
ddddddddeeeeeeee
eeeeeeffffffffff
ffffgggggggggggg
gghhhhhhhhhhhhhh
*/

#ifndef _raw_h_
#define _raw_h_

/* group 8 pixels in 14 bytes to simplify decoding */
struct raw_pixblock
{
    unsigned int b_hi: 2;
    unsigned int a: 14;     // even lines: red; odd lines: green
    unsigned int c_hi: 4;
    unsigned int b_lo: 12;
    unsigned int d_hi: 6;
    unsigned int c_lo: 10;
    unsigned int e_hi: 8;
    unsigned int d_lo: 8;
    unsigned int f_hi: 10;
    unsigned int e_lo: 6;
    unsigned int g_hi: 12;
    unsigned int f_lo: 4;
    unsigned int h: 14;     // even lines: green; odd lines: blue
    unsigned int g_lo: 2;
} __attribute__((packed));

/* call this before performing any raw image analysis */
/* returns 1=success, 0=failed */
int raw_update_params();

/* get a red/green/blue pixel near the specified coords (approximate) */
int raw_red_pixel(int x, int y);
int raw_green_pixel(int x, int y);
int raw_blue_pixel(int x, int y);

/* get/set the pixel at specified coords (exact, but you can get whatever color happens to be there) */
int raw_get_pixel(int x, int y);
int raw_set_pixel(int x, int y, int value);

/* get a grayscale pixel according to some projection from RGB */
int raw_get_gray_pixel(int x, int y, int gray_projection);
#define GRAY_PROJECTION_RED 0
#define GRAY_PROJECTION_GREEN 1
#define GRAY_PROJECTION_BLUE 2
#define GRAY_PROJECTION_AVERAGE_RGB 3
#define GRAY_PROJECTION_MAX_RGB 4
#define GRAY_PROJECTION_MAX_RB 5
#define GRAY_PROJECTION_MEDIAN_RGB 6

/* for dual ISO: get pixel from a specific exposure, bright or dark (autodetected on the fly) */
int raw_red_pixel_dark(int x, int y);
int raw_green_pixel_dark(int x, int y);
int raw_blue_pixel_dark(int x, int y);
int raw_red_pixel_bright(int x, int y);
int raw_green_pixel_bright(int x, int y);
int raw_blue_pixel_bright(int x, int y);

#define GRAY_PROJECTION_BRIGHT_DARK_MASK 0x300
#define GRAY_PROJECTION_DARK_ONLY        0x000 /* by default, analyze the dark exposure only (suitable for highlights, ETTR...) */
#define GRAY_PROJECTION_BRIGHT_ONLY      0x100 /* you can also analyze the bright exposure (suitable for shadows, SNR... */
#define GRAY_PROJECTION_DARK_AND_BRIGHT  0x200 /* warning: might be more accurate on regular images, but has undefined behavior on dual ISO images */

/* input: 0 - 16384 (valid range: from black level to white level) */
/* output: -14 ... 0 */
float raw_to_ev(int raw);
int ev_to_raw(float ev);

/* quick preview of the raw buffer */
void raw_preview_fast();

/* pass -1 if default value for some parameter is fine */
void raw_preview_fast_ex(void* raw_buffer, void* lv_buffer, int start_line, int end_line, int quality);
#define RAW_PREVIEW_COLOR_HALFRES   0   /* 360x480 color, pretty slow */
#define RAW_PREVIEW_GRAY_ULTRA_FAST 1   /* 180x240, aims to be real-time */

/* request/release/check LiveView RAW flag (lv_save_raw) */
/* you have to call request/release in pairs (be careful not to request once and release twice) */
void raw_lv_request();
void raw_lv_release();
int raw_lv_enabled();

/* redirect the LV RAW EDMAC in order to write the raw data at "ptr" */
void raw_lv_redirect_edmac(void* ptr);

/* cut the right part of the LV raw image (makes buffer smaller); may reduce DMA load */
/* returns the value actually used (or 0 if it doesn't work) */
int raw_lv_shave_right(int offset);

/* quick check whether the settings from raw_info are still valid (for lv vsync calls) */
int raw_lv_settings_still_valid();

void raw_set_geometry(int width, int height, int skip_left, int skip_right, int skip_top, int skip_bottom);
void raw_force_aspect_ratio_1to1();
void raw_set_preview_rect(int x, int y, int w, int h);

/* call this after you have altered the preview settings, and you want to restore the original ones */
void raw_set_dirty();

/* for x5 crop mode: get the offset (in pixels) between raw and yuv frames. Return: 1=OK, 0=failed. */
int focus_box_get_raw_crop_offset(int* delta_x, int* delta_y); /* this is in shoot.c */

/* called from state-object.c */
void raw_lv_vsync_cbr();

/* units: EV x100 */
int get_dxo_dynamic_range(int raw_iso);

/* raw image info (geometry, calibration levels, color, DR etc); parts of this were copied from CHDK */
struct raw_info {
    uint32_t api_version;           // increase this when changing the structure
    #if INTPTR_MAX == INT32_MAX     // only works on 32-bit systems
    void* buffer;                   // points to image data
    #else
    uint32_t do_not_use_this;       // this can't work on 64-bit systems
    #endif
    
    int32_t height, width, pitch;
    int32_t frame_size;
    int32_t bits_per_pixel;         // 14

    int32_t black_level;            // autodetected
    int32_t white_level;            // somewhere around 13000 - 16000, varies with camera, settings etc
                                    // would be best to autodetect it, but we can't do this reliably yet
    union                           // DNG JPEG info
    {
        struct
        {
            int32_t x, y;           // DNG JPEG top left corner
            int32_t width, height;  // DNG JPEG size
        } jpeg;
        struct
        {
            int32_t origin[2];
            int32_t size[2];
        } crop;
    };
    union                       // DNG active sensor area (Y1, X1, Y2, X2)
    {
        struct
        {
            int32_t y1, x1, y2, x2;
        } active_area;
        int32_t dng_active_area[4];
    };
    int32_t exposure_bias[2];       // DNG Exposure Bias (idk what's that)
    int32_t cfa_pattern;            // stick to 0x02010100 (RGBG) if you can
    int32_t calibration_illuminant1;
    int32_t color_matrix1[18];      // DNG Color Matrix
    int32_t dynamic_range;          // EV x100, from analyzing black level and noise (very close to DxO)
};

extern struct raw_info raw_info;

/* save a DNG file; all parameters are taken from raw_info */
int save_dng(char* filename, struct raw_info * raw_info);

/* do not include ML headers if used in postprocessing */
#ifdef CONFIG_MAGICLANTERN
/** Menu helpers **/
#include "menu.h"

/* use this if you want to print a warning if you need to shoot raw in order to use your feature (photo or video) */
extern MENU_UPDATE_FUNC(menu_checkdep_raw);

/* this prints the raw warning without checking */
extern MENU_UPDATE_FUNC(menu_print_warning_raw);

/* photo mode, non-LV: to know whether you will have access to raw data */
extern int can_use_raw_overlays_photo();

/* to be used in code using overlays directly (e.g. right before drawing zebras, to decide if they are raw or yuv) */
extern int can_use_raw_overlays();

/* to be used in menu, if you want to check if raw data will available in current mode (not necessarily at the time of displaying the menu) */
extern int can_use_raw_overlays_menu();

#endif

#endif
