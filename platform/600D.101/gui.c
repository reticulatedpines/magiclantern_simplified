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

#include "../../dryos.h"
#include "../../bmp.h"
#include "../../consts.h"
#include "../../propvalues.h"




// Shutter state flags
// halfshutter press is easier to detect from GUI events (PROP_HALFSHUTTER works only in LV mode)
int halfshutter_pressed = 0;
int zoom_in_pressed = 0;
int zoom_out_pressed = 0;
int get_halfshutter_pressed() { return halfshutter_pressed; }
int get_zoom_in_pressed() { return zoom_in_pressed; }
int get_zoom_out_pressed() { return zoom_out_pressed; }


// Unknown declarations here
struct semaphore * gui_sem;

int handle_buttons_active = 0;
struct event fake_event;
struct semaphore * fake_sem;

int swap_menu = 0; // not used on 600D


// This is the main function in gui.c
// Return 0 if you want to block this event
// (But AFAIK, it crashes the camera)
// SO : ALWAYS RETURN 1 by default (for now);
static int handle_buttons(struct event * event) {
    int output = 1;
    // Check for button handling event type and camera state
    if(event->type == BGMT_BUTTON_HANDLING_EVENT_TYPE) {
        // This is button handling. This is for us!
        // All button code contants in "consts-600d.101.h"
        switch(event->param) {
            case BGMT_UNKNOWN1 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN2 :
                 // do nothing for now
                break;
            case BGMT_UNKNOWN3 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN4 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN5 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN6 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN7 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN8 :
                // do nothing for now
                break;
            case BGMT_UNKNOWN9 :
                // do nothing for now
                break;
            case BGMT_MENU :
                // Do nothing for now
                break;
            case BGMT_INFO :
                // Do nothing for now
                break;
            case BGMT_PRESS_DISP :
                // Do nothing for now
                break;
            case BGMT_UNPRESS_DISP :
                // Do nothing for now
                break;
            case BGMT_PLAY :
                // Do nothing for now
                break;
            case BGMT_TRASH :
                // Displays ML menu, except when in "play mode
                if(gui_state != GUISTATE_PLAYMENU) {
                    // Toggle menu on/off
                    if(gui_menu_shown()) gui_stop_menu();
                    else give_semaphore(gui_sem);
                    output = 0;
                }
                break;
            case BGMT_ZOOM_OUT :
                // Do nothing for now
                break;
            case BGMT_Q :
                // Causes certain buttons to not be triggered (known examples are LEFT/RIGHT) under "Q" mode
                // Do nothing for now
                break;
            case BGMT_MOVIE_SHOOTING :
                // Do nothing for now
                break;
            case BGMT_PRESS_SET :
                // Do nothing for now
                break;
            case BGMT_UNPRESS_SET :
                // Only in menu mode
                break;
            case BGMT_PRESS_RIGHT :
                // Do nothing for now
                break;
            case BGMT_UNPRESS_RIGHT :
                // Not triggered when in "Q" mode menu
                // Do nothing for now
                break;
            case BGMT_PRESS_LEFT :
                // Do nothing for now
                break;
            case BGMT_UNPRESS_LEFT :
                // Not triggered when in "Q" mode menu
                // Do nothing for now
                break;
            case BGMT_PRESS_UP :
                // Do nothing for now
                break;
            case BGMT_UNPRESS_UP :
                // Do nothing for now
                break;
            case BGMT_PRESS_DOWN :
                // Do nothing for now
                break;
            case BGMT_UNPRESS_DOWN :
                // Do nothing for now
                break;
            case BGMT_ISO :
                // This button handling can be catched but not overriden (returning 0 does nothing)
                // Do nothing for now
                break;
            case BGMT_PRESS_HALFSHUTTER :
                // Shared with magnify/zoom out under certain circumstances
                // This button handling can be catched but not overriden (returning 0 does nothing)
                // Do nothing for now
                break;
            case BGMT_UNPRESS_HALFSHUTTER :
                // Shared with magnify/zoom out
                // Do nothing for now
                output = 0;
                break;
            case BGMT_PRESS_FULLSHUTTER :
                // As per 60D, we must return default "1" value here (not verified for 600D)
                // Do nothing for now
                break;
            case BGMT_SHUTDOWN :
                // This button handling can be catched but not overriden (returning 0 does nothing)
                // Do nothing for now
                break;
            default :
                bmp_printf(FONT_SMALL, 10, 50, "UNHANDLED BGMT           : %08x", event->param);
                DebugMsg( DM_MAGIC, 3,   "UNHANDLED BGMT : %08x", event->param);
                dumpf();
        }
    }
    return output;
}










struct gui_main_struct {
	void *			obj;		// off_0x00;
	uint32_t		counter;	// off_0x04;
	uint32_t		off_0x08;
	uint32_t		counter_600D;    //off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	struct msg_queue *	msg_queue_60d;	// off_0x30;
	struct msg_queue *	msg_queue;	// off_0x34;
	struct msg_queue *	msg_queue_550d;	// off_0x38;
	uint32_t		off_0x3c;
};
extern struct gui_main_struct gui_main_struct;

// if called from handle_buttons, only last fake button will be executed
// if called from some other task, the function waits until the previous fake button was handled
void fake_simple_button(int bgmt_code)
{
	if (!handle_buttons_active) take_semaphore(fake_sem, 0);
	fake_event.type = 0,
	fake_event.param = bgmt_code, 
	fake_event.obj = 0,
	fake_event.arg = 0,
	msg_queue_post(gui_main_struct.msg_queue_60d, &fake_event, 0, 0);
}

static void gui_main_task_600D() {
	struct event * event = NULL;
	int index = 0;
	void* funcs[GMT_NFUNCS];
	memcpy(funcs, GMT_FUNCTABLE, 4*GMT_NFUNCS);
	gui_init_end();
	while(1) {
		msg_queue_receive(gui_main_struct.msg_queue_60d, &event, 0);
		gui_main_struct.counter_600D--;
		if (event == NULL) continue;
		index = event->type;
		if ((index >= GMT_NFUNCS) || (index < 0))
			continue;
		
		if (!magic_is_off())
			if (handle_buttons(event) == 0) 
				continue;
		
		void(*f)(struct event *) = funcs[index];
		f(event);
	}
} 


TASK_OVERRIDE(gui_main_task, gui_main_task_600D);
