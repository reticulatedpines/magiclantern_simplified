/*

	BRIGHTNESS VALUE = SHUTTER + APERTURE - ISO + EXPOSURE COMPENSATION 
	in M mode EXPO_COMP is indicator of under/over exposure

	http://www.magiclantern.fm/forum/index.php?topic=7208
	https://bitbucket.org/pravdomil/magic-lantern-hack

	###EXPO_COMP only set on 5D2 v 212###
	use ae_spy module to find it on other cameras
	https://bitbucket.org/pravdomil/magic-lantern-hack/commits/2e2ad5262575971a5d29cc8418854c31f9cd0d72
	or send me a camera I will find it!

	EXPO_COMP value overflows its Canon bug
	how to reproduce: set high iso and watch AE compension in M on bright sky

*/
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <menu.h>
#include <lens.h>

#define BV_MAX 160
#define BV_MIN -120

#define RAW2TV(raw) APEX_TV(raw) * 10 / 8
#define RAW2AV(raw) APEX_AV(raw) * 10 / 8
#define RAW2SV(raw) APEX_SV(raw) * 10 / 8
#define RAW2EC(raw) (raw - 1) * 10 / 2048

#define TV2RAW(apex) -APEX_TV(-apex * 100 / 125)
#define AV2RAW(apex) -APEX_AV(-apex * 100 / 125)
#define SV2RAW(apex) -APEX_SV(-apex * 100 / 125)
#define AV2STR(apex) values_aperture[raw2index_aperture(AV2RAW(apex))]

#define GRAPH_XSIZE 2.4f
#define GRAPH_YSIZE 1 //doesnt like floats
#define GRAPH_STEP 5
#define GRAPH_XOFF (720 - (ABS(BV_MAX) + ABS(BV_MIN)) * GRAPH_XSIZE) / 2
#define GRAPH_YOFF 400
#define GRAPH_MAX GRAPH_YSIZE * 130
#define GRAPH_BG 45

#define IS_IN_RANGE(val1, val2) (val1 >= 0 && val1 <= GRAPH_MAX && val2 >=0 && val2 <= GRAPH_MAX)

static CONFIG_INT("auto.expo.enabled", auto_expo_enabled, 0);
static CONFIG_INT("auto.expo.lock_iso", lock_iso, 0);
static CONFIG_INT("auto.expo.tv_min", tv_min, 0);
static CONFIG_INT("auto.expo.av_min", av_min, 10);
static CONFIG_INT("auto.expo.av_max", av_max, 100);
static CONFIG_INT("auto.expo.av_step", av_step, 10);
static CONFIG_INT("auto.expo.av_off", av_off, 150);
static CONFIG_INT("auto.expo.iso_min", iso_min, 50);
static CONFIG_INT("auto.expo.iso_max", iso_max, 100);
static CONFIG_INT("auto.expo.iso_step", iso_step, 5);
static CONFIG_INT("auto.expo.iso_off", iso_off, 60);

static int autoexpo_running = 0;
static int last_key = 0;
static int last_bv = INT_MIN;

int get_shutter_from_bv(int bv, int aperture, int iso){
	return COERCE(bv - aperture + iso, tv_min, 130);
}

int get_aperture_from_bv(int bv, int limit){
	int ap = MAX(av_max + (MIN(bv - av_off, 0) * av_step) / 10, av_min);
	if(lock_iso || limit)
		ap = COERCE(ap, RAW2AV(lens_info.raw_aperture_min), RAW2AV(lens_info.raw_aperture_max));
	return ap;
}

int get_iso_from_bv(int bv){
	int offset = iso_off;
	if(lock_iso){
		int valid = RAW2AV(lens_info.raw_aperture_min) - av_min;
		if(valid > 0) offset += valid;
	}
	return MIN(iso_min - (MIN(bv - offset, 0) * iso_step) / 10, iso_max);
}

static void autoexpo_task()
{
	autoexpo_running = 1;

	if(!lens_info.raw_shutter) goto cleanup; //open menus

	int bv = RAW2TV(lens_info.raw_shutter) + RAW2AV(lens_info.raw_aperture) - RAW2SV(lens_info.iso_equiv_raw) + RAW2EC(EXPO_COMP);

	if(bv < -200){ //EXPO_COMP overflows, set some low values
		lens_set_rawshutter(60 + 56);
		lens_set_rawaperture(1);
		lens_set_rawiso(1);
		NotifyBox(1000, "EXPO_COMP overflows");
		goto cleanup;
	}

	last_bv = bv;

	int av = get_aperture_from_bv(bv, 1);
	lens_set_rawaperture(AV2RAW(av));

	int iso = get_iso_from_bv(bv);
	lens_set_rawiso(SV2RAW(iso));

	int tv = get_shutter_from_bv(bv, av, iso);
	lens_set_rawshutter(TV2RAW(tv));

	cleanup:
	autoexpo_running = 0;
}

static unsigned int autoexpo_shoot_task(){
	if(
		auto_expo_enabled &&
		shooting_mode == SHOOTMODE_M &&
		!lv &&
		EXPO_COMP != 0 &&
		!autoexpo_running
	)
		task_create("autoexpo_task", 0x1c, 0x1000, autoexpo_task, (void*)0);

	return 0;
}

static void update_graph()
{
	int last_tv = 0;
	int last_av = 0;
	int last_iso = 0;
	int last_tv_odd = 0;
	int last_av_odd = 0;
	int last_iso_odd = 0;
	int next = 0;
	int sfont = FONT(FONT_SMALL, COLOR_WHITE, GRAPH_BG);

	BMP_LOCK(
		bmp_fill(GRAPH_BG, 1, GRAPH_YOFF - GRAPH_MAX - 2, 720 - 2, GRAPH_MAX + 2 + 4 + font_med.height);

		for(int bv = BV_MAX; bv >= BV_MIN; bv-=GRAPH_STEP){
			int x = GRAPH_XOFF + (BV_MAX - bv) * GRAPH_XSIZE;
			int x_last = x - GRAPH_XSIZE * GRAPH_STEP;
			int odd = !((BV_MAX - bv) % (GRAPH_STEP * 4));

			//bv
			if(odd){
				char bv_str[4];
				snprintf(bv_str, sizeof(bv_str), "%d", bv / 10);
				bmp_printf(FONT(FONT_MED, COLOR_WHITE, GRAPH_BG),
					x + 3 - strlen(bv_str) * font_med.width / 2, GRAPH_YOFF + 4, bv_str);
			}

			//av
			next = get_aperture_from_bv(bv, 0);
			if(IS_IN_RANGE(next, last_av)){
				if(bv != BV_MAX)
					draw_line(x_last, GRAPH_YOFF - last_av * GRAPH_YSIZE, x, GRAPH_YOFF - next * GRAPH_YSIZE, COLOR_GREEN2);
				if(odd && next != last_av_odd){
					int ap = AV2STR(next);
					bmp_printf(sfont, x + 2, GRAPH_YOFF - MAX(5, next * GRAPH_YSIZE), "%d.%d", ap / 10, ap % 10);
					last_av_odd = next;
				}
			}
			last_av = next;

			//sv
			next = get_iso_from_bv(bv);
			if(IS_IN_RANGE(next, last_iso)){
				if(bv != BV_MAX)
					draw_line(x_last, GRAPH_YOFF - last_iso * GRAPH_YSIZE, x, GRAPH_YOFF - next * GRAPH_YSIZE, COLOR_LIGHT_BLUE);
				if(odd && next != last_iso_odd){
					bmp_printf(sfont, x + 2, GRAPH_YOFF - MAX(5, next * GRAPH_YSIZE), "%d", raw2iso(SV2RAW(next)));
					last_iso_odd = next;
				}
			}
			last_iso = next;

			//tv
			next = get_shutter_from_bv(bv, last_av, last_iso);
			if(IS_IN_RANGE(next, last_tv)){
				if(bv != BV_MAX)
					draw_line(x_last, GRAPH_YOFF - last_tv * GRAPH_YSIZE, x, GRAPH_YOFF - next * GRAPH_YSIZE, COLOR_RED);
				if(odd && next != last_tv_odd){
					bmp_printf(sfont, x + 2, GRAPH_YOFF - MAX(5, next * GRAPH_YSIZE), "%s", lens_format_shutter(TV2RAW(next)));
					last_tv_odd = next;
				}
			}
			last_tv = next;

			//lines
			if(!(bv % 10))
				draw_line(x, GRAPH_YOFF - GRAPH_MAX, x, GRAPH_YOFF, 
					(last_tv + last_av - last_iso != bv) ? COLOR_ORANGE : COLOR_BLACK);
			
		}
		if(last_bv != INT_MIN){
			int x = GRAPH_XOFF + (BV_MAX - last_bv) * GRAPH_XSIZE;
			draw_line(x - 1, GRAPH_YOFF - GRAPH_MAX, x - 1, GRAPH_YOFF, COLOR_CYAN);
			draw_line(x + 1, GRAPH_YOFF - GRAPH_MAX, x + 1, GRAPH_YOFF, COLOR_CYAN);
		}
	)
}

static int last_delta(){
	if(last_key == 6 || last_key == 15) return 1;
	if(last_key == 8 ) return 5;
	if(last_key == 5 || last_key == 16) return -1;
	if(last_key == 7 ) return -5;
	return 0;
}

static MENU_UPDATE_FUNC(aperture_range_upd)
{
	int apmin = AV2STR(av_min);
	int apmax = AV2STR(av_max);
	MENU_SET_VALUE("f/%d.%d - f/%d.%d", apmin / 10, apmin % 10, apmax / 10, apmax % 10);
}

static MENU_SELECT_FUNC(aperture_range_set)
{
	int last_d = last_delta();

	if(ABS(last_d) == 1) av_min += last_d * 5;
	else if(ABS(last_d) == 5) av_max += last_d;

	av_min = COERCE(av_min, 5, 100);
	av_max = COERCE(av_max, av_min, 100);
}

static MENU_UPDATE_FUNC(aperture_curve_upd)
{
	MENU_SET_VALUE("at %s%d.%d -%d.%d EVpBV", FMT_FIXEDPOINT1(av_off), av_step /10, av_step % 10);
}

static MENU_SELECT_FUNC(aperture_curve_set)
{
	int last_d = last_delta();

	if(ABS(last_d) == 1) av_step += last_d;
	else if(ABS(last_d) == 5) av_off -= last_d;

	av_off = COERCE(av_off, BV_MIN - 100, BV_MAX + 100);
	av_step = COERCE(av_step, 0, 50);
}

static MENU_UPDATE_FUNC(iso_range_upd)
{
	MENU_SET_VALUE("%d - %d", raw2iso(SV2RAW(iso_min)), raw2iso(SV2RAW(iso_max)));
}

static MENU_SELECT_FUNC(iso_range_set)
{
	int last_d = last_delta();

	if(ABS(last_d) == 1) iso_min += last_d * 5;
	else if(ABS(last_d) == 5) iso_max += last_d;

	iso_min = COERCE(iso_min, 50, 130);
	iso_max = COERCE(iso_max, iso_min, 130);
}

static MENU_UPDATE_FUNC(iso_curve_upd)
{
	MENU_SET_VALUE("at %s%d.%d +%d.%d EVpBV", FMT_FIXEDPOINT1(iso_off), iso_step / 10, iso_step % 10);
}

static MENU_SELECT_FUNC(iso_curve_set)
{
	int last_d = last_delta();

	if(ABS(last_d) == 1) iso_step += last_d;
	else if(ABS(last_d) == 5) iso_off -= last_d;

	iso_off = COERCE(iso_off, BV_MIN - 100, BV_MAX + 100);
	iso_step = COERCE(iso_step, 0, 50);
}

static MENU_SELECT_FUNC(tv_min_set)
{
	tv_min += delta * 5;
	tv_min = COERCE(tv_min, -50, 130);
}

static MENU_UPDATE_FUNC(tv_min_upd){
	MENU_SET_VALUE("%s", lens_format_shutter(TV2RAW(tv_min)));
	update_graph();
}

static struct menu_entry autoexpo_menu[] =
{
	{
		.name = "Auto exposure",
		.priv = &auto_expo_enabled,
		.max = 1,
		.depends_on = DEP_M_MODE | DEP_NOT_LIVEVIEW,
		.submenu_height = 410,
		.submenu_width = 720,
		.help = "Automatic exposure algorithm based on predefined curves.",
		.help2 = "Approximately 500ms response + 60ms to set new exposure.",
		.children = (struct menu_entry[]) {
			{
				.name = "TV minimum",
				.priv = &tv_min,
				.update = tv_min_upd,
				.select = tv_min_set,
				.icon_type = IT_DICE,
				.help = "Set minimum shutter value.",
			},
			{
				.name = "AV range",
				.update = aperture_range_upd,
				.select = aperture_range_set,
				.icon_type = IT_DICE,
				.help = "Use main dial for maximum and left & right for minimum.",
			},
			{
				.name = "AV curve",
				.update = aperture_curve_upd,
				.select = aperture_curve_set,
				.icon_type = IT_DICE,
				.help = "Main dial - change curve offset.",
				.help2 = "Left & right - set EV steps per BV.",
			},
			{
				.name = "ISO range",
				.update = iso_range_upd,
				.select = iso_range_set,
				.icon_type = IT_DICE,
				.help = "Use main dial for maximum and left & right for minimum.",
				.help2 = "Make sure that it correspond to your extended ISO settings.",
			},
			{
				.name = "ISO curve",
				.update = iso_curve_upd,
				.select = iso_curve_set,
				.icon_type = IT_DICE,
				.help = "Main dial - change curve offset.",
				.help2 = "Left & right - set EV steps per BV.",
			},
			{
				.name = "Lock ISO & AV",
				.priv = &lock_iso,
				.max = 1,
				.help = "Move ISO curve left if min aperture is not in valid range.",
				.help2 = "To get same shutter curve.",
			},
			MENU_EOL,
		}
	}
};

unsigned int autoexpo_init()
{
	if(streq(camera_model_short, "5D2"))
		menu_add("Expo", autoexpo_menu, COUNT(autoexpo_menu));
	return 0;
}

unsigned int autoexpo_deinit()
{
	return 0;
}

unsigned int autoexpo_keypress(unsigned int key)
{
	if(gui_menu_shown())
		last_key = key;
	return 1;
}

MODULE_INFO_START()
	MODULE_INIT(autoexpo_init)
	MODULE_DEINIT(autoexpo_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
	MODULE_STRING("Desc", "Auto exposure algorithm")
	MODULE_STRING("Author", "Pravdomil.cz")
	MODULE_STRING("Credits", "ML dev.")
	MODULE_STRING("5D2", "only")
MODULE_STRINGS_END()

MODULE_CBRS_START()
	MODULE_CBR(CBR_SHOOT_TASK, autoexpo_shoot_task, 0)
	MODULE_CBR(CBR_KEYPRESS, autoexpo_keypress, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
	MODULE_CONFIG(auto_expo_enabled)
	MODULE_CONFIG(lock_iso)
	MODULE_CONFIG(tv_min)
	MODULE_CONFIG(av_min)
	MODULE_CONFIG(av_max)
	MODULE_CONFIG(av_step)
	MODULE_CONFIG(av_off)
	MODULE_CONFIG(iso_min)
	MODULE_CONFIG(iso_max)
	MODULE_CONFIG(iso_step)
	MODULE_CONFIG(iso_off)
MODULE_CONFIGS_END()
