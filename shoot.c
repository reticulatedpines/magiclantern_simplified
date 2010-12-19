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
		"IntervalTime: %4ds", 
		timer_values[*(int*)priv]
	);
}

static void
interval_timer_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 1) % COUNT(timer_values);
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
		"Trap Focus: %s",
		(*(int*)priv) ? "ON " : "OFF"
	);
}

const int iso_values[] = {100,125,160,200,250,320,400,500,640,800,1000,1250,1600,2000,2500,3200,4000,4500,5000,6400,7000,8000,12500, 25600};
const int iso_codes[]  = { 72, 75, 77, 80, 83, 85, 88, 91, 93, 96,  99, 101, 104, 107, 109, 112, 115, 116, 117, 120, 121, 122,  128,   136};
int iso_index = 0;
static void 
iso_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"IsoOverride:%d",
		iso_values[*(int*)priv]
	);
}

static void
iso_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 1) % COUNT(iso_values);
	lens_set_iso(iso_codes[*ptr]);
}



struct menu_entry shoot_menus[] = {
	{
		.priv		= &interval_timer_index,
		.select		= interval_timer_toggle,
		.display	= interval_timer_display,
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
		.priv		= &trap_focus,
		.select		= menu_binary_toggle,
		.display	= trap_focus_display,
	},
	{
		.priv		= &iso_index,
		.select		= iso_toggle,
		.display	= iso_display,
	},
};

int display_sensor_active()
{
	return (*(int*)(0x2dec));
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

static void
shoot_task( void )
{
	int i = 0;
    menu_add( "Shoot", shoot_menus, COUNT(shoot_menus) );
	while(1)
	{
		if (intervalometer_running)
		{
			lens_take_picture(0);
			for (i = 0; i < timer_values[interval_timer_index]; i++)
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
				call( "Release" ); // lens_take_picture may cause black screen (maybe the semaphore messes it up)
				while (display_sensor_active()) { msleep(500); }
			}
		}
		else if (trap_focus)
		{
			msleep(10);
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
			if (*(int*)0x41d0)
			{
				call( "Release" ); // lens_take_picture may cause black screen (maybe the semaphore messes it up)
				msleep(trap_focus_delay);
			}
		}
		else msleep(500);
	}
}

TASK_CREATE( "shoot_task", shoot_task, 0, 0x18, 0x1000 );


