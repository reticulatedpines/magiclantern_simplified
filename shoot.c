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

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COERCE(x,lo,hi) MAX(MIN(x,hi),lo)
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define KELVIN_MIN 1700
#define KELVIN_MAX 10000
#define KELVIN_STEP 100

CONFIG_INT( "interval.timer.index", interval_timer_index, 2 );
CONFIG_INT( "focus.trap", trap_focus, 1);
CONFIG_INT( "focus.trap.delay", trap_focus_delay, 500); // min. delay between two shots in trap focus

int intervalometer_running = 0;
int lcd_release_running = 0;

PROP_INT(PROP_DRIVE, drive_mode);
PROP_INT(PROP_AF_MODE, af_mode);
PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
PROP_INT(PROP_MVR_REC_START, recording);
PROP_INT(PROP_WB_MODE_LV, wb_mode);
PROP_INT(PROP_WB_KELVIN_LV, kelvins);
PROP_INT(PROP_SHUTTER, current_shutter_code);
PROP_INT(PROP_ISO, current_iso_code);
PROP_INT(PROP_WBS_GM, wbs_gm);
PROP_INT(PROP_WBS_BA, wbs_ba);

int timer_values[] = {1,2,5,10,30,60,300,900,3600};

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
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LCD RemoteShot: %s",
		(*(int*)priv) ? "ON " : "OFF"
	);
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

const int iso_values[] = {0,100,110,115,125,140,160,170,185,200,220,235,250,280,320,350,380,400,435,470,500,580,640,700,750,800,860,930,1000,1100,1250,1400,1500,1600,1750,1900,2000,2250,2500,2750,3000,3200,3500,3750,4000,4500,5000,5500,6000,6400,7200,8000,12800,25600};
const int iso_codes[]  = {0, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  128,  136}; 

int get_current_iso_index()
{
	int i;
	for (i = 0; i < COUNT(iso_codes); i++) 
		if(iso_codes[i] == current_iso_code) return i;
	return 0;
}
int get_current_iso()
{
	return iso_values[get_current_iso_index()];
}

static void 
iso_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ISO     : %d",
		get_current_iso()
	);
}

static void
iso_toggle( int sign )
{
	int i = get_current_iso_index();
	while(1)
	{
		i = mod(i + sign, COUNT(iso_codes));
		lens_set_iso(iso_codes[i]);
		msleep(100);
		int j = get_current_iso_index();
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
	lens_set_iso(measure_auto_iso());
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
		lens_set_iso(iso_codes[iso_index]);
		msleep(100);
		bmp_printf(FONT_LARGE, 30, 30, "ISO %d... ", get_current_iso());
		msleep(300);
	}

	int under, over;
	get_under_and_over_exposure_autothr(&under, &over);
	return under - over;
}

static void iso_auto_run()
{
	if (current_iso_code == 0) { lens_set_iso(96); msleep(500); }
	int c0 = crit_iso(-1); // test current iso
	int i;
	if (c0 > 0) i = bin_search(get_current_iso_index(), COUNT(iso_codes), crit_iso);
	else i = bin_search(get_htp() ? 9 : 1, get_current_iso_index()+1, crit_iso);
	lens_set_iso(iso_codes[i]);
	clrscr();
}

const int shutter_values[] = { 30, 33, 37, 40,  45,  50,  53,  57,  60,  67,  75,  80,  90, 100, 110, 115, 125, 135, 150, 160, 180, 200, 210, 220, 235, 250, 275, 300, 320, 360, 400, 435, 470, 500, 550, 600, 640, 720, 800, 875, 925,1000,1100,1200,1250,1400,1600,1750,1900,2000,2150,2300,2500,2800,3200,3500,3750,4000};
const int shutter_codes[]  = { 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152};

int get_current_shutter_index()
{
	int i;
	for (i = 0; i < COUNT(shutter_codes); i++) 
		if(shutter_codes[i] >= current_shutter_code) return i;
	return 0;
}
int get_current_shutter()
{
	return shutter_values[get_current_shutter_index()];
}

static void 
shutter_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Shutter : 1/%d",
		get_current_shutter()
	);
}

static void
shutter_toggle( int sign)
{
	int i = get_current_shutter_index();
	while(1)
	{
		i = mod(i + sign, COUNT(shutter_codes));
		lens_set_shutter(shutter_codes[i]);
		msleep(100);
		int j = get_current_shutter_index();
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
	if (current_iso_code == 0) return;                  // does not work on Auto ISO
	int ciso = current_iso_code;
	int steps = measure_auto_iso() - ciso;              // read delta exposure and compute new shutter value
	int newshutter = COERCE(current_shutter_code - steps, 96, 152);
	lens_set_iso(ciso);                                 // restore iso
	lens_set_shutter(newshutter);                       // set new shutter value
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
		lens_set_shutter(shutter_codes[shutter_index]);
		msleep(100);
		bmp_printf(FONT_LARGE, 30, 30, "Shutter 1/%d... ", get_current_shutter());
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
	if (c0 > 0) i = bin_search(get_current_shutter_index(), COUNT(shutter_codes), crit_shutter);
	else i = bin_search(0, get_current_shutter_index()+1, crit_shutter);
	lens_set_shutter(shutter_codes[i]);
	clrscr();
}

static void
lens_set_kelvin(int k)
{
	k = COERCE(k, KELVIN_MIN, KELVIN_MAX);
	int mode = WB_KELVIN;
	prop_request_change(PROP_WB_MODE_LV, &mode, 4);
	prop_request_change(PROP_WB_KELVIN_LV, &k, 4);
	prop_request_change(PROP_WB_MODE_PH, &mode, 4);
	prop_request_change(PROP_WB_KELVIN_PH, &k, 4);
}

static void
kelvin_toggle( int sign )
{
	int k = kelvins;
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
	if (wb_mode == WB_KELVIN)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WhiteBal: %dK",
			kelvins
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"WhiteBal: %s",
			(wb_mode == 0 ? "Auto" : 
			(wb_mode == 1 ? "Sunny" :
			(wb_mode == 2 ? "Cloudy" : 
			(wb_mode == 3 ? "Tungsten" : 
			(wb_mode == 4 ? "CFL" : 
			(wb_mode == 5 ? "Flash" : 
			(wb_mode == 6 ? "Custom" : 
			(wb_mode == 8 ? "Shade" :
			 "unknown"))))))))
		);
	}
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
	if (c0 > 0) i = bin_search(kelvins/KELVIN_STEP, KELVIN_MAX/KELVIN_STEP + 1, crit_kelvin);
	else i = bin_search(KELVIN_MIN/KELVIN_STEP, kelvins/KELVIN_STEP + 1, crit_kelvin);
	lens_set_kelvin(i * KELVIN_STEP);
	clrscr();
}

PROP_INT(PROP_PICTURE_STYLE, pic_style);

int32_t picstyle_settings[10][6];

// prop_register_slave is much more difficult to use than copy/paste...

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_1 ) {
	memcpy(picstyle_settings[1], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_2 ) {
	memcpy(picstyle_settings[2], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_3 ) {
	memcpy(picstyle_settings[3], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_4 ) {
	memcpy(picstyle_settings[4], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_5 ) {
	memcpy(picstyle_settings[5], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_6 ) {
	memcpy(picstyle_settings[6], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_7 ) {
	memcpy(picstyle_settings[7], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_8 ) {
	memcpy(picstyle_settings[8], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_9 ) {
	memcpy(picstyle_settings[9], buf, 24);
	return prop_cleanup( token, property );
}



int get_prop_picstyle_index()
{
	switch(pic_style)
	{
		case 0x81: return 1;
		case 0x82: return 2;
		case 0x83: return 3;
		case 0x84: return 4;
		case 0x85: return 5;
		case 0x86: return 6;
		case 0x21: return 7;
		case 0x22: return 8;
		case 0x23: return 9;
	}
	bmp_printf(FONT_LARGE, 0, 0, "unk picstyle: %x", pic_style);
	return 0;
}

// get contrast from the current picture style
static int
lens_get_contrast()
{
	int i = get_prop_picstyle_index();
	if (!i) return;
	int32_t* buf = picstyle_settings[i];
	return buf[0];
}

// set contrast in the current picture style (change is permanent!)
static void
lens_set_contrast(int value)
{
	value = COERCE(value, -4, 4);
	int i = get_prop_picstyle_index();
	if (!i) return;
	int32_t* buf = picstyle_settings[i];
	buf[0] = value;
	prop_request_change(PROP_PICSTYLE_SETTINGS_1 - 1 + i, buf, 24);
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
	bmp_printf(FONT_MED, 0, 450, "cfn: %x/%x/%x/%x", cfn[0], cfn[1], cfn[2], cfn[3]);
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


CONFIG_INT("hdr.steps", hdr_steps, 1);
CONFIG_INT("hdr.stepsize", hdr_stepsize, 8);

static void 
hdr_display( void * priv, int x, int y, int selected )
{
	if (hdr_steps == 1)
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"HDR Bracket: OFF"
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"HDR Bracket: %dx%dEV",
			hdr_steps, 
			hdr_stepsize / 8
		);
	}
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
		.select		= menu_binary_toggle,
		.display	= lcd_release_display,
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


PROP_HANDLER( PROP_HALF_SHUTTER )
{
    if (buf[0])
    {
		intervalometer_running = 0;
		lcd_release_running = 0;
	}		
	return prop_cleanup( token, property );
}

void hdr_take_pics(int steps, int step_size)
{
	int i;
	if (shooting_mode == SHOOTMODE_M)
	{
		const int s = current_shutter_code;
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
			int new_s = COERCE(s - step_size * i, 0x10, 152);
			msleep(100);
			lens_set_shutter( new_s );
			msleep(100);
			lens_take_picture( 64000 );
			msleep(100);
		}
		msleep(100);
		lens_set_shutter( s );
	}
	else
	{
		const int ae = lens_get_ae();
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
			int new_ae = ae + step_size * i;
			lens_set_ae( new_ae );
			lens_take_picture( 64000 );
		}
		lens_set_ae( ae );
	}
}

static void
hdr_take_mov(steps, step_size)
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
	
	int g = get_global_draw();
	set_global_draw(0);
	clrscr();
	call("MovieStart");
	while (recording != 2) msleep(100);
	msleep(300);

	int i;
	const int s = current_shutter_code;
	for( i = -steps/2; i <= steps/2; i ++  )
	{
		bmp_printf(FONT_LARGE, 30, 30, "%d   ", i);
		int new_s = COERCE(s - step_size * i, 96, 152);
		lens_set_shutter( new_s );
		msleep(300);
	}
	lens_set_shutter( s );

	while (recording == 2)
	{
		call("MovieEnd",1);
		msleep(100);
	}
	set_global_draw(g);
}


void hdr_shot()
{
	if (shooting_mode == SHOOTMODE_MOVIE)
	{
		hdr_take_mov(hdr_steps, hdr_stepsize);
	}
	else
	{
		hdr_take_pics(hdr_steps, hdr_stepsize);
	}
}

void display_shooting_info() // called from debug task
{
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, 80, bg);

	if (wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 320, 260, "%5dK", kelvins);
	}

	bg = bmp_getpixel(680, 40);
	fnt = FONT(FONT_MED, 80, bg);
	int iso = get_current_iso();
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
}

static void
shoot_task( void )
{
	int i = 0;
    menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
    menu_add( "Expo", expo_menus, COUNT(expo_menus) );
	while(1)
	{
		if (gui_state == GUISTATE_PLAYMENU)
		{
			intervalometer_running = 0;
			lcd_release_running = 0;
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
		
		if (intervalometer_running)
		{
			msleep(1000);
			if (gui_menu_shown() || gui_state == GUISTATE_PLAYMENU) continue;
			hdr_shot();
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
			if (drive_mode != 0 && drive_mode != 1) // timer modes break this function (might lock the camera)
			{
				bmp_printf(FONT_MED, 20, 3, "LCD RemoteShot works if DriveMode is SINGLE or CONTINUOUS");
				continue;
			}
			bmp_printf(FONT_MED, 20, 3, "Move your hand near LCD face sensor to take a picture!");
			if (display_sensor_active())
			{
				hdr_shot();
				while (display_sensor_active()) { msleep(500); }
			}
		}
		else if (trap_focus)
		{
			msleep(1);
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


