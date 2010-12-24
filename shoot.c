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

CONFIG_INT( "interval.timer.index", interval_timer_index, 2 );
CONFIG_INT( "focus.trap", trap_focus, 1);
CONFIG_INT( "focus.trap.delay", trap_focus_delay, 500); // min. delay between two shots in trap focus

int intervalometer_running = 0;
int lcd_release_running = 0;


int timer_values[] = {1,2,5,10,30,60,300,900,3600};

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

int current_iso_code = 0;
PROP_HANDLER(PROP_ISO)
{
	current_iso_code = buf[0];
	return prop_cleanup( token, property );
}

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
		"ISO:         %d",
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

const int shutter_values[] = { 30, 33, 37, 40,  45,  50,  53,  57,  60,  67,  75,  80,  90, 100, 110, 115, 125, 135, 150, 160, 180, 200, 210, 220, 235, 250, 275, 300, 320, 360, 400, 435, 470, 500, 550, 600, 640, 720, 800, 875, 925,1000,1100,1200,1250,1400,1600,1750,1900,2000,2150,2300,2500,2800,3200,3500,3750,4000};
const int shutter_codes[]  = { 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152};


int current_shutter_code = 0;
PROP_HANDLER(PROP_SHUTTER)
{
	current_shutter_code = buf[0];
	return prop_cleanup( token, property );
}

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
		"Shutter:     1/%d",
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


int wb_mode = 0;
int kelvins = 0;
PROP_HANDLER(PROP_WB_MODE)
{
	wb_mode = buf[0];
	return prop_cleanup( token, property );
}
PROP_HANDLER(PROP_WB_KELVIN)
{
	kelvins = buf[0];
	return prop_cleanup( token, property );
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COERCE(x,lo,hi) MAX(MIN(x,hi),lo)

static void
kelvin_toggle( int sign )
{
	int k = kelvins;
	int mode = 9;
	k = (k/100) * 100;
	k = 1700 + mod(k - 1700 + sign * 100, 10100 - 1700);
	prop_request_change(PROP_WB_MODE, &mode, 4);
	prop_request_change(PROP_WB_MODE_MIRROR, &mode, 4);
	prop_request_change(PROP_WB_KELVIN, &k, 4);
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
	if (wb_mode == 9) // kelvin
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"White Bal : %dK",
			kelvins
		);
	}
	else
	{
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"White Bal: %s",
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

struct menu_entry shoot_menus[] = {
	{
		.priv		= &interval_timer_index,
		.display	= interval_timer_display,
		.select		= interval_timer_toggle,
		.select_reverse	= interval_timer_toggle_reverse,
	},
	{
		.priv		= &intervalometer_running,
		.select		= menu_binary_toggle_and_close,
		.display	= intervalometer_display,
	},
	{
		.priv		= &lcd_release_running,
		.select		= menu_binary_toggle_and_close,
		.display	= lcd_release_display,
	},
	{
		.display	= hdr_display,
		.select		= hdr_steps_toggle,
		.select_reverse = hdr_stepsize_toggle,
	},
	{
		.priv		= &trap_focus,
		.select		= menu_binary_toggle,
		.display	= trap_focus_display,
	},
	{
		.display	= iso_display,
		.select		= iso_toggle_forward,
		.select_reverse		= iso_toggle_reverse,
	},
	{
		.display	= shutter_display,
		.select		= shutter_toggle_forward,
		.select_reverse		= shutter_toggle_reverse,
	},
	{
		.display	= kelvin_display,
		.select		= kelvin_toggle_forward,
		.select_reverse		= kelvin_toggle_reverse,
	},
};

int display_sensor_active()
{
	return (*(int*)(DISPLAY_SENSOR_MAYBE));
}

PROP_HANDLER( PROP_GUI_STATE )
{
    int gui_state = buf[0];
    if (gui_state == 1) // PLAYMENU
    {
		intervalometer_running = 0;
		lcd_release_running = 0;
	}		
	return prop_cleanup( token, property );
}

// does not seem to have any effect...
PROP_HANDLER( PROP_HALF_SHUTTER )
{
    if (buf[0])
    {
		intervalometer_running = 0;
		lcd_release_running = 0;
	}		
	return prop_cleanup( token, property );
}

int drive_mode;
PROP_HANDLER( PROP_DRIVE )
{
	drive_mode = buf[0];
	return prop_cleanup( token, property );
}

int af_mode;
PROP_HANDLER(PROP_AF_MODE)
{
	af_mode = (int16_t)buf[0];
	return prop_cleanup( token, property );
}

int shooting_mode;
PROP_HANDLER(PROP_SHOOTING_MODE)
{
	shooting_mode = (int16_t)buf[0];
	return prop_cleanup( token, property );
}
static void
shoot_task( void )
{
	int i = 0;
    menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
	while(1)
	{
		if (intervalometer_running)
		{
			msleep(1000);
			hdr_shot();
			for (i = 0; i < timer_values[interval_timer_index] - 1; i++)
			{
				msleep(1000);
				if (intervalometer_running) bmp_printf(FONT_MED, 20, 35, "Press PLAY or MENU to stop the intervalometer.");
			}
		}
		else if (lcd_release_running)
		{
			msleep(20);
			if (lv_drawn()) 
			{
				bmp_printf(FONT_MED, 20, 35, "LCD RemoteShot does not work in LiveView, sorry...");
				continue;
			}
			if (drive_mode != 0 && drive_mode != 1) // timer modes break this function (might lock the camera)
			{
				bmp_printf(FONT_MED, 20, 35, "LCD RemoteShot works if DriveMode is SINGLE or CONTINUOUS");
				continue;
			}
			bmp_printf(FONT_MED, 20, 35, "Move your hand near LCD face sensor to take a picture!");
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
				lens_take_picture(1000);
				msleep(trap_focus_delay);
			}
		}
		else msleep(500);
	}
}

void hdr_take_pics(int steps, int step_size)
{
	int i;
	if (shooting_mode == 3) // manual
	{
		const int s = current_shutter_code;
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			int new_s = COERCE(s - step_size * i, 0x10, 152);
			lens_set_shutter( new_s );
			lens_take_picture( 100000 );
		}
		msleep(100);
		lens_set_shutter( s );
	}
	else
	{
		const int ae = lens_get_ae();
		for( i = -steps/2; i <= steps/2; i ++  )
		{
			int new_ae = ae + step_size * i;
			lens_set_ae( new_ae );
			lens_take_picture( 100000 );
		}
		lens_set_ae( ae );
	}
}

void hdr_shot()
{
	hdr_take_pics(hdr_steps, hdr_stepsize);
}


TASK_CREATE( "shoot_task", shoot_task, 0, 0x18, 0x1000 );


