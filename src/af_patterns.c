#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "af_patterns.h"
#include "lens.h"

static CONFIG_INT("focus.patterns", af_patterns, 0);

static type_PATTERN_MAP_ITEM pattern_map[] = {
        {AF_PATTERN_CENTER,         AF_PATTERN_SQUARE, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},
        {AF_PATTERN_SQUARE,         AF_PATTERN_HLINE,  AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

        {AF_PATTERN_TOP,            AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_TOPLEFT,       AF_PATTERN_TOPRIGHT},
        {AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_TOPDIAMOND,     AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_RIGHTTRIANGLE},
        {AF_PATTERN_TOPDIAMOND,     AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_TOPHALF,        AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_RIGHTDIAMOND},
        {AF_PATTERN_TOPHALF,        AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_HLINE,          AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

        {AF_PATTERN_BOTTOM,         AF_PATTERN_CENTER, AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_BOTTOM,         AF_PATTERN_BOTTOMLEFT,    AF_PATTERN_BOTTOMRIGHT},
        {AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_CENTER, AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_BOTTOM,         AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_RIGHTTRIANGLE},
        {AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_CENTER, AF_PATTERN_BOTTOMHALF,     AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_RIGHTDIAMOND},
        {AF_PATTERN_BOTTOMHALF,     AF_PATTERN_CENTER, AF_PATTERN_HLINE,          AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

        {AF_PATTERN_TOPLEFT,        AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_LEFT,          AF_PATTERN_TOPRIGHT},
        {AF_PATTERN_TOPRIGHT,       AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_TOPLEFT,       AF_PATTERN_RIGHT},
        {AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_CENTER, AF_PATTERN_TOPLEFT,        AF_PATTERN_BOTTOM,         AF_PATTERN_LEFT,          AF_PATTERN_BOTTOMRIGHT},
        {AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_CENTER, AF_PATTERN_TOPRIGHT,       AF_PATTERN_BOTTOM,         AF_PATTERN_BOTTOMLEFT,    AF_PATTERN_RIGHT},

        {AF_PATTERN_LEFT,           AF_PATTERN_CENTER, AF_PATTERN_TOPLEFT,        AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_LEFT,          AF_PATTERN_LEFTTRIANGLE},
        {AF_PATTERN_LEFTTRIANGLE,   AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_LEFT,          AF_PATTERN_LEFTDIAMOND},
        {AF_PATTERN_LEFTDIAMOND,    AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_LEFTHALF},
        {AF_PATTERN_LEFTHALF,       AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_VLINE},

        {AF_PATTERN_RIGHT,          AF_PATTERN_CENTER, AF_PATTERN_TOPRIGHT,       AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_RIGHTTRIANGLE, AF_PATTERN_RIGHT},
        {AF_PATTERN_RIGHTTRIANGLE,  AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_RIGHTDIAMOND,  AF_PATTERN_RIGHT},
        {AF_PATTERN_RIGHTDIAMOND,   AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_RIGHTHALF,     AF_PATTERN_RIGHTTRIANGLE},
        {AF_PATTERN_RIGHTHALF,      AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_VLINE,         AF_PATTERN_RIGHTDIAMOND},

        {AF_PATTERN_HLINE,          AF_PATTERN_VLINE,  AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},
        {AF_PATTERN_VLINE,          AF_PATTERN_ALL,    AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

        {AF_PATTERN_ALL,            AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

        END_OF_LIST
};

static int afp_transformer (int pattern, type_DIRECTION direction);

static int afp[2];
static int afp_len = 0;
PROP_HANDLER(PROP_AFPOINT)
{
    afp[0] = buf[0];
    afp[1] = buf[1];
    afp_len = len;
}
#define af_point afp[0]

static void afp_show_in_viewfinder() // this function may be called from multiple tasks
{
BMP_LOCK( // reuse this for locking
    info_led_on();
    lens_setup_af(AF_ENABLE); // this has semaphores
    SW1(1,200);
    SW1(0,50);
    lens_cleanup_af();
    info_led_off();
)
}

static void set_af_point(int afpoint)
{
    if (!afp_len) return;
    if (!gui_menu_shown() && beep_enabled) beep();
    afp[0] = afpoint;
    prop_request_change(PROP_AFPOINT, afp, afp_len);
    if (!gui_menu_shown())
        task_create("afp_tmp", 0x18, 0, afp_show_in_viewfinder, 0);
}

static void afp_center () {
    set_af_point(afp_transformer(af_point, DIRECTION_CENTER));
}

static void afp_top () {
    set_af_point(afp_transformer(af_point, DIRECTION_UP));
}

static void afp_bottom () {
    set_af_point(afp_transformer(af_point, DIRECTION_DOWN));
}

static void afp_left () {
    set_af_point(afp_transformer(af_point, DIRECTION_LEFT));
}

static void afp_right () {
    set_af_point(afp_transformer(af_point, DIRECTION_RIGHT));
}

static int afp_transformer (int pattern, type_DIRECTION direction) {
    type_PATTERN_MAP_ITEM *item;

    // Loop over all items in the pattern map
    for (item = pattern_map; ! IS_EOL(item); item++) {

        // When we find an item matching the current pattern...
        if (item->pattern == pattern) {

            // ... we return the next pattern, according to the direction indicated
            switch (direction) {
            case DIRECTION_CENTER:
                return item->next_center;
            case DIRECTION_UP:
                return item->next_top;
            case DIRECTION_DOWN:
                return item->next_bottom;
            case DIRECTION_LEFT:
                return item->next_left;
            case DIRECTION_RIGHT:
                return item->next_right;
            }
        }
    }

    // Just in case something goes wrong
    return AF_PATTERN_CENTER;
}

int handle_af_patterns(struct event * event)
{
    if (af_patterns && !lv && gui_state == GUISTATE_IDLE && !DISPLAY_IS_ON)
    {
        switch (event->param)
        {
        case BGMT_PRESS_LEFT:
            afp_left();
            return 0;
        case BGMT_PRESS_RIGHT:
            afp_right();
            return 0;
        case BGMT_PRESS_UP:
            afp_top();
            return 0;
        case BGMT_PRESS_DOWN:
            afp_bottom();
            return 0;
        case BGMT_PRESS_SET:
            #ifdef CONFIG_60D
            if (get_cfn_function_for_set_button()) return 1; // do that custom function instead
            #endif
            afp_center();
            return 0;
        #ifdef BGMT_PRESS_UP_LEFT
        case BGMT_PRESS_UP_LEFT:
        case BGMT_PRESS_UP_RIGHT:
        case BGMT_PRESS_DOWN_LEFT:
        case BGMT_PRESS_DOWN_RIGHT:
            afp_center();
            return 0;
        #endif
        }
    }
    return 1;
}

void play_zoom_center_on_selected_af_point()
{
#ifdef IMGPLAY_ZOOM_POS_X
    if (af_point == AF_POINT_C) return; // nothing to do, zoom is centered by default
    int x = IMGPLAY_ZOOM_POS_X_CENTER;
    int y = IMGPLAY_ZOOM_POS_Y_CENTER;
    int n = 0;

    if (af_point == AF_POINT_T)
    {
        y -= IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_B)
    {
        y += IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_L)
    {
        x -= IMGPLAY_ZOOM_POS_DELTA_X;
        n++;
    }
    if (af_point == AF_POINT_R)
    {
        x += IMGPLAY_ZOOM_POS_DELTA_X;
        n++;
    }
#if defined(CONFIG_7D)
    if (af_point == AF_POINT_TT)
    {
        y -= 2*IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_BB)
    {
        y += 2*IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_LL)
    {
        x -= 2*IMGPLAY_ZOOM_POS_DELTA_X;
        n++;
    }
    if (af_point == AF_POINT_LLL)
    {
        x -= 3*IMGPLAY_ZOOM_POS_DELTA_X;
        n++;
    }
    if (af_point == AF_POINT_RR)
    {
        x += 2*IMGPLAY_ZOOM_POS_DELTA_X;
        n++;
    }
    if (af_point == AF_POINT_RRR)
    {
        x += 3*IMGPLAY_ZOOM_POS_DELTA_X;
        n++;
    }
    if (af_point == AF_POINT_TL)
    {
        x -= IMGPLAY_ZOOM_POS_DELTA_X;
        y -= IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_TLL)
    {
        x -= 2*IMGPLAY_ZOOM_POS_DELTA_X;
        y -= IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_TR)
    {
        x += IMGPLAY_ZOOM_POS_DELTA_X;
        y -= IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_TRR)
    {
        x += IMGPLAY_ZOOM_POS_DELTA_X;
        y -= 2*IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_BL)
    {
        x -= IMGPLAY_ZOOM_POS_DELTA_X;
        y += IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_BLL)
    {
        x -= 2*IMGPLAY_ZOOM_POS_DELTA_X;
        y += IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_BR)
    {
        x += IMGPLAY_ZOOM_POS_DELTA_X;
        y += IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
    if (af_point == AF_POINT_BRR)
    {
        x += 2*IMGPLAY_ZOOM_POS_DELTA_X;
        y += IMGPLAY_ZOOM_POS_DELTA_Y;
        n++;
    }
#else
    if (af_point == AF_POINT_TL)
    {
        x -= IMGPLAY_ZOOM_POS_DELTA_X / 2;
        y -= IMGPLAY_ZOOM_POS_DELTA_Y / 2;
        n++;
    }
    if (af_point == AF_POINT_TR)
    {
        x += IMGPLAY_ZOOM_POS_DELTA_X / 2;
        y -= IMGPLAY_ZOOM_POS_DELTA_Y / 2;
        n++;
    }
    if (af_point == AF_POINT_BL)
    {
        x -= IMGPLAY_ZOOM_POS_DELTA_X / 2;
        y += IMGPLAY_ZOOM_POS_DELTA_Y / 2;
        n++;
    }
    if (af_point == AF_POINT_BR)
    {
        x += IMGPLAY_ZOOM_POS_DELTA_X / 2;
        y += IMGPLAY_ZOOM_POS_DELTA_Y / 2;
        n++;
    }
#endif

    if (n == 0) return;

    IMGPLAY_ZOOM_POS_X = x;
    IMGPLAY_ZOOM_POS_Y = y;
#endif
}

static void afp_horiz_toggle(void* priv, int delta)
{
    if (delta > 0) afp_right(); else afp_left();
}

static void afp_vert_toggle(void* priv, int delta)
{
    if (delta > 0) afp_top(); else afp_bottom();
}

static void afp_center_toggle(void* priv, int delta)
{
    afp_center();
}

static void draw_af_point(int x, int y, int r, int color)
{
    for (int dr = -1; dr <= 1; dr++)
    {
        int rm = r + dr;
        bmp_draw_rect(color, x - rm, y - rm, rm*2, rm*2);
    }
}

static MENU_UPDATE_FUNC(pattern_display)
{
    if (!info->x) return;
    
    int cx = 500;
    int cy = 350;
    int w = 232;
    int h = 140;
    int dx = w*2/5;
    int dy = h*2/5;

    bmp_fill(COLOR_BLACK, cx-w/2, cy-h/2, w, h);
    bmp_draw_rect(COLOR_WHITE, cx-w/2, cy-h/2, w, h);

    draw_af_point(cx,      cy     , 7, af_point & AF_POINT_C ? COLOR_RED : 50);
    draw_af_point(cx - dx, cy     ,  5, af_point & AF_POINT_L ? COLOR_RED : 50);
    draw_af_point(cx + dx, cy     ,  5, af_point & AF_POINT_R ? COLOR_RED : 50);
    draw_af_point(cx     , cy + dy,  5, af_point & AF_POINT_B ? COLOR_RED : 50);
    draw_af_point(cx     , cy - dy,  5, af_point & AF_POINT_T ? COLOR_RED : 50);

    draw_af_point(cx + dx/2, cy + dy/2,  5, af_point & AF_POINT_BR ? COLOR_RED : 50);
    draw_af_point(cx - dx/2, cy - dy/2,  5, af_point & AF_POINT_TL ? COLOR_RED : 50);
    draw_af_point(cx + dx/2, cy - dy/2,  5, af_point & AF_POINT_TR ? COLOR_RED : 50);
    draw_af_point(cx - dx/2, cy + dy/2,  5, af_point & AF_POINT_BL ? COLOR_RED : 50);
}

static struct menu_entry afp_focus_menu[] = {
#if !defined(CONFIG_5DC) && !defined(CONFIG_5D3) && !defined(CONFIG_7D)
    {
        .name = "Focus Patterns",
        .select = menu_open_submenu,
        .help = "Custom AF patterns (photo mode only). Ported from 400plus.",
        .submenu_height = 280,
        .depends_on = DEP_PHOTO_MODE | DEP_CHIPPED_LENS | DEP_NOT_LIVEVIEW,
        .children =  (struct menu_entry[]) {
            {
                .name = "Center selection",
                .select = afp_center_toggle,
                .help = "Select a center-based AF pattern with LEFT and RIGHT keys.",
            },
            {
                .name = "Horizontal selection",
                .select = afp_horiz_toggle,
                .help = "Select a horizontal AF pattern with LEFT and RIGHT keys.",
            },
            {
                .name = "Vertical selection",
                .select = afp_vert_toggle,
                .update = pattern_display,
                .help = "Select a vertical AF pattern with LEFT and RIGHT keys.",
            },
            {
                .name = "Shortcut keys",
                .priv = &af_patterns,
                .max = 1,
                .help = "Choose patterns outside ML menu, display OFF, arrows+SET.",
            },
            MENU_EOL
        },
    },
#endif
};

void afp_menu_init() // called from focus.c
{
    menu_add( "Focus", afp_focus_menu, COUNT(afp_focus_menu) );
}
