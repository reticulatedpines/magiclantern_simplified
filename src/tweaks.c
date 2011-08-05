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

// ExpSim
//**********************************************************************
CONFIG_INT( "expsim.auto", expsim_auto, 1);

void set_expsim( int x )
{
	if (expsim != x)
		prop_request_change(PROP_LIVE_VIEW_VIEWTYPE, &x, 4);
}
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
}
static void
expsim_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Exposure Simulation : %s",
		expsim_auto ? (expsim ? "Auto (ON)" : "Auto (OFF)") : 
		expsim ? "ON " : "OFF"
	);
	menu_draw_icon(x, y, shooting_mode == SHOOTMODE_MOVIE ? MNI_WARNING : expsim_auto ? MNI_AUTO : MNI_BOOL(expsim), 0);
}

void expsim_update()
{
	if (!lv) return;
	if (shooting_mode == SHOOTMODE_MOVIE) return;
	if (expsim_auto)
	{
		if ((lv_dispsize > 1 || should_draw_zoom_overlay()) && !get_halfshutter_pressed()) set_expsim(0);
		else set_expsim(1);
	}
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

void set_pic_quality(int q)
{
	if (q == -1) return;
	prop_request_change(PROP_PIC_QUALITY, &q, 4);
	prop_request_change(PROP_PIC_QUALITY2, &q, 4);
	prop_request_change(PROP_PIC_QUALITY3, &q, 4);
}

int picq_saved = -1;
void decrease_pic_quality()
{
	if (picq_saved == -1) picq_saved = pic_quality; // save only first change
	
	int newpicq = 0;
 	switch(pic_quality)
 	{
		case PICQ_RAW_JPG_LARGE_FINE:
 			newpicq = PICQ_RAW;
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
 
void restore_pic_quality()
{
	if (picq_saved != -1) set_pic_quality(picq_saved);
	picq_saved = -1;
}

void adjust_burst_pic_quality()
{
	if (burst_count < 3) decrease_pic_quality();
	else if (burst_count >= 3) restore_pic_quality();
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
	call("TurnOnDisplay");
	int level = COERCE(backlight_level + delta, 1, 7);
	prop_request_change(PROP_BACKLIGHT_LEVEL, &level, 4);
	if (!lv) bmp_printf(FONT_LARGE, 200, 240, "Backlight: %d", level);
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
	#ifdef CONFIG_600D
	bmp_fill(0, MAX(xaf,100) - 100, MAX(yaf,100) - 100, 200, 200 );
	#else
	bmp_fill(0, MAX(xaf,100) - 100, MAX(yaf,50) - 50, 200, 100 );
	#endif
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
		"Zoom in PLAY mode   : %s", 
		quickzoom ? "Fast" : "Normal"
	);
}

#ifdef CONFIG_60D

CONFIG_INT("display.off.halfshutter", display_off_by_halfshutter_enabled, 0);

int display_turned_off_by_halfshutter = 0; // 1 = display was turned off, -1 = display should be turned back on (ML should take action)

PROP_INT(PROP_INFO_BUTTON_FUNCTION, info_button_function);

void display_on_and_go_to_main_shooting_screen()
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

void display_off_by_halfshutter()
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

static void
tweak_task( void* unused)
{
	do_movie_mode_remap();
	
	int k;
	for (k = 0; ; k++)
	{
		msleep(50);
		
		/*if (lv_metering && shooting_mode != SHOOTMODE_MOVIE && lv && k % 10 == 0)
		{
			lv_metering_adjust();
		}*/
		
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

		if (af_frame_autohide && afframe_countdown && liveview_display_idle())
		{
			afframe_countdown--;
			if (!afframe_countdown) 
				BMP_SEM( clear_lv_afframe(); )
		}
		
		if (FLASH_BTN_MOVIE_MODE)
		{
			int k = 0;
			int longpress = 0;
			bmp_printf(FONT_MED, 245, 100, "DISP preset toggle");
			while (FLASH_BTN_MOVIE_MODE)
			{
				msleep(100);
				k++;
				
				if (k > 3)
				{
					longpress = 1; // long press doesn't toggle
					bmp_printf(FONT(FONT_MED, 1, 0), 245, 100, "                  ");
				}
					
			}
			if (!longpress) toggle_disp_mode();
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
		"After taking a photo: %s", 
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


extern int swap_menu;
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
		cropmark_movieonly ? "Movie mode" : "All modes"
	);
}

extern int picstyle_disppreset_enabled;
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
}


struct menu_entry tweak_menus[] = {
	{
		.name = "Exposure Simulation",
		.select = expsim_toggle, 
		.display = expsim_display,
		.help = "ExpSim: LCD image reflects exposure settings."
	},
	{
		.name = "AF frame display",
		.priv = &af_frame_autohide, 
		.select = menu_binary_toggle,
		.display = af_frame_autohide_display,
		.help = "You can hide the AF frame (the little white rectangle)."
	},
	#if defined(CONFIG_550D) || defined(CONFIG_500D)
	{
		.name = "LCD Sensor Shortcuts",
		.priv		= &lcd_sensor_shortcuts,
		.select		= menu_binary_toggle,
		.display	= lcd_sensor_shortcuts_print,
	},
	{
		.name = "Auto BurstPicQuality",
		.priv = &auto_burst_pic_quality, 
		.select = menu_binary_toggle, 
		.display = auto_burst_pic_display,
	},
	#endif
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
	{
		.name = "PicSty->DISP preset",
		.priv = &picstyle_disppreset_enabled,
		.display	= picstyle_disppreset_display,
		.select		= menu_binary_toggle,
		.help = "PicStyle can be included in DISP preset for easy toggle."
	},
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
	}
	#endif
/*	{
		.priv = &lv_metering,
		.select = menu_quinternary_toggle, 
		.select_reverse = menu_quinternary_toggle_reverse, 
		.display = lv_metering_print,
		.help = "Custom metering methods in LiveView; too slow for real use."
	},*/
};

void tweak_init()
{
	menu_add( "Tweak", tweak_menus, COUNT(tweak_menus) );
}

INIT_FUNC(__FILE__, tweak_init);
