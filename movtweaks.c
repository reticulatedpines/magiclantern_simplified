/** \file
 * Tweaks specific to movie mode
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"

CONFIG_INT("hdmi.force.vga", hdmi_force_vga, 0);
PROP_INT(PROP_HDMI_CHANGE_CODE, hdmi_code)

// WB workaround (not saved in movie mode)
//**********************************************************************
CONFIG_INT( "white.balance.workaround", white_balance_workaround, 1);
CONFIG_INT( "wb.kelvin", workaround_wb_kelvin, 6500);
CONFIG_INT( "wbs.gm", workaround_wbs_gm, 100);
CONFIG_INT( "wbs.ba", workaround_wbs_ba, 100);

void save_kelvin_wb()
{
	workaround_wb_kelvin = lens_info.kelvin;
	workaround_wbs_gm = lens_info.wbs_gm + 100;
	workaround_wbs_ba = lens_info.wbs_ba + 100;
}

void restore_kelvin_wb()
{
	if (!white_balance_workaround) return;
	
	// sometimes Kelvin WB and WBShift are not remembered, usually in Movie mode 
	lens_set_kelvin_value_only(workaround_wb_kelvin);
	lens_set_wbs_gm(COERCE(((int)workaround_wbs_gm) - 100, -9, 9));
	lens_set_wbs_ba(COERCE(((int)workaround_wbs_ba) - 100, -9, 9));
}

int mode_remap_done = 0;
PROP_HANDLER(PROP_SHOOTING_MODE)
{
	static int shooting_mode = -1;
	if (shooting_mode != (int)buf[0]) mode_remap_done = 0;
	shooting_mode = buf[0];
	restore_kelvin_wb();
	intervalometer_stop();
	return prop_cleanup(token, property);
}

CONFIG_INT("movie.restart", movie_restart,0);
CONFIG_INT("movie.mode-remap", movie_mode_remap, 0);
CONFIG_INT("movie.rec-key", movie_rec_key, 0);

static void
movie_rec_key_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Movie REC key : %s ",
		movie_rec_key ? "HalfShutter" : "Default"
	);
}

PROP_HANDLER(PROP_HALF_SHUTTER)
{
	if (movie_rec_key && buf[0] && shooting_mode == SHOOTMODE_MOVIE && gui_state == GUISTATE_IDLE && !gui_menu_shown())
	{
		if (!recording) schedule_movie_start();
		else schedule_movie_end();
	}
	return prop_cleanup(token, property);
}


static void
movie_restart_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Movie Restart : %s ",
		movie_restart ? "ON " : "OFF"
	);
}

int setting_shooting_mode = 0;
void set_shooting_mode(int m)
{
	setting_shooting_mode = 1;
	msleep(100);
	prop_request_change(PROP_SHOOTING_MODE, &m, 4);
	msleep(500);
	mode_remap_done = 1;
	setting_shooting_mode = 0;
}

void do_movie_mode_remap()
{
	if (!movie_mode_remap) return;
	if (mode_remap_done) return;
	if (setting_shooting_mode) return;
	int movie_newmode = movie_mode_remap == 1 ? SHOOTMODE_ADEP : SHOOTMODE_CA;
	if (shooting_mode == movie_newmode) set_shooting_mode(SHOOTMODE_MOVIE);
	else if (shooting_mode == SHOOTMODE_MOVIE) set_shooting_mode(movie_newmode);
	mode_remap_done = 1;
}

CONFIG_INT("dof.adjust", dof_adjust, 1);
int get_dof_adjust() { return dof_adjust; }

static void
dof_adjust_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"DOF adjust    : %s",
		dof_adjust ? "ON (sensor+Av)" : "OFF"
	);
}

static void apershutter_close(void* priv)
{
	lens_set_rawaperture(lens_info.raw_aperture + 4);
	lens_set_rawshutter(lens_info.raw_shutter - 4);
}
static void apershutter_open(void* priv)
{
	lens_set_rawaperture(lens_info.raw_aperture - 4);
	lens_set_rawshutter(lens_info.raw_shutter + 4);
}

int aperiso_rawap = 0;
int aperiso_rawiso = 0;
static void aperiso_init()
{
	aperiso_rawap = lens_info.raw_aperture;
	aperiso_rawiso = lens_info.raw_iso;
}
static void aperiso_close(void* priv)
{
	aperiso_rawap += 4;
	aperiso_rawiso += 4;
	lens_set_rawaperture(aperiso_rawap);
	lens_set_rawiso(aperiso_rawiso);
}
static void aperiso_open(void* priv)
{
	aperiso_rawap -= 4;
	aperiso_rawiso -= 4;
	lens_set_rawaperture(aperiso_rawap);
	lens_set_rawiso(aperiso_rawiso);
}

static void
mode_remap_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"MovieModeRemap: %s",
		movie_mode_remap == 1 ? "A-DEP" : movie_mode_remap == 2 ? "CA" : "OFF"
	);
}

// start with LV
//**********************************************************************

CONFIG_INT( "enable-liveview",	enable_liveview, 1 );
static void
enable_liveview_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Force LiveView: %s",
		enable_liveview == 1 ? "Start & CPU lenses" : enable_liveview == 2 ? "Always" : "OFF"
	);
	menu_draw_icon(x, y, enable_liveview == 1 ? MNI_AUTO : enable_liveview == 2 ? MNI_ON : MNI_OFF, 0);
}

void force_liveview()
{
	if (shooting_mode == SHOOTMODE_MOVIE)
	{
		set_shooting_mode(SHOOTMODE_NIGHT); // you can run, but you cannot hide :)
		call( "FA_StartLiveView" );
		set_shooting_mode(SHOOTMODE_MOVIE);
	}
	else
	{
		call( "FA_StartLiveView" );
	}
	while (gui_state != GUISTATE_IDLE) msleep(1000);
}

static void
movtweak_task( void* unused )
{
	if (!lv_drawn() && enable_liveview && shooting_mode == SHOOTMODE_MOVIE
		&& (CURRENT_DIALOG_MAYBE == DLG_MOVIE_PRESS_LV_TO_RESUME || 
			CURRENT_DIALOG_MAYBE == DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
	{
		force_liveview();
	}

	int k;
	for (k = 0; ; k++)
	{
		msleep(50);
		
		if (movie_restart)
		{
			static int recording_prev = 0;
			if (recording == 0 && recording_prev && wait_for_lv_err_msg(0))
			{
				msleep(1000);
				movie_start();
			}
			recording_prev = recording;
		}
		
		do_movie_mode_remap();
		
		save_kelvin_wb();

		if ((enable_liveview && CURRENT_DIALOG_MAYBE == DLG_MOVIE_PRESS_LV_TO_RESUME) ||
			(enable_liveview == 2 && CURRENT_DIALOG_MAYBE == DLG_MOVIE_ENSURE_A_LENS_IS_ATTACHED))
		{
			force_liveview();
		}
		
		extern int ext_monitor_hdmi;
		if (hdmi_force_vga && shooting_mode == SHOOTMODE_MOVIE && lv_drawn() && ext_monitor_hdmi && !recording && !gui_menu_shown())
		{
			if (hdmi_code == 5)
			{
				msleep(500);
				ChangeHDMIOutputSizeToVGA();
				msleep(500);
				redraw();
			}
		}
	}
}

TASK_CREATE("movtweak_task", movtweak_task, 0, 0x1f, 0x1000 );

static void
wb_workaround_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"WB workaround : %s", 
		white_balance_workaround ? "ON(save WB in cfg)" : "OFF"
	);
}


extern int zebra_nrec;

static void
zebra_nrec_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Zebra when REC: %s", 
		zebra_nrec ? "Hide" : "Show"
	);
	menu_draw_icon(x, y, MNI_BOOL(!zebra_nrec), 0);
}

static void
hdmi_force_display(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Force HDMI-VGA: %s", 
		hdmi_force_vga ? "ON" : "OFF"
	);
}
static struct menu_entry mov_menus[] = {
	/*{
		.priv = &bitrate_mode,
		.display	= bitrate_print,
		.select		= menu_ternary_toggle,
		.select_auto	= bitrate_toggle_forward,
		.select_reverse	= bitrate_toggle_reverse,
	},*/
	/*{
		.display	= vbr_print,
		.select		= vbr_toggle,
	},*/
	{
		.priv = &movie_restart,
		.display	= movie_restart_print,
		.select		= menu_binary_toggle,
		.help = "Auto-restart movie recording, if it happens to stop."
	},
	/*{
		.priv = &movie_af,
		.display	= movie_af_print,
		.select		= menu_quaternary_toggle,
		.select_reverse = movie_af_noisefilter_bump,
		.select_auto = movie_af_aggressiveness_bump,
	},*/
	{
		.priv = &movie_mode_remap,
		.display	= mode_remap_print,
		.select		= menu_ternary_toggle,
		.help = "Remap movie mode to A-DEP or CA."
	},
	/*{
		.priv = &as_swap_enable, 
		.display = as_swap_print,
		.select = menu_binary_toggle,
	},
	{
		.priv = &dof_adjust, 
		.display = dof_adjust_print, 
		.select = menu_binary_toggle,
		.help = "Cover LCD sensor and adjust aperture => ISO changes too."
	},*/
	{
		.priv = &movie_rec_key, 
		.display = movie_rec_key_print, 
		.select = menu_binary_toggle,
		.help = "Change the button used for recording. Hint: wired remote."
	},
	{
		.priv = &white_balance_workaround,
		.display = wb_workaround_display, 
		.select = menu_binary_toggle,
		.help = "Without this, camera forgets some WB params in Movie mode."
	},
	{
		.priv = &zebra_nrec,
		.select = menu_binary_toggle,
		.display = zebra_nrec_display,
		.help = "You can disable zebra during recording."
	},
	{
		.priv = &enable_liveview,
		.display	= enable_liveview_print,
		.select		= menu_ternary_toggle,
		.help = "Force LiveView in movie mode, even with an unchipped lens."
	},
	{
		.priv = &hdmi_force_vga, 
		.display = hdmi_force_display, 
		.select = menu_binary_toggle,
		.help = "Force low resolution (3:2) on HDMI displays."
	}
};

void movtweak_init()
{
	menu_add( "Movie", mov_menus, COUNT(mov_menus) );
}

INIT_FUNC(__FILE__, movtweak_init);
