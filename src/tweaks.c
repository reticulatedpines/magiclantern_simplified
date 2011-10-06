/** \file
 * Tweaks to default UI behavior
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "propvalues.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "math.h"

#if defined(CONFIG_1100D)
#include "disable-this-module.h"
#endif

CONFIG_INT("dof.preview.sticky", dofpreview_sticky, 0);

static void
dofp_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"DOF Preview (photos): %s",
		dofpreview_sticky  ? "Sticky" : "Normal"
	);
}

static void
dofp_set(int value)
{
	prop_request_change(PROP_DOF_PREVIEW_MAYBE, &value, 2);
}

static void 
dofp_lock(void* priv)
{
	dofp_set(1);
}

static void 
dofp_update()
{
	static int state = 0;
	// 0: allow 0->1, disallow 1->0 (first DOF press)
	// 1: allow everything => reset things (second DOF presss)
	
	static int old_value = 0;
	
	//~ bmp_printf(FONT_MED, 0, 0, "DOFp: btn=%d old=%d state=%d hs=%d ", dofpreview, old_value, state, HALFSHUTTER_PRESSED);
	
	if (dofpreview_sticky == 1)
	{
		if (dofpreview) bmp_printf(FONT_LARGE, 720-font_large.width*3, 50, "DOF");
		else if (old_value) redraw();
		
		if (dofpreview != old_value) // transition
		{
			if (state == 0)
			{
				if (old_value && !dofpreview)
				{
					//~ beep();
					dofp_set(1);
					msleep(100);
					state = 1;
				}
			}
			else
			{
				if (dofpreview == 0) state = 0;
				//~ beep();
			}
		}
		old_value = dofpreview;
	}
}


// ExpSim
//**********************************************************************
CONFIG_INT( "expsim", expsim_setting, 2);

static void set_expsim( int x )
{
	if (expsim != x)
	{
		prop_request_change(PROP_LIVE_VIEW_VIEWTYPE, &x, 4);
	}
}
/*
static void
expsim_toggle( void * priv )
{
	// off, on, auto
	if (!expsim_auto && !expsim) // off->on
	{
		set_expsim(1);
	}
	else if (!expsim_auto && expsim) // on->auto
	{
		expsim_auto = 1;
	}
	else // auto->off
	{
		expsim_auto = 0;
		set_expsim(0);
	}
}*/
static void
expsim_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Exposure Simulation : %s%s",
		expsim_setting == 2 ? (expsim ? "Auto (ON)" : "Auto (OFF)") : 
		expsim ? "ON" : "OFF",
		expsim == get_expsim_auto_value() ? "" : " [!]"
	);
	menu_draw_icon(x, y, expsim != get_expsim_auto_value() ? MNI_WARNING : expsim_setting == 2 ? MNI_AUTO : MNI_BOOL(expsim), 0);
}

int get_expsim_auto_value()
{
	// silent pic in matrix mode requires expsim on
	extern int silent_pic_sweep_running;
	if (silent_pic_sweep_running) return 1;
	
	if (expsim_setting == 2)
	{
		if ((lv_dispsize > 1 || should_draw_zoom_overlay()) && !get_halfshutter_pressed()) return 0;
		else return 1;
	}
	else return expsim_setting;
}
static void expsim_update()
{
	if (!lv) return;
	if (is_movie_mode()) return;
	int expsim_auto_value = get_expsim_auto_value();
	if (expsim_auto_value != expsim)
	{
		if (MENU_MODE && !gui_menu_shown()) // ExpSim changed from Canon menu
		{ 
			expsim_setting = expsim;
			NotifyBox(2000, "ML: Auto ExpSim disabled.");
		}
		else
		{
			set_expsim(expsim_auto_value); // shooting mode, ML decides to toggle ExpSim
		}
	}
}

static void expsim_toggle(void* priv)
{
	menu_ternary_toggle(priv); msleep(100);
}

static void expsim_toggle_reverse(void* priv)
{
	menu_ternary_toggle_reverse(priv); msleep(100);
}

// LV metering
//**********************************************************************
/*
CONFIG_INT("lv.metering", lv_metering, 0);

static void
lv_metering_print( void * priv, int x, int y, int selected )
{
	unsigned z = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LV Metering Override: %s",
		lv_metering == 0 ? "OFF" :
		lv_metering == 1 ? "Spotmeter" :
		lv_metering == 2 ? "CenteredHist" :
		lv_metering == 3 ? "HighlightPri" :
		lv_metering == 4 ? "NoOverexpose" : "err"
	);
}

static void
lv_metering_adjust()
{
	if (!lv) return;
	if (get_halfshutter_pressed()) return;
	if (lv_dispsize != 1) return;
	if (shooting_mode != SHOOTMODE_P && shooting_mode != SHOOTMODE_AV && shooting_mode != SHOOTMODE_TV) return;
	
	if (lv_metering == 1)
	{
		uint8_t Y,U,V;
		get_spot_yuv(5, &Y, &U, &V);
		//bmp_printf(FONT_LARGE, 0, 100, "Y %d AE %d  ", Y, lens_info.ae);
		lens_set_ae(COERCE(lens_info.ae + (128 - Y) / 5, -40, 40));
	}
	else if (lv_metering == 2) // centered histogram
	{
		int under, over;
		get_under_and_over_exposure_autothr(&under, &over);
		if (over > under) lens_set_ae(lens_info.ae - 1);
		else lens_set_ae(lens_info.ae + 1);
	}
	else if (lv_metering == 3) // highlight priority
	{
		int under, over;
		get_under_and_over_exposure(5, 240, &under, &over);
		if (over > 100 && under < over * 5) lens_set_ae(lens_info.ae - 1);
		else lens_set_ae(lens_info.ae + 1);
	}
	else if (lv_metering == 4) // don't overexpose
	{
		int under, over;
		get_under_and_over_exposure(5, 240, &under, &over);
		if (over > 1000) lens_set_ae(lens_info.ae - 1);
		else lens_set_ae(lens_info.ae + 1);
	}
}*/

// auto burst pic quality
//**********************************************************************

CONFIG_INT("burst.auto.picquality", auto_burst_pic_quality, 0);

int burst_count = 0;

static void set_pic_quality(int q)
{
	if (q == -1) return;
	prop_request_change(PROP_PIC_QUALITY, &q, 4);
	prop_request_change(PROP_PIC_QUALITY2, &q, 4);
	prop_request_change(PROP_PIC_QUALITY3, &q, 4);
}

int picq_saved = -1;
static void decrease_pic_quality()
{
	if (picq_saved == -1) picq_saved = pic_quality; // save only first change
	
	int newpicq = 0;
 	switch(pic_quality)
 	{
		case PICQ_RAW_JPG_LARGE_FINE:
 			newpicq = PICQ_LARGE_FINE;
 			break;
 		case PICQ_RAW:
			newpicq = PICQ_LARGE_FINE;
 			break;
 		case PICQ_LARGE_FINE:
 			newpicq = PICQ_MED_FINE;
 			break;
		//~ case PICQ_MED_FINE:
			//~ newpicq = PICQ_SMALL_FINE;
			//~ break;
 		//~ case PICQ_SMALL_FINE:
 			//~ newpicq = PICQ_SMALL_COARSE;
 			//~ break;
 		case PICQ_LARGE_COARSE:
 			newpicq = PICQ_MED_COARSE;
 			break;
		//~ case PICQ_MED_COARSE:
			//~ newpicq = PICQ_SMALL_COARSE;
			//~ break;
 	}
 	if (newpicq) set_pic_quality(newpicq);
}
 
static void restore_pic_quality()
{
	if (picq_saved != -1) set_pic_quality(picq_saved);
	picq_saved = -1;
}

static void adjust_burst_pic_quality()
{
	if (lens_info.job_state == 0) { restore_pic_quality(); return; }
	if (burst_count < 4) decrease_pic_quality();
	else if (burst_count >= 5) restore_pic_quality();
}

PROP_HANDLER(PROP_BURST_COUNT)
{
	burst_count = buf[0];

	if (auto_burst_pic_quality && avail_shot > burst_count)
	{
		adjust_burst_pic_quality();
	}

	return prop_cleanup(token, property);
}

static void
auto_burst_pic_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Auto BurstPicQuality: %s", 
		auto_burst_pic_quality ? "ON" : "OFF"
	);
}

void lcd_sensor_shortcuts_print( void * priv, int x, int y, int selected);
extern int lcd_sensor_shortcuts;

// backlight adjust
//**********************************************************************

void adjust_backlight_level(int delta)
{
	if (backlight_level < 1 || backlight_level > 7) return; // kore wa dame desu yo
	if (tft_status) call("TurnOnDisplay");
	int level = COERCE(backlight_level + delta, 1, 7);
	prop_request_change(PROP_BACKLIGHT_LEVEL, &level, 4);
	NotifyBoxHide();
	NotifyBox(2000, "LCD Backlight: %d", level);
}
void set_backlight_level(int level)
{
	level = COERCE(level, 1, 7);
	prop_request_change(PROP_BACKLIGHT_LEVEL, &level, 4);
}

CONFIG_INT("af.frame.autohide", af_frame_autohide, 1);

static void
af_frame_autohide_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"AF frame display    : %s", 
		af_frame_autohide ? "AutoHide" : "Show"
	);
}

int afframe_countdown = 0;
void afframe_set_dirty()
{
	afframe_countdown = 20;
}

void clear_lv_afframe()
{
	if (!lv) return;
	if (gui_menu_shown()) return;
	if (lv_dispsize != 1) return;
	struct vram_info *	lv = get_yuv422_vram();
	if( !lv->vram )	return;
	int xaf,yaf;
	get_afframe_pos(lv->width, lv->height, &xaf, &yaf);
	//~ bmp_printf(FONT_LARGE, 200, 200, "af %d %d ", xaf, yaf);
	bmp_fill(0, COERCE(xaf,100, 860) - 100, COERCE(yaf,100, 440) - 100, 200, 200 );
	crop_set_dirty(5);
}

CONFIG_INT("play.quick.zoom", quickzoom, 1);

static void
quickzoom_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zoom in PLAY mode : %s", 
		quickzoom ? "Fast" : "Normal"
	);
}

#ifdef CONFIG_60D

CONFIG_INT("display.off.halfshutter", display_off_by_halfshutter_enabled, 0);

int display_turned_off_by_halfshutter = 0; // 1 = display was turned off, -1 = display should be turned back on (ML should take action)

PROP_INT(PROP_INFO_BUTTON_FUNCTION, info_button_function);

static void display_on_and_go_to_main_shooting_screen()
{
	if (lv) return;
	if (tft_status == 0) return; // display already on
	if (gui_state != GUISTATE_IDLE) return;
	
	display_turned_off_by_halfshutter = 0;
	
	int old = info_button_function;
	int x = 1;
	//~ card_led_blink(5,100,100);
	prop_request_change(PROP_INFO_BUTTON_FUNCTION, &x, 4); // temporarily make the INFO button display only the main shooting screen
	fake_simple_button(BGMT_DISP);
	msleep(300);
	prop_request_change(PROP_INFO_BUTTON_FUNCTION, &old, 4); // restore user setting back
}

int handle_disp_button_in_photo_mode() // called from handle_buttons
{
	if (display_off_by_halfshutter_enabled && display_turned_off_by_halfshutter == 1 && gui_state == GUISTATE_IDLE && !gui_menu_shown())
	{
		display_turned_off_by_halfshutter = -1; // request: ML should turn it on
		return 0;
	}
	return 1;
}

static void display_off_by_halfshutter()
{
	static int prev_gui_state = 0;
	if (prev_gui_state != GUISTATE_IDLE) 
	{ 
		msleep(100);
		prev_gui_state = gui_state;
		while (get_halfshutter_pressed()) msleep(100);
		return; 
	}
	prev_gui_state = gui_state;
		
	if (!lv && gui_state == GUISTATE_IDLE) // main shooting screen, photo mode
	{
		if (tft_status == 0) // display is on
		{
			if (get_halfshutter_pressed())
			{
				// wait for long half-shutter press (1 second)
				int i;
				for (i = 0; i < 10; i++)
				{
					msleep(100);
					if (!get_halfshutter_pressed()) return;
					if (tft_status) return;
				}
				fake_simple_button(BGMT_DISP); // turn display off
				while (get_halfshutter_pressed()) msleep(100);
				display_turned_off_by_halfshutter = 1; // next INFO press will go to main shooting screen
				return;
			}
		}
		else // display is off
		{
			if (display_turned_off_by_halfshutter == -1)
			{
				display_on_and_go_to_main_shooting_screen();
				display_turned_off_by_halfshutter = 0;
			}
		}
	}
}

static void
display_off_by_halfshutter_print(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"DispOFF in PhotoMode: %s", // better name for this?
		display_off_by_halfshutter_enabled ? "HalfShutter" : "OFF"
	);
}

#endif

CONFIG_INT("crop.playback", cropmarks_play, 0);

static void
cropmarks_play_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Cropmarks (PLAY)  : %s", 
		cropmarks_play ? "ON" : "OFF"
	);
}

CONFIG_INT("play.set.wheel", play_set_wheel_action, 2);

static void
play_set_wheel_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"SET+MainDial(PLAY): %s", 
		play_set_wheel_action == 0 ? "422 Preview" :
		play_set_wheel_action == 1 ? "ExposureFusion" : 
		play_set_wheel_action == 2 ? "CompareImages" : 
		play_set_wheel_action == 3 ? "TimelapsePlay" : "err"
	);
	menu_draw_icon(x, y, MNI_ON, 0);
}

int timelapse_playback = 0;

void playback_set_wheel_action(int dir)
{
	if (play_set_wheel_action == 0) play_next_422(dir);
	else if (play_set_wheel_action == 1) expfuse_preview_update(dir);
	else if (play_set_wheel_action == 2) playback_compare_images(dir);
	else if (play_set_wheel_action == 3) timelapse_playback += dir;
}

int handle_set_wheel_play(struct event * event)
{
	extern int set_pressed;
	// SET button pressed
	//~ if (event->param == BGMT_PRESS_SET) set_pressed = 1;
	//~ if (event->param == BGMT_UNPRESS_SET) set_pressed = 0;
	//~ if (event->param == BGMT_PLAY) set_pressed = 0;

	// reset exposure fusion preview
	extern int expfuse_running;
	if (set_pressed == 0)
	{
		expfuse_running = 0;
	}

	// SET+Wheel action in PLAY mode
	if ( PLAY_MODE && get_set_pressed())
	{
		if (!IS_FAKE(event) && (event->param == BGMT_WHEEL_LEFT || event->param == BGMT_WHEEL_RIGHT))
		{
			int dir = event->param == BGMT_WHEEL_RIGHT ? 1 : -1;
			playback_set_wheel_action(dir);
			return 0;
		}
	}
	
	return 1;
}

static void
tweak_task( void* unused)
{
	do_movie_mode_remap();
	
	int k;
	for (k = 0; ; k++)
	{
		msleep(50);
		
		/*if (lv_metering && !is_movie_mode() && lv && k % 10 == 0)
		{
			lv_metering_adjust();
		}*/
		
		// timelapse playback
		if (timelapse_playback)
		{
			if (!PLAY_MODE) { timelapse_playback = 0; continue; }
			
			//~ NotifyBox(1000, "Timelapse...");
			fake_simple_button(timelapse_playback > 0 ? BGMT_WHEEL_DOWN : BGMT_WHEEL_UP);
			continue;
		}
		
		// faster zoom in play mode
		if (quickzoom && gui_state == GUISTATE_PLAYMENU)
		{
			if (get_zoom_in_pressed()) 
			{
				msleep(300);
				while (get_zoom_in_pressed()) {	fake_simple_button(BGMT_PRESS_ZOOMIN_MAYBE); msleep(50); }
			}
			
			if (get_zoom_out_pressed())
			{
				msleep(300);
				while (get_zoom_out_pressed()) {	fake_simple_button(BGMT_PRESS_ZOOMOUT_MAYBE); msleep(50); }
			}
		}
		
		expsim_update();
		
		dofp_update();

		#ifndef CONFIG_50D
		if (af_frame_autohide && afframe_countdown && liveview_display_idle())
		{
			afframe_countdown--;
			if (!afframe_countdown) 
				BMP_LOCK( clear_lv_afframe(); )
		}
		#endif
		
		extern int disp_profiles_0;
		if (FLASH_BTN_MOVIE_MODE)
		{
			int k = 0;
			int longpress = 0;
			if (disp_profiles_0) bmp_printf(FONT_MED, 245, 100, "DISP preset toggle");
			while (FLASH_BTN_MOVIE_MODE)
			{
				msleep(100);
				k++;
				
				if (k > 3)
				{
					longpress = 1; // long press doesn't toggle
					if (disp_profiles_0) bmp_printf(FONT(FONT_MED, 1, 0), 245, 100, "                  ");
				}
					
			}
			if (disp_profiles_0 && !longpress) toggle_disp_mode();
			redraw();
		}
		
		#ifdef CONFIG_60D
		if (display_off_by_halfshutter_enabled)
			display_off_by_halfshutter();
		#endif
		
		if (LV_BOTTOM_BAR_DISPLAYED || ISO_ADJUSTMENT_ACTIVE)
			idle_wakeup_reset_counters();
	}
}

TASK_CREATE("tweak_task", tweak_task, 0, 0x1e, 0x1000 );

extern int quick_review_allow_zoom;

static void
qrplay_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"After taking a pic: %s", 
		quick_review_allow_zoom ? "Hold->Play" : "QuickReview"
	);
}
/*
extern int set_on_halfshutter;

static void
set_on_halfshutter_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"HalfShutter in DLGs : %s", 
		set_on_halfshutter ? "SET" : "Cancel"
	);
}*/

extern int iso_round_only;
static void
iso_round_only_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ISO selection       : %s", 
		iso_round_only ? "100x, 160x" : "All values"
	);
}


CONFIG_INT("swap.menu", swap_menu, 0);
static void
swap_menu_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Swap MENU <-> ERASE : %s", 
		swap_menu ? "ON" : "OFF"
	);
}

int handle_swap_menu_erase(struct event * event)
{
	if (swap_menu && !IS_FAKE(event))
	{
		if (event->param == BGMT_TRASH)
		{
			fake_simple_button(BGMT_MENU);
			return 0;
		}
		if (event->param == BGMT_MENU)
		{
			fake_simple_button(BGMT_TRASH);
			return 0;
		}
	}
	return 1;
}

extern int cropmark_movieonly;

static void
crop_movieonly_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Show cropmarks in   : %s", 
		cropmark_movieonly ? "Movie mode" : "Movie&Photo"
	);
}

/*extern int picstyle_disppreset_enabled;
static void
picstyle_disppreset_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"PicSty->DISP preset : %s",
		picstyle_disppreset_enabled ? "ON" : "OFF"
	);
}*/

extern int display_dont_mirror;
static void
display_dont_mirror_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Mirrored Display    : %s", 
		display_dont_mirror ? "Don't allow": "Allow"
	);
	menu_draw_icon(x, y, MNI_BOOL(!display_dont_mirror), 0);
}

/*
int night_vision = 0;
void night_vision_toggle(void* priv)
{
	night_vision = !night_vision;
	call("lvae_setdispgain", night_vision ? 65535 : 0);
	menu_show_only_selected();
}

static void night_vision_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Night Vision Mode   : %s", 
		night_vision ? "ON" : "OFF"
	);
	if (night_vision && (!lv || is_movie_mode()))
		menu_draw_icon(x, y, MNI_WARNING, 0);
}
*/

void set_display_gain(int display_gain)
{
	call("lvae_setdispgain", COERCE(display_gain, 0, 65535));
}
int display_gain = 0;
void display_gain_toggle(int dir)
{
	if (dir > 0)
	{
		display_gain = MAX(display_gain * 2, 2048);
		if (display_gain > 65536) display_gain = 0;
	}
	else if (dir < 0)
	{
		if (display_gain && display_gain <= 2048) display_gain = 0;
		else if (display_gain) display_gain /= 2; 
		else display_gain = 65536;
	}
	else display_gain = 0;
	set_display_gain(display_gain);
	menu_show_only_selected();
}
void display_gain_toggle_forward(void* priv) { display_gain_toggle(1); }
void display_gain_toggle_reverse(void* priv) { display_gain_toggle(-1); }
void display_gain_reset(void* priv) { display_gain_toggle(0); }

int gain_to_ev(int gain)
{
	return (int) roundf(log2f(gain));
}

static void display_gain_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	int gain_ev = 0;
	if (display_gain) gain_ev = gain_to_ev(display_gain) - 10;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LVGain (NightVision): %s%d EV",
		gain_ev ? "+" : "",
		gain_ev
	);
	if (display_gain)
	{
		if (lv) menu_draw_icon(x, y, MNI_PERCENT, gain_ev * 100 / 6);
		else menu_draw_icon(x, y, MNI_WARNING, 0);
	}
}

/*
PROP_INT(PROP_ELECTRIC_SHUTTER, eshutter);

static void eshutter_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	int gain_ev = 0;
	if (display_gain) gain_ev = gain_to_ev(display_gain) - 10;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Electric Shutter    : %d",
		eshutter
	);
}
static void set_eshutter(int e)
{
	e = COERCE(e, 0, 2);
	call("lv_eshutter", e);
	eshutter = e;
	GUI_SetElectricShutter(e);
	//~ prop_request_change(PROP_ELECTRIC_SHUTTER, &e, 4);
}
static void eshutter_toggle(void* priv)
{
	set_eshutter(mod(eshutter + 1, 3));
}*/

struct menu_entry tweak_menus[] = {
/*	{
		.name = "Night Vision Mode",
		.priv = &night_vision, 
		.select = night_vision_toggle, 
		.display = night_vision_print,
		.help = "Maximize LV display gain for framing in darkness (photo)"
	},*/
	{
		.name = "LVGain (NightVision)", 
		.priv = &display_gain,
		.select = display_gain_toggle_forward, 
		.select_reverse = display_gain_toggle_reverse,
		.select_auto = display_gain_reset,
		.display = display_gain_print, 
		.help = "Boosts LV digital display gain (Photo, Movie w.AutoISO)",
	},
	{
		.name = "Exposure Simulation",
		.priv = &expsim_setting,
		.select = expsim_toggle,
		.select_reverse = expsim_toggle_reverse,
		.display = expsim_display,
		.help = "ExpSim: LCD image reflects exposure settings (ISO+Tv+Av)."
	},
	{
		.name = "DOF Preview", 
		.priv = &dofpreview_sticky, 
		.select = menu_binary_toggle, 
		.select_auto = dofp_lock,
		.display = dofp_display,
		.help = "Sticky = click DOF to toggle. Or, press [Q] to lock now."
	},
	/*{
		.name = "Electric Shutter",
		.priv = &eshutter,
		.select = eshutter_toggle,
		.display = eshutter_display,
		.help = "For enabling third-party flashes in LiveView."
	},*/
	#ifndef CONFIG_50D
	{
		.name = "AF frame display",
		.priv = &af_frame_autohide, 
		.select = menu_binary_toggle,
		.display = af_frame_autohide_display,
		.help = "You can hide the AF frame (the little white rectangle)."
	},
	#endif
	#if defined(CONFIG_550D) || defined(CONFIG_500D)
	{
		.name = "LCD Sensor Shortcuts",
		.priv		= &lcd_sensor_shortcuts,
		.select		= menu_binary_toggle,
		.display	= lcd_sensor_shortcuts_print,
	},
	#endif
	#if !defined(CONFIG_60D) && !defined(CONFIG_50D) // 60D doesn't need this
	{
		.name = "Auto BurstPicQuality",
		.priv = &auto_burst_pic_quality, 
		.select = menu_binary_toggle, 
		.display = auto_burst_pic_display,
	},
	#endif
	#if 0
	{
		.name = "HalfShutter in DLGs",
		.priv = &set_on_halfshutter, 
		.select = menu_binary_toggle, 
		.display = set_on_halfshutter_display,
		.help = "Half-shutter press in dialog boxes => OK (SET) or Cancel."
	},
	#endif
	{
		.name = "Show cropmarks in",
		.priv = &cropmark_movieonly,
		.display	= crop_movieonly_display,
		.select		= menu_binary_toggle,
		.help = "Cromparks can be in Movie mode only, or in Photo modes too."
	},
	{
		.name = "ISO selection",
		.priv = &iso_round_only,
		.display	= iso_round_only_display,
		.select		= menu_binary_toggle,
		.help = "You can enable only ISOs which are multiple of 100 and 160."
	},
	/*{
		.name = "PicSty->DISP preset",
		.priv = &picstyle_disppreset_enabled,
		.display	= picstyle_disppreset_display,
		.select		= menu_binary_toggle,
		.help = "PicStyle can be included in DISP preset for easy toggle."
	},*/
	#ifdef CONFIG_60D
	{
		.name = "Swap MENU <--> ERASE",
		.priv = &swap_menu,
		.display	= swap_menu_display,
		.select		= menu_binary_toggle,
		.help = "Swaps MENU and ERASE buttons."
	},
	{
		.name = "DispOFF in PhotoMode",
		.priv = &display_off_by_halfshutter_enabled,
		.display = display_off_by_halfshutter_print, 
		.select = menu_binary_toggle,
		.help = "Outside LV, turn off display with long half-shutter press."
	},
	#endif
	#if defined(CONFIG_60D) || defined(CONFIG_600D)
	{
		.name = "Mirrored Display",
		.priv = &display_dont_mirror,
		.display = display_dont_mirror_display, 
		.select = menu_binary_toggle,
		.help = "Prevents display mirroring, which may reverse ML texts."
	},
	#endif
/*	{
		.priv = &lv_metering,
		.select = menu_quinternary_toggle, 
		.select_reverse = menu_quinternary_toggle_reverse, 
		.display = lv_metering_print,
		.help = "Custom metering methods in LiveView; too slow for real use."
	},*/
};


struct menu_entry play_menus[] = {
	{
		.name = "SET+MainDial (PLAY)",
		.priv = &play_set_wheel_action, 
		.select = menu_quaternary_toggle, 
		.select_reverse = menu_quaternary_toggle_reverse,
		.display = play_set_wheel_display,
		.help = "What to do when you hold SET and turn MainDial (Wheel)"
	},
	{
		.name = "Cropmarks (PLAY)",
		.priv = &cropmarks_play, 
		.select = menu_binary_toggle, 
		.display = cropmarks_play_display,
		.help = "Show cropmarks in PLAY mode"
	},
	{
		.name = "After taking a photo",
		.priv = &quick_review_allow_zoom, 
		.select = menu_binary_toggle, 
		.display = qrplay_display,
		.help = "When you set \"ImageReview: Hold\", it will go to Play mode."
	},
	{
		.name = "Zoom in PLAY mode",
		.priv = &quickzoom, 
		.select = menu_binary_toggle, 
		.display = quickzoom_display,
		.help = "Faster zoom in Play mode, for pixel peeping :)"
	},
};

static void tweak_init()
{
	menu_add( "Tweak", tweak_menus, COUNT(tweak_menus) );
	menu_add( "Play", play_menus, COUNT(play_menus) );
}

INIT_FUNC(__FILE__, tweak_init);
