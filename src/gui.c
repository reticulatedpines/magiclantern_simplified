/**
 * MagicLantern GuiMainTask override
 * This was previously camera-specific
 **/

#ifdef CONFIG_650D
#error "UNSUPPORTED"
#endif

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
 * [D] 5D3  : counter_0x0c <-> msg_queue_0x30
 * [D] 6D   : counter_0x0c <-> msg_queue_0x30
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

#ifdef CONFIG_GUI_DEBUG
int event_ctr = 0;
#endif

#ifdef FEATURE_DETECT_AV_SHORT
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

int handle_av_for_short_press(struct event* event) {
    static int t_press   = 0;
    static int t_unpress = 0;
    unsigned int dt = 0;
    unsigned int is_idle = (gui_state == GUISTATE_IDLE);
    bgmt_av_status = update_bgmt_av_status(event);
    // We can't detect MLEV_AV_SHOT while in ML menu
    if(gui_menu_shown()) {
        t_unpress = 0;
        t_press = 0;
        return 1;
    }
    /** AV long/short press management code. Assumes that the press event is fired only once even if the button is held **/
    if(bgmt_av_status == 1) { // AV PRESSED
        t_press = get_ms_clock_value();
        dt = t_press - t_unpress; // Time elapsed since the button was unpressed
        if(dt < 500) { // Ignore if happened less than half a second ago (anti-bump)
            t_press = 0; 
        } 
    } else if (bgmt_av_status == 0) { // AV UNPRESSED
        t_unpress = get_ms_clock_value();
        dt = t_unpress - t_press; // Time elapsed since the AV button was pressed
        if (dt < 200 && is_idle) { // 200ms  -> short press
            fake_simple_button(MLEV_AV_SHORT);
            return 0;
        }
    }
    return 1;
} 
#endif //FEATURE_DETECT_AV_SHORT

#ifdef CONFIG_MENU_WITH_AV
#ifndef FEATURE_DETECT_AV_SHORT
#error "CONFIG_MENU_WITH_AV requires FEATURE_DETECT_AV_SHORT"
#endif

int handle_av_for_ml_menu(struct event* event) {
    static int last_opened = 0;
    int now = get_ms_clock_value();
    if(event->param == MLEV_AV_SHORT && !gui_menu_shown() && (now - last_opened) > 200) {
        last_opened = now;
        give_semaphore(gui_sem);
        return 0;
    } 
    return 1;
}

#endif //CONFIG_MENU_WITH_AV

#ifdef FEATURE_DIGITAL_ZOOM_SHORTCUT
PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);

int video_mode[5];
PROP_HANDLER(PROP_VIDEO_MODE)
{
	memcpy(video_mode, buf, 20);
}

int disp_pressed = 0;
int get_disp_pressed() { return disp_pressed; }
int disp_zoom_pressed = 0;

int handle_digital_zoom_shortcut(struct event * event)
{
    switch(event->param) {
        case BGMT_PRESS_DISP:
            disp_pressed = 1; 
            disp_zoom_pressed = 0; 
        case BGMT_UNPRESS_DISP:
            disp_pressed = 0;
        case BGMT_PRESS_ZOOMIN_MAYBE: 
        case BGMT_PRESS_ZOOMOUT_MAYBE:
            disp_zoom_pressed = 1;
        default:
            break;
    }

	extern int digital_zoom_shortcut;
	if (digital_zoom_shortcut && lv && is_movie_mode() && disp_pressed)
	{
		if (!video_mode_crop)
		{
			if (video_mode_resolution == 0 && event->param == BGMT_PRESS_ZOOMIN_MAYBE)
			{
				if (!recording)
				{
					video_mode[0] = 0xc;
					video_mode[4] = 2;
					prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
				}
				return 0;
			}
		}
		else
		{
			if (event->param == BGMT_PRESS_ZOOMIN_MAYBE)
			{
				if (!recording)
				{
					int x = 300;
					prop_request_change(PROP_DIGITAL_ZOOM_RATIO, &x, 4);
				}
				NotifyBox(2000, "Zoom greater than 3x is disabled.\n");
				return 0; // don't allow more than 3x zoom
			}
			if (event->param == BGMT_PRESS_ZOOMOUT_MAYBE)
			{
				if (!recording)
				{
					video_mode[0] = 0;
					video_mode[4] = 0;
					prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
				}
				return 0;
			}
		}
	}
    return 1;
}
#endif //FEATURE_DIGITAL_ZOOM_SHORTCUT


// return 0 if you want to block this event
static int handle_buttons(struct event * event)
{
	ASSERT(event->type == 0)

    if (event->type != 0) return 1; // only handle events with type=0 (buttons)
    if (handle_common_events_startup(event) == 0) return 0;
    extern int ml_started;
    if (!ml_started) return 1;
#ifdef FEATURE_LV_FOCUS_SNAP
    if (lv && event->param == BGMT_PRESS_SET && !gui_menu_shown())
    {
        center_lv_afframe();
        return 0;
    }
#endif
#ifdef FEATURE_DETECT_AV_SHORT
    if (handle_av_for_short_press(event) == 0) return 0;
    #ifdef CONFIG_MENU_WITH_AV
    if (handle_av_for_ml_menu(event) == 0) return 0;
    #endif
#endif
#ifdef FEATURE_DIGITAL_ZOOM_SHORTCUT
    if (handle_digital_zoom_shortcut(event) == 0) return 0;
#endif

    if (handle_common_events_by_feature(event) == 0) return 0;
    
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
        if (event == NULL) continue;
        index = event->type;

#ifdef CONFIG_GUI_DEBUG
        if (event->type == 0
            && event->param != 0x69
            && event->param != 0x11
            && event->param != 0xf
            && event->param != 0x54
            )   //~ block some common events
        {
            console_printf("[%d] event->param: 0x%x\n", event_ctr++, event->param);
        }
#endif

		if (!magic_is_off())
		{
			if (event->type == 0)
			{
				if (handle_buttons(event) == 0) // ML button/event handler
					continue;
			}
			else
			{
				if (handle_other_events(event) == 0)
					continue;
			}
		}

        if (IS_FAKE(event)) event->arg = 0;

        if ((index >= GMT_NFUNCS) || (index < 0))
            continue;

        void(*f)(struct event *) = funcs[index];
        f(event);
    }
} 

TASK_OVERRIDE( gui_main_task, ml_gui_main_task);
