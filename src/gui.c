/**
 * MagicLantern GuiMainTask override
 * This was previously camera-specific
 **/

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

#include <gui.h>

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <config-defines.h>
/**
 * Supported cameras [E] Means it's enabled
 * [E] 1100D: counter_0x0c <-> msg_queue_0x30
 * [E] 600D : counter_0x0c <-> msg_queue_0x30
 * [E] 60D  : counter_0x0c <-> msg_queue_0x30
 * [E] 650D : counter_0x0c <-> msg_queue_0x30
 * [E] EOSM : counter_0x0c <-> msg_queue_0x30
 * [E] 5D3  : counter_0x0c <-> msg_queue_0x30
 * [E] 6D   : counter_0x0c <-> msg_queue_0x30
 */

/**
 * Easy to support cameras
 * [E] 550D : counter_0x04 <-> msg_queue_0x38
 * [D] 7D   : counter_0x04 <-> msg_queue_0x38
 */

/**
 * Unsupported cameras for now
 * 5D2  : counter_0x04 <-> msg_queue_0x34
 * 50D  : counter_0x04 <-> msg_queue_0x34
 * 500D : counter_0x04 <-> msg_queue_0x34
 */

struct semaphore * gui_sem;

#ifdef FEATURE_JOY_CENTER_ACTIONS
static int joy_center_press_count = 0;
static int joy_center_action_disabled = 0;
static void joypress_task()
{
	extern int joy_center_pressed;
	TASK_LOOP
	{
		msleep(20);
		if (joy_center_pressed) joy_center_press_count++;
		else
		{
			if (!joy_center_action_disabled && gui_menu_shown() && joy_center_press_count && joy_center_press_count <= 20) // short press, ML menu active
			{
				if (is_submenu_or_edit_mode_active())
				{
					fake_simple_button(BGMT_Q); // close submenu
				}
				else
				{
					fake_simple_button(BGMT_PRESS_SET); // do normal SET
					fake_simple_button(BGMT_UNPRESS_UDLR);
				}
			}
			joy_center_press_count = 0;
		}

		if (!joy_center_action_disabled && joy_center_press_count > 20) // long press
		{
			joy_center_press_count = 0;
			fake_simple_button(BGMT_UNPRESS_UDLR);

			if (gui_menu_shown())
				fake_simple_button(BGMT_Q);
			else if (gui_state == GUISTATE_IDLE || gui_state == GUISTATE_QMENU || PLAY_MODE)
			{
				give_semaphore( gui_sem ); // open ML menu
				joy_center_press_count = 0;
				joy_center_pressed = 0;
			}
			msleep(500);
		}

	}
}
TASK_CREATE( "joypress_task", joypress_task, 0, 0x1a, 0x1000 );
#endif // FEATURE_JOY_CENTER_ACTIONS

#ifdef CONFIG_GUI_DEBUG
int event_ctr = 0;
#endif

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
    ASSERT(event->type == 0)

    if (event->type != 0) return 1; // only handle events with type=0 (buttons)
    if (handle_common_events_startup(event) == 0) return 0;
    extern int ml_started;
    if (!ml_started) return 1;


    if (handle_common_events_by_feature(event) == 0) return 0;

#ifdef FEATURE_JOY_CENTER_ACTIONS
	if (event->param == BGMT_JOY_CENTER && gui_menu_shown())
	{
		joy_center_press_count = 1;
		return 0; // handled above
	}

	if (event->param == BGMT_PRESS_LEFT || event->param == BGMT_PRESS_RIGHT ||
		event->param == BGMT_PRESS_DOWN || event->param == BGMT_PRESS_UP ||
		event->param == BGMT_PRESS_UP_LEFT || event->param == BGMT_PRESS_UP_RIGHT ||
		event->param == BGMT_PRESS_DOWN_LEFT || event->param == BGMT_PRESS_DOWN_RIGHT)
		joy_center_action_disabled = 1;

	if (event->param == BGMT_UNPRESS_UDLR)
		joy_center_action_disabled = 0;
    
#endif // FEATURE_JOY_CENTER_ACTIONS

    return 1;
}

struct gui_main_struct {
  void *          obj;        // off_0x00;
  uint32_t        counter_550d;
  uint32_t        off_0x08;
  uint32_t        counter; // off_0x0c;
  uint32_t        off_0x10;
  uint32_t        off_0x14;
  uint32_t        off_0x18;
  uint32_t        off_0x1c;
  uint32_t        off_0x20;
  uint32_t        off_0x24;
  uint32_t        off_0x28;
  uint32_t        off_0x2c;
  struct msg_queue *    msg_queue;    // off_0x30;
  struct msg_queue *    off_0x34;    // off_0x34;
  struct msg_queue *    msg_queue_550d;    // off_0x38;
  uint32_t        off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

#ifdef CONFIG_GUI_DEBUG
void show_event_codes(struct event * event)
{
    if ( event-> type == 0
            && event->param != 0x69
            && event->param != 0x11
            && event->param != 0xf
            && event->param != 0x54
       )   //~ block some common events
    {
        console_printf("[%d] event->param: 0x%x\n", event_ctr++, event->param);
    }
}
#endif

static void ml_gui_main_task()
{
    struct event * event = NULL;
    int index = 0;
    void* funcs[GMT_NFUNCS];
    memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
    gui_init_end(); // no params?
    while(1)
    {
        #if defined(CONFIG_550D) || defined(CONFIG_7D)
        msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
        gui_main_struct.counter_550d--;
        #else
        msg_queue_receive(gui_main_struct.msg_queue, &event, 0);
        gui_main_struct.counter--;
        #endif

        if (event == NULL) {
            continue;
        }

        index = event->type;

        if (!magic_is_off())
        {

            if (event->type == 0)
            {
                #ifdef CONFIG_GUI_DEBUG
                show_event_codes(event);
                #endif
                if (handle_buttons(event) == 0) { // ML button/event handler
                    continue;
                }
            }
            else
            {
                if (handle_other_events(event) == 0) {
                    continue;
                }
            }
        }

        if (IS_FAKE(event)) {
           event->arg = 0;
        }

        if ((index >= GMT_NFUNCS) || (index < 0)) {
            continue;
        }

        void(*f)(struct event *) = funcs[index];
        f(event);
    }
} 

TASK_OVERRIDE( gui_main_task, ml_gui_main_task);
