/*

    BRIGHTNESS VALUE = SHUTTER + APERTURE - ISO + EXPOSURE COMPENSATION 
    in M mode exposure compensation is indicator of under/over exposure
    
    http://www.magiclantern.fm/forum/index.php?topic=7208
    https://bitbucket.org/pravdomil/

    AE_VALUE value overflows on 5D2 Canon bug
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
#define RAW2EC(raw) raw * 10 / 8

#define TV2RAW(apex) -APEX_TV(-apex * 100 / 125)
#define AV2RAW(apex) -APEX_AV(-apex * 100 / 125)
#define SV2RAW(apex) -APEX_SV(-apex * 100 / 125)
#define AV2STR(apex) values_aperture[raw2index_aperture(AV2RAW(apex))]

#define GRAPH_XSIZE 2.4f
#define GRAPH_YSIZE 2.3f
#define GRAPH_STEP 2 //divisible by 20
#define GRAPH_XOFF (int)((720 - (ABS(BV_MAX) + ABS(BV_MIN)) * GRAPH_XSIZE) / 2)
#define GRAPH_YOFF 400
#define GRAPH_MAX 130 /* APEX value 1/8000 */
#define GRAPH_MAX_PX (int)(GRAPH_MAX * GRAPH_YSIZE)
#define GRAPH_BG 45
#define GRAPH_PADD 2
#define GRAPH_TEXT_PADD 4

#define IS_IN_RANGE(val1, val2) (val1 >= 0 && val1 <= GRAPH_MAX && val2 >=0 && val2 <= GRAPH_MAX)
#define GRAPH_Y(val) (int)(GRAPH_YOFF - (val) * GRAPH_YSIZE)
#define GRAPH_Y_TEXT (int)(GRAPH_YOFF - MAX(5, next * GRAPH_YSIZE))

#define MENU_CUSTOM_DRAW \
    if(show_graph) { \
        info->custom_drawing = CUSTOM_DRAW_THIS_ENTRY; \
        if(entry->selected)entry_print(info->x, 60, 20, entry, info, 0); \
    }


static CONFIG_INT("auto.expo.enabled", auto_expo_enabled, 0);
/* these are for fullframe camereas */
static CONFIG_INT("auto.expo.lock_iso", lock_iso, 1);
static CONFIG_INT("auto.expo.round_iso", round_iso, 0);
static CONFIG_INT("auto.expo.tv_min", tv_min, 0);  /* 1s */
static CONFIG_INT("auto.expo.av_min", av_min, 10); /* f/1.4 */
static CONFIG_INT("auto.expo.av_max", av_max, 75); /* f/13.4 */
static CONFIG_INT("auto.expo.av_step", av_step, 10);
static CONFIG_INT("auto.expo.av_off", av_off, 160);
static CONFIG_INT("auto.expo.iso_min", iso_min, 50);  /* ISO 100 */
static CONFIG_INT("auto.expo.iso_max", iso_max, 120); /* ISO 12 800 */
static CONFIG_INT("auto.expo.iso_step", iso_step, 7);
static CONFIG_INT("auto.expo.iso_off", iso_off, 60);
static CONFIG_INT("auto.expo.ec", ec, 5);  /* exposure compensation */
static CONFIG_INT("auto.expo.ec_min", ec_min, -15);
static CONFIG_INT("auto.expo.ec_max", ec_max, 20);
static CONFIG_INT("auto.expo.ec_step", ec_step, -3);
static CONFIG_INT("auto.expo.ec_off", ec_off, -15);

static int autoexpo_running = 0;
static bool show_graph = 1;
static int last_key = 0;
static int last_bv = INT_MIN;

extern int advanced_mode;

extern void entry_print(
    int x,
    int y,
    int w,
    struct menu_entry * entry,
    struct menu_display_info * info,
    int in_submenu
);


static int get_ec(int bv){
    int offset = ec_off;
    if(lock_iso){
        int valid = RAW2AV(lens_info.raw_aperture_min) - av_min;
        if(valid > 0) offset += valid;
    }
    return COERCE(ec - (MIN(bv - offset, 0) * ec_step) / 10, ec_min, ec_max);
}

static int get_shutter_from_bv(int bv, int aperture, int iso){
    return COERCE((bv - aperture + iso) - get_ec(bv), tv_min, 130);
}

static int get_aperture_from_bv(int bv, int limit){
    int ap = MAX(av_max + (MIN(bv - av_off, 0) * av_step) / 10, av_min);
    if(lock_iso || limit){
        ap = COERCE(ap, RAW2AV(lens_info.raw_aperture_min), RAW2AV(lens_info.raw_aperture_max));
    }
    return ap;
}

static int get_iso_from_bv(int bv){
    int offset = iso_off;
    if(lock_iso){
        int valid = RAW2AV(lens_info.raw_aperture_min) - av_min;
        if(valid > 0) offset += valid;
    }
    int iso = MIN(iso_min - (MIN(bv - offset, 0) * iso_step) / 10, iso_max);
    if(round_iso){
        iso /= 10;
        iso *= 10;
    }
    return iso;
}

static void autoexpo_task()
{
    autoexpo_running = 1;

    if(!lens_info.raw_shutter) goto cleanup; //open menus

    int bv = RAW2TV(lens_info.raw_shutter) + RAW2AV(lens_info.raw_aperture) - RAW2SV(lens_info.iso_equiv_raw) + RAW2EC(get_ae_value());
    
    if(bv < -200){ //AE_VALUE overflows, set some low values
        lens_set_rawshutter(60 + 56);
        lens_set_rawaperture(1);
        lens_set_rawiso(1);
        NotifyBox(1000, "AE_VALUE overflows");
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
        get_ae_value() != 0 &&
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
    int last_ec = 0;
    int last_tv_odd = 0;
    int last_av_odd = 0;
    int last_iso_odd = 0;
    int last_ec_odd = 0;
    int next = 0;
    int sfont = FONT(FONT_SMALL, COLOR_WHITE, GRAPH_BG);

    BMP_LOCK(
        bmp_fill(GRAPH_BG, 1,
            GRAPH_YOFF - GRAPH_MAX_PX - GRAPH_PADD,
            720 - 2,
            GRAPH_MAX_PX + GRAPH_TEXT_PADD + GRAPH_PADD + font_med.height
        );
        
        for(int bv = BV_MAX; bv >= BV_MIN; bv-=GRAPH_STEP){
            int x = GRAPH_XOFF + (BV_MAX - bv) * GRAPH_XSIZE;
            int x_last = x - GRAPH_XSIZE * GRAPH_STEP;
            bool odd = !(bv % 20) && bv != BV_MIN;

            //bv
            if(odd){
                char bv_str[4];
                snprintf(bv_str, sizeof(bv_str), "%d", bv / 10);
                bmp_printf(FONT(FONT_MED, COLOR_WHITE, GRAPH_BG),
                    x + 3 - strlen(bv_str) * font_med.width / 2, GRAPH_YOFF + GRAPH_TEXT_PADD, bv_str);
            }

            //av
            next = get_aperture_from_bv(bv, 0);
            if(IS_IN_RANGE(next, last_av)){
                if(bv != BV_MAX){
                    draw_line(x_last, GRAPH_Y(last_av), x, GRAPH_Y(next), COLOR_GREEN2);
                }
                if(odd && next != last_av_odd){
                    int ap = AV2STR(next);
                    bmp_printf(sfont, x + 2, GRAPH_Y_TEXT, "%d.%d", ap / 10, ap % 10);
                    last_av_odd = next;
                }
            }
            last_av = next;

            //sv
            next = get_iso_from_bv(bv);
            if(IS_IN_RANGE(next, last_iso)){
                if(bv != BV_MAX){
                    draw_line(x_last, GRAPH_Y(last_iso), x, GRAPH_Y(next), COLOR_LIGHT_BLUE);
                }
                if(odd && next != last_iso_odd){
                    bmp_printf(sfont, x + 2, GRAPH_Y_TEXT, "%d", raw2iso(SV2RAW(next)));
                    last_iso_odd = next;
                }
            }
            last_iso = next;

            //tv
            next = get_shutter_from_bv(bv, last_av, last_iso);
            if(IS_IN_RANGE(next, last_tv)){
                if(bv != BV_MAX){
                    draw_line(x_last, GRAPH_Y(last_tv), x, GRAPH_Y(next), COLOR_RED);
                }
                if(odd && next != last_tv_odd){
                    bmp_printf(sfont, x + 2, GRAPH_Y_TEXT, "%s", lens_format_shutter(TV2RAW(next)));
                    last_tv_odd = next;
                }
            }
            last_tv = next;
            
            //ec
            int ec_val = bv - (last_tv + last_av - last_iso);
            next = (GRAPH_MAX / 2) + ec_val;
            if(IS_IN_RANGE(next, last_ec)){
                if(bv != BV_MAX){
                    draw_line(x_last, GRAPH_Y(last_ec), x, GRAPH_Y(next), ec_val == 0 ? COLOR_BLACK : COLOR_ORANGE);
                }
                if(odd && next != last_ec_odd){
                    if(ec_val)bmp_printf(sfont, x + 2, GRAPH_Y_TEXT, "%s%d.%d", FMT_FIXEDPOINT1S(ec_val));
                    last_ec_odd = next;
                }
            }
            last_ec = next;

            //lines
            if(!(bv % 10))draw_line(x, GRAPH_YOFF - GRAPH_MAX_PX, x, GRAPH_YOFF, COLOR_BLACK);
        }
        if(last_bv != INT_MIN){
            int x = GRAPH_XOFF + (BV_MAX - last_bv) * GRAPH_XSIZE;
            draw_line(x - 1, GRAPH_YOFF - GRAPH_MAX_PX, x - 1, GRAPH_YOFF, COLOR_CYAN);
            draw_line(x + 1, GRAPH_YOFF - GRAPH_MAX_PX, x + 1, GRAPH_YOFF, COLOR_CYAN);
        }
    )
}

static int last_delta(){
    if(last_key == MODULE_KEY_WHEEL_DOWN || last_key ==  MODULE_KEY_PRESS_RIGHT) return 1;
    if(last_key == MODULE_KEY_WHEEL_RIGHT ) return 5;
    if(last_key == MODULE_KEY_WHEEL_UP || last_key == MODULE_KEY_PRESS_LEFT) return -1;
    if(last_key == MODULE_KEY_WHEEL_LEFT ) return -5;
    return 0;
}

static MENU_UPDATE_FUNC(aperture_range_upd)
{
    int apmin = AV2STR(av_min);
    int apmax = AV2STR(av_max);
    MENU_SET_VALUE("f/%d.%d - f/%d.%d", apmin / 10, apmin % 10, apmax / 10, apmax % 10);
    MENU_CUSTOM_DRAW;
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
    MENU_CUSTOM_DRAW;
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
    MENU_CUSTOM_DRAW;
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
    MENU_CUSTOM_DRAW;
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
    MENU_CUSTOM_DRAW;
}

static MENU_UPDATE_FUNC(ec_upd)
{
    MENU_SET_VALUE("%s%d.%d EV", FMT_FIXEDPOINT1S(ec));
    MENU_CUSTOM_DRAW;
}

static MENU_SELECT_FUNC(ec_sel)
{
    ec += delta;
}

static MENU_UPDATE_FUNC(ec_range_upd)
{
    MENU_SET_VALUE("%s%d.%d EV - %s%d.%d EV", FMT_FIXEDPOINT1S(ec_min), FMT_FIXEDPOINT1S(ec_max));
    MENU_CUSTOM_DRAW;
}

static MENU_SELECT_FUNC(ec_range_set)
{
    int last_d = last_delta();
    
    if(ABS(last_d) == 1) ec_min += last_d;
    else if(ABS(last_d) == 5) ec_max += last_d / 5;
    
    ec_min = MIN(ec_min, ec_max);
    ec_max = MAX(ec_min, ec_max);
}

static MENU_UPDATE_FUNC(ec_curve_upd)
{
    MENU_SET_VALUE("at %s%d.%d %s%d.%d EVpBV", FMT_FIXEDPOINT1(ec_off), FMT_FIXEDPOINT1S(ec_step));
    MENU_CUSTOM_DRAW;
}

static MENU_SELECT_FUNC(ec_curve_set)
{
    int last_d = last_delta();

    if(ABS(last_d) == 1) ec_step += last_d;
    else if(ABS(last_d) == 5) ec_off -= last_d;

    ec_off = COERCE(ec_off, BV_MIN - 100, BV_MAX + 100);
    ec_step = COERCE(ec_step, -50, 50);
}

static MENU_UPDATE_FUNC(show_graph_upd)
{
    if(show_graph)update_graph();
    MENU_CUSTOM_DRAW;
}

static MENU_UPDATE_FUNC(menu_custom_draw_upd)
{
    MENU_CUSTOM_DRAW;
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
                .help = "Use main dial for minimum and left & right for maximum.",
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
                .help = "Use main dial for minimum and left & right for maximum.",
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
                .name = "EC",
                .update = ec_upd,
                .select = ec_sel,
                .icon_type = IT_DICE,
                .help = "Exposure compensation.",
            },
            {
                .name = "EC range",
                .update = ec_range_upd,
                .select = ec_range_set,
                .icon_type = IT_DICE,
                .help = "Use main dial for minimum and left & right for maximum.",
            },
            {
                .name = "EC curve",
                .update = ec_curve_upd,
                .select = ec_curve_set,
                .icon_type = IT_DICE,
                .help = "Main dial - change curve offset.",
                .help2 = "Left & right - set EV steps per BV.",
            },
            {
                .name = "Lock ISO AV EC",
                .priv = &lock_iso,
                .update = menu_custom_draw_upd,
                .max = 1,
                .help = "Move ISO & EC left if min aperture is not in valid range.",
                .help2 = "To get same shutter curve.",
            },
            {
                .name = "Round ISO",
                .priv = &round_iso,
                .update = menu_custom_draw_upd,
                .max = 1,
                .help = "Stop using digital ISO - 100, 200, 400, 800, etc.",
            },
            {
                .name = "Show graph",
                .priv = &show_graph,
                .update = show_graph_upd,
                .max = 1,
            },
            MENU_EOL,
        }
    }
};

unsigned int autoexpo_init()
{
    menu_add("Expo", autoexpo_menu, COUNT(autoexpo_menu));
    return 0;
}

unsigned int autoexpo_deinit()
{
    return 0;
}

unsigned int autoexpo_keypress(unsigned int key)
{
    if(gui_menu_shown()) last_key = key;
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
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, autoexpo_shoot_task, 0)
    MODULE_CBR(CBR_KEYPRESS, autoexpo_keypress, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(auto_expo_enabled)
    MODULE_CONFIG(lock_iso)
    MODULE_CONFIG(round_iso)
    MODULE_CONFIG(tv_min)
    MODULE_CONFIG(av_min)
    MODULE_CONFIG(av_max)
    MODULE_CONFIG(av_step)
    MODULE_CONFIG(av_off)
    MODULE_CONFIG(iso_min)
    MODULE_CONFIG(iso_max)
    MODULE_CONFIG(iso_step)
    MODULE_CONFIG(iso_off)
    MODULE_CONFIG(ec)
    MODULE_CONFIG(ec_min)
    MODULE_CONFIG(ec_max)
    MODULE_CONFIG(ec_step)
    MODULE_CONFIG(ec_off)
MODULE_CONFIGS_END()
