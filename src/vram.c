/** \file
 * Common functions for image buffers
 * http://magiclantern.wikia.com/wiki/VRAM
 */

#include "dryos.h"
#include "property.h"
#include "propvalues.h"
#include "bmp.h"

struct vram_info vram_lv = {
	.pitch = 720 * 2,
	.width = 720,
	.height = 480,
};

struct vram_info vram_hd = {
	.pitch = 1056 * 2,
	.width = 1056,
	.height = 704,
};

struct vram_info vram_bm = {
	.pitch = 960,
	.width = 720,
	.height = 480,
};

PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

// area from BMP where the LV image (3:2) is effectively drawn, without black bars
// in this area we'll draw cropmarks, zebras and so on
struct bmp_ov_loc_size os;

static void calc_ov_loc_size(struct bmp_ov_loc_size * os)
{
	if (hdmi_code == 2 || ext_monitor_rca)
	{
		os->x0 = 40;
		os->y0 = 24;
		os->x_ex = 640;
		os->y_ex = 388;
	}
	else if (hdmi_code == 5)
	{
		os->x0 = (1920-1620) / 4;
		os->y0 = 0;
		os->x_ex = 540 * 3/2;
		os->y_ex = 540;
	}
	else
	{
		os->x0 = 0;
		os->y0 = 0;
		os->x_ex = 720;
#if defined(CONFIG_50D) || defined(CONFIG_500D)
		os->y_ex = 480 * 8/9; // BMP is 4:3, image is 3:2;
#else
		os->y_ex = 480;
#endif
	}
	os->x_max = os->x0 + os->x_ex;
	os->y_max = os->y0 + os->y_ex;
	os->off_169 = (os->y_ex - os->y_ex * 3/2*9/16) / 2;
}


// these buffer sizes include any black bars
void update_vram_params()
{
	// BMP (used for overlays)
	vram_bm.width  = hdmi_code == 5 ? 960 : 720;
	vram_bm.height = hdmi_code == 5 ? 960 : 480;
	vram_bm.pitch = 960;
	
	// LV buffer (used for display)
#if defined(CONFIG_50D) || defined(CONFIG_500D)
	vram_lv.width  = hdmi_code == 5 ? 1920 : ext_monitor_rca ? 512 : 720 * 8/9;
	vram_lv.height = hdmi_code == 5 ? 1080 : ext_monitor_rca ? 512 : 480 * 8/9;
#endif
#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D)
	vram_lv.width  = hdmi_code == 5 ? 1920 : ext_monitor_rca ? 512 : 720;
	vram_lv.height = hdmi_code == 5 ? 1080 : ext_monitor_rca ? 512 : 480;
#endif
#ifdef CONFIG_1100D
	vram_lv.width  = 720;
	vram_lv.height = 240;
#endif
	vram_lv.pitch = vram_lv.width * 2; 

	// HD buffer (used for recording)
#ifdef CONFIG_50D
	vram_hd.width = recording ? 1560 : 1024;
	vram_hd.height = recording ? 1048 : 680;
#endif
#ifdef CONFIG_500D
	vram_hd.width  = lv_dispsize > 1 ?  944 : !is_movie_mode() ?  928 : recording ? (video_mode_resolution == 0 ? 1576 : video_mode_resolution == 1 ? 1576 : video_mode_resolution == 2 ? 720 : 0) : /*not recording*/ (video_mode_resolution == 0 ? 1576 : video_mode_resolution == 1 ? 928 : video_mode_resolution == 2 ? 928 : 0);
	vram_hd.height = lv_dispsize > 1 ?  632 : !is_movie_mode() ?  616 : recording ? (video_mode_resolution == 0 ? 1048 : video_mode_resolution == 1 ?  632 : video_mode_resolution == 2 ? 480 : 0) : /*not recording*/ (video_mode_resolution == 0 ? 1048 : video_mode_resolution == 1 ? 616 : video_mode_resolution == 2 ? 616 : 0);
#endif
#if defined(CONFIG_550D) || defined(CONFIG_60D)
	vram_hd.width  = lv_dispsize > 1 ? 1024 : !is_movie_mode() ? 1056 : recording ? (video_mode_resolution == 0 ? 1720 : video_mode_resolution == 1 ? 1280 : video_mode_resolution == 2 ? 640 : 0) : /*not recording*/ (video_mode_resolution == 0 ? 1056 : video_mode_resolution == 1 ? 1024 : video_mode_resolution == 2 ? (video_mode_crop? 640:1024) : 0);
	vram_hd.height = lv_dispsize > 1 ?  680 : !is_movie_mode() ?  704 : recording ? (video_mode_resolution == 0 ?  974 : video_mode_resolution == 1 ?  580 : video_mode_resolution == 2 ? 480 : 0) : /*not recording*/ (video_mode_resolution == 0 ?  704 : video_mode_resolution == 1 ?  680 : video_mode_resolution == 2 ? (video_mode_crop? 480: 680) : 0);
#endif
#ifdef CONFIG_600D
	vram_hd.width  = lv_dispsize > 1 ? 1024 : !is_movie_mode() ? 1056 : (video_mode_resolution == 0 ? (digital_zoom_ratio >= 300 ? 1728 : 1680) : video_mode_resolution == 1 ? 1280 : video_mode_resolution == 2 ? (video_mode_crop? 640:1024) : 0);
	vram_hd.height = lv_dispsize > 1 ?  680 : !is_movie_mode() ?  704 : (video_mode_resolution == 0 ? (digital_zoom_ratio >= 300 ?  972 :  945) : video_mode_resolution == 1 ? 560  : video_mode_resolution == 2 ? (video_mode_crop? 480: 680) : 0);
#endif
#ifdef CONFIG_1100D // not tested, just copied from 600D
	vram_hd.width  = lv_dispsize > 1 ? 1024 : !is_movie_mode() ? 1056 : (video_mode_resolution == 0 ? (digital_zoom_ratio >= 300 ? 1728 : 1680) : video_mode_resolution == 1 ? 1280 : video_mode_resolution == 2 ? (video_mode_crop? 640:1024) : 0);
	vram_hd.height = lv_dispsize > 1 ?  680 : !is_movie_mode() ?  704 : (video_mode_resolution == 0 ? (digital_zoom_ratio >= 300 ?  972 :  945) : video_mode_resolution == 1 ? 560  : video_mode_resolution == 2 ? (video_mode_crop? 480: 680) : 0);
#endif
	vram_hd.pitch = vram_hd.width * 2;
	
	calc_ov_loc_size(&os);
}

struct trans2d bm2lv = { 
	.tx = 0,
	.ty = 0,
	.sx = 1024,
	.sy = 1024,
};

struct trans2d lv2hd = { 
	.tx = 0,
	.ty = 0,
	.sx = 2048, // dummy
	.sy = 2048, // dummy
};

/*
int* lut_bm2lv_x = 0;
int* lut_lv2bm_x = 0;

int* lut_lv2hd_x = 0;
int* lut_hd2lv_x = 0;

int* lut_bm2hd_x = 0;
int* lut_hd2bm_x = 0;

int* lut_bm2lv_y = 0;
int* lut_lv2bm_y = 0;

int* lut_lv2hd_y = 0;
int* lut_hd2lv_y = 0;

int* lut_bm2hd_y = 0;
int* lut_hd2bm_y = 0;

void lut_realloc(int** buf, int size)
{
	if (*buf) FreeMemory(*buf);
	*buf = AllocateMemory(size);
}

void lut_init()
{
	lut_realloc(&lut_bm2lv_x, SL.BM.W);
	lut_realloc(&lut_bm2lv_y, SL.BM.H);

	lut_realloc(&lut_lv2bm_x, SL.LV.W);
	lut_realloc(&lut_lv2bm_y, SL.LV.H);

	lut_realloc(&lut_lv2bm);
}*/

#include "bmp.h"

void* get_lcd_422_buf()
{
	switch (YUV422_LV_BUFFER_DMA_ADDR)
	{
		case YUV422_LV_BUFFER_1:
			return (void*)YUV422_LV_BUFFER_1;
		case YUV422_LV_BUFFER_2:
			return (void*)YUV422_LV_BUFFER_2;
		case YUV422_LV_BUFFER_3:
			return (void*)YUV422_LV_BUFFER_3;
	}
	return (void*)YUV422_LV_BUFFER_1; // fall back to default
}

static int fastrefresh_direction = 0;

void guess_fastrefresh_direction() {
	static unsigned old_pos = YUV422_LV_BUFFER_1;
	if (old_pos == YUV422_LV_BUFFER_DMA_ADDR) return;
	if (old_pos == YUV422_LV_BUFFER_1 && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_2) fastrefresh_direction = 1;
	if (old_pos == YUV422_LV_BUFFER_1 && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_3) fastrefresh_direction = 0;
	old_pos = YUV422_LV_BUFFER_DMA_ADDR;
}


void* get_fastrefresh_422_buf()
{
	if (fastrefresh_direction) {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER_1:
				return (void*)YUV422_LV_BUFFER_2;
			case YUV422_LV_BUFFER_2:
				return (void*)YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_3:
				return (void*)YUV422_LV_BUFFER_1;
		}
		return (void*)YUV422_LV_BUFFER_1; // fall back to default
	} else {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER_1:
				return (void*)YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_2:
				return (void*)YUV422_LV_BUFFER_1;
			case YUV422_LV_BUFFER_3:
				return (void*)YUV422_LV_BUFFER_2;
		}
		return (void*)YUV422_LV_BUFFER_1; // fall back to default

	}
}


// YUV422_HD_BUFFER_DMA_ADDR returns many possible values, but usually cycles between last two
// This function returns the value which was used just before the current one
// That buffer is not updated by DMA (and should contain a silent picture without horizontal cut)
void* get_422_hd_idle_buf()
{
	static int idle_buf = 0;
	static int current_buf = 0;
	
	if (!idle_buf) idle_buf = current_buf = YUV422_HD_BUFFER_DMA_ADDR;

	int hd = YUV422_HD_BUFFER_DMA_ADDR;
	//~ bmp_printf(FONT_LARGE, 50, 200, "%x %x %x", hd, current_buf, IS_HD_BUFFER(hd));
	if (IS_HD_BUFFER(hd))
	{
		if (hd != current_buf)
		{
			idle_buf = current_buf;
			current_buf = hd;
		}
	}
	return (void*)idle_buf;
}


struct vram_info * get_yuv422_vram()
{
	extern int lv_paused;
	if (gui_state == GUISTATE_PLAYMENU || lv_paused)
		vram_lv.vram = get_lcd_422_buf();
	else
		vram_lv.vram = get_fastrefresh_422_buf();
	return &vram_lv;
}

struct vram_info * get_yuv422_hd_vram()
{
	vram_hd.vram = get_422_hd_idle_buf();
	return &vram_hd;
}


// those properties may signal a screen layout change

PROP_HANDLER(PROP_HDMI_CHANGE)
{
	update_vram_params();
	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_HDMI_CHANGE_CODE)
{
	update_vram_params();
	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_USBRCA_MONITOR)
{
	update_vram_params();
	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_LV_DISPSIZE)
{
	update_vram_params();
	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_MVR_REC_START)
{
	update_vram_params();
	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_SHOOTING_TYPE)
{
	update_vram_params();
	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_LV_ACTION)
{
	update_vram_params();
	return prop_cleanup(token, property);
}
