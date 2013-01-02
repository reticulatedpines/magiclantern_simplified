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

PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

struct semaphore * gui_sem;

int bgmt_av_status;
int get_bgmt_av_status() {
    return bgmt_av_status;
}

int update_bgmt_av_status(struct event * event) {
    if(!BGMT_AV) return -1;
    if(event == NULL) return -1;
    if(event->obj == NULL) return -1;
    int gmt_int_ev_obj = *(int*)(event->obj);
    switch(shooting_mode) {
        case SHOOTMODE_MOVIE:
        case SHOOTMODE_P:
        case SHOOTMODE_ADEP:
            if(gmt_int_ev_obj == 0x3010040) return 1;
            if(gmt_int_ev_obj == 0x1010040) return 0;
            break;
        case SHOOTMODE_M:
            if(gmt_int_ev_obj == 0x1010006) return 1;
            if(gmt_int_ev_obj == 0x3010006) return 0;
            break;
        case SHOOTMODE_AV:
        case SHOOTMODE_TV:
            if(gmt_int_ev_obj == (0x1010040+2*shooting_mode)) return 1;
            if(gmt_int_ev_obj == (0x3010040+2*shooting_mode)) return 0;
            break;
        default:
            return -1;
    }
    return -1; //Annoying compiler :)
}

// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
    if (event->type != 0) return 1; // only handle events with type=0 (buttons)
    if (handle_common_events_startup(event) == 0) return 0;
    extern int ml_started;
    static int t_press = 0;
    static int t_opened = 0;
    static int t_closed = 0;
    unsigned int dt = 0;
    unsigned int is_idle = (gui_state == GUISTATE_IDLE);
    if (!ml_started) return 1;
    bgmt_av_status = update_bgmt_av_status(event);
    if(!gui_menu_shown()) { // ML menu closed
        /** AV long/short press management code. Assumes that the press event is fired only once even if the button is held **/
        if(bgmt_av_status == 1) { // AV PRESSED
            dt = get_ms_clock_value() - t_closed; // Time elapsed since the menu was closed
            if(dt > 500) { // Ignore if the menu was closed less than half a second ago (anti-bump)
                t_press = get_ms_clock_value();
            } else {
                t_press = 0;
            }
        } else if (bgmt_av_status == 0) { // AV UNPRESSED
            dt = get_ms_clock_value() - t_press; // Time elapsed since the AV button was pressed
            if (dt < 200 && is_idle) { // 200ms  -> short press -> open ML menu
                give_semaphore( gui_sem );
                t_opened = get_ms_clock_value();
                return 0;
            }
        }
    } else { // ML menu open
        if (event->param == BGMT_TRASH || bgmt_av_status == 0) {
            unsigned int dt = get_ms_clock_value() - t_opened;
            if(dt > 500) {
                gui_stop_menu();
                t_closed = get_ms_clock_value(); // Remember when we closed the menu
            }
            return 0;
        }
    }

    if (handle_common_events_by_feature(event) == 0) return 0;
    
    return 1;
}


struct gui_main_struct {
  void *            obj;        // off_0x00;
  uint32_t        counter;    // off_0x04;
  uint32_t        off_0x08;;
  uint32_t        counter_1100d; // off_0x0c;
  uint32_t        off_0x10;
  uint32_t        off_0x14;
  uint32_t        off_0x18;
  uint32_t        off_0x1c;
  uint32_t        off_0x20;
  uint32_t        off_0x24;
  uint32_t        off_0x28;
  uint32_t        off_0x2c;
  struct msg_queue *    msg_queue_1100d; // off_0x30;
  struct msg_queue *    msg_queue;    // off_0x34;
  struct msg_queue *    msg_queue_550d;    // off_0x38;
  uint32_t        off_0x3c;
};

extern struct gui_main_struct gui_main_struct;

// updated for 1100d 104
static void gui_main_task_1100d()
{
    struct event * event = NULL;
    int index = 0;
    void* funcs[GMT_NFUNCS];
    memcpy(funcs, (void*)GMT_FUNCTABLE, 4*GMT_NFUNCS);
    gui_init_end(); // no params?
    while(1)
    {
        msg_queue_receive(gui_main_struct.msg_queue_1100d, &event, 0);
        gui_main_struct.counter_1100d--;
        if (event == NULL) continue;
        index = event->type;
        if (!magic_is_off() && event->type == 0)
        {
            if (handle_buttons(event) == 0) // ML button/event handler
            continue;
        }

        if (IS_FAKE(event)) event->arg = 0;
        //DebugMsg(DM_MAGIC, 3, "calling function: %d", funcs[index]);
        if ((index >= GMT_NFUNCS) || (index < 0))
            continue;

        void(*f)(struct event *) = funcs[index];
        f(event);
    }
} 

// 5D2 has a different version for gui_main_task

// uncomment this when you are ready to find buttons
TASK_OVERRIDE( gui_main_task, gui_main_task_1100d );
