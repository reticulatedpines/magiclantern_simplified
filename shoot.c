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

CONFIG_INT( "interval.timer.index", interval_timer_index, 2 );
CONFIG_INT( "focus.trap", trap_focus, 1);
CONFIG_INT( "focus.trap.delay", trap_focus_delay, 500); // min. delay between two shots in trap focus
CONFIG_INT( "audio.release.level", audio_release_level, 700);
CONFIG_INT( "interval.movie.duration.index", interval_movie_duration_index, 2);
CONFIG_INT( "flash_and_no_flash", flash_and_no_flash, 0);
CONFIG_INT( "silent.pic.mode", silent_pic_mode, 0 );        // 0 = off, 1 = normal, 2 = hi-res, 3 = slit-scan
CONFIG_INT( "silent.pic.burst", silent_pic_burst, 0);       // boolean
CONFIG_INT( "silent.pic.highres", silent_pic_highres, 0);   // index of matrix size (2x1 .. 5x5)
CONFIG_INT( "silent.pic.sweepdelay", silent_pic_sweepdelay, 350);
CONFIG_INT( "silent.pic.slitscan.skipframes", silent_pic_slitscan_skipframes, 1);
CONFIG_INT( "zoom.enable.face", zoom_enable_face, 1);
CONFIG_INT( "zoom.disable.x5", zoom_disable_x5, 0);
CONFIG_INT( "zoom.disable.x10", zoom_disable_x10, 0);
CONFIG_INT( "bulb.duration", bulb_duration, 5000);

int intervalometer_running = 0;
int lcd_release_running = 0;
int audio_release_running = 0;

int drive_mode_bk = -1;
PROP_INT(PROP_DRIVE, drive_mode);
PROP_INT(PROP_AF_MODE, af_mode);
PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
PROP_INT(PROP_MVR_REC_START, recording);
PROP_INT(PROP_WBS_GM, wbs_gm);
PROP_INT(PROP_WBS_BA, wbs_ba);
PROP_INT(PROP_FILE_NUMBER, file_number);
PROP_INT(PROP_FILE_NUMBER_ALSO, file_number_also);
PROP_INT(PROP_FOLDER_NUMBER, folder_number);
PROP_INT(PROP_STROBO_FIRING, strobo_firing);
PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
PROP_INT(PROP_LVAF_MODE, lvaf_mode);
PROP_INT(PROP_GUI_STATE, gui_state);
PROP_INT(PROP_REMOTE_SW1, remote_sw1);

int timer_values[] = {1,2,5,10,15,20,30,60,300,900,3600};

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
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Take a pic every: %ds",
			timer_values[*(int*)priv]
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
		bmp_printf(FONT_MED, x + 510, y+5, "[Q]");
	}
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
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Intervalometer  : %s",
		(*(int*)priv) ? "ON " : "OFF"
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
		v == 1 ? "Near" : (v == 2 ? "Away" : "OFF")
	);
}

static void
audio_release_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Audio RemoteShot: %s",
		audio_release_running ? "ON " : "OFF"
	);
}

static void 
trap_focus_display( void * priv, int x, int y, int selected )
{
	int t = (*(int*)priv);
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Trap Focus      : %s",
		t == 1 ? "Hold" : t == 2 ? "Cont." : "OFF"
	);
}

void set_flash_firing(int mode)
{
	DEBUG("%d", mode);
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
}

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
			silent_pic_burst ? "Burst" : "Single"
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
	else if (silent_pic_mode == 3)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Slit-scan Pic   : 1ln/%dclk",
			silent_pic_slitscan_skipframes
		);
	}
}

static void silent_pic_mode_toggle(void* priv)
{
	silent_pic_mode = mod(silent_pic_mode + 1, 4); // off, normal, hi-res, slit
}

static void silent_pic_toggle(int sign)
{
	if (silent_pic_mode == 1)
		silent_pic_burst = !silent_pic_burst;
	else if (silent_pic_mode == 2) 
		silent_pic_highres = mod(silent_pic_highres + sign, COUNT(silent_pic_sweep_modes_c));
	else if (silent_pic_mode == 3)
		silent_pic_slitscan_skipframes = mod(silent_pic_slitscan_skipframes + sign - 1, 4) + 1;
}
static void silent_pic_toggle_forward(void* priv)
{ silent_pic_toggle(1); }

static void silent_pic_toggle_reverse(void* priv)
{ silent_pic_toggle(-1); }

uint32_t afframe[26];
PROP_HANDLER( PROP_LV_AFFRAME ) {
	memcpy(afframe, buf, 0x68);
	return prop_cleanup( token, property );
}

face_zoom_request = 0;

int hs = 0;
PROP_HANDLER( PROP_HALF_SHUTTER ) {
	int v = *(int*)buf;
	if (zoom_enable_face)
	{
		if (v == 0 && lv_drawn() && lvaf_mode == 2 && gui_state == 0 && !recording) // face detect
			face_zoom_request = 1;
	}
	if (v && gui_menu_shown() && !is_focus_menu_active())
	{
		gui_stop_menu();
	}
	return prop_cleanup( token, property );
}

int job_almost_ready = 0;
PROP_HANDLER( PROP_LAST_JOB_STATE ) {
	int v = *(int*)buf;
	if (v <= 10) job_almost_ready = 1;
	return prop_cleanup( token, property );
}

int sweep_lv_on = 0;
static void 
sweep_lv_start(void* priv)
{
	sweep_lv_on = 1;
}

int center_lv_aff = 0;
void center_lv_afframe()
{
	center_lv_aff = 1;
}
void center_lv_afframe_do()
{
	if (!lv_drawn() || gui_menu_shown() || gui_state != GUISTATE_IDLE) return;
	afframe[2] = (afframe[0] - afframe[4])/2;
	afframe[3] = (afframe[1] - afframe[5])/2;
	//~ bmp_printf(FONT_MED, 30, 30, "center af: %d, %d ", afframe[2], afframe[3]);
	prop_request_change(PROP_LV_AFFRAME, afframe, 0x68);
}

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
}

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

static void vsync(volatile int* addr)
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

static void
silent_pic_take_simple()
{
	struct vram_info * vram = get_yuv422_hd_vram();
	
	int silent_number;
	char imgname[100];
	for (silent_number = 1 ; silent_number < 1000; silent_number++) // may be slow after many pics
	{
		//bmp_printf(FONT_MED, 90,90, "silent: %d ", silent_number);

		snprintf(imgname, sizeof(imgname), "B:/DCIM/%03dCANON/%04d-%03d.422", folder_number, file_number, silent_number);

		unsigned size;
		if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
		if (size == 0) break;
	}

	
	bmp_printf(FONT_MED, 20, 70, "Psst! Taking a pic (%d)      ", silent_number);
	//~ vsync(vram->vram + vram->pitch * vram->height - 100);
	dump_seg(vram->vram, vram->pitch * vram->height, imgname);
	bmp_printf(FONT_MED, 20, 70, "Psst! Just took a pic (%d)   ", silent_number);
	
	if (!silent_pic_burst) // single mode
	{
		while (get_halfshutter_pressed()) msleep(100);
	}
}

static void
silent_pic_take_sweep()
{
	if (recording) return;
	if (!lv_drawn()) return;

	bmp_printf(FONT_MED, 20, 70, "Psst! Preparing for high-res pic   ");
	while (get_halfshutter_pressed()) msleep(100);
	gui_stop_menu();
	msleep(100);

	int afx0 = afframe[2];
	int afy0 = afframe[3];

	int zoom = 5;
	prop_request_change(PROP_LV_DISPSIZE, &zoom, 4);
	msleep(1000);

	struct vram_info * vram = get_yuv422_hd_vram();
	int silent_number;
	char imgname[100];
	for (silent_number = 1 ; silent_number < 1000; silent_number++) // may be slow after many pics
	{
		snprintf(imgname, sizeof(imgname), "B:/DCIM/%03dCANON/%04d-%03d.422", folder_number, file_number, silent_number);
		unsigned size;
		if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
		if (size == 0) break;
	}

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
			bmp_printf(FONT_MED, 20, 70, "Psst! Taking a high-res pic (%d) [%d,%d]      ", silent_number, i, j);
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

	bmp_printf(FONT_MED, 20, 70, "Psst! Just took a high-res pic (%d)   ", silent_number);

}

static void
silent_pic_take_slitscan(int interactive)
{
	if (recording) return; // vsync fails
	if (!lv_drawn()) return;
	gui_stop_menu();
	if (interactive) while (get_halfshutter_pressed()) msleep(100);
	clrscr();
	bmp_printf(FONT_MED, 20, 70, "Psst! Taking a slit-scan pic   ");

	uint8_t * const lvram = UNCACHEABLE(YUV422_LV_BUFFER);
	int lvpitch = YUV422_LV_PITCH;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	#define BMPPITCH 960

	struct vram_info * vram = get_yuv422_hd_vram();
	bmp_printf(FONT_MED, 20, 100, "%dx%d", vram->width, vram->height);
	int silent_number;
	char imgname[100];
	for (silent_number = 1 ; silent_number < 1000; silent_number++) // may be slow after many pics
	{
		snprintf(imgname, sizeof(imgname), "B:/DCIM/%03dCANON/%04d-%03d.422", folder_number, file_number, silent_number);
		unsigned size;
		if( FIO_GetFileSize( imgname, &size ) != 0 ) break;
		if (size == 0) break;
	}

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
		for (k = 0; k < silent_pic_slitscan_skipframes; k++)
			vsync(CLK_25FPS);
		
		FIO_WriteFile(f, (i % 2 ? YUV422_HD_BUFFER : YUV422_HD_BUFFER_2) + i * vram->pitch, vram->pitch);

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
			bmp_printf(FONT_MED, 20, 70, "Slit-scan cancelled.");
			while (get_halfshutter_pressed()) msleep(100);
			return;
		}
	}
	FIO_CloseFile(f);

	bmp_printf(FONT_MED, 20, 70, "Psst! Just took a slit-scan pic (%d)   ", silent_number);

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
	else if (silent_pic_mode == 2) // hi-res
		silent_pic_take_sweep();
	else if (silent_pic_mode == 3) // slit-scan
		silent_pic_take_slitscan(interactive);

	set_global_draw(g);
}


static void 
iso_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ISO         : %d",
		lens_info.iso
	);
	bmp_printf(FONT_MED, x + 450, y+5, "[Q]=Auto");
}

static void
iso_toggle( int sign )
{
	int i = raw2index_iso(lens_info.raw_iso);
	int k;
	for (k = 0; k < 10; k++)
	{
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
	int iso_auto_mode = 0;
	prop_request_change(PROP_ISO, &iso_auto_mode, 4);   // force iso auto
	msleep(500);
	while (iso_auto_code == 0)
	{
		bmp_printf(FONT_LARGE, 0, 0, "Please half-press shutter");
		msleep(100);
		bmp_printf(FONT_LARGE, 0, 0, "                         ");
	}
	return iso_auto_code;
}
static void iso_auto_quick()
{
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
static void get_under_and_over_exposure_autothr(int* under, int* over)
{
	int thr_lo = 0;
	int thr_hi = 255;
	*under = 0;
	*over = 0;
	while (*under < 50 && *over < 50 && thr_lo < thr_hi)
	{
		thr_lo += 20;
		thr_hi -= 20;
		get_under_and_over_exposure(thr_lo, thr_hi, under, over);
	}
}

int crit_iso(int iso_index)
{
	if (!lv_drawn()) return 0;

	if (iso_index >= 0)
	{
		lens_set_rawiso(codes_iso[iso_index]);
		msleep(100);
		bmp_printf(FONT_LARGE, 30, 30, "ISO %d... ", lens_info.iso);
		msleep(300);
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
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
	clrscr();
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
	bmp_printf(FONT_MED, x + 450, y+5, "[Q]=Auto");
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
		msleep(100);
		bmp_printf(FONT_LARGE, 30, 30, "Shutter 1/%d... ", lens_info.shutter);
		msleep(300);
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
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
	clrscr();
}


static void
kelvin_toggle( int sign )
{
	int k = lens_info.kelvin;
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

static void 
kelvin_display( void * priv, int x, int y, int selected )
{
	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WhiteBalance: %dK",
			lens_info.kelvin
		);
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
	}
	bmp_printf(FONT_MED, x + 450, y+5, "[Q]=Auto");
}

int kelvin_auto_flag = 0;
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

int crit_kelvin(int k)
{
	if (!lv_drawn()) return 0;

	if (k > 0)
	{
		lens_set_kelvin(k * KELVIN_STEP);
		bmp_printf(FONT_LARGE, 30, 30, "WB %dK... ", k * KELVIN_STEP);
		msleep(500);
	}

	uint8_t Y;
	int8_t U, V;
	get_spot_yuv(100, &Y, &U, &V);
	bmp_printf(FONT_MED, 300, 30, "%d, %d ", U, V);
	return V - U;
}

static void kelvin_auto_run()
{
	menu_show_only_selected();
	int c0 = crit_kelvin(-1); // test current iso
	int i;
	if (c0 > 0) i = bin_search(lens_info.kelvin/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
	else i = bin_search(KELVIN_MIN/KELVIN_STEP, lens_info.kelvin/KELVIN_STEP + 1, crit_kelvin);
	lens_set_kelvin(i * KELVIN_STEP);
	clrscr();
}

static void 
wbs_gm_display( void * priv, int x, int y, int selected )
{
		int gm = (int8_t)wbs_gm;
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WBShift G/M : %s%d", 
			gm > 0 ? "G" : (gm < 0 ? "M" : ""), 
			ABS(gm)
		);
}

static void
wbs_gm_toggle( int sign )
{
	int gm = (int8_t)wbs_gm;
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

int set_htp(int enable)
{
	if (enable) cfn[1] |= 0x10000;
	else cfn[1] &= ~0x10000;
	prop_request_change(PROP_CFN, cfn, 0xD);
}

PROP_INT(PROP_ALO, alo);

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
}

static void 
zoom_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LiveViewZoom: %s %s %s",
		zoom_disable_x5 ? "" : "x5", 
		zoom_disable_x10 ? "" : "x10", 
		zoom_enable_face ? ":-)" : ""
	);
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
			"HDR Brack:   OFF"
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"HDR Brack: %dx%d%sEV",
			hdr_steps, 
			hdr_stepsize / 8,
			((hdr_stepsize/4) % 2) ? ".5" : ""
		);
	}
	bmp_printf(FONT_MED, x + 400, y+5, "[SET-DISP-Q]");
}

static void
hdr_steps_toggle( void * priv )
{
	hdr_steps = mod(hdr_steps + (hdr_steps <= 2 ? 0 : 1), 10) + 1;
}

static void
hdr_stepsize_toggle( void * priv )
{
	hdr_stepsize = (hdr_stepsize < 8) ? 8 : (hdr_stepsize/8)*8 + 8;
	if (hdr_stepsize > 40) hdr_stepsize = 4;
}

static void
hdr_reset( void * priv )
{
	hdr_steps = 1;
	hdr_stepsize = 8;
}

void SW1(int v)
{
	prop_request_change(PROP_REMOTE_SW1, &v, 2);
	msleep(100);
}
void SW2(int v)
{
	prop_request_change(PROP_REMOTE_SW2, &v, 2);
	msleep(100);
}
int is_bulb_mode()
{
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
	SW1(1);
	SW2(1);
	msleep(duration);
	SW2(0);
	SW1(0);
}

static void bulb_toggle_fwd(void* priv)
{
	bulb_duration = bulb_duration * 2;
	if (bulb_duration >= 3600*1000*2)
		bulb_duration = 1000;
	if (bulb_duration > 3600*1000) // one hour
		bulb_duration = 3600*1000;
}
static void bulb_toggle_rev(void* priv)
{
	bulb_duration = bulb_duration / 2;
	if (bulb_duration < 1000)
		bulb_duration = 3600*1000;
}

static void
bulb_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Bulb Timer %s: %ds",
		is_bulb_mode() ? "     " : "(N/A)",
		bulb_duration/1000
	);
}


struct menu_entry shoot_menus[] = {
	{
		.display	= hdr_display,
		.select		= hdr_steps_toggle,
		.select_reverse = hdr_stepsize_toggle,
		.select_auto = hdr_reset,
	},
	{
		.priv		= &interval_timer_index,
		.display	= interval_timer_display,
		.select		= interval_timer_toggle,
		.select_reverse	= interval_timer_toggle_reverse,
		.select_auto = interval_movie_duration_toggle,
	},
	{
		.priv		= &intervalometer_running,
		.select		= menu_binary_toggle,
		.display	= intervalometer_display,
	},
	{
		.priv		= &lcd_release_running,
		.select		= menu_ternary_toggle, 
		.select_reverse = menu_ternary_toggle_reverse,
		.display	= lcd_release_display,
	},
 	{
		.priv		= &audio_release_running,
		.select		= menu_binary_toggle,
		.display	= audio_release_display,
	},
	{
		.priv		= &trap_focus,
		.select		= menu_ternary_toggle,
		.display	= trap_focus_display,
	},
	{
		.select		= flash_and_no_flash_toggle,
		.display	= flash_and_no_flash_display,
	},
	{
		.select = silent_pic_mode_toggle,
		.select_reverse = silent_pic_toggle_reverse,
		.select_auto = silent_pic_toggle_forward,
		.display = silent_pic_display,
	},
	{
		.display = bulb_display, 
		.select = bulb_toggle_fwd, 
		.select_reverse = bulb_toggle_rev,
	}
};

struct menu_entry vid_menus[] = {
	{
		.priv = &zoom_enable_face,
		.select = menu_binary_toggle,
		.select_reverse = zoom_toggle, 
		.display = zoom_display,
	},
};

struct menu_entry expo_menus[] = {
	{
		.display	= iso_display,
		.select		= iso_toggle_forward,
		.select_reverse		= iso_toggle_reverse,
		.select_auto = iso_auto,
	},
	{
		.display	= shutter_display,
		.select		= shutter_toggle_forward,
		.select_reverse		= shutter_toggle_reverse,
		.select_auto = shutter_auto,
	},
	{
		.display	= kelvin_display,
		.select		= kelvin_toggle_forward,
		.select_reverse		= kelvin_toggle_reverse,
		.select_auto = kelvin_auto,
	},
	{
		.display = wbs_gm_display, 
		.select = wbs_gm_toggle_forward, 
		.select_reverse = wbs_gm_toggle_reverse,
	},
	{
		.display	= contrast_display,
		.select		= contrast_toggle_forward,
		.select_reverse		= contrast_toggle_reverse,
		//~ .select_auto = contrast_auto,
	},
	{
		.display	= ladj_display,
		.select		= ladj_toggle_forward,
		.select_reverse		= ladj_toggle_reverse,
	},
};

int display_sensor_active()
{
	return (*(int*)(DISPLAY_SENSOR_MAYBE));
}

void hdr_create_script(int steps, int skip0)
{
	if (steps <= 1) return;
	DEBUG();
	FILE * f = INVALID_PTR;
	char name[100];
	int f0 = skip0 ? file_number_also : file_number_also+1;
	snprintf(name, sizeof(name), "B:/DCIM/%03dCANON/HDR_%04d.sh", folder_number, f0);
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
	my_fprintf(f, "\n# HDR_%04d.JPG from IMG_%04d.JPG ... IMG_%04d.JPG\n\n", f0, f0, mod(f0 + steps - 1, 10000));
	my_fprintf(f, "enfuse \"$@\" --output=HDR_%04d.JPG ", f0);
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

// skip0: don't take the middle exposure
void hdr_take_pics(int steps, int step_size, int skip0)
{
	hdr_create_script(steps, skip0);
	int i;
	if ((lens_info.iso && shooting_mode == SHOOTMODE_M) || (shooting_mode == SHOOTMODE_MOVIE))
	{
		const int s = lens_info.raw_shutter;
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			if (skip0 && (i == 0)) continue;
			bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
			msleep(1);
			int new_s = COERCE(s - step_size * i, 0x10, 152);
			lens_set_rawshutter( new_s );
			msleep(1);
			if (!silent_pic_mode || !lv_drawn()) lens_take_picture_forced();
			else { msleep(300); silent_pic_take(0); }
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
			bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
			msleep(10);
			int new_ae = ae + step_size * i;
			lens_set_ae( new_ae );
			msleep(10);
			if (!silent_pic_mode || !lv_drawn()) lens_take_picture_forced();
			else { msleep(300); silent_pic_take(0); }
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

	call("MovieEnd");
}

static void
hdr_take_mov(steps, step_size)
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
void hdr_shot(int skip0)
{
	//~ bmp_printf(FONT_LARGE, 50, 50, "SKIP%d", skip0);
	//~ msleep(2000);
	if (is_bulb_mode())
	{
		bulb_take_pic(bulb_duration);
	}
	else if (shooting_mode == SHOOTMODE_MOVIE && !silent_pic_mode)
	{
		hdr_take_mov(hdr_steps, hdr_stepsize);
	}
	else
	{
		if (drive_mode != DRIVE_SINGLE && drive_mode != DRIVE_CONTINUOUS) 
			lens_set_drivemode(DRIVE_CONTINUOUS);
		if (hdr_steps == 2)
			hdr_take_pics(hdr_steps, hdr_stepsize/2, 1);
		else
			hdr_take_pics(hdr_steps, hdr_stepsize, skip0);
		while (lens_info.job_state) msleep(500);
	}
}

// take one shot, a sequence of HDR shots, or start a movie
// to be called by remote triggers
void remote_shot()
{
	if (is_bulb_mode())
	{
		bulb_take_pic(bulb_duration);
	}
	else if (hdr_steps > 1)
	{
		hdr_shot(0);
	}
	else
	{
		if (silent_pic_mode && lv_drawn())
			silent_pic_take(0);
		else if (shooting_mode == SHOOTMODE_MOVIE)
			movie_start();
		else
			lens_take_picture(64000); // hdr_shot messes with the self timer mode
	}
	while (lens_info.job_state) msleep(500);
}

void display_shooting_info() // called from debug task
{
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, 80, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 320, 260, "%5dK", lens_info.kelvin);
	}

	bg = bmp_getpixel(680, 40);
	fnt = FONT(FONT_MED, 80, bg);
	int iso = lens_info.iso;
	if (iso)
		bmp_printf(fnt, 470, 27, "ISO %5d", iso);
	else
		bmp_printf(fnt, 470, 27, "ISO AUTO");

	bg = bmp_getpixel(410, 330);
	fnt = FONT(FONT_MED, (trap_focus == 2 ? COLOR_RED : 80), bg);
	if (trap_focus && ((af_mode & 0xF) == 3))
		bmp_printf(fnt, 410, 331, "TRAP \nFOCUS");

	if (wbs_gm || wbs_ba)
	{
		fnt = FONT(FONT_LARGE, 80, bg);

		int ba = (int8_t)wbs_ba;
		if (ba) bmp_printf(fnt, 435, 240, "%s%d ", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(fnt, 431, 240, "   ");

		int gm = (int8_t)wbs_gm;
		if (gm) bmp_printf(fnt, 435, 270, "%s%d ", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(fnt, 431, 270, "   ");
	}
	
	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, 80, bg);
	
	if (hdr_steps > 1)
		bmp_printf(fnt, 380, 450, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 380, 450, "           ");

	bmp_printf(fnt, 200, 450, "Flash:%s%s", 
		strobo_firing == 0 ? " ON" : 
		strobo_firing == 1 ? "OFF" : "Auto", 
		strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
		);
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
static void
shoot_task( void )
{
	int i = 0;
	menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
	menu_add( "Expo", expo_menus, COUNT(expo_menus) );
	msleep(1000);
	menu_add( "Video", vid_menus, COUNT(vid_menus) );
	struct audio_level *al=get_audio_levels();
	while(1)
	{
		msleep(10);
		if (gui_state == GUISTATE_PLAYMENU || gui_state == GUISTATE_MENUDISP)
 		{
			if (intervalometer_running || lcd_release_running || audio_release_running)
			{
				bmp_printf(FONT_MED, 20, (lv_drawn() ? 40 : 3), "Stopped                                             ");
			}
			intervalometer_running = 0;
			lcd_release_running = 0;
			audio_release_running = 0;
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
		if (zoom_disable_x5 && lv_dispsize == 5 && !silent_pic_highres) //silent_pic_highres needs x5 zoom
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
		if (sweep_lv_on)
		{
			sweep_lv();
			sweep_lv_on = 0;
		}
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

		if (lens_info.job_state) // just took a picture, maybe we should take another one
		{
			if (hdr_steps > 1) hdr_shot(1); // skip the middle exposure, which was just taken
		}

		// toggle flash on/off for next picture
		if (flash_and_no_flash && strobo_firing < 2 && strobo_firing != file_number % 2)
		{
			if (lens_info.job_state > 10) // not safe to change flash setting
			{
				job_almost_ready = 0;
				while (!job_almost_ready) msleep(1);
			}
			strobo_firing = file_number % 2;
			set_flash_firing(strobo_firing);
		}

		if (trap_focus && !lv_drawn() && (af_mode & 0xF) == 3 && !gui_menu_shown()) // MF
		{
			if (trap_focus == 2 && cfn[2] & 0xF00 != 0) bmp_printf(FONT_MED, 0, 0, "Set CFn9 to 0 (AF on half-shutter press)");
			if (trap_focus == 2 && gui_state == GUISTATE_IDLE) SW1(1);
			if (*(int*)FOCUS_CONFIRMATION)
			{
				lens_take_picture(64000);
				msleep(trap_focus_delay);
			}
			if (trap_focus == 2 && gui_state == GUISTATE_IDLE) SW1(0);
		}
		
		if (silent_pic_mode && lv_drawn() && get_halfshutter_pressed())
		{
			silent_pic_take(1);
		}

		if (intervalometer_running)
		{
			if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			msleep(1000);
			if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			hdr_shot(0);
			for (i = 0; i < timer_values[interval_timer_index] - 1; i++)
			{
				msleep(1000);
				if (intervalometer_running) bmp_printf(FONT_MED, 20, (lv_drawn() ? 40 : 3), "Press PLAY or MENU to stop the intervalometer...%d   ", timer_values[interval_timer_index] - i - 1);
				if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
				
				if (shooting_mode != SHOOTMODE_MOVIE)
				{
					if (lens_info.shutter > 100) bmp_printf(FONT_MED, 0, 70,                                 "Tip: use shutter speeds slower than 1/100 to prevent flicker.");
					else if (shooting_mode != SHOOTMODE_M || lens_info.iso == 0) bmp_printf(FONT_MED, 0, 70, "Tip: use fully manual exposure to prevent flicker.           ");
					else if ((af_mode & 0xF) != 3) bmp_printf(FONT_MED, 0, 70,                               "Tip: use manual focus                                        ");
				}
			}
		}
		else
		{
			if (lcd_release_running)
			{
				msleep(20);
				if (gui_menu_shown()) continue;
				if (lv_drawn()) 
				{
					bmp_printf(FONT_MED, 20, 40, "LCD RemoteShot does not work in LiveView, sorry...");
					continue;
				}
				bmp_printf(FONT_MED, 20, 3, "Move your hand near LCD face sensor to take a picture!");
				if (display_sensor_active())
				{
					if (lcd_release_running == 2) // take pic when you move the hand away
						while (display_sensor_active()) 
							msleep(10);
					remote_shot();
					while (display_sensor_active()) { msleep(500); }
				}
			}
			if (audio_release_running) 
			{
				bmp_printf(FONT_MED, 20, lv_drawn() ? 40 : 3, "Audio release ON (%d)   ", al[0].peak);
				if (al[0].peak > audio_release_level) 
				{
					remote_shot();
					// this may trigger an infinite shooting loop due to shutter noise
					int k;
					for (k = 0; k < 5; k++)
					{
						msleep(100);
						while (al[0].peak > audio_release_level)
						{
							bmp_printf(FONT_MED, 20, lv_drawn() ? 40 : 3, "Waiting for silence (%d)...", al[0].peak);
							msleep(100);
						}
					}
					bmp_printf(FONT_MED, 20, lv_drawn() ? 40 : 3, "                                        ");
				}
			}
		}
	}
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x18, 0x1000 );


