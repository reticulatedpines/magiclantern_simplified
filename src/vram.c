/** \file
 * Common functions for image buffers
 * http://magiclantern.wikia.com/wiki/VRAM
 */

#include "dryos.h"
#include "property.h"
#include "propvalues.h"
#include "bmp.h"
#include "menu.h"

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

// area from BMP where the LV image (3:2) is effectively drawn, without black bars
// in this area we'll draw cropmarks, zebras and so on
struct bmp_ov_loc_size os = {
	.x0 = 0,
	.y0 = 0,
	.x_ex = 480,
	.y_ex = 720,
};

// LV aspect ratio is 3:2 (so far)
// HD aspect ratio may vary (3:2, 16:9, 4:3)
#define lv_ratio_num 3
#define lv_ratio_den 2
int hd_ratio_num = 3;
int hd_ratio_den = 2;

int increment = 4;

int* vram_params[] = { 
	&increment,
	&vram_bm.width, &vram_bm.height, 
	&os.x0, &os.y0, &os.x_ex, &os.y_ex, 
	&vram_lv.width, &vram_lv.height, 
	&bm2lv.tx, &bm2lv.ty, &bm2lv.sx, &bm2lv.sy,
	&vram_hd.width, &vram_hd.height, 
	&hd_ratio_num, &hd_ratio_den ,
	&lv2hd.tx, &lv2hd.ty, &lv2hd.sx, &lv2hd.sy,
};
char vram_param_names[][12] = {
	"increment ",
	"bmp.width ", "bmp.height",
	"os.x_left ", "os.y_top  ",
	"os.x_ex   ", "os.y_ex   ",
	"lv.width  ", "lv.height ",
	"bm2lv.tx  ", "bm2lv.ty  ",
	"bm2lv.sx  ", "bm2lv.sy  ",
	"hd.width  ", "hd.height ",
	"ratio_num ", "ratio_den ",
	"lv2hd.tx* ", "lv2hd.ty* ",
	"lv2hd.sx* ", "lv2hd.sy* ",
};

PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

PROP_INT(PROP_VIDEO_SYSTEM, pal);

// these buffer sizes include any black bars
void update_vram_params()
{
	// BMP (used for overlays)
	vram_bm.width  = hdmi_code == 5 ? 960 : 720;
	vram_bm.height = hdmi_code == 5 ? 540 : 480;
	vram_bm.pitch = 960;
	
	// LV crop area
	os.x0   = hdmi_code == 5 ?  75 : hdmi_code == 2 ? 40 : ext_monitor_rca ? 32 :    0;
	os.y0   = hdmi_code == 5 ?   0 : hdmi_code == 2 ? 40 : ext_monitor_rca ? 28 :    0;
	os.x_ex = hdmi_code == 5 ? 810 : (hdmi_code == 2 || ext_monitor_rca) ? 640 : 720;
	os.y_ex = hdmi_code == 5 ? 540 : (hdmi_code == 2 || ext_monitor_rca) ? 388 : 480;
#if defined(CONFIG_50D) || defined(CONFIG_500D)
	os.y_ex = 480 * 8/9; // BMP is 4:3, image is 3:2;
#endif

	// LV buffer (used for display)
#if defined(CONFIG_50D) || defined(CONFIG_500D)
	vram_lv.width  = hdmi_code == 5 ? 1920 : ext_monitor_rca ? 512 : 720;
	vram_lv.height = hdmi_code == 5 ? 1080 : ext_monitor_rca ? 512 : 480 * 8/9;
#endif
#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_600D)
	vram_lv.width  = hdmi_code == 5 ? 1920 : ext_monitor_rca ? 540 : 720;
	vram_lv.height = hdmi_code == 5 ? 1080 : ext_monitor_rca ? (pal ? 572 : 480) : 480;
#endif
#ifdef CONFIG_1100D
	vram_lv.width  = 720;
	vram_lv.height = 240;
#endif


	// bmp to lv transformation
	bm2lv.tx = ext_monitor_rca ? 4 : 0;
	bm2lv.ty = 0;
	bm2lv.sx = hdmi_code == 5 ? 2048 : ext_monitor_rca ? 768 : 1024;
	bm2lv.sy = 1024 * vram_lv.height / vram_bm.height; // no black bars at top or bottom

	// HD buffer (used for recording)
	hd_ratio_num = recording ? (video_mode_resolution < 2 ? 16 : 4) : 3;
	hd_ratio_den = recording ? (video_mode_resolution < 2 ?  9 : 3) : 2;

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

	update_vram_params_calc();
}

void update_vram_params_calc()
{
	// those params are dependent on others
	// they can be computed (these formulas should be generic, not camera-specific)
	os.x_max = os.x0 + os.x_ex;
	os.y_max = os.y0 + os.y_ex;
	os.off_169 = (os.y_ex - os.y_ex * 3/2*9/16) / 2;

	vram_lv.pitch = vram_lv.width * 2; 
	vram_hd.pitch = vram_hd.width * 2;
	
	// here we assume HD and LV are always centered, but their aspect ratios (and sizes) may differ
	if (lv_ratio_num * hd_ratio_den > lv_ratio_den * hd_ratio_num) // crop sides
	{
		lv2hd.tx = - (vram_hd.width * lv_ratio_num / lv_ratio_den * hd_ratio_den / hd_ratio_num - vram_hd.width) / 2;
		lv2hd.ty = 0;
		lv2hd.sx = 1024 * vram_hd.width / vram_lv.width * lv_ratio_num / lv_ratio_den * hd_ratio_den / hd_ratio_num;
		lv2hd.sy = 1024 * vram_hd.height / vram_lv.height;
	}
	else // crop top and bottom
	{
		lv2hd.tx = 0;
		lv2hd.ty = - (vram_hd.height * lv_ratio_den / lv_ratio_num * hd_ratio_num / hd_ratio_den - vram_hd.height) / 2;
		lv2hd.sx = 1024 * vram_hd.width / vram_lv.width;
		lv2hd.sy = 1024 * vram_hd.height / vram_lv.height * lv_ratio_den / lv_ratio_num * hd_ratio_num / hd_ratio_den;
	}
}

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

static void
vram_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	unsigned		menu_font = selected ? MENU_FONT_SEL : MENU_FONT;
	unsigned		font = FONT(FONT_MED, FONT_FG(menu_font), FONT_BG(menu_font));
	unsigned		height = fontspec_height( font );

	y = y * 2/3 + 20;
	if (y > 400) { y = y - 400 + 50; x = 360; }
	bmp_printf(
		font,
		x, y,
		"%s = %d",
		vram_param_names[(int)priv], MEM(vram_params[(int)priv])
	);
	menu_draw_icon(x,y,MNI_NONE,0);
}

static void vram_toggle(void* priv, int delta)
{
	MEM(vram_params[(int)priv]) += priv ? delta : SGN(delta);
	menu_show_only_selected();
	crop_set_dirty(1);
	update_vram_params_calc();
}

static void vram_toggle_fwd(void* priv) { vram_toggle(priv,  increment); }
static void vram_toggle_rev(void* priv) { vram_toggle(priv, -increment); }
static void vram_toggle_delta(void* priv)  { menu_quinternary_toggle(&increment); }

#define VRAM_MENU_ENTRY(x)	{ \
		.priv = x, \
		.display	= vram_print, \
		.select		= vram_toggle_fwd, \
		.select_reverse = vram_toggle_rev, \
		.select_auto = update_vram_params_calc, \
	}, \

static struct menu_entry vram_menus[] = {
	VRAM_MENU_ENTRY(0)
	VRAM_MENU_ENTRY(1)
	VRAM_MENU_ENTRY(2)
	VRAM_MENU_ENTRY(3)
	VRAM_MENU_ENTRY(4)
	VRAM_MENU_ENTRY(5)
	VRAM_MENU_ENTRY(6)
	VRAM_MENU_ENTRY(7)
	VRAM_MENU_ENTRY(8)
	VRAM_MENU_ENTRY(9)
	VRAM_MENU_ENTRY(10)
	VRAM_MENU_ENTRY(11)
	VRAM_MENU_ENTRY(12)
	VRAM_MENU_ENTRY(13)
	VRAM_MENU_ENTRY(14)
	VRAM_MENU_ENTRY(15)
	VRAM_MENU_ENTRY(16)
	VRAM_MENU_ENTRY(17)
	VRAM_MENU_ENTRY(18)
	VRAM_MENU_ENTRY(19)
	VRAM_MENU_ENTRY(20)
};

void vram_menus_init()
{
	menu_add("VRAM", vram_menus, COUNT(vram_menus));
}

INIT_FUNC(__FILE__, vram_menus_init);
