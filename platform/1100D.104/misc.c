// misc functions specific to 60D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

int lv_disp_mode;
PROP_HANDLER(PROP_HOUTPUT_TYPE)
{
	lv_disp_mode = buf[1];
	return prop_cleanup(token, property);
}

void display_shooting_info() // called from debug task
{
	if (lv) return;
	
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 185, 250, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		bg = bmp_getpixel(380, 250);
		fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 380 + 2 * font_med.width, 250, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else    bmp_printf(fnt, 380 + 2 * font_med.width, 250, "  ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 380, 250, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else    bmp_printf(fnt, 380, 250, "  ");
	}

	iso_refresh_display();

	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	extern int hdr_steps, hdr_stepsize;
	if (hdr_steps > 1)
		bmp_printf(fnt, 190, 450, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 190, 450, "         ");

	//~ bmp_printf(fnt, 400, 450, "Flash:%s", 
		//~ strobo_firing == 0 ? " ON" : 
		//~ strobo_firing == 1 ? "OFF" : "Auto"
		//~ strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
		//~ );

	bmp_printf(fnt, 40, 460, get_mlu() ? "MLU" : "   ");

	//~ display_lcd_remote_info();
	display_trap_focus_info();
}


// some dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}

// image buffers
// http://magiclantern.wikia.com/wiki/VRAM

PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

struct vram_info * get_yuv422_hd_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = YUV422_HD_BUFFER_DMA_ADDR;
	_vram_info.width =			 lv_dispsize > 1 ? 1024
								  : !is_movie_mode() ? 1056
								  : (video_mode_resolution == 0 ? (digital_zoom_ratio >= 300 ? 1728 : 1680) : 
								  	video_mode_resolution == 1 ? 1280 :
									 video_mode_resolution == 2 ? (video_mode_crop? 640:1024) : 0);
	_vram_info.pitch = _vram_info.width << 1; 
	_vram_info.height =			lv_dispsize > 1 ? 680
								  : !is_movie_mode() ? 704
								  : (video_mode_resolution == 0 ? (digital_zoom_ratio >= 300 ? 972 : 945) : 
								  	video_mode_resolution == 1 ? 560 :
									 video_mode_resolution == 2 ? (video_mode_crop? 480:680) : 0);

	return &_vram_info;
}

static int fastrefresh_direction = 0;


void* get_fastrefresh_422_buf()
{
	if (fastrefresh_direction) {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER:
				return YUV422_LV_BUFFER_2;
			case YUV422_LV_BUFFER_2:
				return YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_3:
				return YUV422_LV_BUFFER;
		}
		return YUV422_LV_BUFFER; // fall back to default
	} else {
		switch (YUV422_LV_BUFFER_DMA_ADDR)
		{
			case YUV422_LV_BUFFER:
				return YUV422_LV_BUFFER_3;
			case YUV422_LV_BUFFER_2:
				return YUV422_LV_BUFFER;
			case YUV422_LV_BUFFER_3:
				return YUV422_LV_BUFFER_2;
		}
		return YUV422_LV_BUFFER; // fall back to default

	}
}

void guess_fastrefresh_direction() {
	static int old_pos = YUV422_LV_BUFFER;
	if (old_pos == YUV422_LV_BUFFER_DMA_ADDR) return;
	if (old_pos == YUV422_LV_BUFFER && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_2) fastrefresh_direction = 1;
	if (old_pos == YUV422_LV_BUFFER && YUV422_LV_BUFFER_DMA_ADDR == YUV422_LV_BUFFER_3) fastrefresh_direction = 0;
	old_pos = YUV422_LV_BUFFER_DMA_ADDR;
}

void* get_write_422_buf()
{
	switch (YUV422_LV_BUFFER_DMA_ADDR)
	{
		case YUV422_LV_BUFFER:
			return YUV422_LV_BUFFER;
		case YUV422_LV_BUFFER_2:
			return YUV422_LV_BUFFER_2;
		case YUV422_LV_BUFFER_3:
			return YUV422_LV_BUFFER_3;
	}
	return YUV422_LV_BUFFER; // fall back to default
}

int vram_width = 720;
int vram_height = 480;
PROP_HANDLER(PROP_VRAM_SIZE_MAYBE)
{
	vram_width = buf[1];
	vram_height = buf[2];
	return prop_cleanup(token, property);
}

struct vram_info * get_yuv422_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = get_fastrefresh_422_buf();
	if (gui_state == GUISTATE_PLAYMENU) _vram_info.vram = (void*) YUV422_LV_BUFFER_DMA_ADDR;

	_vram_info.width = vram_width;
	_vram_info.height = vram_width * 2 / 3;
	_vram_info.pitch = _vram_info.width * 2;

	//~ bmp_printf(FONT_LARGE, 100, 100, "%d x %d", _vram_info.width, _vram_info.height);

	return &_vram_info;
}

/*
int GetBatteryLevel()
{
	if (!is_safe_to_mess_with_the_display(0)) return -1;
	return -1;
	//~ return PD_GetBatteryPower() + 1;
}*/


// dummy stubs, just to compile

// 0xFF1DA424 might be good (dumps=1, score=18)
struct fio_dirent *
FIO_FindFirstEx(
	const char *		dirname,
	struct fio_file *	file
)
{
	return 1;
}

// 0xFF1DA518  might be good (dumps=1, score=17)
/** Returns 0 on success */
int
FIO_FindNextEx(
	struct fio_dirent *	dirent,
	struct fio_file *	file
)
{ return 1;} 

void prop_request_change(unsigned property, void * addr, size_t len) {} // 0xFF05B464 might be good (dumps=1, score=86)
struct gui_task_list gui_task_list; // 0xAF6C  might be good (dumps=1, score=10)
struct mvr_config mvr_config; // 0x5B4C  might be good (dumps=1, score=8.1)
void GUI_Control(){} // 0xFF020E04 might be good (dumps=1, score=6.4)
int CreateRecursiveLock(){ return 0; } // 0xFF073A84  might be good (dumps=1, score=3.3)
void MirrorDisplay(){} // 0xFF337C34 might be good (dumps=1, score=3.3)
void NormalDisplay(){} // 0xFF337C94 might be good (dumps=1, score=3.3)
void AJ_guess_LED_ON(){} // 0xFF347830 might be good (dumps=1, score=2.8)
void AJ_guess_LED_OFF(){} // 0xFF347800 might be good (dumps=1, score=1.9)

void AcquireRecursiveLock(){}
void ReleaseRecursiveLock(){}
void FIO_CleanupAfterFindNext_maybe(){}
void ChangeColorPalette(){}
void ReverseDisplay(){}  
void dialog_redraw(){}
void HideBottomInfoDisp_maybe(){}

int digital_zoom_shortcut = 0;
