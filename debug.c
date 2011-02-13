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

extern void bootdisk_disable();

int focus_value = 0; // heuristic from 0 to 100
int focus_value_delta = 0;

#if 0
void
display_full_hd(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	unsigned * gui_struct = (void*) 0x3548;
	unsigned * ps_struct = (void*) 0x11bb8;
	// disp=0x08 == 3 == 1080p, 5 == vga
	// vid=0x0c == 0
	// hdmi=0x10 == 3, 5 == vga
	// c4=0xc4 == 0, 1 == vga

	unsigned (*GetDisplayType)(void) = (void*) 0xFF863590;

	uint32_t * lv_struct = (void*) 0x37fc;

	bmp_printf(
		FONT_MED,
		x, y,
		//23456789012
#if 0
		"disp=%x vid=%x hdmi=%x c4=%x ps=%x type=%x/%x",
		gui_struct[0x08 / 4],
		gui_struct[0x0C / 4],
		gui_struct[0x10 / 4],
		gui_struct[0xC4 / 4],
		ps_struct[0x230 / 4],
		hdmi_config.disp_type,
		hdmi_config.off_0x0c
#else
		"mvr %d/%x",
		mvr_struct->fps,
		mvr_struct->bit_rate
#endif
	);

	bmp_hexdump( FONT_SMALL, 300, 400,
		(void*) 0x7b40,
		64
	);
}


void enable_full_hd( void * priv )
{
#if 0
	if( mvr_struct->fps == 0 )
		mvr_struct->fps = 30;
	uint8_t bitrate = 60;
	//call( "mvrSetBitRate", &bitrate );
	void (*mvrSetBitRate)( uint8_t * bitrate ) = (void*) 0xff84f990;
	mvrSetBitRate( &bitrate );
#endif
	void (*mvrSetQScale)( int8_t * ) = (void*) 0xff9715e0;
	int8_t scale = -30;
	mvrSetQScale( &scale );
	return;

	DebugMsg( DM_MAGIC, 3, "Attempting to set HDMI to full HD" );

#if 1
	thunk ChangeHDMIOutputSizeToFULLHD = (thunk) 0xFFA96260;
	//void (*SetDisplayType)(int) = (void*) 0xFF8620DC;
	void (*SetDisplayType)(int) = (void*) 0xFFB4835C;

	//SetDisplayType( 0 );
	unsigned * gui_struct = (void*) 0x3548;
	unsigned * ps_struct = (void*) 0x11bb8;
	ChangeHDMIOutputSizeToFULLHD();
	//gui_struct[ 0xC4/4 ] = 0; // not resized?
	//ps_struct[ 0x230/4 ] = 6; // who knows

	DebugMsg( DM_MAGIC, 3, "Full HD done?" );
#else
	// This will select the image on the full screen.
	// but not change the output resolution
	void (*lv_output_device)( const char ** arg ) = (void*) 0xFF833218;
	const char * full_hd = "1080full";
	lv_output_device( &full_hd );
#endif
}
#endif


void take_screenshot( void * priv )
{
	call( "dispcheck" );
	silent_pic_take_lv_dbg();
}


static PROP_INT( PROP_EFIC_TEMP, efic_temp );
static PROP_INT(PROP_GUI_STATE, gui_state);
static PROP_INT(PROP_MAX_AUTO_ISO, max_auto_iso);

/*
PROP_HANDLER( PROP_HDMI_CHANGE_CODE )
{
	DebugMsg( DM_MAGIC, 3, "They try to set code to %d", buf[0] );
	return prop_cleanup( token, property );
}
*/

static void
efic_temp_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"CMOS temp:  %ld",
		efic_temp
	);
}

static void
mvr_time_const_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
  /*
	uint8_t * mvr_struct = (void*) 0x1ed4;
	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	struct state_object ** const mvr_state_object = (void*) 0x68a4;
  */
	struct tm now;
	LoadCalendarFromRTC( &now );

	bmp_printf(
		FONT_MED, // selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Date %4d/%2d/%2d %02d:%02d:%02d",
		now.tm_year + 1900,
		now.tm_mon,
		now.tm_mday,
		now.tm_hour,
		now.tm_min,
		now.tm_sec
	);
}

#if 0
static void
mvr_time_const_select( void * priv )
{
/*
	void (*mvr_set_time_const)(int *) = (void*) 0xff9716cc;
	void (*mvr_setd_fullhd)(int *) = (void*) 0xff9716cc;
	//int args[] = { 640, 480 };
	//DebugMsg( DM_MAGIC, 3, "calling mvr_setd_fullhd %d %d", args[0], args[1] );
	//mvr_setd_fullhd( args );

	uint32_t buf[] = { 8 };
	//prop_request_change( 0x207000c, buf, sizeof(buf) );
	void (*lv_magnify)( int * ) = (void*) 0xff83359c;
	lv_magnify( buf );
	void (*mvrSetBitRate)( int * ) = (void*) 0xff84f990;
	//int rate = 24;
	//mvrSetBitRate( &rate );
	mvr_struct->is_vga	= 0;
	mvr_struct->width	= 1920;
	mvr_struct->height	= 1080;
	mvr_struct->fps		= 24;
*/

	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	*(unsigned *)( mvr_hdr + 0x60 ) = 2400;
	*(unsigned *)( mvr_hdr + 0x64 ) = 100;
	*(unsigned *)( mvr_hdr + 0x68 ) = 24;
}
#endif


CONFIG_INT( "debug.draw-prop",		draw_prop, 0 );
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


CONFIG_INT( "debug.mem-spy",		mem_spy, 0 );
CONFIG_INT( "debug.mem-spy.start.lo",	mem_spy_start_lo,	0 ); // start from here
CONFIG_INT( "debug.mem-spy.start.hi",	mem_spy_start_hi,	0 ); // start from here
CONFIG_INT( "debug.mem-spy.len",	mem_spy_len,	10000 );     // look at ### int32's
CONFIG_INT( "debug.mem-spy.bool",	mem_spy_bool,	0 );         // only display booleans (0,1,-1)
CONFIG_INT( "debug.mem-spy.small",	mem_spy_small,	0 );         // only display small numbers (less than 10)

#define mem_spy_start ((uint32_t)mem_spy_start_lo | ((uint32_t)mem_spy_start_hi << 16))

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

static void
save_config( void * priv )
{
	config_save_file( global_config, "B:/magic.cfg" );
}

//----------------begin qscale-----------------
CONFIG_INT( "h264.qscale.neg", qscale_neg, 0 );
CONFIG_INT( "h264.qscale.max.neg", qscale_max_neg, 1 );
CONFIG_INT( "h264.qscale.min.neg", qscale_min_neg, 16 );
CONFIG_INT( "h264.qscale.force", qscale_force, 1);

int16_t qscale = 0;
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define QSCALE_MAX MAX(-(int)qscale_min_neg, -(int)qscale_max_neg) // idiot-proof :)
#define QSCALE_MIN MIN(-(int)qscale_min_neg, -(int)qscale_max_neg)
#define QSCALE_OFF (QSCALE_MAX + 1)

void mvrFixQScale(uint16_t *);
void mvrSetDefQScale(int16_t *);

void vbr_set()
{
	qscale = COERCE(qscale, QSCALE_MIN, QSCALE_OFF);
	//~ if (qscale == QSCALE_OFF) bmp_printf(FONT_SMALL, 10,50, "QScale OFF");
	//~ else bmp_printf(FONT_SMALL, 10,50, "QScale %d ", qscale);
	uint16_t param = (qscale == QSCALE_OFF) ? 0 : 1;                  // select fixed rate or VBR
	mvrFixQScale(&param);
	if (qscale != QSCALE_OFF) mvrSetDefQScale(&qscale);
}

void cbr_set() // test
{
	gui_stop_menu();
	msleep(100);
	bmp_printf(FONT_MED, 10, 150, "cbr: %d", *(int*)(708 + *(int*)7792));
	msleep(1000);
	uint16_t param = 0;
	uint8_t bitrate = 2;
	mvrFixQScale(&param);
	mvrSetBitRate(&bitrate);
}

void vbr_toggle( void * priv )
{
	qscale = MIN(qscale, QSCALE_OFF);
	qscale -= 1;
	if (qscale < QSCALE_MIN)
		qscale = QSCALE_OFF;
	qscale_neg = -qscale;
	//~ vbr_set(); // error 70 if you switch from CBR to VBR while recording
}

void vbr_toggle_reverse( void * priv )
{
	qscale = MIN(qscale, QSCALE_OFF);
	qscale += 1;
	if (qscale > QSCALE_OFF)
		qscale = QSCALE_MIN;
	//~ vbr_set();
}

static void
vbr_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (qscale == QSCALE_OFF)
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"QScale        : OFF (CBR)"
		);
	else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"QScale        : %d (VBR)",
			qscale
		);
}
//-------------------------end qscale--------------

CONFIG_INT("movie.af", movie_af, 0);
CONFIG_INT("movie.af.aggressiveness", movie_af_aggressiveness, 4);
CONFIG_INT("movie.af.noisefilter", movie_af_noisefilter, 7); // 0 ... 9
CONFIG_INT("movie.restart", movie_restart,0);
CONFIG_INT("movie.mode-remap", movie_mode_remap, 0);
int movie_af_stepsize = 10;

PROP_INT(PROP_MVR_REC_START, recording);

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
int shooting_mode;

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
	if (!get_focus_graph()) return 0; // it depends on the graph
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
	shooting_mode = buf[0];
	mode_remap_done = 0;
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

int movie_af_active()
{
	return shooting_mode == SHOOTMODE_MOVIE && lv_drawn() && (af_mode & 0xF) != 3 && (focus_done || movie_af==3);
}

static int hsp = 0;
int movie_af_reverse_dir_request = 0;
PROP_HANDLER(PROP_HALF_SHUTTER)
{
	if (buf[0] && !hsp) movie_af_reverse_dir_request = 1;
	hsp = buf[0];
	hsp_countdown = 15;
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

void set_shooting_mode(int m)
{
	msleep(200);
	prop_request_change(PROP_SHOOTING_MODE, &m, 4);
	msleep(500);
	mode_remap_done = 1;
}

void do_movie_mode_remap()
{
	if (!movie_mode_remap) return;
	if (mode_remap_done) return;
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
}

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

static void lv_test(void* priv)
{
	DispSensorStart();
}

static void turn_off_display(void* priv)
{
	gui_stop_menu();
	msleep(250);
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

static void
max_auto_iso_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Max Auto ISO  : %x", max_auto_iso
	);
}

static void max_auto_iso_toggle(void* priv)
{
	uint16_t newmax = (((max_auto_iso & 0xFF) - 8 ) & 0xFF) | (max_auto_iso & 0xFF00);
	prop_request_change(PROP_MAX_AUTO_ISO, &newmax, 2);
}


static uint32_t* dbg_memmirror = 0;
static uint32_t* dbg_memchanges = 0;

static void dbg_memspy_init() // initial state of the analyzed memory
{
	//~ bmp_printf(FONT_MED, 10,10, "memspy init @ %x ... (+%x) ... %x", mem_spy_start, mem_spy_len, mem_spy_start + mem_spy_len * 4);
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
	//~ bmp_printf(FONT_MED, 10,10, "memspy: %x", crc);
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

void display_info()
{
	bmp_printf(FONT_MED, 20, 400, "Shutter Count: %d", shutter_count);
	bmp_printf(FONT_MED, 20, 420, "CMOS Temperat: %d", efic_temp);
	bmp_printf(FONT_MED, 20, 440, "Lens: %s          ", lens_info.name);
	//~ bmp_printf(FONT_MED, 20, 440, "%d  ", *(int*)0x25334);

}
void display_clock()
{
	int bg = bmp_getpixel(15, 430);
	uint32_t fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);

	struct tm now;
	LoadCalendarFromRTC( &now );
	bmp_printf(fnt, 200, 410, "%02d:%02d", now.tm_hour, now.tm_min);
}


static void dbg_draw_props(int changed);
static unsigned dbg_last_changed_propindex = 0;
int screenshot_sec = 0;
static void
debug_loop_task( void ) // screenshot, draw_prop
{
	do_movie_mode_remap();
	if (!lv_drawn() && ((enable_liveview == 2) || (enable_liveview == 1 && shooting_mode == SHOOTMODE_MOVIE)))
	{
		bmp_printf(FONT_LARGE, 0, 0, "Starting LiveView...");
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
		
		if (!lv_drawn() && gui_state == GUISTATE_IDLE && !gui_menu_shown() && bmp_getpixel(2,10) != 2)
		{
			display_clock();
			display_shooting_info();
		}
		
		if (lv_drawn())
		{
			display_shooting_info_lv();
		}
		
		if (screenshot_sec)
		{
			if (screenshot_sec >= 5) bmp_printf( FONT_SMALL, 0, 0, "Screenshot in %d seconds", screenshot_sec);
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
		
		if (lv_drawn() && shooting_mode == SHOOTMODE_MOVIE && !recording && k % 10 == 0)
		{
			vbr_set();
		}

		if (!DISPLAY_SENSOR_POWERED && gui_state == GUISTATE_IDLE) // force sensor on
		{
			DispSensorStart();
		}

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


struct menu_entry debug_menus[] = {
	{
		.priv		= "Save config",
		.select		= save_config,
		.display	= menu_print,
	},
	{
		.priv = "Turn display off",
		.display	= menu_print, 
		.select = turn_off_display,
	},
	{
		.priv		= "Draw palette",
		.select		= bmp_draw_palette,
		.display	= menu_print,
	},
	{
		.priv		= "Screenshot (10 s)",
		.select		= screenshot_start,
		.select_auto = take_screenshot,
		.display	= menu_print,
	},
	{
		.priv		= "Dump dmlog",
		.select		= (void*) dumpf,
		.display	= menu_print,
	},
	{
		.select		= draw_prop_select,
		.select_reverse = toggle_draw_event,
		.select_auto = mem_spy_select,
		.display	= spy_print,
	},
	/*{
		.priv		= "LV test",
		.select		= lv_test,
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
};

struct menu_entry mov_menus[] = {
	{
		.display	= vbr_print,
		.select		= vbr_toggle,
		.select_reverse	= vbr_toggle_reverse,
	},
	{
		.priv = &movie_restart,
		.display	= movie_restart_print,
		.select		= menu_binary_toggle,
	},
	/*{
		.display	= max_auto_iso_print,
		.select		= max_auto_iso_toggle,
	},*/
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

	DebugMsg( DM_MAGIC, 3, "Prop %08x: %2x: %08x %08x %08x %08x",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0
	);
		
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

#if 1
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

				if( actual_num_properties > num_properties )
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
}

CONFIG_INT( "debug.timed-dump",		timed_dump, 0 );

//~ CONFIG_INT( "debug.dump_prop", dump_prop, 0 );
//~ CONFIG_INT( "debug.dumpaddr", dump_addr, 0 );
//~ CONFIG_INT( "debug.dumplen", dump_len, 0 );

CONFIG_INT( "magic.disable_bootdiskf",	disable_bootdiskf, 0 );

void show_logo()
{
	gui_stop_menu();
	msleep(1000);
	struct bmp_file_t * bmp = bmp_load("B:/logo.bmp");
	int i;
	for (i = 0; i < 100; i++)
	{
		bmp_draw(bmp,0,0,0,0);
		msleep(10);
	}
}


static void
dump_task( void )
{

	//lua_State * L = lua_open();
	//~ show_logo();
	//~ clrscr();
	// Parse our config file
	const char * config_filename = "B:/magic.cfg";
	global_config = config_parse_file( config_filename );
	/*bmp_printf( FONT_MED, 0, 70,
		"Config file %s: %s",
		config_filename,
		global_config ? "YES" : "NO"
	);*/

	qscale = -((int)qscale_neg);

	// It was too early to turn these down in debug_init().
	// Only record important events for the display and face detect
	
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
	DEBUG();
	
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
	debug_loop_task();
}


TASK_CREATE( "dump_task", dump_task, 0, 0x1e, 0x1000 );
//~ TASK_CREATE( "debug_loop_task", debug_loop_task, 0, 0x1f, 0x1000 );

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
