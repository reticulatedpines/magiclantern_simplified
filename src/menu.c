/** \file
 * Magic Lantern GUI
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"
#include "math.h"
#include "version.h"
#include "bmp.h"
#include "gui.h"
#include "config.h"
#include "property.h"
#include "lens.h"
#include "font.h"
#include "menu.h"
#include "beep.h"
#include "zebra.h"
#include "focus.h"
#include "menuhelp.h"
#include "console.h"
#include "debug.h"
#include "lvinfo.h"
#include "powersave.h"

#define CONFIG_MENU_ICONS
//~ #define CONFIG_MENU_DIM_HACKS
#undef SUBMENU_DEBUG_JUNKIE

#ifdef FEATURE_VRAM_RGBA
// For D6+ RGBA we don't need double buffering
#define DOUBLE_BUFFERING 0
#else
#define DOUBLE_BUFFERING 1
#endif

//~ #define MENU_KEYHELP_Y_POS (menu_lv_transparent_mode ? 425 : 430)
#define MENU_HELP_Y_POS 435
#define MENU_HELP_Y_POS_2 458
#define MENU_WARNING_Y_POS (menu_lv_transparent_mode ? 425 : 458)

#define MENU_BG_COLOR_HEADER_FOOTER 42

extern int bmp_color_scheme;
#define MENU_BAR_COLOR (bmp_color_scheme ? COLOR_LIGHT_BLUE : COLOR_BLUE)

#ifdef CONFIG_MENU_ICONS
#define SUBMENU_OFFSET 48
#define MENU_OFFSET 38
#else
#define SUBMENU_OFFSET 30
#define MENU_OFFSET 20
#endif

#define MY_MENU_NAME "MyMenu"
static struct menu * my_menu;
static int my_menu_dirty = 0;

/* modified settings menu */
#define MOD_MENU_NAME "Modified"
static struct menu * mod_menu;
static int mod_menu_dirty = 1;

/* menu is checked for duplicate entries after adding new items */
static void check_duplicate_entries();
static int duplicate_check_dirty = 1;

//for vscroll
#define MENU_LEN 11

/*
int sem_line = 0;

int take_semapore_dbg(int sem, int timeout, int line) 
{ 
    bmp_printf(FONT_LARGE, 0, 0, "take: %d (%d) ", sem_line, line); 
    int ans = take_semaphore(sem, timeout); 
    sem_line = line; 
    bmp_printf(FONT_LARGE, 0, 0, "take: OK "); 
    return ans; 
}

#define TAKE_SEMAPHORE(sem, timeout) take_semapore_dbg(sem, timeout, __LINE__)
#define GIVE_SEMAPHORE(sem) { sem_line = 0; give_semaphore(sem); }
*/

static struct semaphore * menu_sem;
extern struct semaphore * gui_sem;
static struct semaphore * menu_redraw_sem;
static int menu_damage;
static int menu_shown = false;
static int menu_lv_transparent_mode; // for ISO, kelvin...
static int config_dirty = 0;

static int menu_flags_save_dirty = 0;
static int menu_flags_load_dirty = 1;

//~ static int menu_hidden_should_display_help = 0;
static int menu_zebras_mirror_dirty = 0; // to clear zebras from mirror (avoids display artifacts if, for example, you enable false colors in menu, then you disable them, and preview LV)

int menu_help_active = 0; // also used in menuhelp.c
int menu_redraw_blocked = 0; // also used in flexinfo
static int menu_redraw_cancel = 0;

static int submenu_level = 0;
static int edit_mode = 0;
static int customize_mode = 0;
static int advanced_mode = 0;       /* cached value; only for submenus for now */
static int caret_position = 0;

#define SUBMENU_OR_EDIT (submenu_level || edit_mode)
#define EDIT_OR_TRANSPARENT (edit_mode || menu_lv_transparent_mode)

/* fixme: better solution? */
static struct menu_entry * entry_being_updated = 0;
static int entry_removed_itself = 0;

#ifdef FEATURE_JUNKIE_MENU
static CONFIG_INT("menu.junkie", junkie_mode, 0);
#else
#define junkie_mode 0   /* let the compiler optimize out this code */
#endif
//~ static CONFIG_INT("menu.set", set_action, 2);
//~ static CONFIG_INT("menu.start.my", start_in_my_menu, 0);

static int is_customize_selected();

extern void CancelDateTimer();

#define CAN_HAVE_PICKBOX(entry) ((entry)->max > (entry)->min && (((entry)->max - (entry)->min < 15) || (entry)->choices) && IS_ML_PTR((entry)->priv))
#define SHOULD_HAVE_PICKBOX(entry) ((entry)->max > (entry)->min + 1 && (entry)->max - (entry)->min < 10 && IS_ML_PTR((entry)->priv))
#define IS_BOOL(entry) (((entry)->max - (entry)->min == 1 && IS_ML_PTR((entry)->priv)) || (entry->icon_type == IT_BOOL))
#define IS_ACTION(entry) ((entry)->icon_type == IT_ACTION || (entry)->icon_type == IT_SUBMENU)
#define SHOULD_USE_EDIT_MODE(entry) (!IS_BOOL(entry) && !IS_ACTION(entry))

#define HAS_SINGLE_ITEM_SUBMENU(entry) ((entry)->children && !(entry)->children[0].next && !(entry)->children[0].prev && !MENU_IS_EOL(entry->children))
#define IS_SINGLE_ITEM_SUBMENU_ENTRY(entry) (!(entry)->next && !(entry)->prev)

static int can_be_turned_off(struct menu_entry * entry)
{
    return 
    (IS_BOOL(entry) && entry->icon_type != IT_DICE) ||
     entry->icon_type == IT_PERCENT_OFF ||
     entry->icon_type == IT_PERCENT_LOG_OFF ||
     entry->icon_type == IT_DICE_OFF ||
     entry->icon_type == IT_SUBMENU;
}

#define HAS_HIDDEN_FLAG(entry) ((entry)->hidden)
#define HAS_JHIDDEN_FLAG(entry) ((entry)->jhidden)
#define HAS_SHIDDEN_FLAG(entry) ((entry)->shidden) // this is *never* displayed
#define HAS_STARRED_FLAG(entry) ((entry)->starred) // in junkie mode, this is only displayed in MyMenu (implicit hiding from main menus)

#define HAS_CURRENT_HIDDEN_FLAG(entry) ( \
    (!junkie_mode && HAS_HIDDEN_FLAG(entry)) || \
    (junkie_mode && HAS_JHIDDEN_FLAG(entry)) )

// junkie mode, entry present in my menu, hide it from normal menu
#define IMPLICIT_MY_MENU_HIDING(entry) \
    (junkie_mode && HAS_STARRED_FLAG(entry))

static int is_visible(struct menu_entry * entry)
{
    return 
        (
            !(HAS_CURRENT_HIDDEN_FLAG(entry) || IMPLICIT_MY_MENU_HIDING(entry)) ||
            customize_mode ||
            junkie_mode==3
       )
       &&
       (
            !HAS_SHIDDEN_FLAG(entry)
       )
       &&
       (
            advanced_mode || !entry->advanced || entry->selected || config_var_was_changed(entry->priv)
       )
       ;
}

static int g_submenu_width = 0;
//~ #define g_submenu_width 720
static int redraw_flood_stop = 0;

#define MENU_REDRAW 1

static int hist_countdown = 3; // histogram is slow, so draw it less often

int is_submenu_or_edit_mode_active() { return gui_menu_shown() && SUBMENU_OR_EDIT; }
int get_menu_edit_mode() { return edit_mode; }

//~ static CONFIG_INT("menu.transparent", semitransparent, 0);

//static CONFIG_INT("menu.first", menu_first_by_icon, ICON_i);
static CONFIG_INT("menu.first", menu_first_by_icon, ICON_ML_INFO);

void menu_set_dirty() { menu_damage = 1; }

int is_menu_help_active() { return gui_menu_shown() && menu_help_active; }

static void select_menu_by_icon(int icon);
static void menu_help_go_to_selected_entry(struct menu * menu);
//~ static void menu_init( void );
static void menu_show_version(void);
static struct menu * get_current_submenu();
static struct menu * get_current_menu_or_submenu();
static struct menu * get_selected_toplevel_menu();
static void menu_make_sure_selection_is_valid();
static void config_menu_reload_flags();
static int guess_submenu_enabled(struct menu_entry * entry);
static void menu_draw_icon(int x, int y, int type, intptr_t arg, int warn); // private
static struct menu_entry * entry_find_by_name(const char* name, const char* entry_name);
static struct menu_entry * get_selected_menu_entry(struct menu * menu);
static void submenu_display(struct menu * submenu);
static void start_redraw_flood();
static struct menu * menu_find_by_name(const char * name,  int icon);
void menu_toggle_submenu();

extern int gui_state;
void menu_enable_lv_transparent_mode()
{
    if (lv) menu_lv_transparent_mode = 1;
    menu_damage = 1;
}
void menu_disable_lv_transparent_mode()
{
    menu_lv_transparent_mode = 0;
}
int menu_active_but_hidden() { return gui_menu_shown() && ( menu_lv_transparent_mode ); }
int menu_active_and_not_hidden() { return gui_menu_shown() && !( menu_lv_transparent_mode && hist_countdown < 2 ); }

static void
draw_version( void )
{
    bmp_printf(
        FONT( FONT_SMALL, COLOR_WHITE, COLOR_BLUE ),
        0, 0,
        "Magic Lantern Firmware version %s (%s)\nBuilt on%s by %s\n%s",
        build_version,
        build_id,
        build_date,
        build_user,
        "http://www.magiclantern.fm/"
    );

/*
    int y = 200;
    struct config * config = global_config;
    bmp_printf( FONT_SMALL, 0, y, "Config: %x", (unsigned) global_config );
    y += font_small.height;

    while( config )
    {
        bmp_printf( FONT_SMALL, 0, y, "'%s' => '%s'", config->name, config->value );
        config = config->next;
        y += font_small.height;
    }
*/
}

#ifdef CONFIG_RELEASE_BUILD
int beta_should_warn() { return 0; }
#else
CONFIG_INT("beta.warn", beta_warn, 0);
static int get_beta_timestamp()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    return now.tm_mday;
}
static int beta_should_warn()
{
    int t = get_beta_timestamp();
    return beta_warn != t;
}

static void beta_set_warned()
{
    unsigned t = get_beta_timestamp();
    beta_warn = t;
}
#endif

static MENU_SELECT_FUNC(customize_toggle)
{
    customize_mode = !customize_mode;
    my_menu_dirty = 1;
    mod_menu_dirty = 1;
}

static struct menu_entry customize_menu[] = {
    {
        .name   = "Customize Menus",
        .priv   = &customize_mode,
        .max    = 1,
        .select = customize_toggle,
    }
};

static int is_customize_selected(struct menu * menu) // argument is optional, just for speedup
{
    struct menu_entry * selected_entry = get_selected_menu_entry(menu);
    if (selected_entry == &customize_menu[0])
        return 1;
    return 0;
}

#define MY_MENU_ENTRY \
        { \
            .name = "(empty)", \
            .hidden = 1, \
            .jhidden = 1, \
        },

static struct menu_entry my_menu_placeholders[] = {
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
};
/*
static struct menu_entry menu_prefs[] = {
    {
        .name = "Menu Preferences",
        .select     = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            {
                .name = "Start in MyMenu",
                .priv = &start_in_my_menu,
                .max = 1,
                .help  = "Go to My Menu every time you open ML menu.",
            },
            {
                .name = "SET action",
                .priv = &set_action,
                .max = 2,
                .choices = (const char *[]) {"Pickbox", "Toggle", "Auto"},
                .help  = "Choose the behavior of SET key in ML menu:",
                .help2 = "Pickbox: SET shows a list of choices, select and confirm.\n"
                         "Toggle: SET will toggle ON/OFF or increment current value.\n"
                         "Auto: SET toggles ON/OFF items, pickbox for 3+ choices.",
            },
            MENU_EOL,
        },
    }
};
*/

/* todo: use dynamic entries, like in file_man */
static struct menu_entry mod_menu_placeholders[] = {
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY

    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY

    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY

    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY

    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
    MY_MENU_ENTRY
};

void customize_menu_init()
{
    menu_add("Prefs", customize_menu, COUNT(customize_menu));

    // this is added at the end, after all the others
    my_menu = menu_find_by_name( MY_MENU_NAME, ICON_ML_MYMENU  );
    menu_add(MY_MENU_NAME, my_menu_placeholders, COUNT(my_menu_placeholders));
    my_menu->no_name_lookup = 1;
    
    mod_menu = menu_find_by_name(MOD_MENU_NAME, ICON_ML_MODIFIED);
    menu_add(MOD_MENU_NAME, mod_menu_placeholders, COUNT(mod_menu_placeholders));
    mod_menu->no_name_lookup = 1;
}

static struct menu * menus;

struct menu * menu_get_root() {
  return menus;
}

// 1-2-5 series - https://en.wikipedia.org/wiki/Preferred_number#1-2-5_series
static int round_to_125(int val)
{
    if (val < 0)
    {
        return -round_to_125(-val);
    }
    
    int mag = 1;
    while (val >= 30)
    {
        val /= 10;
        mag *= 10;
    }
    
    if (val <= 2)
        {}
    else if (val <= 3)
        val = 2;
    else if (val <= 7)
        val = 5;
    else if (val <= 14)
        val = 10;
    else
        val = 20;
    
    return val * mag;
}

// ISO 3 R10": 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100
static int round_to_R10(int val)
{
    if (val < 0)
        return -round_to_R10(-val);
    
    int mag = 1;
    while (val >= 30)
    {
        val /= 10;
        mag *= 10;
    }
    
    if (val <= 6)
        {}
    else if (val <= 8)
        val = 8;
    else if (val <= 11)
        val = 10;
    else if (val <= 13)
        val = 12;
    else if (val <= 17)
        val = 15;
    else if (val <= 22)
        val = 20;
    else if (val <= 27)
        val = 25;
    else
        val = 30;
    
    return val * mag;
}

// ISO 3 R20": 10, 11, 12, 14, 15, 18, 20, 22, 25, 28, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90, 100
static int round_to_R20(int val)
{
    if (val < 0)
        return -round_to_R20(-val);
    
    int mag = 1;
    while (val >= 60)
    {
        val /= 10;
        mag *= 10;
    }
    
    if (val <= 12)
        {}
    else if (val <= 14)
        val = 14;
    else if (val <= 16)
        val = 15;
    else if (val <= 19)
        val = 18;
    else if (val <= 21)
        val = 20;
    else if (val <= 23)
        val = 22;
    else if (val <= 26)
        val = 25;
    else if (val <= 29)
        val = 28;
    else if (val <= 32)
        val = 30;
    else if (val <= 37)
        val = 35;
    else if (val <= 42)
        val = 40;
    else if (val <= 47)
        val = 45;
    else if (val <= 52)
        val = 50;
    else if (val <= 57)
        val = 55;
    else
        val = 60;
    
    return val * mag;
}

static int round_to_pow2(int val)
{
    int stops = (int)roundf(log2f(val));
    return (int)roundf(powf(2, stops));
}

static void menu_numeric_toggle_rounded(int* val, int delta, int min, int max, int (*round_func)(int))
{
    ASSERT(IS_ML_PTR(val));

    int v = *val;

    if (v >= max && delta > 0)
        v = min;
    else if (v <= min && delta < 0)
        v = max;
    else
    {
        int v0 = round_func(v);
        if (v0 != v && SGN(v0 - v) == SGN(delta)) // did we round in the correct direction? if so, stop here
        {
            v = v0;
            goto end;
        }
        // slow, but works (fast enough for numbers like 5000)
        while (v0 == round_func(v))
            v += delta;
        v = COERCE(round_func(v), min, max);
    }

end:
    set_config_var_ptr(val, v);
}

static void menu_numeric_toggle_R10(int* val, int delta, int min, int max)
{
    return menu_numeric_toggle_rounded(val, delta, min, max, round_to_R10);
}

static void menu_numeric_toggle_R20(int* val, int delta, int min, int max)
{
    return menu_numeric_toggle_rounded(val, delta, min, max, round_to_R20);
}

static void menu_numeric_toggle_125(int* val, int delta, int min, int max)
{
    return menu_numeric_toggle_rounded(val, delta, min, max, round_to_125);
}

static void menu_numeric_toggle_pow2(int* val, int delta, int min, int max)
{
    return menu_numeric_toggle_rounded(val, delta, min, max, round_to_pow2);
}

static void menu_numeric_toggle_long_range(int* val, int delta, int min, int max)
{
    ASSERT(IS_ML_PTR(val));

    int v = *val;

    if (v >= max && delta > 0)
        v = min;
    else if (v <= min && delta < 0)
        v = max;
    else
    {
        int a = ABS(v + delta);
        int M = 1;
        if (a >= 20000) M = 1000;
        else if (a >= 10000) M = 500;
        else if (a >= 2000) M = 100;
        else if (a >= 1000) M = 50;
        else if (a >= 200) M = 10;
        else if (a >= 100) M = 5;
        
        v += delta;
        while (v % M)
        {
            if (v >= max) break;
            if (v <= min) break;
            v += delta;
        }
    }
    
    set_config_var_ptr(val, v);
}

/* for editing with caret */
static int get_caret_delta(struct menu_entry * entry, int sign)
{
    if(!EDIT_OR_TRANSPARENT)
    {
        return sign;
    }

    switch (entry->unit)
    {
        case UNIT_DEC:
        case UNIT_TIME_MS:
        case UNIT_TIME_US:
        {
            return sign * powi(10, caret_position);
        }

        case UNIT_HEX:
        {
            return sign * powi(16, caret_position);
        }

        case UNIT_TIME:
        {
            const int increments[] = { 0, 1, 10, 0, 60, 600, 0, 3600, 36000 };
            return sign * increments[caret_position];
        }

        default:
        {
            return sign;
        }
    }
}

static int uses_caret_editing(struct menu_entry * entry)
{
    return 
        entry->select == 0 &&   /* caret editing requires its own toggle logic */
        (entry->unit == UNIT_DEC || entry->unit == UNIT_HEX  || entry->unit == UNIT_TIME ||
         entry->unit == UNIT_TIME_MS || entry->unit == UNIT_TIME_US);  /* only these caret edit modes are supported */
}

static int editing_with_caret(struct menu_entry * entry)
{
    return EDIT_OR_TRANSPARENT && uses_caret_editing(entry);
}

static void caret_move(struct menu_entry * entry, int delta)
{
    int max = (entry->unit == UNIT_TIME) ? 7 :
              (entry->unit == UNIT_HEX)  ? log2i(MAX(ABS(entry->max),ABS(entry->min)))/4
                                         : log10i(MAX(ABS(entry->max),ABS(entry->min))/2) ;

    menu_numeric_toggle(&caret_position, delta, 0, max);

    /* skip "h", "m" and "s" positions for time fields */
    if(entry->unit == UNIT_TIME && (caret_position == 0 || caret_position == 3 || caret_position == 6))
    {
        menu_numeric_toggle(&caret_position, delta, 0, max);
    }
}

void menu_numeric_toggle(int* val, int delta, int min, int max)
{
    ASSERT(IS_ML_PTR(val));

    set_config_var_ptr(val, MOD(*val - min + delta, max - min + 1) + min);
}

void menu_numeric_toggle_time(int * val, int delta, int min, int max)
{
    int deltas[] = {1,5,15,30,60,300,900,1800,3600};
    int i = 0;
    for(i = COUNT(deltas) - 1; i > 0; i--)
        if(deltas[i] * 4 <= (delta < 0 ? *val - 1 : *val)) break;
    delta *= deltas[i];
    
    int new_val = (*val + delta) / delta * delta;
    if(new_val > max) new_val = min;
    if(new_val < min) new_val = max;
    
    set_config_var_ptr(val, new_val);
}

static void menu_numeric_toggle_fast(int* val, int delta, int min, int max, int unit, int edit_mode, int ignore_timing)
{
    ASSERT(IS_ML_PTR(val));
    
    static int prev_t = 0;
    static int prev_delta = 1000;
    int t = get_ms_clock();

    if (unit == UNIT_TIME)
    {
        menu_numeric_toggle_time(val, delta, min, max);
    }
    else if (edit_mode & EM_ROUND_ISO_R10)
    {
        menu_numeric_toggle_R10(val, delta, min, max);
    }
    else if (edit_mode & EM_ROUND_ISO_R20)
    {
        menu_numeric_toggle_R20(val, delta, min, max);
    }
    else if (edit_mode & EM_ROUND_1_2_5_10)
    {
        menu_numeric_toggle_125(val, delta, min, max);
    }
    else if (edit_mode & EM_ROUND_POWER_OF_2)
    {
        menu_numeric_toggle_pow2(val, delta, min, max);
    }
    else if (max - min > 20)
    {
        if (t - prev_t < 200 && prev_delta < 200 && !ignore_timing)
        {
            menu_numeric_toggle_R20(val, delta, min, max);
        }
        else
        {
            menu_numeric_toggle_long_range(val, delta, min, max);
        }
    }
    else
    {
        // SJE FIXME - does the following need a check to avoid div by zero?
        // If so, consider fixing MOD itself
        set_config_var_ptr(val, MOD(*val - min + delta, max - min + 1) + min);
    }
    
    prev_delta = t - prev_t;
    prev_t = t;
}

static void entry_guess_icon_type(struct menu_entry * entry)
{
    if (entry->icon_type == IT_AUTO)
    {
        if (entry->select == menu_open_submenu)
        {
            entry->icon_type = IT_SUBMENU;
        }
        else if (!IS_ML_PTR(entry->priv) || entry->select == run_in_separate_task)
        {
            entry->icon_type = IT_ACTION;
        }
        else if(entry->choices)
        {
            const char* first_choice = entry->choices[0];
            if (streq(first_choice, "OFF") || streq(first_choice, "Hide"))
                entry->icon_type = entry->max == 1 ? IT_BOOL : IT_DICE_OFF;
            else if (streq(first_choice, "ON"))
                entry->icon_type = IT_BOOL_NEG;
            else if (streq(first_choice, "Small"))
                entry->icon_type = IT_SIZE;
            else
                entry->icon_type = IT_DICE;
        }
        else if (entry->min != entry->max)
        {
            entry->icon_type = entry->max == 1 && entry->min == 0 ? IT_BOOL :
                entry->max - entry->min > 10 ? 
                    (entry->max * entry->min <= 0 ? IT_PERCENT_LOG_OFF : IT_PERCENT_LOG) :
                    (entry->max * entry->min <= 0 ? IT_PERCENT_OFF : IT_PERCENT);
        }
        else
            entry->icon_type = IT_BOOL;
    }
}

static int entry_guess_enabled(struct menu_entry * entry)
{
    if (entry->icon_type == IT_BOOL || entry->icon_type == IT_DICE_OFF || entry->icon_type == IT_PERCENT_OFF || entry->icon_type == IT_PERCENT_LOG_OFF)
        return MENU_INT(entry);
    else if (entry->icon_type == IT_BOOL_NEG)
        return !MENU_INT(entry);
    else if (entry->icon_type == IT_SUBMENU)
        return guess_submenu_enabled(entry);
    else return 1;
}

static int guess_submenu_enabled(struct menu_entry * entry)
{
    if IS_ML_PTR(entry->priv) // if it has a priv field, use it as truth value for the entire group
    {
        return MENU_INT(entry);
    }
    else 
    {   // otherwise, look in the children submenus; if one is true, then submenu icon is drawn as "true"
        struct menu_entry * e = entry->children;

        for( ; e ; e = e->next )
        {
            if (MENU_INT(e) && can_be_turned_off(e))
            {
                return 1;
            }
        }

        return 0;
    }
}

static int log_percent(struct menu_entry * entry)
{
    if (entry->min * entry->max < 0) // goes to both sizes? consider 50% in the middle
    {
        int v = MENU_INT(entry);
        if (v == 0)
            return 50;
        else if (v > 0)
            return 50 + log_length(v + 1) * 50 / log_length(entry->max + 1);
        else
            return 50 - log_length(-v + 1) * 50 / log_length(-entry->min + 1);
    }
    else
    {
        return log_length(SELECTED_INDEX(entry) + 1) * 100 / log_length(NUM_CHOICES(entry));
    }
}

static void entry_draw_icon(
    struct menu_entry * entry,
    int         x,
    int         y,
    int         enabled,
    int         warn
)
{
    switch (entry->icon_type)
    {
        case IT_BOOL:
        case IT_BOOL_NEG:
            menu_draw_icon(x, y, MNI_BOOL(enabled), 0, warn);
            break;

        case IT_ACTION:
            menu_draw_icon(x, y, MNI_ACTION, 0, warn);
            break;

        case IT_ALWAYS_ON:
            menu_draw_icon(x, y, MNI_AUTO, 0, warn);
            break;
            
        //~ case IT_SIZE:
            //~ if (!enabled) menu_draw_icon(x, y, MNI_OFF, 0, warn);
            //~ else menu_draw_icon(x, y, MNI_SIZE, SELECTED_INDEX(entry) | (NUM_CHOICES(entry) << 16), warn);
            //~ break;

        case IT_DICE:
            if (!enabled) menu_draw_icon(x, y, MNI_DICE_OFF, 0 | (NUM_CHOICES(entry) << 16), warn);
            else menu_draw_icon(x, y, MNI_DICE, SELECTED_INDEX(entry) | (NUM_CHOICES(entry) << 16), warn);
            break;
        
        case IT_DICE_OFF:
            if (!enabled) menu_draw_icon(x, y, MNI_DICE_OFF, 0 | (NUM_CHOICES(entry) << 16), warn);
            menu_draw_icon(x, y, MNI_DICE_OFF, SELECTED_INDEX(entry) | (NUM_CHOICES(entry) << 16), warn);
            break;
        
        case IT_PERCENT_OFF:
        {
            int p = SELECTED_INDEX(entry) * 100 / (NUM_CHOICES(entry)-1);
            if (!enabled) menu_draw_icon(x, y, MNI_PERCENT_OFF, p, warn);
            menu_draw_icon(x, y, MNI_PERCENT_ALLOW_OFF, p, warn);
            break;
        }
        case IT_PERCENT:
            //~ if (entry->min < 0) menu_draw_icon(x, y, MNI_PERCENT_PM, (CURRENT_VALUE & 0xFF) | ((entry->min & 0xFF) << 8) | ((entry->max & 0xFF) << 16), warn);
            menu_draw_icon(x, y, MNI_PERCENT, SELECTED_INDEX(entry) * 100 / (NUM_CHOICES(entry)-1), warn);
            break;

        case IT_PERCENT_LOG_OFF:
        {
            int p = log_percent(entry);
            if (!enabled) menu_draw_icon(x, y, MNI_PERCENT_OFF, p, warn);
            menu_draw_icon(x, y, MNI_PERCENT_ALLOW_OFF, p, warn);
            break;
        }
        case IT_PERCENT_LOG:
        {
            int p = log_percent(entry);
            menu_draw_icon(x, y, MNI_PERCENT, p, warn);
            break;
        }

        //~ case IT_NAMED_COLOR:
            //~ if (!enabled) menu_draw_icon(x, y, MNI_OFF, 0, warn);
            //~ else menu_draw_icon(x, y, MNI_NAMED_COLOR, (intptr_t) entry->choices[SELECTED_INDEX(entry)], warn);
            //~ break;

        case IT_DISABLE_SOME_FEATURE:
            menu_draw_icon(x, y, MENU_INT(entry) ? MNI_DISABLE : MNI_NEUTRAL, 0, warn);
            break;

/*        
        case IT_DISABLE_SOME_FEATURE_NEG:
            menu_draw_icon(x, y, MENU_INT(entry) ? MNI_NEUTRAL : MNI_DISABLE, 0, warn);
            break;

        case IT_REPLACE_SOME_FEATURE:
            menu_draw_icon(x, y, MENU_INT(entry) ? MNI_ON : MNI_NEUTRAL, 0, warn);
            break;
*/

        case IT_SUBMENU:
        {
            int value = guess_submenu_enabled(entry);
            if (!enabled) value = 0;
            menu_draw_icon(x, y, MNI_SUBMENU, value, warn);
            break;
        }
    }
}


/*
static struct menu_entry*
menu_find_by_id_entry(
    struct menu_entry* root,
    uint32_t id
)
{
    struct menu_entry *       menu = root;

    for( ; menu ; menu = menu->next )
    {
        if( menu->id == id )
        {
            return menu;
        }
        if( menu->children )
        {
          struct menu_entry * ch = menu_find_by_id_entry(menu->children, id);
          if (ch!=NULL) return ch;
        }
    }
    return NULL;
}

struct menu_entry *
menu_find_by_id(
    uint32_t id
)
{
    struct menu *       menu = menus;

    for( ; menu ; menu = menu->next )
    {
        if( menu->id == id )
        {
            return menu->children;
        }
        if( menu->children )
        {
          struct menu_entry * ch = menu_find_by_id_entry(menu->children, id);
          if (ch!=NULL) return ch;
        }
    }

    return NULL;
}
*/

static struct menu *
menu_find_by_name_internal(
    const char *        name,
    int icon
)
{
    ASSERT(name);

    struct menu * menu = menus;

    for( ; menu ; menu = menu->next )
    {
        ASSERT(menu->name);
        if( streq( menu->name, name ) )
        {
            if (icon && !menu->icon) menu->icon = icon;
            return menu;
        }

        // Stop just before we get to the end
        if( !menu->next )
            break;
    }

    // Not found; create it
    struct menu * new_menu = malloc( sizeof(*new_menu) );
    if( !new_menu )
    {
        return NULL;
    }

    memset(new_menu, 0, sizeof(struct menu));
    new_menu->name      = name;
    new_menu->icon      = icon;
    new_menu->prev      = menu;
    new_menu->next      = NULL; // Inserting at end
    new_menu->split_pos = -16;

    // menu points to the last entry or NULL if there are none
    if( menu )
    {
        // We are adding to the end
        menu->next      = new_menu;
        new_menu->selected  = 0;
    } else {
        // This is the first one
        menus           = new_menu;
        new_menu->selected  = 1;
    }

    return new_menu;
}

static struct menu * 
menu_find_by_name(
    const char *        name,
    int icon
)
{
    take_semaphore( menu_sem, 0 );

    struct menu * menu = menu_find_by_name_internal(name, icon);

    give_semaphore( menu_sem );
    return menu;
}

static int get_menu_visible_count(struct menu * menu)
{
    int n = 0;
    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        if (is_visible(entry))
        {
            n ++;
        }
    }
    return n;
}

static int get_menu_selected_pos(struct menu * menu)
{
    int n = 0;
    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        if (is_visible(entry))
        {
            n ++;
            if (entry->selected)
                return n;
        }
    }
    return 0;
}

static int
menu_has_visible_items(struct menu * menu)
{
    if (junkie_mode) // hide Modules, Help and Modified
    {
        if (
            streq(menu->name, "Modules") ||
            streq(menu->name, "Help") ||
            streq(menu->name, MOD_MENU_NAME) ||
           0)
            return 0;
    }
    
    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        /* hide the Customize menu if everything else from Prefs is also hidden */
        if (entry == &customize_menu[0]) 
            continue;

        if (is_visible(entry))
        {
            return 1;
        }
    }
    return 0;
}

static int
are_there_any_visible_menus()
{
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (!IS_SUBMENU(menu) && menu_has_visible_items(menu))
        {
            return 1;
        }
    }
    return 0;
}


#if defined(POSITION_INDEPENDENT)
/* when compiling position independent, we have to fix all link-time addresses to load-time addresses */
void menu_fixup_pic(struct menu_entry * new_entry, int count)
{
    struct menu_entry * main_ptr = new_entry;
    struct menu_entry * sub_ptr;
    
    while(count)
    {
        main_ptr->select = PIC_RESOLVE(main_ptr->select);
        //~ main_ptr->select_reverse = PIC_RESOLVE(main_ptr->select_reverse);
        main_ptr->select_Q = PIC_RESOLVE(main_ptr->select_Q);
        main_ptr->display = PIC_RESOLVE(main_ptr->display);
        main_ptr->help = PIC_RESOLVE(main_ptr->help);
        main_ptr->name = PIC_RESOLVE(main_ptr->name);
        main_ptr->priv = PIC_RESOLVE(main_ptr->priv);
        main_ptr->children = PIC_RESOLVE(main_ptr->children);
        main_ptr->choices = PIC_RESOLVE(main_ptr->choices);
        
        if(main_ptr->choices)
        {
            for(int pos = 0; pos <= main_ptr->max; pos++)
            {
                main_ptr->choices[pos] = PIC_RESOLVE(main_ptr->choices[pos]);
            }
        }   
        
        if(main_ptr->children)
        {
            int entries = 0;
            
            while(main_ptr->children[entries].priv != MENU_EOL_PRIV)
            {
                entries++;
            }
            
            menu_fixup_pic(main_ptr->children, entries);
        }
        
        main_ptr++;
        count--;
    }
}
#endif

static void menu_update_split_pos(struct menu * menu, struct menu_entry * entry)
{
    // auto adjust width so that all things can be printed nicely
    // only "negative" numbers are auto-adjusted (if you override the width, you do so with a positive value)
    if (menu->split_pos < 0)// && entry->priv)
    {
        menu->split_pos = -MAX(-menu->split_pos, bmp_string_width(FONT_LARGE, entry->name)/20 + 2);
        if (-menu->split_pos > 28) menu->split_pos = -28;
    }
}

static void placeholder_copy(struct menu_entry * dst, struct menu_entry * src)
{
    /* keep linked list pointers and customization flags from the old entry */
    void* next = dst->next;
    void* prev = dst->prev;
    int selected = dst->selected;
    int starred = dst->starred;
    int hidden = dst->hidden;
    int jhidden = dst->jhidden;
    uint64_t usage_counters = dst->usage_counters;
    
    /* also keep the name pointer, which will help when removing the menu and restoring the placeholder */
    char* name = (char*) dst->name;
    
    memcpy(dst, src, sizeof(struct menu_entry));

    dst->next = next;
    dst->prev = prev;
    dst->name = name;
    dst->selected = selected;
    dst->starred = starred;
    dst->hidden = hidden;
    dst->jhidden = jhidden;
    dst->usage_counters = usage_counters;
}

/* if we find a placeholder entry, use it for changing the menu order */
/* todo: check interference with menu customization */
static void
menu_update_placeholder(struct menu * menu, struct menu_entry * new_entry)
{
    if (!menu) return;

    if (MENU_IS_PLACEHOLDER(new_entry))
    {
        menu->has_placeholders = 1;
        return;
    }

    if (!menu->has_placeholders)
    {
        return;
    }
    
    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        if (entry != new_entry && MENU_IS_PLACEHOLDER(entry) && streq(entry->name, new_entry->name))
        { /* found, let's try to swap the entries */
            
            placeholder_copy(entry, new_entry);
            entry->shidden = 0;
            new_entry->shidden = 1;
            new_entry->placeholder = 1;
            
            if (entry->starred)
                my_menu_dirty = 1;

            /* warning: the unused entry is still kept in place, but hidden; important to delete? */
            break;
        }
    }
}


static void
menu_add_internal(
    const char *        name,
    struct menu_entry * new_entry,
    int                 count
)
{
#if defined(POSITION_INDEPENDENT)
    menu_fixup_pic(new_entry, count);
#endif

    // There is nothing to display. Sounds crazy (but might result from ifdef's)
    if ( count == 0 )
        return;

    // Walk the menu list to find a menu
    struct menu * menu = menu_find_by_name_internal( name, 0);
    if( !menu )
        return;

    struct menu_entry * parent = NULL;

    if (IS_SUBMENU(menu))
    {
        /* all submenus should have some valid parent */
        /* note: some submenus might be used by more than one menu entry;
         * in this case, any of them is valid; all of them will have the same name */
        ASSERT(menu->parent_menu);
        ASSERT(menu->parent_entry);
        ASSERT(streq(name, menu->parent_entry->name));

        /* all entries from the submenu should be linked to the parent menu entry */
        parent = menu->parent_entry;
        ASSERT(streq(parent->name, name));
    }

    menu_flags_load_dirty = 1;
    duplicate_check_dirty = 1;
    
    int count0 = count; // for submenus

    struct menu_entry * head = menu->children;
    if( !head )
    {
        // First one -- insert it as the selected item
        // fixme: duplicate code
        head = menu->children = new_entry;
        ASSERT(new_entry->name);
        ASSERT(new_entry->next == NULL);
        ASSERT(new_entry->prev == NULL);
        ASSERT((parent == NULL) ^ IS_SUBMENU(menu));
        ASSERT(new_entry->parent_menu == NULL);
        new_entry->parent = parent;
        new_entry->depends_on    |= (parent ? parent->depends_on : 0); // inherit dependencies
        new_entry->works_best_in |= (parent ? parent->works_best_in : 0);
        new_entry->parent_menu = menu;
        new_entry->selected = 1;
        menu_update_split_pos(menu, new_entry);
        entry_guess_icon_type(new_entry);
        menu_update_placeholder(menu, new_entry);
        new_entry++;
        count--;
    }

    // Find the end of the entries on the menu already
    while( head->next )
        head = head->next;

    for (int i = 0; i < count; i++)
    {
        ASSERT(new_entry->name);
        ASSERT(new_entry->next == NULL);
        ASSERT(new_entry->prev == NULL);
        ASSERT((parent == NULL) ^ IS_SUBMENU(menu));
        ASSERT(new_entry->parent_menu == NULL);
        new_entry->parent = parent;
        new_entry->depends_on    |= (parent ? parent->depends_on : 0); // inherit dependencies
        new_entry->works_best_in |= (parent ? parent->works_best_in : 0);
        new_entry->parent_menu = menu;
        new_entry->selected = 0;
        new_entry->prev = head;
        head->next      = new_entry;
        head            = new_entry;
        menu_update_split_pos(menu, new_entry);
        entry_guess_icon_type(new_entry);
        menu_update_placeholder(menu, new_entry);
        new_entry++;
    }

    // create submenus

    struct menu_entry * entry = head;
    for (int i = 0; i < count0; i++)
    {
        if (entry->children)
        {
            int count = 0;
            for (struct menu_entry * child = entry->children; !MENU_IS_EOL(child); child++)
            {
                count++;
            }

            struct menu * submenu = menu_find_by_name_internal( entry->name, ICON_ML_SUBMENU);
            submenu->parent_menu = menu;
            submenu->parent_entry = entry;
            submenu->submenu_width = entry->submenu_width;
            submenu->submenu_height = entry->submenu_height;

            if (submenu->children != entry->children)
            {
                /* sometimes the submenus are reused (e.g. Module menu)
                 * only add them once */
                menu_add_internal(entry->name, entry->children, count);
            }

            /* the menu might have been created before as a regular menu (not as submenu) */
            /* ensure the "children" field always points to the very first item in the submenu */
            /* also make sure the parent entries and dependency flags are correct */
            while (entry->children->prev)
            {
                entry->children = entry->children->prev;
                entry->children->parent = entry;
                entry->children->depends_on |= entry->depends_on;
                entry->children->works_best_in |= entry->works_best_in;
                printf("updating %s -> %s\n", entry->name, entry->children->name);
            }
        }
        entry = entry->prev;
        if (!entry) break;
    }
}

void 
menu_add(
    const char *        name,
    struct menu_entry * new_entry,
    int                 count
)
{
    take_semaphore( menu_sem, 0 );

    menu_add_internal(name, new_entry, count);

    give_semaphore( menu_sem );
}

static void menu_remove_entry(struct menu * menu, struct menu_entry * entry)
{
    if (entry == entry_being_updated)
    {
        entry_removed_itself = 1;
    }
    if (menu->children == entry)
    {
        menu->children = entry->next;
    }
    if (entry->prev)
    {
        // printf("link %s to %x\n", entry->prev->name, entry->next);
        entry->prev->next = entry->next;
    }
    if (entry->next)
    {
        // printf("link %s to %x\n", entry->next->name, entry->prev);
        entry->next->prev = entry->prev;
    }
    
    /* remove the submenu */
    /* fixme: won't work with composite submenus */
    if (entry->children)
    {
        struct menu * submenu = menu_find_by_name( entry->name, ICON_ML_SUBMENU);
        if (submenu)
        {
            // printf("unlink submenu %s\n", submenu->name);
            submenu->children = 0;
        }
    }
    
    /* look for placeholder */
    for (struct menu_entry * placeholder = entry->prev; placeholder; placeholder = placeholder->prev)
    {
        if (streq(placeholder->name, entry->name))
        {
            // printf("restore placeholder %s\n", entry->name);
            struct menu_entry restored_placeholder = MENU_PLACEHOLDER(placeholder->name);
            placeholder_copy(placeholder, &restored_placeholder);
            break;
        }
    }
}

void
menu_remove(
    const char *        name,
    struct menu_entry * old_entry,
    int         count
)
{
    if ( count == 0 )
        return;

    struct menu * menu = menu_find_by_name( name, 0);
    if( !menu )
        return;

    int removed = 0;

    struct menu_entry * entry = menu->children;
    while(entry && removed < count) {
        if (entry >= old_entry && entry < old_entry + count)
        {
            menu_remove_entry(menu, entry);
            removed++;
        }
        entry = entry->next;
    }
}


/* Usage counters
 * ==============
 * We use a long-term and a short-term counter
 * so if some item is frequently used during e.g. one day,
 * it should appear quickly in the auto menu,
 * and should disappear if no longer used in the following days,
 * but if some other item is used let's say once or twice every day,
 * it should also appear appear in the menu,
 * but it should not disappear if unused for 2-3 days.
 */

/* these act as a forgetting factor for all other entries */
/* except we only need to update the current entry in O(1) */
static float usage_counter_delta_long = 1.0;
static float usage_counter_delta_short = 1.0;

/* threshold for displaying the most used menu items */
/* submenu items will all end up in the * column, so we'll adjust the threshold to avoid clutter */
/* max value is used only for showing debug info */
static float usage_counter_thr = 0;
static float usage_counter_thr_sub = 0;
static float usage_counter_max = 0;

/* normalize the usage counters so the next increment is 1.0 */
static void menu_normalize_usage_counters(void)
{
    take_semaphore(menu_sem, 0);

    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            entry->usage_counter_long_term  /= usage_counter_delta_long;
            entry->usage_counter_short_term /= usage_counter_delta_short;
        }
    }
    usage_counter_delta_long  = 1.0;
    usage_counter_delta_short = 1.0;

    give_semaphore(menu_sem);
}

/* update usage counters for the selected entry */
/* (called when user clicks on it) */
static void menu_update_usage_counters(struct menu_entry * entry)
{
    take_semaphore(menu_sem, 0);

    if (!entry->parent_menu->selected)
    {
        /* not at home? use original entry */
        /* fixme: remove the lookup */
        entry = entry_find_by_name(entry->parent_menu->name, entry->name);
        if (!entry) goto end;
    }

    if (entry->parent_menu->no_name_lookup)
    {
        /* ignore this special menu */
        goto end;
    }

    /* selecting the same menu entry multiple times during a short time span
     * should count as one */
    static struct menu_entry * prev_entry = 0;
    static int prev_timestamp = 0;
    int ms_clock = get_ms_clock();
    if (entry == prev_entry)
    {
        int elapsed = ms_clock - prev_timestamp;
        if (elapsed < 5000)
        {
            /* same entry selected recently? skip it */
            prev_timestamp = ms_clock;
            goto end;
        }
    }
    prev_entry = entry;
    prev_timestamp = ms_clock;

    /* update increments (equivalent to adjusting the forgetting factors) */
    usage_counter_delta_long  *= 1.001;
    usage_counter_delta_short *= 1.1;

    /* update the counters for current entry */
    /* note: the increment is higher than for the older entries, */
    /* so it's as if those older entries used a forgetting factor */
    entry->usage_counter_long_term  += usage_counter_delta_long;
    entry->usage_counter_short_term += usage_counter_delta_short;

    /* update dirty flags */
    menu_flags_save_dirty = 1;
    my_menu_dirty = 1;

end:
    give_semaphore(menu_sem);
}

static void menu_usage_counters_update_threshold(int num, int only_submenu_entries, int only_nonsubmenu_entries)
{
    take_semaphore(menu_sem, 0);

    int num_entries = 0;

    /* count the menu items */
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->no_name_lookup)
            continue;

        if (only_submenu_entries && !IS_SUBMENU(menu))
            continue;

        if (only_nonsubmenu_entries && IS_SUBMENU(menu))
            continue;

        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            num_entries++;
        }
    }

    /* extract the usage counters into an array */
    /* and compute the max value too */
    float * counters = malloc(sizeof(float) * num_entries);
    if (!counters) goto end;

    int k = 0;
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->no_name_lookup)
            continue;

        if (only_submenu_entries && !IS_SUBMENU(menu))
            continue;

        if (only_nonsubmenu_entries && IS_SUBMENU(menu))
            continue;

        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            float counter = MAX(entry->usage_counter_long_term, entry->usage_counter_short_term);
            counters[k++] = counter;
            usage_counter_max = MAX(usage_counter_max, counter);
        }
    }

    /* sort the usage counters */
    /* we only need counters[num-1], so no need to sort the entire thing */
    for (int i = 0; i < MIN(num_entries, num); i++)
    {
        for (int j = i+1; j < num_entries; j++)
        {
            if (counters[i] < counters[j])
            {
                float aux = counters[i];
                counters[i] = counters[j];
                counters[j] = aux;
            }
        }
    }

    /* pick a threshold that selects the best "num" entries */
    float thr = (num > 0) ? MAX(counters[num-1], 0.01) : 1e5;

    if (!only_submenu_entries)
    {
        usage_counter_thr = thr;
    }

    if (!only_nonsubmenu_entries)
    {
        usage_counter_thr_sub = thr;
    }

    free(counters);

end:
    give_semaphore(menu_sem);
}

/*
 * Display routines
 */

static void dot(int x, int y, int color, int radius)
{
    fill_circle(x+16, y+16, radius, color);
}

#ifdef CONFIG_MENU_ICONS

static void maru(int x, int y, int color)
{
    dot(x, y, color, 10);
}

static void batsu(int x, int y, int c)
{
    int i;
    for (i = 1; i < 4; i++)
    {
        draw_line(x + 8 + i, y + 9, x + 21 + i, y + 22, c);
        draw_line(x + 21 + i, y + 9, x + 8 + i, y + 22, c);
    }
}

/*
static void crossout(int x, int y, int color)
{
    x += 16;
    y += 16;
    int r;
    for (r = 9; r < 10; r++)
    {
        int i = r-9;
        draw_circle(x,     y, r, color);
        draw_circle(x + 1, y, r, color);
        draw_line(x + 5 + i, y - 5, x - 5 + i, y + 5, color);
    }
}

static void percent(int x, int y, int value, int fg, int bg)
{
    int i;
    y -= 2;
    value = value * 28 / 100;
    for (i = 0; i < 28; i++)
        draw_line(x + 2 + i, y + 25, x + 2 + i, y + 25 - i/3 - 5,
            i <= value ? fg : bg
        );
}

static void clockmeter(int x, int y, int value, int fg, int bg)
{
    fill_circle(x+16, y+16, 10, bg);
    for (int a = 0; a <= value * 3600 / 100; a+=10)
        draw_angled_line(x+16, y+16, 10, a-900, fg);
    draw_circle(x+16, y+16, 10, fg);
}

static void clockmeter_half(int x, int y, int value, int fg, int bg)
{
    int thr = value * 1800 / 100;
    for (int a = 1800; a >=0 ; a-=5)
        draw_angled_line(x+16, y+21, 12, a-1800, a <= thr ? fg : bg);
}
*/

/*
static void clockmeter_pm(int x, int y, uint32_t arg, int fg, int bg)
{
    int value = (int8_t)(arg & 0xFF);
    int min = (int8_t)((arg>>8) & 0xFF);
    int max = (int8_t)((arg>>16) & 0xFF);
    
    int M = MAX(ABS(min), ABS(max));
    
    int thr = value * 1800 / M;
    for (int a = 0; a <= 1800; a+=5)
    {
        int v = a * M / 1800;
        if (v > max) break;
        draw_angled_line(x+16, y+21, 12, a-1800, a <= thr ? fg : bg);
    }
    for (int a = 0; a >= -1800; a-=5)
    {
        int v = a * M / 1800;
        if (v < min) break;
        draw_angled_line(x+16, y+21, 12, a-1800, a >= thr ? fg : bg);
    }
}
*/

static void playicon(int x, int y, int color)
{
    int i;
    for (i = 7; i < 32-7; i++)
    {
        draw_line(x + 9, y + i, x + 23, y + 16, color);
        draw_line(x + 9, y + i, x + 23, y + 16, color);
    }
}

static void leftright_sign(int x, int y)
{
    int i;
    for (i = 5; i < 32-5; i++)
    {
        draw_line(x + 3, y + i, x + 18 + 3, y + 16, COLOR_CYAN);
        draw_line(x + 3, y + i, x + 18 + 3, y + 16, COLOR_CYAN);

        draw_line(x - 3, y + i, x - 18 - 3, y + 16, COLOR_CYAN);
        draw_line(x - 3, y + i, x - 18 - 3, y + 16, COLOR_CYAN);
    }
}

/*
void submenu_icon(int x, int y)
{
    //~ int color = COLOR_WHITE;
    x -= MENU_OFFSET;
    //~ bmp_draw_rect(45, x+2, y+5, 32-3, 32-10+1);
    draw_line(x+20, y+28, x+30, y+28, COLOR_WHITE);
    for (int i = -2; i <= 2; i++)
    draw_line(x+26, y+28+i, x+30, y+28, COLOR_WHITE);
    //~ for (int r = 0; r < 2; r++)
    //~ {
        //~ draw_circle(x + 30, y + 28, r, color);
        //~ draw_circle(x + 23, y + 28, r, color);
        //~ draw_circle(x + 16, y + 28, r, color);
    //~ }
}
*/

/*
static void size_icon(int x, int y, int current, int nmax, int color)
{
    dot(x, y, color, COERCE(current * (nmax > 2 ? 9 : 7) / (nmax-1) + 3, 1, 12));
}

static void dice_icon(int x, int y, int current, int nmax, int color_on, int color_off)
{
    #define C(i) (current == (i) ? color_on : color_off), (current == (i) ? 6 : 4)
    //~ x -= 40;
    //~ x += 16; y += 16;
    switch (nmax)
    {
        case 2:
            dot(x - 5, y + 5, C(0)+1);
            dot(x + 5, y - 5, C(1)+1);
            break;
        case 3:
            dot(x    , y - 7, C(0));
            dot(x - 7, y + 3, C(1));
            dot(x + 7, y + 3, C(2));
            break;
        case 4:
            dot(x - 6, y - 6, C(0));
            dot(x + 6, y - 6, C(1));
            dot(x - 6, y + 6, C(2));
            dot(x + 6, y + 6, C(3));
            break;
        case 5:
            dot(x,     y,     C(0));
            dot(x - 8, y - 8, C(1));
            dot(x + 8, y - 8, C(2));
            dot(x + 8, y + 8, C(3));
            dot(x - 8, y + 8, C(4));
            break;
        case 6:
            dot(x - 10, y - 8, C(0));
            dot(x     , y - 8, C(1));
            dot(x + 10, y - 8, C(2));
            dot(x - 10, y + 8, C(3));
            dot(x     , y + 8, C(4));
            dot(x + 10, y + 8, C(5));
            break;
        case 7:
            dot(x - 10, y - 10, C(0));
            dot(x     , y - 10, C(1));
            dot(x + 10, y - 10, C(2));
            dot(x - 10, y + 10, C(3));
            dot(x     , y + 10, C(4));
            dot(x + 10, y + 10, C(5));
            dot(x     , y     , C(6));
            break;
        case 8:
            dot(x - 10, y - 10, C(0));
            dot(x     , y - 10, C(1));
            dot(x + 10, y - 10, C(2));
            dot(x - 10, y + 10, C(3));
            dot(x     , y + 10, C(4));
            dot(x + 10, y + 10, C(5));
            dot(x -  5, y     , C(6));
            dot(x +  5, y     , C(7));
            break;
        case 9:
            dot(x - 10, y - 10, C(0));
            dot(x     , y - 10, C(1));
            dot(x + 10, y - 10, C(2));
            dot(x - 10, y     , C(3));
            dot(x     , y     , C(4));
            dot(x + 10, y     , C(5));
            dot(x - 10, y + 10, C(6));
            dot(x     , y + 10, C(7));
            dot(x + 10, y + 10, C(8));
            break;
        default:
            size_icon(x, y, current, nmax, color_on);
            break;
    }
    #undef C
}

static void pizza_slice(int x, int y, int current, int nmax, int fg, int bg)
{
    dot(x, y, bg, 10);
    int a0 = current * 3600 / nmax;
    int w = 3600 / nmax;
    for (int a = a0-w/2; a < a0+w/2; a += 10)
    {
        draw_angled_line(x+16, y+16, 10, a-900, fg);
    }
}*/

/*
static void slider_box(int x, int y, int w, int h, int c)
{
    bmp_draw_rect_chamfer(c, x, y, w, h, 1, 0);
    bmp_fill(c, x+1, y+1, w-1, h-1);
}
static void hslider(int x, int y, int current, int nmax, int fg, int bg)
{
#define SW 30
#define SH 15
#define SO ((30-SH)/2)

    int w = MIN(SW / nmax, 10);
    int W = w * nmax;
    x += (SW-W)/2;
    
    for (int i = 0; i < nmax; i++)
    {
        int xc = x + i*w;
        slider_box(xc, y+SO, MAX(3, w-2), SH, bg);
    }
    int xc = x + current * w; 
    slider_box(xc, y+SO, MAX(3, w-2), SH, fg);

#undef SW
#undef SH
#undef SO
}

static void vslider(int x, int y, int current, int nmax, int fg, int bg)
{
#define SW 22
#define SH 16
#define SO ((30-SH)/2)

    if (nmax > 3)
    {
        y += (32-SW)/2;
        slider_box(x+SO, y, SH, SW, bg);
        int w = 8;
        int yc = y + COERCE(current, 0, nmax-1) * (SW-w) / (nmax-1);
        slider_box(x+SO, yc, SH, w, fg);
    }
    else
    {
        int w = MIN(SW / nmax, 10);
        int W = w * nmax;
        y += (32-W)/2;
        for (int i = 0; i < nmax; i++)
        {
            int yc = y + i*w;
            slider_box(x+SO, yc, SH, w-3, bg);
        }
        int yc = y + COERCE(current, 0, nmax-1) * w; 
        slider_box(x+SO, yc, SH, w-3, fg);
    }
    
#undef SW
#undef SH
#undef SO
}

#define slider vslider
*/

static void round_box(int c, int x, int y, int w, int h)
{
    bmp_draw_rect_chamfer(c, x-1, y-1, w+2, h+2, 2, 0);
    bmp_draw_rect_chamfer(c, x, y, w, h, 1, 0);
    if (w >= 4) bmp_fill(c, x+1, y+1, w-1, h-1);
}

static void round_box_meter(int x, int y, int value, int fg, int bg)
{
    value = COERCE(value, 0, 100);
    round_box(fg, x+8, y+8, 16, 16);
    int X = x+9 + 12 * value / 100;
    bmp_draw_rect(bg, X, y+10, 2, 12);
}

static void slider(int x, int y, int current, int nmax, int fg, int bg)
{
    if (nmax >= 2)
        round_box_meter(x, y, current * 100 / (nmax-1), fg, bg);
    else
        round_box(fg, x+8, y+8, 16, 16);
}

static void submenu_only_icon(int x, int y, int color)
{
    round_box(color, x+8, y+8, 16, 16);
    /*
    for (int r = 0; r < 3; r++)
    {
        draw_circle(x + 8, y + 10, r, color);
        draw_circle(x + 8, y + 16, r, color);
        draw_circle(x + 8, y + 22, r, color);
        draw_circle(x + 9, y + 10, r, color);
        draw_circle(x + 9, y + 16, r, color);
        draw_circle(x + 9, y + 22, r, color);
    }
    
    if (color == COLOR_GREEN1) color = COLOR_WHITE;
    bmp_draw_rect(color, x + 15, y + 10, 10, 1);
    bmp_draw_rect(color, x + 15, y + 16, 10, 1);
    bmp_draw_rect(color, x + 15, y + 22, 10, 1);
    */
}

/*
void color_icon(int x, int y, const char* color)
{
    if (streq(color, "Red"))
        maru(x, y, COLOR_RED);
    else if (streq(color, "Green"))
        maru(x, y, COLOR_GREEN2);
    else if (streq(color, "Blue"))
        maru(x, y, COLOR_LIGHT_BLUE);
    else if (streq(color, "Cyan"))
        maru(x, y, COLOR_CYAN);
    else if (streq(color, "Magenta"))
        maru(x, y, 14);
    else if (streq(color, "Yellow"))
        maru(x, y, COLOR_YELLOW);
    else if (streq(color, "Orange"))
        maru(x, y, COLOR_ORANGE);
    else if (streq(color, "White"))
        maru(x, y, COLOR_WHITE);
    else if (streq(color, "Black"))
        maru(x, y, COLOR_WHITE);
    else if (streq(color, "Luma") || streq(color, "Luma Fast"))
        maru(x, y, 60);
    else if (streq(color, "RGB"))
    {
        dot(x,     y - 7, COLOR_RED, 5);
        dot(x - 7, y + 3, COLOR_GREEN2, 5);
        dot(x + 7, y + 3, COLOR_LIGHT_BLUE, 5);
    }
    else if (streq(color, "ON"))
        maru(x, y, COLOR_GREEN1);
    else if (streq(color, "OFF"))
        maru(x, y, 40);
    else
    {
        dot(x,     y - 7, COLOR_CYAN, 5);
        dot(x - 7, y + 3, COLOR_RED, 5);
        dot(x + 7, y + 3, COLOR_YELLOW, 5);
    }
}
*/

#endif // CONFIG_MENU_ICONS

static void FAST selection_bar_backend(int c, int black, int x0, int y0, int w, int h)
{
    uint8_t* B = bmp_vram();
    #ifdef CONFIG_VXWORKS
    c = D2V(c);
    black = D2V(black);
    #endif
    #define P(x,y) B[BM(x,y)]
    for (int y = y0; y < y0 + h; y++)
    {
        for (int x = x0; x < x0 + w; x++)
        {
            if (P(x,y) == black)
                P(x,y) = c;
        }
    }
    // use a shadow for better readability, especially for gray text
    for (int y = y0; y < y0 + h; y++)
    {
        for (int x = x0; x < x0 + w; x++)
        {
            if (P(x,y) != c && P(x,y) != black)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        if (P(x+dx,y+dy) == c)
                            P(x+dx,y+dy) = black;
                    }
                }
            }
        }
    }
}

#ifdef CONFIG_MENU_DIM_HACKS
void FAST replace_color(int old, int new, int x0, int y0, int w, int h)
{
    uint8_t* B = bmp_vram();
    #ifdef CONFIG_VXWORKS
    old = D2V(old);
    new = D2V(new);
    #endif
    #define P(x,y) B[BM(x,y)]
    for (int y = y0; y < y0 + h; y++)
    {
        for (int x = x0; x < x0 + w; x++)
        {
            if (P(x,y) == old)
                P(x,y) = new;
        }
    }
}
void FAST dim_screen(int fg, int bg, int x0, int y0, int w, int h)
{
    uint8_t* B = bmp_vram();
    #ifdef CONFIG_VXWORKS
    bg = D2V(bg);
    fg = D2V(fg);
    #endif
    #define P(x,y) B[BM(x,y)]
    for (int y = y0; y < y0 + h; y++)
    {
        for (int x = x0; x < x0 + w; x++)
        {
            if (P(x,y) != bg)
            {
                if (P(x,y) >= COLOR_ALMOST_BLACK && P(x,y) < fg) continue;
                P(x,y) = fg;
            }
        }
    }
}
#endif


// By default, icon type is MNI_BOOL(MENU_INT(entry))
// To override, call menu_draw_icon from the display functions

// Icon is only drawn once for each menu item, even if this is called multiple times
// Only the first call is executed

static int icon_drawn = 0;
static void menu_draw_icon(int x, int y, int type, intptr_t arg, int warn)
{
    if (icon_drawn) return;
    
    icon_drawn = type;

#ifdef CONFIG_MENU_ICONS

    x -= MENU_OFFSET;
    
    int color_on = warn ? COLOR_DARK_GREEN1_MOD : COLOR_GREEN1;
    int color_off = 45;
    int color_dis = warn ? 50 : COLOR_RED;
    int color_slider_fg = warn ? COLOR_DARK_CYAN1_MOD : COLOR_CYAN;
    int color_slider_bg = warn ? COLOR_BLACK : 45;
    int color_slider_off_fg = warn ? COLOR_DARK_GREEN1_MOD : COLOR_GREEN1;
    int color_action = warn ? 45 : COLOR_WHITE;

    switch(type)
    {
        case MNI_OFF: maru(x, y, color_off); return;
        case MNI_ON: maru(x, y, color_on); return;
        case MNI_DISABLE: batsu(x, y, color_dis); return;
        case MNI_NEUTRAL: maru(x, y, 55); return;
        case MNI_AUTO: slider(x, y, 0, 0, color_slider_fg, color_slider_bg); return;
        case MNI_PERCENT: round_box_meter(x, y, arg, color_slider_fg, color_slider_bg); return;
        case MNI_PERCENT_ALLOW_OFF: round_box_meter(x, y, arg, color_slider_off_fg, color_slider_bg); return;
        case MNI_PERCENT_OFF: round_box_meter(x, y, arg, color_off, color_off); return;
        //~ case MNI_PERCENT_PM: clockmeter_pm(x, y, arg, color_slider_fg, color_slider_bg); return;
        case MNI_ACTION: playicon(x, y, color_action); return;
        case MNI_DICE:
        {
            int i = arg & 0xFFFF;
            int N = arg >> 16;
            slider(x, y, i, N, color_slider_fg, color_slider_bg); return;
        }

        case MNI_DICE_OFF:
        {
            int i = arg & 0xFFFF;
            int N = arg >> 16;

            //~ maru(x, y, i ? color_on : color_off); return;
            
            //~ if (i == 0) dice_icon(x, y, i-1, N-1, 40, 40);
            //~ else dice_icon(x, y, i-1, N-1, COLOR_GREEN1, 50);
            if (i == 0) //maru(x, y, color_off);
                slider(x, y, i-1, N-1, color_off, color_off);
            else
                slider(x, y, i-1, N-1, color_slider_off_fg, color_slider_bg);

            return;
        }
        //~ case MNI_SIZE: //size_icon(x, y, arg & 0xFFFF, arg >> 16, color_slider_fg); return;
        //~ {
            //~ int i = arg & 0xFFFF;
            //~ int N = arg >> 16;
            //~ round_box_meter(x, y, i*100/(N-1), color_slider_fg, color_slider_bg);
            //~ return;
        //~ }
        //~ case MNI_NAMED_COLOR:
        //~ {
            //~ if (warn) maru(x, y, color_on);
            //~ else color_icon(x, y, (char *)arg); 
            //~ return;
        //~ }
        case MNI_RECORD:
            maru(x, y, COLOR_RED);
            return;
            
        case MNI_SUBMENU: submenu_only_icon(x, y, arg ? color_on : color_off); return;
    }
#endif
}

// if the help text contains more lines (separated by '\n'), display the i'th line
// if line number is too high, display the first line


static char* menu_help_get_line(const char* help, int line, char buf[MENU_MAX_HELP_LEN])
{
    char * p = strchr(help, '\n');
    if (!p) return (char*) help; // help text contains a single line, no more fuss

    // help text contains more than one line, choose the i'th one
    int i = line;
    if (i < 0) i = 0;
    
    char* start = (char*) help;
    char* end = p;
    
    while (i > 0) // we need to skip some lines
    {
        if (*end == 0) // too many lines skipped? fall back to first line
        {
            start = (char*) help;
            end = p;
            break;
        }
        
        // there are more lines, skip to next one
        start = end + 1;
        end = strchr(start+1, '\n');
        if (!end) end = (char*) help + strlen(help);
        i--;
    }
    
    // return the substring from "start" to "end"
    int len = MIN(MENU_MAX_HELP_LEN, end - start + 1);
    snprintf(buf, len, "%s", start);
    return buf;
}

static char* pickbox_string(struct menu_entry * entry, int i)
{
    if (entry->choices) return (char*) entry->choices[i - entry->min];
    if (entry->min == 0 && entry->max == 1) return i ? "ON" : "OFF";

    // not configured; just use some reasonable defaults
    static char msg[20];
    snprintf(msg, sizeof(msg), "%d", i);
    return msg;
}
static void pickbox_draw(struct menu_entry * entry, int x0, int y0)
{
    int lo = entry->min;
    int hi = entry->max;
    int sel = SELECTED_INDEX(entry) + lo;
    int fnt = SHADOW_FONT(FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK));
    
    // don't draw too many items in the pickbox
    if (hi - lo > 10)
    {
        lo = MAX(lo, sel - (sel < hi ? 9 : 10));
        hi = MIN(hi, lo + 10);
    }
    
    // compute the width of the pickbox (what's the longest string?)
    int w = 100;
    for (int i = lo; i <= hi; i++)
    {
        w = MAX(w, font_large.width * strlen(pickbox_string(entry, i)));
    }

    // don't draw the pickbox out of the screen
    int h = 32 * (hi-lo+1);
    
    /*#define SUBMENU_HINT_SUFFIX ": advanced..."
    if (entry->children)
    {
        h += 20; // has submenu, display a hint here
        w = MAX(w, font_med.width * (strlen(Q_BTN_NAME) + strlen(SUBMENU_HINT_SUFFIX)));
    }*/
    
    if (y0 + h > 410)
        y0 = 410 - h;

    if (x0 + w > 700)
        x0 = 700 - w;
    
    w = 720-x0+16; // extend it till the right edge
    
    // draw the pickbox
    bmp_fill(45, x0-16, y0, w, h+1);
    for (int i = lo; i <= hi; i++)
    {
        int y = y0 + (i-lo) * 32;
        if (i == sel)
            selection_bar_backend(MENU_BAR_COLOR, 45, x0-16, y, w, 32);
        bmp_printf(fnt, x0, y, pickbox_string(entry, i));
    }
    
    /*
    if (entry->children)
        bmp_printf(
            SHADOW_FONT(FONT(FONT_MED, COLOR_CYAN, 45)), 
            x0, y0 + (hi-lo+1) * 32 + 5, 
            "%s" SUBMENU_HINT_SUFFIX,
            Q_BTN_NAME
        );*/
}

static void submenu_key_hint(int x, int y, int fg, int bg, int chr)
{
    bmp_fill(bg, x+12, y+1, 25, 30);
    bfnt_draw_char(chr, x, y-5, fg, NO_BG_ERASE);
}

static void menu_clean_footer()
{
    int h = 50;
    if (is_menu_active("Help")) h = font_med.height * 3 + 2;
    int bgu = MENU_BG_COLOR_HEADER_FOOTER;
    bmp_fill(bgu, 0, 480-h, 720, h);
}

static int check_default_warnings(struct menu_entry * entry, char* warning)
{
    warning[0] = 0;
    
    /* all submenu entries depend on the master entry, if any */
    if (IS_SUBMENU(entry->parent_menu))
    {
        if (entry->parent && IS_ML_PTR(entry->parent->priv) &&  /* does it have a parent with a valid priv field? */
            entry->parent->priv != entry->priv)         /* priv different from our own? (cannot depend on itself) */
        {
            if (!MENU_INT(entry->parent))   /* is the master menu entry disabled? if so, gray out the entire submenu */
            {
                int is_plural = entry->parent->name[strlen(entry->parent->name)-1] == 's';
                snprintf(warning, MENU_MAX_WARNING_LEN, "%s %s disabled.", entry->parent->name, is_plural ? "are" : "is");
                return MENU_WARN_NOT_WORKING;
            }
        }
    }
    
    // default warnings
         if (DEPENDS_ON(DEP_GLOBAL_DRAW) && !get_global_draw())
        snprintf(warning, MENU_MAX_WARNING_LEN, GDR_WARNING_MSG);
    else if (DEPENDS_ON(DEP_MOVIE_MODE) && !is_movie_mode())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature only works in movie mode.");
    else if (DEPENDS_ON(DEP_PHOTO_MODE) && is_movie_mode())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature only works in photo mode.");
    else if (DEPENDS_ON(DEP_LIVEVIEW) && !lv)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature only works in LiveView.");
    else if (DEPENDS_ON(DEP_NOT_LIVEVIEW) && lv)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature does not work in LiveView.");
    else if (DEPENDS_ON(DEP_AUTOFOCUS) && is_manual_focus())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires autofocus enabled.");
    else if (DEPENDS_ON(DEP_MANUAL_FOCUS) && !is_manual_focus())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires manual focus.");
    else if (DEPENDS_ON(DEP_CFN_AF_HALFSHUTTER) && cfn_get_af_button_assignment() != AF_BTN_HALFSHUTTER && !is_manual_focus())
        snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to Half-Shutter from Canon menu (CFn / custom ctrl), or use MF.");
#if !defined(CONFIG_NO_HALFSHUTTER_AF_IN_LIVEVIEW)
    else if (DEPENDS_ON(DEP_CFN_AF_BACK_BUTTON) && cfn_get_af_button_assignment() == AF_BTN_HALFSHUTTER && !is_manual_focus())
        snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to back btn (*) from Canon menu (CFn / custom ctrl), or use MF.");
#endif
    else if (DEPENDS_ON(DEP_EXPSIM) && lv && !lv_luma_is_accurate())
        snprintf(warning, MENU_MAX_WARNING_LEN, EXPSIM_WARNING_MSG);
    //~ else if (DEPENDS_ON(DEP_NOT_EXPSIM) && lv && lv_luma_is_accurate())
        //~ snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires ExpSim disabled.");
    else if (DEPENDS_ON(DEP_MANUAL_FOCUS) && !is_manual_focus())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires manual focus.");
    else if (DEPENDS_ON(DEP_CHIPPED_LENS) && !lens_info.lens_exists)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires a chipped (electronic) lens.");
    else if (DEPENDS_ON(DEP_M_MODE) && shooting_mode != SHOOTMODE_M)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires Manual (M) mode.");
    else if (DEPENDS_ON(DEP_MANUAL_ISO) && !lens_info.raw_iso)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires manual ISO.");
    else if (DEPENDS_ON(DEP_SOUND_RECORDING) && !sound_recording_enabled())
        snprintf(warning, MENU_MAX_WARNING_LEN, (was_sound_recording_disabled_by_fps_override() && !fps_should_record_wav()) ? 
            "Sound recording was disabled by FPS override." :
            "Sound recording is disabled. Enable it from Canon menu."
        );
    else if (DEPENDS_ON(DEP_NOT_SOUND_RECORDING) && sound_recording_enabled())
        snprintf(warning, MENU_MAX_WARNING_LEN, "Disable sound recording from Canon menu!");
    
    if (warning[0]) 
        return MENU_WARN_NOT_WORKING;
    
    if (entry->selected) // check recommendations
    {
             if (WORKS_BEST_IN(DEP_GLOBAL_DRAW) && !get_global_draw())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with GlobalDraw enabled.");
        else if (WORKS_BEST_IN(DEP_MOVIE_MODE) && !is_movie_mode())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best in movie mode.");
        else if (WORKS_BEST_IN(DEP_PHOTO_MODE) && is_movie_mode())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best in photo mode.");
        else if (WORKS_BEST_IN(DEP_LIVEVIEW) && !lv)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best in LiveView.");
        else if (WORKS_BEST_IN(DEP_NOT_LIVEVIEW) && lv)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best outside LiveView.");
        else if (WORKS_BEST_IN(DEP_AUTOFOCUS) && is_manual_focus())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with autofocus enabled.");
        else if (WORKS_BEST_IN(DEP_MANUAL_FOCUS) && !is_manual_focus())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with manual focus.");
        else if (WORKS_BEST_IN(DEP_CFN_AF_HALFSHUTTER) && cfn_get_af_button_assignment() != AF_BTN_HALFSHUTTER && !is_manual_focus())
            snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to Half-Shutter from Canon menu (CFn / custom ctrl).");
#if !defined(CONFIG_NO_HALFSHUTTER_AF_IN_LIVEVIEW)
        else if (WORKS_BEST_IN(DEP_CFN_AF_BACK_BUTTON) && cfn_get_af_button_assignment() == AF_BTN_HALFSHUTTER && !is_manual_focus())
            snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to back btn (*) from Canon menu (CFn / custom ctrl).");
#endif
        //~ else if (WORKS_BEST_IN(DEP_EXPSIM) && lv && !lv_luma_is_accurate())
            //~ snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with ExpSim enabled.");
        //~ else if (WORKS_BEST_IN(DEP_NOT_EXPSIM) && lv && lv_luma_is_accurate())
            //~ snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with ExpSim disabled.");
        //~ else if (WORKS_BEST_IN(DEP_CHIPPED_LENS) && !lens_info.name[0])
            //~ snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with a chipped (electronic) lens.");
        else if (WORKS_BEST_IN(DEP_M_MODE) && shooting_mode != SHOOTMODE_M)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best in Manual (M) mode.");
        else if (WORKS_BEST_IN(DEP_MANUAL_ISO) && !lens_info.raw_iso)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with manual ISO.");
        //~ else if (WORKS_BEST_IN(DEP_SOUND_RECORDING) && !sound_recording_enabled())
            //~ snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with sound recording enabled.");
        //~ else if (WORKS_BEST_IN(DEP_NOT_SOUND_RECORDING) && sound_recording_enabled())
            //~ snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with sound recording disabled.");
        
        if (warning[0]) 
            return MENU_WARN_ADVICE;
    }
    
    return MENU_WARN_NONE;
}

static void
entry_default_display_info(
    struct menu_entry * entry,
    struct menu_display_info * info
    )
{
    static char name[MENU_MAX_NAME_LEN];
    static char short_name[MENU_MAX_SHORT_NAME_LEN];
    static char value[MENU_MAX_VALUE_LEN];
    static char short_value[MENU_MAX_SHORT_VALUE_LEN];
    static char help[MENU_MAX_HELP_LEN];
    static char warning[MENU_MAX_WARNING_LEN];
    static char rinfo[MENU_MAX_RINFO_LEN];

    name[0] = 0;
    short_name[0] = 0;
    value[0] = 0;
    short_value[0] = 0;
    help[0] = 0;
    warning[0] = 0;
    rinfo[0] = 0;
    
    info->name = name;
    info->short_name = short_name;
    info->value = value;
    info->short_value = short_value;
    info->help = help;
    info->warning = warning;
    info->rinfo = rinfo;
    info->custom_drawing = 0;
    info->can_custom_draw = 1;
    info->icon = 0;
    info->icon_arg = 0;

    info->enabled = entry_guess_enabled(entry);
    info->warning_level = check_default_warnings(entry, warning);
    
    snprintf(name, sizeof(name), "%s", entry->name);

    if (entry->choices && SELECTED_INDEX(entry) >= 0 && SELECTED_INDEX(entry) < NUM_CHOICES(entry))
    {
        STR_APPEND(value, "%s", entry->choices[SELECTED_INDEX(entry)]);
    }

    else if (IS_ML_PTR(entry->priv) &&
            entry->icon_type != IT_ACTION &&    /* no default value for actions */
            entry->icon_type != IT_SUBMENU)     /* no default value for submenus */
    {
        if (entry->min == 0 && entry->max == 1)
        {
            STR_APPEND(value, MENU_INT(entry) ? "ON" : "OFF");
        }
        else
        {
            switch (entry->unit)
            {
                case UNIT_1_8_EV:
                case UNIT_x10:
                case UNIT_PERCENT_x10:
                {
                    int v = MENU_INT(entry);
                    int den = entry->unit == UNIT_1_8_EV ? 8 : 10;
                    STR_APPEND(value, "%s%d", v < 0 ? "-" : "", ABS(v)/den);
                    int r = (ABS(v)%den)*10/den;
                    if (r) STR_APPEND(value, ".%d", r);
                    STR_APPEND(value, "%s",
                        entry->unit == UNIT_1_8_EV ? " EV" :
                        entry->unit == UNIT_PERCENT_x10 ? "%" : ""
                    );
                    break;
                }
                case UNIT_PERCENT:
                {
                    STR_APPEND(value, "%d%%", MEM(entry->priv));
                    break;
                }
                case UNIT_ISO:
                {
                    if (!MEM(entry->priv)) { STR_APPEND(value, "Auto"); }
                    else { STR_APPEND(value, "%d", raw2iso(MEM(entry->priv))); }
                    break;
                }
                case UNIT_DEC:
                {
                    if(edit_mode)
                    {
                        char* zero_pad = "00000000";
                        STR_APPEND(value, "%s%d", (zero_pad + COERCE(8-(caret_position - log10i(MEM(entry->priv))),0,8)), MEM(entry->priv));
                    }
                    else
                    {
                        STR_APPEND(value, "%d", MEM(entry->priv));
                    }
                    break;
                }
                case UNIT_HEX:
                {
                    if(edit_mode)
                    {
                        char* zero_pad = "00000000";
                        STR_APPEND(value, "0x%s%x", (zero_pad + COERCE(8-(caret_position - log2i(MEM(entry->priv))/4),0,8)), MEM(entry->priv));
                    }
                    else
                    {
                        STR_APPEND(value, "0x%x", MEM(entry->priv));
                    }
                    break;
                }
                case UNIT_TIME:
                {
                    if(MEM(entry->priv) / 3600 > 0 || (entry->selected && caret_position > 5))
                    {
                        STR_APPEND(value,"%dh%02dm%02ds", MEM(entry->priv) / 3600, MEM(entry->priv) / 60 % 60, MEM(entry->priv) % 60);
                    }
                    else if((entry->selected && caret_position > 4))
                    {
                        STR_APPEND(value,"%02dm%02ds", MEM(entry->priv) / 60, MEM(entry->priv) % 60);
                    }
                    else if(MEM(entry->priv) / 60 > 0 || (entry->selected && caret_position > 2))
                    {
                        STR_APPEND(value,"%dm%02ds", MEM(entry->priv) / 60, MEM(entry->priv) % 60);
                    }
                    else if((entry->selected && caret_position > 1))
                    {
                        STR_APPEND(value,"%02ds", MEM(entry->priv) % 60);
                    }
                    else
                    {
                        STR_APPEND(value,"%ds", MEM(entry->priv));
                    }
                    break;                    
                }
                case UNIT_TIME_MS:
                case UNIT_TIME_US:
                {
                    if(edit_mode)
                    {
                        char* zero_pad = "00000000";
                        STR_APPEND(value, "%s%d", (zero_pad + COERCE(8-(caret_position - log10i(MEM(entry->priv))),0,8)), MEM(entry->priv));
                    }
                    else
                    {
                        if (entry->unit == UNIT_TIME_MS) {
                            STR_APPEND(value, "%d ms", MEM(entry->priv));
                        } else {
                            STR_APPEND(value, "%d " SYM_MICRO "s", MEM(entry->priv));
                        }
                    }
                    break;
                }
                default:
                {
                    STR_APPEND(value, "%d", MEM(entry->priv));
                    break;
                }
            }
        }
    }
}

static inline int get_customize_color()
{
    return COLOR_DARK_ORANGE_MOD;
}

static void display_customize_marker(struct menu_entry * entry, int x, int y)
{
    // star marker
    if (entry->starred)
        bfnt_draw_char(ICON_ML_MYMENU, x, y-4, COLOR_GREEN1, NO_BG_ERASE);
    
    // hidden marker
    else if (HAS_CURRENT_HIDDEN_FLAG(entry))
        batsu(x+4, y, junkie_mode ? COLOR_ORANGE : COLOR_RED);
}

static void print_help_line(int color, int x, int y, char* msg)
{
    int fnt = FONT(FONT_MED, color, MENU_BG_COLOR_HEADER_FOOTER);
    int len = bmp_string_width(fnt, msg);
    
    if (len > 720 - 2*x)
    {
        /* squeeze long lines (if the help text is just a bit too long) */
        /* TODO: detect if squeezing fails and beep or print the help line in red; requires changes in the font backend */
        fnt |= FONT_ALIGN_JUSTIFIED | FONT_TEXT_WIDTH(720 - 2*x);
    }
    
    bmp_printf(
        fnt, x, y,
        "%s", msg
    );
}

static void
entry_print(
    int x,
    int y,
    int w,
    int h,
    struct menu_entry * entry,
    struct menu_display_info * info,
    int in_submenu
)
{
    int w0 = w;
    
    int fnt = MENU_FONT;

    if (info->warning_level == MENU_WARN_NOT_WORKING) 
        fnt = MENU_FONT_GRAY;
    
    if (submenu_level && !in_submenu)
        fnt = MENU_FONT_GRAY;
    
    int use_small_font = 0;
    int x_font_offset = 0;
    int y_font_offset = (h - (int)font_large.height) / 2;
    
    int not_at_home = 
            !entry->parent_menu->selected &&     /* is it in some dynamic menu? (not in its original place) */
            !submenu_level &&                     /* hack: submenus are not marked as "selected", so we can't have dynamic submenus for now */
            1;
    
    /* do not show right-side info in dynamic menus (looks a little tidier) */
    if (not_at_home)
        info->rinfo[0] = 0;
    
    if (
            not_at_home &&                       /* special display to show where it's coming from */
            IS_SUBMENU(entry->parent_menu) &&    /* if it's from a top-level menu, it's obvious where it's coming from */
            !edit_mode &&                        /* show unmodified entry when editing */
       1)
    {
        /* use a smaller font */
        use_small_font = 1;
        x_font_offset = 28;
        fnt = (fnt & ~FONT_MASK) | FONT_MED_LARGE;
        y_font_offset = (h - (int)fontspec_font(fnt)->height) / 2;

        /* in My Menu and Recent menu, we will include the submenu name in the original entry */
        if (my_menu->selected)// || mru_menu->selected)
        {
            /* how much space we have to print our stuff? (we got some extra because of the smaller font) */
            int max_len = w;
            int current_len = bmp_string_width(fnt, info->name);
            int extra_len = bmp_strlen_clipped(fnt, entry->parent_menu->name, max_len - current_len - 50);

            /* try to modify the name to show where it's coming from */
            char new_name[100];
            new_name[0] = 0;
            
            if (extra_len >= 5)
            {
                /* we have some space to show the menu where the original entry is coming from */
                /* (or at least some part of it) */
                snprintf(new_name, MIN(extra_len + 1, sizeof(new_name)), "%s", entry->parent_menu->name);
                STR_APPEND(new_name, " - ");
            }

            /* print the original name */
            STR_APPEND(new_name, "%s", info->name);

            /* if it's too long, add some dots */
            if ((int)strlen(new_name) > max_len)
            {
                new_name[max_len-1] = new_name[max_len-2] = new_name[max_len-3] = '.';
                new_name[max_len] = 0;
            }

            bmp_printf(
                fnt,
                x, y + y_font_offset,
                new_name
            );
            
            /* don't indent */
            x_font_offset = 0;
            
            /* don't print the name in the normal way */
            goto skip_name;
        }
    }

    bmp_printf(
        fnt,
        x + x_font_offset, y + y_font_offset,
        info->name
    );

skip_name:

    // debug
    if (0)
        bmp_printf(FONT_SMALL, x, y, "name(%s)(%d) value(%s)(%d)", info->name, strlen(info->name), info->value, strlen(info->value));

    if (info->enabled == 0) 
        fnt = MENU_FONT_GRAY;
    
    if (use_small_font)
        fnt = (fnt & ~FONT_MASK) | FONT_MED_LARGE;
    
    // far right end
    int x_end = in_submenu ? x + g_submenu_width - SUBMENU_OFFSET : 717;
    
    int char_width = fontspec_font(fnt)->width;
    w = MAX(w, bmp_string_width(fnt, info->name) + char_width);

    // both submenu marker and value? make sure they don't overlap
    if (entry->icon_type == IT_SUBMENU && info->value[0])
        w += 2 * char_width;
    
    // value string too big? move it to the left
    int val_width = bmp_string_width(fnt, info->value);
    int end = w + val_width;
    int wmax = x_end - x;

    // right-justified info field?
    int rlen = bmp_string_width(fnt, info->rinfo);
    int rinfo_x = x_end - rlen - 35;
    if (rlen) wmax -= rlen + char_width + 35;
    
    // no right info? then make sure there's room for the Q symbol
    else if (entry->children && !in_submenu && !menu_lv_transparent_mode && (entry->priv || entry->select))
    {
        wmax -= 35;
    }
    
    if (end > wmax)
        w -= (end - wmax);
    
    int xval = x + w;

    // value overlaps name? show value only (overwrite the name)
    if (xval < x + bmp_string_width(fnt, info->name))
    {
        xval = x;
    }

    if (entry->selected && 
        editing_with_caret(entry) && 
        caret_position >= (int)strlen(info->value))
    {
        bmp_fill(COLOR_WHITE, xval, y + fontspec_font(fnt)->height - 4, char_width, 2);
        xval += char_width * (caret_position - strlen(info->value) + 1);
    }

    // print value field
    bmp_printf(
        fnt,
        xval, y + y_font_offset,
        "%s",
        info->value
    );
    
    if(entry->selected &&
       editing_with_caret(entry) &&
       caret_position < (int)strlen(info->value) &&
       strlen(info->value) > 0)
    {
        int w1 = bmp_string_width(fnt, (info->value + strlen(info->value) - caret_position));
        int w2 = bmp_string_width(fnt, (info->value + strlen(info->value) - caret_position - 1));
        bmp_fill(COLOR_WHITE, xval + val_width - w2, y + fontspec_font(fnt)->height - 4, w2 - w1, 2);
    }

    // print right-justified info, if any
    if (info->rinfo[0])
    {
        bmp_printf(
            MENU_FONT_GRAY,
            rinfo_x, y + y_font_offset,
            "%s",
            info->rinfo
        );
    }

    int y_icon_offset = (h - 32) / 2 - 1;

    if (entry->icon_type == IT_SUBMENU )
    {
        // Forward sign for submenus that open with SET
        submenu_key_hint(
            xval-18 - (info->value[0] ? font_large.width*2 : 0), y + y_icon_offset, 
            info->warning_level == MENU_WARN_NOT_WORKING ? MENU_FONT_GRAY : 60, 
            COLOR_BLACK, 
            ICON_ML_FORWARD
        );
    }
    else if (entry->children && !SUBMENU_OR_EDIT && !menu_lv_transparent_mode)
    {
        // Q sign for selected item, if submenu opens with Q
        // Discrete placeholder for non-selected item
        if (entry->selected)
            submenu_key_hint(720-40, y + y_icon_offset, COLOR_WHITE, COLOR_BLACK, ICON_ML_Q_FORWARD);
        else
            submenu_key_hint(720-35, y + y_icon_offset, 40, COLOR_BLACK, ICON_ML_FORWARD);
    }

    if (my_menu->selected && streq(my_menu->name, "Recent") && !junkie_mode)
    {
        /* debug info: show usage counters as small bars */
        int bar_color = entry->selected ? COLOR_LIGHT_BLUE : COLOR_GRAY(5);
        selection_bar_backend(bar_color, COLOR_BLACK, 580, y + 10, entry->usage_counter_long_term * 100 / usage_counter_max, 5);
        selection_bar_backend(bar_color, COLOR_BLACK, 580, y + 18, entry->usage_counter_short_term * 100 / usage_counter_max, 5);
    }

    // selection bar params
    int xl = x - 5 + x_font_offset;
    int xc = x - 5 + x_font_offset;

    if ((in_submenu || edit_mode) && info->value[0])
    {
        /* highlight value field */
        xc = MAX(xl, xval - 15);
    }

    // selection bar
    if (entry->selected)
    {
        int color_left = 45;
        int color_right = MENU_BAR_COLOR;
        if (junkie_mode && !in_submenu) color_left = color_right = COLOR_BLACK;
        if (customize_mode) { color_left = color_right = get_customize_color(); }

        selection_bar_backend(color_left, COLOR_BLACK, xl, y, xc-xl, h-1);
        selection_bar_backend(color_right, COLOR_BLACK, xc, y, x_end-xc, h-1);
        
        // use a pickbox if possible
        if (edit_mode && CAN_HAVE_PICKBOX(entry))
        {
            int px = x + w0;
            pickbox_draw(entry, px, y);
        }
    }

    // display help
    if (entry->selected && !menu_lv_transparent_mode)
    {
        char help1_buf[MENU_MAX_HELP_LEN];
        char help2_buf[MENU_MAX_HELP_LEN];
        int help_color = 70;
        
        /* overriden help will go in first free slot */
        char* help1 = (char*)entry->help;
        if (entry->help)
        {
            /* if there are multiple help lines, pick the one that matches current choice */
            help1 = menu_help_get_line(entry->help, SELECTED_INDEX(entry), help1_buf);
        }
        else
        {
            /* no help1? put overriden help (via MENU_SET_HELP) here */
            help1 = info->help;
        }

        if (help1)
        {
            print_help_line(help_color, 10, MENU_HELP_Y_POS, help1);
        }

        char* help2 = 0;
        if (help1 != info->help && info->help && info->help[0])
        {
            /* help1 already used for something else?
             * put overriden help (via MENU_SET_HELP) here */
            help2 = info->help;
        }
        else if (entry->help2)
        {
            /* pick help line according to selected choice */
            help2 = menu_help_get_line(entry->help2, SELECTED_INDEX(entry), help2_buf);
        }
        
        if (!help2 || !help2[0]) // default help just list the choices
        {
            int num = NUM_CHOICES(entry);
            if (num > 2 && num < 10)
            {
                help2_buf[0] = 0;
                for (int i = entry->min; i <= entry->max; i++)
                {
                    int len = bmp_string_width(FONT_MED, help2_buf);
                    if (len > 700) break;
                    STR_APPEND(help2_buf, "%s%s", pickbox_string(entry, i), i < entry->max ? " / " : ".");
                }
                help_color = 50;
                help2 = help2_buf;
            }
        }

        /* only show the second help line if there are no audio meters */
        if (help2 && !audio_meters_are_drawn())
        {
            print_help_line(help_color, 10, MENU_HELP_Y_POS_2, help2);
        }
    }

    // if there's a warning message set, display it
    if (entry->selected && info->warning[0])
    {
        int warn_color = 
            info->warning_level == MENU_WARN_INFO ? COLOR_GREEN1 : 
            info->warning_level == MENU_WARN_ADVICE ? COLOR_YELLOW : 
            info->warning_level == MENU_WARN_NOT_WORKING ? COLOR_ORANGE : COLOR_WHITE;
        
        int warn_y = audio_meters_are_drawn() ? MENU_HELP_Y_POS : MENU_WARNING_Y_POS;
        
        bmp_fill(MENU_BG_COLOR_HEADER_FOOTER, 10, warn_y, 720, font_med.height);
        print_help_line(warn_color, 10, warn_y, info->warning);
    }
    
    /* from now on, we'll draw the icon only, which should be shifted */
    x += x_font_offset;
    y += y_icon_offset - 1;

    // customization markers
    if (customize_mode)
    {
        display_customize_marker(entry, x - 44, y);
        return; // do not display icons
    }

    // warning icon, if any
    int warn = (info->warning_level == MENU_WARN_NOT_WORKING);

    // overriden icon has the highest priority
    if (info->icon)
        menu_draw_icon(x, y, info->icon, info->icon_arg, warn);
    
    if (entry->icon_type == IT_BOOL)
        menu_draw_icon(x, y, info->enabled ? MNI_ON : MNI_OFF, 0, warn);
    
    entry_draw_icon(entry, x, y, info->enabled, warn);
}

static void
menu_post_display()
{
    char* cfg_preset = get_config_preset_name();
    if (cfg_preset && !submenu_level)
    {
        bmp_printf(
            SHADOW_FONT(FONT(FONT_MED, COLOR_GRAY(40), COLOR_BLACK)) | FONT_ALIGN_RIGHT,
            715, 480-50-font_med.height+4,
            "%s", cfg_preset
        );
    }

    if (!CURRENT_GUI_MODE)
    {
        // we can't use the scrollwheel
        // and you need to be careful because you will change shooting settings while recording!
        #if defined(CONFIG_DIGIC_678)
        // SJE FIXME we can't use ICON_MAINDIAL as that's in Canon bitmap font
        // and Digic >= 7 doesn't have it.  So I substitute a different icon.
        // A better fix might be to make our own dial icon and add it to ico.c,
        // then we could use the same code on all cams.
        bfnt_draw_char(ICON_ML_MODIFIED, 680, 395, MENU_WARNING_COLOR, NO_BG_ERASE);
        #else
        bfnt_draw_char(ICON_MAINDIAL, 680, 395, MENU_WARNING_COLOR, NO_BG_ERASE);
        #endif
        draw_line(720, 405, 680, 427, MENU_WARNING_COLOR);
        draw_line(720, 406, 680, 428, MENU_WARNING_COLOR);
    }

    // display help about how to customize the menu
    if (customize_mode)
    {
        bmp_printf(
            FONT(FONT_MED, get_customize_color(), MENU_BG_COLOR_HEADER_FOOTER),
             5,  MENU_HELP_Y_POS_2, 
                //~ CUSTOMIZE_MODE_HIDING ? "Press SET to show/hide items you don't use.                 " :
                //~ CUSTOMIZE_MODE_MYMENU ? "Press SET to choose your favorite items for MyMenu.         " : ""
                "Press SET to choose MyMenu items or hide what you don't use."
        );
    }
}

static int
menu_entry_process(
    struct menu * menu,
    struct menu_entry * entry,
    int         x,
    int         y,
    int         h,
    int only_selected
)
{
    // fill in default text, warning checks etc 
    static struct menu_display_info info;
    entry_default_display_info(entry, &info);
    info.x = x;
    info.y = y;
    info.x_val = x + 20 * ABS(menu->split_pos);
    info.can_custom_draw = menu != my_menu && menu != mod_menu && !menu_lv_transparent_mode;
    
    // display icon (only the first icon is drawn)
    icon_drawn = 0;

    if ((!menu_lv_transparent_mode && !only_selected) || entry->selected)
    {
        // should we override some things?
        if (entry->update)
        {
            /* in edit mode with caret, we will not allow the update function to override the entry value */
            char default_value[MENU_MAX_VALUE_LEN];
            if (editing_with_caret(entry))
                snprintf(default_value, MENU_MAX_VALUE_LEN, "%s", info.value);
            
            entry->update(entry, &info);
            
            if (editing_with_caret(entry))
                snprintf(info.value, MENU_MAX_VALUE_LEN, "%s", default_value);
        }

        // menu->update asked to draw the entire screen by itself? stop drawing right now
        if (info.custom_drawing == CUSTOM_DRAW_THIS_MENU)
            return 0;
        
        if (info.custom_drawing == CUSTOM_DRAW_DO_NOT_DRAW)
            menu_redraw_cancel = 1;
        
        // print the menu on the screen
        if (info.custom_drawing == CUSTOM_DRAW_DISABLE)
            entry_print(info.x, info.y, info.x_val - x, h, entry, &info, IS_SUBMENU(menu));
    }
    return 1;
}


/*
 * Dynamic menus
 * (My Menu / Recent menu, Modified menu)
 * 
 */

static void
dyn_menu_add_entry(struct menu *dyn_menu, struct menu_entry *entry, struct menu_entry *dyn_entry)
{
    // copy most things from old menu structure to this one
    // except for some essential things :P
    void *next;
    void *prev;
    int selected;

    ASSERT(dyn_entry);

    next = dyn_entry->next;
    prev = dyn_entry->prev;
    selected = dyn_entry->selected;

    memcpy(dyn_entry, entry, sizeof(struct menu_entry));
    dyn_entry->next = next;
    dyn_entry->prev = prev;
    dyn_entry->selected = selected;
    dyn_entry->shidden = entry->shidden;
    dyn_entry->hidden = 0;
    dyn_entry->jhidden = 0;
    dyn_entry->starred = 0;
    
    // update split position
    menu_update_split_pos(dyn_menu, dyn_entry);
}

static int my_menu_select_func(struct menu_entry * entry)
{
    return entry->starred ? 1 : 0;
}

static struct menu_entry * mod_menu_selected_entry = 0;

static int mod_menu_select_func(struct menu_entry * entry)
{
    if (config_var_was_changed(entry->priv))
        return 1;
    
    /* don't delete currently selected entry */
    if (mod_menu->selected && entry == mod_menu_selected_entry)
        return 1;
    
    /* anything from submenu was changed? */
    if (entry->children)
    {
        struct menu * submenu = menu_find_by_name(entry->name, ICON_ML_SUBMENU);
        if (submenu)
        {
            for (struct menu_entry * e = submenu->children; e; e = e->next)
            {
                if (mod_menu_select_func(e))
                    return 1;
            }
        }
    }
    
    return 0;
}

static int mru_menu_select_func(struct menu_entry * entry)
{
    float thr = IS_SUBMENU(entry->parent_menu)
        ? usage_counter_thr_sub
        : usage_counter_thr;

    return
        entry->usage_counter_long_term  >= thr ||
        entry->usage_counter_short_term >= thr ;
}

static int mru_junkie_my_menu_select_func(struct menu_entry * entry)
{
    return entry->jstarred;
}

#define DYN_MENU_DO_NOT_EXPAND_SUBMENUS 0
#define DYN_MENU_EXPAND_ALL_SUBMENUS 1
#define DYN_MENU_EXPAND_ONLY_ACTIVE_SUBMENUS 2

static int
dyn_menu_rebuild(struct menu * dyn_menu, int (*select_func)(struct menu_entry * entry), struct menu_entry * placeholders, int max_placeholders, int expand_submenus)
{
    dyn_menu->split_pos = -20;

    int i = 0;
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->no_name_lookup)
            continue;

        if (IS_SUBMENU(menu))
            continue;

        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            if (entry->shidden)
                continue;
            
            if (select_func(entry))
            {
                if (i >= max_placeholders) // too many items in our dynamic menu
                    return 0; // whoops
                
                dyn_menu_add_entry(dyn_menu, entry, &placeholders[i]);
                i++;
            }
            
            // any submenu?
            if (entry->children)
            {
                int should_expand = 
                    expand_submenus == DYN_MENU_EXPAND_ALL_SUBMENUS ||
                    (expand_submenus == DYN_MENU_EXPAND_ONLY_ACTIVE_SUBMENUS && !(IS_ML_PTR(entry->priv) && !MENU_INT(entry)));
                
                if (!should_expand)
                    continue;
                
                struct menu * submenu = menu_find_by_name(entry->name, ICON_ML_SUBMENU);
                if (submenu)
                {
                    for(struct menu_entry * e = submenu->children; e; e = e->next)
                    {
                        if (select_func(e))
                        {
                            if (i >= max_placeholders) // too many items in our dynamic menu
                                return 0; // whoops
                            
                            dyn_menu_add_entry(dyn_menu, e, &placeholders[i]);
                            i++;
                        }
                    }
                }
            }
        }
    }
    
    for ( ; i < max_placeholders; i++)
    {
        struct menu_entry * dyn_entry = &(placeholders[i]);
        dyn_entry->shidden = 1;
        dyn_entry->hidden = 1;
        dyn_entry->jhidden = 1;
        dyn_entry->name = "(empty)";
        dyn_entry->priv = 0;
        dyn_entry->select = 0;
        dyn_entry->select_Q = 0;
        dyn_entry->update = 0;
    }
    
    return 1; // success
}

/* hide menu items infrequently used (based on usage counters)
 * min_items:     if some menus end with too few items, move them to My Menu
 * count_max:     length of the longest menu (output)
 * count_my:      length of My Menu (result with current min_items)
 * count_my_next: length of My Menu (what would happen with min_items + 1)
 */
static void junkie_menu_rebuild(int min_items, int * count_max, int * count_my, int * count_my_next)
{
    *count_max = 0;
    *count_my = 0;
    *count_my_next = 0;

    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->no_name_lookup)
            continue;

        int count = 0;
        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            if (!IS_SUBMENU(menu))
            {
                /* items from regular menus; select based on usage counters */
                entry->jhidden = !mru_menu_select_func(entry);
                entry->jstarred = 0;
                count += (entry->jhidden) ? 0 : 1;
            }
            else
            {
                /* items from submenus will be selected for My Menu in the next block */
                entry->jhidden = 0;
                entry->jstarred = 0;
            }
        }
        *count_max = MAX(*count_max, count);

        /* too few items in this menu? move them to My Menu */
        /* note: submenus will always have count=0 here */
        if (count < min_items)
        {
            for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
            {
                if (mru_menu_select_func(entry))
                {
                    /* move items from main menu, but not from submenus */
                    entry->jhidden = !IS_SUBMENU(menu);
                    entry->jstarred = 1;
                    (*count_my)++;
                }
            }
        }

        if (count == min_items)
        {
            (*count_my_next) += count;
        }
    }
    
    (*count_my_next) += (*count_my);
}

static int
my_menu_rebuild()
{
    my_menu_dirty = 0;
    int ok = dyn_menu_rebuild(my_menu, my_menu_select_func, my_menu_placeholders, COUNT(my_menu_placeholders), DYN_MENU_EXPAND_ALL_SUBMENUS);

    if (!menu_has_visible_items(my_menu) && !customize_mode)
    {
        /* no user preferences? build it dynamically from usage counters */
        my_menu->name = "Recent";
        menu_normalize_usage_counters();

        if (junkie_mode)
        {
            /* Build junkie menu.
             * Items from "small" menus are moved into My Menu.
             * Threshold is increased until My Menu becomes
             * not much bigger than the longest menu.
             */
            menu_usage_counters_update_threshold(junkie_mode * 10, 0, 0);

            int count_max, count_my, count_my_next, count_my_0;
            junkie_menu_rebuild(1, &count_max, &count_my_0, &count_my_next);
            int min = 2;
            do
            {
                junkie_menu_rebuild(min, &count_max, &count_my, &count_my_next);
                min++;
            }
            while (min < 5 && count_my_next <= count_max + 2);

            if (count_my > 0 && ABS(count_my - count_max) > 2)
            {
                /* My Menu ended up with too few or too many items
                 * it may look a bit unbalanced - let's "equalize" it */
                int moved = count_my - count_my_0;
                int sub_target = MAX(MAX(count_max, junkie_mode*4) - moved, 2);
                menu_usage_counters_update_threshold(junkie_mode * 10 - sub_target, 0, 1);
                menu_usage_counters_update_threshold(sub_target, 1, 0);
                junkie_menu_rebuild(min-1, &count_max, &count_my, &count_my_next);
            }

            return dyn_menu_rebuild(my_menu, mru_junkie_my_menu_select_func, my_menu_placeholders, COUNT(my_menu_placeholders), DYN_MENU_EXPAND_ALL_SUBMENUS);
        }
        else
        {
            menu_usage_counters_update_threshold(10, 0, 0);
            return dyn_menu_rebuild(my_menu, mru_menu_select_func, my_menu_placeholders, COUNT(my_menu_placeholders), DYN_MENU_EXPAND_ALL_SUBMENUS);
        }
    }

    my_menu->name = MY_MENU_NAME;
    return ok;
}

static int mod_menu_rebuild()
{
    if (customize_mode)
    {
        /* clear this menu during customizations */
        return dyn_menu_rebuild(mod_menu, (void*) ret_0, mod_menu_placeholders, COUNT(mod_menu_placeholders), DYN_MENU_EXPAND_ONLY_ACTIVE_SUBMENUS);
    }

    mod_menu_dirty = 0;
    
    mod_menu_selected_entry = get_selected_menu_entry(mod_menu);

    /* mod_menu_selected_entry must be from the regular menu (not the dynamic one) */
    if (mod_menu_selected_entry && mod_menu_selected_entry->name)
    {
        mod_menu_selected_entry = entry_find_by_name(mod_menu_selected_entry->parent_menu->name, mod_menu_selected_entry->name);
    }
    
    int ok = dyn_menu_rebuild(mod_menu, mod_menu_select_func, mod_menu_placeholders, COUNT(mod_menu_placeholders), DYN_MENU_EXPAND_ONLY_ACTIVE_SUBMENUS);
    
    /* make sure the selection doesn't move because of updating */
    if (mod_menu->selected && mod_menu_selected_entry)
    {
        select_menu_by_name(MOD_MENU_NAME, mod_menu_selected_entry->name);
    }
    return ok;
}

static void
menu_display(
    struct menu * menu,
    int         x,
    int         y, 
    int only_selected
)
{
    struct menu_entry * entry = menu->children;
    
    //hide upper menu for vscroll
    int pos = get_menu_selected_pos(menu);
    int num_visible = get_menu_visible_count(menu);
    int target_height = menu->submenu_height ? menu->submenu_height - 54 : 370;
    if (is_menu_active("Help")) target_height -= 20;
    if (is_menu_active("Focus")) target_height -= 70;
    int natural_height = num_visible * font_large.height;
    int ideal_num_items = target_height / font_large.height;

    /* if the menu items does not exceed max count by too much (e.g. 12 instead of 11),
     * prefer to squeeze them vertically in order to avoid scrolling. */
    
    /* but if we can't avoid scrolling, don't squeeze */
    if (num_visible > ideal_num_items + 1)
    {
        num_visible = ideal_num_items;
        natural_height = num_visible * font_large.height;
        /* leave some space for the scroll indicators */
        target_height -= submenu_level ? 16 : 12;
        y += submenu_level ? 4 : 2;
    }
    else /* we can fit everything */
    {
        menu->scroll_pos = 0;
    }
    
    int extra_spacing = (target_height - natural_height);
    
    /* don't stretch too much */
    extra_spacing = MIN(extra_spacing, 2 * num_visible);

    /* use Bresenham line-drawing algorithm to divide space evenly with integer-only math */
    /* http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm#Algorithm_with_Integer_Arithmetic */
    /* up to 3 pixels of spacing per row */
    /* x: from 0 to (num_visible-1)*3 */
    /* y: space accumulated (from 0 to extra_spacing) */
    int dx = (num_visible-1)*3;
    int dy = ABS(extra_spacing);
    dy = MIN(dy, dx);
    int D = 2*dy - dx;

    int scroll_pos = menu->scroll_pos; // how many menu entries to skip
    scroll_pos = MAX(scroll_pos, pos - num_visible);
    scroll_pos = MIN(scroll_pos, pos - 1);
    menu->scroll_pos = scroll_pos;
    
    for(int i=0;i<scroll_pos;i++){
        while(!is_visible(entry)) entry = entry->next;
        entry = entry->next;
    }

    if (scroll_pos > 0)
    {
        for (int i = -13; i <= 13; i++)
            draw_line(360 - i, y + 8 - 12, 360, y - 12, MENU_BAR_COLOR);
    }

    //<== vscroll

    if (!menu_lv_transparent_mode)
        menu_clean_footer();

    for (int i = 0; i < num_visible && entry; )
    {
        if (is_visible(entry))
        {
            /* how much extra spacing for this menu entry? */
            /* (Bresenham step) */
            int local_spacing = 0;
            for (int i = 0; i < 3; i++)
            {
                if (D > 0)
                {
                    local_spacing += SGN(extra_spacing);
                    D = D + (2*dy - 2*dx);
                }
                else
                {
                    D = D + 2*dy;
                }
            }
            
            // display current entry
            int ok = menu_entry_process(menu, entry, x, y, font_large.height + local_spacing, only_selected);
            
            // entry asked for custom draw? stop here
            if (!ok)
                goto end;
            
            // move down for next item
            y += font_large.height + local_spacing;
            
            i++;
        }

        entry = entry->next;
    }

    int more_entries = 0;
    while (entry) {
        if (is_visible(entry)) more_entries++;
        entry = entry->next;
    }

    if (more_entries)
    {
        y += 10;
        for (int i = -13; i <= 13; i++)
            draw_line(360 - i, y - 8, 360, y, MENU_BAR_COLOR);
    }

end:
    // all menus displayed, now some extra stuff
    menu_post_display();
}

static int startswith(char* str, char* prefix)
{
    char* s = str;
    char* p = prefix;
    for (; *p; s++,p++)
        if (*s != *p) return 0;
    return 1;
}

static inline int islovowel(char c)
{
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
        return 1;
    return 0;
}
static char* junkie_get_shortname(struct menu_display_info * info, int fnt, int maxlen)
{
    static char tmp[30];
    static char sname[20];
    memset(sname, 0, sizeof(sname));

    if (info->short_name[0])
    {
        snprintf(tmp, sizeof(tmp), "%s", info->short_name);
    }
    else
    {
        // skip some common words
        int skip = 0;
        if (startswith(info->name, "Movie")) skip = 5;
        else if (startswith(info->name, "Magic")) skip = 5;
        else if (startswith(info->name, "Focus")) skip = 5;
        else if (startswith(info->name, "Expo")) skip = 4;
        else if (startswith(info->name, "Advanced")) skip = 8;
        else if (startswith(info->name, "LV Display")) skip = 10;
        else if (startswith(info->name, "LV")) skip = 2;
        else if (startswith(info->name, "ML")) skip = 2;
        
        // keep the first letter from the skipped word
        char abbr[4] = "";
        if (skip > 2)
        {
            abbr[0] = info->name[0];
            abbr[1] = 0;
        }
        snprintf(tmp, sizeof(tmp), "%s%s", abbr, info->name + skip);
    }

    int N = strlen(tmp);

    int char_width = fontspec_font(fnt)->width;

    int i,j;
    for (i = 0, j = 0; i < COUNT(sname)-1 && j < N && bmp_string_width(fnt, sname) < maxlen - char_width; j++)
    {
        char c = tmp[j];
        if (c == ' ') { tmp[j+1] = toupper(tmp[j+1]); continue; }
        if (c == '.') continue;
        if (c == '(') break;
        if (maxlen < 3*char_width && islower(c)) continue;
        sname[i] = c;
        i++;
    }
    
    return sname;
}

static char* junkie_get_shortvalue(struct menu_display_info * info, int fnt, int maxlen)
{
    static char tmp[30];
    static char svalue[20];
    memset(svalue, 0, sizeof(svalue));

    if (info->short_value[0])
    {
        snprintf(tmp, sizeof(tmp), "%s", info->short_value);
    }
    else
    {
        // skip some common words
        int skip = 0;
        if (startswith(info->value, "ON,")) skip = 3;
        if (startswith(info->value, "Press")) skip = 5;
        if (startswith(info->value, "up to ")) skip = 6;
        if (startswith(info->value, "Photo,")) skip = 6;
        snprintf(tmp, sizeof(tmp), "%s", info->value + skip);
    }

    int N = strlen(tmp);

    int char_width = fontspec_font(fnt)->width;

    int i,j;
    for (i = 0, j = 0; i < COUNT(svalue)-1 && j < N && bmp_string_width(fnt, svalue) < maxlen - char_width; j++)
    {
        char c = tmp[j];
        if (c == ' ') continue;
        if (c == '(') break;
        svalue[i] = c;
        i++;
    }
    
    return svalue;
}

static char* junkie_get_shorttext(struct menu_display_info * info, int fnt, int maxlen)
{
    // print name or value?
    if (streq(info->value, "ON") || streq(info->value, "Default") || startswith(info->value, "OFF") || streq(info->value, "Normal") || (!info->value[0] && !info->short_value[0]))
    {
        // ON/OFF is obvious by color; print just the name
        return junkie_get_shortname(info, fnt, maxlen);
    }
    else // print value only
    {
        char* svalue = junkie_get_shortvalue(info, fnt, maxlen);
        int len = bmp_string_width(fnt, svalue);
        int char_width = fontspec_font(fnt)->width;
        if (maxlen - len >= char_width * 4) // still plenty of space? try to print part of name too
        {
            static char nv[30];
            char* sname = junkie_get_shortname(info, fnt, maxlen - len - bmp_string_width(fnt, " "));
            if (bmp_string_width(fnt, sname) >= char_width * 2)
            {
                snprintf(nv, sizeof(nv), "%s %s", sname, svalue);
                return nv;
            }
        }
        return svalue;
    }
}

static void
entry_print_junkie(
    int x,
    int y,
    int w,
    int h,
    struct menu_entry * entry,
    struct menu_display_info * info,
    int menu_selected
)
{
    int sel = menu_selected && entry->selected;

    int fg = 65;
    int bg = 45;

    if (info->warning_level == MENU_WARN_NOT_WORKING) 
    {
        if (info->enabled)
        {
            bg = can_be_turned_off(entry) ? COLOR_DARK_GREEN1_MOD : COLOR_DARK_CYAN1_MOD;
            fg = COLOR_BLACK;
        }
        else
        {
            bg = 45;
            fg = COLOR_BLACK;
        }
    }
    else if (info->enabled)
    {
        bg = can_be_turned_off(entry) ? COLOR_GREEN1 : COLOR_DARK_CYAN2_MOD;
        fg = COLOR_BLACK;
        if (customize_mode) bg = 60;
    }
    
    w -= 2;
    x += 1;
    h -= 1;

    if (sel) // display the full selected entry normally
    {
        entry_print(MENU_OFFSET, 390, 330, font_large.height, entry, info, 0);
        
        // brighten the selection
        if (bg == COLOR_GREEN1) bg = COLOR_GREEN2;
        else if (bg == COLOR_DARK_GREEN1_MOD) bg = COLOR_DARK_GREEN2_MOD;
        else if (bg == COLOR_DARK_CYAN1_MOD) bg = COLOR_DARK_CYAN2_MOD;
        else if (bg == COLOR_DARK_CYAN2_MOD) bg = COLOR_CYAN;
        else if (bg == 45) bg = 50;

        if (fg == 65) fg = COLOR_WHITE;
    }

    int fnt = FONT(FONT_MED, fg, bg);

    if (h > 30 && w > 130) // we can use large font when we have 5 or fewer tabs
        fnt = FONT(FONT_LARGE, fg, bg);

    int maxlen = (w - 8);

    bmp_fill(bg, x+2, y+2, w-4, h-4);
    //~ bmp_draw_rect(bg, x+2, y+2, w-4, h-4);

    char* shorttext = junkie_get_shorttext(info, fnt, maxlen);
    
    bmp_printf(
        fnt,
        x + (w - bmp_string_width(fnt, shorttext)) / 2 + 2, 
        y + (h - fontspec_height(fnt)) / 2,
        "%s", shorttext
    );

    // selection bar params
    
    // selection bar
    int selc = sel ? COLOR_WHITE : COLOR_BLACK; //menu_selected ? COLOR_BLUE : entry->selected ? COLOR_BLACK : COLOR_BLACK;
    bmp_draw_rect_chamfer(selc, x, y, w, h, 3, 0);
    bmp_draw_rect_chamfer(selc, x+1, y+1, w-2, h-2, 2, 1);
    bmp_draw_rect_chamfer(selc, x+2, y+2, w-4, h-4, 2, 1);
    bmp_draw_rect_chamfer(COLOR_BLACK, x+3, y+3, w-6, h-6, 2, 1);
    //~ draw_line(x, y+h+1, x+w, y+h+1, selc);
    
    // round corners
    /*
    bmp_putpixel(x+3, y+3, selc);
    bmp_putpixel(x+3, y+4, selc);
    bmp_putpixel(x+4, y+3, selc);
    bmp_putpixel(x+w-3, y+3, selc);
    bmp_putpixel(x+w-3, y+4, selc);
    bmp_putpixel(x+w-4, y+3, selc);
    bmp_putpixel(x+3, y+h-3, selc);
    bmp_putpixel(x+3, y+h-4, selc);
    bmp_putpixel(x+4, y+h-3, selc);
    bmp_putpixel(x+w-3, y+h-3, selc);
    bmp_putpixel(x+w-3, y+h-4, selc);
    bmp_putpixel(x+w-4, y+h-3, selc);
    */

    // customization markers
    if (customize_mode)
    {
        display_customize_marker(entry, x + w - 35, y);
    }
}

static int
menu_entry_process_junkie(
    struct menu * menu,
    struct menu_entry * entry,
    int         x,
    int         y, 
    int         w,
    int         h
)
{
    // fill in default text, warning checks etc 
    static struct menu_display_info info;
    entry_default_display_info(entry, &info);
    info.x = 0;
    info.y = 0;
    info.x_val = 0;
    info.can_custom_draw = 0;

    // display icon (only the first icon is drawn)
    icon_drawn = 0;

    //~ if ((!menu_lv_transparent_mode && !only_selected) || entry->selected)
    {
        // should we override some things?
        if (entry->update)
            entry->update(entry, &info);

        // menu->update asked to draw the entire screen by itself? stop drawing right now
        if (info.custom_drawing == CUSTOM_DRAW_THIS_MENU)
            return 0;

        if (info.custom_drawing == CUSTOM_DRAW_DO_NOT_DRAW)
            menu_redraw_cancel = 1;

        // print the menu on the screen
        if (info.custom_drawing == CUSTOM_DRAW_DISABLE)
            entry_print_junkie(x, y, w, h, entry, &info, menu->selected);
    }
    return 1;
}

static void
menu_entry_move(
    struct menu *       menu,
    int         direction
);

static int junkie_get_selection_y(struct menu * menu, int* h)
{
    int num = get_menu_visible_count(menu);
    
    int space_left = 330;
    *h = space_left / num;
    
    int y = 0;

    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        if (is_visible(entry))
        {
            // move down for next item
            int dh = space_left / num;
            if (entry->selected) // found!
                return y + dh/2;
            y += dh;
            space_left -= dh;
            num--;
        }
    }
    return 0;
}

static int junkie_selection_pos_y = 10;

static void junkie_update_selection_pos(struct menu * menu)
{
    int h;
    int y = junkie_get_selection_y(menu, &h);
    int steps = (ABS(junkie_selection_pos_y - y) + h/2) / h;
    int dir = SGN(junkie_selection_pos_y - y);
    for (int i = 0; i < steps; i++)
        menu_entry_move(menu, dir);
}

static void junkie_sync_selection()
{
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (!menu->selected && !IS_SUBMENU(menu))
        {
            junkie_update_selection_pos(menu);
        }
    }
}

static void
menu_display_junkie(
    struct menu * menu,
    int         x,
    int         y,
    int         w
)
{
    int num = get_menu_visible_count(menu);
    
    int h = 330 / num;
    int space_left = 330;

    if (!menu_lv_transparent_mode && menu->selected)
        menu_clean_footer();

    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        if (is_visible(entry))
        {
            //~ int h = font_large.height - 3;
            
            // display current entry
            int ok = menu_entry_process_junkie(menu, entry, x, y, w, h);
            
            // entry asked for custom draw? stop here
            if (!ok) break;

            //~ if (entry->warning_level == MENU_WARN_NOT_WORKING)
                //~ continue;

            // move down for next item
            int dh = space_left / num;
            y += dh;
            space_left -= dh;
            num--;
        }
    }

    // all menus displayed, now some extra stuff
    menu_post_display();
}

static void
show_hidden_items(struct menu * menu, int force_clear)
{
    // show any items that may be hidden
    if (!menu_lv_transparent_mode)
    {
        char hidden_msg[100];
        snprintf(hidden_msg, sizeof(hidden_msg), "Hidden: ");
        int hidden_count = 0;

        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            /* fixme: check without streq */
            if (HAS_HIDDEN_FLAG(entry) && !streq(entry->name, "(empty)"))
            {
                if (hidden_count) { STR_APPEND(hidden_msg, ", "); }
                int len = strlen(hidden_msg);
                STR_APPEND(hidden_msg, "%s", entry->name);
                while (isspace(hidden_msg[strlen(hidden_msg)-1])) hidden_msg[strlen(hidden_msg)-1] = '\0';
                while (ispunct(hidden_msg[strlen(hidden_msg)-1])) hidden_msg[strlen(hidden_msg)-1] = '\0';
                hidden_msg[MIN(len+15, (int)sizeof(hidden_msg)-1)] = '\0';
                hidden_count++;
            }
        }
        STR_APPEND(hidden_msg, customize_mode ? "." : " (Prefs->Customize).");
        
        unsigned maxlen = bmp_strlen_clipped(FONT_MED, hidden_msg, 700);
        if (strlen(hidden_msg) > maxlen)
        {
            hidden_msg[maxlen-1] = hidden_msg[maxlen-2] = hidden_msg[maxlen-3] = '.';
            hidden_msg[maxlen] = '\0';
        }

        int hidden_pos_y = 410;
        if (is_menu_active("Help")) hidden_pos_y -= font_med.height;
        if (hidden_count)
        {
            bmp_fill(COLOR_BLACK, 0, hidden_pos_y, 720, 19);
            bmp_printf(
                SHADOW_FONT(FONT(FONT_MED, customize_mode ? MENU_WARNING_COLOR : COLOR_ORANGE , MENU_BG_COLOR_HEADER_FOOTER)), 
                 10, hidden_pos_y, 
                 hidden_msg
            );
        }
    }
}

static void
show_vscroll(struct menu * parent){
    
    if (edit_mode)
        return;
    
    int pos = get_menu_selected_pos(parent);
    int max = get_menu_visible_count(parent);

    int menu_len = MENU_LEN;
    
    if(max > menu_len + 1){
        int y_lo = 44;
        int h = submenu_level ? 378 : 385;
        int size = (h - y_lo) * menu_len / max;
        int y = y_lo + ((h - size) * (pos-1) / (max-1));
        int x = MIN(360 + g_submenu_width/2, 720-3);
        if (submenu_level) x -= 6;
        
        bmp_fill(COLOR_BLACK, x-2, y_lo, 6, h);
        bmp_fill(MENU_BAR_COLOR, x, y, 3, size);
    }
}

static void
menus_display(
    struct menu *       menu,
    int         orig_x,
    int         y
)
{
    g_submenu_width = 720;

    if (duplicate_check_dirty)
        check_duplicate_entries();

    if (my_menu_dirty)
        my_menu_rebuild();
    
    if (mod_menu_dirty)
        mod_menu_rebuild();

    if (get_selected_toplevel_menu()->icon != menu_first_by_icon)
    {
        select_menu_by_icon(menu_first_by_icon);
    }

    menu_make_sure_selection_is_valid();

    struct menu * submenu = 0;
    if (submenu_level)
    {
        submenu = get_current_submenu();

        if (!submenu)
        {
            printf("no submenu, fall back to edit mode\n");
            submenu_level--;
            edit_mode = 1;
        }
    }
    
    advanced_mode = submenu ? submenu->advanced : 1;

    if (junkie_mode) junkie_sync_selection();
    
    #ifdef SUBMENU_DEBUG_JUNKIE
    struct menu * junkie_sub = 0;
    if (junkie_mode == 2)
    {
        struct menu_entry * entry = get_selected_menu_entry(0);
        if (entry && entry->children)
            junkie_sub = menu_find_by_name(entry->name, 0);
    }
    #endif

    take_semaphore( menu_sem, 0 );

    // will override them only if rack focus items are selected
    reset_override_zoom_buttons();

    // how many tabs should we display? we should know in order to adjust the spacing between them
    // keep the conditions in sync with the next loop
    int num_tabs = 0;
    for(struct menu * tmp_menu = menu ; tmp_menu ; tmp_menu = tmp_menu->next )
    {
        if (!menu_has_visible_items(tmp_menu) && !tmp_menu->selected)
            continue; // empty menu
        if (IS_SUBMENU(tmp_menu))
            continue;
        num_tabs++;
    }
    
    int x = orig_x + junkie_mode ? 2 : 150;
    int icon_spacing = junkie_mode ? 716 / num_tabs : (720 - 150) / num_tabs;
    
    int bgs = COLOR_BLACK;
    int bgu = MENU_BG_COLOR_HEADER_FOOTER;
    int fgu = COLOR_GRAY(35);
    int fgs = COLOR_WHITE;

    if (customize_mode) fgs = get_customize_color();

    bmp_fill(bgu, orig_x, y, 720, 42);
    //~ bmp_fill(fgu, orig_x, y+42, 720, 2);
    
    for( ; menu ; menu = menu->next )
    {
        if (!menu_has_visible_items(menu) && !menu->selected)
            continue; // empty menu
        if (IS_SUBMENU(menu))
            continue;
        int fg = menu->selected ? fgs : fgu;
        int bg = menu->selected ? bgs : bgu;
        
        if (!menu_lv_transparent_mode)
        {
            if (menu->selected)
                bmp_fill(bg, x-1, y+2, icon_spacing+3, 38);

            int icon_char = menu->icon ? menu->icon : menu->name[0];
            int icon_width = bfnt_char_get_width(icon_char);
            int x_ico = x + (icon_spacing - icon_width) / 2 + 1;
            bfnt_draw_char(icon_char, x_ico, y + 2, fg, NO_BG_ERASE);

            if (menu->selected)
            {
                    //~ bmp_printf(FONT_MED, 720 - strlen(menu->name)*font_med.width, 50, menu->name);
                //~ else
                if (!junkie_mode)
                    bmp_printf(FONT(FONT_CANON, fg, NO_BG_ERASE), 5, y, "%s", menu->name);
                
                int x1 = x - 1;
                int x2 = x1 + icon_spacing + 2;

                //~ draw_line(x1, y+42-4, x1, y+5, fgu);
                //~ draw_line(x2, y+42-4, x2, y+5, fgu);
                //~ draw_line(x1-1, y+42-4, x1-1, y+5, fgu);
                //~ draw_line(x2+1, y+42-4, x2+1, y+5, fgu);

                //~ draw_line(x1+4, y+1, x2-4, y+1, fgu);
                //~ draw_line(x1+4, y, x2-4, y, fgu);

                draw_line(x1-1, y+40, x2+1, y+40, bgs);
                draw_line(x1-2, y+41, x2+2, y+41, bgs);
                draw_line(x1-3, y+42, x2+3, y+42, bgs);
                draw_line(x1-4, y+43, x2+4, y+43, bgs);

                //~ draw_line(x1-4, y+42, x1, y+42-4, fgu);
                //~ draw_line(x2+4, y+42, x2, y+42-4, fgu);
                //~ draw_line(x1-4, y+41, x1, y+41-4, fgu);
                //~ draw_line(x2+4, y+41, x2, y+41-4, fgu);

                //~ draw_line(x1, y+5, x1+4, y+1, fgu);
                //~ draw_line(x2, y+5, x2-4, y+1, fgu);
                //~ draw_line(x1, y+4, x1+4, y, fgu);
                //~ draw_line(x2, y+4, x2-4, y, fgu);
                
                draw_line(x1, y+2, x1, y+3, bgu);
                draw_line(x1+1, y+2, x1+1, y+2, bgu);

                draw_line(x2, y+2, x2, y+3, bgu);
                draw_line(x2-1, y+2, x2-1, y+2, bgu);
            }
            x += icon_spacing;
        }
        
        if (submenu) continue;
        
        if (junkie_mode && !edit_mode && !menu_lv_transparent_mode)
        {
            struct menu * mn = menu;
            
            #ifdef SUBMENU_DEBUG_JUNKIE
            if (junkie_sub && mn == my_menu) mn = junkie_sub;
            #endif
            
            menu_display_junkie(
                mn,
                x - icon_spacing,
                y + 55,
                icon_spacing
            );
        }
        else if( menu->selected)
        {
            menu_display(
                menu,
                orig_x + MENU_OFFSET,
                y + 55, 
                edit_mode ? 1 : 0
            );
            
            show_vscroll(menu);
            show_hidden_items(menu, 0);
        }
    }
    
    // debug
    //~ if (junkie_mode)
        //~ draw_line(0, 55 + junkie_selection_pos_y, 720, 55 + junkie_selection_pos_y, COLOR_BLUE);
    
    if (submenu)
    {
        //~ dim_screen(43, COLOR_BLACK, 0, 45, 720, 480-45-50);
        
        submenu_display(submenu);
        show_vscroll(submenu);
    }
    
    give_semaphore( menu_sem );
}

/*
static void
implicit_submenu_display()
{
    struct menu * menu = get_selected_toplevel_menu();
    menu_display(
        menu,
        MENU_OFFSET,
         55,
         1
    );
}
*/

static int submenu_default_height(int count)
{
    return MIN(422, count * (font_large.height + 2) + 40 + 50 - (count > 7 ? 30 : 0));
    /* body + titlebar + padding - smaller padding for large submenus */
}
static void
submenu_display(struct menu *submenu)
{
    if (!submenu)
        return;

    int count = get_menu_visible_count(submenu);
    int h = submenu->submenu_height ? submenu->submenu_height : submenu_default_height(count);
        
    int w = submenu->submenu_width  ? submenu->submenu_width : 600;

    // submenu promoted to pickbox? expand the pickbox by default
    if (submenu->children && IS_SINGLE_ITEM_SUBMENU_ENTRY(submenu->children))
    {
        w = 720;
        int num_choices = submenu->children[0].max - submenu->children[0].min;
        if (CAN_HAVE_PICKBOX(submenu->children))
        {
            h = MAX(h, submenu_default_height(num_choices) + 7);
        }
    }
    
    w = MIN(w, 720 - 10);
    
    g_submenu_width = w;
    int bx = (720 - w)/2;
    int by = (480 - h)/2 - 30;
    by = MAX(by, 3);
    
    // submenu header
    if (
            (submenu->children && IS_SINGLE_ITEM_SUBMENU_ENTRY(submenu->children) && edit_mode) // promoted submenu
                ||
            (!menu_lv_transparent_mode && !edit_mode)
        )
    {
        w = 720 - 2 * bx;
        bmp_fill(MENU_BG_COLOR_HEADER_FOOTER,  bx,  by, w, 40);
        bmp_fill(COLOR_BLACK,  bx,  by + 40, w, h-40);
        bmp_printf(FONT(FONT_CANON, COLOR_WHITE, NO_BG_ERASE),  bx + 15,  by + 2, "%s", submenu->name);

        for (int i = 0; i < 5; i++)
            bmp_draw_rect(45,  bx - i,  by - i, w + i * 2, h + i * 2);

/* gradient experiments
        for (int i = 0; i < 3; i++)
            bmp_draw_rect(38 + i,  bx-i,  by-i, w+i*2, h+i*2);
        
        for (int i = 3; i < 7; i++)
            bmp_draw_rect(42,  bx-i,  by-i, w+i*2, h+i*2);

        for (int i = 7; i < 10; i++)
            bmp_draw_rect(48-i,  bx-i,  by-i, w+i*2, h+i*2);

        for (int i = 10; i < 15; i++)
            bmp_draw_rect(COLOR_BLACK,  bx-i,  by-i, w+i*2, h+i*2);
*/            

        submenu_key_hint(720 - bx - 45, by + 5, COLOR_WHITE, MENU_BG_COLOR_HEADER_FOOTER, ICON_ML_Q_BACK);
    }
                                                   /* titlebar + padding difference for large submenus */
    menu_display(submenu,  bx + SUBMENU_OFFSET,  by + 40 + (count > 7 ? 10 : 25), edit_mode ? 1 : 0);
    show_hidden_items(submenu, 1);
}

static void
menu_entry_showhide_toggle(
    struct menu * menu,
    struct menu_entry * entry
)
{
    if( !entry )
        return;

    if (junkie_mode)
        entry->jhidden = !entry->jhidden;
    else
        entry->hidden = !entry->hidden;
}

// this can fail if there are too many starred items (1=success, 0=fail)
static int
menu_entry_star_toggle(
    struct menu_entry * entry
)
{
    if( !entry )
        return 0;

    entry->starred = !entry->starred;
    menu_flags_save_dirty = 1;
    int ok = my_menu_rebuild();
    if (!ok)
    {
        entry->starred = 0;
        my_menu_rebuild();
        return 0;
    }
    return 1;
}

// normal -> starred -> hidden
static void
menu_entry_customize_toggle(
    struct menu *   menu
)
{
    if (menu->no_name_lookup && menu != my_menu)
    {
        /* we depend on name look-ups */
        /* my_menu is a special case, see below */
        beep();
        return;
    }

    struct menu_entry * entry = get_selected_menu_entry(menu);
    if (!entry) return;

    /* make sure the customized menu entry can be looked up by name */
    struct menu_entry * entry_by_name = entry_find_by_name(entry->parent_menu->name, entry->name);
    if (!entry_by_name)
    {
        beep();
        printf("Not found: %s\n", entry->name);
        return;
    }

    if (menu == my_menu) // special case
    {
        // lookup the corresponding entry in normal menus, and toggle that one instead
        struct menu_entry * orig_entry = entry_by_name;
        ASSERT(orig_entry->starred);
        ASSERT(orig_entry->parent_menu);
        ASSERT(orig_entry->parent_menu->no_name_lookup == 0);
        menu_entry_star_toggle(orig_entry); // should not fail
        return;
    }

    ASSERT(entry_by_name == entry);

    if (entry->starred && HAS_CURRENT_HIDDEN_FLAG(entry)) // both flags active, abnormal
    {
        menu_entry_showhide_toggle(menu, entry); // keep the star flag
    }
    
    if (!entry->starred && !HAS_CURRENT_HIDDEN_FLAG(entry)) // normal -> starred
    {
        int ok = menu_entry_star_toggle(entry);
        if (!ok) menu_entry_showhide_toggle(menu, entry); // too many starred items? just hide
    }
    else if (entry->starred && !HAS_CURRENT_HIDDEN_FLAG(entry)) // starred -> hidden
    {
        menu_entry_star_toggle(entry); // should not fail
        menu_entry_showhide_toggle(menu, entry);
    }
    else if (!entry->starred && HAS_CURRENT_HIDDEN_FLAG(entry)) // hidden -> normal
    {
        menu_entry_showhide_toggle(menu, entry);
    }

    menu_flags_save_dirty = 1;
    my_menu_dirty = 1;
    menu_make_sure_selection_is_valid();
}

static void
menu_entry_select(
    struct menu *   menu,
    int mode // 0 = increment, 1 = decrement, 2 = Q, 3 = SET
)
{
    if( !menu )
        return;

    struct menu_entry * entry = get_selected_menu_entry(menu);
    if( !entry )
    {
        /* empty submenu? go back */
        menu_lv_transparent_mode = edit_mode = 0;
        submenu_level = MAX(submenu_level - 1, 0);
        return;
    }

    // don't perform actions on empty items (can happen on empty submenus)
    if (!is_visible(entry))
    {
        edit_mode = 0;
        submenu_level = MAX(submenu_level - 1, 0);
        menu_lv_transparent_mode = 0;
        return;
    }

    /* note: entry->select() can delete itself (see e.g. file_man) */
    /* we must be careful to prevent using entry if this happened */
    /* fixme: better solution? */
    ASSERT(entry_being_updated == 0);
    entry_being_updated = entry;
    entry_removed_itself = 0;

    /* usage counters are only updated for actual toggles */
    /* they are not updated during submenu navigation */
    int entry_used = 0;

    if(mode == 1) // decrement
    {
        if (entry->select)
        {
            /* custom select function? use it */
            entry->select( entry->priv, -1);
        }
        else if IS_ML_PTR(entry->priv)
        {
            /* .priv is a variable? in edit mode, increment according to caret_position, otherwise use exponential R20 toggle */
            /* exception: hex fields are never fast-toggled */
            if (editing_with_caret(entry) || (entry->unit == UNIT_HEX))
                menu_numeric_toggle(entry->priv, get_caret_delta(entry,-1), entry->min, entry->max);
            else
                menu_numeric_toggle_fast(entry->priv, -1, entry->min, entry->max, entry->unit, entry->edit_mode, 0);
        }
        entry_used = 1;
    }
    else if (mode == 2) // Q
    {
        bool promotable_to_pickbox = HAS_SINGLE_ITEM_SUBMENU(entry) && SHOULD_USE_EDIT_MODE(entry->children);

        if (menu_lv_transparent_mode) { menu_lv_transparent_mode = 0; }
        else if (edit_mode)
        {
            edit_mode = 0;
            submenu_level = MAX(submenu_level - 1, 0);
        }
        else if ( entry->select_Q )
        {
            // caution: entry may now be a dangling pointer
            // fixme: when?
            entry->select_Q( entry->priv, 1);
            entry_used = 1;
        }
        else
        {
            menu_toggle_submenu();
        }

         // submenu with a single entry? promote it as pickbox
        if (submenu_level && promotable_to_pickbox)
            edit_mode = 1;
    }
    else if (mode == 3) // SET
    {
        /*
        if (set_action == 0) // pickbox
        {
            if (entry->icon_type != IT_SUBMENU) edit_mode = !edit_mode;
            else if( entry->select ) entry->select( entry->priv, 1);
            else edit_mode = !edit_mode;
        }
        else if (set_action == 1) // toggle
        {
            if (edit_mode) edit_mode = 0;
            else if( entry->select ) entry->select( entry->priv, 1);
            else if IS_ML_PTR(entry->priv) menu_numeric_toggle_fast(entry->priv, 1, entry->min, entry->max);
        }
        else */
        {
            if (submenu_level && edit_mode && IS_SINGLE_ITEM_SUBMENU_ENTRY(entry))
            {
                edit_mode = 0;
                submenu_level = MAX(submenu_level - 1, 0);
            }
            else if (edit_mode)
            {
                edit_mode = 0;
            }
            else if (menu_lv_transparent_mode && entry->icon_type != IT_ACTION)
            {
                menu_lv_transparent_mode = 0;
            }
            else if (entry->edit_mode & EM_SHOW_LIVEVIEW)
            {
                if (lv) menu_lv_transparent_mode = !menu_lv_transparent_mode;
                else edit_mode = !edit_mode;
            }
            else if (SHOULD_USE_EDIT_MODE(entry))
            {
                edit_mode = !edit_mode;
            }
            else if (entry->select)
            {
                entry->select( entry->priv, 1);
                entry_used = 1;
            }
            else if IS_ML_PTR(entry->priv)
            {
                menu_numeric_toggle_fast(entry->priv, 1, entry->min, entry->max, entry->unit, entry->edit_mode, 0);
                entry_used = 1;
            }
        }
    }
    else // increment (same logic as decrement)
    {
        if( entry->select )
        {
            entry->select( entry->priv, 1);
        }
        else if (IS_ML_PTR(entry->priv))
        {
            if (editing_with_caret(entry) || (entry->unit == UNIT_HEX))
                menu_numeric_toggle(entry->priv, get_caret_delta(entry,1), entry->min, entry->max);
            else
                menu_numeric_toggle_fast(entry->priv, 1, entry->min, entry->max, entry->unit, entry->edit_mode, 0);
        }

        entry_used = 1;
    }

    entry_being_updated = 0;

    if (!entry_removed_itself)
    {
        if (entry->unit == UNIT_TIME && edit_mode && caret_position == 0)
        {
            caret_position = 1;
        }

        if (entry_used)
        {
            menu_update_usage_counters(entry);
        }
    }

    config_dirty = 1;
    mod_menu_dirty = 1;
}

/** Scroll side to side in the list of menus */
static void
menu_move(
    struct menu *       menu,
    int         direction
)
{
    //~ menu_damage = 1;

    if( !menu )
        return;
    
    take_semaphore( menu_sem, 0 );

    // Deselect the current one
    menu->selected      = 0;

    do
    {
        if( direction < 0 )
        {
            if( menu->prev )
                menu = menu->prev;
            else {
                // Go to the last one
                while( menu->next )
                    menu = menu->next;
            }
        } else {
            if( menu->next )
                menu = menu->next;
            else {
                // Go to the first one
                while( menu->prev )
                    menu = menu->prev;
            }
        }
    }
    while ((IS_SUBMENU(menu)) || /* always skip submenus */
          (!menu_has_visible_items(menu) && are_there_any_visible_menus())); /* skip empty menus */

    // Select the new one (which might be the same)
    menu->selected      = 1;
    menu_first_by_icon = menu->icon;
    
    /* rebuild the modified settings menu */
    mod_menu_dirty = 1;
    
    give_semaphore( menu_sem );
}


/** Scroll up or down in the currently displayed menu */
static void
menu_entry_move(
    struct menu *       menu,
    int         direction
)
{
    if( !menu )
        return;

    take_semaphore( menu_sem, 0 );
    
    if (!menu_has_visible_items(menu))
    {
        give_semaphore( menu_sem );
        return;
    }

    struct menu_entry * entry = menu->children;

    for( ; entry ; entry = entry->next )
    {
        if( entry->selected ) break;
    }


    // Nothing selected?
    if( !entry )
    {
        give_semaphore( menu_sem );
        return;
    }
    
    // Deslect the current one
    entry->selected = 0;

    do
    {
        if( direction < 0 )
        {
            // First and moving up?
            if( entry->prev ){
                entry = entry->prev;
            }else {
                // Go to the last one
                while( entry->next ) entry = entry->next;
            }
        } else {
            // Last and moving down?
            if( entry->next ){
                entry = entry->next;
            }else {
                // Go to the first one
                while( entry->prev ) entry = entry->prev;
            }
        }
    }
    while (!is_visible(entry) && menu_has_visible_items(menu)); /* skip hidden items */

    // Select the new one, which might be the same as the old one
    entry->selected = 1;
    
    if (!menu_lv_transparent_mode)
    {
        /* reset caret_position */
        /* don't reset it when LV is behind our menu, since in this case we usually want to edit many similar fields */
        caret_position = entry->unit == UNIT_TIME ? 1 : 0;
    }
    
    give_semaphore( menu_sem );

    if (junkie_mode && menu->selected)
    {
        int unused;
        junkie_selection_pos_y = junkie_get_selection_y(menu, &unused);
    }
}


// Make sure we will not display an empty menu
// If the menu or the selection is empty, move back and forth to restore a valid selection
static void menu_make_sure_selection_is_valid()
{
    struct menu * menu = get_current_menu_or_submenu();
 
    // current menu has any valid items in current mode?
    if (!menu_has_visible_items(menu))
    {
        if (submenu_level) return; // empty submenu
        menu_move(menu, -1); menu = get_selected_toplevel_menu();
        menu_move(menu, 1); menu = get_selected_toplevel_menu();
    }

    // currently selected menu entry is visible?
    struct menu_entry * entry = get_selected_menu_entry(menu);
    if (!entry) return;

    if (entry->selected && !is_visible(entry))
    {
        menu_entry_move(menu, -1);
        menu_entry_move(menu, 1);
    }
}


/*static void menu_select_current(int reverse)
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    menu_entry_select(menu,reverse);
}*/

CONFIG_INT("menu.upside.down", menu_upside_down, 0);

static void
menu_redraw_do()
{
    menu_damage = 0;
    //~ g_submenu_width = 720;

    if (!DISPLAY_IS_ON)
        return;
    if (sensor_cleaning)
        return;
    if (gui_state == GUISTATE_MENUDISP)
        return;

    if (menu_help_active)
    {
        menu_help_redraw();
        menu_damage = 0;
    }
    else
    {
        if (!lv)
            menu_lv_transparent_mode = 0;
        if (menu_lv_transparent_mode && edit_mode)
            edit_mode = 0;

        if (DOUBLE_BUFFERING)
        {
            // draw to mirror buffer to avoid flicker
            //~ bmp_idle_copy(0); // no need, drawing is fullscreen anyway
            bmp_draw_to_idle(1);
        }

        /*
        int z = zebra_should_run();
        if (menu_zebras_mirror_dirty && !z)
        {
            clear_zebras_from_mirror();
            menu_zebras_mirror_dirty = 0;
        }*/

        if (menu_lv_transparent_mode)
        {
            bmp_fill( 0, 0, 0, 720, 480 );
            
            /*
            if (z)
            {
                if (prev_z) copy_zebras_from_mirror();
                else cropmark_clear_cache(); // will clear BVRAM mirror and reset cropmarks
                menu_zebras_mirror_dirty = 1;
            }
            */
            
            if (hist_countdown == 0 && !should_draw_zoom_overlay())
                draw_histogram_and_waveform(0); // too slow
            else
                hist_countdown--;
        }
        else
        {
            bmp_fill(COLOR_BLACK, 0, 40, 720, 400 );
        }
        //~ prev_z = z;
        
        menus_display( menus, 0, 0 ); 

        if (!menu_lv_transparent_mode && !SUBMENU_OR_EDIT && !junkie_mode)
        {
            if (is_menu_active("Help"))
                menu_show_version();
        }
        
        if (menu_lv_transparent_mode) 
        {
            draw_ml_topbar();
            draw_ml_bottombar();
            bfnt_draw_char(ICON_ML_Q_BACK, 680, -5, COLOR_WHITE, NO_BG_ERASE);
        }

        if (beta_should_warn())
            draw_beta_warning();
        
        #ifdef CONFIG_CONSOLE
        console_draw_from_menu();
        #endif

        if (DOUBLE_BUFFERING)
        {
            // copy image to main buffer
            bmp_draw_to_idle(0);

            if (menu_redraw_cancel)
            {
                /* maybe next time */
                menu_redraw_cancel = 0;
            }
            else
            {
                int screen_layout = get_screen_layout();
                if (hdmi_code == 2) // copy at a smaller scale to fit the screen
                {
                    if (screen_layout == SCREENLAYOUT_16_10)
                        bmp_zoom(bmp_vram(), bmp_vram_idle(),  360,  150, /* 128 div */ 143, /* 128 div */ 169);
                    else if (screen_layout == SCREENLAYOUT_16_9)
                        bmp_zoom(bmp_vram(), bmp_vram_idle(),  360,  165, /* 128 div */ 143, /* 128 div */ 185);
                    else
                    {
                        if (menu_upside_down)
                            bmp_flip(bmp_vram(), bmp_vram_idle(), 0);
                        else
                            bmp_idle_copy(1,0);
                    }
                }
                else if (EXT_MONITOR_RCA)
                    bmp_zoom(bmp_vram(), bmp_vram_idle(),  360,  200, /* 128 div */ 135, /* 128 div */ 135);
                else
                {
                    if (menu_upside_down)
                        bmp_flip(bmp_vram(), bmp_vram_idle(), 0);
                    else
                        bmp_idle_copy(1,0);
                }
            }
            //~ bmp_idle_clear();
        }
        //~ update_stuff();
        lens_display_set_dirty();
    }
    
    bmp_on();

#ifdef FEATURE_COLOR_SCHEME  
    if (!bmp_color_scheme)
    {
        // adjust some colors for better contrast
        alter_bitmap_palette_entry(COLOR_DARK_GREEN1_MOD, COLOR_GREEN1, 100, 100);
        alter_bitmap_palette_entry(COLOR_DARK_GREEN2_MOD, COLOR_GREEN1, 200, 200);
        alter_bitmap_palette_entry(COLOR_GREEN2, COLOR_GREEN2, 300, 256);
        //~ alter_bitmap_palette_entry(COLOR_ORANGE, COLOR_ORANGE, 160, 160);
        alter_bitmap_palette_entry(COLOR_DARK_ORANGE_MOD,   COLOR_ORANGE, 160, 160);
        alter_bitmap_palette_entry(COLOR_DARK_CYAN1_MOD,   COLOR_CYAN, 60, 60);
        alter_bitmap_palette_entry(COLOR_DARK_CYAN2_MOD,   COLOR_CYAN, 128, 128);
        // alter_bitmap_palette_entry(COLOR_DARK_YELLOW_MOD,   COLOR_YELLOW, 128, 128);

        if (RECORDING)
            alter_bitmap_palette_entry(COLOR_BLACK, COLOR_BG, 256, 256);
    }
#endif

    #ifdef CONFIG_VXWORKS   
    set_ml_palette();    
    #endif
    ml_refresh_display_needed = 1;
}

void menu_benchmark()
{
    SetGUIRequestMode(1);
    msleep(1000);
    int t0 = get_ms_clock();

    for (int i = 0; i < 500; i++)
    {
        menu_redraw_do();
        bmp_printf(FONT_MED, 0, 0, "%d%% ", i/5);
    }
    int t1 = get_ms_clock();

    clrscr();
    NotifyBox(20000, "Elapsed time: %d ms", t1 - t0);
}

static int menu_ensure_canon_dialog()
{
#ifndef CONFIG_VXWORKS
    if (CURRENT_GUI_MODE != GUIMODE_ML_MENU && CURRENT_GUI_MODE != GUIMODE_PLAY)
    {
        if (redraw_flood_stop)
        {
            // Canon dialog changed?
            return 0;
        }
    }

#if defined(CONFIG_MENU_TIMEOUT_FIX)
    // refresh Canon dialog before it times out (EOSM, 6D etc)
    // apparently it's the MPU that decides to turn off the underlying Canon dialog
    // so we have to keep poking it to stay awake
    static int last_refresh = 0;
    if (lv && should_run_polling_action(2000, &last_refresh))
    {
        SetGUIRequestMode(GUIMODE_ML_MENU);
    }
#endif

#endif
    return 1;
}

static struct msg_queue *menu_redraw_queue = 0;

static void
menu_redraw_task()
{
    DryosDebugMsg(0, 15, "starting menu_redraw_task");
    menu_redraw_queue = (struct msg_queue *) msg_queue_create("menu_redraw_mq", 1);
    TASK_LOOP
    {
        /* this loop will only receive redraw messages */
        int msg;
        int err = msg_queue_receive(menu_redraw_queue, (struct event**)&msg, 500);
        if (err) {
            //DryosDebugMsg(0, 15, "err from queue, continuing anyway: 0x%x", err);
            //SJE FIXME - we see 0x9 errors.
            // There looks to be only one path where msg_queue_receive() returns 9,
            // might be useful to understand the cause
            continue;
        }
        else {
            //DryosDebugMsg(0, 15, "no err from queue");

            // SJE this is a handy place to put checks you want to run periodically
            //bmp_fill(COLOR_RED, 280, 280, 40, 40);
            //DryosDebugMsg(0, 15, "*fec8: 0x%x", *(int *)0xfec8);
            //clrscr();
        }
        
        if (gui_menu_shown())
        {
            if (get_halfshutter_pressed())
            {
                /* close menu on half-shutter */
                /* (the event is not always caught by the key handler) */
                DryosDebugMsg(0, 15, "halfshutter, skipping, menu_redraw_do");
                gui_stop_menu();
                continue;
            }

            /* make sure to check the canon dialog even if drawing is blocked
             * (for scripts and such that piggyback the ML menu) */
            if (!menu_ensure_canon_dialog())
            {
                /* didn't work, close ML menu */
                DryosDebugMsg(0, 15, "canon dialog, skipping, menu_redraw_do");
                gui_stop_menu();
                continue;
            }
            
            if (!menu_redraw_blocked)
            {
                menu_redraw_do();
            }
        }
    }
}

TASK_CREATE( "menu_redraw_task", menu_redraw_task, 0, 0x1a, 0x8000 );

void
menu_redraw()
{
    if (!DISPLAY_IS_ON)
        return;
    if (ml_shutdown_requested)
        return;
    if (menu_help_active)
        bmp_draw_request_stop();
    if (menu_redraw_queue)
        msg_queue_post(menu_redraw_queue, MENU_REDRAW);
}

static void
menu_redraw_full()
{
    if (!DISPLAY_IS_ON)
        return;
    if (ml_shutdown_requested)
        return;
    if (menu_help_active)
        bmp_draw_request_stop();
    if (menu_redraw_queue) {
        msg_queue_post(menu_redraw_queue, MENU_REDRAW);
    }
}


static struct menu * get_selected_toplevel_menu()
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    return menu;
}

// argument is optional; 0 = top-level menus; otherwise, any menu can be used
static struct menu_entry * get_selected_menu_entry(struct menu * menu)
{
    if (!menu)
    {
        /* find the currently selected top-level menu */
        menu = menus;
        for( ; menu ; menu = menu->next )
            if( menu->selected )
                break;
    }
    for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
    {
        if( entry->selected )
            return entry;
    }
    return 0;
}

static struct menu * get_current_submenu()
{
    struct menu_entry * entry = get_selected_menu_entry(0);
    if (!entry) return 0;
    
    for(int level = submenu_level; level > 1; level--)
    {
        for(entry = entry->children ; entry ; entry = entry->next )
        {
            if( entry->selected )
                break;
        }
        if(!entry) break;
    }

    if (entry && entry->children)
    {
        /* fixme */
        return menu_find_by_name(entry->name, 0);
        //return entry->children->parent_menu;
    }

    return 0;
}

static struct menu * get_current_menu_or_submenu()
{
    // Find the selected menu (should be cached?)
    struct menu * menu = get_selected_toplevel_menu();

    struct menu * main_menu = menu;
    if (submenu_level)
    {
        main_menu = menu;
        menu = get_current_submenu();

        if (!menu)
        {
            // no submenu, operate on same item
            menu = main_menu;
        }
    }

    return menu;
}

static int keyrepeat = 0;
static int keyrep_countdown = 4;
static int keyrep_ack = 0;
int handle_ml_menu_keyrepeat(struct event * event)
{
    //~ if (menu_shown || arrow_keys_shortcuts_active())
    {
        switch(event->param)
        {
            case BGMT_PRESS_LEFT:
            case BGMT_PRESS_RIGHT:
            case BGMT_PRESS_UP:
            case BGMT_PRESS_DOWN:
            #ifdef BGMT_PRESS_UP_LEFT
            case BGMT_PRESS_UP_LEFT:
            case BGMT_PRESS_UP_RIGHT:
            case BGMT_PRESS_DOWN_LEFT:
            case BGMT_PRESS_DOWN_RIGHT:
            #endif
                if (keyrepeat && event->param != keyrepeat) keyrepeat = 0;
                else keyrepeat = event->param;
                break;

            #ifdef BGMT_UNPRESS_UDLR
            case BGMT_UNPRESS_UDLR:
            #else
            case BGMT_UNPRESS_LEFT:
            case BGMT_UNPRESS_RIGHT:
            case BGMT_UNPRESS_UP:
            case BGMT_UNPRESS_DOWN:
            #endif
                keyrepeat = 0;
                keyrep_countdown = 4;
                keyrep_ack = 0;
                break;
        }
    }
    return 1;
}

void keyrepeat_ack(int button_code) // also for arrow shortcuts
{
    keyrep_ack = (button_code == keyrepeat);
}

#ifdef CONFIG_TOUCHSCREEN
int handle_ml_menu_touch(struct event * event)
{
    int button_code = event->param;
    switch (button_code) {
        case BGMT_TOUCH_1_FINGER:
            fake_simple_button(BGMT_Q);
            return 0;
        case BGMT_TOUCH_2_FINGER:
            fake_simple_button(BGMT_TRASH);
            return 0;
        case BGMT_UNTOUCH_1_FINGER:
        case BGMT_UNTOUCH_2_FINGER:
            return 0;
        default:
            return 1;
    }
    return 1;
}
#endif

int
handle_ml_menu_keys(struct event * event) 
{
    if (menu_shown || arrow_keys_shortcuts_active())
        handle_ml_menu_keyrepeat(event);

    if (!menu_shown)
        return 1;
    if (!DISPLAY_IS_ON)
    {
        if (event->param != BGMT_PRESS_HALFSHUTTER)
            return 1;
    }

    // on some cameras, scroll events may arrive grouped; we can't handle it, so split into individual events
    if (handle_scrollwheel_fast_clicks(event)==0)
        return 0;

    // rack focus may override some menu keys
    if (handle_rack_focus_menu_overrides(event)==0)
        return 0;

    if (beta_should_warn())
    {
        if (event->param == BGMT_PRESS_SET ||
            event->param == BGMT_MENU ||
            event->param == BGMT_TRASH ||
            event->param == BGMT_PLAY ||
            event->param == BGMT_PRESS_HALFSHUTTER ||
            event->param == BGMT_PRESS_UP ||
            event->param == BGMT_PRESS_DOWN ||
            event->param == BGMT_PRESS_LEFT ||
            event->param == BGMT_PRESS_RIGHT ||
            event->param == BGMT_WHEEL_UP ||
            event->param == BGMT_WHEEL_DOWN ||
            event->param == BGMT_WHEEL_LEFT ||
            event->param == BGMT_WHEEL_RIGHT
           )
        #ifndef CONFIG_RELEASE_BUILD  //gives compiling errors on 5DC
        {
            beta_set_warned();
            menu_redraw();
        }
        #endif
        if (event->param != BGMT_PRESS_HALFSHUTTER)
            return 0;
    }
    
    // Find the selected menu or submenu (should be cached?)
    struct menu * menu = get_current_menu_or_submenu();
    
    int button_code = event->param;
#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_7D) // Q not working while recording, use INFO instead
    if (button_code == BGMT_INFO && RECORDING)
        button_code = BGMT_Q;
#endif

    int menu_needs_full_redraw = 0; // if true, do not allow quick redraws
    
    switch( button_code )
    {
    case BGMT_MENU:
    {
        if (SUBMENU_OR_EDIT || menu_lv_transparent_mode || menu_help_active)
        {
            submenu_level = 0;
            edit_mode = 0;
            menu_lv_transparent_mode = 0;
            menu_help_active = 0;
        }
        else
        {
            #ifdef FEATURE_JUNKIE_MENU
            // each MENU press adjusts number of Junkie items
            // (off, 10, 20); 3 = show all (unused)
            junkie_mode = MOD(junkie_mode+1, 3);
            my_menu_dirty = 1;
            #else
            // close ML menu
            give_semaphore(gui_sem);
            #endif
        }
        break;
    }
    
    #ifdef BGMT_PRESS_UP_LEFT
    case BGMT_PRESS_UP_LEFT:
    case BGMT_PRESS_UP_RIGHT:
    case BGMT_PRESS_DOWN_LEFT:
    case BGMT_PRESS_DOWN_RIGHT:
        return 0; // ignore diagonal buttons
    #endif

    case BGMT_PRESS_HALFSHUTTER: // If they press the shutter halfway
        //~ menu_close();
        redraw_flood_stop = 1;
        give_semaphore(gui_sem);
        return 1;
    
    #if !defined(CONFIG_500D) && !defined(CONFIG_5DC) // LV is Q
    case BGMT_LV:
        if (!lv)
            return 1;
    #endif
    // fall through
    case BGMT_PRESS_ZOOM_IN:
        if (lv)
            menu_lv_transparent_mode = !menu_lv_transparent_mode;
        else
            edit_mode = !edit_mode;
        menu_damage = 1;
        menu_help_active = 0;
        break;

    case BGMT_PRESS_UP:
        if (edit_mode && !menu_lv_transparent_mode)
        {
            struct menu_entry * entry = get_selected_menu_entry(menu);
            if(entry && uses_caret_editing(entry))
            {
                menu_entry_select(menu, 0);
                break;
            }
        }
    // fall through
    case BGMT_WHEEL_UP:
        if (menu_help_active)
        {
            menu_help_prev_page();
            break;
        }

        if (edit_mode && !menu_lv_transparent_mode)
        {
            menu_entry_select(menu, 1);
        }
        else
        {
            menu_entry_move(menu, -1);
            if (menu_lv_transparent_mode) menu_needs_full_redraw = 1;
        }

        break;

    case BGMT_PRESS_DOWN:
        if (edit_mode && !menu_lv_transparent_mode)
        {
            struct menu_entry * entry = get_selected_menu_entry(menu);
            if(entry && uses_caret_editing(entry))
            {
                menu_entry_select(menu, 1);
                break;
            }
        }
    // fall through
    case BGMT_WHEEL_DOWN:
        if (menu_help_active)
        {
            menu_help_next_page();
            break;
        }
        
        if (edit_mode && !menu_lv_transparent_mode)
        {
            menu_entry_select(menu, 0);
        }
        else
        {
            menu_entry_move(menu, 1);
            if (menu_lv_transparent_mode) menu_needs_full_redraw = 1;
        }

        break;

    case BGMT_PRESS_RIGHT:
        if(EDIT_OR_TRANSPARENT)
        {
            struct menu_entry * entry = get_selected_menu_entry(menu);
            if(entry && uses_caret_editing(entry))
            {
                caret_move(entry, -1);
                menu_damage = 1;
                break;
            }
        }
    // fall through
    case BGMT_WHEEL_RIGHT:
        menu_damage = 1;
        if (menu_help_active)
        {
            menu_help_next_page();
            break;
        }
        if (SUBMENU_OR_EDIT || menu_lv_transparent_mode)
        {
            menu_entry_select(menu, 0);
        }
        else
        {
            menu_move(menu, 1);
            menu_lv_transparent_mode = 0;
            menu_needs_full_redraw = 1;
        }
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_PRESS_LEFT:
        if(EDIT_OR_TRANSPARENT)
        {
            struct menu_entry * entry = get_selected_menu_entry(menu);
            if(entry && uses_caret_editing(entry))
            {
                caret_move(entry, 1);
                menu_damage = 1;
                break;
            }
        }
    // fall through
    case BGMT_WHEEL_LEFT:
        menu_damage = 1;
        if (menu_help_active)
        {
            menu_help_prev_page();
            break;
        }
        if (SUBMENU_OR_EDIT || menu_lv_transparent_mode)
        {
            menu_entry_select(menu, 1);
        }
        else
        {
            menu_move(menu, -1);
            menu_lv_transparent_mode = 0;
            menu_needs_full_redraw = 1;
        }
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_UNPRESS_SET:
        return 0; // block Canon menu redraws

#if defined(CONFIG_7D)
    case BGMT_JOY_CENTER:
#endif
    case BGMT_PRESS_SET:
        if (menu_help_active) // pel, don't touch this!
        { 
            menu_help_active = 0;
            break; 
        }
        else if (customize_mode && !is_customize_selected(menu))
        {
            menu_entry_customize_toggle(menu);
        }
        else
        {
            menu_entry_select( menu, 3 ); // "SET" select
            menu_needs_full_redraw = 1;
        }
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_INFO:
        menu_help_active = !menu_help_active;
        menu_lv_transparent_mode = 0;
        if (menu_help_active)
        {
            /* fixme: go up one level until the help page is found */
            menu_help_go_to_selected_entry(get_selected_toplevel_menu());
        }
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;

#if 0
    case BGMT_PLAY:
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        menu_entry_select( menu, 1 ); // decrement
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;
#endif
#ifdef CONFIG_TOUCHSCREEN
    case BGMT_TOUCH_1_FINGER:
    case BGMT_TOUCH_2_FINGER:
    case BGMT_UNTOUCH_1_FINGER:
    case BGMT_UNTOUCH_2_FINGER:
        return handle_ml_menu_touch(event);
#endif

    /* Q is always defined */
    case BGMT_Q:
    case MLEV_JOYSTICK_LONG:
    case BGMT_PLAY:
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        menu_entry_select( menu, 2 ); // Q action select
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;

    default:
        // SJE The below can't be used with CONFIG_QEMU, build is broken,
        // looks like a bug with a macro
        /*
        DebugMsg( DM_MAGIC, 3, "%s: unknown event %08x? %08x %08x %x08",
            __func__,
            event,
            arg2,
            arg3,
            arg4
        );
        */
        return 1;
    }

    // If we end up here, something has been changed.
    // Reset the timeout
    
    // if submenu mode was changed, force a full redraw
    static int prev_menu_mode = 0;
    int menu_mode = submenu_level | edit_mode*2 | menu_lv_transparent_mode*4 | customize_mode*8 | junkie_mode*16;
    if (menu_mode != prev_menu_mode)
        menu_needs_full_redraw = 1;
    prev_menu_mode = menu_mode;
    
    if (menu_needs_full_redraw)
        menu_redraw_full();
    else
        menu_redraw();
    keyrepeat_ack(button_code);
    hist_countdown = 3;
    return 0;
}



void
menu_init( void )
{
    menus = NULL;
    menu_sem = create_named_semaphore( "menus", 1 );
    gui_sem = create_named_semaphore( "gui", 0 );
    DryosDebugMsg(0, 15, "created gui_sem in menu_init()");
    menu_redraw_sem = create_named_semaphore( "menu_r", 1);

    menu_find_by_name( "Audio",     ICON_ML_AUDIO   );
    menu_find_by_name( "Expo",      ICON_ML_EXPO    );
    menu_find_by_name( "Overlay",   ICON_ML_OVERLAY );
    menu_find_by_name( "Movie",     ICON_ML_MOVIE   );
    menu_find_by_name( "Shoot",     ICON_ML_SHOOT   );
    menu_find_by_name( "Focus",     ICON_ML_FOCUS   );
    menu_find_by_name( "Display",   ICON_ML_DISPLAY );
    menu_find_by_name( "Prefs",     ICON_ML_PREFS   );
    menu_find_by_name( "Scripts",   ICON_ML_SCRIPT  );
    menu_find_by_name( "Games",     ICON_ML_GAMES  );
    menu_find_by_name( "Modules",   ICON_ML_MODULES );
    menu_find_by_name( "Debug",     ICON_ML_DEBUG   );
    menu_find_by_name( "Help",      ICON_ML_INFO    );

    struct menu *m = menu_find_by_name( "Modules", 0 );
    ASSERT(m);
    m->split_pos = -11;
    m->no_name_lookup = 1;

    m = menu_find_by_name( "Help", 0 );
    ASSERT(m);
    m->no_name_lookup = 1;
}

/*
CONFIG_INT("guimode.ml.menu", guimode_ml_menu, 2);

static void
guimode_ml_menu_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "GUIMode for ML menu: %d",
        guimode_ml_menu
    );
}

static void guimode_ml_menu_inc(void* priv) { guimode_ml_menu++; }
static void guimode_ml_menu_dec(void* priv) { guimode_ml_menu--; }
*/

void
gui_stop_menu( )
{
    if (gui_menu_shown())
        give_semaphore(gui_sem);
}

void
gui_open_menu( )
{
    if (!gui_menu_shown())
        give_semaphore(gui_sem);
}

int FAST
gui_menu_shown( void )
{
    return menu_shown;
}

static void
open_canon_menu()
{
    //~ while(1)
    //~ {
        fake_simple_button(BGMT_MENU);
        int i;
        for (i = 0; i < 10; i++)
        {
            if (MENU_MODE) return;
            msleep(100);
        }
    //~ }
}

// pump a few redraws quickly, to mask Canon's back menu
void menu_redraw_flood()
{
    if (!lv) msleep(100);
    else if (EXT_MONITOR_CONNECTED) msleep(300);
    for (int i = 0; i < 5; i++)
    {
        if (redraw_flood_stop) break;
        if (!menu_shown) break;
        canon_gui_enable_front_buffer(0);
        menu_redraw_full();
        msleep(20);
    }
#ifdef FEATURE_VRAM_RGBA
    // force redraw now, it looks glitchy if you only set ml_refresh_display_needed
    refresh_yuv_from_rgb();
#endif
    msleep(50);
    redraw_flood_stop = 1;
}

static void start_redraw_flood()
{
    redraw_flood_stop = 0; 
    task_create("menu_redraw_flood", 0x1c, 0, menu_redraw_flood, 0);
}

static void piggyback_canon_menu()
{
#ifdef GUIMODE_ML_MENU
    int new_gui_mode = GUIMODE_ML_MENU;
    if (!new_gui_mode) return;
    if (sensor_cleaning) return;
    if (gui_state == GUISTATE_MENUDISP) return;
    NotifyBoxHide();
    if (new_gui_mode != (int)CURRENT_GUI_MODE) 
    { 
        start_redraw_flood();
        if (lv) bmp_off(); // mask out the underlying Canon menu :)
        SetGUIRequestMode(new_gui_mode); msleep(200); 
        // bmp will be enabled after first redraw
    }
#endif
}

static void close_canon_menu()
{
#ifdef GUIMODE_ML_MENU
    if (CURRENT_GUI_MODE == 0) return;
    if (sensor_cleaning) return;
    if (gui_state == GUISTATE_MENUDISP) return;
    if (lv) bmp_off(); // mask out the underlying Canon menu :)
    SetGUIRequestMode(0);
    msleep(100);
    // bitmap will be re-enabled in the caller
#endif
#ifdef CONFIG_5DC
    //~ forces the 5dc screen to turn off for ML menu.
    if (DISPLAY_IS_ON && !HALFSHUTTER_PRESSED) 
        fake_simple_button(BGMT_MENU);
    msleep(50);
#endif
}

static void menu_open() 
{ 
    if (menu_shown)
        return;

    DryosDebugMsg(0, 15, "in menu_open");
    
    // start in my menu, if configured
    /*
    if (start_in_my_menu)
    {
        struct menu * my_menu = menu_find_by_name(MY_MENU_NAME, 0);
        if (menu_has_visible_items(my_menu))
            select_menu_by_icon(ICON_ML_MYMENU);
    }
    */

#ifdef CONFIG_5DC
    //~ forces the 5dc screen to turn on for ML menu.
    if (!DISPLAY_IS_ON)
        fake_simple_button(BGMT_MENU);
    msleep(50);
#endif
    
    menu_lv_transparent_mode = 0;
    submenu_level = 0;
    edit_mode = 0;
    customize_mode = 0;
    menu_help_active = 0;
    keyrepeat = 0;
    menu_shown = 1;
    //~ menu_hidden_should_display_help = 0;
    if (lv)
        menu_zebras_mirror_dirty = 1;

    piggyback_canon_menu();
    canon_gui_disable_front_buffer(0);
    if (lv && EXT_MONITOR_CONNECTED)
        clrscr();

    CancelDateTimer();

    menu_redraw_full();
}

static void menu_close() 
{ 
    if (!menu_shown)
        return;
    menu_shown = false;

    customize_mode = 0;
    update_disp_mode_bits_from_params();

    lens_focus_stop();
    menu_lv_transparent_mode = 0;
    
    close_canon_menu();
    canon_gui_enable_front_buffer(0);

    #ifdef FEATURE_VRAM_RGBA
    // we need to blank the indexed RGB buffer with transparent black,
    // to remove ML menus.  Otherwise, floating elements that we want
    // to display over Canon GUI will trigger redraw, and we'll display
    // ML menu elements too.
    clrscr();
    #endif

    redraw();

    if (lv)
        bmp_on();
}

/*
void show_welcome_screen()
{
    if (menu_first_by_icon == ICON_i) // true on first startup
    {
        piggyback_canon_menu();
        canon_gui_disable();
        clrscr();
        bmp_printf(FONT_LARGE, 50, 100, 
            "  Welcome to Magic Lantern!  \n"
            "                             \n"
            "Press DELETE to open ML menu.");
        canon_gui_enable();
        SetGUIRequestMode(0);
    }
}*/

static void
menu_task( void* unused )
{
    extern int ml_started;
    while (!ml_started)
        msleep(100);
    
    debug_menu_init();
    
    int initial_mode = 0; // shooting mode when menu was opened (if changed, menu should close)
    
    TASK_LOOP
    {
        int keyrepeat_active = keyrepeat &&
            (menu_shown || arrow_keys_shortcuts_active());
        
        int transparent_menu_magic_zoom =
            should_draw_zoom_overlay() && menu_lv_transparent_mode;

        int dt = 
            (keyrepeat_active)
                ?   /* repeat delay when holding a key */
                    COERCE(100 + keyrep_countdown*5, 20, 100)
                :   /* otherwise (no keys held) */
                    (transparent_menu_magic_zoom ? 2000 : 500 );

        int rc = take_semaphore( gui_sem, dt );

        if( rc == 0 )
        {
            /* menu toggle request */
            if (menu_shown)
            {
                menu_close();
            }
            else
            {
                menu_open();
                initial_mode = shooting_mode;
            }
        }
        else
        {
            /* semaphore timeout - perform periodical checks (polling) */
            if (keyrepeat_active)
            {
                if (keyrep_ack) {
                    keyrep_countdown--;
                    if (keyrep_countdown <= 0) {
                        keyrep_ack = 0;
                        #ifndef CONFIG_DIGIC_678
                        //SJE FIXME - find out why this doesn't work, it repeats
                        // until another key is pressed
                        fake_simple_button(keyrepeat);
                        #endif
                    }
                }
                continue;
            }

            /* executed once at startup,
             * and whenever new menus appear (after menu_add) */
            if (menu_flags_load_dirty)
            {
                config_menu_reload_flags();
                menu_flags_load_dirty = 0;
            }
            
            if (menu_shown)
            {
                /* should we still display the menu? */
                if (sensor_cleaning ||
                    initial_mode != shooting_mode ||
                    gui_state == GUISTATE_MENUDISP ||
                    (!DISPLAY_IS_ON && CURRENT_GUI_MODE != GUIMODE_PLAY))
                {
                    /* close ML menu */
                    gui_stop_menu();
                    continue;
                }

                /* redraw either periodically (every 500ms),
                 * or on request (menu_damage) */
                if ((!menu_help_active && !menu_lv_transparent_mode) || menu_damage) {
                    menu_redraw();
                    ml_refresh_display_needed = 1;
                }
            }
            else
            {
                /* menu no longer displayed */
                /* if we changed anything in the menu, save config */
                extern int config_autosave;
                if (config_autosave && (config_dirty || menu_flags_save_dirty) && NOT_RECORDING && !ml_shutdown_requested)
                {
                    config_save();
                    config_dirty = 0;
                    menu_flags_save_dirty = 0;
                }
            }
        }
    }
}

static void
menu_task_minimal( void* unused )
{
    select_menu_by_icon(menu_first_by_icon);

    TASK_LOOP
    {
        int rc = take_semaphore( gui_sem, 500 );
        if( rc != 0 )
        {
            // We woke up after 1 second
            continue;
        }
        
        //~ canon_gui_toggle();
        //~ menu_shown = !canon_gui_disabled();
        //~ extern void* test_dialog;

        if( !menu_shown )
        {
            //~ menu_shown = true;
            menu_open();
        }
        else
        {
            menu_close();
            //~ menu_shown = false;
        }
    }
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1a, 0x2000 );

//~ TASK_CREATE( "menu_task_minimal", menu_task_minimal, 0, 0x1a, 0x2000 );

int is_menu_entry_selected(char* menu_name, char* entry_name)
{
    struct menu * menu = get_current_menu_or_submenu();
    if (streq(menu->name, menu_name))
    {
        struct menu_entry * entry = get_selected_menu_entry(menu);
        if (!entry) return 0;
        if (!entry->name) return 0;
        return streq(entry->name, entry_name);
    }
    return 0;
}

int is_menu_selected(char* name)
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    return streq(menu->name, name);
}

int is_menu_active(char* name)
{
    if (!menu_shown) return 0;
    if (menu_help_active) return 0;
    if (beta_should_warn()) return 0;
    return is_menu_selected(name);
}

void select_menu(char* name, int entry_index)
{
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        menu->selected = streq(menu->name, name);
        if (menu->selected)
        {
            struct menu_entry * entry = menu->children;
            for(int i = 0; entry; entry = entry->next, i++)
            {
                entry->selected = (i == entry_index);
            }
        }
    }
    //~ menu_damage = 1;
}

static void select_menu_recursive(struct menu * selected_menu, const char * entry_name)
{
    printf("select_menu %s -> %s\n", selected_menu->name, entry_name);

    /* update selection flag for all entries from this menu */
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        int menu_selected = (menu == selected_menu);

        if (!IS_SUBMENU(selected_menu))
        {
            /* only select the menu if it's at the top level */
            menu->selected = menu_selected;

            if (menu_selected)
            {
                /* update last selected menu (only at the top level); see menu_move */
                menu_first_by_icon = selected_menu->icon;
            }
        }

        if ((menu == selected_menu) && entry_name) 
        {
            /* select the requested menu entry from this menu */
            struct menu_entry * selected_entry = 0;
            for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
            {
                if (streq(entry->name, entry_name))
                {
                    selected_entry = entry;
                    break;
                }
            }
            
            if (selected_entry)
            {
                /* update selection flag for all entries from this menu */
                for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
                {
                    entry->selected = (entry == selected_entry);
                }

                /* select parent menu entry, if any */
                /* (don't do this in dynamic menus) */
                if (selected_entry->parent && menu != mod_menu && menu != my_menu)
                {
                    selected_entry = selected_entry->parent;
                    select_menu_recursive(selected_entry->parent_menu, selected_entry->name);
                    submenu_level++;
                }
            }
        }
    }
}

void select_menu_by_name(char* name, const char* entry_name)
{
    take_semaphore(menu_sem, 0);

    /* select the first menu that matches the name, if any given */
    /* otherwise, keep the selection unchanged */
    struct menu * selected_menu = 0;
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (streq(menu->name, name))
        {
            selected_menu = menu;
            break;
        }
    }

    if (selected_menu)
    {
        submenu_level = 0;
        select_menu_recursive(selected_menu, entry_name);

        /* make sure it won't display the startup screen */
        beta_set_warned();

        /* rebuild the modified settings menu */
        mod_menu_dirty = 1;
    }

    give_semaphore(menu_sem);
}

static struct menu_entry * entry_find_by_name(const char* menu_name, const char* entry_name)
{
    if (!menu_name || !entry_name)
    {
        return 0;
    }

    struct menu_entry * ans = 0;
    int count = 0;

    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        /* skip special menus */
        if (menu->no_name_lookup)
            continue;

        if (streq(menu->name, menu_name))
        {
            for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
            {
                /* skip placeholders */
                if (MENU_IS_PLACEHOLDER(entry))
                    continue;

                if (streq(entry->name, entry_name))
                {
                    ans = entry;
                    count++;
                }
            }
        }
    }

    if (count > 1)
    {
        console_show();
        printf("Duplicate menu: %s -> %s (%d)\n", menu_name, entry_name, count);
        return 0;
    }

    return ans;
}

static void select_menu_by_icon(int icon)
{
    take_semaphore(menu_sem, 0);
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->icon == icon) // found!
        {
            for (struct menu * menu = menus; menu; menu = menu->next)
            {
                menu->selected = (menu->icon == icon);
            }
            break;
        }
    }
    give_semaphore(menu_sem);
}

static void
menu_help_go_to_selected_entry(
    struct menu *   menu
)
{
    if( !menu )
        return;

    struct menu_entry * entry = get_selected_menu_entry(menu);
    if (!entry) return;
    menu_help_go_to_label((char*) entry->name, 0);
    give_semaphore(menu_sem);
}

static void menu_show_version(void)
{
    big_bmp_printf(FONT(FONT_MED, 60, MENU_BG_COLOR_HEADER_FOOTER),  10,  480 - font_med.height * 3,
        "Magic Lantern version: %s\n"
        "Git commit: %s\n"
        "Built on %s by %s.",
        build_version,
        build_id,
        build_date,
        build_user);
}

struct longpress
{
    int pressed;            /* boolean - is the key still pressed? */
    int count;              /* number of iterations since it was pressed */
    int action_disabled;    /* boolean - if something unexpected happened (such as joystick direction press while holding the center button), skip the long-press event */
    int long_btn_press;     /* what button code to send for a long press event */
    int long_btn_unpress;   /* optional: unpress code */
    int short_btn_press;    /* optional: what button code to send for a short press event */
    int short_btn_unpress;  /* optional: unpress code */
    int (*long_cbr)();      /* optional: function to tell whether long press/unpress should be sent */
    int (*short_cbr)();     /* optional: function to tell whether short press/unpress should be sent */
    int pos_x;              /* where to draw the animated indicator */
    int pos_y;              /* coords: BMP space (0,0 - 720,480 on most models) */
};

static void draw_longpress_indicator(struct longpress * longpress)
{
    /* longpress->count goes from 0 to 25; if < 15; it's considered a short press */
    /* in practice, it seems to behave as if were < 13, figure out why */

    int n = longpress->count / 2;
    int x0 = longpress->pos_x;
    int y0 = longpress->pos_y;
    int pressed = longpress->pressed;

    for (int i = 0; i < MIN(n, 12); i++)
    {
        /* a = 360 * i / 12 * pi / 180 - pi/2; x = round(15 * cos(a)); y = round(15 * sin(a)) */
        const int8_t sin_table[12] = { -15, -13, -8, 0, 7, 13, 15, 13, 8, 0, -8, -13 };
        int x = x0 + sin_table[MOD(i+3, 12)];
        int y = y0 + sin_table[MOD(i, 12)];

        int color = (!pressed)  ? COLOR_GRAY(50) :  /* button just released */
                    (n >= 25/2) ? COLOR_ORANGE   :  /* long press event fired */
                    (i <= 12/2) ? COLOR_GREEN1   :  /* interpreted short press if released */
                                  COLOR_YELLOW   ;  /* on the way to long press */
        fill_circle(x, y, 2, color);
    }
}

/* called from GUI timers */
static void longpress_check(int timer, void * opaque)
{
    struct longpress * longpress = opaque;

    if (longpress->action_disabled)
    {
        /* erase the indicator and stop here */
        redraw();
        return;
    }
    
    if (longpress->pressed)
    {
        longpress->count++;
        delayed_call(20, longpress_check, opaque);
    }
    else
    {
        /* erase the indicator and keep processing */
        redraw();
    }

    draw_longpress_indicator(longpress);

    if (longpress->count == 25)
    {
        if (!longpress->long_cbr || longpress->long_cbr())
        {
            /* long press (500ms) */
            ASSERT(longpress->long_btn_press);
            fake_simple_button(longpress->long_btn_press);

            /* optional unpress event */
            if (longpress->long_btn_unpress)
            {
                fake_simple_button(longpress->long_btn_unpress);
            }
        }

        /* make sure it won't re-trigger */
        longpress->count++;
    }
    else if (longpress->count < 15 && !longpress->pressed)
    {
        if (!gui_menu_shown())
        {
            return;
        }

        if (!longpress->short_cbr || longpress->short_cbr())
        {
            /* optional short press ( < 300 ms) */
            if (longpress->short_btn_press)
            {
                fake_simple_button(longpress->short_btn_press);
            }

            /* optional unpress event */
            if (longpress->short_btn_unpress)
            {
                fake_simple_button(longpress->short_btn_unpress);
            }
        }
    }
}

#ifdef CONFIG_LONG_PRESS_JOYSTICK_MENU
static struct longpress joystick_longpress = {
    .long_btn_press     = MLEV_JOYSTICK_LONG,   /* long press (500ms) opens ML menu */
    .short_btn_press    = BGMT_PRESS_SET,       /* short press is the same as SET in ML menu */
    #ifdef BGMT_UNPRESS_UDLR
    .short_btn_unpress  = BGMT_UNPRESS_UDLR,    /* fixme: still needed? */
    #endif
    .short_cbr          = gui_menu_shown,       /* short press only inside ML menu */
    .pos_x = 690,   /* both ML menu and Q screen; updated on trigger */
    .pos_y = 400,
};
#endif

#ifdef CONFIG_LONG_PRESS_SET_MENU
static struct longpress set_longpress = {
    .long_btn_press     = BGMT_Q,               /* long press (500ms) is interpreted as Q */
    .short_btn_press    = BGMT_PRESS_SET,       /* short press is interpreted as SET */
    #ifdef BGMT_UNPRESS_UDLR
    .short_btn_unpress  = BGMT_UNPRESS_UDLR,    /* fixme: UNPRESS_SET will be sent to the underlying Canon menu */
    #endif
    .pos_x = 690,
    .pos_y = 400,
};
#endif

#if defined(CONFIG_850D)
// This cam toggles GUI control lock with Trash button,
// and sends a different button code depending on state of the lock.
// There doesn't seem to be a single code for Trash,
// so I'm using long press Q.
static struct longpress q_longpress = {
    .long_btn_press     = BGMT_TRASH,   // long press (500ms) opens ML menu.
    .short_btn_press    = BGMT_Q,       // short press => do a regular Q
    .short_btn_unpress  = BGMT_UNPRESS_Q,
    .pos_x = 680,   /* in LiveView */
    .pos_y = 350,   /* above ExpSim */
};
#endif

#if defined(CONFIG_EOSM)
static struct longpress erase_longpress = {
    .long_btn_press     = BGMT_TRASH,           /* long press (500ms) opens ML menu */
    .short_btn_press    = BGMT_PRESS_DOWN,      /* short press => do a regular "down/erase" */
    .short_btn_unpress  = BGMT_UNPRESS_DOWN,
    .pos_x = 680,   /* in LiveView */
    .pos_y = 350,   /* above ExpSim */
};
#endif

#ifdef BGMT_Q_SET
static struct longpress qset_longpress = {
    .long_btn_press     = BGMT_Q_SET,           /* long press opens Q-menu */
    .short_btn_press    = BGMT_PRESS_SET,       /* short press => fake SET button (centering AF Frame in LV etc...) */
    .short_btn_unpress  = BGMT_UNPRESS_SET,
    .pos_x = 670,   /* outside ML menu, on the Q screen */
    .pos_y = 343,
};
#endif

// this should work on most cameras
int handle_ml_menu_erase(struct event *event)
{
// SJE if we get here, ML GUI is almost working!
//    DryosDebugMsg(0, 15, "in handle_ml_menu_erase");
    if (dofpreview)
        return 1; // don't open menu when DOF preview is locked

// SJE useful for logging buttons
//    DryosDebugMsg(0, 15, "event->param 0x%x", event->param);

// SJE logging GUIMODE
//    DryosDebugMsg(0, 15, "guimode: %d", CURRENT_GUI_MODE);

#if 0
// SJE bubbles hack for fun
    int n = 1 + rand() % 12;
    while(n)
    {
        int x = 30 + rand() % 600;
        int y = 30 + rand() % 400;
        int c = 1 + rand() % 10;
        int r = 20 + rand() % 40;
        fill_circle(x, y, r, c);
        n--;
    }
#endif

#if 0
// bmp_draw_scaled_ex() testing
    static struct bmp_file_t *image = NULL;
    if (image == NULL)
        image = bmp_load("B:/256test.bmp", 0);
    if (image != NULL)
    {
        bmp_draw_scaled_ex(image, 240, 240,
                           image->width, image->height,
                           0);
    }
#endif


#if 0
    // these are Gryp related logging callbacks
    // and log threshold values
    extern int uart_printf(const char * fmt, ...);
    if (MEM(0x115d8) == NULL)
    {
        MEM(0x115d8) = &uart_printf;
    }
    else
    {
        DryosDebugMsg(0, 15, "*115d8        : 0x%x", MEM(0x115d8));
    }
    // this is something like log priority threshold
    MEM(0x115e0) = 0x0000ffff;

    if (MEM(0x115e4) == NULL)
    {
        MEM(0x115e4) = &uart_printf;
    }
    else
    {
        DryosDebugMsg(0, 15, "*115e4        : 0x%x", MEM(0x115e4));
    }
    MEM(0x115d4) = 0x0000ffff;
#endif

    if (event->param == BGMT_TRASH ||
        #ifdef CONFIG_TOUCHSCREEN
        event->param == BGMT_TOUCH_2_FINGER ||
        #endif
       0)
    {
        #if defined(CONFIG_QEMU) && (defined(CONFIG_EOSM) || defined(CONFIG_EOSM2))
        /* allow opening ML menu from anywhere, since the emulation doesn't enter LiveView */
        int gui_state = GUISTATE_IDLE;
        #endif

        if (gui_state == GUISTATE_IDLE || (gui_menu_shown() && !beta_should_warn()))
        {
            give_semaphore( gui_sem );
            return 0;
        }
    }

    if (event->param == MLEV_JOYSTICK_LONG && !gui_menu_shown())
    {
        /* some cameras will trigger the Q menu (with photo settings) from a joystick press, others will do nothing */
        if (gui_state == GUISTATE_IDLE || gui_state == GUISTATE_QMENU)
        {
            give_semaphore( gui_sem );
            return 0;
        }
    }

    return 1;
}

int handle_longpress_events(struct event * event)
{    
#ifdef CONFIG_LONG_PRESS_JOYSTICK_MENU
    /* also trigger menu by a long joystick press */
    switch (event->param)
    {
        #ifdef BGMT_JOY_CENTER
        case BGMT_JOY_CENTER:
        {
            if (joystick_longpress.action_disabled)
            {
                return gui_menu_shown() ? 0 : 1;
            }

            if (is_submenu_or_edit_mode_active())
            {
                /* in submenus, a short press goes back to main menu (since you can edit with left and right) */
                fake_simple_button(MLEV_JOYSTICK_LONG);
                return 0;
            }
            else if (gui_state == GUISTATE_IDLE || gui_state == GUISTATE_QMENU || gui_menu_shown())
            {
                /* if we can make use of a long joystick press, check it */
                joystick_longpress.pressed = 1;
                joystick_longpress.count = 0;
                joystick_longpress.pos_x = gui_menu_shown() ? 690 : 480;    /* checked 5D3, 5D2, 50D */
                joystick_longpress.pos_y = gui_menu_shown() ? 400 : 420;    /* could be fine-tuned for each model, but... */
                delayed_call(20, longpress_check, &joystick_longpress);
                if (gui_menu_shown()) return 0;
            }
            break;
        }
        #endif

        #ifdef BGMT_UNPRESS_UDLR
        case BGMT_UNPRESS_UDLR:
        #endif
        #ifdef BGMT_UNPRESS_LEFT
        case BGMT_UNPRESS_LEFT:
        case BGMT_UNPRESS_RIGHT:
        case BGMT_UNPRESS_UP:
        case BGMT_UNPRESS_DOWN:
        #endif
        {
            joystick_longpress.pressed = 0;
            joystick_longpress.action_disabled = 0;
            break;
        }

        case BGMT_PRESS_LEFT:
        case BGMT_PRESS_RIGHT:
        case BGMT_PRESS_DOWN:
        case BGMT_PRESS_UP:
        #ifdef BGMT_PRESS_UP_LEFT
        case BGMT_PRESS_UP_LEFT:
        case BGMT_PRESS_UP_RIGHT:
        case BGMT_PRESS_DOWN_LEFT:
        case BGMT_PRESS_DOWN_RIGHT:
        #endif
        {
            joystick_longpress.action_disabled = 1;
            break;
        }
    }
#endif

#ifdef CONFIG_LONG_PRESS_SET_MENU
    /* open submenus with a long press on SET */
    /* note: if you enable this, the regular actions will be triggered
     * when de-pressing SET, which may feel a little sluggish */
    if (event->param == BGMT_PRESS_SET && !IS_FAKE(event))
    {
        if (gui_menu_shown())
        {
            set_longpress.pressed = 1;
            set_longpress.count = 0;
            delayed_call(20, longpress_check, &set_longpress);
            return 0;
        }
    }
    else if (event->param == BGMT_UNPRESS_SET)
    {
        set_longpress.pressed = 0;
    }
#endif

#if defined(CONFIG_850D)
    // trigger menu by a long press on Q
    if (event->param == BGMT_Q)
    {
        if (gui_state == GUISTATE_IDLE && !gui_menu_shown() && !IS_FAKE(event))
        {
            q_longpress.pressed = 1;
            q_longpress.count = 0;
            delayed_call(20, longpress_check, &q_longpress);
            return 0;
        }
    }
    else if (event->param == BGMT_UNPRESS_Q)
    {
        q_longpress.pressed = 0;
    }
#endif

#if defined(CONFIG_EOSM)
    /* also trigger menu by a long press on ERASE (DOWN) */
    if (event->param == BGMT_PRESS_DOWN)
    {
        if (gui_state == GUISTATE_IDLE && !gui_menu_shown() && !IS_FAKE(event))
        {
            erase_longpress.pressed = 1;
            erase_longpress.count = 0;
            delayed_call(20, longpress_check, &erase_longpress);
            return 0;
        }
    }
    else if (event->param == BGMT_UNPRESS_DOWN)
    {
        erase_longpress.pressed = 0;
    }
#endif

/* probably not the best place to implement this but let us avoid dirty hacks for now  */
/* the combined q/set button needs to return 0 for a short press and we bring back     */
/* its functionality of calling "Quick Control screen" by a long press.                */
/* canon menu C.Fn IV / Assign SET button needs to be set to 0:Quick control screen    */
/* unallowed options are 1,2,3,4,5 */
/* to avoid unnecessary discussions we could check and override it at ML boot          */
/* it will also accept a fake value */
/* ----------------------------------------------------------------------------------- */
/* this c.Fn does work for 100D on ML boot:                                            */
/* int cfn_get_setbtn_assignment()                                                     */
/* {                                                                                   */
/*     return GetCFnData(0, 7);                                                        */
/* }                                                                                   */
/*                                                                                     */
/* void cfn_set_setbtn(int value)                                                      */
/* {                                                                                   */
/*     SetCFnData(0, 7, value);                                                        */
/* }                                                                                   */
/* ---------------- below would move to boot-hack.c ---------------------------------- */
/*  100D has six options from 0 to 5                                                   */
/*  we check and assign the value to 0                                                 */
/*  #if defined(CONFIG_100D)                                                           */
/*     extern int cfn_get_setbtn_assignment();                                         */
/*     void cfn_set_setbtn(int value);                                                 */
/*     if(cfn_get_setbtn_assignment()!=0)                                              */
/*         cfn_set_setbtn(0);                                                          */
/*  #endif                                                                             */

#ifdef BGMT_Q_SET
    /* triggers Q-menu by a long press on the combined q/set button */
    if (event->param == BGMT_Q_SET)
    {
        if (gui_state == GUISTATE_IDLE && !gui_menu_shown() && !IS_FAKE(event))
        {
            qset_longpress.pressed = 1;
            qset_longpress.count = 0;
            delayed_call(20, longpress_check, &qset_longpress);
            return 0;
        }
    }
    else if (event->param == BGMT_UNPRESS_SET)
    {
        qset_longpress.pressed = 0;
    }
#endif

    return 1;
}

void menu_open_submenu(struct menu_entry * entry)
{
    submenu_level++;
    edit_mode = 0;
    menu_lv_transparent_mode = 0;
}

void menu_close_submenu()
{
    submenu_level = MAX(submenu_level - 1, 0); //make sure we don't go negative
    edit_mode = 0;
    menu_lv_transparent_mode = 0;
}

void menu_toggle_submenu()
{
    if (!edit_mode || submenu_level)
    {
        if(submenu_level == 0)
            submenu_level++;
        else
            submenu_level--;
    }
    edit_mode = 0;
    menu_lv_transparent_mode = 0;
}

int handle_quick_access_menu_items(struct event * event)
{
/* only for cameras with a native (not emulated) Q button */
#if defined(BGMT_Q) && BGMT_Q > 0
    // quick access to some menu items
    if (event->param == BGMT_Q && !gui_menu_shown())
    {
        #ifdef ISO_ADJUSTMENT_ACTIVE
        if (ISO_ADJUSTMENT_ACTIVE)
        #else
        if (0)
        #endif
        {
            select_menu("Expo", 0);
            give_semaphore( gui_sem ); 
            return 0;
        }
        #ifdef CURRENT_GUI_MODE_2
        else if (CURRENT_GUI_MODE_2 == DLG2_FOCUS_MODE)
        #else
        else if (CURRENT_GUI_MODE == GUIMODE_FOCUS_MODE)
        #endif
        {
            select_menu("Focus", 0);
            give_semaphore( gui_sem ); 
            return 0;
        }
    }
#endif
    return 1;
}

static int menu_pack_flags(struct menu_entry * entry)
{
    return
        entry->starred * 1 +
        entry->hidden  * 2 +
        entry->jhidden * 4 ;
}

static void menu_unpack_flags(struct menu_entry * entry, uint32_t flags)
{
    if (flags & 1)
        entry->starred = 1;
    if (flags & 2)
        entry->hidden  = 1;
    if (flags & 4)
        entry->jhidden = 1;
}

static int menu_parse_flags_line(
    char * buf, int i, int size, int *prev,
    char ** menu_name,
    char ** entry_name,
    uint32_t * flags,
    uint32_t * usage_counter_l,
    uint32_t * usage_counter_s
)
{
    int sep = 0;
    int start = i;

    for ( ; i < size; i++)
    {
        if (buf[i] == '\\')
        {
            sep = i;
        }
        else if (buf[i] == '\n')
        {
            if (start+20 < sep && sep+1 < i)
            {
                buf[i] = 0;
                buf[sep] = 0;
                *menu_name = &buf[start+20];
                *entry_name = &buf[sep+1];
                *flags = buf[start] - '0';
                *usage_counter_l = strtol(&buf[start+2], 0, 16);
                *usage_counter_s = strtol(&buf[start+11], 0, 16);
                return i + 1;
            }

            /* invalid line? */
            start = i + 1;
        }
    }

    /* finished */
    return -1;
}

static void menu_reload_flags(char* filename)
{
    int size = 0;
    char* buf = (char*)read_entire_file(filename , &size);
    if (!buf) return;
    int prev = -1;
    int i = 0;

    char *menu_name, *entry_name;
    uint32_t flags, usage_counter_l, usage_counter_s;

    while ((i = menu_parse_flags_line(
                    buf, i, size, &prev, 
                    &menu_name, &entry_name,
                    &flags, &usage_counter_l, &usage_counter_s)) >= 0)
    {
        /* fixme: entries with same name may give trouble */
        struct menu_entry * entry = entry_find_by_name(menu_name, entry_name);
        if (entry && !entry->cust_loaded)
        {
            menu_unpack_flags(entry, flags);
            entry->usage_counter_long_term_raw = usage_counter_l;
            entry->usage_counter_short_term_raw = usage_counter_s;
            entry->cust_loaded = 1;
        }
    }

    free(buf);
}

#define CFG_APPEND(fmt, ...) ({ cfglen += snprintf(cfg + cfglen, CFG_SIZE - cfglen, fmt, ## __VA_ARGS__); })
#define CFG_SIZE (256*1024)

static int menu_save_unloaded_flags(char* filename, char * cfg, int cfglen)
{
    int size = 0;
    char* buf = (char*)read_entire_file(filename , &size);
    if (!buf) return cfglen;
    int prev = -1;
    int i = 0;

    char *menu_name, *entry_name;
    uint32_t flags, usage_counter_l, usage_counter_s;

    while ((i = menu_parse_flags_line(
                    buf, i, size, &prev, 
                    &menu_name, &entry_name,
                    &flags, &usage_counter_l, &usage_counter_s)) >= 0)
    {
        /* fixme: entries with same name may give trouble */
        struct menu_entry * entry = entry_find_by_name(menu_name, entry_name);
        if (!entry)
        {
            CFG_APPEND("%d %08X %08X %s\\%s\n",
                flags, usage_counter_l, usage_counter_s,
                menu_name, entry_name
            );
        }
    }

    free(buf);
    return cfglen;
}

static void menu_save_flags(char* filename)
{
    menu_normalize_usage_counters();

    char* cfg = malloc(CFG_SIZE);
    cfg[0] = '\0';
    int cfglen = 0;

    cfglen = menu_save_unloaded_flags(filename, cfg, cfglen);

    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->no_name_lookup)
            continue;

        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            if (!entry->name) continue;
            if (!entry->name[0]) continue;
            if (MENU_IS_PLACEHOLDER(entry)) continue;

            uint32_t flags = menu_pack_flags(entry);

            if (flags || entry->usage_counters)
            {
                CFG_APPEND("%d %08X %08X %s\\%s\n",
                    flags,
                    entry->usage_counter_long_term_raw,
                    entry->usage_counter_short_term_raw,
                    menu->name,
                    entry->name
                );
            }
        }
    }
    
    FILE * file = FIO_CreateFile(filename);
    if (!file)
        goto end;
    
    FIO_WriteFile(file, cfg, strlen(cfg));

    FIO_CloseFile( file );

end:
    free(cfg);
}

static void config_menu_reload_flags()
{
    char menu_config_file[0x80];
    snprintf(menu_config_file, sizeof(menu_config_file), "%sMENUS.CFG", get_config_dir());
    menu_reload_flags(menu_config_file);
    my_menu_dirty = 1;
}

void config_menu_save_flags()
{
    if (!menu_flags_save_dirty) return;
    char menu_config_file[0x80];
    snprintf(menu_config_file, sizeof(menu_config_file), "%sMENUS.CFG", get_config_dir());
    menu_save_flags(menu_config_file);
}


/*void menu_save_all_items_dbg()
{
    char* cfg = malloc(CFG_SIZE);
    cfg[0] = '\0';

    int unnamed = 0;
    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            CFG_APPEND("%s\\%s\n", menu->name, entry->name);
            if (strlen(entry->name) == 0 || strlen(menu->name) == 0) unnamed++;
        }
    }
    
    FILE * file = FIO_CreateFile( "ML/LOGS/MENUS.LOG" );
    if (!file)
        return;
    
    FIO_WriteFile(file, cfg, strlen(cfg));

    FIO_CloseFile( file );
    
    NotifyBox(5000, "Menu items: %d unnamed.", unnamed);
end:
    free(cfg);
}*/

int menu_get_value_from_script(const char* name, const char* entry_name)
{
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (!entry)
    {
        printf("Menu not found: %s -> %s\n", name, entry_name);
        return INT_MIN;
    }
    
    return CURRENT_VALUE;
}

/* not thread-safe */
static char* menu_get_str_value_from_script_do(const char* name, const char* entry_name, struct menu_display_info * info)
{
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (!entry)
    {
        printf("Menu not found: %s -> %s\n", name, entry_name);
        return NULL;
    }

    /* not thread-safe; must be guarded by menu_sem */
    entry_default_display_info(entry, info);
    if (entry->update) entry->update(entry, info);
    return info->value;
}

/* requires passing a pointer to a local struct menu_display_info for thread safety */
char* menu_get_str_value_from_script(const char* name, const char* entry_name, struct menu_display_info * info)
{
    take_semaphore(menu_sem, 0);
    char* ans = menu_get_str_value_from_script_do(name, entry_name, info);
    give_semaphore(menu_sem);
    return ans;
}

int menu_set_str_value_from_script(const char* name, const char* entry_name, char* value, int value_int)
{
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (!entry)
    {
        printf("Menu not found: %s -> %s\n", name, entry_name);
        return INT_MIN;
    }

    /* if the menu item has multiple choices defined,
     * or just a valid min/max range, it's easy */
    if (IS_ML_PTR(entry->priv) && (entry->choices || (entry->max > entry->min)))
    {
        for (int i = entry->min; i < entry->max; i++)
        {
            if (streq(value, pickbox_string(entry, i)))
            {
                *(int*)(entry->priv) = i;
                return 1;
            }
        }
    }

    /* otherwise, we need to check the hard way -
     * maybe the menu logic sets some custom values that are not easy to guess */

    // entry_default_display_info is not thread safe
    take_semaphore(menu_sem, 0);
    
    // if it doesn't seem to cycle, cancel earlier
    char first[MENU_MAX_VALUE_LEN];
    char last[MENU_MAX_VALUE_LEN];
    struct menu_display_info info;
    snprintf(first, sizeof(first), "%s", menu_get_str_value_from_script_do(name, entry_name, &info));
    snprintf(last, sizeof(last), "%s", menu_get_str_value_from_script_do(name, entry_name, &info));

    /* keep cycling until we get the desired value */
    /* other stop conditions:
     * - repeats the same value
     * - goes back to initial value
     * - timeout 2 seconds
     */
    int wait_retries = 0;
    int tstart = get_ms_clock();
    for (int i = 0; get_ms_clock() - tstart < 2000; i++)
    {
        char* current = menu_get_str_value_from_script_do(name, entry_name, &info);
        if (streq(current, value))
        {
            //~ printf("menu_set('%s', '%s'): match str (%s)\n", entry_name, value, current);
            goto ok; // success!!
        }

        /* optional argument to allow numeric match? */
        if (value_int != INT_MIN && IS_ML_PTR(entry->priv) && CURRENT_VALUE == value_int)
        {
            //~ printf("menu_set('%s', '%s'): match int (%d, %s)\n", entry_name, value, value_int, current);
            goto ok; // also success!
        }

        /* accept 3500 instead of 3500K, or ON instead of ON,blahblah
         * but not 160 instead of 1600, or 1m instead of 1m10s */
        int len_val = strlen(value);
        int len_cur = strlen(current);
        if (len_val < len_cur && startswith(current, value))
        {
            /* comma after the requested value? ok, assume separator */
            if (current[len_val] == ',')
            {
                //~ printf("menu_set('%s', '%s'): match comma (%s)\n", entry_name, value, current);
                goto ok;
            }
            
            /* requested 10, got 10m? accept (but refuse 105) */
            if (len_cur == len_val + 1 && !isdigit(current[len_val]))
            {
                //~ printf("menu_set('%s', '%s'): match 1-chr suffix (%s)\n", entry_name, value, current);
                goto ok;
            }

            /* requested 10, got 10cm? accept (but refuse 10.5) */
            if (len_cur == len_val + 2 && !isdigit(current[len_val]) && !isdigit(current[len_val+1]))
            {
                //~ printf("menu_set('%s', '%s'): match 2-chr suffix (%s)\n", entry_name, value, current);
                goto ok;
            }
        }
        
        if (i > 0 && streq(current, last)) // value not changing?
        {
            if (wait_retries < 5)
            {
                /* we may need to wait for other tasks */
                //~ printf("menu_set('%s', '%s'): wait (%s, %d)\n", entry_name, value, current, retries);
                msleep(100);
                wait_retries++;
                /* check the current string again */
                continue;
            }
            
            printf("menu_set('%s', '%s'): value not changing (%s)\n", entry_name, value, current);
            break;
        }
        
        if (i > 0 && streq(current, first)) // back to first value? stop here
        {
            printf("menu_set('%s', '%s'): back to first value (%s)\n", entry_name, value, current);
            break;
        }
        
        // for debugging, print this always
        if (i > 50 && i % 10 == 0) // it's getting fishy, maybe it's good to show some progress
        {
            printf("menu_set('%s', '%s') [%d]: trying %s (%d), was %s...\n", entry_name, value, i, current, CURRENT_VALUE, last);
        }

        snprintf(last, sizeof(last), "%s", current);
        wait_retries = 0;

        if (entry->select)
        {
            /* custom menu selection logic */
            entry->select( entry->priv, 1);
            /* fixme: will crash in file_man */

            /* the custom logic might rely on other tasks to update */
            msleep(50);
        }
        else if IS_ML_PTR(entry->priv)
        {
            if (entry->max - entry->min > 1000)
            {
                /* for very long min-max ranges, don't try every single value */
                menu_numeric_toggle_fast(entry->priv, 1, entry->min, entry->max, entry->unit, entry->edit_mode, 1);
            }
            else
            {
                /* for reasonable min-max ranges, try every single value */
                (*(int*)(entry->priv))++;
                
                if (*(int*)(entry->priv) > entry->max)
                {
                    *(int*)(entry->priv) = entry->min;
                }
            }
        }
        else
        {
            printf("menu_set('%s', '%s') don't know how to toggle\n", entry_name, value);
            break;
        }
    }
    printf("Could not set value '%s' for menu %s -> %s\n", value, name, entry_name);
    give_semaphore(menu_sem);
    return 0; // boo :(

ok:
    give_semaphore(menu_sem);
    return 1; // :)
}

int menu_set_value_from_script(const char* name, const char* entry_name, int value)
{
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (!entry)
    {
        printf("Menu not found: %s -> %s\n", name, entry_name);
        return INT_MIN;
    }
    
    if( entry->select ) // special item, we need some heuristics
    {
        // we'll just cycle until either the displayed value or priv field looks alright
        char value_str[10];
        snprintf(value_str, sizeof(value_str), "%d", value);
        return menu_set_str_value_from_script(name, entry_name, value_str, value);
    }
    else if IS_ML_PTR(entry->priv) // numeric item, just set it
    {
        if (entry->max > entry->min)
        {
            /* perform range checking */
            if (value < entry->min) return 0;
            if (value > entry->max) return 0;
        }

        *(int*)(entry->priv) = value;
        return 1; // success!
    }
    else // unknown
    {
        printf("Cannot set value for %s -> %s\n", name, entry->name);
        return 0; // boo :(
    }
}

/* returns 1 if the backend is ready to use, 0 if caller should call this one again to re-check */
int menu_request_image_backend()
{
    static int last_guimode_request = 0;
    int t = get_ms_clock();
    
    if (CURRENT_GUI_MODE != GUIMODE_PLAY)
    {
        if (t > last_guimode_request + 1000)
        {
            SetGUIRequestMode(GUIMODE_PLAY);
            last_guimode_request = t;
        }
        
        /* not ready, please retry */
        return 0;
    }

    if (t > last_guimode_request + 500 && DISPLAY_IS_ON)
    {
        if (get_yuv422_vram()->vram)
        {
            /* ready to draw on the YUV buffer! */
            clrscr();
            return 1;
        }
        else
        {
            /* something might be wrong */
            yuv422_buffer_check();
        }
    }
    
    /* not yet ready, please retry */
    return 0;
}

MENU_SELECT_FUNC(menu_advanced_toggle)
{
    struct menu * menu = get_current_menu_or_submenu();
    advanced_mode = menu->advanced = !menu->advanced;
    menu->scroll_pos = 0;
}

MENU_UPDATE_FUNC(menu_advanced_update)
{
    MENU_SET_NAME(advanced_mode ? "Simple..." : "Advanced...");
    MENU_SET_ICON(IT_ACTION, 0);
    MENU_SET_HELP(advanced_mode ? "Back to 'beginner' mode." : "Advanced options for experts. Use with care.");
}

/* run something in new task, with powersave disabled
 * (usually, such actions are short-lived tasks
 * that shouldn't be interrupted by Canon's auto power off) */

struct cbr
{
    void (*user_routine)();
    int argument;
};

static void task_without_powersave(struct cbr * cbr)
{
    powersave_prohibit();
    cbr->user_routine(cbr->argument);
    free(cbr);
    powersave_permit();
}

void run_in_separate_task(void* routine, int argument)
{
    gui_stop_menu();
    if (!routine) return;
    
    struct cbr * cbr = malloc(sizeof(struct cbr));
    cbr->user_routine = routine;
    cbr->argument = argument;
    task_create("run_test", 0x1a, 0x8000, task_without_powersave, cbr);
}

/* fixme: may be slow on large menus */
static void check_duplicate_entries()
{
    duplicate_check_dirty = 0;

    info_led_on();

    for (struct menu * menu = menus; menu; menu = menu->next)
    {
        if (menu->no_name_lookup)
            continue;

        for (struct menu_entry * entry = menu->children; entry; entry = entry->next)
        {
            if (entry->shidden)
                continue;

            /* make sure each item can be looked up by name */
            /* entry_find_by_name will print a warning if there are duplicates */
            /* and it should either find the right thing, or fail */
            ASSERT(entry->name);
            struct menu_entry * e = entry_find_by_name(menu->name, entry->name);
            ASSERT(e == entry || e == 0);

            if (IS_SUBMENU(menu))
            {
                /* this entry must be linked to its parent */
                ASSERT(entry->parent);
                ASSERT(entry->parent_menu == menu);
                ASSERT(streq(entry->parent->name, menu->name));
            }
        }
    }

    info_led_off();
}
