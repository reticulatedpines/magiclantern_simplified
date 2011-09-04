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
		bmp_printf(fnt, 360, 279, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		bg = bmp_getpixel(15, 430);
		fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 320 + 2 * font_med.width, 450, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else    bmp_printf(fnt, 320 + 2 * font_med.width, 450, "  ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 320, 450, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else    bmp_printf(fnt, 320, 450, "  ");
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

void* get_422_hd_idle_buf() // the one which is not updated by DMA
{
	switch (YUV422_HD_BUFFER_DMA_ADDR)
	{
		case YUV422_HD_BUFFER:
			return YUV422_HD_BUFFER_2;
		case YUV422_HD_BUFFER_2:
			return YUV422_HD_BUFFER;
	}
	return YUV422_HD_BUFFER; // fall back to default
}

struct vram_info * get_yuv422_hd_vram()
{
	static struct vram_info _vram_info;
	_vram_info.vram = get_422_hd_idle_buf();
	_vram_info.width = recording ? (video_mode_resolution == 0 ? 1720 : 
									video_mode_resolution == 1 ? 1280 : 
									video_mode_resolution == 2 ? 640 : 0)
								  : lv_dispsize > 1 ? 1024
								  : !is_movie_mode() ? 1056
								  : (video_mode_resolution == 0 ? 1056 : 
								  	video_mode_resolution == 1 ? 1024 :
									 video_mode_resolution == 2 ? (video_mode_crop? 640:1024) : 0);
	_vram_info.pitch = _vram_info.width << 1; 
	_vram_info.height = recording ? (video_mode_resolution == 0 ? 974 : 
									video_mode_resolution == 1 ? 580 : 
									video_mode_resolution == 2 ? 480 : 0)
								  : lv_dispsize > 1 ? 680
								  : !is_movie_mode() ? 704
								  : (video_mode_resolution == 0 ? 704 : 
								  	video_mode_resolution == 1 ? 680 :
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


int battery_level = 0;
PROP_HANDLER(PROP_BATTERY_REPORT)
{
	battery_level = buf[1] & 0xff;
	return prop_cleanup(token, property);
}
int GetBatteryLevel()
{
	struct tm now;
	LoadCalendarFromRTC( &now );
	int m = now.tm_min;
	static int prev_m = 0;
	if (m != prev_m) // don't be too aggressive... refresh battery level only once per minute
	{
		if (!is_safe_to_mess_with_the_display(0)) return -1;
		prev_m = m;
		send_event_to_IDLEHandler(LOCAL_REFRESH_BATTERIESHISTORY); 
	}

	return battery_level;
}
