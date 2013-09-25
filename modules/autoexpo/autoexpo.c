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

#define TV2RAW(apex) -APEX_TV(-(apex) * 100 / 125)
#define AV2RAW(apex) -APEX_AV(-(apex) * 100 / 125)
#define SV2RAW(apex) -APEX_SV(-(apex) * 100 / 125)
#define AV2STR(apex) values_aperture[raw2index_aperture(AV2RAW(apex))]

#define GRAPH_XSIZE 2.4f
#define GRAPH_YSIZE 2.3f
#define GRAPH_STEP 5 //divisible by 10
#define GRAPH_XOFF (int)((720 - (ABS(BV_MAX) + ABS(BV_MIN)) * GRAPH_XSIZE) / 2)
#define GRAPH_YOFF 400
#define GRAPH_MAX 130 // APEX value 1/8000
#define GRAPH_MAX_PX (int)(GRAPH_MAX * GRAPH_YSIZE)
#define GRAPH_BG 45
#define GRAPH_FONT SHADOW_FONT(FONT_MED)
#define GRAPH_FONT_H ((int)font_med.height)
#define GRAPH_PADD 2
#define GRAPH_TEXT_PADD 4

#define GRAPH_Y(val) (int)(GRAPH_YOFF - (val) * GRAPH_YSIZE)
#define GRAPH_Y_TEXT(val) (int)(GRAPH_YOFF - COERCE(val * GRAPH_YSIZE + GRAPH_FONT_H - 3, GRAPH_FONT_H, GRAPH_MAX_PX))
#define IS_IN_RANGE(val1, val2) (val1 >= 0 && val1 <= GRAPH_MAX && val2 >=0 && val2 <= GRAPH_MAX)
#define GRAPH_DRAW_CURVE(val, col) \
    if(IS_IN_RANGE(expo.val, last_expo.val)) { \
        draw_line(x_last, GRAPH_Y(last_expo.val), x, GRAPH_Y(expo.val), \
            IS_SEL(val) ? COLOR_WHITE : col); \
    } \

#define MENU_CUSTOM_DRAW(item) \
    if(show_graph && info->can_custom_draw) { \
        entry->parent_menu->scroll_pos = 20;     /* force this entry to be the first one (menu backend will bring it back in range) */ \
        if (entry->selected) { \
            sel_item = ITEM_##item; \
            update_graph(); \
        } \
        else if (entry->prev && entry->prev->selected) { \
            info->custom_drawing = CUSTOM_DRAW_THIS_MENU; /* stop drawing after the selected entry */ \
        } \
    }
#define RANGE_SET(val, min, max) \
    if(!rear_dial()) { \
        val##_min += delta * 5; \
        val##_min = COERCE(val##_min, min, max); \
        val##_max = COERCE(val##_max, val##_min, max); \
    } else { \
        val##_max += delta * 5; \
        val##_max = COERCE(val##_max, min, max); \
        val##_min = COERCE(val##_min, min, val##_max); \
    }
#define CURVE_SET(val, step_min, step_max) \
    if(!rear_dial()) val##_step += delta; \
    else val##_off -= delta * 5; \
    val##_off = COERCE( val##_off, BV_MIN - 100, BV_MAX + 100); \
    val##_step = COERCE( val##_step, step_min, step_max);

#define LENS_AV_THIS 5

#define IS_SEL(val) sel_item == ITEM_##val
#define ITEM_0 0
#define ITEM_ec 1
#define ITEM_tv 2
#define ITEM_av 3
#define ITEM_sv 4
#define ITEM_browse 5

static CONFIG_INT("auto.expo.enabled", auto_expo_enabled, 0);
// these are for fullframe camereas
static CONFIG_INT("auto.expo.same_tv", same_tv, 1);
static CONFIG_INT("auto.expo.lens_av", lens_av, LENS_AV_THIS);
static CONFIG_INT("auto.expo.round_iso", round_iso, 0);
static CONFIG_INT("auto.expo.tv_min", tv_min, 0);  // 1s
static CONFIG_INT("auto.expo.av_min", av_min, 10); // f/1.4
static CONFIG_INT("auto.expo.av_max", av_max, 80); // f/16
static CONFIG_INT("auto.expo.av_step", av_step, 10);
static CONFIG_INT("auto.expo.av_off", av_off, 160);
static CONFIG_INT("auto.expo.iso_min", iso_min, 50);  // ISO 100
static CONFIG_INT("auto.expo.iso_max", iso_max, 120); // ISO 12 800
static CONFIG_INT("auto.expo.iso_step", iso_step, 7);
static CONFIG_INT("auto.expo.iso_off", iso_off, 60);
static CONFIG_INT("auto.expo.ec", ec, 0);  // exposure compensation
static CONFIG_INT("auto.expo.ec_min", ec_min, -15);
static CONFIG_INT("auto.expo.ec_max", ec_max, 20);
static CONFIG_INT("auto.expo.ec_step", ec_step, -3);
static CONFIG_INT("auto.expo.ec_off", ec_off, -15);

static int autoexpo_running = 0;
static bool show_graph = 1;
static int last_key = 0;
static int last_bv = INT_MIN;
static int sel_item = 0;

typedef struct
{
    int ec; // E
    int tv; // T
    int av; // A
    int sv; // S
} exposure;

extern void entry_print(
    int x,
    int y,
    int w,
    struct menu_entry * entry,
    struct menu_display_info * info,
    int in_submenu
);

static exposure get_exposure(int bv, int simul) {
    exposure expo;
    
    //same tv 
    int same_tv_offset = 0;
    if(same_tv) {
        if(simul && lens_av != LENS_AV_THIS) same_tv_offset = MAX(0, lens_av - av_min);
        else same_tv_offset = MAX(0, RAW2AV(lens_info.raw_aperture_min) - av_min);
    }
    
    //av
    expo.av = MAX(av_max + (MIN(bv - av_off, 0) * av_step) / 10, av_min);
    if(simul && lens_av != LENS_AV_THIS) expo.av = MAX(lens_av, expo.av);
    else expo.av = COERCE(expo.av, RAW2AV(lens_info.raw_aperture_min), RAW2AV(lens_info.raw_aperture_max));
    
    //av
    expo.sv  = MIN(iso_min - (MIN(bv - (iso_off + same_tv_offset), 0) * iso_step) / 10, iso_max);
    if(round_iso) { expo.sv /= 10; expo.sv *= 10; }
    
    //ec
    expo.ec = COERCE(ec - (MIN(bv - (ec_off + same_tv_offset), 0) * ec_step) / 10, ec_min, ec_max);
    
    //tv
    expo.tv = COERCE((bv - expo.av + expo.sv) - expo.ec, tv_min, 130);
    
    //ec
    expo.ec = bv - (expo.tv + expo.av - expo.sv);
    
    return expo;
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
    
    exposure expo = get_exposure(bv, 0);
    lens_set_rawaperture(AV2RAW(expo.av));
    lens_set_rawiso(SV2RAW(expo.sv));
    lens_set_rawshutter(TV2RAW(expo.tv));

    cleanup:
    autoexpo_running = 0;
}

static unsigned int autoexpo_shoot_task(){
    if(
        auto_expo_enabled &&
        shooting_mode == SHOOTMODE_M &&
        !lv &&
        get_ae_state() != 0 &&
        !autoexpo_running
    )
        task_create("autoexpo_task", 0x1c, 0x1000, autoexpo_task, (void*)0);

    return 0;
}

static void update_graph()
{
    exposure last_expo;
    bool draw_label = 0;
    
    //bg rect
    bmp_fill(GRAPH_BG, 1,
        GRAPH_YOFF - GRAPH_MAX_PX - GRAPH_PADD,
        720 - 2,
        GRAPH_MAX_PX + GRAPH_TEXT_PADD + GRAPH_PADD + font_med.height
    );
    
    // current BV
    if(last_bv != INT_MIN){
        int x = GRAPH_XOFF + (BV_MAX - last_bv) * GRAPH_XSIZE;
        draw_line(x - 1, GRAPH_YOFF - GRAPH_MAX_PX,
            x - 1, GRAPH_YOFF, IS_SEL(browse) ? COLOR_WHITE : COLOR_CYAN);
        draw_line(x + 1, GRAPH_YOFF - GRAPH_MAX_PX,
            x + 1, GRAPH_YOFF, IS_SEL(browse) ? COLOR_WHITE : COLOR_CYAN);
    }
    
    graph_draw:
    last_expo = (exposure){-1, -1, -1, -1};
    for(int bv = BV_MAX; bv >= BV_MIN; bv -= (draw_label) ? 20 : GRAPH_STEP)
    {
        int x = GRAPH_XOFF + (BV_MAX - bv) * GRAPH_XSIZE;
        
        exposure expo =  get_exposure(bv, 1);
        int ec_val = expo.ec;
        expo.ec = (GRAPH_MAX / 2) + expo.ec;
        
        if(!draw_label) {
            int x_last = x - GRAPH_XSIZE * GRAPH_STEP;
            
            // bg lines
            if(!(bv % 10))draw_line(x, GRAPH_YOFF - GRAPH_MAX_PX, x, GRAPH_YOFF, COLOR_BLACK);
            
            // sv curve
            GRAPH_DRAW_CURVE(sv, COLOR_LIGHT_BLUE);
            
            // av curve
            GRAPH_DRAW_CURVE(av, COLOR_GREEN2);
            
            // ec curve
            GRAPH_DRAW_CURVE(ec,
                (last_expo.ec - (GRAPH_MAX / 2) == 0 && ec_val == 0) ? COLOR_BLACK : COLOR_ORANGE);
            
            // tv curve
            GRAPH_DRAW_CURVE(tv, COLOR_RED);
            
        } else {
            // bv value
            {
                char bv_str[3];
                snprintf(bv_str, sizeof(bv_str), "%d", ABS(bv / 10));
                int center = strlen(bv_str) * font_med.width / 2;
                if(bv < 0) center += font_med.width;
                bmp_printf(GRAPH_FONT, x + 3 - center, GRAPH_YOFF + GRAPH_TEXT_PADD, "%d", bv / 10);
            }
            
            //do not print on the right edge of graph
            if(BV_MAX + bv <= 40) continue;
            
            // sv value
            if(expo.sv != last_expo.sv) {
                bmp_printf(GRAPH_FONT, x, GRAPH_Y_TEXT(expo.sv), "%d", raw2iso(SV2RAW(expo.sv)));
            }
            
            // av value
            if(expo.av != last_expo.av) {
                int ap = AV2STR(expo.av);
                bmp_printf(GRAPH_FONT, x, GRAPH_Y_TEXT(expo.av), "%d.%d", ap / 10, ap % 10);
            }
            
            // ec value
            if(expo.ec != last_expo.ec && ec_val) {
                bmp_printf(GRAPH_FONT, x, GRAPH_Y_TEXT(expo.ec),
                    "%s%d.%d", FMT_FIXEDPOINT1S(ec_val));
            }
            
            // tv value
            if(expo.tv != last_expo.tv) {
                bmp_printf(GRAPH_FONT, x, GRAPH_Y_TEXT(expo.tv),
                    "%s", lens_format_shutter(TV2RAW(expo.tv)));
            }
            
        }
        
        last_expo = expo;
    }
    
    if(!draw_label) {
        draw_label = 1;
        goto graph_draw;
    }
}

static int rear_dial() {
    return last_key == MODULE_KEY_WHEEL_RIGHT || last_key == MODULE_KEY_WHEEL_LEFT;
}
static void set_rear() { last_key = MODULE_KEY_WHEEL_RIGHT; }
static void unset_rear() { last_key = 0; }

// curves
static MENU_SELECT_FUNC(aperture_curve_set) { CURVE_SET(av, 0, 50); }
static MENU_UPDATE_FUNC(aperture_curve_upd) {
    MENU_SET_VALUE("at %s%d.%d -%d.%d EVpBV", FMT_FIXEDPOINT1(av_off), av_step /10, av_step % 10);
    MENU_CUSTOM_DRAW(av);
}

static MENU_SELECT_FUNC(iso_curve_set) { CURVE_SET(iso, 0, 50); }
static MENU_UPDATE_FUNC(iso_curve_upd) {
    MENU_SET_VALUE("at %s%d.%d +%d.%d EVpBV", FMT_FIXEDPOINT1(iso_off), iso_step / 10, iso_step % 10);
    MENU_CUSTOM_DRAW(sv);
}

static MENU_SELECT_FUNC(ec_curve_set) { CURVE_SET(ec, -50, 50); }
static MENU_UPDATE_FUNC(ec_curve_upd) {
    MENU_SET_VALUE("at %s%d.%d %s%d.%d EVpBV", FMT_FIXEDPOINT1(ec_off), FMT_FIXEDPOINT1S(ec_step));
    MENU_CUSTOM_DRAW(ec);
}

//others
static MENU_UPDATE_FUNC(aperture_range_upd) {
    int apmin = AV2STR(av_min);
    int apmax = AV2STR(av_max);
    MENU_SET_VALUE(SYM_F_SLASH"%d.%d - "SYM_F_SLASH"%d.%d", apmin / 10, apmin % 10, apmax / 10, apmax % 10);
    MENU_CUSTOM_DRAW(av);
}

static MENU_SELECT_FUNC(aperture_range_set) {
    int set = av_min;
    RANGE_SET(av, 5, 100);
    
    if(!rear_dial() && same_tv && av_min - set != 0) {
        set_rear();
        iso_curve_set(NULL, delta * -1);
        ec_curve_set(NULL, delta * -1);
    }
}

static MENU_UPDATE_FUNC(lens_av_upd) {
    if(!lens_av) {
        MENU_SET_VALUE("OFF");
        MENU_SET_ENABLED(0);
        MENU_SET_ICON(IT_DICE_OFF, 0);
    }
    else if (lens_av == LENS_AV_THIS) {
        int ap = AV2STR(RAW2AV(lens_info.raw_aperture_min));
        MENU_SET_VALUE("this lens "SYM_F_SLASH"%d.%d", ap / 10, ap % 10);
        MENU_SET_ENABLED(1);
        MENU_SET_ICON(IT_DICE, 0);
    }
    else {
        int ap = AV2STR(lens_av);
        MENU_SET_VALUE(SYM_F_SLASH"%d.%d", ap / 10, ap % 10);
        MENU_SET_ENABLED(1);
        MENU_SET_ICON(IT_DICE, 0);
    }
    MENU_CUSTOM_DRAW(av);
}

static MENU_SELECT_FUNC(lens_av_set) {
    lens_av += delta * 5;
    if(lens_av < 0) lens_av = LENS_AV_THIS;
    lens_av = COERCE(lens_av, 0, 80);
}

static MENU_UPDATE_FUNC(iso_range_upd) {
    MENU_SET_VALUE("%d - %d", raw2iso(SV2RAW(iso_min)), raw2iso(SV2RAW(iso_max)));
    MENU_CUSTOM_DRAW(sv);
}

static MENU_SELECT_FUNC(iso_range_set) {
    int set = iso_min;
    RANGE_SET(iso, 50, 130);
    
    if(!rear_dial() && same_tv && iso_min - set != 0) {
        set_rear();
        aperture_range_set(NULL, delta);
        iso_curve_set(NULL, delta);
    }
}

static MENU_UPDATE_FUNC(round_iso_upd) { MENU_CUSTOM_DRAW(sv); }

static MENU_SELECT_FUNC(tv_min_set) {
    tv_min = COERCE(tv_min + delta * 5, -50, 130);
}

static MENU_UPDATE_FUNC(tv_min_upd) {
    MENU_SET_VALUE("%s", lens_format_shutter(TV2RAW(tv_min)));
    MENU_CUSTOM_DRAW(tv);
}

static MENU_UPDATE_FUNC(same_tv_upd) { MENU_CUSTOM_DRAW(tv); }

static MENU_UPDATE_FUNC(ec_upd) {
    MENU_SET_VALUE("%s%d.%d EV", FMT_FIXEDPOINT1S(ec));
    MENU_CUSTOM_DRAW(ec);
}

static MENU_SELECT_FUNC(ec_sel) {
    int set = ec;
    ec = COERCE(ec + delta * 5, -50, 50);
    
    if(same_tv && ec - set != 0) {
        set_rear();
        aperture_range_set(NULL, delta * -1);
        iso_curve_set(NULL, delta * -1);
        ec_curve_set(NULL, delta * -1);
    }
}

static MENU_UPDATE_FUNC(ec_range_upd) {
    MENU_SET_VALUE("%s%d.%d EV - %s%d.%d EV", FMT_FIXEDPOINT1S(ec_min), FMT_FIXEDPOINT1S(ec_max));
    MENU_CUSTOM_DRAW(ec);
}

static MENU_SELECT_FUNC(ec_range_set) { RANGE_SET(ec, -50, 50); }

static MENU_UPDATE_FUNC(last_bv_upd) {
    if(last_bv != INT_MIN) {
        exposure expo = get_exposure(last_bv, 1);
        expo.av = AV2STR(expo.av);
        
        MENU_SET_VALUE("%s%d.%d BV", FMT_FIXEDPOINT1(last_bv));
        MENU_SET_HELP("%s "SYM_F_SLASH"%d.%d   %d ISO   %s%d.%d EC",
            lens_format_shutter(TV2RAW(expo.tv)),
            expo.av / 10, expo.av % 10,
            raw2iso(SV2RAW(expo.sv)),
            FMT_FIXEDPOINT1S(expo.ec)
        );
    }
    MENU_CUSTOM_DRAW(browse);
}

static MENU_SELECT_FUNC(last_bv_set) {
    if(last_bv == INT_MIN) last_bv = 20;
    else last_bv = COERCE(last_bv + delta * -5, BV_MIN, BV_MAX);
}

static MENU_UPDATE_FUNC(menu_custom_draw_upd) { MENU_CUSTOM_DRAW(0); }

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
        .help2 = "You have to press halfshutter for at least 60ms.",
        .children = (struct menu_entry[]) {
            {
                .name = "Show graph",
                .priv = &show_graph,
                .update = menu_custom_draw_upd,
                .max = 1,
            },
            {
                .name = "TV minimum",
                .priv = &tv_min,
                .update = tv_min_upd,
                .select = tv_min_set,
                .icon_type = IT_DICE,
                .help = "Set minimum shutter value.",
            },
            {
                .name = "Same TV curve",
                .priv = &same_tv,
                .update = same_tv_upd,
                .max = 1,
                .help = "Auto compensate min ISO & min AV & EC changes.",
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
                .name = "Lens AV",
                .update = lens_av_upd,
                .select = lens_av_set,
                .icon_type = IT_DICE,
                .help = "Simulate graph for specific lens aperture.",
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
                .name = "Round ISO",
                .priv = &round_iso,
                .update = round_iso_upd,
                .max = 1,
                .help = "Stop using digital ISO - 100, 200, 400, 800, etc.",
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
                .name = "Browse",
                .update = last_bv_upd,
                .select = last_bv_set,
                .icon_type = IT_DICE,
            },
            MENU_EOL,
        }
    }
};

static unsigned int autoexpo_init()
{
    menu_add("Expo", autoexpo_menu, COUNT(autoexpo_menu));
    return 0;
}

static unsigned int autoexpo_deinit()
{
    return 0;
}

static unsigned int autoexpo_keypress(unsigned int key)
{
    if(gui_menu_shown()) last_key = key;
    return 1;
}

MODULE_INFO_START()
    MODULE_INIT(autoexpo_init)
    MODULE_DEINIT(autoexpo_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, autoexpo_shoot_task, 0)
    MODULE_CBR(CBR_KEYPRESS, autoexpo_keypress, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(auto_expo_enabled)
    MODULE_CONFIG(same_tv)
    MODULE_CONFIG(lens_av)
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
