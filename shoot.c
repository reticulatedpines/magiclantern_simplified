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

void move_lv_afframe(int dx, int dy);
void movie_start();
void movie_end();
void display_trap_focus_info();
void display_lcd_remote_icon(int x0, int y0);

CONFIG_INT( "interval.timer.index", interval_timer_index, 2 );
CONFIG_INT( "focus.trap", trap_focus, 0);
CONFIG_INT( "focus.trap.delay", trap_focus_delay, 1000); // min. delay between two shots in trap focus
CONFIG_INT( "audio.release-level", audio_release_level, 10);
CONFIG_INT( "interval.movie.duration.index", interval_movie_duration_index, 2);
//~ CONFIG_INT( "flash_and_no_flash", flash_and_no_flash, 0);
CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );        // 0 = off, 1 = normal, 2 = hi-res, 3 = long-exp, 4 = slit-scan
CONFIG_INT( "silent.pic.submode", silent_pic_submode, 0);   // simple, burst, fullhd
#define silent_pic_burst (silent_pic_submode == 1)
#define silent_pic_fullhd (silent_pic_submode == 2)
CONFIG_INT( "silent.pic.highres", silent_pic_highres, 0);   // index of matrix size (2x1 .. 5x5)
CONFIG_INT( "silent.pic.sweepdelay", silent_pic_sweepdelay, 350);
CONFIG_INT( "silent.pic.slitscan.skipframes", silent_pic_slitscan_skipframes, 1);
CONFIG_INT( "silent.pic.longexp.time.index", silent_pic_longexp_time_index, 5);
CONFIG_INT( "silent.pic.longexp.method", silent_pic_longexp_method, 0);
CONFIG_INT( "zoom.enable.face", zoom_enable_face, 1);
CONFIG_INT( "zoom.disable.x5", zoom_disable_x5, 0);
CONFIG_INT( "zoom.disable.x10", zoom_disable_x10, 0);
CONFIG_INT( "bulb.duration.index", bulb_duration_index, 2);
CONFIG_INT( "lcd.release", lcd_release_running, 0);
CONFIG_INT( "mlu.mode", mlu_mode, 2); // off, on, auto

//New option for the sensitivty of the motion release
CONFIG_INT( "motion.release-level", motion_detect_level, 8);

int get_silent_pic_mode() { return silent_pic_mode; } // silent pic will disable trap focus

CONFIG_INT("intervalometer.wait", intervalometer_wait, 1);

int intervalometer_running = 0;
int audio_release_running = 0;
int motion_detect = 0;
//int motion_detect_level = 8;
int drive_mode_bk = -1;

int gui_state = 0;
CONFIG_INT("quick.review.allow.zoom", quick_review_allow_zoom, 1);
PROP_HANDLER(PROP_GUI_STATE)
{
	gui_state = buf[0];

	if (gui_state == 3 && image_review_time == 0xff && quick_review_allow_zoom && !intervalometer_running)
	{
		fake_simple_button(BGMT_PLAY);
	}

	return prop_cleanup(token, property);
}

int timer_values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 25, 30, 35, 40, 45, 50, 55, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 1200, 1800, 2700, 3600, 5400, 7200, 9000, 10800, 14400, 18000, 21600, 25200, 28800};
int timer_values_ms[] = {100, 200, 300, 500, 700, 1000, 2000, 3000, 5000, 7000, 10000, 15000, 20000, 30000, 50000, 60000, 120000, 180000, 300000, 600000, 900000, 1800000};

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

static void
interval_timer_display( void * priv, int x, int y, int selected )
{
	if (shooting_mode != SHOOTMODE_MOVIE || silent_pic_mode)
	{
		int d = timer_values[*(int*)priv];
		if (!d)
			bmp_printf(
				selected ? MENU_FONT_SEL : MENU_FONT,
				x, y,
				"Take pics like crazy"
			);
		else
			bmp_printf(
				selected ? MENU_FONT_SEL : MENU_FONT,
				x, y,
				"Take a pic every: %d%s",
				d < 60 ? d : d/60, 
				d < 60 ? "s" : "min"
			);
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
	
	menu_draw_icon(x, y, intervalometer_running ? MNI_PERCENT : MNI_WARNING, (*(int*)priv) * 100 / COUNT(timer_values));
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
	if (shooting_mode == SHOOTMODE_MOVIE && silent_pic_mode == 0)
		interval_movie_duration_index = mod(interval_movie_duration_index + 1, COUNT(timer_values));
}

static void 
intervalometer_display( void * priv, int x, int y, int selected )
{
	int p = *(int*)priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Intervalometer  : %s%s",
		p ? "ON" : "OFF",
		p ? (intervalometer_wait ? ",Wait" : ",NoWait") : ""
	);
}

static void 
lcd_release_display( void * priv, int x, int y, int selected )
{
	int v = (*(int*)priv);
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LCD Remote Shot : %s",
		v == 1 ? "Near" : v == 2 ? (mlu_mode ? "Away/MLU" : "Away") : v == 3 ? "Wave" : "OFF"
	);
	if (v) display_lcd_remote_icon(x-25, y+5);
	menu_draw_icon(x, y, v ? -1 : MNI_OFF, 0);
}

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
	menu_draw_icon(x, y, MNI_BOOL_LV(motion_detect), 0);
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
//~ static const int16_t silent_pic_sweep_modes_l[] = {2, 2, 2, 3, 3, 4, 4, 5};
//~ static const int16_t silent_pic_sweep_modes_c[] = {1, 2, 3, 3, 4, 4, 5, 5};
//~ #define SILENTPIC_NL COERCE(silent_pic_sweep_modes_l[COERCE(silent_pic_highres,0,COUNT(silent_pic_sweep_modes_l)-1)], 0, 5)
//~ #define SILENTPIC_NC COERCE(silent_pic_sweep_modes_c[COERCE(silent_pic_highres,0,COUNT(silent_pic_sweep_modes_c)-1)], 0, 5)

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
/*	else if (silent_pic_mode == 2)
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
	else if (silent_pic_mode == 3)
	{
		int t = timer_values_ms[mod(silent_pic_longexp_time_index, COUNT(timer_values_ms))];
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Silent Pic LongX: %s%ds,%s",
			t < 1000 ? "0." : "",
			t < 1000 ? t / 100 : t / 1000,
			silent_pic_longexp_method ? "MAX" : "AVG"
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
	menu_draw_icon(x, y, MNI_BOOL_LV(silent_pic_mode), 0);
}

static void silent_pic_mode_toggle(void* priv)
{
	silent_pic_mode = mod(silent_pic_mode + 1, 5); // off, normal, hi-res, long-exp, slit
	if (silent_pic_mode == 3) silent_pic_mode = 4; // skip longx, not working
	if (silent_pic_mode == 2) silent_pic_mode = 4; // skip hi-res
}

static void silent_pic_toggle(int sign)
{
	if (silent_pic_mode == 1)
		silent_pic_submode = mod(silent_pic_submode + 1, 3);
	/*else if (silent_pic_mode == 2) 
		silent_pic_highres = mod(silent_pic_highres + sign, COUNT(silent_pic_sweep_modes_c));
	else if (silent_pic_mode == 3) 
	{
		if (sign < 0)
		{
			silent_pic_longexp_method = !silent_pic_longexp_method;
		}
		else
		{
			silent_pic_longexp_time_index = mod(silent_pic_longexp_time_index + 1, COUNT(timer_values_ms));
		}
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

int hs = 0;
PROP_HANDLER( PROP_HALF_SHUTTER ) {
	int v = *(int*)buf;
	if (zoom_enable_face)
	{
		if (v == 0 && lv_drawn() && lvaf_mode == 2 && gui_state == 0 && !recording) // face detect
			face_zoom_request = 1;
	}
/*	if (v && gui_menu_shown() && !is_menu_active("Focus"))
	{
		gui_stop_menu();
	}*/
	return prop_cleanup( token, property );
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
	if (!lv_drawn() || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;
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
	if (!lv_drawn() || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;
	afframe[2] = COERCE(afframe[2] + dx, 500, afframe[0] - afframe[4]);
	afframe[3] = COERCE(afframe[3] + dy, 500, afframe[1] - afframe[5]);
	prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
}

/*
static void 
sweep_lv()
{
	if (recording) return;
	if (!lv_drawn()) return;
	gui_stop_menu();
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
	convert_yuv_to_bmp("B:/DCIM/100CANON/1324-001.422", "B:/DCIM/100CANON/1324-001.BMP");
	return;
	bmp_printf(FONT_MED, 0, 40, "yuv to bmp...");
	struct fio_file file;
	struct fio_dirent * dirent = FIO_FindFirstEx( "B:/DCIM/", &file );
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
			snprintf(folder, sizeof(folder), "B:/DCIM/%s/", file.name);
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
		msleep(1);
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
			snprintf(imgname, sizeof(imgname), "B:/DCIM/%03dCANON/%08d.422", folder_number, silent_number);
			unsigned size;
			if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
			if (size == 0) break;
		}
	}
	else
	{
		for ( ; silent_number < 1000; silent_number++)
		{
			snprintf(imgname, sizeof(imgname), "B:/DCIM/%03dCANON/%04d-%03d.422", folder_number, file_number, silent_number);
			unsigned size;
			if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
			if (size == 0) break;
		}
	}
	bmp_printf(FONT_MED, 100, 130, "%s    ", imgname);
	return imgname;
}
/*
int ms100_clock = 0;
static void
ms100_clock_task( void )
{
	while(1)
	{
		msleep(100);
		ms100_clock += 100;
	}
}
TASK_CREATE( "ms100_clock_task", ms100_clock_task, 0, 0x19, 0x1000 );*/


// not working
/*
static void
silent_pic_take_longexp()
{
	struct vram_info * vram = get_yuv422_hd_vram();
	uint8_t* buf = AllocateMemory(vram->pitch * vram->width * 2);
	if (!buf)
	{
		bmp_printf(FONT_MED, 100, 100, "Psst! Not enough memory :(  ");
		return;
	}
	FreeMemory(buf);
	
	char* imgname = silent_pic_get_name();
//~ 
	//~ FIO_RemoveFile(imgname);
	//~ FILE* f = FIO_CreateFile(imgname);
	//~ if (f == INVALID_PTR)
	//~ {
		//~ bmp_printf(FONT_SMALL, 120, 40, "FCreate: Err %s", imgname);
		//~ return;
	//~ }
//~ 
	//~ ms100_clock = 0;
	//~ int tmax = timer_values_ms[silent_pic_longexp_time_index];
	//~ while (ms100_clock < tmax)
	//~ {
		//~ bmp_printf(FONT_MED, 100, 100, "Psst! Taking a long-exp silent pic (%d/%d)...   ", ms100_clock, tmax);
		//~ int ans = FIO_WriteFile(f, vram->vram, vram->height * vram->pitch);
		//~ msleep(10);
	//~ }
	//~ FIO_CloseFile(f);
	//~ 
	bmp_printf(FONT_MED, 100, 100, "Psst! Just took a long-exp silent pic   ");
	
	if (!silent_pic_burst) // single mode
	{
		while (get_halfshutter_pressed()) msleep(100);
	}
}*/

static int
silent_pic_ensure_movie_mode()
{
	if (silent_pic_fullhd && shooting_mode != SHOOTMODE_MOVIE) 
	{ 
		set_shooting_mode(SHOOTMODE_MOVIE);
		msleep(1000); 
	}
	if (silent_pic_fullhd && !recording)
	{
		movie_start();
		return 1;
	}
	return 0;
}

static void
silent_pic_stop_dummy_movie()
{ 
	movie_end();
	char name[100];
	snprintf(name, sizeof(name), "B:/DCIM/%03dCANON/MVI_%04d.THM", folder_number, file_number);
	FIO_RemoveFile(name);
	snprintf(name, sizeof(name), "B:/DCIM/%03dCANON/MVI_%04d.MOV", folder_number, file_number);
	FIO_RemoveFile(name);
}

static void
silent_pic_take_simple()
{
	int movie_started = silent_pic_ensure_movie_mode();
	
	struct vram_info * vram = get_yuv422_hd_vram();
	
	char* imgname = silent_pic_get_name();
	
	bmp_printf(FONT_MED, 100, 100, "Psst! Taking a pic      ");
	//~ vsync(vram->vram + vram->pitch * vram->height - 100);
	dump_seg(vram->vram, vram->pitch * vram->height, imgname);
	bmp_printf(FONT_MED, 100, 100, "Psst! Just took a pic   ");
	
	if (movie_started) silent_pic_stop_dummy_movie();

	if (!silent_pic_burst) // single mode
	{
		while (get_halfshutter_pressed()) msleep(100);
	}
}

void
silent_pic_take_lv_dbg()
{
	struct vram_info * vram = get_yuv422_vram();
	int silent_number;
	char imgname[100];
	for (silent_number = 0 ; silent_number < 1000; silent_number++) // may be slow after many pics
	{
		snprintf(imgname, sizeof(imgname), "B:/VRAM%d.422", silent_number);
		unsigned size;
		if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
		if (size == 0) break;
	}
	dump_seg(vram->vram, vram->pitch * vram->height, imgname);
}

/*static void
silent_pic_take_sweep()
{
	if (recording) return;
	if (!lv_drawn()) return;
	if ((af_mode & 0xF) != 3 )
	{
		bmp_printf(FONT_MED, 100, 100, "Please switch to Manual Focus."); 
		return; 
	}

	bmp_printf(FONT_MED, 100, 100, "Psst! Preparing for high-res pic   ");
	while (get_halfshutter_pressed()) msleep(100);
	gui_stop_menu();
	msleep(100);

	int afx0 = afframe[2];
	int afy0 = afframe[3];

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
			int ans = FIO_WriteFile(f, vram->vram, 1024 * 680 * 2);
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

	bmp_printf(FONT_MED, 100, 100, "Psst! Just took a high-res pic   ");

}*/

static void
silent_pic_take_slitscan(int interactive)
{
	if (recording) return; // vsync fails
	if (!lv_drawn()) return;
	gui_stop_menu();
	while (get_halfshutter_pressed()) msleep(100);
	msleep(500);
	clrscr();

	uint8_t * const lvram = UNCACHEABLE(YUV422_LV_BUFFER);
	int lvpitch = YUV422_LV_PITCH;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	#define BMPPITCH 960

	struct vram_info * vram = get_yuv422_hd_vram();
	bmp_printf(FONT_MED, 100, 100, "Psst! Taking a slit-scan pic (%dx%d)", vram->width, vram->height);

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
			bmp_printf(FONT_MED, 100, 100, "Slit-scan cancelled.");
			while (get_halfshutter_pressed()) msleep(100);
			return;
		}
	}
	FIO_CloseFile(f);

	bmp_printf(FONT_MED, 100, 100, "Psst! Just took a slit-scan pic   ");

	if (!interactive) return;
	// wait half-shutter press and clear the screen
	while (!get_halfshutter_pressed()) msleep(100);
	clrscr();
	while (get_halfshutter_pressed()) msleep(100);
	clrscr();
}

static void
silent_pic_take(int interactive) // for remote release, set interactive=0
{
	if (!lv_drawn()) return;
	
	int g = get_global_draw();
	set_global_draw(0);

	if (silent_pic_mode == 1) // normal
		silent_pic_take_simple();
	//~ else if (silent_pic_mode == 2) // hi-res
		//~ silent_pic_take_sweep();
	//~ else if (silent_pic_mode == 3) // long exposure
		//~ silent_pic_take_longexp();
	else if (silent_pic_mode == 4) // slit-scan
		silent_pic_take_slitscan(interactive);

	set_global_draw(g);
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
		return 1;
	}
	return 0;
}

int is_round_iso(int iso)
{
	return is_native_iso(iso) || is_lowgain_iso(iso);
}

CONFIG_INT("iso.round.only", iso_round_only, 0);


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

PROP_INT(PROP_ISO_AUTO, iso_auto_code);
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
	while (iso_auto_code == 0)
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
}
static void iso_auto_quick()
{
	if (MENU_MODE) return;
	lens_set_rawiso(measure_auto_iso());
}

int iso_auto_flag = 0;
static void iso_auto()
{
	if (lv_drawn()) iso_auto_flag = 1; // it takes some time, so it's better to do it in another task
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
	if (!lv_drawn()) return 0;

	if (iso_index >= 0)
	{
		lens_set_rawiso(codes_iso[iso_index]);
		msleep(500);
		menu_show_only_selected();
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
	menu_show_only_selected();
	return under - over;
}

static void iso_auto_run()
{
	menu_show_only_selected();
	if (lens_info.raw_iso == 0) { lens_set_rawiso(96); msleep(500); }
	int c0 = crit_iso(-1); // test current iso
	int i;
	if (c0 > 0) i = bin_search(raw2index_iso(lens_info.raw_iso), COUNT(codes_iso), crit_iso);
	else i = bin_search(get_htp() ? 9 : 1, raw2index_iso(lens_info.raw_iso)+1, crit_iso);
	lens_set_rawiso(codes_iso[i]);
	//~ clrscr();
}


static void 
shutter_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Shutter     : 1/%d",
		lens_info.shutter
	);
	bmp_printf(FONT_MED, x + 550, y+5, "[Q]=Auto");
	menu_draw_icon(x, y, MNI_PERCENT, (lens_info.raw_shutter - codes_shutter[1]) * 100 / (codes_shutter[COUNT(codes_shutter)-1] - codes_shutter[1]));
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
		msleep(100);
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
	if (lv_drawn()) shutter_auto_flag = 1; // it takes some time, so it's better to do it in another task
	else 
	{
		shutter_auto_quick();
		shutter_auto_quick();
	}
}

int crit_shutter(int shutter_index)
{
	if (!lv_drawn()) return 0;

	if (shutter_index >= 0)
	{
		lens_set_rawshutter(codes_shutter[shutter_index]);
		menu_show_only_selected();
		msleep(500);
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
	menu_show_only_selected();
	return over - under;
}

static void shutter_auto_run()
{
	menu_show_only_selected();
	int c0 = crit_shutter(-1); // test current shutter
	int i;
	if (c0 > 0) i = bin_search(raw2index_shutter(lens_info.raw_shutter), COUNT(codes_shutter), crit_shutter);
	else i = bin_search(1, raw2index_shutter(lens_info.raw_shutter)+1, crit_shutter);
	lens_set_rawshutter(codes_shutter[i]);
	//~ clrscr();
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
	menu_draw_icon(x, y, lens_info.aperture ? MNI_PERCENT : MNI_WARNING, (lens_info.raw_aperture - codes_aperture[1]) * 100 / (codes_shutter[COUNT(codes_aperture)-1] - codes_aperture[1]));
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
	if (lv_drawn()) kelvin_auto_flag = 1;
	else
	{
		bmp_printf(FONT_LARGE, 20,450, "Only works in LiveView");
		msleep(1000);
		bmp_printf(FONT_LARGE, 20,450, "                      ");
	}
}

static void wbs_gm_auto()
{
	if (lv_drawn()) wbs_gm_auto_flag = 1;
	else
	{
		bmp_printf(FONT_LARGE, 20,450, "Only works in LiveView");
		msleep(1000);
		bmp_printf(FONT_LARGE, 20,450, "                      ");
	}
}

int crit_kelvin(int k)
{
	if (!lv_drawn()) return 0;

	if (k > 0)
	{
		lens_set_kelvin(k * KELVIN_STEP);
		msleep(500);
		menu_show_only_selected();
	}

	int Y, U, V;
	get_spot_yuv(100, &Y, &U, &V);

	menu_show_only_selected();

	int R = Y + 1437 * V / 1024;
	//~ int G = Y -  352 * U / 1024 - 731 * V / 1024;
	int B = Y + 1812 * U / 1024;

	return B - R;
}

int crit_wbs_gm(int k)
{
	if (!lv_drawn()) return 0;

	if (k < 10 && k > -10)
	{
		lens_set_wbs_gm(k);
		msleep(500);
		menu_show_only_selected();
	}

	int Y, U, V;
	get_spot_yuv(100, &Y, &U, &V);

	int R = Y + 1437 * V / 1024;
	int G = Y -  352 * U / 1024 - 731 * V / 1024;
	int B = Y + 1812 * U / 1024;

	menu_show_only_selected();
	return (R+B)/2 - G;
}

static void kelvin_auto_run()
{
	menu_show_only_selected();
	int c0 = crit_kelvin(-1); // test current kelvin
	int i;
	if (c0 > 0) i = bin_search(lens_info.kelvin/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
	else i = bin_search(KELVIN_MIN/KELVIN_STEP, lens_info.kelvin/KELVIN_STEP + 1, crit_kelvin);
	lens_set_kelvin(i * KELVIN_STEP);
	//~ clrscr();
}

static void wbs_gm_auto_run()
{
	menu_show_only_selected();
	int c0 = crit_wbs_gm(100); // test current value
	int i;
	if (c0 > 0) i = bin_search(lens_info.wbs_gm, 10, crit_wbs_gm);
	else i = bin_search(-9, lens_info.wbs_gm + 1, crit_wbs_gm);
	lens_set_wbs_gm(i);
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
	menu_draw_icon(x, y, MNI_PERCENT, (lens_get_sharpness() + 4) * 100 / 8);
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
			"Saturation  : 0x%X",
		s
	);
	menu_draw_icon(x, y, s >= -4 && s <= 4 ? MNI_PERCENT : MNI_OFF, (s + 4) * 100 / 8);
}

static void 
picstyle_display( void * priv, int x, int y, int selected )
{
	int p = lens_info.raw_picstyle;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"PictureStyle: %s ",
		p == 0x81 ? "Standard" : 
		p == 0x82 ? "Portrait" :
		p == 0x83 ? "Landscape" :
		p == 0x84 ? "Neutral" :
		p == 0x85 ? "Faithful" :
		p == 0x86 ? "Monochrome" :
		p == 0x21 ? "User Def 1" :
		p == 0x22 ? "User Def 2" :
		p == 0x23 ? "User Def 3" : "Unknown"
	);
	menu_draw_icon(x, y, MNI_ON, 0);
}

static void
picstyle_toggle( int sign )
{
	int p = lens_info.picstyle;
	p = mod(p + sign - 1, 9) + 1;
	p = get_prop_picstyle_from_index(p);
	if (p) prop_request_change(PROP_PICTURE_STYLE, &p, 4);
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

PROP_INT(PROP_STROBO_AECOMP, flash_ae);

static void
flash_ae_toggle( int sign )
{
	int ae = (int8_t)flash_ae;
	int newae = ae + sign * (ABS(ae) <= 16 ? 4 : 8);
	if (newae > 24) newae = -80;
	if (newae < -80) newae = 24;
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
		"Flash AEcomp: %d.%d EV",
		ae_ev / 10, 
		ABS(ae_ev % 10)
	);
	menu_draw_icon(x, y, MNI_PERCENT, (ae_ev + 80) * 100 / (24+80));
}


uint32_t cfn[4];
PROP_HANDLER( PROP_CFN )
{
	cfn[0] = buf[0];
	cfn[1] = buf[1];
	cfn[2] = buf[2];
	cfn[3] = buf[3] & 0xFF;
	//~ bmp_printf(FONT_MED, 0, 450, "cfn: %x/%x/%x/%x", cfn[0], cfn[1], cfn[2], cfn[3]);
	return prop_cleanup( token, property );
}

int get_htp()
{
	if (cfn[1] & 0x10000) return 1;
	return 0;
}

void set_htp(int enable)
{
	if (enable) cfn[1] |= 0x10000;
	else cfn[1] &= ~0x10000;
	prop_request_change(PROP_CFN, cfn, 0xD);
}

void set_mlu(int enable)
{
	if (enable) cfn[2] |= 0x1;
	else cfn[2] &= ~0x1;
	prop_request_change(PROP_CFN, cfn, 0xD);
}
int get_mlu()
{
	return cfn[2] & 0x1;
}

void set_alo(int value)
{
	value = COERCE(value, 0, 3);
	prop_request_change(PROP_ALO, &value, 4);
}

// 0 = off, 1 = alo, 2 = htp
int get_ladj()
{
	if (get_htp()) return 2;
	if (alo != ALO_OFF) return 1;
	return 0;
}

static void
ladj_toggle( int sign )
{
	int ladj = get_ladj();
	ladj = mod(ladj + sign, 3);
	if (ladj == 0)
	{
		set_htp(0);
		set_alo(ALO_OFF);
	}
	else if (ladj == 1)
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

static void 
ladj_display( void * priv, int x, int y, int selected )
{
	int htp = get_htp();
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
	menu_draw_icon(x, y, MNI_BOOL(zoom_enable_face || zoom_disable_x5 || zoom_disable_x10), 0);
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

int hdr_steps = 1;
CONFIG_INT("hdr.stepsize", hdr_stepsize, 8);

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
	if (shooting_mode == SHOOTMODE_BULB) return 1;
	if (shooting_mode != SHOOTMODE_M) return 0;
	if (lens_info.raw_shutter != 0xC) return 0;
	return 1;
}
static void
bulb_take_pic(int duration)
{
	if (!is_bulb_mode())
	{
		bmp_printf(FONT_LARGE, 0, 30, "Pls select bulb mode");
		return;
	}
	if (drive_mode != DRIVE_SINGLE) lens_set_drivemode(DRIVE_SINGLE);
	if (get_mlu()) { lens_take_picture(64); msleep(2000); }
	SW1(1,100);
	SW2(1,100);
	int i;
	int d = duration/1000;
	for (i = 0; i < d; i++)
	{
		bmp_printf(FONT_LARGE, 30, 30, "Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
		msleep(1000);
		if (lens_info.job_state == 0) break;
	}
	SW2(0,100);
	SW1(0,100);
}

static void bulb_toggle_fwd(void* priv)
{
	bulb_duration_index = mod(bulb_duration_index + 1, COUNT(timer_values));
}
static void bulb_toggle_rev(void* priv)
{
	bulb_duration_index = mod(bulb_duration_index - 1, COUNT(timer_values));
}

static void
bulb_display( void * priv, int x, int y, int selected )
{
	int d = timer_values[bulb_duration_index];
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Bulb Timer %s: %d%s",
		is_bulb_mode() ? "     " : "(N/A)",
		d < 60 ? d : d/60, 
		d < 60 ? "s" : "min"
	);
	menu_draw_icon(x, y, !bulb_duration_index ? MNI_OFF : is_bulb_mode() ? MNI_PERCENT : MNI_WARNING, bulb_duration_index * 100 / COUNT(timer_values));
}

static void
mlu_display( void * priv, int x, int y, int selected )
{
	//~ int d = timer_values[bulb_duration_index];
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Mirror Lockup   : %s",
		mlu_mode == 1 ? "ON" : mlu_mode == 2 ? "Timer+Remote" : "OFF"
	);
	menu_draw_icon(x, y, MNI_BOOL_AUTO(mlu_mode), 0);
}

static void 
intervalometer_wait_toggle(void* priv)
{
	intervalometer_wait = !intervalometer_wait;
}

#ifdef CONFIG_SRAW
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

struct menu_entry shoot_menus[] = {
	{
		.display	= hdr_display,
		.select		= hdr_steps_toggle,
		.select_reverse = hdr_stepsize_toggle,
		.select_auto = hdr_reset,
		.help = "Exposure bracketing, useful for HDR images."
	},
	{
		.priv		= &interval_timer_index,
		.display	= interval_timer_display,
		.select		= interval_timer_toggle,
		.select_reverse	= interval_timer_toggle_reverse,
		.select_auto = interval_movie_duration_toggle,
		.help = "Intervalometer setting: duration between two shots."
	},
	{
		.priv		= &intervalometer_running,
		.select		= menu_binary_toggle,
		.display	= intervalometer_display,
		.select_auto = intervalometer_wait_toggle,
		.help = "Intervalometer. For precise timing, choose NoWait [Q]."
	},
	{
		.priv		= &lcd_release_running,
		.select		= menu_quaternary_toggle, 
		.select_reverse = menu_quaternary_toggle_reverse,
		.display	= lcd_release_display,
		.help = "Use the LCD sensor as a remote trigger. Avoids shake."
	},
 	{
		.priv		= &audio_release_running,
		.select		= menu_binary_toggle,
		.display	= audio_release_display,
		.select_auto = audio_release_level_toggle, 
		.select_reverse = audio_release_level_toggle_reverse,
		.help = "Clap your hands or pop a balloon to take a picture."
	},
	{
		.priv		= &motion_detect,
		.select		= menu_ternary_toggle, 
		.display	= motion_detect_display,
		.select_auto = motion_release_level_toggle, 
		.select_reverse = motion_release_level_toggle_reverse,
		.help = "LV Motion detection: EXPosure change / frame DIFference."
	},
/*	{
		.select		= flash_and_no_flash_toggle,
		.display	= flash_and_no_flash_display,
		.help = "Take odd pictures with flash, even pictures without flash."
	},*/
	{
		.priv = &silent_pic_mode,
		.select = silent_pic_mode_toggle,
		.select_reverse = silent_pic_toggle_reverse,
		.select_auto = silent_pic_toggle_forward,
		.display = silent_pic_display,
		.help = "Take pics in LiveView without increasing shutter count."
	},
	{
		.display = bulb_display, 
		.select = bulb_toggle_fwd, 
		.select_reverse = bulb_toggle_rev,
		.select_auto = bulb_toggle_fwd,
		.help = "Bulb timer for very long exposures, useful for astrophotos"
	},
	{
		.priv = &mlu_mode,
		.display = mlu_display, 
		.select = menu_ternary_toggle,
		.help = "MLU setting can be linked with self-timer and LCD remote."
	},
	/*{
		.display = picq_display, 
		.select = picq_toggle_raw,
		.select_reverse = picq_toggle_jpegsize, 
		.select_auto = picq_toggle_jpegtype,
	}*/
#ifdef CONFIG_SRAW
	{
		.display = picq_display, 
		.select = picq_toggle, 
		.help = "Experimental SRAW/MRAW mode. You may get corrupted files."
	}
#endif
};

static struct menu_entry vid_menus[] = {
	{
		.priv = &zoom_enable_face,
		.select = menu_binary_toggle,
		.select_reverse = zoom_toggle, 
		.display = zoom_display,
		.help = "Disable x5 or x10, or enable zoom during Face Detection :)"
	},
};

struct menu_entry expo_menus[] = {
	{
		.display	= iso_display,
		.select		= iso_toggle_forward,
		.select_reverse		= iso_toggle_reverse,
		.select_auto = iso_auto,
		.help = "Adjust ISO in 1/8EV steps. Press [Q] for auto tuning."
	},
	{
		.display	= kelvin_display,
		.select		= kelvin_toggle_forward,
		.select_reverse		= kelvin_toggle_reverse,
		.select_auto = kelvin_auto,
		.help = "Adjust Kelvin WB. Press [Q] for auto tuning."
	},
	{
		.display = wbs_gm_display, 
		.select = wbs_gm_toggle_forward, 
		.select_reverse = wbs_gm_toggle_reverse,
		.select_auto = wbs_gm_auto,
		.help = "Green-Magenta white balance shift, for fluorescent lights."
	},
	{
		.display	= shutter_display,
		.select		= shutter_toggle_forward,
		.select_reverse		= shutter_toggle_reverse,
		.select_auto = shutter_auto,
		.help = "Shutter in 1/8EV steps. ML shows it with 2 nonzero digits."
	},
	{
		.display	= aperture_display,
		.select		= aperture_toggle_forward,
		.select_reverse		= aperture_toggle_reverse,
		.help = "Adjust aperture. Useful if the wheel stops working."
	},
	{
		.display	= ladj_display,
		.select		= ladj_toggle_forward,
		.select_reverse		= ladj_toggle_reverse,
		.help = "Enable/disable HTP and ALO from the same place."
	},
	{
		.display	= picstyle_display,
		.select		= picstyle_toggle_forward,
		.select_reverse		= picstyle_toggle_reverse,
		.help = "Change current picture style."
	},
	{
		.display	= contrast_display,
		.select		= contrast_toggle_forward,
		.select_reverse		= contrast_toggle_reverse,
		//~ .select_auto = contrast_auto,
		.help = "Adjust contrast in current picture style."
	},
	{
		.display	= saturation_display,
		.select		= saturation_toggle_forward,
		.select_reverse		= saturation_toggle_reverse,
		.help = "Adjust saturation in current picture style."
	},
	{
		.display	= flash_ae_display,
		.select		= flash_ae_toggle_forward,
		.select_reverse		= flash_ae_toggle_reverse,
		.help = "Flash exposure compensation, from -10EV to +3EV."
	},
	/*{
		.display	= sharpness_display,
		.select		= sharpness_toggle_forward,
		.select_reverse		= sharpness_toggle_reverse,
	},*/
};

void hdr_create_script(int steps, int skip0, int focus_stack)
{
	if (steps <= 1) return;
	DEBUG();
	FILE * f = INVALID_PTR;
	char name[100];
	int f0 = skip0 ? file_number_also : file_number_also+1;
	snprintf(name, sizeof(name), "B:/DCIM/%03dCANON/%s_%04d.sh", folder_number, focus_stack ? "FST" : "HDR", f0);
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

void hdr_shutter_release()
{
	lens_wait_readytotakepic(64);
	if (!silent_pic_mode || !lv_drawn())
	{
		if (get_mlu()) { lens_take_picture(64); msleep(500); }
		lens_take_picture(64);
	}
	else { msleep(300); silent_pic_take(0); }
}

// skip0: don't take the middle exposure
void hdr_take_pics(int steps, int step_size, int skip0)
{
	if (step_size) hdr_create_script(steps, skip0, 0);
	int i;
	if ((lens_info.iso && shooting_mode == SHOOTMODE_M) || (shooting_mode == SHOOTMODE_MOVIE))
	{
		const int s = lens_info.raw_shutter;
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			if (skip0 && (i == 0)) continue;
			bmp_printf(FONT_LARGE, 0, 200, "%d   ", i);
			int new_s = COERCE(s - step_size * i, 0x10, 160);
			lens_set_rawshutter( new_s );
			hdr_shutter_release();
		}
		msleep(100);
		lens_set_rawshutter( s );
	}
	else
	{
		const int ae = lens_get_ae();
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			if (skip0 && (i == 0)) continue;
			bmp_printf(FONT_LARGE, 0, 200, "%d   ", i);
			int new_ae = ae + step_size * i;
			lens_set_ae( new_ae );
			hdr_shutter_release();
		}
		lens_set_ae( ae );
	}
}

void movie_start()
{
	if (shooting_type != 3 && shooting_mode != SHOOTMODE_MOVIE)
	{
		bmp_printf(FONT_LARGE, 30, 30, "Not in movie (%d,%d) ", shooting_type, shooting_mode);
		return;
	}
	if (recording)
	{
		bmp_printf(FONT_LARGE, 30, 30, "Already recording ");
		return;
	}
	
	while (get_halfshutter_pressed()) msleep(100);

	call("MovieStart");
	while (recording != 2) msleep(100);
	msleep(500);
}

void movie_end()
{
	if (shooting_type != 3 && shooting_mode != SHOOTMODE_MOVIE)
	{
		bmp_printf(FONT_LARGE, 30, 30, "Not in movie (%d,%d) ", shooting_type, shooting_mode);
		return;
	}
	if (!recording)
	{
		bmp_printf(FONT_LARGE, 30, 30, "Not recording ");
		return;
	}

	while (get_halfshutter_pressed()) msleep(100);
	msleep(500);

	call("MovieEnd");

	while (recording) msleep(100);
	msleep(500);
}

static void
hdr_take_mov(int steps, int step_size)
{
	int g = get_global_draw();
	set_global_draw(0);
	clrscr();

	movie_start();
	int i;
	const int s = lens_info.raw_shutter;
	for( i = -steps/2; i <= steps/2; i ++  )
	{
		bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
		int new_s = COERCE(s - step_size * i, 96, 152);
		lens_set_rawshutter( new_s );
		msleep(timer_values[interval_movie_duration_index] * 1000);
	}
	lens_set_rawshutter( s );
	movie_end();
	set_global_draw(g);
}

// take a HDR shot (sequence of stills or a small movie)
// to be used with the intervalometer
void hdr_shot(int skip0, int wait)
{
	//~ bmp_printf(FONT_LARGE, 50, 50, "SKIP%d", skip0);
	//~ msleep(2000);
	if (is_bulb_mode())
	{
		bulb_take_pic(timer_values[bulb_duration_index] * 1000);
	}
	else if (shooting_mode == SHOOTMODE_MOVIE && !silent_pic_mode)
	{
		hdr_take_mov(hdr_steps, hdr_stepsize);
	}
	else if (hdr_steps > 1)
	{
		if (drive_mode != DRIVE_SINGLE && drive_mode != DRIVE_CONTINUOUS) 
			lens_set_drivemode(DRIVE_CONTINUOUS);
		if (hdr_steps == 2)
			hdr_take_pics(hdr_steps, hdr_stepsize/2, 1);
		else
			hdr_take_pics(hdr_steps, hdr_stepsize, skip0);
		while (lens_info.job_state) msleep(500);
	}
	else // regular pic
	{
		if (wait)
		{
			hdr_take_pics(0,0,0);
		}
		else
		{
			if (!silent_pic_mode || !lv_drawn()) lens_take_picture(0);
			else silent_pic_take(0);
			return;
		}
	}

	while (lens_info.job_state) msleep(500);

}

int remote_shot_flag = 0;
void schedule_remote_shot() { remote_shot_flag = 1; }

int movie_start_flag = 0;
void schedule_movie_start() { movie_start_flag = 1; }
int is_movie_start_scheduled() { return movie_start_flag; }

int movie_end_flag = 0;
void schedule_movie_end() { movie_end_flag = 1; }

void get_out_of_play_mode()
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
	msleep(500);
}

// take one shot, a sequence of HDR shots, or start a movie
// to be called by remote triggers
void remote_shot()
{
	// save zoom value (x1, x5 or x10)
	int zoom = lv_dispsize;
	
	if (is_bulb_mode())
	{
		bulb_take_pic(timer_values[bulb_duration_index] * 1000);
	}
	else if (hdr_steps > 1)
	{
		hdr_shot(0,1);
	}
	else
	{
		if (silent_pic_mode && lv_drawn())
			silent_pic_take(0);
		else if (shooting_mode == SHOOTMODE_MOVIE)
			movie_start();
		else
			lens_take_picture(64); // hdr_shot messes with the self timer mode
	}
	msleep(200);
	if (get_mlu() && lens_info.job_state < 10) return;
	
	while (lens_info.job_state >= 10) msleep(500);
	
	msleep(1000);
	while (gui_state != GUISTATE_IDLE) msleep(100);
	msleep(500);
	// restore zoom
	if (lv_drawn() && !recording && zoom > 1) prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
}

void iso_refresh_display()
{
	if (lv_drawn())
	{
		update_lens_display(lens_info);
		return;
	}
	int bg = bmp_getpixel(680, 40);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	int iso = lens_info.iso;
	if (iso)
		bmp_printf(fnt, 470, 27, "ISO %5d", iso);
	else
		bmp_printf(fnt, 470, 27, "ISO AUTO");
}

void display_shooting_info() // called from debug task
{
	if (lv_drawn()) return;
	
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 320, 260, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 435, 240, "%s%d ", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(fnt, 435, 240, "   ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 435, 270, "%s%d ", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(fnt, 435, 270, "   ");
	}

	iso_refresh_display();

	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	if (hdr_steps > 1)
		bmp_printf(fnt, 380, 450, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 380, 450, "           ");

	//~ bmp_printf(fnt, 200, 450, "Flash:%s", 
		//~ strobo_firing == 0 ? " ON" : 
		//~ strobo_firing == 1 ? "OFF" : "Auto"
		//~ strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
		//~ );

	bmp_printf(fnt, 40, 460, get_mlu() ? "MLU" : "   ");

	display_lcd_remote_icon(480, 0);
	display_trap_focus_info();
}

void display_shooting_info_lv()
{
	display_lcd_remote_icon(480, 0);
	display_trap_focus_info();
}

void display_trap_focus_info()
{
	int show, fg, bg, x, y;
	if (lv_drawn())
	{
		show = trap_focus && can_lv_trap_focus_be_active();
		int active = show && get_halfshutter_pressed();
		bg = active ? COLOR_BG : 0;
		fg = active ? COLOR_RED : COLOR_BG;
		x = 8; y = 160;
	}
	else
	{
		show = (trap_focus && ((af_mode & 0xF) == 3) && lens_info.raw_aperture);
		bg = bmp_getpixel(410, 330);
		fg = trap_focus == 2 || FOCUS_CONFIRMATION_AF_PRESSED ? COLOR_RED : COLOR_FG_NONLV;
		x = 410; y = 331;
	}
	static int show_prev = 0;
	if (show || show_prev) bmp_printf(FONT(FONT_MED, fg, bg), x, y, show ? "TRAP \nFOCUS" : "     \n     ");
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

int wave_count = 0;
int wave_count_countdown = 0;
PROP_HANDLER(PROP_DISPSENSOR_CTRL)
{
	static int prev = 0;
	int on = !buf[0];
	int off = !on;
	if (on == prev) // false alarm
		goto end;
	prev = on;
	
	if (remote_shot_flag) goto end;
	
	if (lcd_release_running && gui_state == GUISTATE_IDLE && !intervalometer_running)
	{
		if (gui_menu_shown()) goto end;
		
		if (lcd_release_running == 1 && off) goto end;
		if (lcd_release_running == 2 && on && !get_mlu()) goto end;
		if (lcd_release_running == 3) { wave_count++; wave_count_countdown = 50; }
		if (lcd_release_running == 3 && wave_count < 5) goto end;

		if (lcd_release_running == 3 && recording) schedule_movie_end(); // wave mode is allowed to stop movies
		else schedule_remote_shot();
		wave_count = 0;
	}
	else wave_count = 0;

	end:
	return prop_cleanup(token, property);
}

void display_lcd_remote_icon(int x0, int y0)
{
	int cl_on = COLOR_RED;
	int cl_off = lv_drawn() ? COLOR_WHITE : COLOR_FG_NONLV;
	int cl = display_sensor ? cl_on : cl_off;
	int bg = lv_drawn() ? 0 : bmp_getpixel(x0 - 20, 1);

	if (gui_menu_shown()) cl = COLOR_WHITE;

	if (lcd_release_running == 1)
	{
		draw_circle(x0, 10+y0, 8, cl);
		draw_circle(x0, 10+y0, 7, cl);
		draw_line(x0-5,10-5+y0,x0+5,10+5+y0,cl);
		draw_line(x0-5,10+5+y0,x0+5,10-5+y0,cl);
	}
	else if (lcd_release_running == 2)
	{
		draw_circle(x0, 10+y0, 8, cl);
		draw_circle(x0, 10+y0, 7, cl);
		draw_circle(x0, 10+y0, 1, cl);
		draw_circle(x0, 10+y0, 2, cl);
	}
	else if (lcd_release_running == 3)
	{
		int yup = 5+y0;
		int ydn = 15+y0;
		int step = 5;
		int k;
		for (k = 0; k < 2; k++)
		{
			draw_line(x0 - 2*step, ydn, x0 - 1*step, yup, wave_count > 0 ? cl_on : cl_off);
			draw_line(x0 - 1*step, yup, x0 - 0*step, ydn, wave_count > 1 ? cl_on : cl_off);
			draw_line(x0 - 0*step, ydn, x0 + 1*step, yup, wave_count > 2 ? cl_on : cl_off);
			draw_line(x0 + 1*step, yup, x0 + 2*step, ydn, wave_count > 3 ? cl_on : cl_off);
			draw_line(x0 + 2*step, ydn, x0 + 3*step, yup, wave_count > 4 ? cl_on : cl_off);
			x0++;
		}
	}
	
	if (gui_menu_shown()) return;
	
	static unsigned int prev_lr = 0;
	if (prev_lr != lcd_release_running) bmp_fill(bg, x0 - 20, y0, 40, 20);
	prev_lr = lcd_release_running;
}


void intervalometer_stop()
{
	if (intervalometer_running)
	{
		bmp_printf(FONT_MED, 20, (lv_drawn() ? 40 : 3), "Stopped                                             ");
		intervalometer_running = 0;
		//~ display_on();
	}
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
	}
}

int sw1_pressed = 0;

static void
shoot_task( void* unused )
{
	int i = 0;
	menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
	menu_add( "Expo", expo_menus, COUNT(expo_menus) );
	msleep(1000);
	menu_add( "Tweak", vid_menus, COUNT(vid_menus) );
	
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
		
		if (wave_count_countdown)
		{
			wave_count_countdown--;
			if (!wave_count_countdown) wave_count = 0;
		}
		
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
		if (remote_shot_flag)
		{
			remote_shot();
			remote_shot_flag = 0;
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
		
		if (!lv_drawn()) // MLU
		{
			if (mlu_mode == 0 && get_mlu()) set_mlu(0);
			if (mlu_mode == 1 && !get_mlu()) set_mlu(1);
			if (mlu_mode == 2)
			{
				if ((drive_mode == DRIVE_SELFTIMER_2SEC || drive_mode == DRIVE_SELFTIMER_REMOTE || lcd_release_running == 2) && (hdr_steps < 2))
				{
					if (!get_mlu()) set_mlu(1);
				}
				else
				{
					if (get_mlu()) set_mlu(0);
				}
			}
		}
		
		if (lv_drawn() && face_zoom_request && lv_dispsize == 1 && !recording)
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
		
		if (hdr_steps > 1 && get_halfshutter_pressed() && drive_mode != DRIVE_SELFTIMER_2SEC)
		{
			drive_mode_bk = drive_mode;
			lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
		}

		// restore drive mode if it was changed
		if (!get_halfshutter_pressed() && drive_mode_bk >= 0)
		{
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
				int d = timer_values[bulb_duration_index];
				while (get_halfshutter_pressed())
				{
					bmp_printf(FONT_LARGE, 0, 0, "[HS] Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
					msleep(100);
				}
				bmp_printf(FONT_LARGE, 0, 0, "[2s] Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
				msleep(1000);
				if (gui_state != GUISTATE_IDLE) continue;
				bmp_printf(FONT_LARGE, 0, 0, "[1s] Bulb timer: %d%s", d < 60 ? d : d/60, d < 60 ? "s" : "min");
				msleep(1000);
				if (gui_state != GUISTATE_IDLE) continue;
				bulb_take_pic(d * 1000);
			}
			was_idle_not_pressed = is_idle_not_pressed;
		}

		if (lens_info.job_state) // just took a picture, maybe we should take another one
		{
			if (hdr_steps > 1) hdr_shot(1,1); // skip the middle exposure, which was just taken
		}

		// toggle flash on/off for next picture
		/*if (shooting_mode != SHOOTMODE_MOVIE && flash_and_no_flash && strobo_firing < 2 && strobo_firing != file_number % 2)
		{
			strobo_firing = file_number % 2;
			set_flash_firing(strobo_firing);
		}*/

		//~ static int sw1_countdown = 0;
		
		// trap focus (outside LV) and all the preconditions
		int tfx = trap_focus && (af_mode & 0xF) == 3 && gui_state == GUISTATE_IDLE && !gui_menu_shown() && !intervalometer_running;

		// same for motion detect
		int mdx = motion_detect && gui_state == GUISTATE_IDLE && !gui_menu_shown() && lv_drawn();
		
		//Reset the counter so that if you go in and out of live view, it doesn't start clicking away right away.
		static int K = 0;

		if(!mdx) K = 0;
		// emulate half-shutter press (for trap focus or motion detection)
		/* this can cause the camera not to shutdown properly... 
		if (!lv_drawn() && ((tfx && trap_focus == 2) || mdx ))
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
			if ((!lv_drawn() && FOCUS_CONFIRMATION) || get_lv_focus_confirmation())
			{
				remote_shot();
				msleep(trap_focus_delay);
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
					remote_shot();
					msleep(trap_focus_delay);
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
					remote_shot();
					msleep(trap_focus_delay);
					K = 0;
				}
			}
		}

		if (silent_pic_mode && lv_drawn() && get_halfshutter_pressed())
		{
			silent_pic_take(1);
		}

		// force powersave mode for intervalometer in silent mode
		if (intervalometer_running)
		{
			if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			
			if (timer_values[interval_timer_index])
			{
				card_led_blink(5, 50, 50);
				wait_till_next_second();
			}
			
			if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			if (get_halfshutter_pressed()) continue;

			hdr_shot(0, intervalometer_wait);
			
			// simulate a half-shutter press to avoid mirror going up
			// once per minute is enough and easy to check
			static int prev_min = 0;
			LoadCalendarFromRTC( &now );
			if (lv_drawn() && now.tm_min != prev_min) 
			{
				prev_min = now.tm_min;
				SW1(1,10);
				SW1(0,10);
			}
			
			for (i = 0; i < timer_values[interval_timer_index] - 1; i++)
			{
				card_led_blink(1, 50, 0);
				wait_till_next_second();

				if (intervalometer_running) bmp_printf(FONT_MED, 20, (lv_drawn() ? 40 : 3), "Press PLAY or MENU to stop the intervalometer...%d   ", timer_values[interval_timer_index] - i - 1);
				else break;

				if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;

				while (get_halfshutter_pressed()) msleep(100);
				
				if (!lv_drawn())
				{
					SW1(1,10); // prevent camera from entering in "deep sleep" mode
					SW1(0,10); // (some kind of sleep where it won't wake up from msleep)
				}

				//~ if (shooting_mode != SHOOTMODE_MOVIE)
				//~ {
					//~ if (lens_info.shutter > 100 && !silent_pic_mode) bmp_printf(FONT_MED, 0, 70,             "Tip: use shutter speeds slower than 1/100 to prevent flicker.");
					//~ else if (shooting_mode != SHOOTMODE_M || lens_info.iso == 0) bmp_printf(FONT_MED, 0, 70, "Tip: use fully manual exposure to prevent flicker.           ");
					//~ else if ((af_mode & 0xF) != 3) bmp_printf(FONT_MED, 0, 70,                               "Tip: use manual focus                                        ");
				//~ }
			}
		}
		else
		{
			if (audio_release_running) 
			{
				static int countdown = 0;
				if (gui_state != GUISTATE_IDLE || gui_menu_shown()) countdown = 50;
				if (countdown) { countdown--; continue; }

				extern struct audio_level audio_levels[];
				bmp_printf(FONT_MED, 20, lv_drawn() ? 40 : 3, "Audio release ON (%d / %d)   ", audio_levels[0].peak / audio_levels[0].avg, audio_release_level);
				if (audio_levels[0].peak > audio_levels[0].avg * (int)audio_release_level) 
				{
					remote_shot();
					while (lens_info.job_state) msleep(100);
				}
			}
		}
	}
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x1a, 0x1000 );
