/** \file
 * Magic Lantern debugging and reverse engineering code
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
//#include "lua.h"

int config_autosave = 1;
#define CONFIG_AUTOSAVE_FLAG_FILE "B:/AUTOSAVE.NEG"

CONFIG_INT( "white.balance.workaround", white_balance_workaround, 1);
CONFIG_INT( "wb.kelvin", workaround_wb_kelvin, 6500);
CONFIG_INT( "wbs.gm", workaround_wbs_gm, 100);
CONFIG_INT( "wbs.ba", workaround_wbs_ba, 100);

CONFIG_INT( "expsim.auto", expsim_auto, 1);

PROP_INT( PROP_LIVE_VIEW_VIEWTYPE, expsim )
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
		"Exposure Sim.  : %s",
		expsim_auto ? (expsim ? "Auto (ON)" : "Auto (OFF)") : 
		expsim ? "ON " : "OFF"
	);
}

PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
int shooting_mode;

void expsim_update()
{
	if (!lv_drawn()) return;
	if (shooting_mode == SHOOTMODE_MOVIE) return;
	if (expsim_auto)
	{
		if (lv_dispsize > 1 || should_draw_zoom_overlay()) set_expsim(0);
		else set_expsim(1);
	}
}

//////////////////////////////////////////////////////////
// debug manager enable/disable
//////////////////////////////////////////////////////////

CONFIG_INT("dm.enable", dm_enable, 0);

void dm_update()
{
	if (dm_enable) dmstart();
	else dmstop();
}

static void
dm_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	int mode = *(int*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Debug logging  : %s",
		dm_enable ? "ON, Q=dump" : "OFF,Q=dump"
	);
}

static void dm_toggle(void* priv)
{
	dm_enable = !dm_enable;
	dm_update();
}
//////////////////////////////////////////////////////////

static PROP_INT(PROP_EFIC_TEMP, efic_temp );
static PROP_INT(PROP_GUI_STATE, gui_state);
static PROP_INT(PROP_MAX_AUTO_ISO, max_auto_iso);
static PROP_INT(PROP_PIC_QUALITY, pic_quality);

extern void bootdisk_disable();

int display_force_off = 0;

CONFIG_INT("lv.metering", lv_metering, 0);

static void
lv_metering_print( void * priv, int x, int y, int selected )
{
	unsigned z = *(unsigned*) priv;
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"LV metering    : %s",
		lv_metering == 0 ? "Default" :
		lv_metering == 1 ? "Spotmeter" :
		lv_metering == 2 ? "CenteredHist" :
		lv_metering == 3 ? "HighlightPri" :
		lv_metering == 4 ? "NoOverexpose" : "err"
	);
}

static void
lv_metering_adjust()
{
	if (!lv_drawn()) return;
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
}


CONFIG_INT("burst.auto.picquality", auto_burst_pic_quality, 0);
int burst_count = 0; // PROP_BURST_COUNT = how many more pics can be taken in burst mode

void set_pic_quality(int q)
{
	switch(q)
	{
		case PICQ_RAW:
		case PICQ_RAW_JPG:
		case PICQ_LARGE_FINE:
		case PICQ_LARGE_COARSE:
		case PICQ_MED_FINE:
		case PICQ_MED_COARSE:
		case PICQ_SMALL_FINE:
		case PICQ_SMALL_COARSE:
			bmp_printf(FONT_LARGE, 0, 0, "SET_PIC_Q OK: %x", q);
			prop_request_change(PROP_PIC_QUALITY, &q, 4);
			prop_request_change(PROP_PIC_QUALITY2, &q, 4);
			prop_request_change(PROP_PIC_QUALITY3, &q, 4);
			break;
		default:
			bmp_printf(FONT_LARGE, 0, 0, "SET_PIC_Q invalid: %x", q);
	}
}

int picq_saved = -1;
void decrease_pic_quality()
{
	if (picq_saved == -1) picq_saved = pic_quality; // save only first change
	
	int newpicq = 0;
	switch(pic_quality)
	{
		case PICQ_RAW_JPG:
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

PROP_INT(PROP_AVAIL_SHOT, avail_shot);

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
		"Auto Burst PicQ: %s", 
		auto_burst_pic_quality ? "ON" : "OFF"
	);
}


CONFIG_INT("af.frame.autohide", af_frame_autohide, 1);

static void
af_frame_autohide_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"AF frame       : %s", 
		af_frame_autohide ? "AutoHide" : "Default"
	);
}

int afframe_countdown = 0;

PROP_HANDLER(PROP_LV_AFFRAME)
{
	afframe_countdown = 50;
	return prop_cleanup(token, property);
}

void clear_lv_afframe()
{
	if (!lv_drawn()) return;
	if (gui_menu_shown()) return;
	if (lv_dispsize != 1) return;
	struct vram_info *	lv = get_yuv422_vram();
	if( !lv->vram )	return;
	int xaf,yaf;
	get_afframe_pos(lv->width, lv->height, &xaf, &yaf);
	bmp_fill(0, MAX(xaf,100) - 100, MAX(yaf,50) - 50, 200, 100 );
	crop_set_dirty(1);
}

static void
display_off_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"ForceDisplayOFF: %s", 
		display_force_off ? "Enabled" : "Disabled"
	);
}

int focus_value = 0; // heuristic from 0 to 100
int focus_value_delta = 0;

/*
int big_clock = 0;

static void
big_clock_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Big Clock      : %s", 
		big_clock == 0 ? "OFF" : "ON"
		//~ big_clock == 1 ? "Clock" :
		//~ big_clock == 2 ? "Timer (1h)" :
		//~ big_clock == 3 ? "Timer (1m)" : "err"
	);
}

void show_big_clock()
{
	if (gui_state != 0) return;
	if (gui_menu_shown()) return;
	struct tm now;
	LoadCalendarFromRTC( &now );
	if (big_clock == 1)
	{
		static int tm_sec_prev = 0;
		if (now.tm_sec == tm_sec_prev) return;
		tm_sec_prev = now.tm_sec;

		int ang_hour = (now.tm_hour % 12) * 360 / 12 - 90;
		int ang_min = now.tm_min * 360 / 60 - 90;
		int ang_sec = now.tm_sec * 360 / 60 - 90;
		
		clrscr();
		draw_circle(360, 240, 240, COLOR_WHITE);
		draw_circle(360, 240, 239, COLOR_WHITE);
		draw_circle(360, 240, 238, COLOR_WHITE);
		draw_pie(360, 240, 150, ang_hour - 1, ang_hour + 1, COLOR_WHITE);
		draw_pie(360, 240, 200, ang_min - 1, ang_min + 1, COLOR_WHITE);
		draw_pie(360, 240, 220, ang_sec - 1, ang_sec + 1, COLOR_RED);
	}
}
*/

CONFIG_INT("lcd.sensor.shortcuts", lcd_sensor_shortcuts, 1);

int get_lcd_sensor_shortcuts() { return lcd_sensor_shortcuts; }

static void
lcd_sensor_shortcuts_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"SensorShortcuts: %s", 
		lcd_sensor_shortcuts ? "ON" : "OFF"
	);
}


void take_screenshot( void * priv )
{
	call( "dispcheck" );
	silent_pic_take_lv_dbg();
}

int draw_prop = 0;

CONFIG_INT( "enable-liveview",	enable_liveview, 0 );

static void
draw_prop_select( void * priv )
{
	draw_prop = !draw_prop;
}

static int dbg_propn = 0;
static void 
draw_prop_reset( void * priv )
{
	dbg_propn = 0;
}

int mem_spy = 0;
int mem_spy_start = 0; // start from here
int mem_spy_len = 0x10000/4;    // look at ### int32's
int mem_spy_bool = 0;           // only display booleans (0,1,-1)
int mem_spy_small = 0;          // only display small numbers (less than 10)

static void
mem_spy_select( void * priv )
{
	mem_spy = !mem_spy;
}

void card_led_on() { AJ_guess_LED_ON(1); }
void card_led_off() { AJ_guess_LED_OFF(1); }
void card_led_blink(int times, int delay_on, int delay_off)
{
	int i;
	for (i = 0; i < times; i++)
	{
		card_led_on();
		msleep(delay_on);
		card_led_off();
		msleep(delay_off);
	}
}

int config_ok = 0;

static void
save_config( void * priv )
{
	config_save_file( "B:/magic.cfg" );
}
static void
delete_config( void * priv )
{
	FIO_RemoveFile( "B:/magic.cfg" );
	if (config_autosave) config_autosave_toggle(0);
}

static void
config_autosave_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Config AutoSave: %s", 
		config_autosave ? "ON" : "OFF"
	);
}

void
config_autosave_toggle(void* priv)
{
	config_flag_file_setting_save(CONFIG_AUTOSAVE_FLAG_FILE, !!config_autosave);
	msleep(50);
	config_autosave = !config_flag_file_setting_load(CONFIG_AUTOSAVE_FLAG_FILE);
}

PROP_INT(PROP_MVR_REC_START, recording);

//----------------begin qscale-----------------
CONFIG_INT( "h264.qscale.index", qscale_index, 6 );
CONFIG_INT( "h264.bitrate.mode", bitrate_mode, 0 ); // off, CBR, VBR, MAX
CONFIG_INT( "h264.bitrate.value.index", bitrate_value_index, 14 );

int qscale_values[] = {16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16};
int bitrate_values[] = {1,2,3,4,5,6,7,8,10,12,15,18,20,25,30,35,40,45,50,60,70,80,90,100,110,120};

#define BITRATE_VALUE bitrate_values[mod(bitrate_value_index, COUNT(bitrate_values))]

int get_prescribed_bitrate() { return BITRATE_VALUE; }
int get_bitrate_mode() { return bitrate_mode; }

int qscale = 0; // prescribed value

// don't call those outside vbr_fix / vbr_set
void mvrFixQScale(uint16_t *);    // only safe to call when not recording
void mvrSetDefQScale(int16_t *);  // when recording, only change qscale by 1 at a time
// otherwise ther appears a nice error message which shows the shutter count [quote AlinS] :)

void vbr_fix(uint16_t param)
{
	if (!lv_drawn()) return;
	if (shooting_mode != SHOOTMODE_MOVIE) return; 
	if (recording) return; // err70 if you do this while recording

	mvrFixQScale(&param);
}
void vbr_set()
{
	static int k = 0;
	k++;
	//~ bmp_printf(FONT_SMALL, 10,70, "vbr_set %3d %d %d", k, bitrate_mode, qscale);

	if (!lv_drawn()) return;
	if (shooting_mode != SHOOTMODE_MOVIE) return; 
	if (recording == 1) return; 
	
	if (bitrate_mode == 0)
	{
		//~ bmp_printf(FONT_SMALL, 10,50, "QScale OFF");
		vbr_fix(0);
	}
	else
	{
		static int16_t qscale_slow = 0;
		//~ bmp_printf(FONT_SMALL, 10,50, "QScale %d %d", qscale, qscale_slow);
		qscale_slow += SGN(qscale - qscale_slow);
		qscale_slow = COERCE(qscale_slow, -16, 16);
		vbr_fix(1);
		mvrSetDefQScale(&qscale_slow);
		//~ bmp_printf(FONT_MED, 0, 100, "B=%d,%d Q=%d  ", MVR_BUFFER_USAGE_FRAME, MVR_BUFFER_USAGE_SOUND, qscale_slow);
	}
}

int get_qscale() { return qscale; }

void vbr_toggle( void * priv )
{
	qscale_index = mod(qscale_index - 1, COUNT(qscale_values));
	qscale = qscale_values[qscale_index];
}

void vbr_toggle_reverse( void * priv )
{
	qscale_index = mod(qscale_index + 1, COUNT(qscale_values));
	qscale = qscale_values[qscale_index];
}

void vbr_bump(int delta) // do not change the saved setting (=> do not change qscale_index)
{
	//~ bmp_printf(FONT_MED, 0, 200, "bump %d  ", delta);
	qscale = COERCE(qscale + delta, -16, 16);
}
//-------------------------end qscale--------------


static void
bitrate_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (bitrate_mode == 0)
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Bit Rate      : FW default");
	else if (bitrate_mode == 1)
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Bit Rate      : CBRe,%dm/s", BITRATE_VALUE);
	else if (bitrate_mode == 2)
	{
		qscale = qscale_values[qscale_index];
		bmp_printf( selected ? MENU_FONT_SEL : MENU_FONT, x, y, "Bit Rate      : QScale %d", qscale);
	}
}

static void 
bitrate_toggle_forward(void* priv)
{
	if (bitrate_mode == 0)
		return;
	else if (bitrate_mode == 1)
		bitrate_value_index = mod(bitrate_value_index + 1, COUNT(bitrate_values));
	else if (bitrate_mode == 2)
		vbr_toggle(0);
}

static void 
bitrate_toggle_reverse(void* priv)
{
	if (bitrate_mode == 0)
		return;
	else if (bitrate_mode == 1)
		bitrate_value_index = mod(bitrate_value_index - 1, COUNT(bitrate_values));
	else if (bitrate_mode == 2)
		vbr_toggle_reverse(0);
}


CONFIG_INT("movie.af", movie_af, 0);
CONFIG_INT("movie.af.aggressiveness", movie_af_aggressiveness, 4);
CONFIG_INT("movie.af.noisefilter", movie_af_noisefilter, 7); // 0 ... 9
CONFIG_INT("movie.restart", movie_restart,0);
CONFIG_INT("movie.mode-remap", movie_mode_remap, 0);
CONFIG_INT("movie.rec-key", movie_rec_key, 0);
int movie_af_stepsize = 10;

int get_focus_graph() { return (movie_af || (get_trap_focus() && can_lv_trap_focus_be_active()) || get_follow_focus_stop_on_focus()) && zebra_should_run(); }

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

PROP_INT(PROP_BACKLIGHT_LEVEL, backlight_level);
void adjust_backlight_level(int delta)
{
	if (backlight_level < 1 || backlight_level > 7) return; // kore wa dame desu yo
	display_on_force();
	int level = COERCE(backlight_level + delta, 1, 7);
	prop_request_change(PROP_BACKLIGHT_LEVEL, &level, 4);
	if (!lv_drawn()) bmp_printf(FONT_LARGE, 200, 240, "Backlight: %d", level);
}


PROP_INT(PROP_AF_MODE, af_mode);
PROP_INT(PROP_DOF_PREVIEW_MAYBE, dofpreview);

int lv_focus_confirmation = 0;
static int hsp_countdown = 0;
int can_lv_trap_focus_be_active()
{
	if (!lv_drawn()) return 0;
	if (hsp_countdown) return 0; // half-shutter can be mistaken for DOF preview, but DOF preview property triggers a bit later
	if (dofpreview) return 0;
	if (shooting_mode == SHOOTMODE_MOVIE) return 0;
	if (gui_state != GUISTATE_IDLE) return 0;
	if (get_silent_pic_mode()) return 0;
	if ((af_mode & 0xF) != 3) return 0;
	return 1;
}

int get_lv_focus_confirmation() 
{ 
	if (!can_lv_trap_focus_be_active()) return 0;
	if (!get_halfshutter_pressed()) return 0;
	int ans = lv_focus_confirmation;
	lv_focus_confirmation = 0;
	return ans; 
}

volatile int focus_done = 0;
volatile uint32_t focus_done_raw = 0;
PROP_HANDLER(PROP_LV_FOCUS_DONE)
{
	focus_done_raw = buf[0];
	focus_done = 1;
	return prop_cleanup(token, property);
}

int mode_remap_done = 0;
PROP_HANDLER(PROP_SHOOTING_MODE)
{
	if (shooting_mode != buf[0]) mode_remap_done = 0;
	shooting_mode = buf[0];
	restore_kelvin_wb();
	return prop_cleanup(token, property);
}

static int vmax(int* x, int n)
{
	int i; 
	int m = -100000;
	for (i = 0; i < n; i++)
		if (x[i] > m)
			m = x[i];
	return m;
}

int is_manual_focus()
{
	return (af_mode & 0xF) == 3;
}

int movie_af_active()
{
	return shooting_mode == SHOOTMODE_MOVIE && lv_drawn() && !is_manual_focus() && (focus_done || movie_af==3);
}

static int hsp = 0;
int movie_af_reverse_dir_request = 0;
PROP_HANDLER(PROP_HALF_SHUTTER)
{
	if (buf[0] && !hsp) movie_af_reverse_dir_request = 1;
	hsp = buf[0];
	hsp_countdown = 15;
	if (get_zoom_overlay_z()) zoom_overlay_disable();
	
	if (movie_rec_key && hsp && shooting_mode == SHOOTMODE_MOVIE)
	{
		if (!recording) schedule_movie_start();
		else schedule_movie_end();
	}
	
	return prop_cleanup(token, property);
}

static void movie_af_step(int mag)
{
	if (!movie_af_active()) return;
	
	#define MAXSTEPSIZE 64
	#define NP ((int)movie_af_noisefilter)
	#define NQ (10 - NP)
	
	int dirchange = 0;
	static int dir = 1;
	static int prev_mag = 0;
	static int target_focus_rate = 1;
	if (mag == prev_mag) return;
	
	bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "    ");

	static int dmag = 0;
	dmag = ((mag - prev_mag) * NQ + dmag * NP) / 10; // focus derivative is filtered (it's noisy)
	int dmagp = dmag * 10000 / prev_mag;
	static int dmagp_acc = 0;
	static int acc_num = 0;
	dmagp_acc += dmagp;
	acc_num++;
	
	if (focus_done_raw & 0x1000) // bam! focus motor has hit something
	{
		dirchange = 1;
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "BAM!");
	}
	else if (movie_af_reverse_dir_request)
	{
		dirchange = 1;
		movie_af_reverse_dir_request = 0;
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "REV ");
	}
	else
	{
		if (dmagp_acc < -500 && acc_num >= 2) dirchange = 1;
		if (ABS(dmagp_acc) < 500)
		{
			bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, " !! "); // confused
		}
		else
		{
			dmagp_acc = 0;
			acc_num = 0;
			bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, " :) "); // it knows exactly if it's going well or not
		}

		if (ABS(dmagp) > target_focus_rate) movie_af_stepsize /= 2;       // adjust step size in order to maintain a preset rate of change in focus amount
		else movie_af_stepsize = movie_af_stepsize * 3 / 2;               // when focus is "locked", derivative of focus amount is very high => step size will be very low
		movie_af_stepsize = COERCE(movie_af_stepsize, 2, MAXSTEPSIZE);    // when OOF, derivative is very small => will increase focus speed
	}
	
	if (dirchange)
	{
		dir = -dir;
		dmag = 0;
		target_focus_rate /= 4;
	}
	else
	{
		target_focus_rate = target_focus_rate * 11/10;
	}
	target_focus_rate = COERCE(target_focus_rate, movie_af_aggressiveness * 20, movie_af_aggressiveness * 100);

	focus_done = 0;	
	static int focus_pos = 0;
	int focus_delta = movie_af_stepsize * SGN(dir);
	focus_pos += focus_delta;
	lens_focus(7, focus_delta);  // send focus command

	//~ bmp_draw_rect(7, COERCE(350 + focus_pos, 100, 620), COERCE(380 - mag/200, 100, 380), 2, 2);
	
	if (get_global_draw())
	{
		bmp_fill(0, 8, 151, 128, 10);                                          // display focus info
		bmp_fill(COLOR_RED, 8, 151, movie_af_stepsize, 5);
		bmp_fill(COLOR_BLUE, 8, 156, 64 * target_focus_rate / movie_af_aggressiveness / 100, 5);
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 160, "%s %d%%   ", dir > 0 ? "FAR " : "NEAR", dmagp/100);
	}
	prev_mag = mag;
}


static void plot_focus_mag(int mag)
{
	if (gui_state != GUISTATE_IDLE) return;
	if (!lv_drawn()) return;
	#define NMAGS 64
	#define FH COERCE(mags[i] * 45 / maxmagf, 0, 50)
	static int mags[NMAGS] = {0};
	int maxmag = 1;
	int i;
	#define WEIGHT(i) 1
	for (i = 0; i < NMAGS-1; i++)
		if (mags[i] * WEIGHT(i) > maxmag) maxmag = mags[i] * WEIGHT(i);

	static int maxmagf = 1;
	maxmagf = (maxmagf * 4 + maxmag * 1) / 5;
	
	for (i = 0; i < NMAGS-1; i++)
	{
		bmp_draw_rect(COLOR_BLACK, 8 + i, 100, 0, 50);
		mags[i] = mags[i+1];
		bmp_draw_rect(COLOR_YELLOW, 8 + i, 150 - FH, 0, FH);
	}
	// i = NMAGS-1
	mags[i] = mag;

	focus_value_delta = FH * 2 - focus_value;
	focus_value = FH * 2;
	lv_focus_confirmation = (focus_value + focus_value_delta*3 > 110);
	
	static int rev_countdown = 0;
	static int stop_countdown = 0;
	if (is_follow_focus_active())
	{
		plot_focus_status();
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "    ");
		if (get_follow_focus_stop_on_focus() && !stop_countdown)
		{
			if (!rev_countdown)
			{
				if (focus_value - focus_value_delta*5 > 110)
				{
					follow_focus_reverse_dir();
					rev_countdown = 5;
					bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "PEAK");
				}
			}
			else
			{
				rev_countdown--;
				bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "PEAK");
				if (focus_value + focus_value_delta*5 > 110) rev_countdown = 0;
				if (!rev_countdown)
				{
					bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "LOCK");
					lens_focus_stop();
					stop_countdown = 10;
				}
			}
		}
		if (stop_countdown) stop_countdown--;
	}
	#undef FH
	#undef NMAGS
}

int focus_mag_a = 0;
int focus_mag_b = 0;
int focus_mag_c = 0;
PROP_HANDLER(PROP_LV_FOCUS_DATA)
{
	focus_mag_a = buf[2];
	focus_mag_b = buf[3];
	focus_mag_c = buf[4];
	
	if (movie_af != 3)
	{
		if (get_focus_graph() && get_global_draw()) plot_focus_mag(focus_mag_a + focus_mag_b);
		if ((movie_af == 2) || (movie_af == 1 && get_halfshutter_pressed())) 
			movie_af_step(focus_mag_a + focus_mag_b);
	}
	return prop_cleanup(token, property);
}

static void
movie_af_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (movie_af)
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Movie AF      : %s A%d N%d",
			movie_af == 1 ? "Hold" : movie_af == 2 ? "Cont" : movie_af == 3 ? "CFPk" : "Err",
			movie_af_aggressiveness,
			movie_af_noisefilter
		);
	else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Movie AF      : OFF"
		);
}

void movie_af_aggressiveness_bump(void* priv)
{
	movie_af_aggressiveness = movie_af_aggressiveness * 2;
	if (movie_af_aggressiveness > 64) movie_af_aggressiveness = 1;
}
void movie_af_noisefilter_bump(void* priv)
{
	movie_af_noisefilter = (movie_af_noisefilter + 1) % 10;
}

int setting_shooting_mode = 0;
void set_shooting_mode(int m)
{
	setting_shooting_mode = 1;
	msleep(200);
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
/*
void fake_lens(void* priv)
{
	uint32_t lens[4];
	lens[0] = 0x00101001;
	lens[1] = 0xFF01001b;
	lens[2] = 0x0D0490FF;
	lens[3] = 0;
	bmp_printf(FONT_MED, 0, 0, "lens chg");
	prop_request_change(PROP_LENS, lens, 14);
	msleep(500);

	int ap = 0x10;
	prop_request_change(PROP_APERTURE3, &ap, 4);
	msleep(100);
	prop_request_change(PROP_APERTURE2, &ap, 4);
	msleep(100);
	prop_request_change(PROP_APERTURE, &ap, 4);
	msleep(100);
	bmp_printf(FONT_MED, 0, 0, "ap chg");
	msleep(500);
}
*/

/*
int foc_mod = 7;
int foc_en = 1;
void focus_test(void* priv)
{
	lens_focus_ex(7, 1000, 1);
	msleep(10);
	lens_focus_ex(foc_mod, -100, foc_en);
}
void focus_mod_bump(void* priv)
{
	foc_mod = (foc_mod + 1) % 16;
}
void focus_en_bump(void* priv)
{
	foc_en = (foc_en + 1) % 16;
}
static void
focus_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Focus mode=%x en=%x", foc_mod, foc_en);
}*/

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
		"Start with LV : %s",
		enable_liveview == 1 ? "Movie mode" : enable_liveview == 2 ? "All modes" : "OFF"
	);
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
		dof_adjust ? "ON" : "OFF"
	);
}


CONFIG_INT("swap.aperture.shutter", as_swap_enable, 0);

static void
as_swap_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Swap Av <-> Tv: %s",
		as_swap_enable ? "ON" : "OFF"
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
PROP_INT(PROP_DISPSENSOR_CTRL, display_sensor_neg);

/*
volatile int swap_a = 0;
volatile int swap_s = 0;

void swap_countdown()
{
	bmp_printf(FONT_MED, 0, 40, "s%d a%d", swap_s, swap_a);
	if (swap_a) swap_a--;
	if (swap_s) swap_s--;
}

PROP_HANDLER(PROP_APERTURE)
{
	static int old = 0;
	
	if (lv_drawn() && display_sensor_neg == 0 && old)
	{
		if (buf[0] != old)
		{
			int newiso = COERCE(lens_info.raw_iso + buf[0] - old, codes_iso[1], codes_iso[COUNT(codes_iso)-1]);
			lens_set_rawiso(newiso);
		}
	}

	if (as_swap_enable && shooting_mode == SHOOTMODE_MOVIE && old && swap_a==0 && buf[0] != old && !gui_menu_shown())
	{
		bmp_printf(FONT_LARGE, 0, 0, "SWAP AV");
		swap_s = 10;
		swap_a = 10;
		int d = buf[0] - old;
		lens_set_rawaperture(old);
		lens_set_rawshutter(lens_info.raw_shutter + d);
		swap_a = 0;
	}
	else
	{
		old = buf[0];
	}

	return prop_cleanup(token, property);
}

PROP_HANDLER(PROP_SHUTTER)
{
	static int old = 0;
	if (as_swap_enable && shooting_mode == SHOOTMODE_MOVIE && old && swap_s==0 && buf[0] != old && !gui_menu_shown())
	{
		bmp_printf(FONT_LARGE, 0, 0, "SWAP TV");
		swap_s = 10;
		swap_a = 10;
		int d = buf[0] - old;
		lens_set_rawshutter(old);
		lens_set_rawaperture(lens_info.raw_aperture + d);
		swap_s = 0;
	}
	else
	{
		old = buf[0];
	}
	return prop_cleanup(token, property);
} */
/*
void font_test(void* priv)
{
	gui_stop_menu();
	msleep(500);
	
	bfnt_puts("Hello, world!", 10, 20, COLOR_BLACK, COLOR_WHITE);
	int msg[] = {0x9381e3, 0x9382e3, 0xab81e3, 0xa181e3, 0xaf81e3, 0};
	bfnt_puts_utf8(msg, 250, 20, COLOR_BLACK, COLOR_WHITE);
}*/

void xx_test(void* priv)
{
	static int i = 0;
	ChangeColorPalette(i);
	i++;
	/*
	int i;
	char fn[100];
	for (i = 0; i < 5000; i++)
	{
		snprintf(fn, 100, "B:/DCIM/100CANON/%08d.422", i);
		bmp_printf(FONT_MED, 0, 0, fn);
		FIO_RemoveFile(fn);
	}*/
}

void toggle_mirror_display()
{
	zebra_pause();
	if (lv_drawn()) msleep(200); // redrawing screen while zebra is active seems to cause trouble
	static int i = 0;
	if (i) MirrorDisplay();
	else NormalDisplay();
	i = !i;
	msleep(200);
	zebra_resume();
}

void fake_simple_button(int bgmt_code)
{
	struct event e = {
		.type = 0,
		.param = bgmt_code, 
		.obj = 0,
		.arg = 0,
	};
	GUI_CONTROL(&e);
}

void lv_redraw()
{
	if (recording && MVR_FRAME_NUMBER < 50) return;

	if (lv_drawn())
	{
		zebra_pause();
		bmp_enabled = 0;
		msleep(200);
		redraw_maybe();
		ChangeColorPaletteLV(2);
		msleep(200);
		bmp_enabled = 1;
		zebra_resume();
	}
	else
		redraw_maybe();

	afframe_countdown = 50;
}

void turn_off_display()
{
	if (lens_info.job_state) return;
	display_off();
	call("TurnOffDisplay"); // force it
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

static uint32_t* dbg_memmirror = 0;
static uint32_t* dbg_memchanges = 0;

static void dbg_memspy_init() // initial state of the analyzed memory
{
	bmp_printf(FONT_MED, 10,10, "memspy init @ %x ... (+%x) ... %x", mem_spy_start, mem_spy_len, mem_spy_start + mem_spy_len * 4);
	//~ msleep(2000);
	//mem_spy_len is number of int32's
	if (!dbg_memmirror) dbg_memmirror = AllocateMemory(mem_spy_len*4 + 100); // local copy of mem area analyzed
	if (!dbg_memmirror) return;
	if (!dbg_memchanges) dbg_memchanges = AllocateMemory(mem_spy_len*4 + 100); // local copy of mem area analyzed
	if (!dbg_memchanges) return;
	int i;
	//~ bmp_printf(FONT_MED, 10,10, "memspy alloc");
	uint32_t crc = 0;
	for (i = 0; i < mem_spy_len; i++)
	{
		uint32_t addr = mem_spy_start + i*4;
		dbg_memmirror[i] = *(uint32_t*)(addr);
		dbg_memchanges[i] = 0;
		crc += dbg_memmirror[i];
		//~ bmp_printf(FONT_MED, 10,10, "memspy: %8x => %8x ", addr, dbg_memmirror[i]);
	}
	//~ bmp_printf(FONT_MED, 10,10, "memspy OK: %x", crc);
}
static void dbg_memspy_update()
{
	if (!dbg_memmirror) return;
	if (!dbg_memchanges) return;
	int i;
	int k=0;
	for (i = 0; i < mem_spy_len; i++)
	{
		uint32_t fnt = FONT_SMALL;
		uint32_t addr = mem_spy_start + i*4;
		int32_t oldval = dbg_memmirror[i];
		int32_t newval = *(uint32_t*)(addr);
		if (oldval != newval)
		{
			//~ bmp_printf(FONT_MED, 10,460, "memspy: %8x: %8x => %8x", addr, oldval, newval);
			dbg_memmirror[i] = newval;
			if (dbg_memchanges[i] < 10000) dbg_memchanges[i]++;
			fnt = FONT(FONT_SMALL, 5, COLOR_BG);
		}
		//~ else continue;

		if (mem_spy_bool && newval != 0 && newval != 1 && newval != -1) continue;
		if (mem_spy_small && ABS(newval) > 10) continue;

		// show addresses which change, but not those which change like mad
		if (dbg_memchanges[i] > 5 && dbg_memchanges[i] < 50)
		{
			int x = 10 + 8 * 22 * (k % 4);
			int y = 10 + 12 * (k / 4);
			bmp_printf(fnt, x, y, "%8x:%2d:%8x", addr, dbg_memchanges[i], newval);
			k = (k + 1) % 120;
		}
	}

	for (i = 0; i < 10; i++)
	{
		int x = 10 + 8 * 22 * (k % 4);
		int y = 10 + 12 * (k / 4);
		bmp_printf(FONT_SMALL, x, y, "                    ");
		k = (k + 1) % 120;
	}
}

PROP_INT(PROP_SHUTTER_COUNT, shutter_count);

/*int battery_level_raw = 0;
PROP_HANDLER(PROP_BATTERY_CHECK)
{
	battery_level_raw = buf[21];
	//~ bmp_hexdump(FONT_SMALL, 0, 20, buf, 32*30);
	//~ call("dispcheck");
	return prop_cleanup(token, property);
}
PROP_INT(PROP_BATTERY_RAW_LEVEL_MAYBE, battery_level_raw_maybe);*/

void display_info()
{
	bmp_printf(FONT_MED, 20, 400, "Shutter Count: %d", shutter_count);
	bmp_printf(FONT_MED, 20, 420, "CMOS Temperat: %d", efic_temp);
	//~ bmp_printf(FONT_MED, 20, 440, "Battery level: %d or %d", battery_level_raw, battery_level_raw_maybe);
	bmp_printf(FONT_MED, 20, 440, "Lens: %s          ", lens_info.name);
	//~ bmp_printf(FONT_MED, 20, 440, "%d  ", *(int*)0x25334);
}

void display_shortcut_key_hints_lv()
{
	static int old_mode = 0;
	int mode = 0;
	if (!zebra_should_run()) return;
	if (shooting_mode == SHOOTMODE_MOVIE && FLASH_BTN_MOVIE_MODE) mode = 1;
	else if (get_lcd_sensor_shortcuts() && !gui_menu_shown() && display_sensor_neg == 0 && DISPLAY_SENSOR_POWERED) mode = 2;
	else if (is_follow_focus_active() && !is_manual_focus() && (display_sensor_neg != 0 || !get_lcd_sensor_shortcuts())) mode = 3;
	if (mode == 0 && old_mode == 0) return;

	int mz = (mode == 2 && get_zoom_overlay_z() && lv_dispsize == 1);
	
	if (mode == 1)
	{
		bmp_printf(FONT_MED, 360 - 100 - font_med.width*2, 240 - font_med.height/2, "ISO-");
		bmp_printf(FONT_MED, 360 + 100 - font_med.width*2, 240 - font_med.height/2, "ISO+");
		bmp_printf(FONT_MED, 360 - font_med.width*2, 240 - 100 - font_med.height/2, "Vol+");
		bmp_printf(FONT_MED, 360 - font_med.width*2, 240 + 100 - font_med.height/2, "Vol-");
	}
	else if (mode == 2)
	{
		bmp_printf(FONT_MED, 360 - 100 - font_med.width*2, 240 - font_med.height/2, "Kel-");
		bmp_printf(FONT_MED, 360 + 100 - font_med.width*2, 240 - font_med.height/2, "Kel+");
		bmp_printf(FONT_MED, 360 - font_med.width*2, 240 - 100 - font_med.height/2, "LCD+");
		bmp_printf(FONT_MED, 360 - font_med.width*2, 240 + 100 - font_med.height/2, "LCD-");
	}
	else if (mode == 3)
	{
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 - 100 - font_med.width*2, 240 - font_med.height/2, "FF+ ");
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 + 100 - font_med.width*2, 240 - font_med.height/2, "FF- ");
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 - font_med.width*2, 240 - 100 - font_med.height/2, "FF++");
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 - font_med.width*2, 240 + 100 - font_med.height/2, "FF--");
	}
	else
	{
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 - 100 - font_med.width*2, 240 - font_med.height/2, "    ");
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 + 100 - font_med.width*2, 240 - font_med.height/2, "    ");
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 - font_med.width*2, 240 - 100 - font_med.height/2, "    ");
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 - font_med.width*2, 240 + 100 - font_med.height/2, "    ");
	}

	if (mz) bmp_printf(FONT_MED, 360 + 100, 240 - 150, "Magic Zoom");
	else bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 360 + 100, 240 - 150, "          ");

	old_mode = mode;
}

void display_clock()
{
	int bg = bmp_getpixel(15, 430);

	struct tm now;
	LoadCalendarFromRTC( &now );
	if (lv_drawn())
	{
		uint32_t fnt = FONT(FONT_MED, COLOR_WHITE, TOPBAR_BGCOLOR);
		bmp_printf(fnt, 0, 0, "%02d:%02d", now.tm_hour, now.tm_min);
	}
	else
	{
		uint32_t fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);
		bmp_printf(fnt, 200, 410, "%02d:%02d", now.tm_hour, now.tm_min);
	}
}

PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);

static void dbg_draw_props(int changed);
static unsigned dbg_last_changed_propindex = 0;
int screenshot_sec = 0;
static void
debug_loop_task( void ) // screenshot, draw_prop
{
	//~ gui_unlock();
	do_movie_mode_remap();
	if (!lv_drawn() && ((enable_liveview == 2) || (enable_liveview == 1 && shooting_mode == SHOOTMODE_MOVIE)))
	{
		if (shooting_mode == SHOOTMODE_MOVIE)
		{
			set_shooting_mode(SHOOTMODE_NIGHT); // you can run, but you cannot hide :)
			msleep(500);
			call( "FA_StartLiveView" );
			msleep(1000);
			set_shooting_mode(SHOOTMODE_MOVIE);
		}
		else
		{
			call( "FA_StartLiveView" );
		}
	}
	
	dbg_memspy_init();
	int k;
	for (k = 0; ; k++)
	{
		msleep(10);
		if (gui_state == GUISTATE_MENUDISP)
		{
			display_info();
		}
		
		//~ bmp_printf(FONT_MED, 0, 0, "%x %x %x", AUDIO_MONITORING_HEADPHONES_CONNECTED, *(int*)VIDEO_OUT_PROP_DELIVER_COUNTER, *(int*)VIDEO_OUT_PROP_DELIVER_VALUE);
		//~ struct tm now;
		//~ LoadCalendarFromRTC(&now);
		//~ bmp_hexdump(FONT_SMALL, 0, 20, 0x14c00, 32*5);
		//~ bmp_hexdump(FONT_SMALL, 0, 200, 0x26B8, 32*5);
		
		//~ if (recording == 2)
			//~ bmp_printf(FONT_MED, 0, 0, "frame=%d bytes=%8x", MVR_FRAME_NUMBER, MVR_BYTES_WRITTEN);
		//~ bmp_hexdump(FONT_SMALL, 0, 20, 0x1E774, 32*10);
		//~ bmp_printf(FONT_MED, 0, 0, "%x  ", *(int*)131030);
		//~ DEBUG("MovRecState: %d", MOV_REC_CURRENT_STATE);
		
		if (!lv_drawn() && gui_state == GUISTATE_IDLE && !gui_menu_shown() && /*!big_clock &&*/ bmp_getpixel(2,10) != 2 && k % 10 == 0)
		{
			display_clock();
			display_shooting_info();
		}
		
		if (lv_drawn())
		{
			display_shooting_info_lv();
			if (shooting_mode == SHOOTMODE_MOVIE && !ae_mode_movie && !gui_menu_shown()) 
				bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, 0), 100, 50, "!!! Auto exposure !!!");
			display_shortcut_key_hints_lv();
		}
		
		if (screenshot_sec)
		{
			if (screenshot_sec >= 5) bmp_printf( FONT_SMALL, 0, 0, "Screenshot in %d seconds ", screenshot_sec);
			if (screenshot_sec == 4) lv_redraw();
			screenshot_sec--;
			msleep( 1000 );
			if (!screenshot_sec)
				take_screenshot(0);
		}
		
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
		
		if (movie_af == 3)
		{
			int fm = get_spot_focus(100);
			if (get_focus_graph() && get_global_draw()) plot_focus_mag(fm);
			movie_af_step(fm);
		}
		
		do_movie_mode_remap();
		
		if (lv_drawn() && shooting_mode == SHOOTMODE_MOVIE && k % 5 == 0) 
		{
			if (recording == 2)
			{
				static int prev_fn = 0;
				if (prev_fn != MVR_FRAME_NUMBER) // only run this once per frame
				{
					static int prev_buffer_usage = 0;
					int buffer_usage = MVR_BUFFER_USAGE_FRAME;
					int buffer_delta = buffer_usage - prev_buffer_usage;
					prev_buffer_usage = buffer_usage;
					
					//~ if (buffer_delta > 0 && MVR_BUFFER_USAGE > 70) vbr_bump(10); // panic
					//~ else if (buffer_delta > 0 && MVR_BUFFER_USAGE > 55) vbr_bump(3); // just a bit of panic
					//if (buffer_delta > 0 && MVR_BUFFER_USAGE_FRAME > 35) vbr_bump(1);
					//else if (buffer_usage < 35 && k % 10 == 0) // buffer ok, we can adjust qscale according to the selected preset
					
					int comp = 0;
					
					if (buffer_delta > 0 && buffer_usage > 50)
					{
						bmp_fill(COLOR_RED, 720-64, 60, 32, 4);
						comp = -10;
					}
					else
					{
						bmp_fill(0, 720-64, 60, 32, 4);
						comp = 0;
					}
					
					if (bitrate_mode == 1 && get_new_measurement()) // CBRe
					{
						if (get_measured_bitrate() > BITRATE_VALUE + comp) vbr_bump(1);
						else if (get_measured_bitrate() < BITRATE_VALUE + comp) vbr_bump(-1);
					}
					else if (bitrate_mode == 2) // qscale
					{
						vbr_bump(SGN(qscale_values[qscale_index] - qscale));
					}
				}
				prev_fn = MVR_FRAME_NUMBER;
			}
			vbr_set();
		}
		
		if (af_frame_autohide && lv_drawn() && afframe_countdown)
		{
			afframe_countdown--;
			if (!afframe_countdown) clear_lv_afframe();
		}

		if (!DISPLAY_SENSOR_POWERED) // force sensor on
		{
			DispSensorStart();
		}
		
		if (lv_drawn() && display_force_off && !gui_menu_shown() && gui_state == GUISTATE_IDLE && !get_halfshutter_pressed() && k % 100 == 0 && (!DISPLAY_SENSOR_POWERED || display_sensor_neg))
		{
			turn_off_display();
			if (k % 500 == 0) card_led_blink(1, 20, 0);
		}
		if (lv_drawn() && (DISPLAY_SENSOR_POWERED && !display_sensor_neg))
		{
			display_on();
		}
		
		if (lv_metering && shooting_mode != SHOOTMODE_MOVIE && lv_drawn() && k % 10 == 0)
		{
			lv_metering_adjust();
		}
		
		// faster zoom in play mode
		if (gui_state == GUISTATE_PLAYMENU)
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
		
		workaround_wb_kelvin = lens_info.kelvin;
		workaround_wbs_gm = lens_info.wbs_gm + 100;
		workaround_wbs_ba = lens_info.wbs_ba + 100;
		
		expsim_update();
		
		if (BTN_METERING_PRESSED_IN_LV)
		{
			toggle_disp_mode();
			while (BTN_METERING_PRESSED_IN_LV) msleep(100);
		}

		/*if (big_clock && k % 10 == 0)
		{
			show_big_clock();
		}*/
		
		if (draw_prop)
		{
			dbg_draw_props(dbg_last_changed_propindex);
		}
		else if (mem_spy)
		{
			dbg_memspy_update();
		}
		
		if (hsp_countdown) hsp_countdown--;
		
		msleep(10);
	}
}

static void screenshot_start(void)
{
	screenshot_sec = 10;
}

void toggle_draw_event( void * priv );

static void
spy_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Spy %s/%s/%s (s/d/q)",
		draw_prop ? "PROP" : "prop",
		get_draw_event() ? "EVT" : "evt", 
		mem_spy ? "MEM" : "mem"
	);
}

struct rolling_pitching 
{
	uint8_t status;
	uint8_t cameraposture;
	int8_t roll_sensor1;
	int8_t roll_sensor2;
	int8_t pitch_sensor1;
	int8_t pitch_sensor2;
};
struct rolling_pitching level_data;

PROP_HANDLER(PROP_ROLLING_PITCHING_LEVEL)
{
	memcpy(&level_data, buf, 6);
	return prop_cleanup(token, property);
}

struct menu_entry debug_menus[] = {
	{
		.priv = &display_force_off,
		.display	= display_off_print, 
		.select = menu_binary_toggle,
	},
	{
		.priv		= &lcd_sensor_shortcuts,
		.select		= menu_binary_toggle,
		.display	= lcd_sensor_shortcuts_print,
	},
	/*{
		.priv = &big_clock, 
		.select = menu_binary_toggle,
		.display = big_clock_print,
	},*/
	{
		.priv = &af_frame_autohide, 
		.select = menu_binary_toggle,
		.display = af_frame_autohide_display,
	},
	{
		.priv = &auto_burst_pic_quality, 
		.select = menu_binary_toggle, 
		.display = auto_burst_pic_display,
	},
	{
		.select = expsim_toggle, 
		.display = expsim_display,
	},
	{
		.priv = &lv_metering,
		.select = menu_quinternary_toggle, 
		.select_reverse = menu_quinternary_toggle_reverse, 
		.display = lv_metering_print,
	},
	/*{
		.priv		= "Draw palette",
		.select		= bmp_draw_palette,
		.display	= menu_print,
	},*/
	{
		.priv		= "Screenshot (10 s)",
		.select		= screenshot_start,
		.select_auto = take_screenshot,
		.display	= menu_print,
	},
	{
		.select = dm_toggle, 
		.select_auto		= (void*) dumpf,
		.display	= dm_display,
	},
	{
		.select		= draw_prop_select,
		.select_reverse = toggle_draw_event,
		.select_auto = mem_spy_select,
		.display	= spy_print,
	},
	/*{
		.priv		= "Don't click me!",
		.select		= xx_test,
		.display	= menu_print,
	}*/
/*	{
		.select = focus_test,
		.display = focus_print,
		.select_reverse = focus_en_bump,
		.select_auto = focus_mod_bump
	},
	{
		.priv = "CBR test", 
		.select = cbr_set,
		.display = menu_print,
	}*/

#if 0
	{
		.priv		= "Enable full HD",
		.select		= enable_full_hd,
		.display	= display_full_hd,
	},
	{
		.display	= mvr_time_const_display,
		.select		= mvr_time_const_select,
	},
#endif
/*	{
		.priv		= "Clear config",
		.select		= clear_config,
		.display	= menu_print,
	}, */
};

struct menu_entry mov_menus[] = {
	{
		.priv = &bitrate_mode,
		.display	= bitrate_print,
		.select		= menu_ternary_toggle,
		.select_auto	= bitrate_toggle_forward,
		.select_reverse	= bitrate_toggle_reverse,
	},
	/*{
		.display	= vbr_print,
		.select		= vbr_toggle,
	},*/
	{
		.priv = &movie_restart,
		.display	= movie_restart_print,
		.select		= menu_binary_toggle,
	},
	{
		.priv = &movie_af,
		.display	= movie_af_print,
		.select		= menu_quaternary_toggle,
		.select_reverse = movie_af_noisefilter_bump,
		.select_auto = movie_af_aggressiveness_bump,
	},
	{
		.priv = &enable_liveview,
		.display	= enable_liveview_print,
		.select		= menu_ternary_toggle,
	},
	{
		.priv = &movie_mode_remap,
		.display	= mode_remap_print,
		.select		= menu_ternary_toggle,
	},
	/*{
		.priv = &as_swap_enable, 
		.display = as_swap_print,
		.select = menu_binary_toggle,
	},*/
	{
		.priv = &dof_adjust, 
		.display = dof_adjust_print, 
		.select = menu_binary_toggle,
	},
	{
		.priv = &movie_rec_key, 
		.display = movie_rec_key_print, 
		.select = menu_binary_toggle,
	},
};

static struct menu_entry cfg_menus[] = {
	{
		.display	= config_autosave_display,
		.select		= config_autosave_toggle,
	},
	{
		.priv = "Save config now",
		.display	= menu_print,
		.select		= save_config,
	},
	{
		.priv = "Delete config file",
		.display	= menu_print,
		.select		= delete_config,
	},
};


static void * debug_token;

static void
debug_token_handler(
	void *			token,
	void *			arg1,
	void *			arg2,
	void *			arg3
)
{
	debug_token = token;
	DebugMsg( DM_MAGIC, 3, "token %08x arg=%08x %08x %08x",
		(unsigned) token,
		(unsigned) arg1,
		(unsigned) arg2,
		(unsigned) arg3
	);
}

//~ static int dbg_propn = 0;
#define MAXPROP 30
static unsigned dbg_props[MAXPROP] = {0};
static unsigned dbg_props_len[MAXPROP] = {0};
static unsigned dbg_props_a[MAXPROP] = {0};
static unsigned dbg_props_b[MAXPROP] = {0};
static unsigned dbg_props_c[MAXPROP] = {0};
static unsigned dbg_props_d[MAXPROP] = {0};
static unsigned dbg_props_e[MAXPROP] = {0};
static unsigned dbg_props_f[MAXPROP] = {0};
static void dbg_draw_props(int changed)
{
	dbg_last_changed_propindex = changed;
	int i; 
	for (i = 0; i < dbg_propn; i++)
	{
		unsigned x = 80;
		unsigned y = 15 + i * font_small.height;
		unsigned property = dbg_props[i];
		unsigned len = dbg_props_len[i];
		unsigned fnt = FONT_SMALL;
		if (i == changed) fnt = FONT(FONT_SMALL, 5, COLOR_BG);
		bmp_printf(fnt, x, y,
			"%08x %04x: %8lx %8lx %8lx %8lx %8lx %8lx",
			property,
			len,
			len > 0x00 ? dbg_props_a[i] : 0,
			len > 0x04 ? dbg_props_b[i] : 0,
			len > 0x08 ? dbg_props_c[i] : 0,
			len > 0x0c ? dbg_props_d[i] : 0,
			len > 0x10 ? dbg_props_e[i] : 0,
			len > 0x14 ? dbg_props_f[i] : 0
		);
	}
}


static void *
debug_property_handler(
	unsigned		property,
	void *			UNUSED( priv ),
	void *			buf,
	unsigned		len
)
{
	const uint32_t * const addr = buf;

	/*console_printf("Prop %08x: %2x: %08x %08x %08x %08x\n",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0
	);*/
	
	if( !draw_prop )
		goto ack;
	
	// maybe the property is already in the array
	int i;
	for (i = 0; i < dbg_propn; i++)
	{
		if (dbg_props[i] == property)
		{
			dbg_props_len[i] = len;
			dbg_props_a[i] = addr[0];
			dbg_props_b[i] = addr[1];
			dbg_props_c[i] = addr[2];
			dbg_props_d[i] = addr[3];
			dbg_props_e[i] = addr[4];
			dbg_props_f[i] = addr[5];
			dbg_draw_props(i);
			goto ack; // return with cleanup
		}
	}
	// new property
	if (dbg_propn >= MAXPROP) dbg_propn = MAXPROP-1; // too much is bad :)
	dbg_props[dbg_propn] = property;
	dbg_props_len[dbg_propn] = len;
	dbg_props_a[dbg_propn] = addr[0];
	dbg_props_b[dbg_propn] = addr[1];
	dbg_props_c[dbg_propn] = addr[2];
	dbg_props_d[dbg_propn] = addr[3];
	dbg_props_e[dbg_propn] = addr[4];
	dbg_props_f[dbg_propn] = addr[5];
	dbg_propn++;
	dbg_draw_props(dbg_propn);

ack:
	return prop_cleanup( debug_token, property );
}



#define num_properties 4096
unsigned* property_list = 0;


void
debug_init( void )
{
	draw_prop = 0;

#if 0
	if (!property_list) property_list = AllocateMemory(num_properties * sizeof(unsigned));
	if (!property_list) return;
	unsigned i, j, k;
	unsigned actual_num_properties = 0;
	
	unsigned is[] = {0x80, 0xe, 0x5, 0x4, 0x2, 0x1, 0x0};
	for( i=0 ; i<COUNT(is) ; i++ )
	{
		for( j=0 ; j<=0x8 ; j++ )
		{
			for( k=0 ; k<0x40 ; k++ )
			{
				unsigned prop = 0
					| (is[i] << 24) 
					| (j << 16)
					| (k <<  0);

				property_list[ actual_num_properties++ ] = prop;

				if( actual_num_properties >= num_properties )
					goto thats_all;
			}
		}
	}

thats_all:

	prop_register_slave(
		property_list,
		actual_num_properties,
		debug_property_handler,
		(void*) 0xdeadbeef,
		debug_token_handler
	);
#endif

	menu_add( "Debug", debug_menus, COUNT(debug_menus) );
	menu_add( "Movie", mov_menus, COUNT(mov_menus) );
    menu_add( "Config", cfg_menus, COUNT(cfg_menus) );
}

CONFIG_INT( "debug.timed-dump",		timed_dump, 0 );

//~ CONFIG_INT( "debug.dump_prop", dump_prop, 0 );
//~ CONFIG_INT( "debug.dumpaddr", dump_addr, 0 );
//~ CONFIG_INT( "debug.dumplen", dump_len, 0 );

CONFIG_INT( "magic.disable_bootdiskf",	disable_bootdiskf, 0 );

struct bmp_file_t * logo = -1;
void load_logo()
{
	if (logo == -1) 
		logo = bmp_load("B:/logo.bmp");
}
void show_logo()
{
	load_logo();
	if (logo > 0)
	{
		bmp_draw(logo, 360 - logo->width/2, 240 - logo->height/2, 0, 0);
	}
	else
	{
		bmp_printf( FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK), 200, 100,
			"Magic Lantern\n"
			"...loading...\n"
		);
	}
}

void restore_kelvin_wb()
{
	if (!white_balance_workaround) return;
	
	// sometimes Kelvin WB and WBShift are not remembered, usually in Movie mode 
	lens_set_kelvin_value_only(workaround_wb_kelvin);
	lens_set_wbs_gm(COERCE(((int)workaround_wbs_gm) - 100, -9, 9));
	lens_set_wbs_ba(COERCE(((int)workaround_wbs_ba) - 100, -9, 9));
}

void
debug_init_stuff( void )
{
	config_autosave = !config_flag_file_setting_load(CONFIG_AUTOSAVE_FLAG_FILE);
	config_ok = 1;
	
	dm_update();

	// set qscale from the vector of available values
	qscale_index = mod(qscale_index, COUNT(qscale_values));
	qscale = qscale_values[qscale_index];
	
	restore_kelvin_wb();
	// It was too early to turn these down in debug_init().
	// Only record important events for the display and face detect
	
	
	/*
	DEBUG();
	dm_set_store_level( DM_DISP, 7 );
	dm_set_store_level( DM_LVFD, 7 );
	dm_set_store_level( DM_LVCFG, 7 );
	dm_set_store_level( DM_LVCDEV, 7 );
	dm_set_store_level( DM_LV, 7 );
	dm_set_store_level( DM_RSC, 7 );
	dm_set_store_level( DM_MAC, 7 );
	dm_set_store_level( DM_CRP, 7 );
	dm_set_store_level( DM_SETPROP, 7 );
	dm_set_store_level( DM_PRP, 7 );
	dm_set_store_level( DM_PROPAD, 7 );
	dm_set_store_level( DM_INTCOM, 7 );
	dm_set_store_level( DM_WINSYS, 7 );
	dm_set_store_level( DM_CTRLSRV, 7 );
	dm_set_store_level( DM_GUI, 7);
	dm_set_store_level( DM_GUI_M, 7);
	dm_set_store_level( DM_GUI_E, 7);
	dm_set_store_level( DM_BIND, 7);
	dm_set_store_level( DM_DISP, 7);
	DEBUG();*/
	
	//msleep(1000);
	//bmp_draw_palette();
	//dispcheck();


	unsigned x=10;
	unsigned y=32;

	if (disable_bootdiskf!=0) {
	  bmp_printf( FONT_SMALL, x, y, "**disable_bootdiskf**%s","" );
	  bootdisk_disable();
	}

	if( timed_dump == 0 )
		goto end;

	int sec = timed_dump;

	DebugMsg( DM_MAGIC, 3, "%s: Will do debug dump in %d sec",
		__func__,
		sec
	);

	while( sec-- )
	{
		//~ bmp_printf( FONT_SMALL, 600, 400, "dump %2d", sec );
		msleep( 1000 );
	}

	DebugMsg( DM_MAGIC, 3, "%s: calling dumpf", __func__ );
	dumpf();

end:
	return;
}


//~ TASK_CREATE( "dump_task", dump_task, 0, 0x1e, 0x1000 );
TASK_CREATE( "debug_loop_task", debug_loop_task, 0, 0x1e, 0x1000 );

//~ CONFIG_INT( "debug.timed-start",	timed_start, 0 );
/*
static void
movie_start( void )
{
	int sec = timed_start;
	if( sec == 0 )
		return;

	const int x = 320;
	const int y = 150;

	while( --sec > 0 )
	{
		msleep( 1000 );
		bmp_printf(
			FONT(
				FONT_LARGE,
				sec > 4 ? COLOR_WHITE : COLOR_RED,
				0
			),
			x, y,
			"T-%d",
			sec
		);
	}

	bmp_printf( FONT(FONT_LARGE,COLOR_WHITE,0), x, y, "GO!" );

	call( "MovieStart" );

	msleep( 1000 );

	bmp_printf( FONT(FONT_LARGE,COLOR_WHITE,0), x, y, "   " );
}*/

//~ TASK_CREATE( "movie_start", movie_start, 0, 0x1f, 0x1000 );


PROP_HANDLER(PROP_TERMINATE_SHUT_REQ)
{
	//bmp_printf(FONT_MED, 0, 0, "SHUT REQ %d ", buf[0]);
	if (config_autosave && buf[0] == 0 && config_ok) save_config(0);
	return prop_cleanup(token, property);
}

/*
PROP_HANDLER(PROP_APERTURE)
{
	static int old = 0;
	
	if (old && lv_drawn())
	{
		if (display_sensor_neg == 0)
		{
			if (buf[0] != old)
			{
				int newiso = COERCE(lens_info.raw_iso + buf[0] - old, codes_iso[1], codes_iso[COUNT(codes_iso)-1]);
				lens_set_rawiso(newiso);
			}
		}
	}

	old = buf[0];

	return prop_cleanup(token, property);
}*/

/*
PROP_HANDLER(PROP_SHUTTER)
{
	if (lv_drawn() && shooting_mode == SHOOTMODE_MOVIE)
	{
		static volatile int old = 0;
		
		if (old)
		{
			if (buf[0] != old)
			{
				//~ int newiso = COERCE(lens_info.raw_iso + buf[0] - old, codes_iso[1], codes_iso[COUNT(codes_iso)-1]);
				//~ lens_set_rawiso(newiso);
				buf[0] = old;
			}
		}
		old = buf[0];
	}
	return prop_cleanup(token, property);
}*/
