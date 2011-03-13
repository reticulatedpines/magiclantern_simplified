/** \file
 * Magic Lantern GUI main task.
 *
 * Overrides the DryOS gui_main_task() to be able to re-map events.
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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
#include "property.h"
#include "bmp.h"
#include "config.h"
#include "consts-550d.109.h"
#include "lens.h"

static PROP_INT(PROP_GUI_STATE, gui_state);
static PROP_INT(PROP_DISPSENSOR_CTRL, display_sensor_neg);
static PROP_INT(PROP_HOUTPUT_TYPE, houtput_type);
static PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
PROP_INT(PROP_MVR_REC_START, recording);

int button_menu_on = BGMT_TRASH;
int button_menu_off = BGMT_TRASH;
int button_center_lvafframe = BGMT_PRESS_SET;

// halfshutter press is easier to detect from GUI events (PROP_HALFSHUTTER works only in LV mode)
int halfshutter_pressed = 0;
int get_halfshutter_pressed() 
{ 
	return halfshutter_pressed; 
}

int zoom_in_pressed = 0;
int zoom_out_pressed = 0;
int get_zoom_in_pressed() { return zoom_in_pressed; }
int get_zoom_out_pressed() { return zoom_out_pressed; }

struct semaphore * gui_sem;

struct gui_main_struct {
	void *			obj;		// off_0x00;
	uint32_t		counter;	// off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	struct msg_queue *	msg_queue;	// off_0x34;
	struct msg_queue *	msg_queue_550d;	// off_0x38;
	uint32_t		off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

struct gui_timer_struct
{
	void *			obj;	// off_0x00
};

extern struct gui_timer_struct gui_timer_struct;

extern void* gui_main_task_functbl;

CONFIG_INT("set.on.halfshutter", set_on_halfshutter, 1);

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	static int kev = 0;

	// volume adjust (FLASH + UP/DOWN)
	if (shooting_mode == SHOOTMODE_MOVIE && gui_state == GUISTATE_IDLE && FLASH_BTN_MOVIE_MODE)
	{
		if (event->type == 0 && event->param == BGMT_PRESS_UP)
		{
			volume_up();
			falsecolor_cancel();
			return 0;
		}
		if (event->type == 0 && event->param == BGMT_PRESS_DOWN)
		{
			volume_down();
			falsecolor_cancel();
			return 0;
		}
	}

	// event 0 is button press maybe?
	if( gui_state != GUISTATE_PLAYMENU && event->type == 0 )
	{
		if (event->param == button_menu_on && !gui_menu_shown()) 
		{
			give_semaphore( gui_sem );
			return 0;
		}
		if (event->param == button_menu_off && gui_menu_shown()) 
		{
			gui_stop_menu();
			return 0;
		}
		if (lv_drawn() && event->param == button_center_lvafframe && !gui_menu_shown())
		{
			center_lv_afframe();
			return 0;
		}
	}
	if (get_draw_event())
	{
		if (event->type == 0)
		{
			kev++;
			bmp_printf(FONT_SMALL, 0, 460, "Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x", 
				kev, 
				event->type, 
				event->param, 
				event->obj ? *(uint32_t*)(event->obj) : 0,
				event->obj ? *(uint32_t*)(event->obj + 4) : 0,
				event->obj ? *(uint32_t*)(event->obj + 8) : 0,
				event->arg);
			console_printf(FONT_SMALL, 0, 460, "Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x\ns", 
				kev, 
				event->type, 
				event->param, 
				event->obj ? *(uint32_t*)(event->obj) : 0,
				event->obj ? *(uint32_t*)(event->obj + 4) : 0,
				event->obj ? *(uint32_t*)(event->obj + 8) : 0,
				event->arg);
			//msleep(250);
		}
	}
	
	if (gui_menu_shown() && event->type == 0) // some buttons hard to detect from main menu loop
	{
		if (lv_drawn() && event->param == BGMT_UNPRESS_HALFSHUTTER || event->param == BGMT_UNPRESS_ZOOMOUT_MAYBE) // zoom out unpress, shared with halfshutter
		{
			gui_hide_menu( 2 );
			lens_focus_stop();
			return 0;
		}
		if (lv_drawn() && (event->param == BGMT_PRESS_HALFSHUTTER || event->param == BGMT_PRESS_ZOOMOUT_MAYBE)) // zoom out press, shared with halfshutter
		{
			gui_hide_menu( 50 );
			lens_focus_start( get_focus_dir() );
			return 0;
		}
	}
	
	if (event->type == 0 && display_sensor_neg == 0) // button presses while display sensor is covered
	{ // those are shortcut keys
		if (get_backlight_keys() && !gui_menu_shown())
		{
			if (event->param == BGMT_PRESS_UP)
			{
				adjust_backlight_level(1);
				return 0;
			}
			else if (event->param == BGMT_PRESS_DOWN)
			{
				adjust_backlight_level(-1);
				return 0;
			}
		}
	}

	if (event->type == 0)
	{
		if (is_follow_focus_active() && !is_manual_focus() && !gui_menu_shown() && lv_drawn() && display_sensor_neg != 0)
		{
			switch(event->param)
			{
				case BGMT_PRESS_LEFT:
					lens_focus_start(1 * get_follow_focus_dir_h());
					return 0;
				case BGMT_PRESS_RIGHT:
					lens_focus_start(-1 * get_follow_focus_dir_h());
					return 0;
				case BGMT_PRESS_UP:
					lens_focus_start(5 * get_follow_focus_dir_v());
					return 0;
				case BGMT_PRESS_DOWN:
					lens_focus_start(-5 * get_follow_focus_dir_v());
					return 0;
				case BGMT_UNPRESS_LEFT:
				case BGMT_UNPRESS_RIGHT:
				case BGMT_UNPRESS_UP:
				case BGMT_UNPRESS_DOWN:
					lens_focus_stop();
					return 0;
			}
		}
	}
	
	if (event->type == 0)
	{
		if (event->param == BGMT_PRESS_HALFSHUTTER) halfshutter_pressed = 1;
		if (event->param == BGMT_UNPRESS_HALFSHUTTER) halfshutter_pressed = 0;
	}
	
	// force a SET press in photo mode when you adjust the settings and press half-shutter
	if (set_on_halfshutter && event->type == 0 && event->param == BGMT_PRESS_HALFSHUTTER && gui_state == GUISTATE_PLAYMENU && !lv_drawn())
	{
		fake_simple_button(BGMT_PRESS_SET);
		msleep(50);
		fake_simple_button(BGMT_UNPRESS_SET);
		msleep(50);
	}
	
	// for faster zoom in in Play mode
	if (event->type == 0)
	{
		if (event->param == BGMT_PRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 1; zoom_out_pressed = 0; }
		if (event->param == BGMT_UNPRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 0; zoom_out_pressed = 0; }
		if (event->param == BGMT_PRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 1; zoom_in_pressed = 0; }
		if (event->param == BGMT_UNPRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 0; zoom_in_pressed = 0; }
 	}
	
	// override DISP button in LiveView mode
	if (event->type == 0 && event->param == BGMT_DISP && lv_drawn() && !gui_menu_shown() && gui_state == GUISTATE_IDLE)
	{
		if (houtput_type == 0)
		{
			return toggle_disp_mode();
		}
	}
	
	// MENU while recording => force a redraw
	if (recording && event->type == 0 && event->param == BGMT_MENU)
	{
		lv_redraw();
	}
	
	// stop intervalometer with MENU or PLAY
	if (event->type == 0 && (event->param == BGMT_MENU || event->param == BGMT_PLAY))
		intervalometer_stop();
		
	
	// zoom overlay
	
	if (get_zoom_overlay_z() && recording && event->type == 0 && event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
	{
		zoom_overlay_toggle();
	}
	
	if (recording && get_zoom_overlay_mode())
	{
		if (event->type == 0 && event->param == BGMT_PRESS_LEFT)
			move_lv_afframe(-200, 0);
		if (event->type == 0 && event->param == BGMT_PRESS_RIGHT)
			move_lv_afframe(200, 0);
		if (event->type == 0 && event->param == BGMT_PRESS_UP)
			move_lv_afframe(0, -200);
		if (event->type == 0 && event->param == BGMT_PRESS_DOWN)
			move_lv_afframe(0, 200);
	}

/*
	
	if (event->param == 0 && *(uint32_t*)(event->obj) == PROP_SHUTTER)
	{
		int value = *(int*)(event->obj + 4);
		bmp_printf(FONT_LARGE, 0, 0, "Tv %d", value);
		DEBUG("Tv %d", value);
	}
	if (event->param == 0 && *(uint32_t*)(event->obj) == PROP_APERTURE)
	{
		int value = *(int*)(event->obj + 4);
		bmp_printf(FONT_LARGE, 0, 0, "Av %d", value);
		DEBUG("Av %d", value);
		
		static int old = 0;
		
		if (old && lv_drawn())
		{
			if (display_sensor_neg == 0)
			{
				if (value != old)
				{
					int newiso = lens_info.raw_iso + value - old;
					if (newiso >= 72 && newiso <= 120)
					{
						lens_set_rawiso(newiso);
					}
					else return 0;
				}
			}
		}
		old = value; 

	}
	if (event->param == 5 && *(uint32_t*)(event->obj) == 0x80010001)
	{
		DEBUG("limit");
		bmp_printf(FONT_MED, 0, 0, "Limit %8x %8x %8x %8x", 
			*(uint32_t*)(event->obj + 4), 
			*(uint32_t*)(event->obj + 8), 
			event->param,
			event->arg);
	}
	
	*/

	if (event->param == 0 && *(uint32_t*)(event->obj) == PROP_APERTURE)
	{
		int value = *(int*)(event->obj + 4);
		//~ bmp_printf(FONT_LARGE, 0, 0, "Av %d", value);
		//~ DEBUG("Av %d", value);
		
		static int old = 0;
		
		if (get_dof_adjust() && old && lv_drawn())
		{
			if (display_sensor_neg == 0)
			{
				if (value != old)
				{
					int newiso = lens_info.raw_iso + value - old;
					if (newiso >= 72 && newiso <= 120)
					{
						lens_set_rawiso(newiso);
					}
					else return 0;
				}
			}
		}
		old = value; 
	}

	return 1;
}

static void gui_main_task_550d()
{
	struct event * event = NULL;
	int index = 0;
	void* funcs[GMT_NFUNCS];
	memcpy(funcs, GMT_FUNCTABLE, 4*GMT_NFUNCS);
	gui_init_end();
	while(1)
	{
		msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
		gui_main_struct.counter--;
		if (event == NULL) continue;
		index = event->type;
		if ((index >= GMT_NFUNCS) || (index < 0))
			continue;
		
		if (handle_buttons(event) == 0) continue;
		
		void(*f)(struct event *) = funcs[index];
		f(event);
	}
} 

// 5D2 has a different version for gui_main_task

TASK_OVERRIDE( gui_main_task, gui_main_task_550d );
