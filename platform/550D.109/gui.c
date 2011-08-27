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

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

#define FAKE_BTN -123456
#define IS_FAKE(event) (event->arg == FAKE_BTN)

unsigned int button_center_lvafframe = BGMT_PRESS_SET;

int swap_menu = 0; // not used on 550D

// halfshutter press is easier to detect from GUI events (PROP_HALFSHUTTER works only in LV mode)
int halfshutter_pressed = 0;
int get_halfshutter_pressed() { return halfshutter_pressed; }

int flash_movie_pressed = 0;
int get_flash_movie_pressed() { return flash_movie_pressed; }

int zoom_in_pressed = 0;
int zoom_out_pressed = 0;
int set_pressed = 0;
int get_zoom_in_pressed() { return zoom_in_pressed; }
int get_zoom_out_pressed() { return zoom_out_pressed; }
int get_set_pressed() { return set_pressed; }

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

//~ CONFIG_INT("set.on.halfshutter", set_on_halfshutter, 0);

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	if (event->type != 0) return 1; // only handle events with type=0 (buttons)

	extern int ml_started;
	if (!ml_started)
	{
		if (event->param == BGMT_LV)
			return 0; // discard REC button if it's pressed too early
		else
			return 1; // don't alter any other buttons/events until ML is fully initialized
	}
	
	if (event->param != 0x56)
 	{
		idle_wakeup_reset_counters();
 	}
 
	// AF patterns
	extern int af_patterns;
	if (af_patterns && !lv && gui_state == GUISTATE_IDLE && tft_status)
	{
		if (event->param == BGMT_PRESS_LEFT)   { afp_left(); return 0; }
		if (event->param == BGMT_PRESS_RIGHT)  { afp_right(); return 0; }
		if (event->param == BGMT_PRESS_UP)     { afp_top(); return 0; }
		if (event->param == BGMT_PRESS_DOWN)   { afp_bottom(); return 0; }
		if (event->param == BGMT_PRESS_SET)    { afp_center(); return 0; }
	}

	static int kev = 0;
	
	// volume adjust (FLASH + UP/DOWN) and ISO adjust (FLASH + LEFT/RIGHT)
	if (shooting_mode == SHOOTMODE_MOVIE && gui_state == GUISTATE_IDLE && FLASH_BTN_MOVIE_MODE)
	{
		if (event->param == BGMT_PRESS_UP)
		{
			kelvin_toggle(1);
			return 0;
		}
		if (event->param == BGMT_PRESS_DOWN)
		{
			kelvin_toggle(-1);
			return 0;
		}
		if (event->param == BGMT_PRESS_LEFT)
		{
			iso_toggle(-1);
			return 0;
		}
		if (event->param == BGMT_PRESS_RIGHT)
		{
			iso_toggle(1);
			return 0;
		}
	}

	if (event->param == BGMT_TRASH)
	{
		if (!gui_menu_shown() && gui_state == GUISTATE_IDLE) 
		{
			give_semaphore( gui_sem );
			return 0;
		}
		else if (gui_menu_shown())
		{
			gui_stop_menu();
			return 0;
		}
 	}
	
	if (get_draw_event())
	{
		if (1)
		{
			kev++;
			bmp_printf(FONT_SMALL, 0, 460, "Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x", 
				kev,
				event->type, 
				event->param, 
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 4)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 8)) : 0,
				event->arg);
/*			console_printf("Ev%d[%d]: p=%8x *o=%8x/%8x/%8x a=%8x\ns", 
				kev,
				event->type, 
				event->param, 
				event->obj ? ((int)event->obj & 0xf0000000 ? event->obj : *(uint32_t*)(event->obj)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? event->obj : *(uint32_t*)(event->obj + 4)) : 0,
				event->obj ? ((int)event->obj & 0xf0000000 ? event->obj : *(uint32_t*)(event->obj + 8)) : 0,
				event->arg);*/
			//msleep(250);
		}
	}
	
	if (gui_menu_shown()) // some buttons hard to detect from main menu loop
	{
		if (lv && event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
		{
			gui_hide_menu( 2 );
			lens_focus_stop();
			return 0;
		}
		if (lv && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
		{
			gui_hide_menu( 50 );
			lens_focus_start( get_focus_dir() );
			return 0;
		}
	}
	if (gui_menu_shown())
	{
		if (event->param == 0x5a) return 0;
	}
	
	if (get_lcd_sensor_shortcuts() && !gui_menu_shown() && display_sensor && DISPLAY_SENSOR_POWERED) // button presses while display sensor is covered
	{ // those are shortcut keys
		if (!gui_menu_shown())
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
		if (lv)
		{
			if (event->param == BGMT_PRESS_LEFT)
			{
				volume_down();
				return 0;
			}
			else if (event->param == BGMT_PRESS_RIGHT)
			{
				volume_up();
				return 0;
			}
		}
	}

	if (1)
	{
		if (is_follow_focus_active() && !is_manual_focus() && !gui_menu_shown() && lv && (!display_sensor || !get_lcd_sensor_shortcuts()) && gui_state == GUISTATE_IDLE)
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
	
	if (1)
	{
		if (event->param == BGMT_PRESS_HALFSHUTTER) halfshutter_pressed = 1;
		if (event->param == BGMT_UNPRESS_HALFSHUTTER) halfshutter_pressed = 0;
	}
	
	// force a SET press in photo mode when you adjust the settings and press half-shutter
	/*if (set_on_halfshutter && event->param == BGMT_PRESS_HALFSHUTTER && gui_state == GUISTATE_PLAYMENU && !lv && !gui_menu_shown())
	{
		fake_simple_button(BGMT_PRESS_SET);
		fake_simple_button(BGMT_UNPRESS_SET);
	}*/
	
	// for faster zoom in in Play mode
	if (1)
	{
		if (event->param == BGMT_PRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 1; zoom_out_pressed = 0; }
		if (event->param == BGMT_UNPRESS_ZOOMIN_MAYBE) {zoom_in_pressed = 0; zoom_out_pressed = 0; }
		if (event->param == BGMT_PRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 1; zoom_in_pressed = 0; }
		if (event->param == BGMT_UNPRESS_ZOOMOUT_MAYBE) { zoom_out_pressed = 0; zoom_in_pressed = 0; }
 	}
		
	// MENU while recording => force a redraw
	if (recording && event->param == BGMT_MENU)
	{
		redraw();
	}
	
	// stop intervalometer with MENU or PLAY
	if (!IS_FAKE(event) && (event->param == BGMT_MENU || event->param == BGMT_PLAY) && !gui_menu_shown())
		intervalometer_stop();
		
	
	// zoom overlay
	
	if (get_zoom_overlay_mode() && recording == 2 && event->param == BGMT_UNPRESS_ZOOMIN_MAYBE)
	{
		zoom_overlay_toggle();
		return 0;
	}

	if (lv && get_zoom_overlay() && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
	{
		zoom_overlay_toggle();
		return 0;
	}
	
	if (lv && get_zoom_overlay_mode() && lv_dispsize == 1 && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
	{
		// magic zoom toggled by sensor+zoom in
		if (get_zoom_overlay_mode() != 3 && get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED)
		{
			zoom_overlay_toggle();
			return 0;
		}
		// magic zoom toggled by zoom in, normal zoom by sensor+zoom in
		else if (get_zoom_overlay_mode() == 3 && !get_halfshutter_pressed() && !(get_lcd_sensor_shortcuts() && display_sensor && DISPLAY_SENSOR_POWERED))
		{
			zoom_overlay_toggle();
			return 0;
		}
	}
	
	if (recording && get_zoom_overlay_mode())
	{
		if (event->param == BGMT_PRESS_LEFT)
			move_lv_afframe(-200, 0);
		if (event->param == BGMT_PRESS_RIGHT)
			move_lv_afframe(200, 0);
		if (event->param == BGMT_PRESS_UP)
			move_lv_afframe(0, -200);
		if (event->param == BGMT_PRESS_DOWN)
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
		
		if (old && lv)
		{
			if (display_sensor)
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
	
	// quick access to some menu items
	if (event->param == BGMT_Q_ALT && !gui_menu_shown())
	{
		if (ISO_ADJUSTMENT_ACTIVE)
		{
			select_menu("Expo", 0);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (CURRENT_DIALOG_MAYBE == DLG_FOCUS_MODE)
		{
			select_menu("Focus", 0);
			give_semaphore( gui_sem ); 
			return 0;
		}
		/*
		else if (CURRENT_DIALOG_MAYBE == DLG_WB)
		{
			select_menu("Expo", 1);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (CURRENT_DIALOG_MAYBE == DLG_FOCUS_MODE)
		{
			select_menu("Shoot", 5);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (CURRENT_DIALOG_MAYBE == DLG_DRIVE_MODE)
		{
			select_menu("Shoot", 3);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (CURRENT_DIALOG_MAYBE == DLG_PICTURE_STYLE)
		{
			select_menu("Expo", 7);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (CURRENT_DIALOG_MAYBE == DLG_FLASH_AE)
		{
			select_menu("Expo", 9);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (CURRENT_DIALOG_MAYBE == DLG_PICQ)
		{
			select_menu("Debug", 2);
			give_semaphore( gui_sem ); 
			return 0;
		}
		else if (lv_dispsize > 1)
		{
			select_menu("LiveV", 8);
			give_semaphore( gui_sem ); 
			return 0;
		}*/
		
	}

/*	if (event->param == 0 && *(uint32_t*)(event->obj) == PROP_APERTURE)
	{
		int value = *(int*)(event->obj + 4);
		//~ bmp_printf(FONT_LARGE, 0, 0, "Av %d", value);
		//~ DEBUG("Av %d", value);
		
		static int old = 0;
		
		if (get_lcd_sensor_shortcuts() && !gui_menu_shown() && get_dof_adjust() && old && lv)
		{
			if (display_sensor)
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
	}*/

	// movie mode shortcut
	if (event->param == BGMT_LV && ISO_ADJUSTMENT_ACTIVE)
	{
		if (shooting_mode != SHOOTMODE_MOVIE)
		{
			set_shooting_mode(SHOOTMODE_MOVIE);
			return 0;
		}
	}

	if (event->param == BGMT_DISP && ISO_ADJUSTMENT_ACTIVE && gui_state == GUISTATE_IDLE)
	{
		toggle_disp_mode();
		return 0;
	}

	if (lv && !gui_menu_shown() && event->param == BGMT_DISP)
	{
		redraw();
	}
		
	// enable LiveV stuff in Play mode
	if ( PLAY_MODE) 
	{
		if (event->param == BGMT_Q_ALT)
		{
			livev_playback_toggle();
			return 0;
		}
		else
			livev_playback_reset();
	}

	// transparent overlay
	extern int transparent_overlay;

	if (transparent_overlay && event->param == BGMT_LV && (gui_state == GUISTATE_QR || PLAY_MODE))
	{
		schedule_transparent_overlay();
		return 0;
	}

	if (transparent_overlay && lv && gui_state == GUISTATE_IDLE && !gui_menu_shown())
	{
		if (event->param == BGMT_PRESS_UP)
		{
			transparent_overlay_offset(0, -40);
			return 0;
		}
		if (event->param == BGMT_PRESS_DOWN)
		{
			transparent_overlay_offset(0, 40);
			return 0;
		}
		if (event->param == BGMT_PRESS_LEFT)
		{
			transparent_overlay_offset(-40, 0);
			return 0;
		}
		if (event->param == BGMT_PRESS_RIGHT)
		{
			transparent_overlay_offset(40, 0);
			return 0;
		}
		if (event->param == BGMT_PRESS_SET)
		{
			transparent_overlay_offset_clear();
			transparent_overlay_offset(0, 0);
			return 0;
		}
	}

	if (BGMT_FLASH_MOVIE)
	{
		flash_movie_pressed = BGMT_PRESS_FLASH_MOVIE;
		return !BGMT_PRESS_FLASH_MOVIE;
	}

	if (lv && event->param == button_center_lvafframe && !gui_menu_shown())
	{
		center_lv_afframe();
		return 0;
	}

	// 422 play

	if (event->param == BGMT_PRESS_SET) set_pressed = 1;
	if (event->param == BGMT_UNPRESS_SET) set_pressed = 0;
	if (event->param == BGMT_PLAY) set_pressed = 0;

	if ( PLAY_MODE && event->param == BGMT_WHEEL_RIGHT && get_set_pressed())
	{
		play_next_422();
		return 0;
	}

	return 1;
}

void fake_simple_button(int bgmt_code)
{
	GUI_Control(bgmt_code, 0, FAKE_BTN, 0);
}

void send_event_to_IDLEHandler(int event)
{
	ctrlman_dispatch_event((void*)GMT_IDLEHANDLER_TASK, event, 0, 0);
}

static void gui_main_task_550d()
{
	bmp_sem_init();
	struct event * event = NULL;
	int index = 0;
	void* funcs[GMT_NFUNCS];
	memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
	gui_init_end();
	while(1)
	{
		msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
		gui_main_struct.counter--;
		if (event == NULL) continue;
		index = event->type;
		
		if (!magic_is_off() && event->type == 0)
		{
			if (handle_buttons(event) == 0) // ML button/event handler
				continue;
		}
		
		if (IS_FAKE(event)) event->arg = 0;

		if ((index >= GMT_NFUNCS) || (index < 0))
			continue;
		
		// sync with other Canon calls => prevents some race conditions
 		// weak version will timeout after 300ms
 		// so if there's some hidden bug, it will not freeze at least
		// not a good programming practice... but works for an undocumented system
		GMT_LOCK_WEAK(
			void(*f)(struct event *) = funcs[index];
			f(event);
		)
	}
} 

// 5D2 has a different version for gui_main_task

TASK_OVERRIDE( gui_main_task, gui_main_task_550d );
