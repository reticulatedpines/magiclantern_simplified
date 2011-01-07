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
CONFIG_INT( "interval.movie.duration", interval_movie_duration, 1000);

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
PROP_INT(PROP_FOLDER_NUMBER, folder_number);

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
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"IntervalTime:   %ds", 
		timer_values[*(int*)priv]
	);
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
intervalometer_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Intervalometer: %s",
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
		"LCD RemoteShot: %s",
		v == 1 ? "Near" : (v == 2 ? "Away" : "OFF")
	);
}

static void
lcd_release_toggle( void * priv )
{
	*(int*)priv = mod(*(int*)priv + 1, 3);
}
static void
audio_release_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Audio Rem.Shot: %s",
		audio_release_running ? "ON " : "OFF"
	);
}

static void
audio_release_toggle(void* priv)
{
	audio_release_running = !audio_release_running;
	if (!audio_release_running) clrscr();
}

static void 
trap_focus_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Trap Focus:  %s",
		(*(int*)priv) ? "ON " : "OFF"
	);
}

static void 
iso_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ISO     : %d",
		lens_info.iso
	);
	bmp_printf(FONT_MED, x + 450, y+5, "[Q]=Auto");
}

static void
iso_toggle( int sign )
{
	int i = raw2index_iso(lens_info.raw_iso);
	while(1)
	{
		i = mod(i + sign, COUNT(codes_iso));
		lens_set_rawiso(codes_iso[i]);
		msleep(100);
		int j = raw2index_iso(lens_info.raw_iso);
		if (i == j) break;
	}
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
		"Shutter : 1/%d",
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
			"WhiteBal: %dK",
			lens_info.kelvin
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WhiteBal: %s",
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
	return V - U;
}

static void kelvin_auto_run()
{
	int c0 = crit_kelvin(-1); // test current iso
	int i;
	if (c0 > 0) i = bin_search(lens_info.kelvin/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
	else i = bin_search(KELVIN_MIN/KELVIN_STEP, lens_info.kelvin/KELVIN_STEP + 1, crit_kelvin);
	lens_set_kelvin(i * KELVIN_STEP);
	clrscr();
}

static void
contrast_toggle( int sign )
{
	int c = lens_get_contrast();
	int newc = mod((c + 4 + sign), 9) - 4;
	lens_set_contrast(newc);
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
		"Contrast: %d ",
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
		"LightAdj: %s",
		(htp ? "HTP" :
		(alo == ALO_STD ? "ALO std" :
		(alo == ALO_LOW ? "ALO low" : 
		(alo == ALO_HIGH ? "ALO hi " :
		(alo == ALO_OFF ? "OFF" : "err")))))
	);
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
			"HDR Bracket:OFF"
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"HDR Brack:%dx%dEV",
			hdr_steps, 
			hdr_stepsize / 8
		);
	}
	bmp_printf(FONT_MED, x + 440, y, "[SET ]\n[DISP]");
	bmp_printf(FONT_MED, x + 510, y+5, "[Q]");
}

static void
hdr_steps_toggle( void * priv )
{
	hdr_steps = mod(hdr_steps + 2, 10);
}

static void
hdr_stepsize_toggle( void * priv )
{
	hdr_stepsize = mod(hdr_stepsize, 40) + 8;
}

static void
hdr_reset( void * priv )
{
	hdr_steps = 1;
	hdr_stepsize = 8;
}

int mov_test_en = 0;

static void mov_test(void* priv)
{
	mov_test_en = !mov_test_en;
	bmp_printf(FONT_LARGE, 30, 30, "en=%d ", mov_test_en);
}

struct menu_entry shoot_menus[] = {
	{
		.priv		= &interval_timer_index,
		.display	= interval_timer_display,
		.select		= interval_timer_toggle,
		.select_reverse	= interval_timer_toggle_reverse,
	},
	{
		.priv		= &intervalometer_running,
		.select		= menu_binary_toggle,
		.display	= intervalometer_display,
	},
	{
		.priv		= &lcd_release_running,
		.select		= lcd_release_toggle,
		.display	= lcd_release_display,
	},
 	{
		.select		= audio_release_toggle,
		.display	= audio_release_display,
	},
	{
		.display	= hdr_display,
		.select		= hdr_steps_toggle,
		.select_reverse = hdr_stepsize_toggle,
		.select_auto = hdr_reset,
	},
	{
		.priv		= &trap_focus,
		.select		= menu_binary_toggle,
		.display	= trap_focus_display,
	}
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
	int f0 = skip0 ? file_number : file_number+1;
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
	if (shooting_mode == SHOOTMODE_M)
	{
		const int s = lens_info.raw_shutter;
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			if (skip0 && (i == 0)) continue;
			bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
			msleep(10);
			int new_s = COERCE(s - step_size * i, 0x10, 152);
			lens_set_rawshutter( new_s );
			msleep(10);
			lens_take_picture( 64000 );
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
			lens_take_picture( 64000 );
		}
		lens_set_ae( ae );
	}
}

static void
movie_start()
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

static void
movie_end()
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
		msleep(interval_movie_duration);
	}
	lens_set_rawshutter( s );
	movie_end();
	set_global_draw(g);
}

// take a HDR shot (sequence of stills or a small movie)
void hdr_shot(int skip0)
{
	//~ bmp_printf(FONT_LARGE, 50, 50, "SKIP%d", skip0);
	//~ msleep(2000);
	if (shooting_mode == SHOOTMODE_MOVIE)
	{
		hdr_take_mov(hdr_steps, hdr_stepsize);
	}
	else
	{
		if (drive_mode != DRIVE_SINGLE && drive_mode != DRIVE_CONTINUOUS) 
			lens_set_drivemode(DRIVE_CONTINUOUS);
		hdr_take_pics(hdr_steps, hdr_stepsize, skip0);
		while (lens_info.job_state) msleep(500);
	}
}

// take one shot, a sequence of HDR shots, or start a movie
// to be called by remote triggers
void remote_shot()
{
	//~ bmp_printf(FONT_LARGE, 50, 50, "REMOT");
	//~ msleep(2000);
	if (hdr_steps > 1) hdr_shot(0);
	else
	{
		if (shooting_mode == SHOOTMODE_MOVIE)
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
		bmp_printf(fnt, 470, 30, "ISO %5d", iso);
	else
		bmp_printf(fnt, 470, 30, "ISO AUTO");

	bg = bmp_getpixel(410, 330);
	fnt = FONT(FONT_MED, 80, bg);
	if (trap_focus && ((af_mode & 0xF) == 3))
		bmp_printf(fnt, 410, 331, "TRAP \nFOCUS");

	if (wbs_gm || wbs_ba)
	{
		fnt = FONT(FONT_LARGE, 80, bg);

		int ba = (int8_t)wbs_ba;
		if (ba) bmp_printf(fnt, 431, 240, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(fnt, 431, 240, "  ");

		int gm = (int8_t)wbs_gm;
		if (gm) bmp_printf(fnt, 431, 270, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(fnt, 431, 270, "  ");
	}
	
	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, 80, bg);
	
	if (hdr_steps > 1)
		bmp_printf(fnt, 380, 450, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 380, 450, "           ");
}

static void
shoot_task( void )
{
	int i = 0;
	menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
	menu_add( "Expo", expo_menus, COUNT(expo_menus) );
	msleep(1000);
	struct audio_level *al=get_audio_levels();
	while(1)
	{
		msleep(1);
		if (gui_state == GUISTATE_PLAYMENU/* || get_halfshutter_pressed()*/)
		{
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

		// restore drive mode if it was changed
		// it might have been changed to 2sec timer by the HDR triggered by picture taken
		if (!get_halfshutter_pressed() && drive_mode_bk >= 0)
		{
			lens_set_drivemode(drive_mode_bk);
			drive_mode_bk = -1;
		}

		if (intervalometer_running)
		{
			msleep(1000);
			if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			hdr_shot(0);
			for (i = 0; i < timer_values[interval_timer_index] - 1; i++)
			{
				msleep(1000);
				if (intervalometer_running) bmp_printf(FONT_MED, 20, (lv_drawn() ? 40 : 3), "Press PLAY or MENU to stop the intervalometer.");
				if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			}
		}
		else if (lcd_release_running)
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
		else if (audio_release_running) 
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
			else 
			{
				msleep(5); // peak is updated at 16 ms
			}
		}
		else if (hdr_steps > 1) // no remote control enabled => will trigger HDR by taking a normal pic
		{
			// avoid camera shake
			if (get_halfshutter_pressed() && drive_mode != DRIVE_SELFTIMER_2SEC)
			{
				drive_mode_bk = drive_mode;
				lens_set_drivemode(DRIVE_SELFTIMER_2SEC);
			}
			if (lens_info.job_state && lens_info.job_state <= 0xA) // just took a picture
			{
				hdr_shot(1); // skip the middle exposure, which was just taken
			}
			msleep(5);
		}
		else if (trap_focus)
		{
			if (lv_drawn()) 
			{
				//~ bmp_printf(FONT_MED, 20, 35, "Trap Focus does not work in LiveView, sorry...");
				msleep(500);
				continue;
			}
			if ((af_mode & 0xF) != 3) // != MF
			{
				//~ bmp_printf(FONT_MED, 20, 35, "Please switch the lens to Manual Focus mode. %d", af_mode);
				msleep(500);
				continue;
			}
			if (*(int*)FOCUS_CONFIRMATION)
			{
				lens_take_picture(64000);
				msleep(trap_focus_delay);
			}
		} 
		else msleep(500);
	}
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x18, 0x1000 );


