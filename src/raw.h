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
} __attribute__((packed,aligned(2)));

/* call this before performing any raw image analysis */
/* in LiveView, this will retry as needed */
/* returns 1=success, 0=failed */
int raw_update_params();

/* get a red/green/blue pixel near the specified coords (approximate) */
int raw_red_pixel(int x, int y);
int raw_green_pixel(int x, int y);
int raw_blue_pixel(int x, int y);

/* get/set the pixel at specified coords (exact, but you can get whatever color happens to be there) */
int raw_get_pixel(int x, int y);
void raw_set_pixel(int x, int y, int value);

/* get a pixel from a custom raw buffer (not from the main one) */
int raw_get_pixel_ex(void* raw_buffer, int x, int y);

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

// framed preview parameters:
#define FRAMED_PREVIEW_PARAM__ENGINE                0    // engine type to use to deal with framed preview
#define FRAMED_PREVIEW_PARAM__IDLE_STYLE            1    // style to apply when idle
#define FRAMED_PREVIEW_PARAM__IDLE_RESOLUTION       2    // resolution to apply when idle
#define FRAMED_PREVIEW_PARAM__RECORDING_STYLE       3    // style to apply when recording
#define FRAMED_PREVIEW_PARAM__RECORDING_RESOLUTION  4    // resolution to apply when recording
#define FRAMED_PREVIEW_PARAM__TIMING                5    // timing policy
#define FRAMED_PREVIEW_PARAM__STATISTICS            6    // statistics dump state

// framed preview engine values:
#define FRAMED_PREVIEW_PARAM__ENGINE__LEGACY        0    // "legacy" engine
#define FRAMED_PREVIEW_PARAM__ENGINE__ULTRAFAST     1    // ultrafast (cached) engine

// framed preview style values:
#define FRAMED_PREVIEW_PARAM__STYLE__COLORED        0    // colored display
#define FRAMED_PREVIEW_PARAM__STYLE__GRAYSCALED     1    // grayscaled display (faster)

// framed preview resolution values:
#define FRAMED_PREVIEW_PARAM__RESOLUTION_HALF       0    // half resolution (more accurate)
#define FRAMED_PREVIEW_PARAM__RESOLUTION_QUARTER    1    // quarter resolution (faster)

// framed preview timing values:
#define FRAMED_PREVIEW_PARAM__TIMING__LEGACY        0    // "legacy" timing (regular sleep statements)
#define FRAMED_PREVIEW_PARAM__TIMING__TEMPERED      1    // tempered timing policy, affecting idle only
#define FRAMED_PREVIEW_PARAM__TIMING__AGRESSIVE     2    // agressive timing, affecting also recording

// framed preview statistics values:
#define FRAMED_PREVIEW_PARAM__STATISTICS_OFF        0    // deactivated statistics console dump
#define FRAMED_PREVIEW_PARAM__STATISTICS_ON         1    // activated statistics console dump

// change the value of a given framed preview parameter:
void set_framed_preview_param( const int _param, const int _value );

// get the current value of a given framed preview parameter:
int get_framed_preview_param( const int _param );

/* input: 0 - 16384 (valid range: from black level to white level) */
/* output: -14 ... 0 */
float raw_to_ev(int raw);
int ev_to_raw(float ev);

/* quick preview of the raw buffer */
void raw_preview_fast();

/* pass -1 if default value for some parameter is fine */
void raw_preview_fast_ex( void * _p_raw_buffer, void * _p_lv_buffer, int _y1, int _y2, int _quality );

// possible quality values:
#define RAW_PREVIEW_COLOR_HALFRES   0    // 360x480 color, pretty slow
#define RAW_PREVIEW_GRAY_ULTRA_FAST 1    // 180x240, aims to be real-time
#define RAW_PREVIEW_ADAPTIVE        2    // choice depends on idle & recording framed preview settings

// updated framed preview drawing routine, requiring an additional recording state:
void raw_preview_fast_ex2( void * _p_raw_buffer, void * _p_lv_buffer, const int _y1, const int _y2, const int _quality, const bool _recording );

/* request/release/check LiveView RAW flag (lv_save_raw) */
/* you have to call request/release in pairs (be careful not to request once and release twice) */
void raw_lv_request();
void raw_lv_release();
void raw_lv_request_bpp(int bpp);
void raw_lv_request_digital_gain(int gain); /* 4096 = 1.0, 0 = disable */
int raw_lv_enabled();

/* redirect the LV RAW EDMAC in order to write the raw data at "ptr" */
void raw_lv_redirect_edmac(void* ptr);

/* cut the right part of the LV raw image (makes buffer smaller); may reduce DMA load */
/* returns the value actually used (or 0 if it doesn't work) */
int raw_lv_shave_right(int offset);

/* quick check whether the settings from raw_info are still valid (for lv vsync calls) */
int raw_lv_settings_still_valid();

void raw_set_geometry(int width, int height, int skip_left, int skip_right, int skip_top, int skip_bottom);
void raw_force_aspect_ratio(int factor_x, int factor_y);
void raw_set_preview_rect(int x, int y, int w, int h, int obey_info_bars);

/* call this after you have altered the preview settings, and you want to restore the original ones */
void raw_set_dirty(void);

/* for x5 crop mode: get the offset (in pixels) between raw and yuv frames. Return: 1=OK, 0=failed. */
int focus_box_get_raw_crop_offset(int* delta_x, int* delta_y); /* this is in shoot.c */

/* called from state-object.c */
void raw_lv_vsync();

/* called from lv-img-engio.c */
int _raw_lv_get_iso_post_gain();

/* units: EV x100 */
int get_dxo_dynamic_range(int raw_iso);

/* raw image info (geometry, calibration levels, color, DR etc); parts of this were copied from CHDK */
struct raw_info {
    uint32_t api_version;           // increase this when changing the structure
    void* buffer;                   // points to image data
    
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

/* raw image info "raw_info_t" used for file IO on host pc tools */
#if INTPTR_MAX != INT32_MAX
typedef struct {
    uint32_t api_version;           // increase this when changing the structure
    uint32_t do_not_use_this;       // was the memory buffer, this can't work on 64-bit systems
    
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
} raw_info_t;
#else
typedef struct raw_info raw_info_t;
#endif

extern struct raw_info raw_info;

static inline void raw_info_to_camera(raw_info_t *dst, struct raw_info *src)
{
    dst->api_version = src->api_version;
    dst->height = src->height;
    dst->width = src->width;
    dst->pitch = src->pitch;
    dst->bits_per_pixel = src->bits_per_pixel;
    dst->black_level = src->black_level;
    dst->white_level = src->white_level;
    dst->jpeg.x = src->jpeg.x;
    dst->jpeg.y = src->jpeg.y;
    dst->jpeg.width = src->jpeg.width;
    dst->jpeg.height = src->jpeg.height;
    dst->exposure_bias[0] = src->exposure_bias[0];
    dst->exposure_bias[1] = src->exposure_bias[1];
    dst->cfa_pattern = src->cfa_pattern;
    dst->calibration_illuminant1 = src->calibration_illuminant1;
    memcpy(dst->color_matrix1, src->color_matrix1, sizeof(dst->color_matrix1));
    memcpy(dst->dng_active_area, src->dng_active_area, sizeof(dst->dng_active_area));
    dst->dynamic_range = src->dynamic_range;
}

static inline void raw_info_from_camera(struct raw_info *dst, raw_info_t *src)
{
    dst->api_version = src->api_version;
    dst->height = src->height;
    dst->width = src->width;
    dst->pitch = src->pitch;
    dst->bits_per_pixel = src->bits_per_pixel;
    dst->black_level = src->black_level;
    dst->white_level = src->white_level;
    dst->jpeg.x = src->jpeg.x;
    dst->jpeg.y = src->jpeg.y;
    dst->jpeg.width = src->jpeg.width;
    dst->jpeg.height = src->jpeg.height;
    dst->exposure_bias[0] = src->exposure_bias[0];
    dst->exposure_bias[1] = src->exposure_bias[1];
    dst->cfa_pattern = src->cfa_pattern;
    dst->calibration_illuminant1 = src->calibration_illuminant1;
    memcpy(dst->color_matrix1, src->color_matrix1, sizeof(dst->color_matrix1));
    memcpy(dst->dng_active_area, src->dng_active_area, sizeof(dst->dng_active_area));
    dst->dynamic_range = src->dynamic_range;
}

/* image capture parameters */
struct raw_capture_info {
    /* sensor attributes: resolution, crop factor */
    uint16_t sensor_res_x;  /* sensor resolution */
    uint16_t sensor_res_y;  /* 2-3 GPixel cameras anytime soon? (to overflow this) */
    uint16_t sensor_crop;   /* sensor crop factor x100 */
    uint16_t reserved;      /* reserved for future use */

    /* video mode attributes */
    /* (how the sensor is configured for image capture) */
    /* subsampling factor: (binning_x+skipping_x) x (binning_y+skipping_y) */
    uint8_t  binning_x;     /* 3 (1080p and 720p); 1 (crop, zoom) */
    uint8_t  skipping_x;    /* so far, 0 everywhere */
    uint8_t  binning_y;     /* 1 (most cameras in 1080/720p; also all crop modes); 3 (5D3 1080p); 5 (5D3 720p) */
    uint8_t  skipping_y;    /* 2 (most cameras in 1080p); 4 (most cameras in 720p); 0 (5D3) */
    int16_t  offset_x;      /* crop offset (top-left active pixel) - optional (SHRT_MIN if unknown) */
    int16_t  offset_y;      /* relative to top-left active pixel from a full-res image (FRSP or CR2) */
   
    /* The captured *active* area (raw_info.active_area) will be mapped
     * on a full-res image (which does not use subsampling) as follows:
     *   active_width  = raw_info.active_area.x2 - raw_info.active_area.x1
     *   active_height = raw_info.active_area.y2 - raw_info.active_area.y1
     *   .x1 (left)  : offset_x + full_res.active_area.x1
     *   .y1 (top)   : offset_y + full_res.active_area.y1
     *   .x2 (right) : offset_x + active_width  * (binning_x+skipping_x) + full_res.active_area.x1
     *   .y2 (bottom): offset_y + active_height * (binning_y+skipping_y) + full_res.active_area.y1
     */
};

extern struct raw_capture_info raw_capture_info;

/* save a DNG file; all parameters are taken from raw_info */
int save_dng(char* filename, struct raw_info * raw_info);

/* do not include ML headers if used in postprocessing */
#ifdef CONFIG_MAGICLANTERN
/** Menu helpers **/
#include "menu.h"

/* use this if you want to print a warning if you need to shoot raw in order to use your feature (photo or video) */
extern MENU_UPDATE_FUNC(menu_checkdep_raw);

/* this prints the raw warning without checking */
extern MENU_UPDATE_FUNC(menu_set_warning_raw);

/* photo mode, non-LV: to know whether you will have access to raw data */
extern int can_use_raw_overlays_photo();

/* to be used in code using overlays directly (e.g. right before drawing zebras, to decide if they are raw or yuv) */
extern int can_use_raw_overlays();

/* to be used in menu, if you want to check if raw data will available in current mode (not necessarily at the time of displaying the menu) */
extern int can_use_raw_overlays_menu();

#endif

#if defined(CONFIG_RAW_LIVEVIEW) || defined(MODULE)
/* returns true if LiveView is currently in RAW mode */
/* for movie mode, this only happens if some sort of raw recorder is active */
/* for photo mode, it should happen when some raw overlays are active */
extern int raw_lv_is_enabled();
#else
/* with this macro, the compiler will optimize out the code blocks that depend on LiveView raw support */
/* (no need to sprinkle the code with #ifdef CONFIG_RAW_LIVEVIEW) */
/* Q: any way to make this cleaner? (with weak func, the compiler no longer optimizes these things) */
#define raw_lv_is_enabled() 0 
#endif

#endif
