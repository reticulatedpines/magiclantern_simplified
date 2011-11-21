/** \file
 * Shooting experiments: intervalometer, LCD RemoteShot. More to come.
 * 
 * (C) 2010 Alex Dumitrache, broscutamaker@gmail.com
 */
/*
 * Magic Lantern is Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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
#include "lens.h"
#include "gui.h"
#include "math.h"

#ifdef CONFIG_1100D
#include "disable-this-module.h"
#endif

void move_lv_afframe(int dx, int dy);
void movie_start();
void movie_end();
void display_trap_focus_info();
void display_lcd_remote_icon(int x0, int y0);
void intervalometer_stop();
void bulb_ramping_showinfo();
void get_out_of_play_mode();
void wait_till_next_second();

volatile int bulb_shutter_value = 0;

CONFIG_INT("hdr.steps", hdr_steps, 1);
CONFIG_INT("hdr.stepsize", hdr_stepsize, 8);

static CONFIG_INT( "interval.timer.index", interval_timer_index, 2 );
CONFIG_INT( "focus.trap", trap_focus, 0);
//~ static CONFIG_INT( "focus.trap.delay", trap_focus_delay, 1000); // min. delay between two shots in trap focus
static CONFIG_INT( "audio.release-level", audio_release_level, 10);
static CONFIG_INT( "interval.movie.duration.index", interval_movie_duration_index, 2);
//~ static CONFIG_INT( "flash_and_no_flash", flash_and_no_flash, 0);
static CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );        // 0 = off, 1 = normal, 2 = hi-res, 3 = long-exp, 4 = slit-scan
static CONFIG_INT( "silent.pic.submode", silent_pic_submode, 0);   // simple, burst, fullhd
#define silent_pic_burst (silent_pic_submode == 1)
#define silent_pic_fullhd (silent_pic_submode == 2)
static CONFIG_INT( "silent.pic.highres", silent_pic_highres, 0);   // index of matrix size (2x1 .. 5x5)
static CONFIG_INT( "silent.pic.sweepdelay", silent_pic_sweepdelay, 350);
static CONFIG_INT( "silent.pic.slitscan.skipframes", silent_pic_slitscan_skipframes, 1);
static CONFIG_INT( "silent.pic.longexp.time.index", silent_pic_longexp_time_index, 5);
static CONFIG_INT( "silent.pic.longexp.method", silent_pic_longexp_method, 0);
static CONFIG_INT( "zoom.enable.face", zoom_enable_face, 1);
static CONFIG_INT( "zoom.disable.x5", zoom_disable_x5, 0);
static CONFIG_INT( "zoom.disable.x10", zoom_disable_x10, 0);
static CONFIG_INT( "bulb.duration.index", bulb_duration_index, 0);
static CONFIG_INT( "mlu.auto", mlu_auto, 1);

extern int lcd_release_running;

//New option for the sensitivty of the motion release
static CONFIG_INT( "motion.release-level", motion_detect_level, 8);

int get_silent_pic_mode() { return silent_pic_mode; } // silent pic will disable trap focus

static CONFIG_INT("bulb.ramping", bulb_ramping_enabled, 0);
static CONFIG_INT("bulb.ramping.percentile", bramp_percentile, 70);

static volatile int intervalometer_running = 0;
int is_intervalometer_running() { return intervalometer_running; }
static int audio_release_running = 0;
int motion_detect = 0;
//int motion_detect_level = 8;

CONFIG_INT("quick.review.allow.zoom", quick_review_allow_zoom, 0);
PROP_HANDLER(PROP_GUI_STATE)
{
	int gui_state = buf[0];

	if (gui_state == 3 && image_review_time == 0xff && quick_review_allow_zoom && !intervalometer_running && hdr_steps == 1)
	{
		fake_simple_button(BGMT_PLAY);
	}

	return prop_cleanup(token, property);
}

int timer_values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 25, 30, 35, 40, 45, 50, 55, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 1200, 1800, 2700, 3600, 5400, 7200, 9000, 10800, 14400, 18000, 21600, 25200, 28800};
int timer_values_longexp[] = {5, 7, 10, 15, 20, 30, 50, 60, 120, 180, 300, 600, 900, 1800};

typedef int (*CritFunc)(int);
// crit returns negative if the tested value is too high, positive if too low, 0 if perfect
static int bin_search(int lo, int hi, CritFunc crit)
{
	if (lo >= hi-1) return lo;
	int m = (lo+hi)/2;
	int c = crit(m);
	if (c == 0) return m;
	if (c > 0) return bin_search(m, hi, crit);
	return bin_search(lo, m, crit);
}

static int get_exposure_time_ms()
{
	if (is_bulb_mode()) return bulb_shutter_value;
	else return raw2shutter_ms(lens_info.raw_shutter);
}

int get_exposure_time_raw()
{
	if (is_bulb_mode()) return shutter_ms_to_raw(bulb_shutter_value);
	return lens_info.raw_shutter;
}

static void timelapse_calc_display(void* priv, int x, int y, int selected)
{
	int d = timer_values[*(int*)priv];
	int total_time_s = d * avail_shot;
	int total_time_m = total_time_s / 60;
	bmp_printf(FONT(FONT_LARGE, 55, COLOR_BLACK), 
		x, y,
		"%dh%02dm, %dshots, %dfps => %02dm%02ds", 
		total_time_m / 60, 
		total_time_m % 60, 
		avail_shot, video_mode_fps, 
		(avail_shot / video_mode_fps) / 60, 
		(avail_shot / video_mode_fps) % 60
	);
}

static void
interval_timer_display( void * priv, int x, int y, int selected )
{
	if (!is_movie_mode() || silent_pic_mode)
	{
		int d = timer_values[*(int*)priv];
		if (!d)
			bmp_printf(
				selected ? MENU_FONT_SEL : MENU_FONT,
				x, y,
				"Take pics like crazy"
			);
		else
		{
			bmp_printf(
				selected ? MENU_FONT_SEL : MENU_FONT,
				x, y,
				"Take a pic every: %d%s",
				d < 60 ? d : d/60, 
				d < 60 ? "s" : "m"
			);
			if (selected) timelapse_calc_display(&interval_timer_index, x - font_large.width*2, y + font_large.height * 9, selected);
		}
 	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Record %ds, pause %ds",
			timer_values[interval_movie_duration_index],
			timer_values[*(int*)priv]
		);
	}
	
	if (intervalometer_running) menu_draw_icon(x, y, MNI_PERCENT, (*(int*)priv) * 100 / COUNT(timer_values));
	else menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Intervalometer is not active");
}

static void
interval_timer_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = mod(*ptr + 1, COUNT(timer_values));
}
static void
interval_timer_toggle_reverse( void * priv )
{
	unsigned * ptr = priv;
	*ptr = mod(*ptr - 1, COUNT(timer_values));
}

static void
interval_movie_duration_toggle( void * priv )
{
	if (is_movie_mode() && silent_pic_mode == 0)
		interval_movie_duration_index = mod(interval_movie_duration_index + 1, 35);
}

static void 
intervalometer_display( void * priv, int x, int y, int selected )
{
	int p = *(int*)priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Intervalometer  : %s",
		p ? "ON" : "OFF"
	);
	if (selected) timelapse_calc_display(&interval_timer_index, x - font_large.width*2, y + font_large.height * 8, selected);
}

// in lcdsensor.c
void lcd_release_display( void * priv, int x, int y, int selected );

static void
audio_release_display( void * priv, int x, int y, int selected )
{
	//~ if (audio_release_running)
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Audio RemoteShot: %s, level=%d",
		audio_release_running ? "ON" : "OFF",
		audio_release_level
	);
	/*else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Audio RemoteShot: OFF"
		);*/
	//~ menu_draw_icon(x, y, audio_release_running ? MNI_PERCENT : MNI_OFF, audio_release_level * 100 / 30);
}

static void
audio_release_level_toggle(void* priv)
{
	audio_release_level = mod(audio_release_level - 5 + 1, 26) + 5;
}
static void
audio_release_level_toggle_reverse(void* priv)
{
	audio_release_level = mod(audio_release_level - 5 - 1, 26) + 5;
}

//GUI Functions for the motion detect sensitivity.	
static void
motion_release_level_toggle(void* priv)
{
	motion_detect_level = mod(motion_detect_level - 1 + 1, 31) + 1;
}
static void
motion_release_level_toggle_reverse(void* priv)
{
	motion_detect_level = mod(motion_detect_level - 1 - 1, 31) + 1;
}

static void 
motion_detect_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Motion Detect   : %s, level=%d",
		motion_detect == 0 ? "OFF" :
		motion_detect == 1 ? "EXP" :
		motion_detect == 2 ? "DIF" : "err",
		motion_detect_level
	);
	menu_draw_icon(x, y, MNI_BOOL_LV(motion_detect));
}


int get_trap_focus() { return trap_focus; }
/*
void set_flash_firing(int mode)
{
	lens_wait_readytotakepic(64);
	mode = COERCE(mode, 0, 2);
	prop_request_change(PROP_STROBO_FIRING, &mode, 4);
}
static void 
flash_and_no_flash_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Flash / No flash: %s",
		strobo_firing == 2 ? "N/A" : 
		flash_and_no_flash ? "ON " : "OFF"
	);
}

static void
flash_and_no_flash_toggle( void * priv )
{
	flash_and_no_flash = !flash_and_no_flash;
	if (!flash_and_no_flash)
		set_flash_firing(0); // force on
}*/

                                                 //2  4  6  9 12 16 20 25
static const int16_t silent_pic_sweep_modes_l[] = {2, 2, 2, 3, 3, 4, 4, 5};
static const int16_t silent_pic_sweep_modes_c[] = {1, 2, 3, 3, 4, 4, 5, 5};
#define SILENTPIC_NL COERCE(silent_pic_sweep_modes_l[COERCE(silent_pic_highres,0,COUNT(silent_pic_sweep_modes_l)-1)], 0, 5)
#define SILENTPIC_NC COERCE(silent_pic_sweep_modes_c[COERCE(silent_pic_highres,0,COUNT(silent_pic_sweep_modes_c)-1)], 0, 5)

static void 
silent_pic_display( void * priv, int x, int y, int selected )
{
	if (silent_pic_mode == 0)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Silent/Slit Pic : OFF"
		);
	}
	else if (silent_pic_mode == 1)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Silent Picture  : %s",
			silent_pic_burst ? "Burst" : 
			silent_pic_fullhd ? "FullHD" : "Single"
		);
	}
	else if (silent_pic_mode == 2)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Silent Pic HiRes: %dx%d",
			SILENTPIC_NL,
			SILENTPIC_NC
		);
		bmp_printf(FONT_MED, x + 430, y+5, "%dx%d", SILENTPIC_NC*(1024-8), SILENTPIC_NL*(680-8));
	}
	/*else if (silent_pic_mode == 3)
	{
		int t = timer_values_longexp[mod(silent_pic_longexp_time_index, COUNT(timer_values_longexp))];
		unsigned fnt = selected ? MENU_FONT_SEL : MENU_FONT;
		bmp_printf(
			FONT(fnt, COLOR_RED, FONT_BG(fnt)),
			x, y,
			"Silent Pic LongX: %ds",
			t
			//~ silent_pic_longexp_method == 0 ? "AVG" :
			//~ silent_pic_longexp_method == 1 ? "MAX" :
			//~ silent_pic_longexp_method == 2 ? "SUM" : "err"
		);
	}*/
	else if (silent_pic_mode == 4)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Slit-scan Pic   : 1ln/%dclk",
			silent_pic_slitscan_skipframes
		);
	}
	//~ menu_draw_icon(x, y, MNI_BOOL_LV(silent_pic_mode), 0);
}

static void silent_pic_mode_increment()
{
	silent_pic_mode = mod(silent_pic_mode + 1, 5); // off, normal, hi-res, long-exp, slit
}
static void silent_pic_mode_toggle(void* priv)
{
	silent_pic_mode_increment();
	//~ if (silent_pic_mode == 2) silent_pic_mode_increment(); // skip hi-res
	if (silent_pic_mode == 3) silent_pic_mode_increment(); // skip longx
	//~ if (silent_pic_mode == 4) silent_pic_mode_increment(); // skip slit
}

static void silent_pic_toggle(int sign)
{
	if (silent_pic_mode == 1)
		silent_pic_submode = mod(silent_pic_submode + 1, 3);
	else if (silent_pic_mode == 2) 
		silent_pic_highres = mod(silent_pic_highres + sign, COUNT(silent_pic_sweep_modes_c));
	/*else if (silent_pic_mode == 3)
	{
		silent_pic_longexp_time_index = mod(silent_pic_longexp_time_index + sign, COUNT(timer_values_longexp));
	}*/
	else if (silent_pic_mode == 4)
		silent_pic_slitscan_skipframes = mod(silent_pic_slitscan_skipframes + sign - 1, 4) + 1;
}
static void silent_pic_toggle_forward(void* priv)
{ silent_pic_toggle(1); }

static void silent_pic_toggle_reverse(void* priv)
{ silent_pic_toggle(-1); }

int afframe[26];
PROP_HANDLER( PROP_LV_AFFRAME ) {
	memcpy(afframe, buf, 0x68);
	return prop_cleanup( token, property );
}

void get_afframe_pos(int W, int H, int* x, int* y)
{
	*x = (afframe[2] + afframe[4]/2) * W / afframe[0];
	*y = (afframe[3] + afframe[5]/2) * H / afframe[1];
}

int face_zoom_request = 0;

#if 0
int hdr_intercept = 1;

void halfshutter_action(int v)
{
	if (!hdr_intercept) return;
	static int prev_v;
	if (v == prev_v) return;
	prev_v = v;


	// avoid camera shake for HDR shots => force self timer
	static int drive_mode_bk = -1;
	if (v == 1 && (hdr_steps > 1 || is_focus_stack_enabled()) && drive_mode != DRIVE_SELFTIMER_2SEC && drive_mode != DRIVE_SELFTIMER_REMOTE)
	{
		drive_mode_bk = drive_mode;
		lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
	}

	// restore drive mode if it was changed
	if (v == 0 && drive_mode_bk >= 0)
	{
		lens_set_drivemode(drive_mode_bk);
		drive_mode_bk = -1;
	}
}
#endif

int hs = 0;
PROP_HANDLER( PROP_HALF_SHUTTER ) {
	int v = *(int*)buf;
	if (zoom_enable_face)
	{
		if (v == 0 && lv && lvaf_mode == 2 && gui_state == 0 && !recording) // face detect
			face_zoom_request = 1;
	}
/*	if (v && gui_menu_shown() && !is_menu_active("Focus"))
	{
		menu_stop();
	}*/
	
	//~ if (hdr_steps > 1) halfshutter_action(v);
	
	return prop_cleanup( token, property );
}

int handle_shutter_events(struct event * event)
{
	return 1;
#if 0 // not reliable
	if (hdr_steps > 1)
	{
		switch(event->param)
		{
			case BGMT_PRESS_HALFSHUTTER:
			case BGMT_UNPRESS_HALFSHUTTER:
			{
				int h = HALFSHUTTER_PRESSED;
				if (!h) msleep(50); // avoids cancelling self-timer too early
				halfshutter_action(h);
			}
		}
	}
	return 1;
#endif
}

/*int sweep_lv_on = 0;
static void 
sweep_lv_start(void* priv)
{
	sweep_lv_on = 1;
}*/

int center_lv_aff = 0;
void center_lv_afframe()
{
	center_lv_aff = 1;
}
void center_lv_afframe_do()
{
	if (!lv || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;
	int cx = (afframe[0] - afframe[4])/2;
	int cy = (afframe[1] - afframe[5])/2;
	if (afframe[2] == cx && afframe[3] == cy) 
	{
		move_lv_afframe(10,10);
		msleep(100);
	}
	afframe[2] = cx;
	afframe[3] = cy;
	prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
}

void move_lv_afframe(int dx, int dy)
{
	if (!lv || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;
	afframe[2] = COERCE(afframe[2] + dx, 500, afframe[0] - afframe[4]);
	afframe[3] = COERCE(afframe[3] + dy, 500, afframe[1] - afframe[5]);
	prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
}

/*
static void 
sweep_lv()
{
	if (recording) return;
	if (!lv) return;
	menu_stop();
	msleep(2000);
	int zoom = 5;
	prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
	msleep(2000);
	
	int i,j;
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 5; j++)
		{
			bmp_printf(FONT_LARGE, 50, 50, "AFF %d, %d ", i, j);
			afframe[2] = 250 + 918 * j;
			afframe[3] = 434 + 490 * i;
			prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
			msleep(100);
		}
	}

	zoom = 1;
	prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
}*/

#if 0 // does not work... out of memory?
uint8_t* read_entire_file(const char * filename, int* buf_size)
{
	bmp_printf(FONT_LARGE, 0, 40, "read %s ", filename);
	msleep(1000);

	*buf_size = 0;
	unsigned size;
	if( FIO_GetFileSize( filename, &size ) != 0 )
		goto getfilesize_fail;

	DEBUG("File '%s' size %d bytes", filename, size);

	bmp_printf(FONT_LARGE, 0, 40, "size %d ", size);
	msleep(1000);

	uint8_t * buf = alloc_dma_memory( size );
	if( !buf )
	{
		DebugMsg( DM_MAGIC, 3, "%s: alloc_dma_memory failed", filename );
		goto malloc_fail;
	}

	bmp_printf(FONT_LARGE, 0, 40, "alloc %x ", buf);
	msleep(1000);

	size_t rc = read_file( filename, buf, size );
	if( rc != size )
		goto read_fail;

	bmp_printf(FONT_LARGE, 0, 40, "read ok ");
	msleep(1000);

	// Since the read was into uncacheable memory, it will
	// be very slow to access.  Copy it into a cached buffer
	// and release the uncacheable space.
	//~ uint8_t * fast_buf = AllocateMemory( size + 32);
	//~ if( !fast_buf )
		//~ goto fail_buf_copy;

	//~ bmp_printf(FONT_LARGE, 0, 40, "alloc fast %x ", fast_buf);
	//~ msleep(1000);

	//~ memcpy(fast_buf, buf, size);
	//~ free_dma_memory( buf );
	*buf_size = size;

	bmp_printf(FONT_LARGE, 0, 40, "almost done ");
	msleep(1000);

	return buf;

fail_buf_copy:
read_fail:
	free_dma_memory( buf );
malloc_fail:
getfilesize_fail:
	DEBUG("failed");
	return NULL;
}

static void
convert_yuv_to_bmp(char* file_yuv, char* file_bmp)
{
	int yuv_size;
	int width, height;
	void* yuv = read_entire_file(file_yuv, &yuv_size);
	if (!yuv)
	{
		bmp_printf(FONT_LARGE, 0, 40, "read error %s", file_yuv);
		msleep(1000);
		return;
	}
	if (yuv_size == 1056*704*2)
	{
		width = 1056;
		height = 704;
	}
	else if (yuv_size == 1720*974*2)
	{
		width = 1720;
		height = 974;
	}
	else
	{
		bmp_printf(FONT_LARGE, 0, 40, "unk yuv size: %d ", yuv_size);
		free_dma_memory(yuv);
		return;
	}
	
	int bmp_size = width * height * 3;
	void* bmpbuf = AllocateMemory(bmp_size + 32);
	if (!bmpbuf)
	{
		bmp_printf(FONT_LARGE, 0, 40, "malloc error");
		free_dma_memory(yuv);
		return;
	}
	
	// AJ equations for YUV -> RGB
	// anyone wants to optimize this?
	#define Y(i) (*(uint8_t*)(yuv + i * 2 + 1))
	#define U(i) (*(int8_t*)(yuv + i * 4))
	#define V(i) (*(int8_t*)(yuv + i * 4 + 2))
	#define R(i) (*(uint8_t*)(bmpbuf + i * 3))
	#define G(i) (*(uint8_t*)(bmpbuf + i * 3 + 1))
	#define B(i) (*(uint8_t*)(bmpbuf + i * 3 + 2))
	int i;
	int N = width*height;
	for (i = 0; i < N; i++)
	{
		R(i) = COERCE( Y(i) + 1.403 * V(i/2),                  0, 255);
		G(i) = COERCE( Y(i) - 0.344 * U(i/2) - 0.714 * V(i/2), 0, 255);
		B(i) = COERCE( Y(i) + 1.770 * U(i/2),                  0, 255);
	}
	#undef Y
	#undef U
	#undef V
	#undef R
	#undef G
	#undef B

	struct bmp_file_t bmp;
	bmp.signature = 0x4D42;
	bmp.size = bmp_size + sizeof(bmp);
	bmp.res_0 = 0;
	bmp.res_1 = 0;
	bmp.image = 54; // offset
	bmp.hdr_size = 40;
	bmp.width = width;
	bmp.height = height;
	bmp.planes = 1;
	bmp.bits_per_pixel = 24;
	bmp.compression = 0;
	bmp.image_size = bmp_size; // yuv buffers are always multiples of 16
	bmp.hpix_per_meter = 2835; // from wikipedia
	bmp.vpix_per_meter = 2835; // from wikipedia
	bmp.num_colors = 0;
	bmp.num_imp_colors = 0;


	FILE *f;
	FIO_RemoveFile(file_bmp);
	f = FIO_CreateFile(file_bmp);
	if (f == INVALID_PTR)
	{
		bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", file_bmp);
		free_dma_memory(yuv);
		FreeMemory(bmpbuf);
		return;
	}
	FIO_WriteFile(f, &bmp, sizeof(bmp));
	FIO_WriteFile(f, &bmpbuf, bmp_size);

	FIO_CloseFile(f);
	free_dma_memory(yuv);
	FreeMemory(bmpbuf);
}

static void
convert_all_yuvs_to_bmp_folder(char* folder) // folder includes /
{
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( folder, &file );
	if( IS_ERROR(dirent) )
	{
		bmp_printf( FONT_LARGE, 40, 40,
			"%s: dirent=%08x!",
			__func__,
			(unsigned) dirent
		);
		return;
	}

	int k = 0;
	do {
		if (file.mode & 0x20) // regular file
		{
			char* s = strstr(file.name, ".422");
			if (s)
			{
				char yuvname[100];
				char bmpname[100];
				snprintf(yuvname, sizeof(yuvname), "%s%s", folder, file.name);
				*s = 0;
				snprintf(bmpname, sizeof(yuvname), "%s%s.BMP", folder, file.name);
				bmp_printf(FONT_MED, 0, 40, "bmp %s \nyuv %s ", bmpname, yuvname);
				msleep(1000);
				convert_yuv_to_bmp(yuvname, bmpname);
				msleep(1000);
				return;
			}
		}
	} while( FIO_FindNextEx( dirent, &file ) == 0);
}
static void
convert_all_yuvs_to_bmp()
{
	convert_yuv_to_bmp(CARD_DRIVE "DCIM/100CANON/1324-001.422", CARD_DRIVE "DCIM/100CANON/1324-001.BMP");
	return;
	bmp_printf(FONT_MED, 0, 40, "yuv to bmp...");
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "DCIM/", &file );
	if( IS_ERROR(dirent) )
	{
		bmp_printf( FONT_LARGE, 40, 40,
			"%s: dirent=%08x!",
			__func__,
			(unsigned) dirent
		);
		return;
	}

	int k = 0;
	do {
		if ((file.mode & 0x10) && (file.name[0] != '.')) // directory
		{
			char folder[100];
			snprintf(folder, sizeof(folder), CARD_DRIVE "DCIM/%s/", file.name);
			convert_all_yuvs_to_bmp_folder(folder);
		}
	} while( FIO_FindNextEx( dirent, &file ) == 0);
}

int convert_yuv_bmp_flag = 0;
static void
convert_all_yuvs_start()
{
	convert_yuv_bmp_flag = 1;
}
#endif

void vsync(volatile int* addr)
{
	int i;
	int v0 = *addr;
	for (i = 0; i < 100; i++)
	{
		if (*addr != v0) return;
		msleep(10);
	}
	bmp_printf(FONT_MED, 30, 100, "vsync failed");
}

static char* silent_pic_get_name()
{
	static char imgname[100];
	static int silent_number = 1; // cache this number for speed (so it won't check all files until 10000 to find the next free number)
	
	static int prev_file_number = -1;
	static int prev_folder_number = -1;
	
	if (prev_file_number != file_number) silent_number = 1;
	if (prev_folder_number != folder_number) silent_number = 1;
	
	prev_file_number = file_number;
	prev_folder_number = folder_number;
	
	if (intervalometer_running)
	{
		//~ int timelapse_number;
		for ( ; silent_number < 100000000; silent_number++)
		{
			snprintf(imgname, sizeof(imgname), CARD_DRIVE "DCIM/%03dCANON/%08d.422", folder_number, silent_number);
			unsigned size;
			if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
			if (size == 0) break;
		}
	}
	else
	{
		for ( ; silent_number < 1000; silent_number++)
		{
			snprintf(imgname, sizeof(imgname), CARD_DRIVE "DCIM/%03dCANON/%04d-%03d.422", folder_number, file_number, silent_number);
			unsigned size;
			if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
			if (size == 0) break;
		}
	}
	bmp_printf(FONT_MED, 100, 130, "%s    ", imgname);
	return imgname;
}

int ms100_clock = 0;
static void
ms100_clock_task( void* unused )
{
	while(1)
	{
		msleep(100);
		ms100_clock += 100;
	}
}
TASK_CREATE( "ms100_clock_task", ms100_clock_task, 0, 0x19, 0x1000 );

int expfuse_running = 0;
int expfuse_num_images = 0;
struct semaphore * set_maindial_sem = 0;

int compute_signature(int* start, int num)
{
	int c = 0;
	int* p;
	for (p = start; p < start + num; p++)
	{
		c += *p;
	}
	//~ return SIG_60D_110;
	return c;
}

void add_yuv_acc16bit_src8bit(void* acc, void* src, int numpix)
{
	int16_t* accs = acc;
	uint16_t* accu = acc;
	int8_t* srcs = src;
	uint8_t* srcu = src;
	int i;
	for (i = 0; i < numpix; i++)
	{
		accs[i*2] += srcs[i*2]; // chroma, signed
		accu[i*2+1] += srcu[i*2+1]; // luma, unsigned
	}
}

void div_yuv_by_const_dst8bit_src16bit(void* dst, void* src, int numpix, int den)
{
	int8_t* dsts = dst;
	uint8_t* dstu = dst;
	int16_t* srcs = src;
	uint16_t* srcu = src;
	int i;
	for (i = 0; i < numpix; i++)
	{
		dsts[i*2] = srcs[i*2] / den; // chroma, signed
		dstu[i*2+1] = srcu[i*2+1] / den; // luma, unsigned
	}
}

// octave:
// x = linspace(0,1,256);
// f = @(x) exp(-(x-0.5).^2 ./ 0.32) # mean=0.5, sigma=0.4
// sprintf("0x%02x, ",f(x) * 100)
static uint8_t gauss_lut[] = {0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 0x39, 0x3a, 0x3b, 0x3b, 0x3c, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f, 0x3f, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x48, 0x48, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4c, 0x4c, 0x4d, 0x4d, 0x4e, 0x4e, 0x4f, 0x4f, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 0x58, 0x58, 0x58, 0x59, 0x59, 0x5a, 0x5a, 0x5a, 0x5b, 0x5b, 0x5c, 0x5c, 0x5c, 0x5d, 0x5d, 0x5d, 0x5e, 0x5e, 0x5e, 0x5f, 0x5f, 0x5f, 0x5f, 0x60, 0x60, 0x60, 0x60, 0x61, 0x61, 0x61, 0x61, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x61, 0x61, 0x61, 0x61, 0x60, 0x60, 0x60, 0x60, 0x5f, 0x5f, 0x5f, 0x5f, 0x5e, 0x5e, 0x5e, 0x5d, 0x5d, 0x5d, 0x5c, 0x5c, 0x5c, 0x5b, 0x5b, 0x5a, 0x5a, 0x5a, 0x59, 0x59, 0x58, 0x58, 0x58, 0x57, 0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x54, 0x53, 0x53, 0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x4f, 0x4f, 0x4e, 0x4e, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4a, 0x4a, 0x49, 0x49, 0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 0x44, 0x44, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40, 0x3f, 0x3f, 0x3e, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x38, 0x37, 0x37, 0x36, 0x35, 0x35, 0x34, 0x34, 0x33, 0x32, 0x32, 0x31, 0x31, 0x30, 0x30, 0x2f, 0x2e, 0x2e, 0x2d};

void weighted_mean_yuv_init_acc32bit_ws16bit(void* acc, void* weightsum, int numpix)
{
	bzero32(acc, numpix*8);
	bzero32(weightsum, numpix*4);
}

void weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(void* acc, void* src, void* weightsum, int numpix)
{
	int32_t* accs = acc;
	uint32_t* accu = acc;
	int8_t* srcs = src;
	uint8_t* srcu = src;
	uint16_t* ws = weightsum;
	int i;
	for (i = 0; i < numpix; i++)
	{
		int w = gauss_lut[srcu[i*2+1]];
		accs[i*2] += srcs[i*2] * w; // chroma, signed
		accu[i*2+1] += srcu[i*2+1] * w; // luma, unsigned
		ws[i] += w;
	}
}

void weighted_mean_yuv_div_dst8bit_src32bit_ws16bit(void* dst, void* src, void* weightsum, int numpix)
{
	int8_t* dsts = dst;
	uint8_t* dstu = dst;
	int32_t* srcs = src;
	uint32_t* srcu = src;
	uint16_t* ws = weightsum;
	int i;
	for (i = 0; i < numpix; i++)
	{
		int wt = ws[i];
		dsts[i*2] = srcs[i*2] / wt; // chroma, signed
		dstu[i*2+1] = COERCE(srcu[i*2+1] / wt, 0, 255); // luma, unsigned
	}
}

void next_image_in_play_mode(int dir)
{
	if (!PLAY_MODE) return;
	void* buf_lv = get_yuv422_vram()->vram;
	// ask for next image
	fake_simple_button(dir > 0 ? BGMT_WHEEL_DOWN : BGMT_WHEEL_UP);
	int k = 0;
	// wait for image buffer location to be flipped => next image was loaded
	while (get_yuv422_vram()->vram == buf_lv && k < 50) 
	{
		msleep(100);
		k++;
	}
}

void playback_compare_images_task(int dir)
{
	take_semaphore(set_maindial_sem, 0);
	
	if (!PLAY_MODE) { fake_simple_button(BGMT_PLAY); msleep(500); }
	if (!PLAY_MODE) { NotifyBox(1000, "CompareImages: Not in PLAY mode"); return; }
	
	if (dir == 0) // reserved for intervalometer
	{
		next_image_in_play_mode(-1);
		dir = 1;
	}
	
	void* aux_buf = (void*)YUV422_HD_BUFFER_2;
	void* current_buf;
	int w = get_yuv422_vram()->width;
	int h = get_yuv422_vram()->height;
	int buf_size = w * h * 2;
	current_buf = get_yuv422_vram()->vram;
	yuv_halfcopy(aux_buf, current_buf, w, h, 1);
	next_image_in_play_mode(dir);
	current_buf = get_yuv422_vram()->vram;
	yuv_halfcopy(aux_buf, current_buf, w, h, 0);
	current_buf = get_yuv422_vram()->vram;
	memcpy(current_buf, aux_buf, buf_size);
	give_semaphore(set_maindial_sem);
}

void playback_compare_images(int dir)
{
	task_create("playcompare_task", 0x1c, 0, playback_compare_images_task, (void*)dir);
}

void expfuse_preview_update_task(int dir)
{
	take_semaphore(set_maindial_sem, 0);
	void* buf_acc = (void*)YUV422_HD_BUFFER_1;
	void* buf_ws = (void*)YUV422_HD_BUFFER_2;
	void* buf_lv = get_yuv422_vram()->vram;
	int numpix = get_yuv422_vram()->width * get_yuv422_vram()->height;
	if (!expfuse_running)
	{
		// first image 
		weighted_mean_yuv_init_acc32bit_ws16bit(buf_acc, buf_ws, numpix);
		weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(buf_acc, buf_lv, buf_ws, numpix);
		expfuse_num_images = 1;
		expfuse_running = 1;
	}
	next_image_in_play_mode(dir);
	buf_lv = get_yuv422_vram()->vram; // refresh
	// add new image

	weighted_mean_yuv_add_acc32bit_src8bit_ws16bit(buf_acc, buf_lv, buf_ws, numpix);
	weighted_mean_yuv_div_dst8bit_src32bit_ws16bit(buf_lv, buf_acc, buf_ws, numpix);
	expfuse_num_images++;
	bmp_printf(FONT_MED, 0, 0, "%d images  ", expfuse_num_images);
	//~ bmp_printf(FONT_LARGE, 0, 480 - font_large.height, "Do not press Delete!");

	give_semaphore(set_maindial_sem);
}

void expfuse_preview_update(int dir)
{
	task_create("expfuse_task", 0x1c, 0, expfuse_preview_update_task, (void*)dir);
}

// that's extremely inefficient
int find_422(int * index, char* fn)
{
	struct fio_file file;
	struct fio_dirent * dirent = 0;
	int N = 0;
	
	dirent = FIO_FindFirstEx( CARD_DRIVE "DCIM/100CANON/", &file );
	if( IS_ERROR(dirent) )
	{
		bmp_printf( FONT_LARGE, 40, 40, "dir err" );
		return 0;
	}

	do {
		if (file.mode & 0x10) continue; // is a directory
		int n = strlen(file.name);
		if ((n > 4) && (streq(file.name + n - 4, ".422")))
			N++;
	} while( FIO_FindNextEx( dirent, &file ) == 0);
	FIO_CleanupAfterFindNext_maybe(dirent);

	static int old_N = 0;
	if (N != old_N) // number of pictures was changed, display the last one
	{
		old_N = N;
		*index = N-1;
	}
	
	*index = mod(*index, N);

	dirent = FIO_FindFirstEx( CARD_DRIVE "DCIM/100CANON/", &file );
	if( IS_ERROR(dirent) )
	{
		bmp_printf( FONT_LARGE, 40, 40, "dir err" );
		return 0;
	}

	int k = 0;
	int found = 0;
	do {
		if (file.mode & 0x10) continue; // is a directory
		int n = strlen(file.name);
		if ((n > 4) && (streq(file.name + n - 4, ".422")))
		{
			if (k == *index)
			{
				snprintf(fn, 100, CARD_DRIVE "DCIM/100CANON/%s", file.name);
				found = 1;
			}
			k++;
		}
	} while( FIO_FindNextEx( dirent, &file ) == 0);
	FIO_CleanupAfterFindNext_maybe(dirent);
	return found;
}

void play_next_422_task(int dir)
{
	take_semaphore(set_maindial_sem, 0);
	
	static int index = -1;
	static char ffn[100];
	
	index += dir;
	
	if (find_422(&index, ffn))
	{
		play_422(ffn);
		//~ bmp_printf(FONT_LARGE, 0, 0, ffn);
	}
	else
	{
		bmp_printf(FONT_LARGE, 0, 0, "No 422 files found");
	}

	give_semaphore(set_maindial_sem);
}


void play_next_422(int dir)
{
	task_create("422_task", 0x1c, 0, play_next_422_task, (void*)dir);
}


/*
static void
silent_pic_take_longexp()
{
	bmp_printf(FONT_MED, 100, 100, "Psst!");
	struct vram_info * vram = get_yuv422_hd_vram();
	int bufsize = vram->height * vram->pitch;
	int numpix = vram->height * vram->width;
	void* longexp_buf = 0x44000060 + bufsize + 4096;
	bzero32(longexp_buf, bufsize*2);
	
	// check if the buffer appears to be used
	int i;
	int s1 = compute_signature(longexp_buf, bufsize/2);
	msleep(100);
	int s2 = compute_signature(longexp_buf, bufsize/2);
	if (s1 != s2) { bmp_printf(FONT_MED, 100, 100, "Psst! can't use buffer at %x ", longexp_buf); return; }

	ms100_clock = 0;
	int tmax = timer_values_longexp[silent_pic_longexp_time_index] * 1000;
	int num = 0;
	while (ms100_clock < tmax)
	{
		bmp_printf(FONT_MED, 100, 100, "Psst! Taking a long-exp silent pic (%dimg,%ds/%ds)...   ", num, ms100_clock/1000, tmax/1000);
		add_yuv_acc16bit_src8bit(longexp_buf, vram->vram, numpix);
		num += 1;
	}
	open_canon_menu();
	msleep(500);
	div_yuv_by_const_dst8bit_src16bit(vram->vram, longexp_buf, numpix, num);
	char* imgname = silent_pic_get_name();
	FIO_RemoveFile(imgname);
	FILE* f = FIO_CreateFile(imgname);
	if (f == INVALID_PTR)
	{
		bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
		return;
	}
	FIO_WriteFile(f, vram->vram, vram->height * vram->pitch);
	FIO_CloseFile(f);
	clrscr(); play_422(imgname);
	bmp_printf(FONT_MED, 100, 100, "Psst! Just took a long-exp silent pic   ");
}
*/

void ensure_movie_mode()
{
	if (!is_movie_mode())
	{
		#ifdef CONFIG_50D
		if (!lv) force_liveview();
		GUI_SetLvMode(2);
		GUI_SetMovieSize_b(1);
		#else
		 #ifdef CONFIG_5D2
			GUI_SetLvMode(2);
		 #else
		  set_shooting_mode(SHOOTMODE_MOVIE);
		 #endif
		#endif
		msleep(500); 
	}
	if (!lv) force_liveview();
}

static int
silent_pic_ensure_movie_mode()
{
	if (silent_pic_fullhd && !is_movie_mode()) 
	{ 
		ensure_movie_mode();
	}
	#ifndef CONFIG_600D // on 600D you only have to go in movie mode
	if (silent_pic_fullhd && !recording)
	{
		movie_start();
		return 1;
	}
	#endif
	return 0;
}

void stop_recording_and_delete_movie()
{
	if (recording)
	{
		movie_end();
		char name[100];
		snprintf(name, sizeof(name), CARD_DRIVE "DCIM/%03dCANON/MVI_%04d.THM", folder_number, file_number);
		FIO_RemoveFile(name);
		snprintf(name, sizeof(name), CARD_DRIVE "DCIM/%03dCANON/MVI_%04d.MOV", folder_number, file_number);
		FIO_RemoveFile(name);
	}
}

static void
silent_pic_stop_dummy_movie()
{ 
	#ifndef CONFIG_600D
	stop_recording_and_delete_movie();
	#endif
}

static void
silent_pic_take_simple(int interactive)
{
	int movie_started = silent_pic_ensure_movie_mode();
	
	char* imgname = silent_pic_get_name();

	if (interactive)
	{
		NotifyBoxHide();
		NotifyBox(10000, "Psst! Taking a picture");
	}

	if (!silent_pic_burst) // single mode
	{
		while (get_halfshutter_pressed()) msleep(100);
		//~ if (!recording) { open_canon_menu(); msleep(300); clrscr(); }
	}

	if (!silent_pic_burst) { PauseLiveView(); }

	struct vram_info * vram = get_yuv422_hd_vram();
	dump_seg(vram->vram, vram->pitch * vram->height, imgname);

	if (interactive && !silent_pic_burst)
	{
		NotifyBoxHide();
		msleep(500); clrscr();
		play_422(imgname);
		msleep(1000);
	}
	
	if (!silent_pic_burst) ResumeLiveView();
	
	if (movie_started) silent_pic_stop_dummy_movie();
}

void
silent_pic_take_lv_dbg()
{
	struct vram_info * vram = get_yuv422_vram();
	int silent_number;
	char imgname[100];
	for (silent_number = 0 ; silent_number < 1000; silent_number++) // may be slow after many pics
	{
		snprintf(imgname, sizeof(imgname), CARD_DRIVE "VRAM%d.422", silent_number);
		unsigned size;
		if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
		if (size == 0) break;
	}
	dump_seg(vram->vram, vram->pitch * vram->height, imgname);
}

int silent_pic_sweep_running = 0;
static void
silent_pic_take_sweep(int interactive)
{
	if (recording) return;
	if (!lv) return;
	if (SILENTPIC_NL > 4 || SILENTPIC_NC > 4)
	{
		if ((af_mode & 0xF) != 3 )
		{
			NotifyBox(2000, "Matrices higher than 4x4\n"
							"require manual focus.   "); 
			msleep(2000);
			return; 
		}
	}

	bmp_printf(FONT_MED, 100, 100, "Psst! Preparing for high-res pic   ");
	while (get_halfshutter_pressed()) msleep(100);
	menu_stop();

	bmp_draw_rect(COLOR_WHITE, (5-SILENTPIC_NC) * 360/5, (5-SILENTPIC_NL)*240/5, SILENTPIC_NC*720/5-1, SILENTPIC_NL*480/5-1);
	msleep(200);
	if (interactive) msleep(2000);
	redraw(); msleep(100);
	
	int afx0 = afframe[2];
	int afy0 = afframe[3];

	silent_pic_sweep_running = 1;
	int zoom = 5;
	prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
	msleep(1000);

	struct vram_info * vram = get_yuv422_hd_vram();

	char* imgname = silent_pic_get_name();

	FIO_RemoveFile(imgname);
	FILE* f = FIO_CreateFile(imgname);
	if (f == INVALID_PTR)
	{
		bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
		return;
	}
	int i,j;
	int NL = SILENTPIC_NL;
	int NC = SILENTPIC_NC;
	int x0 = (SENSOR_RES_X - NC * 1024) / 2;
	int y0 = (SENSOR_RES_Y - NL * 680) / 2;
	for (i = 0; i < NL; i++)
	{
		for (j = 0; j < NC; j++)
		{
			// afframe[2,3]: x,y
			// range obtained by moving the zoom window: 250 ... 3922, 434 ... 2394 => upper left corner
			// full-res: 5202x3465
			// buffer size: 1024x680
			bmp_printf(FONT_MED, 100, 100, "Psst! Taking a high-res pic [%d,%d]      ", i, j);
			afframe[2] = x0 + 1024 * j;
			afframe[3] = y0 + 680 * i;
			prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
			//~ msleep(500);
			msleep(silent_pic_sweepdelay);
			FIO_WriteFile(f, vram->vram, 1024 * 680 * 2);
			//~ bmp_printf(FONT_MED, 20, 150, "=> %d", ans);
			msleep(50);
		}
	}
	FIO_CloseFile(f);
	
	// restore
	zoom = 1;
	prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
	msleep(1000);
	afframe[2] = afx0;
	afframe[3] = afy0;
	prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
	silent_pic_sweep_running = 0;

	bmp_printf(FONT_MED, 100, 100, "Psst! Just took a high-res pic   ");

}

static void
silent_pic_take_slitscan(int interactive)
{
	#if defined(CONFIG_550D) || defined(CONFIG_500D) || defined(CONFIG_60D)
	//~ if (recording) return; // vsync fails
	if (!lv) return;
	menu_stop();
	while (get_halfshutter_pressed()) msleep(100);
	msleep(500);
	clrscr();

	uint8_t * const lvram = UNCACHEABLE(YUV422_LV_BUFFER_1);
	int lvpitch = YUV422_LV_PITCH;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	#define BMPPITCH 960

	struct vram_info * vram = get_yuv422_hd_vram();
	NotifyBox(60000, "Psst! Slit-scan pic (%dx%d)", vram->width, vram->height);

	char* imgname = silent_pic_get_name();

	FIO_RemoveFile(imgname);
	FILE* f = FIO_CreateFile(imgname);
	if (f == INVALID_PTR)
	{
		bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
		return;
	}
	int i;
	for (i = 0; i < vram->height; i++)
	{
		int k;
		for (k = 0; k < (int)silent_pic_slitscan_skipframes; k++)
			vsync((void*)YUV422_HD_BUFFER_DMA_ADDR);
		
		FIO_WriteFile(f, (void*)(YUV422_HD_BUFFER_DMA_ADDR + i * vram->pitch), vram->pitch);

		int y = i * 480 / vram->height;
		uint16_t * const v_row = (uint16_t*)( lvram + y * lvpitch );        // 1 pixel
		uint8_t * const b_row = (uint8_t*)( bvram + y * BMPPITCH);          // 1 pixel
		uint16_t* lvp; // that's a moving pointer through lv vram
		uint8_t* bp;  // through bmp vram
		for (lvp = v_row, bp = b_row; lvp < v_row + 720 ; lvp++, bp++)
			*bp = ((*lvp) * 41 >> 16) + 38;
		
		if (get_halfshutter_pressed())
		{
			FIO_CloseFile(f);
			FIO_RemoveFile(imgname);
			clrscr();
			NotifyBoxHide();
			NotifyBox(2000, "Slit-scan cancelled.");
			while (get_halfshutter_pressed()) msleep(100);
			return;
		}
	}
	FIO_CloseFile(f);

	NotifyBoxHide();
	//~ NotifyBox(2000, "Psst! Just took a slit-scan pic");

	if (!interactive) return;

	PauseLiveView();
	play_422(imgname);
	// wait half-shutter press and clear the screen
	while (!get_halfshutter_pressed()) msleep(100);
	while (get_halfshutter_pressed()) msleep(100);
	clrscr();
	ResumeLiveView();
	
	#endif
}

static void
silent_pic_take(int interactive) // for remote release, set interactive=0
{
	if (!lv) force_liveview();

	if (beep_enabled) Beep();
	
	idle_globaldraw_dis();
	
	if (silent_pic_mode == 1) // normal
		silent_pic_take_simple(interactive);
	else if (silent_pic_mode == 2) // hi-res
		silent_pic_take_sweep(interactive);
	//~ else if (silent_pic_mode == 3) // long exposure
		//~ silent_pic_take_longexp();
	else if (silent_pic_mode == 4) // slit-scan
		silent_pic_take_slitscan(interactive);

	idle_globaldraw_en();
}


static void 
iso_display( void * priv, int x, int y, int selected )
{
	int fnt = selected ? MENU_FONT_SEL : MENU_FONT;
	bmp_printf(
		fnt,
		x, y,
		"ISO         : %s", 
		lens_info.iso ? "" : "Auto"
	);

	bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");

	fnt = FONT(
		fnt, 
		is_native_iso(lens_info.iso) ? COLOR_YELLOW :
		is_lowgain_iso(lens_info.iso) ? COLOR_GREEN2 : FONT_FG(fnt),
		FONT_BG(fnt));

	if (lens_info.iso)
	{
		bmp_printf(
			fnt,
			x + 14 * font_large.width, y,
			"%d", lens_info.iso
		);
	}
	menu_draw_icon(x, y, lens_info.iso ? MNI_PERCENT : MNI_AUTO, (lens_info.raw_iso - codes_iso[1]) * 100 / (codes_iso[COUNT(codes_iso)-1] - codes_iso[1]));
}

int is_native_iso(int iso)
{
	switch(iso)
	{
		case 100:
		case 200:
		case 400:
		case 800:
		case 1600:
		case 3200:
		case 6400:
		case 12800:
		case 25600:
			return 1;
	}
	return 0;
}

int is_lowgain_iso(int iso)
{
	switch(iso)
	{
		case 160:
		case 320:
		case 640:
		case 1250:
		case 2500:
		case 5000:
		return 1;
	}
	return 0;
}

int is_round_iso(int iso)
{
	return is_native_iso(iso) || is_lowgain_iso(iso) || iso == 0;
}

CONFIG_INT("iso.round.only", iso_round_only, 1);


void
iso_toggle( int sign )
{
	int i = raw2index_iso(lens_info.raw_iso);
	int k;
	for (k = 0; k < 10; k++)
	{
		i = mod(i + sign, COUNT(codes_iso));
		
		while (iso_round_only && !is_round_iso(values_iso[i]))
			i = mod(i + sign, COUNT(codes_iso));
		
		lens_set_rawiso(codes_iso[i]);
		msleep(100);
		int j = raw2index_iso(lens_info.raw_iso);
		if (i == j) break;
	}
	menu_show_only_selected();
}

static void
iso_toggle_forward( void * priv )
{
	iso_toggle(1);
}

static void
iso_toggle_reverse( void * priv )
{
	iso_toggle(-1);
}

/*PROP_INT(PROP_ISO_AUTO, iso_auto_code);
static int measure_auto_iso()
{
	// temporary changes during measurement:
	// * max auto iso => 12800
	// * iso: 800 => 2 or 3 stops down, 3 or 4 stops up
	// * AE shift to keep the same exposure
	uint16_t ma = max_auto_iso;
	uint16_t ma0 = (ma & 0xFF00) | 0x80;
	
	int is0 = lens_info.raw_iso;
	int ae0 = lens_info.ae;
	int dif = 0x60 - is0;
	lens_set_rawiso(is0 + dif); // = 0x60 = ISO 800
	lens_set_ae(ae0 - dif);
	
	prop_request_change(PROP_MAX_AUTO_ISO, &ma0, 2);
	
	int iso_auto_mode = 0;
	prop_request_change(PROP_ISO, &iso_auto_mode, 4);   // force iso auto
	msleep(500);
	while (iso_auto_code == 0) // force metering event
	{
		SW1(1,100);
		SW1(0,100);
	}
	
	int ans = iso_auto_code;
	
	// restore stuff back
	prop_request_change(PROP_MAX_AUTO_ISO, &ma, 2);
	lens_set_rawiso(is0);
	lens_set_ae(ae0);
	
	return ans;
}*/

static int measure_auto_iso()
{
	SW1(1,10); // trigger metering event
	SW1(0,100);
	return COERCE(lens_info.raw_iso - AE_VALUE, 72, 128);
}

static void iso_auto_quick()
{
	//~ if (MENU_MODE) return;
	int new_iso = measure_auto_iso();
	lens_set_rawiso(new_iso);
}

int iso_auto_flag = 0;
static void iso_auto()
{
	if (lv) iso_auto_flag = 1; // it takes some time, so it's better to do it in another task
	else 
	{
		iso_auto_quick();
		iso_auto_quick(); // sometimes it gets better result the second time
	}
}
void get_under_and_over_exposure_autothr(int* under, int* over)
{
	int thr_lo = 0;
	int thr_hi = 255;
	*under = 0;
	*over = 0;
	while (*under < 50 && *over < 50 && thr_lo < thr_hi)
	{
		thr_lo += 10;
		thr_hi -= 10;
		get_under_and_over_exposure(thr_lo, thr_hi, under, over);
	}
}

int crit_iso(int iso_index)
{
	if (!lv) return 0;

	if (iso_index >= 0)
	{
		lens_set_rawiso(codes_iso[iso_index]);
		msleep(750);
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
	BMP_LOCK( draw_ml_bottombar(0); )
	return under - over;
}

static void iso_auto_run()
{
	menu_stop();
	if (lens_info.raw_iso == 0) { lens_set_rawiso(96); msleep(500); }
	int c0 = crit_iso(-1); // test current iso
	int i;
	if (c0 > 0) i = bin_search(raw2index_iso(lens_info.raw_iso), COUNT(codes_iso), crit_iso);
	else i = bin_search(get_htp() ? 9 : 1, raw2index_iso(lens_info.raw_iso)+1, crit_iso);
	lens_set_rawiso(codes_iso[i]);
	redraw();
}


static void 
shutter_display( void * priv, int x, int y, int selected )
{
	char msg[100];
	if (is_movie_mode())
	{
		snprintf(msg, sizeof(msg),
			"Shutter     : 1/%d, %d",
			lens_info.shutter, 
			360 * video_mode_fps / lens_info.shutter);
	}
	else
	{
		snprintf(msg, sizeof(msg),
			"Shutter     : 1/%d",
			lens_info.shutter
		);
	}
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		msg
	);
	if (is_movie_mode())
	{
		int xc = x + font_large.width * strlen(msg);
		draw_circle(xc + 2, y + 7, 3, COLOR_WHITE);
		draw_circle(xc + 2, y + 7, 4, COLOR_WHITE);
	}
	bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
	menu_draw_icon(x, y, lens_info.raw_shutter ? MNI_PERCENT : MNI_WARNING, lens_info.raw_shutter ? (lens_info.raw_shutter - codes_shutter[1]) * 100 / (codes_shutter[COUNT(codes_shutter)-1] - codes_shutter[1]) : 0);
}

static void
shutter_toggle( int sign)
{
	int i = raw2index_shutter(lens_info.raw_shutter);
	int k;
	for (k = 0; k < 10; k++)
	{
		i = mod(i + sign, COUNT(codes_shutter));
		lens_set_rawshutter(codes_shutter[i]);
		msleep(10);
		int j = raw2index_shutter(lens_info.raw_shutter);
		if (i == j) break;
	}
	menu_show_only_selected();
}

static void
shutter_toggle_forward( void * priv )
{
	shutter_toggle(1);
}

static void
shutter_toggle_reverse( void * priv )
{
	shutter_toggle(-1);
}

static void shutter_auto_quick()
{
	if (MENU_MODE) return;
	if (lens_info.raw_iso == 0) return;                  // does not work on Auto ISO
	int ciso = lens_info.raw_iso;
	int steps = measure_auto_iso() - ciso;              // read delta exposure and compute new shutter value
	int newshutter = COERCE(lens_info.raw_shutter - steps, 96, 152);
	lens_set_rawiso(ciso);                                 // restore iso
	lens_set_rawshutter(newshutter);                       // set new shutter value
}

int shutter_auto_flag = 0;
static void shutter_auto()
{
	if (lv) shutter_auto_flag = 1; // it takes some time, so it's better to do it in another task
	else 
	{
		shutter_auto_quick();
		shutter_auto_quick();
	}
}

int crit_shutter(int shutter_index)
{
	if (!lv) return 0;

	if (shutter_index >= 0)
	{
		lens_set_rawshutter(codes_shutter[shutter_index]);
		msleep(750);
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
	BMP_LOCK( draw_ml_bottombar(0); )
	return over - under;
}

static void shutter_auto_run()
{
	menu_stop();
	int c0 = crit_shutter(-1); // test current shutter
	int i;
	if (c0 > 0) i = bin_search(raw2index_shutter(lens_info.raw_shutter), COUNT(codes_shutter), crit_shutter);
	else i = bin_search(1, raw2index_shutter(lens_info.raw_shutter)+1, crit_shutter);
	lens_set_rawshutter(codes_shutter[i]);
	redraw();
}

static void 
aperture_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Aperture    : f/%d.%d",
		lens_info.aperture / 10,
		lens_info.aperture % 10
	);
	menu_draw_icon(x, y, lens_info.aperture ? MNI_PERCENT : MNI_WARNING, lens_info.aperture ? (lens_info.raw_aperture - codes_aperture[1]) * 100 / (codes_shutter[COUNT(codes_aperture)-1] - codes_aperture[1]) : 0);
}

static void
aperture_toggle( int sign)
{
	int amin = codes_aperture[1];
	int amax = codes_aperture[COUNT(codes_aperture)-1];
	
	int a = lens_info.raw_aperture;
	int a0 = a;

	int k;
	for (k = 0; k < 20; k++)
	{
		a += sign;
		if (a > amax) a = amin;
		if (a < amin) a = amax;

		lens_set_rawaperture(a);
		msleep(100);
		if (lens_info.raw_aperture != a0) break;
	}
	menu_show_only_selected();
}

static void
aperture_toggle_forward( void * priv )
{
	aperture_toggle(1);
}

static void
aperture_toggle_reverse( void * priv )
{
	aperture_toggle(-1);
}


void
kelvin_toggle( int sign )
{
	int k;
	switch (lens_info.wb_mode)
	{
		case WB_SUNNY: k = 5200; break;
		case WB_SHADE: k = 7000; break;
		case WB_CLOUDY: k = 6000; break;
		case WB_TUNGSTEN: k = 3200; break;
		case WB_FLUORESCENT: k = 4000; break;
		case WB_FLASH: k = 6500; break; // maybe?
		default: k = lens_info.kelvin;
	}
	k = (k/KELVIN_STEP) * KELVIN_STEP;
	k = KELVIN_MIN + mod(k - KELVIN_MIN + sign * KELVIN_STEP, KELVIN_MAX - KELVIN_MIN + KELVIN_STEP);
	lens_set_kelvin(k);
	menu_show_only_selected();
}

static void
kelvin_toggle_forward( void * priv )
{
	kelvin_toggle(1);
}

static void
kelvin_toggle_reverse( void * priv )
{
	kelvin_toggle(-1);
}

PROP_INT( PROP_WB_KELVIN_PH, wb_kelvin_ph );

static void 
kelvin_display( void * priv, int x, int y, int selected )
{
	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WhiteBalance: %dK%s",
			lens_info.kelvin,
			lens_info.kelvin == wb_kelvin_ph ? "" : "*"
		);
		menu_draw_icon(x, y, MNI_PERCENT, (lens_info.kelvin - KELVIN_MIN) * 100 / (KELVIN_MAX - KELVIN_MIN));
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WhiteBalance: %s",
			(lens_info.wb_mode == 0 ? "Auto" : 
			(lens_info.wb_mode == 1 ? "Sunny" :
			(lens_info.wb_mode == 2 ? "Cloudy" : 
			(lens_info.wb_mode == 3 ? "Tungsten" : 
			(lens_info.wb_mode == 4 ? "CFL" : 
			(lens_info.wb_mode == 5 ? "Flash" : 
			(lens_info.wb_mode == 6 ? "Custom" : 
			(lens_info.wb_mode == 8 ? "Shade" :
			 "unknown"))))))))
		);
		menu_draw_icon(x, y, MNI_AUTO, 0);
	}
	bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
}

int kelvin_auto_flag = 0;
int wbs_gm_auto_flag = 0;
static void kelvin_auto()
{
	if (lv) kelvin_auto_flag = 1;
	else
	{
		NotifyBox(2000, "Auto WB only works in LiveView");
	}
}

static void wbs_gm_auto()
{
	if (lv) wbs_gm_auto_flag = 1;
	else
	{
		NotifyBox(2000, "Auto WBS only works in LiveView");
	}
}

int crit_kelvin(int k)
{
	if (!lv) return 0;

	if (k > 0)
	{
		lens_set_kelvin(k * KELVIN_STEP);
		msleep(750);
	}

	int Y, U, V;
	get_spot_yuv(100, &Y, &U, &V);
	BMP_LOCK( draw_ml_bottombar(0); )

	int R = Y + 1437 * V / 1024;
	//~ int G = Y -  352 * U / 1024 - 731 * V / 1024;
	int B = Y + 1812 * U / 1024;

	return B - R;
}

int crit_wbs_gm(int k)
{
	if (!lv) return 0;

	k = COERCE(k, -10, 10);
	lens_set_wbs_gm(k);
	msleep(750);

	int Y, U, V;
	get_spot_yuv(100, &Y, &U, &V);

	int R = Y + 1437 * V / 1024;
	int G = Y -  352 * U / 1024 - 731 * V / 1024;
	int B = Y + 1812 * U / 1024;

	BMP_LOCK( draw_ml_bottombar(0); )
	return (R+B)/2 - G;
}

static void kelvin_auto_run()
{
	menu_stop();
	int c0 = crit_kelvin(-1); // test current kelvin
	int i;
	if (c0 > 0) i = bin_search(lens_info.kelvin/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
	else i = bin_search(KELVIN_MIN/KELVIN_STEP, lens_info.kelvin/KELVIN_STEP + 1, crit_kelvin);
	lens_set_kelvin(i * KELVIN_STEP);
	redraw();
}

static void wbs_gm_auto_run()
{
	menu_stop();
	int c0 = crit_wbs_gm(100); // test current value
	int i;
	if (c0 > 0) i = bin_search(lens_info.wbs_gm, 10, crit_wbs_gm);
	else i = bin_search(-9, lens_info.wbs_gm + 1, crit_wbs_gm);
	lens_set_wbs_gm(i);
	redraw();
}

static void 
wbs_gm_display( void * priv, int x, int y, int selected )
{
		int gm = lens_info.wbs_gm;
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WBShift G/M : %s%d", 
			gm > 0 ? "Green " : (gm < 0 ? "Magenta " : ""), 
			ABS(gm)
		);
		menu_draw_icon(x, y, MNI_PERCENT, (-lens_info.wbs_gm + 9) * 100 / 18);
	bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
}

static void
wbs_gm_toggle( int sign )
{
	int gm = lens_info.wbs_gm;
	int newgm = mod((gm + 9 + sign), 19) - 9;
	newgm = newgm & 0xFF;
	prop_request_change(PROP_WBS_GM, &newgm, 4);
	menu_show_only_selected();
}

static void
wbs_gm_toggle_forward( void * priv )
{
	wbs_gm_toggle(-1);
}

static void
wbs_gm_toggle_reverse( void * priv )
{
	wbs_gm_toggle(1);
}

static void 
wbs_ba_display( void * priv, int x, int y, int selected )
{
		int ba = lens_info.wbs_ba;
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WBShift B/A : %s%d", 
			ba > 0 ? "Amber " : (ba < 0 ? "Blue " : ""), 
			ABS(ba)
		);
		menu_draw_icon(x, y, MNI_PERCENT, (lens_info.wbs_ba + 9) * 100 / 18);
}

static void
wbs_ba_toggle( int sign )
{
	int ba = lens_info.wbs_ba;
	int newba = mod((ba + 9 + sign), 19) - 9;
	newba = newba & 0xFF;
	prop_request_change(PROP_WBS_BA, &newba, 4);
	menu_show_only_selected();
}

static void
wbs_ba_toggle_forward( void * priv )
{
	wbs_ba_toggle(1);
}

static void
wbs_ba_toggle_reverse( void * priv )
{
	wbs_ba_toggle(-1);
}

static void
contrast_toggle( int sign )
{
	int c = lens_get_contrast();
	if (c < -4 || c > 4) return;
	int newc = mod((c + 4 + sign), 9) - 4;
	lens_set_contrast(newc);
	menu_show_only_selected();
}

static void
contrast_toggle_forward( void * priv )
{
	contrast_toggle(1);
}

static void
contrast_toggle_reverse( void * priv )
{
	contrast_toggle(-1);
}

static void 
contrast_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Contrast    : %d ",
		lens_get_contrast()
	);
	menu_draw_icon(x, y, MNI_PERCENT, (lens_get_contrast() + 4) * 100 / 8);
}

static void
sharpness_toggle( int sign )
{
	int c = lens_get_sharpness();
	if (c < 0 || c > 7) return;
	int newc = mod(c + sign, 8);
	lens_set_sharpness(newc);
	menu_show_only_selected();
}

static void
sharpness_toggle_forward( void * priv )
{
	sharpness_toggle(1);
}

static void
sharpness_toggle_reverse( void * priv )
{
	sharpness_toggle(-1);
}

static void 
sharpness_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Sharpness   : %d ",
		lens_get_sharpness()
	);
	menu_draw_icon(x, y, MNI_PERCENT, lens_get_sharpness() * 100 / 7);
}

static void
saturation_toggle( int sign )
{
	int c = lens_get_saturation();
	if (c < -4 || c > 4) return;
	int newc = mod((c + 4 + sign), 9) - 4;
	lens_set_saturation(newc);
	menu_show_only_selected();
}

static void
saturation_toggle_forward( void * priv )
{
	saturation_toggle(1);
}

static void
saturation_toggle_reverse( void * priv )
{
	saturation_toggle(-1);
}

static void 
saturation_display( void * priv, int x, int y, int selected )
{
	int s = lens_get_saturation();
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		(s >= -4 && s <= 4) ? 
			"Saturation  : %d " :
			"Saturation  : N/A",
		s
	);
	if (s >= -4 && s <= 4) menu_draw_icon(x, y, MNI_PERCENT, (s + 4) * 100 / 8);
	else menu_draw_icon(x, y, MNI_WARNING, 0);
}

static CONFIG_INT("picstyle.rec", picstyle_rec, 0);
int picstyle_before_rec = 0; // if you use a custom picstyle during REC, the old one will be saved here

const char* get_picstyle_name(int raw_picstyle)
{
	return
		raw_picstyle == 0x81 ? "Standard" : 
		raw_picstyle == 0x82 ? "Portrait" :
		raw_picstyle == 0x83 ? "Landscape" :
		raw_picstyle == 0x84 ? "Neutral" :
		raw_picstyle == 0x85 ? "Faithful" :
		raw_picstyle == 0x86 ? "Monochrom" :
		raw_picstyle == 0x87 ? "Auto" :
		raw_picstyle == 0x21 ? "UserDef1" :
		raw_picstyle == 0x22 ? "UserDef2" :
		raw_picstyle == 0x23 ? "UserDef3" : "Unknown";
}

const char* get_picstyle_shortname(int raw_picstyle)
{
	return
		raw_picstyle == 0x81 ? "Std." : 
		raw_picstyle == 0x82 ? "Port." :
		raw_picstyle == 0x83 ? "Land." :
		raw_picstyle == 0x84 ? "Neut." :
		raw_picstyle == 0x85 ? "Fait." :
		raw_picstyle == 0x86 ? "Mono." :
		raw_picstyle == 0x87 ? "Auto" :
		raw_picstyle == 0x21 ? "User1" :
		raw_picstyle == 0x22 ? "User2" :
		raw_picstyle == 0x23 ? "User3" : "Unk.";
}
static void 
picstyle_display( void * priv, int x, int y, int selected )
{
	int p = get_prop_picstyle_from_index(picstyle_rec && recording ? picstyle_before_rec : (int)lens_info.picstyle);
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"PictureStyle: %s%s(%d,%d,%d,%d)",
		get_picstyle_name(p),
		picstyle_before_rec ? "*" : " ",
		lens_get_sharpness(),
		lens_get_contrast(),
		ABS(lens_get_saturation()) < 10 ? lens_get_saturation() : 0,
		ABS(lens_get_color_tone()) < 10 ? lens_get_color_tone() : 0
	);
	menu_draw_icon(x, y, MNI_ON, 0);
}

static void
picstyle_toggle( int sign )
{
	if (recording) return;
	int p = lens_info.picstyle;
	p = mod(p + sign - 1, NUM_PICSTYLES) + 1;
	if (p)
	{
		p = get_prop_picstyle_from_index(p);
		prop_request_change(PROP_PICTURE_STYLE, &p, 4);
	}
	menu_show_only_selected();
}

static void
picstyle_toggle_forward( void * priv )
{
	picstyle_toggle(1);
}

static void
picstyle_toggle_reverse( void * priv )
{
	picstyle_toggle(-1);
}

static void 
picstyle_rec_display( void * priv, int x, int y, int selected )
{
	if (!picstyle_rec)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"REC PicStyle: Don't change"
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"REC PicStyle: %s (%d,%d,%d,%d)",
			get_picstyle_name(get_prop_picstyle_from_index(picstyle_rec)),
			lens_get_from_other_picstyle_sharpness(picstyle_rec),
			lens_get_from_other_picstyle_contrast(picstyle_rec),
			ABS(lens_get_from_other_picstyle_saturation(picstyle_rec)) < 10 ? lens_get_from_other_picstyle_saturation(picstyle_rec) : 0,
			ABS(lens_get_from_other_picstyle_color_tone(picstyle_rec)) < 10 ? lens_get_from_other_picstyle_color_tone(picstyle_rec) : 0
		);
	}
}

static void
picstyle_rec_toggle( void * priv )
{
	if (recording) return;
	picstyle_rec = mod(picstyle_rec + 1, NUM_PICSTYLES + 1);
}

static void
picstyle_rec_toggle_reverse( void * priv )
{
	if (recording) return;
	picstyle_rec = mod(picstyle_rec - 1, NUM_PICSTYLES + 1);
}

void redraw_after_task(int msec)
{
	msleep(msec);
	redraw();
}

void redraw_after(int msec)
{
	task_create("redraw", 0x1d, 0, redraw_after_task, (void*)msec);
}

void rec_picstyle_change(int rec)
{
	static int prev = -1;

	if (picstyle_rec)
	{
		if (prev == 0 && rec) // will start recording
		{
			picstyle_before_rec = lens_info.picstyle;
			int p = get_prop_picstyle_from_index(picstyle_rec);
			if (p)
			{
				NotifyBox(2000, "Picture Style : %s", get_picstyle_name(p));
				prop_request_change(PROP_PICTURE_STYLE, &p, 4);
			}
		}
		else if (prev == 2 && rec == 0) // recording => will stop
		{
			int p = get_prop_picstyle_from_index(picstyle_before_rec);
			if (p)
			{
				NotifyBox(2000, "Picture Style : %s", get_picstyle_name(p));
				prop_request_change(PROP_PICTURE_STYLE, &p, 4);
			}
			picstyle_before_rec = 0;
		}
	}
	prev = rec;
}

#ifdef CONFIG_50D
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
	int rec = (shooting_type == 4 ? 2 : 0);
	rec_picstyle_change(rec);
	shutter_btn_rec_do(rec);
	rec_notify_trigger(rec);
	return prop_cleanup(token, property);
}
#else
PROP_HANDLER(PROP_MVR_REC_START)
{
	int rec = buf[0];
	rec_notify_trigger(rec);
	rec_picstyle_change(rec);
	return prop_cleanup(token, property);
}
#endif


PROP_INT(PROP_STROBO_AECOMP, flash_ae);

static void
flash_ae_toggle( int sign )
{
	int ae = (int8_t)flash_ae;
	int newae = ae + sign * (ABS(ae + sign) <= 24 ? 4 : 8);
	if (newae > FLASH_MAX_EV * 8) newae = FLASH_MIN_EV * 8;
	if (newae < FLASH_MIN_EV * 8) newae = FLASH_MAX_EV * 8;
	ae &= 0xFF;
	prop_request_change(PROP_STROBO_AECOMP, &newae, 4);
}

static void
flash_ae_toggle_forward( void * priv )
{
	flash_ae_toggle(1);
}

static void
flash_ae_toggle_reverse( void * priv )
{
	flash_ae_toggle(-1);
}

static void 
flash_ae_display( void * priv, int x, int y, int selected )
{
	int ae_ev = ((int8_t)flash_ae) * 10 / 8;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Flash AEcomp: %s%d.%d EV",
		ae_ev < 0 ? "-" : "",
		ABS(ae_ev) / 10, 
		ABS(ae_ev % 10)
	);
	menu_draw_icon(x, y, MNI_PERCENT, (ae_ev + 80) * 100 / (24+80));
}

// 0 = off, 1 = alo, 2 = htp
int get_ladj()
{
	int alo = get_alo();
	if (get_htp()) return 4;
	if (alo == ALO_LOW) return 1;
	if (alo == ALO_STD) return 2;
	if (alo == ALO_HIGH) return 3;
	return 0;
}

#ifdef CONFIG_500D
static void
alo_toggle( void * priv )
{
	int alo = get_alo();
	switch (alo)
	{
		case ALO_OFF:
			set_alo(ALO_STD);
			break;
		case ALO_STD:
			set_alo(ALO_LOW);
			break;
		case ALO_LOW:
			set_alo(ALO_HIGH);
			break;
		case ALO_HIGH:
			set_alo(ALO_OFF);
			break;
	}
}

static void
htp_toggle( void * priv )
{
	int htp = get_htp();
	if (htp)
		set_htp(0);
	else
		set_htp(1);
}

#endif

static void
ladj_toggle( int sign )
{
	int ladj = get_ladj();
	ladj = mod(ladj + sign, 5);
	if (ladj == 0)
	{
		set_htp(0);
		set_alo(ALO_OFF);
	}
	else if (ladj == 1)
	{
		set_htp(0);
		set_alo(ALO_LOW);
	}
	else if (ladj == 2)
	{
		set_htp(0);
		set_alo(ALO_STD);
	}
	else if (ladj == 3)
	{
		set_htp(0);
		set_alo(ALO_HIGH);
	}
	else
	{
		set_htp(1); // this disables ALO
	}
	menu_show_only_selected();
}

static void
ladj_toggle_forward( void * priv )
{
	ladj_toggle(1);
}

static void
ladj_toggle_reverse( void * priv )
{
	ladj_toggle(-1);
}

#ifdef CONFIG_500D
static void 
ladj_display( void * priv, int x, int y, int selected )
{
	int htp = get_htp();
	int alo = get_alo();
	bmp_printf(
			   selected ? MENU_FONT_SEL : MENU_FONT,
			   x, y,
			   "HTP/ALO     : %s/%s",
			   (htp ? "ON" : "OFF"),
			   (alo == ALO_STD ? "Standard" :
				alo == ALO_LOW ? "Low" :
				alo == ALO_HIGH ? "Strong" :
				alo == ALO_OFF ? "OFF" : "err")
			   );
	menu_draw_icon(x, y, MNI_BOOL_GDR_EXPSIM(htp || (alo != ALO_OFF)));
}
#else
static void 
ladj_display( void * priv, int x, int y, int selected )
{
	int htp = get_htp();
	int alo = get_alo();
	bmp_printf(
			   selected ? MENU_FONT_SEL : MENU_FONT,
			   x, y,
			   "Light Adjust: %s",
			   (htp ? "HTP" :
				(alo == ALO_STD ? "ALO std" :
				 (alo == ALO_LOW ? "ALO low" : 
				  (alo == ALO_HIGH ? "ALO strong " :
				   (alo == ALO_OFF ? "OFF" : "err")))))
			   );
	menu_draw_icon(x, y, alo != ALO_OFF ? MNI_ON : htp ? MNI_AUTO : MNI_OFF, 0);
}
#endif

static void 
zoom_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LiveView Zoom       : %s%s %s",
		zoom_disable_x5 ? "" : "x5", 
		zoom_disable_x10 ? "" : "x10", 
		zoom_enable_face ? ":-)" : ""
	);
	menu_draw_icon(x, y, MNI_BOOL_LV(zoom_enable_face || zoom_disable_x5 || zoom_disable_x10));
}

static void zoom_toggle(void* priv)
{
	// x5 x10
	// x5
	// x10
	if (!zoom_disable_x5 && !zoom_disable_x10) // both enabled
	{
		zoom_disable_x5 = 0;
		zoom_disable_x10 = 1;
	}
	else if (!zoom_disable_x10)
	{
		zoom_disable_x5 = 0;
		zoom_disable_x10 = 0;
	}
	else
	{
		zoom_disable_x5 = 1;
		zoom_disable_x10 = 0;
	}
}

static void 
hdr_display( void * priv, int x, int y, int selected )
{
	if (hdr_steps == 1)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"HDR Bracketing  : OFF"
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"HDR Bracketing  : %dx%d%sEV",
			hdr_steps, 
			hdr_stepsize / 8,
			((hdr_stepsize/4) % 2) ? ".5" : ""
		);
	}
	menu_draw_icon(x, y, MNI_BOOL(hdr_steps != 1), 0);
}

static void
hdr_steps_toggle( void * priv )
{
	hdr_steps = mod(hdr_steps + (hdr_steps <= 2 ? 0 : 1), 10) + 1;
}

static void
hdr_stepsize_toggle( void * priv )
{
	hdr_stepsize = (hdr_stepsize < 8) ? MAX(hdr_stepsize * 2, 4) : (hdr_stepsize/8)*8 + 8;
	if (hdr_stepsize > 40) hdr_stepsize = 0;
}

static void
hdr_reset( void * priv )
{
	hdr_steps = 1;
	hdr_stepsize = 8;
}

int is_bulb_mode()
{
	if (bulb_ramping_enabled && intervalometer_running) return 1; // this will force bulb mode when needed
	if (shooting_mode == SHOOTMODE_BULB) return 1;
	if (shooting_mode != SHOOTMODE_M) return 0;
	if (lens_info.raw_shutter != 0xC) return 0;
	return 1;
}

void ensure_bulb_mode()
{
	lens_wait_readytotakepic(64);
	#ifdef CONFIG_60D
	int a = lens_info.raw_aperture;
	set_shooting_mode(SHOOTMODE_BULB);
	lens_set_rawaperture(a);
	#else
	if (shooting_mode != SHOOTMODE_M)
		set_shooting_mode(SHOOTMODE_M);
	int shutter = 12; // huh?!
	prop_request_change( PROP_SHUTTER, &shutter, 4 );
	prop_request_change( PROP_SHUTTER_ALSO, &shutter, 4 );
	#endif
}

// goes to Bulb mode and takes a pic with the specified duration (ms)
void
bulb_take_pic(int duration)
{
	//~ NotifyBox(2000,  "Bulb: %d ", duration); msleep(2000);
	duration = MAX(duration, BULB_MIN_EXPOSURE) + BULB_EXPOSURE_CORRECTION;
	int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
	int m0r = shooting_mode;
	ensure_bulb_mode();
	
	//~ #ifdef CONFIG_600D
	assign_af_button_to_star_button();
	//~ #endif
	
	msleep(100);
	if (beep_enabled) beep();
	
	int d0 = drive_mode;
	lens_set_drivemode(DRIVE_SINGLE);
	//~ NotifyBox(3000, "BulbStart (%d)", duration); msleep(1000);
	mlu_lock_mirror_if_needed();
	//~ SW1(1,50);
	//~ SW2(1,0);
	
	wait_till_next_second();
	
	//~ int x = 0;
	//~ prop_request_change(PROP_REMOTE_BULB_RELEASE_START, &x, 4);
	SW1(1,0);
	SW2(1,0);
	
	//~ msleep(duration);
	int d = duration/1000;
	for (int i = 0; i < d; i++)
	{
		bmp_printf(FONT_LARGE, 30, 30, "Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
		wait_till_next_second();
		if (lens_info.job_state == 0) break;
	}
	msleep(duration % 1000);
	//~ prop_request_change(PROP_REMOTE_BULB_RELEASE_END, &x, 4);
	//~ NotifyBox(3000, "BulbEnd");
	SW2(0,0);
	SW1(0,0);
	//~ msleep(100);
	//~ #ifdef CONFIG_600D
	lens_wait_readytotakepic(64);
	if (beep_enabled) beep();
	restore_af_button_assignment();
	//~ #endif
	get_out_of_play_mode(1000);
	lens_set_drivemode(d0);
	prop_request_change( PROP_SHUTTER, &s0r, 4 );
	prop_request_change( PROP_SHUTTER_ALSO, &s0r, 4);
	set_shooting_mode(m0r);
	msleep(200);
}

static void bulb_toggle_fwd(void* priv)
{
	bulb_duration_index = mod(bulb_duration_index + 1, COUNT(timer_values));
	bulb_shutter_value = timer_values[bulb_duration_index] * 1000;
}
static void bulb_toggle_rev(void* priv)
{
	bulb_duration_index = mod(bulb_duration_index - 1, COUNT(timer_values));
	bulb_shutter_value = timer_values[bulb_duration_index] * 1000;
}

static void
bulb_display( void * priv, int x, int y, int selected )
{
	int d = bulb_shutter_value/1000;
	if (!bulb_duration_index) d = 0;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Bulb Timer      : %d%s",
		d < 60 ? d : d/60, 
		bulb_duration_index == 0 ? " (OFF)" : d < 60 ? "s" : "min"
	);
	menu_draw_icon(x, y, !bulb_duration_index ? MNI_OFF : is_bulb_mode() ? MNI_PERCENT : MNI_WARNING, is_bulb_mode() ? (intptr_t)( bulb_duration_index * 100 / COUNT(timer_values)) : (intptr_t) "Bulb timer only works in BULB mode");
	if (selected && is_bulb_mode()) timelapse_calc_display(&interval_timer_index, x - font_large.width*2, y + font_large.height * 6, selected);
}

// like expsim_toggle
static void
mlu_toggle( void * priv )
{
	// off, on, auto
	if (!mlu_auto && !get_mlu()) // off->on
	{
		set_mlu(1);
	}
	else if (!mlu_auto && get_mlu()) // on->auto
	{
		mlu_auto = 1;
	}
	else // auto->off
	{
		mlu_auto = 0;
		set_mlu(0);
	}
}

static void
mlu_display( void * priv, int x, int y, int selected )
{
	//~ int d = timer_values[bulb_duration_index];
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Mirror Lockup   : %s",
		#if defined(CONFIG_550D) || defined(CONFIG_500D)
		mlu_auto ? "Timer+Remote"
		#else
		mlu_auto ? "Self-timer only"
		#endif
		: get_mlu() ? "ON" : "OFF"
	);
	if (get_mlu() && lv) menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Mirror Lockup does not work in LiveView");
	else menu_draw_icon(x, y, mlu_auto ? MNI_AUTO : MNI_BOOL(get_mlu()), 0);
}

#if 0
static void
picq_display( void * priv, int x, int y, int selected )
{
	int raw = pic_quality & 0x60000;
	int rawsize = pic_quality & 0xF;
	int jpegtype = pic_quality >> 24;
	int jpegsize = (pic_quality >> 8) & 0xF;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Picture Quality : %s%s%s%s%s",
		rawsize == 1 ? "M" : rawsize == 2 ? "S" : "",
		raw ? "RAW" : "",
		jpegtype != 4 && raw ? "+" : "",
		jpegtype == 4 ? "" : jpegsize == 0 ? "Large" : jpegsize == 1 ? "Med" : "Small",
		jpegtype == 2 ? "Coarse" : jpegtype == 3 ? "Fine" : ""
	);
	menu_draw_icon(x, y, MNI_ON, 0);
}

static void picq_toggle_rawsize(void* priv)
{
	int p = pic_quality;
	int r = p & 0xF;
	r = mod(r+1, 3);
	int newp = (p & 0xfffffff0) | r;
	set_pic_quality(newp);
}

static void picq_toggle_raw_on_off(void* priv)
{
	int raw = pic_quality & 0x60000;
	int newp;
	if (raw)
	{
		int jt = (pic_quality >> 24) & 0xF;
		if (jt == 4) newp = PICQ_LARGE_FINE;
		else newp = (pic_quality & 0xf0f1fff0) | (jt << 24);
	}
	else newp = pic_quality | 0x60000;
	console_printf("%x\n", newp);
	set_pic_quality(newp);
}

static void picq_toggle_raw(void* priv)
{
	int raw = pic_quality & 0x60000;
	int rsize = pic_quality & 0xF;
	if (raw && rsize < 2) picq_toggle_rawsize(0);
	else picq_toggle_raw_on_off(0);
}

static void picq_toggle_jpegsize(void* priv)
{
	int js = (pic_quality >> 8) & 0xF;
	js = mod(js+1, 3);
	int newp = (pic_quality & 0xfffff0ff) | (js << 8);
	set_pic_quality(newp);
}

static void picq_toggle_jpegtype(void* priv)
{
	int jt = (pic_quality >> 24) & 0xF;
	jt = mod(jt-1, 3) + 2;
	int newp = (pic_quality & 0xf0ffffff) | (jt << 24);
	int raw = pic_quality & 0x60000;
	int rawsize = pic_quality & 0xF;
	if (jt == 4) newp = PICQ_RAW + rawsize;
	set_pic_quality(newp);
}

static int picq_next(int p)
{
	switch(pic_quality)
	{
		case PICQ_RAW: return PICQ_MRAW;
		case PICQ_MRAW: return PICQ_SRAW;
		case PICQ_SRAW: return PICQ_RAW_JPG_LARGE_FINE;
		case PICQ_RAW_JPG_LARGE_FINE: return PICQ_MRAW_JPG_LARGE_FINE;
		case PICQ_MRAW_JPG_LARGE_FINE: return PICQ_SRAW_JPG_LARGE_FINE;
		case PICQ_SRAW_JPG_LARGE_FINE: return PICQ_SRAW_JPG_MED_FINE;
		case PICQ_SRAW_JPG_MED_FINE: return PICQ_SRAW_JPG_SMALL_FINE;
		case PICQ_SRAW_JPG_SMALL_FINE: return PICQ_LARGE_FINE;
		case PICQ_LARGE_FINE: return PICQ_MED_FINE;
		case PICQ_MED_FINE: return PICQ_SMALL_FINE;
	}
	return PICQ_RAW;
}

static void picq_toggle(void* priv)
{
	int newp = picq_next(pic_quality);
	set_pic_quality(newp);
}
#endif

static int bulb_ramping_adjust_iso_180_rule_without_changing_exposure(int intervalometer_delay)
{
	int raw_shutter_0 = shutter_ms_to_raw(bulb_shutter_value);
	int raw_iso_0 = lens_info.raw_iso;
	
	int ideal_shutter_speed_ms = intervalometer_delay * 1000 / 2; // 180 degree rule => ideal value
	int ideal_shutter_speed_raw = shutter_ms_to_raw(ideal_shutter_speed_ms);

	int delta = 0;  // between 90 and 180 degrees => OK

	if (ideal_shutter_speed_raw > raw_shutter_0 + 4)
		delta = 8; // shutter too slow (more than 270 degrees -- ideal value) => boost ISO

	if (ideal_shutter_speed_raw < raw_shutter_0 - 8)
		delta = -8; // shutter too fast (less than 90 degrees) => lower ISO
	
	if (delta) // should we change something?
	{
		int max_auto_iso = auto_iso_range & 0xFF;
		int new_raw_iso = COERCE(lens_info.raw_iso + delta, get_htp() ? 78 : 72, max_auto_iso); // Allowed values: ISO 100 (or 200 with HTP) ... max auto ISO from Canon menu
		delta = new_raw_iso - raw_iso_0;
		if (delta == 0) return 0; // nothing to change
		int new_bulb_shutter = 
			delta ==  8 ? bulb_shutter_value / 2 :
			delta == -8 ? bulb_shutter_value * 2 :
			bulb_shutter_value;
		
		lens_set_rawiso(new_raw_iso); // try to set new iso
		msleep(50);
		if (lens_info.raw_iso == new_raw_iso) // new iso accepted
		{
			bulb_shutter_value = new_bulb_shutter;
			return 1;
		}
		// if we are here, either iso was refused
		// => restore old iso, just to be sure
		lens_set_rawiso(raw_iso_0); 
	}
	return 0; // nothing changed
}

static int bramp_init_state = 0;
static int bramp_init_done = 0;
static int bramp_reference_level = 0;
static int bramp_measured_level = 0;
//~ int bramp_level_ev_ratio = 0;
static int bramp_hist_dirty = 0;

static int seconds_clock = 0;
static void
seconds_clock_task( void* unused )
{
	while(1)
	{
		wait_till_next_second();
		seconds_clock++;

		if (bulb_ramping_enabled && intervalometer_running && !gui_menu_shown())
			bulb_ramping_showinfo();

		if (intervalometer_running && lens_info.job_state == 0 && !gui_menu_shown())
			card_led_blink(1, 50, 0);
	}
}
TASK_CREATE( "seconds_clock_task", seconds_clock_task, 0, 0x19, 0x1000 );

static int measure_brightness_level(int initial_wait)
{
	if (!PLAY_MODE)
	{
		fake_simple_button(BGMT_PLAY);
		while (!PLAY_MODE) msleep(100);
		bramp_hist_dirty = 1;
	}
	msleep(initial_wait);

	if (bramp_hist_dirty)
	{
		struct vram_info * vram = get_yuv422_vram();
		hist_build(vram->vram, vram->width, vram->pitch);
		bramp_hist_dirty = 0;
	}
	int ans = hist_get_percentile_level(bramp_percentile);
	//~ get_out_of_play_mode(500);
	return ans;
}

static void bramp_change_percentile(int dir)
{
	bramp_percentile = COERCE(bramp_percentile + dir * 5, 5, 95);
	
	int i;
	for (i = 0; i <= 20; i++)
	{
		bramp_reference_level = measure_brightness_level(0); // at bramp_percentile
		if (bramp_reference_level > 90) bramp_percentile = COERCE(bramp_percentile - 5, 5, 95);
		else if (bramp_reference_level < 10) bramp_percentile = COERCE(bramp_percentile + 5, 5, 95);
		else break;
	}
	if (i >= 20) { NotifyBox(1000, "Image not properly exposed"); return; }

	int level_8bit = bramp_reference_level * 255 / 100;
	int level_8bit_plus = level_8bit + 5; //hist_get_percentile_level(bramp_percentile + 5) * 255 / 100;
	int level_8bit_minus = level_8bit - 5; //hist_get_percentile_level(bramp_percentile - 5) * 255 / 100;
	clrscr();
	highlight_luma_range(level_8bit_minus, level_8bit_plus, COLOR_BLUE, COLOR_WHITE);
	hist_highlight(level_8bit);
	bmp_printf(FONT_LARGE, 50, 420, 
		"%d%% brightness at %dth percentile\n",
		bramp_reference_level, 0,
		bramp_percentile);
}

int handle_bulb_ramping_keys(struct event * event)
{
	if (intervalometer_running && bramp_init_state)
	{
		switch (event->param)
		{
			case BGMT_PRESS_SET:
			{
				bramp_init_state = 0; // OK :)
				NotifyBox(1000, "OK");
				return 1;
			}
			case BGMT_WHEEL_LEFT:
			case BGMT_WHEEL_RIGHT:
			{
				int dir = event->param == BGMT_WHEEL_LEFT ? -1 : 1;
				bramp_change_percentile(dir);
				NotifyBoxHide();
				return 0;
			}
		}
	}
	
	// test interpolation on luma-ev curve
	//~ for (int i = 0; i < 255; i += 5)
		//~ bramp_plot_luma_ev_point(i, COLOR_GREEN1);

	return 1;
}

static int crit_dispgain_50(int gain)
{
	if (!lv) return 0;

	set_display_gain(gain);
	msleep(500);
	
	int Y,U,V;
	get_spot_yuv(200, &Y, &U, &V);
	NotifyBox(1000, "Gain=%d => Luma=%d ", gain, Y);
	return 128 - Y;
}


static int bramp_luma_ev[11];

static void bramp_plot_luma_ev()
{
	for (int i = -5; i < 5; i++)
	{
		int luma1 = bramp_luma_ev[i+5];
		int luma2 = bramp_luma_ev[i+6];
		int x1 = 320 + i * 20;
		int x2 = 320 + (i+1) * 20;
		int y1 = 200 - (luma1-128)/2;
		int y2 = 200 - (luma2-128)/2;
		draw_line(x1, y1, x2, y2, COLOR_RED);
		draw_line(x1, y1+1, x2, y2+1, COLOR_RED);
		draw_line(x1, y1+2, x2, y2+2, COLOR_WHITE);
		draw_line(x1, y1-1, x2, y2-1, COLOR_WHITE);
	}
	int x1 = 320 - 5 * 20;
	int x2 = 320 + 5 * 20;
	int y1 = 200 - 128/2;
	int y2 = 200 + 128/2;
	bmp_draw_rect(COLOR_WHITE, x1, y1, x2-x1, y2-y1);
}

static int bramp_luma_to_ev_x100(int luma)
{
	int i;
	for (i = -5; i < 5; i++)
		if (luma <= bramp_luma_ev[i+5]) break;
	i = COERCE(i-1, -5, 4);
	// now, luma is between luma1 and luma2
	// EV correction is between i EV and (i+1) EV => linear approximation
	int luma1 = bramp_luma_ev[i+5];
	int luma2 = bramp_luma_ev[i+6];
	int k = (luma-luma1) * 1000 / (luma2-luma1);
	//~ return i * 100;
	int ev_x100 = ((1000-k) * i + k * (i+1))/10;
	//~ NotifyBox(1000, "%d,%d=>%d", luma, i, ev_x100);
	return ev_x100;
}

static void bramp_plot_luma_ev_point(int luma, int color)
{
	luma = COERCE(luma, 0, 255);
	int ev = bramp_luma_to_ev_x100(luma);
	ev = COERCE(ev, -500, 500);
	int x = 320 + ev * 20 / 100;
	int y = 200 - (luma-128)/2;
	for (int r = 0; r < 5; r++)
	{
		draw_circle(x, y, r, color);
		draw_circle(x+1, y, r, color);
	}
	draw_circle(x, y, 6, COLOR_WHITE);
}

void bulb_ramping_init()
{
	if (bramp_init_done) return;

	bulb_duration_index = 0; // disable bulb timer to avoid interference

	NotifyBox(100000, "Calibration...");
	
	set_shooting_mode(SHOOTMODE_P);
	msleep(1000);
	lens_set_rawiso(0);
	if (!lv) force_liveview();
	msleep(2000);
	int zoom = 10;
	prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);

calib_start:
	SW1(1,50); // reset power management timers
	SW1(0,50);
	lens_set_ae(0);
	int gain0 = bin_search(128, 2500, crit_dispgain_50);
	set_display_gain(gain0);
	msleep(500);
	int Y,U,V;
	get_spot_yuv(200, &Y, &U, &V);
	if (ABS(Y-128) > 1) 
	{
		NotifyBox(1000, "Scene not static, or maybe  \n"
	                    "too dark/bright, retrying..."); 

		zoom = zoom == 10 ? 5 : zoom == 5 ? 1 : 10;
		prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
	    
	    goto calib_start;
	}
	
	for (int i = -5; i <= 5; i++)
	{
		set_display_gain(gain0 * (1 << (i+10)) / 1024);
		//~ lens_set_ae(i*4);
		msleep(500);
		get_spot_yuv(200, &Y, &U, &V);
		NotifyBox(500, "%d EV => luma=%d  ", i, Y);
		if (i == 0) // here, luma should be 128
		{
			if (ABS(Y-128) > 1) {NotifyBox(1000, "Scene not static, retrying..."); goto calib_start;}
			else Y = 128;
		}
		if (i > -5 && Y < bramp_luma_ev[i+5-1]) {NotifyBox(1000, "Scene not static, retrying..."); goto calib_start;}
		bramp_luma_ev[i+5] = Y;
		bramp_plot_luma_ev();
		//~ set_display_gain(1<<i);
	}
	
	// final check
	set_display_gain(gain0);
	msleep(2000);
	get_spot_yuv(200, &Y, &U, &V);
	if (ABS(Y-128) > 1) {NotifyBox(1000, "Scene not static, retrying..."); goto calib_start;}

	// calibration accepted :)
	set_display_gain(0);
	lens_set_ae(0);
#ifdef CONFIG_500D
	fake_simple_button(BGMT_Q);
#else
	fake_simple_button(BGMT_LV);
#endif
	msleep(500);
	fake_simple_button(BGMT_PLAY);
	msleep(1000);
	
	if (!PLAY_MODE) { NotifyBox(1000, "BRamp: could not go to PLAY mode"); msleep(2000); intervalometer_stop(); return; }
	
	//~ bramp_level_ev_ratio = 0;
	bramp_measured_level = 0;
	
	bramp_init_state = 1;
	NotifyBox(100000, "Choose a well-exposed photo  \n"
	                  "and tonal range to meter for.\n"
	                  "Keys: arrows, main dial, SET.");
	
	msleep(200);
	bramp_hist_dirty = 1;
	bramp_change_percentile(0); // show current selection;
	
	void* play_buf = get_yuv422_vram()->vram;
	while (PLAY_MODE && bramp_init_state == 1)
	{
		if (get_yuv422_vram()->vram != play_buf) // another image selected
		{
			bramp_hist_dirty = 1;
			bramp_change_percentile(0); // update current selection
			play_buf = get_yuv422_vram()->vram;
		}
		msleep(100);
	}
	if (!PLAY_MODE) { intervalometer_stop(); return; }
	
	bulb_shutter_value = 1000;
	bramp_init_done = 1; // OK :)
	set_shooting_mode(SHOOTMODE_M);
	msleep(1000);
}

static void compute_exposure_for_next_shot()
{
	if (!bramp_init_done) return;
	
	NotifyBoxHide();
	NotifyBox(2000, "Exposure for next shot...");
	//~ msleep(500);
	
	bramp_measured_level = measure_brightness_level(500);
	//~ NotifyBox(1000, "Exposure level: %d ", bramp_measured_level); msleep(1000);
	
	//~ int err = bramp_measured_level - bramp_reference_level;
	//~ if (ABS(err) <= 1) err = 0;
	//~ int correction_ev_x100 = - 100 * err / bramp_level_ev_ratio / 2;
	int correction_ev_x100 = bramp_luma_to_ev_x100(bramp_reference_level*255/100) - bramp_luma_to_ev_x100(bramp_measured_level*255/100);
	NotifyBox(1000, "Exposure difference: %s%d.%02d EV ", correction_ev_x100 < 0 ? "-" : "+", ABS(correction_ev_x100)/100, ABS(correction_ev_x100)%100);
	correction_ev_x100 = correction_ev_x100 * 80 / 100; // do only 80% of the correction
	bulb_shutter_value = bulb_shutter_value * roundf(1000.0*powf(2, correction_ev_x100 / 100.0))/1000;

	msleep(500);

	bulb_ramping_adjust_iso_180_rule_without_changing_exposure(timer_values[interval_timer_index]);
	
	// don't go slower than intervalometer, and reserve 2 seconds just in case
	bulb_shutter_value = COERCE(bulb_shutter_value, 1, 1000 * MAX(2, timer_values[interval_timer_index] - 2));
	
	NotifyBoxHide();
}

void bulb_ramping_showinfo()
{
	int s = bulb_shutter_value;
	bmp_printf(FONT_MED, 50, 350, 
		//~ "Reference level (%2dth prc) :%3d%%    \n"
		//~ "Measured  level (%2dth prc) :%3d%%    \n"
		//~ "Level/EV ratio             :%3d%%/EV \n"
		"ISO     :%5d    \n"
		"Shutter :%3d.%03d s",
		//~ bramp_percentile, bramp_reference_level, 0,
		//~ bramp_percentile, bramp_measured_level, 0,
		//~ bramp_level_ev_ratio, 0,
		lens_info.iso,
		s / 1000, s % 1000);
	
	if (gui_state != GUISTATE_IDLE)
	{
		bramp_plot_luma_ev();
		bramp_plot_luma_ev_point(bramp_measured_level * 255/100, COLOR_RED);
		bramp_plot_luma_ev_point(bramp_reference_level * 255/100, COLOR_BLUE);
	}
}


static void 
bulb_ramping_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Bulb Ramping    : %s", 
		bulb_ramping_enabled ? "ON" : "OFF"
	);
	if (selected) timelapse_calc_display(&interval_timer_index, x - font_large.width*2, y + font_large.height * 7, selected);
}


static struct menu_entry shoot_menus[] = {
	{
		.name = "HDR Bracket",
		.display	= hdr_display,
		.select		= hdr_steps_toggle,
		.select_reverse = hdr_stepsize_toggle,
		.select_auto = hdr_reset,
		.help = "Exposure bracketing, useful for HDR images.",
		.essential = FOR_PHOTO,
	},
	{
		.name = "Take a pic every",
		.priv		= &interval_timer_index,
		.display	= interval_timer_display,
		.select		= interval_timer_toggle,
		.select_reverse	= interval_timer_toggle_reverse,
		.select_auto = interval_movie_duration_toggle,
		.help = "Intervalometer setting: duration between two shots.",
		.essential = FOR_PHOTO,
	},
	{
		.name = "Intervalometer",
		.priv		= &intervalometer_running,
		.select		= menu_binary_toggle,
		.display	= intervalometer_display,
		.help = "Intervalometer. For precise timing, choose NoWait [Q].",
		.essential = FOR_PHOTO,
	},
	{
		.name = "Bulb Ramping",
		.priv		= &bulb_ramping_enabled,
		.select		= menu_binary_toggle,
		.display	= bulb_ramping_display,
		.help = "Automatic bulb ramping for day-to-night timelapse",
	},
	{
		.name = "Bulb Timer",
		.display = bulb_display, 
		.select = bulb_toggle_fwd, 
		.select_reverse = bulb_toggle_rev,
		.select_auto = bulb_toggle_fwd,
		.help = "Bulb timer for very long exposures, useful for astrophotos",
		.essential = FOR_PHOTO,
	},
	#if defined(CONFIG_550D) || defined(CONFIG_500D)
	{
		.name = "LCD Remote Shot",
		.priv		= &lcd_release_running,
		.select		= menu_quaternary_toggle, 
		.select_reverse = menu_quaternary_toggle_reverse,
		.display	= lcd_release_display,
		.help = "Avoid shake using the LCD face sensor as a simple remote.",
		.essential = FOR_PHOTO,
	},
	#endif
	#if !defined(CONFIG_600D) && !defined(CONFIG_50D)
 	{
		.name = "Audio RemoteShot",
		.priv		= &audio_release_running,
		.select		= menu_binary_toggle,
		.display	= audio_release_display,
		.select_auto = audio_release_level_toggle, 
		.select_reverse = audio_release_level_toggle_reverse,
		.help = "Clap your hands or pop a balloon to take a picture.",
		.essential = FOR_PHOTO,
	},
	#endif
	{
		.name = "Motion Detect",
		.priv		= &motion_detect,
		.select		= menu_ternary_toggle, 
		.display	= motion_detect_display,
		.select_auto = motion_release_level_toggle, 
		.select_reverse = motion_release_level_toggle_reverse,
		.help = "LV Motion detection: EXPosure change / frame DIFference.",
		.essential = FOR_PHOTO,
	},
/*	{
		.select		= flash_and_no_flash_toggle,
		.display	= flash_and_no_flash_display,
		.help = "Take odd pictures with flash, even pictures without flash."
	},*/
	{
		.name = "Silent Picture",
		.priv = &silent_pic_mode,
		.select = silent_pic_mode_toggle,
		.select_reverse = silent_pic_toggle_reverse,
		.select_auto = silent_pic_toggle_forward,
		.display = silent_pic_display,
		.help = "Take pics in LiveView without increasing shutter count.",
	},
	{
		.name = "Mirror Lockup",
		.priv = &mlu_auto,
		.display = mlu_display, 
		.select = mlu_toggle,
		.help = "MLU setting can be linked with self-timer and LCD remote.",
		.essential = FOR_PHOTO,
	},
	/*{
		.display = picq_display, 
		.select = picq_toggle_raw,
		.select_reverse = picq_toggle_jpegsize, 
		.select_auto = picq_toggle_jpegtype,
	}
	{
		.display = picq_display, 
		.select = picq_toggle, 
		.help = "Experimental SRAW/MRAW mode. You may get corrupted files."
	}*/
};

static struct menu_entry vid_menus[] = {
	{
		.name = "LiveView Zoom",
		.priv = &zoom_enable_face,
		.select = menu_binary_toggle,
		.select_reverse = zoom_toggle, 
		.display = zoom_display,
		.help = "Disable x5 or x10, or enable zoom during Face Detection :)"
	},
};

static struct menu_entry expo_menus[] = {
	{
		.name = "ISO",
		.display	= iso_display,
		.select		= iso_toggle_forward,
		.select_reverse		= iso_toggle_reverse,
		.select_auto = iso_auto,
		.help = "Adjust ISO in 1/8EV steps. Press [Q] for auto tuning.",
		.essential = FOR_PHOTO | FOR_MOVIE,
	},
	{
		.name = "WhiteBalance",
		.display	= kelvin_display,
		.select		= kelvin_toggle_forward,
		.select_reverse		= kelvin_toggle_reverse,
		.select_auto = kelvin_auto,
		.help = "Adjust Kelvin WB. Press [Q] for auto tuning.",
		.essential = FOR_PHOTO | FOR_MOVIE,
	},
	{
		.name = "WBShift G/M",
		.display = wbs_gm_display, 
		.select = wbs_gm_toggle_forward, 
		.select_reverse = wbs_gm_toggle_reverse,
		.select_auto = wbs_gm_auto,
		.help = "Green-Magenta white balance shift, for fluorescent lights.",
		.essential = FOR_PHOTO | FOR_MOVIE,
	},
	{
		.name = "WBShift B/A",
		.display = wbs_ba_display, 
		.select = wbs_ba_toggle_forward, 
		.select_reverse = wbs_ba_toggle_reverse,
		.help = "Blue-Amber WBShift; 1 unit = 5 mireks on Kelvin axis.",
		.essential = FOR_PHOTO | FOR_MOVIE,
	},
	{
		.name = "Shutter",
		.display	= shutter_display,
		.select		= shutter_toggle_forward,
		.select_reverse		= shutter_toggle_reverse,
		.select_auto = shutter_auto,
		.help = "Shutter in 1/8EV steps. ML shows it with 2 nonzero digits.",
		.essential = FOR_PHOTO | FOR_MOVIE,
	},
	{
		.name = "Aperture",
		.display	= aperture_display,
		.select		= aperture_toggle_forward,
		.select_reverse		= aperture_toggle_reverse,
		.help = "Adjust aperture. Useful if the wheel stops working.",
	},
#ifdef CONFIG_500D
	{
		.name		 = "Light Adjust",
		.select		 = htp_toggle,
		.select_auto = alo_toggle,
		.display	 = ladj_display,
		.help = "Enable/disable HTP and ALO from the same place."
	},
#else
	{
		.name = "Light Adjust",
		.display	= ladj_display,
		.select		= ladj_toggle_forward,
		.select_reverse		= ladj_toggle_reverse,
		.help = "Enable/disable HTP and ALO from the same place."
	},
#endif
	{
		.name = "PictureStyle",
		.display	= picstyle_display,
		.select		= picstyle_toggle_forward,
		.select_reverse		= picstyle_toggle_reverse,
		.help = "Change current picture style.",
		.essential = FOR_MOVIE,
	},
	{
		.priv = &picstyle_rec,
		.name = "REC PicStyle",
		.display	= picstyle_rec_display,
		.select		= picstyle_rec_toggle,
		.select_reverse		= picstyle_rec_toggle_reverse,
		.help = "You can use a different picture style when recording.",
		.essential = FOR_MOVIE,
	},
	{
		.name = "Contrast/Saturation/Sharpness",
		.display	= contrast_display,
		.select		= contrast_toggle_forward,
		.select_reverse		= contrast_toggle_reverse,
		//~ .select_auto = contrast_auto,
		.help = "Adjust contrast in current picture style.",
		.essential = FOR_MOVIE,
	},
	{
		.name = "Contrast/Saturation/Sharpness",
		.display	= saturation_display,
		.select		= saturation_toggle_forward,
		.select_reverse		= saturation_toggle_reverse,
		.help = "Adjust saturation in current picture style.",
		.essential = FOR_MOVIE,
	},
	{
		.name = "Contrast/Saturation/Sharpness",
		.display	= sharpness_display,
		.select		= sharpness_toggle_forward,
		.select_reverse		= sharpness_toggle_reverse,
		.help = "Adjust sharpness in current picture style.",
		.essential = FOR_MOVIE,
	},
	{
		.name = "Flash AEcomp",
		.display	= flash_ae_display,
		.select		= flash_ae_toggle_forward,
		.select_reverse		= flash_ae_toggle_reverse,
		.help = "Flash exposure compensation, from -5EV to +3EV.",
		.essential = FOR_PHOTO,
	},
};

// only being called in live view for some reason.
void hdr_create_script(int steps, int skip0, int focus_stack)
{
	if (steps <= 1) return;
	DEBUG();
	FILE * f = INVALID_PTR;
	char name[100];
	int f0 = skip0 ? file_number_also : file_number_also+1;
	snprintf(name, sizeof(name), CARD_DRIVE "DCIM/%03dCANON/%s_%04d.sh", folder_number, focus_stack ? "FST" : "HDR", f0);
	DEBUG("name=%s", name);
	FIO_RemoveFile(name);
	f = FIO_CreateFile(name);
	if ( f == INVALID_PTR )
	{
		bmp_printf( FONT_LARGE, 30, 30, "FCreate: Err %s", name );
		return;
	}
	DEBUG();
	my_fprintf(f, "#!/usr/bin/env bash\n");
	my_fprintf(f, "\n# %s_%04d.JPG from IMG_%04d.JPG ... IMG_%04d.JPG\n\n", focus_stack ? "FST" : "HDR", f0, f0, mod(f0 + steps - 1, 10000));
	my_fprintf(f, "enfuse \"$@\" %s --output=%s_%04d.JPG ", focus_stack ? "--exposure-weight=0 --saturation-weight=0 --contrast-weight=1 --hard-mask" : "", focus_stack ? "FST" : "HDR", f0);
	int i;
	for( i = 0; i < steps; i++ )
	{
		my_fprintf(f, "IMG_%04d.JPG ", mod(f0 + i, 10000));
	}
	my_fprintf(f, "\n");
	DEBUG();
	FIO_CloseFile(f);
	DEBUG();
}

// normal pic, silent pic, bulb pic...
static void take_a_pic(int allow_af)
{
	if (silent_pic_mode)
	{
		silent_pic_take(0); 
	}
	else
	{
		if (is_bulb_mode()) bulb_take_pic(bulb_shutter_value);
		else lens_take_picture(64, allow_af);
	}
	lens_wait_readytotakepic(64);
}

// Here, you specify the correction in 1/8 EV steps (for shutter or exposure compensation)
// The function chooses the best method for applying this correction (as exposure compensation, altering shutter value, or bulb timer)
// And then it takes a picture
// .. and restores settings back
static void hdr_shutter_release(int ev_x8, int allow_af)
{
	//~ NotifyBox(2000, "hdr_shutter_release: %d", ev_x8); msleep(2000);
	lens_wait_readytotakepic(64);

	int manual = (shooting_mode == SHOOTMODE_M || is_movie_mode() || is_bulb_mode());
	if (!manual) // auto modes
	{
		int ae0 = lens_get_ae();
		lens_set_ae(ae0 + ev_x8);
		take_a_pic(allow_af);
		lens_set_ae(ae0);
	}
	else // manual mode or bulb
	{

		//~ if (lens_info.raw_iso == 0) // it's set on auto ISO
			//~ iso_auto_quick(); // => lock the ISO here, otherwise it won't bracket

		// apply EV correction in both "domains" (milliseconds and EV)
		int ms = get_exposure_time_ms();
		int msc = ms * roundf(1000.0*powf(2, ev_x8 / 8.0))/1000;
		
		int rs = get_exposure_time_raw();
		int rc = rs - ev_x8;

		int s0r = lens_info.raw_shutter; // save settings (for restoring them back)
		
		//NotifyBox(2000, "ms=%d msc=%d rs=%x rc=%x", ms,msc,rs,rc); msleep(2000);

		// then choose the best option (bulb for long exposures, regular for short exposures)
		if (msc >= 10000 || (bulb_ramping_enabled && msc > BULB_MIN_EXPOSURE))
		{
			bulb_take_pic(msc);
		}
		else
		{
			int b = bulb_ramping_enabled;
			bulb_ramping_enabled = 0; // to force a pic in manual mode
			
			lens_set_rawshutter(rc);
			take_a_pic(allow_af);
			
			bulb_ramping_enabled = b;
		}

		// restore settings back
		//~ set_shooting_mode(m0r);
		prop_request_change( PROP_SHUTTER, &s0r, 4 );
		prop_request_change( PROP_SHUTTER_ALSO, &s0r, 4);
	}
	msleep(100);
	lens_wait_readytotakepic(64);
	msleep(100);
}

// skip0: don't take the middle exposure
static void hdr_take_pics(int steps, int step_size, int skip0)
{
	//~ NotifyBox(2000, "hdr_take_pics: %d, %d, %d", steps, step_size, skip0); msleep(2000);
	hdr_create_script(steps, skip0, 0);
	//~ NotifyBox(2000, "HDR script created"); msleep(2000);
	int i;
	
	int m = shooting_mode;
	
	for( i = -steps/2; i <= steps/2; i ++  )
	{
		if (skip0 && (i == 0)) continue;
		hdr_shutter_release(step_size * i, 0);
		
		// cancel bracketing
		if (shooting_mode != m || MENU_MODE) 
		{ 
			beep(); 
			while (lens_info.job_state) msleep(100); 
			return; 
		}
	}
}

static void press_rec_button()
{
#if defined(CONFIG_50D) || defined(CONFIG_5D2)
	fake_simple_button(BGMT_PRESS_SET);
#else
	fake_simple_button(BGMT_LV);
#endif
}

void movie_start()
{
	ensure_movie_mode();
	
	if (recording)
	{
		NotifyBox(2000, "Already recording ");
		return;
	}
	
	while (get_halfshutter_pressed()) msleep(100);
	
	press_rec_button();
	
	while (recording != 2) msleep(100);
	msleep(500);
}

void movie_end()
{
	if (shooting_type != 3 && !is_movie_mode())
	{
		NotifyBox(2000, "movie_end: not movie mode (%d,%d) ", shooting_type, shooting_mode);
		return;
	}
	if (!recording)
	{
		NotifyBox(2000, "movie_end: not recording ");
		return;
	}

	while (get_halfshutter_pressed()) msleep(100);

	msleep(500);

	press_rec_button();

	// wait until it stops recording, but not more than 2s
	for (int i = 0; i < 20; i++)
	{
		msleep(100);
		if (!recording) break;
	}
	msleep(500);
}


static void
short_movie()
{
	movie_start();
	msleep(timer_values[interval_movie_duration_index] * 1000);
	movie_end();
}

// take one picture or a HDR / focus stack sequence
// to be used with the intervalometer
void hdr_shot(int skip0, int wait)
{
	NotifyBoxHide();
	if (hdr_steps > 1)
	{
		//~ NotifyBox(1000, "HDR shot (%dx%dEV)...", hdr_steps, hdr_stepsize/8); msleep(1000);
		int drive_mode_bak = 0;
		if (drive_mode != DRIVE_SINGLE && drive_mode != DRIVE_CONTINUOUS) 
		{
			drive_mode_bak = drive_mode;
			lens_set_drivemode(DRIVE_CONTINUOUS);
		}
		if (hdr_steps == 2)
			hdr_take_pics(hdr_steps, hdr_stepsize/2, 1);
		else
			hdr_take_pics(hdr_steps, hdr_stepsize, skip0);
		while (lens_info.job_state >= 10) msleep(100);
		if (drive_mode_bak) lens_set_drivemode(drive_mode_bak);
	}
	else // regular pic (not HDR)
	{
		hdr_shutter_release(0, 1);
	}
}

int remote_shot_flag = 0;
void schedule_remote_shot() { remote_shot_flag = 1; }

int mlu_lock_flag = 0;
void schedule_mlu_lock() { mlu_lock_flag = 1; }

int movie_start_flag = 0;
void schedule_movie_start() { movie_start_flag = 1; }
int is_movie_start_scheduled() { return movie_start_flag; }

int movie_end_flag = 0;
void schedule_movie_end() { movie_end_flag = 1; }

void get_out_of_play_mode(int extra_wait)
{
	if (gui_state == GUISTATE_QR)
	{
		fake_simple_button(BGMT_PLAY);
		msleep(200);
		fake_simple_button(BGMT_PLAY);
	}
	else if (PLAY_MODE) 
	{
		fake_simple_button(BGMT_PLAY);
	}
	while (PLAY_MODE) msleep(100);
	msleep(extra_wait);
}

// take one shot, a sequence of HDR shots, or start a movie
// to be called by remote triggers
void remote_shot(int wait)
{
	// save zoom value (x1, x5 or x10)
	int zoom = lv_dispsize;
	
	if (is_focus_stack_enabled())
	{
		focus_stack_run(0);
	}
	else if (is_movie_mode())
	{
		movie_start();
	}
	else
	{
		hdr_shot(0, wait);
	}
	if (!wait) return;
	
	lens_wait_readytotakepic(64);
	msleep(500);
	while (gui_state != GUISTATE_IDLE) msleep(100);
	msleep(500);
	// restore zoom
	if (lv && !recording && zoom > 1) prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
}

void iso_refresh_display()
{
	int bg = bmp_getpixel(MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	int iso = lens_info.iso;
	if (iso)
		bmp_printf(fnt, MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y, "ISO %5d", iso);
	else
		bmp_printf(fnt, MENU_DISP_ISO_POS_X, MENU_DISP_ISO_POS_Y, "ISO AUTO");
}

static void display_expsim_status()
{
	static int prev_expsim = 0;
	int x = 610 + 2 * font_med.width;
	int y = 400;
	if (!expsim)
	{
		bmp_printf( FONT(FONT_MED, COLOR_WHITE, 0), x, y, "ExpSim" );
		draw_line(x-5, y + font_med.height * 3/4, x + font_med.width * 6, y + font_med.height * 1/4, COLOR_WHITE);
	}
	else
	{
		if (expsim != prev_expsim)// redraw();
			bmp_printf( FONT(FONT_MED, COLOR_WHITE, 0), x, y, "      " );
	}
	prev_expsim = expsim;
}


void display_shooting_info_lv()
{
	int screen_layout = get_screen_layout();
	int audio_meters_at_top = audio_meters_are_drawn() 
		&& (screen_layout == SCREENLAYOUT_3_2 || screen_layout == SCREENLAYOUT_16_10);

	display_lcd_remote_icon(480, audio_meters_at_top ? 40 : 0);
	display_trap_focus_info();
	display_expsim_status();
}

void display_trap_focus_info()
{
	int show, fg, bg, x, y;
	static int show_prev = 0;
	if (lv)
	{
		show = trap_focus && can_lv_trap_focus_be_active();
		int active = show && get_halfshutter_pressed();
		bg = active ? COLOR_BG : 0;
		fg = active ? COLOR_RED : COLOR_BG;
		x = 8; y = 160;
		if (show || show_prev) bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? "TRAP \nFOCUS" : "     \n     ");
	}
	else
	{
		show = (trap_focus && ((af_mode & 0xF) == 3) && lens_info.raw_aperture);
		bg = bmp_getpixel(DISPLAY_TRAP_FOCUS_POS_X, DISPLAY_TRAP_FOCUS_POS_Y);
		fg = trap_focus == 2 || HALFSHUTTER_PRESSED ? COLOR_RED : COLOR_FG_NONLV;
		x = DISPLAY_TRAP_FOCUS_POS_X; y = DISPLAY_TRAP_FOCUS_POS_Y;
		if (show || show_prev) bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? DISPLAY_TRAP_FOCUS_MSG : DISPLAY_TRAP_FOCUS_MSG_BLANK);
	}
	show_prev = show;
}

// may be unreliable
int wait_for_lv_err_msg(int wait) // 1 = msg appeared, 0 = did not appear
{
	int i;
	for (i = 0; i <= wait/20; i++)
	{
		int msgcolor = 3; // may give wrong results if cropmark uses this color; may be camera-dependent
		if (bmp_getpixel(300,150) == msgcolor &&
			bmp_getpixel(60,250) == msgcolor &&
			bmp_getpixel(400,300) == msgcolor
			) return 1;
		msleep(20);
	}
	return 0;
}

void intervalometer_stop()
{
	if (intervalometer_running)
	{
		intervalometer_running = 0;
		bramp_init_state = 0;
		NotifyBox(2000, "Intervalometer stopped.");
		//~ display_on();
	}
}

int handle_intervalometer(struct event * event)
{
	// stop intervalometer with MENU or PLAY
	if (!IS_FAKE(event) && (event->param == BGMT_MENU || event->param == BGMT_PLAY) && !gui_menu_shown())
		intervalometer_stop();
	return 1;
}

// this syncs with real-time clock
void wait_till_next_second()
{
	struct tm now;
	LoadCalendarFromRTC( &now );
	int s = now.tm_sec;
	
	while (now.tm_sec == s)
	{
		LoadCalendarFromRTC( &now );
		msleep(20);
/*		if (lens_info.job_state == 0) // unsafe otherwise?
		{
			call("DisablePowerSave"); // trick from AJ_MREQ_ISR
			call("EnablePowerSave"); // to prevent camera for entering "deep sleep"
		}*/
	}
}

static int intervalometer_pictures_taken = 0;
static int intervalometer_next_shot_time = 0;

static void
shoot_task( void* unused )
{
	//~ int i = 0;
	menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
	menu_add( "Expo", expo_menus, COUNT(expo_menus) );
	msleep(1000);
	menu_add( "Tweaks", vid_menus, COUNT(vid_menus) );

	bulb_shutter_value = timer_values[bulb_duration_index] * 1000;

	// :-)
	struct tm now;
	LoadCalendarFromRTC( &now );
	if (now.tm_mday == 1 && now.tm_mon == 3)
	{
		toggle_mirror_display();
	}
	
	while(1)
	{
		msleep(10);

		if (iso_auto_flag)
		{
			iso_auto_run();
			iso_auto_flag = 0;
		}
		if (shutter_auto_flag)
		{
			shutter_auto_run();
			shutter_auto_flag = 0;
		}
		if (kelvin_auto_flag)
		{
			kelvin_auto_run();
			kelvin_auto_flag = 0;
		}
		if (wbs_gm_auto_flag)
		{
			wbs_gm_auto_run();
			wbs_gm_auto_flag = 0;
		}
		
		//~ if (gui_menu_shown()) continue; // be patient :)

		lcd_release_step();
		
		if (remote_shot_flag)
		{
			remote_shot(1);
			remote_shot_flag = 0;
		}
		if (mlu_lock_flag)
		{
			mlu_lock_mirror_if_needed();
			mlu_lock_flag = 0;
		}
		if (movie_start_flag)
		{
			movie_start();
			movie_start_flag = 0;
		}
		if (movie_end_flag)
		{
			movie_end();
			movie_end_flag = 0;
		}
		
		if (!lv) // MLU
		{
			//~ if (mlu_mode == 0 && get_mlu()) set_mlu(0);
			//~ if (mlu_mode == 1 && !get_mlu()) set_mlu(1);
			if (mlu_auto)
			{
				int mlu_auto_value = ((drive_mode == DRIVE_SELFTIMER_2SEC || drive_mode == DRIVE_SELFTIMER_REMOTE || lcd_release_running == 2) && (hdr_steps < 2)) ? 1 : 0;
				int mlu_current_value = get_mlu() ? 1 : 0;
				if (mlu_auto_value != mlu_current_value && !is_movie_mode() && !lv)
				{
					if (MENU_MODE && !gui_menu_shown()) // MLU changed from Canon menu
					{ 
						mlu_auto = 0;
						NotifyBox(2000, "ML: Auto MLU disabled");
					}
					else
					{
						set_mlu(mlu_auto_value); // shooting mode, ML decides to toggle MLU
					}
				}
			}
		}
		
		if (lv && face_zoom_request && lv_dispsize == 1 && !recording)
		{
			if (lvaf_mode == 2 && wait_for_lv_err_msg(200)) // zoom request in face detect mode; temporary switch to live focus mode
			{
				int afmode = 1;
				int zoom = 5;
				int afx = afframe[2];
				int afy = afframe[3];
				prop_request_change(PROP_LVAF_MODE, &afmode, 4);
				msleep(100);
				afframe[2] = afx;
				afframe[3] = afy;
				prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
				msleep(1);
				prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
				msleep(1);
			}
			else if (lvaf_mode == 1) // back from temporary live focus mode
			{
				int afmode = 2;
				prop_request_change(PROP_LVAF_MODE, &afmode, 4);
				msleep(100);
				face_zoom_request = 0;
				//~ bmp_printf(FONT_LARGE, 10, 50, "       ");
			}
			else // cancel zoom request
			{
				msleep(100);
				face_zoom_request = 0;
				//~ bmp_printf(FONT_LARGE, 10, 50, "Zoom :(");
			}
		}
		if (zoom_disable_x5 && lv_dispsize == 5)
		{
			int zoom = 10;
			prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
			msleep(100);
		}
		if (zoom_disable_x10 && lv_dispsize == 10)
		{
			int zoom = 1;
			prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
			msleep(100);
		}
		/*if (sweep_lv_on)
		{
			sweep_lv();
			sweep_lv_on = 0;
		}*/
		if (center_lv_aff)
		{
			center_lv_afframe_do();
			center_lv_aff = 0;
		}

		// avoid camera shake for HDR shots => force self timer
		static int drive_mode_bk = -1;
		if ((hdr_steps > 1 || is_focus_stack_enabled()) && get_halfshutter_pressed() && drive_mode != DRIVE_SELFTIMER_2SEC && drive_mode != DRIVE_SELFTIMER_REMOTE)
		{
			drive_mode_bk = drive_mode;
			lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
		}
		
		// restore drive mode if it was changed
		if (!get_halfshutter_pressed() && drive_mode_bk >= 0)
		{
			msleep(50);
			lens_set_drivemode(drive_mode_bk);
			drive_mode_bk = -1;
		}
	
		if (bulb_duration_index && is_bulb_mode() && !gui_menu_shown())
		{
			// look for a transition of half-shutter during idle state
			static int was_idle_not_pressed = 0;
			int is_idle_not_pressed = !get_halfshutter_pressed() && gui_state == GUISTATE_IDLE;
			int is_idle_and_pressed = get_halfshutter_pressed() && gui_state == GUISTATE_IDLE;

			if (was_idle_not_pressed && is_idle_and_pressed)
			{
				int d = bulb_shutter_value/1000;
				NotifyBoxHide();
				NotifyBox(10000, "[HalfShutter] Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
				while (get_halfshutter_pressed())
				{
					msleep(100);
				}
				int m0 = shooting_mode;
				wait_till_next_second();
				NotifyBoxHide();
				NotifyBox(2000, "[2s] Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
				wait_till_next_second();
				if (get_halfshutter_pressed()) continue;
				if (gui_state != GUISTATE_IDLE) continue;
				if (m0 != shooting_mode) continue;
				NotifyBoxHide();
				NotifyBox(2000, "[1s] Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
				wait_till_next_second();
				if (get_halfshutter_pressed()) continue;
				if (gui_state != GUISTATE_IDLE) continue;
				if (m0 != shooting_mode) continue;
				bulb_take_pic(d * 1000);
			}
			was_idle_not_pressed = is_idle_not_pressed;
		}
		
		if (lens_info.job_state >= 10 && !recording) // just took a picture, maybe we should take another one
		{
			//~ hdr_intercept = 0;
			lens_wait_readytotakepic(64);
			//~ if (beep_enabled) beep();
			if (is_focus_stack_enabled()) focus_stack_run(1); // skip first exposure, we already took it
			else if (hdr_steps > 1) hdr_shot(1,1); // skip the middle exposure, which was just taken
			//~ if (beep_enabled) beep();
			//~ hdr_intercept = 1;
		}

		// toggle flash on/off for next picture
		/*if (!is_movie_mode() && flash_and_no_flash && strobo_firing < 2 && strobo_firing != file_number % 2)
		{
			strobo_firing = file_number % 2;
			set_flash_firing(strobo_firing);
		}*/

		//~ static int sw1_countdown = 0;
		
		// trap focus (outside LV) and all the preconditions
		int tfx = trap_focus && (af_mode & 0xF) == 3 && gui_state == GUISTATE_IDLE && !gui_menu_shown() && !intervalometer_running;

		// same for motion detect
		int mdx = motion_detect && gui_state == GUISTATE_IDLE && !gui_menu_shown() && lv && !recording;
		
		//Reset the counter so that if you go in and out of live view, it doesn't start clicking away right away.
		static int K = 0;

		if(!mdx) K = 0;
		// emulate half-shutter press (for trap focus or motion detection)
		/* this can cause the camera not to shutdown properly... 
		if (!lv && ((tfx && trap_focus == 2) || mdx ))
		{
			if (trap_focus == 2 && (cfn[2] & 0xF00) != 0) bmp_printf(FONT_MED, 0, 0, "Set CFn9 to 0 (AF on half-shutter press)");
			if (!sw1_countdown) // press half-shutter periodically
			{
				if (sw1_pressed) { SW1(0,10); sw1_pressed = 0; }
				{ SW1(1,10); sw1_pressed = 1; }
				sw1_countdown = motion_detect ? 2 : 10;
			}
			else
			{
				sw1_countdown--;
			}
		}
		else // cleanup sw1
			if (sw1_pressed) { SW1(0,10); sw1_pressed = 0; } */

		if (tfx) // MF
		{
			if (HALFSHUTTER_PRESSED) card_led_on(); else card_led_off();
			if ((!lv && FOCUS_CONFIRMATION) || get_lv_focus_confirmation())
			{
				remote_shot(0);
				//~ msleep(trap_focus_delay);
			}
		}
		
		if (mdx)
		{
			K = COERCE(K+1, 0, 1000);
			//~ bmp_printf(FONT_MED, 0, 50, "K= %d   ", K);

			if (motion_detect == 1)
			{
				int aev = 0;
				//If the new value has changed by more than the detection level, shoot.
				static int old_ae_avg = 0;
				int y,u,v;
				//TODO: maybe get the spot yuv of the target box
				get_spot_yuv(100, &y, &u, &v);
				aev = y / 2;
				if (K > 50) bmp_printf(FONT_MED, 0, 50, "Average exposure: %3d    New exposure: %3d   ", old_ae_avg/100, aev);
				if (K > 50 && ABS(old_ae_avg/100 - aev) >= (int)motion_detect_level)
				{
					remote_shot(1);
					//~ msleep(trap_focus_delay);
					K = 0;
				}
				old_ae_avg = old_ae_avg * 90/100 + aev * 10;
			}
			else if (motion_detect == 2)
			{
				int d = get_spot_motion(100, get_global_draw());
				if (K > 50) bmp_printf(FONT_MED, 0, 50, "Motion level: %d   ", d);
				if (K > 50 && d >= (int)motion_detect_level)
				{
					remote_shot(1);
					//~ msleep(trap_focus_delay);
					K = 0;
				}
			}
		}

		static int silent_pic_countdown;
		if (gui_state != GUISTATE_IDLE || gui_menu_shown())
		{
			silent_pic_countdown = 10;
		}
		else if (!get_halfshutter_pressed())
		{
			if (silent_pic_countdown) silent_pic_countdown--;
		}

		if (lv && silent_pic_mode && get_halfshutter_pressed())
		{
			if (silent_pic_countdown) // half-shutter was pressed while in playback mode, for example
				continue;
			if (is_focus_stack_enabled()) focus_stack_run(0); // shoot all frames
			else if (hdr_steps == 1) silent_pic_take(1);
			else 
			{
				NotifyBox(5000, "HDR silent picture...");
				if (beep_enabled) Beep();
				while (get_halfshutter_pressed()) msleep(100);
				if (!lv) force_liveview();
				hdr_shot(0,1);
			}
		}
		
		#define SECONDS_REMAINING (intervalometer_next_shot_time - seconds_clock)
		#define SECONDS_ELAPSED (seconds_clock - seconds_clock_0)

		if (intervalometer_running)
		{
			int seconds_clock_0 = seconds_clock;
			int display_turned_off = 0;
			int images_compared = 0;
			msleep(100);
			while (SECONDS_REMAINING > 0)
			{
				msleep(100);

				if (!intervalometer_running) continue;
				
				if (gui_menu_shown() || get_halfshutter_pressed())
				{
					intervalometer_next_shot_time++;
					wait_till_next_second();
					continue;
				}
				bmp_printf(FONT_LARGE, 50, 50, 
								" Intervalometer:%4d \n"
								" Pictures taken:%4d ", 
								SECONDS_REMAINING,
								intervalometer_pictures_taken);

				if (!images_compared && SECONDS_ELAPSED >= 2 && SECONDS_REMAINING >= 2 && image_review_time - SECONDS_ELAPSED >= 1 && bramp_init_done)
				{
					playback_compare_images(0);
					images_compared = 1; // do this only once
				}
				if (PLAY_MODE && SECONDS_ELAPSED >= image_review_time)
				{
					get_out_of_play_mode(0);
				}

				extern int idle_display_turn_off_after;
				if (idle_display_turn_off_after && lens_info.job_state == 0 && gui_state == GUISTATE_IDLE && intervalometer_running && lv && !gui_menu_shown() && !display_turned_off)
				{
					// stop LiveView and turn off display to save power
					PauseLiveView();
					msleep(200);
					display_off_force();
					display_turned_off = 1; // ... but only once per picture (don't be too aggressive)
				}
			}

			if (PLAY_MODE) get_out_of_play_mode(0);
			ResumeLiveView();

			if (!intervalometer_running) continue;
			if (gui_menu_shown() || get_halfshutter_pressed()) continue;

			if (bulb_ramping_enabled)
			{
				bulb_ramping_init();
				compute_exposure_for_next_shot();
			}

			if (!intervalometer_running) continue;
			if (gui_menu_shown() || get_halfshutter_pressed()) continue;

			int dt = timer_values[interval_timer_index];
			// compute the moment for next shot; make sure it stays somewhat in sync with the clock :)
			intervalometer_next_shot_time = COERCE(intervalometer_next_shot_time + dt, seconds_clock, seconds_clock + dt);

			if (dt == 0)
			{
				take_a_pic(1);
			}
			else if (!is_movie_mode() || silent_pic_mode)
			{
				hdr_shot(0, 1);
				intervalometer_next_shot_time = MAX(intervalometer_next_shot_time, seconds_clock);
			}
			else
			{
				short_movie();
				// in movie mode, time starts since the end of last movie (not since the start)
				intervalometer_next_shot_time = seconds_clock + dt;
			}
			intervalometer_pictures_taken++;
			
		}
		else // intervalometer not running
		{
			bramp_init_done = 0;
			intervalometer_pictures_taken = 0;
			intervalometer_next_shot_time = seconds_clock + 3;
			
			if (audio_release_running) 
			{
				static int countdown = 0;
				if (gui_state != GUISTATE_IDLE || gui_menu_shown()) countdown = 50;
				if (countdown) { countdown--; continue; }

				extern struct audio_level audio_levels[];

				static int avg_prev0 = 1000;
				static int avg_prev1 = 1000;
				static int avg_prev2 = 1000;
				static int avg_prev3 = 1000;
				int current_pulse_level = audio_levels[0].peak / avg_prev3;
	
				bmp_printf(FONT_MED, 20, lv ? 40 : 3, "Audio release ON (%d / %d)   ", current_pulse_level, audio_release_level);
				if (current_pulse_level > (int)audio_release_level) 
				{
					remote_shot(1);
					msleep(100);
					/* Initial forced sleep is necesarry when using camera self timer,
					 * otherwise remote_shot returns right after the countdown 
					 * and the loop below seems to miss the actual picture taking.
					 * This means we will trigger again on the sound of the shutter
					 * (and again, and again, ...)
					 * TODO: should this be fixed in remote_shot itself? */
					while (lens_info.job_state) msleep(100);
				}
				avg_prev3 = avg_prev2;
				avg_prev2 = avg_prev1;
				avg_prev1 = avg_prev0;
				avg_prev0 = audio_levels[0].avg;
			}
		}
	}
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x1a, 0x4000 );

void shoot_init()
{
	set_maindial_sem = create_named_semaphore("set_maindial_sem", 1);
}

INIT_FUNC("shoot", shoot_init);
