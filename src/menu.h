/** \file
 * Menu structures and functions.
 *
 * When adding a menu item, the convention in the Magic Lantern menus
 * is that there are 12 characters for the label and up to 7 characters
 * for the value.  The box that is drawn is 540 pixels wide, enough
 * for 19 characters in FONT_LARGE.
 *
 * There is room for 8 entries in the menu.
 *
 * New menus must have a 5 character top level name.
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

#ifndef _menu_h_
#define _menu_h_

#include <stdint.h>
#include <stdbool.h>

#define MENU_FONT           FONT(FONT_LARGE,COLOR_WHITE,COLOR_BLACK)
#define MENU_FONT_SEL       MENU_FONT
#define MENU_FONT_GRAY      FONT(FONT_LARGE, entry->selected ? 60 : 50, COLOR_BLACK)

int get_menu_font_sel();
int gui_menu_shown();
void menu_show_only_selected();
int get_menu_advanced_mode();

/* call this if you want to display some image data in ML menu */
/* (will stop LiveView and go to play mode) */
int menu_request_image_backend();

struct menu_display_info
{
    char* name;
    char* value;
    char* short_name;
    char* short_value;
    char* help;
    char* warning;
    char* rinfo; // displayed on the right side
    int enabled;
    int icon;
    int icon_arg;
    enum {MENU_WARN_NONE, MENU_WARN_INFO, MENU_WARN_ADVICE, MENU_WARN_NOT_WORKING} 
        warning_level;
    enum {CUSTOM_DRAW_DISABLE, CUSTOM_DRAW_THIS_ENTRY, CUSTOM_DRAW_THIS_MENU, CUSTOM_DRAW_DO_NOT_DRAW} 
        custom_drawing;
    int x;
    int y; // for custom drawing
    int x_val;
    int can_custom_draw; //do not custom draw in junkie or MyMenu
    // etc
};

#define MENU_MAX_NAME_LEN 35
#define MENU_MAX_VALUE_LEN 25
#define MENU_MAX_SHORT_NAME_LEN 15
#define MENU_MAX_SHORT_VALUE_LEN 15
#define MENU_MAX_HELP_LEN 100
#define MENU_MAX_WARNING_LEN 100
#define MENU_MAX_RINFO_LEN 30

#define MENU_SET_NAME(fmt, ...)        snprintf(info->name,        MENU_MAX_NAME_LEN,        fmt, ## __VA_ARGS__)
#define MENU_SET_VALUE(fmt, ...)       snprintf(info->value,       MENU_MAX_VALUE_LEN,       fmt, ## __VA_ARGS__)
#define MENU_SET_SHORT_NAME(fmt, ...)  snprintf(info->short_name,  MENU_MAX_SHORT_NAME_LEN,  fmt, ## __VA_ARGS__)
#define MENU_SET_SHORT_VALUE(fmt, ...) snprintf(info->short_value, MENU_MAX_SHORT_VALUE_LEN, fmt, ## __VA_ARGS__)
#define MENU_SET_RINFO(fmt, ...)       snprintf(info->rinfo,       MENU_MAX_RINFO_LEN,       fmt, ## __VA_ARGS__)
#define MENU_SET_SHIDDEN(state)        entry->shidden=state

#define MENU_APPEND_VALUE(fmt, ...)    snprintf(info->value + strlen(info->value),   MENU_MAX_VALUE_LEN - strlen(info->value),     fmt,    ## __VA_ARGS__)
#define MENU_APPEND_RINFO(fmt, ...)    snprintf(info->rinfo + strlen(info->rinfo),   MENU_MAX_RINFO_LEN - strlen(info->rinfo),     fmt,    ## __VA_ARGS__)

/* when the item is not selected, the help and warning overrides will not be parsed */
/* warning level is still considered, for graying out menu items */

#define MENU_SET_HELP(fmt, ...) do { \
                                    if (entry->selected) \
                                        snprintf(info->help,    MENU_MAX_HELP_LEN,      fmt,    ## __VA_ARGS__); \
                                } while(0)

// only show the highest-level warning
#define MENU_SET_WARNING(level, fmt, ...) do { \
                                    if ((level) > info->warning_level) { \
                                        info->warning_level = (level); \
                                        if (entry->selected) { snprintf(info->warning, MENU_MAX_WARNING_LEN,   fmt,    ## __VA_ARGS__); } \
                                    } \
                                } while(0)

#define MENU_SET_ENABLED(val)   info->enabled = (val) // whether the feature is ON or OFF
#define MENU_SET_ICON(ico, arg)  ({ info->icon = (ico); info->icon_arg = (arg); })

struct menu_entry;
struct menu_display_info;

typedef void (*menu_select_func)(
                void * priv,
                int    delta
        );

typedef void (*menu_update_func)(                    // called before displaying
                struct menu_entry *         entry,   // menu item to be displayed
                struct menu_display_info *  info     // runtime display info
        );

#define MENU_SELECT_FUNC(func) \
    void func ( \
                void * priv, \
                int    delta \
    )

#define MENU_UPDATE_FUNC(func) \
    void func ( \
                struct menu_entry *         entry, \
                struct menu_display_info *  info \
    )


struct menu_entry
{
        struct menu_entry *     next;
        struct menu_entry *     prev;
        struct menu_entry * children;
        struct menu * parent_menu; // mostly for custom menus, so we know where each entry comes from

        const char * name;
        void * priv;
        
        int min;
        int max;
        
        const char** choices;

        menu_select_func select;
        menu_select_func select_Q;
        menu_update_func update;

        unsigned selected   : 1;

        unsigned starred    : 1; // present in "my menu"
        unsigned hidden     : 1; // hidden from main menu
        unsigned jhidden    : 1; // hidden from junkie menu
        unsigned shidden    : 1; // special hide, not toggleable by user
        
        unsigned advanced   : 1; // advanced setting in submenus; add a MENU_ADVANCED_TOGGLE if you use it

        unsigned edit_mode  : 2;
        unsigned unit       : 4;
        unsigned icon_type  : 4;
        
        const char * help;
        const char * help2;
    
        // not required for entry item, but makes it easier to declare in existing menu structures
        int16_t submenu_width; 
        int16_t submenu_height;
        
        uint32_t depends_on;     // hard requirement, won't work otherwise
        uint32_t works_best_in;  // soft requirement, it will work, but not as well
};


#define MENU_INT(entry) (IS_ML_PTR((entry)->priv) ? *(int*)(entry)->priv : 0)
#define CURRENT_VALUE (MENU_INT(entry))

// index into choices[] array
#define SELECTED_INDEX(entry) (MENU_INT(entry) - (entry)->min)

// how many choices we have (index runs from 0 to N-1)
#define NUM_CHOICES(entry) ((entry)->max - (entry)->min + 1)
#define CHOICES(...) (const char *[]) { __VA_ARGS__ }

#define EM_FEW_VALUES 0
#define EM_MANY_VALUES 1
#define EM_MANY_VALUES_LV 2

/*#define EM_FEW_VALUES 0
#define EM_MANY_VALUES 0
#define EM_MANY_VALUES_LV 0*/


#define IT_AUTO 0
#define IT_BOOL 1
#define IT_DICE 2
#define IT_PERCENT 3
#define IT_ALWAYS_ON 4
#define IT_ACTION 5
#define IT_BOOL_NEG 6
#define IT_DISABLE_SOME_FEATURE 7
//~ #define IT_DISABLE_SOME_FEATURE_NEG 8
//~ #define IT_REPLACE_SOME_FEATURE 9
#define IT_SUBMENU 10
#define IT_DICE_OFF 11
#define IT_PERCENT_OFF 12
#define IT_PERCENT_LOG 13
#define IT_PERCENT_LOG_OFF 14

#define IT_SIZE IT_DICE

#define UNIT_1_8_EV 1
#define UNIT_x10 2
#define UNIT_PERCENT 3
#define UNIT_PERCENT_x10 4
#define UNIT_ISO 5
#define UNIT_HEX 6

#define DEPENDS_ON(foo) (entry->depends_on & (foo))
#define WORKS_BEST_IN(foo) (entry->works_best_in & (foo))

#define DEP_GLOBAL_DRAW (1<<0)
#define DEP_LIVEVIEW (1<<1)
#define DEP_NOT_LIVEVIEW (1<<2)
#define DEP_MOVIE_MODE (1<<3)
#define DEP_PHOTO_MODE (1<<4)
#define DEP_AUTOFOCUS (1<<5)
#define DEP_MANUAL_FOCUS (1<<6)
#define DEP_CFN_AF_HALFSHUTTER (1<<7)
#define DEP_CFN_AF_BACK_BUTTON (1<<8)
#define DEP_EXPSIM (1<<9)
#define DEP_NOT_EXPSIM (1<<10)
#define DEP_CHIPPED_LENS (1<<11)
#define DEP_M_MODE (1<<12)
#define DEP_MANUAL_ISO (1<<13)

#define DEP_SOUND_RECORDING (1<<14)
#define DEP_NOT_SOUND_RECORDING (1<<15)

struct menu
{
        struct menu *           next;
        struct menu *           prev;
        const char *            name;
        struct menu_entry *     children;
        int                     selected;
        int icon;
        int16_t submenu_width;
        int16_t submenu_height;
        int16_t scroll_pos;
        int split_pos; // the limit between name and value columns
        char advanced;
};

#define IS_SUBMENU(menu) (menu->icon == ICON_ML_SUBMENU)

extern void
menu_print(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
);

extern void
menu_select(
        struct menu_entry *     entry
);

extern void menu_numeric_toggle(int* val, int delta, int min, int max);

extern void run_in_separate_task(void (*priv)(void), int delta);

extern void menu_add( const char * name, struct menu_entry * new_entry, int count );
extern void menu_add_base( const char * name, struct menu_entry * new_entry, int count, bool update_placeholders );

extern void menu_remove(const char * name, struct menu_entry * old_entry, int count);

extern void select_menu_by_name(char* name, const char* entry_name);

extern void
menu_init( void );

#define MNI_NONE -1
#define MNI_OFF -2
#define MNI_ON 1
#define MNI_AUTO 2
#define MNI_PERCENT 3
#define MNI_PERCENT_OFF 4
#define MNI_ACTION 5
#define MNI_DICE 6
//~ #define MNI_SIZE 7
//~ #define MNI_NAMED_COLOR 8
#define MNI_RECORD 8
#define MNI_NEUTRAL 9
#define MNI_DISABLE 10
#define MNI_SUBMENU 11
#define MNI_DICE_OFF 12
#define MNI_PERCENT_ALLOW_OFF 13

#define MNI_BOOL(x) ((x) ? MNI_ON : MNI_OFF)
#define MNI_BOOL_AUTO(x) ((x) == 1 ? MNI_ON : (x) == 0 ? MNI_OFF : MNI_AUTO)

#define MNI_STOP_DRAWING -100

#define _ZEBRAS_IN_LIVEVIEW (get_global_draw_setting() & 1)
#define GDR_WARNING_MSG ((lv && lv_disp_mode && _ZEBRAS_IN_LIVEVIEW) ? "Press " INFO_BTN_NAME " (outside ML menu) to turn Canon displays off." : get_global_draw_setting() ? "GlobalDraw is disabled, check your settings." : "GlobalDraw is OFF.")
#define EXPSIM_WARNING_MSG (get_expsim() == 0 ? "ExpSim is OFF." : "Display Gain is active.") // no other causes.. yet

// deprecated
//~ #define MNI_BOOL_GDR(x) ((x) ? ( get_global_draw() ? MNI_ON : MNI_WARNING ) : MNI_OFF), (intptr_t) GDR_WARNING_MSG
//~ #define MNI_BOOL_GDR_EXPSIM(x) ((x) ? ( get_global_draw() && (lv_luma_is_accurate() || !lv) ? MNI_ON : MNI_WARNING ) : MNI_OFF), (intptr_t)( !get_global_draw() ? GDR_WARNING_MSG : EXPSIM_WARNING_MSG )
//~ #define MNI_BOOL_LV(x) ((x) ? ( lv ? MNI_ON : MNI_WARNING ) : MNI_OFF), (intptr_t) "This option only works in LiveView." 

#define MENU_EOL_PRIV (void*)-1
#define MENU_EOL { .priv = MENU_EOL_PRIV }
#define MENU_IS_EOL(entry) ((intptr_t)(entry)->priv == -1)

#define MENU_PLACEHOLDER(namae) { .name = namae, .priv = (void*) -2, .shidden = 1 }
#define MENU_IS_PLACEHOLDER(entry) ((intptr_t)(entry)->priv == -2)

#define MENU_ADVANCED_TOGGLE { .select = menu_advanced_toggle, .update = menu_advanced_update }

extern MENU_SELECT_FUNC(menu_advanced_toggle);
extern MENU_UPDATE_FUNC(menu_advanced_update);

//~ #ifdef CONFIG_VXWORKS
#define MENU_WARNING_COLOR COLOR_RED
//~ #else
//~ #define MENU_WARNING_COLOR 254
//~ #endif


#endif
