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
#define MENU_MAX_VALUE_LEN 35
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
#define MENU_SET_ICON(ico, arg)  do { info->icon = (ico); info->icon_arg = (arg); } while(0)

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
        struct menu_entry * next;           /* [readonly] linked list pointers (set by menu_add) */
        struct menu_entry * prev;
        struct menu_entry * children;       /* [init,opt] submenu entries */
        struct menu_entry * parent;         /* [readonly] parent menu entry, only for entries in submenus */
        struct menu       * parent_menu;    /* [readonly] always valid; used for custom menus, so we know where each entry comes from */

        const char * name;          /* always valid; must be unique, as name look-ups are used often */
        void * priv;                /* [opt] usually pointer to int32 value, but can be anything */
        
        int min;                    /* [opt] valid toggle range (for int32); not enforced */
        int max;
        
        const char** choices;       /* [opt] pickbox choices (use the CHOICES macro; dynamic choices are difficult, but possible) */

        menu_select_func select;    /* [opt] function called on SET/left/right (or when changing the value from script) */
        menu_select_func select_Q;  /* [opt] function called when pressing Q or equivalent button */
        menu_update_func update;    /* [opt] function called before display (or when reading the value from script) */

        unsigned selected   : 1;    /* [readonly] only one entry from each menu has this property */

        unsigned starred    : 1;    /* [internal] present in "my menu" */
        unsigned hidden     : 1;    /* [internal] hidden from main menu */
        unsigned jhidden    : 1;    /* [internal] hidden from junkie menu  */
        unsigned jstarred   : 1;    /* [internal] in junkie menu, auto-placed in My Menu */
        unsigned shidden    : 1;    /* [opt] special hide, not toggleable from GUI, but can be set by user code */
        unsigned placeholder: 1;    /* [internal] place reserved for a future menu (see MENU_PLACEHOLDER) */
        unsigned cust_loaded: 1;    /* [internal] whether customization data was loaded (hidden/starred/jhidden/jstarred, usage counters) */
        
        unsigned advanced   : 1;    /* [opt] advanced setting in submenus; add a MENU_ADVANCED_TOGGLE if you use it */

        unsigned edit_mode  : 8;    /* [opt] EM_ constants (fine-tune edit behavior) */
        unsigned unit       : 4;    /* [opt] UNIT_ constants (fine-tune display and toggle behavior) */
        unsigned icon_type  : 4;    /* [auto-set,override] IT_ constants (to be specified only when automatic detection fails) */
        
        const char * help;          /* [opt] first help line; can be customized for each choice using \n as separator */
        const char * help2;         /* [opt] second help line (one-size-fits-all or per-choice, same as above) */
    
        /* not required for entry item, but makes it easier to declare in existing menu structures  */
        int16_t submenu_width;      /* [opt] copied to submenu, if any (see struct menu) */
        int16_t submenu_height;     /* [opt] same */
        
        /* predefined warning messages for commonly-used cases (for others cases, use MENU_SET_WARNING) */
        uint32_t depends_on;        /* [opt] hard requirement, won't work otherwise */
        uint32_t works_best_in;     /* [opt] soft requirement, it will work, but not as well */

        /* internal */
        union
        {
            uint64_t usage_counters;
            struct
            {
                union { float usage_counter_long_term;  uint32_t usage_counter_long_term_raw;  };
                union { float usage_counter_short_term; uint32_t usage_counter_short_term_raw; };
            };
        };
};


#define MENU_INT(entry) (IS_ML_PTR((entry)->priv) ? *(int*)(entry)->priv : 0)
#define CURRENT_VALUE (MENU_INT(entry))

// index into choices[] array
#define SELECTED_INDEX(entry) (MENU_INT(entry) - (entry)->min)

// how many choices we have (index runs from 0 to N-1)
#define NUM_CHOICES(entry) ((entry)->max - (entry)->min + 1)
#define CHOICES(...) (const char *[]) { __VA_ARGS__ }

#define EM_AUTO 0
#define EM_SHOW_LIVEVIEW 1

/* rounding modes */
#define EM_ROUND_ISO_R10    0x10      /* ISO 3 R"10: 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100 ... */
#define EM_ROUND_ISO_R20    0x20      /* ISO 3 R"20: 10, 11, 12, 14, 15, 18, 20, 22, 25, 28, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90, 100 ... */
#define EM_ROUND_1_2_5_10   0x40      /* 1, 2, 5, 10, 20, 50, 100 ... (modified ISO 3 R3?) */
#define EM_ROUND_POWER_OF_2 0x80      /* 1, 2, 4, 8, 16... */

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
#define UNIT_HEX 6      /* unsigned */
#define UNIT_DEC 7      /* signed */
#define UNIT_TIME 8     /* seconds */
#define UNIT_TIME_MS 9  /* milliseconds */
#define UNIT_TIME_US 10 /* microseconds */

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

#define DEP_CONTINUOUS_AF (1<<16)
#define DEP_NOT_CONTINUOUS_AF (1<<17)

struct menu
{
    struct menu *       next;               /* [auto-set] linked list pointers */
    struct menu *       prev;
    const char *        name;               /* [init] always valid (specified when creating) */
    struct menu_entry * children;           /* [auto-set] menu entries (set on menu_add) */
    struct menu_entry * parent_entry;       /* [readonly] submenus: menu entry with the same name */
    struct menu       * parent_menu;        /* [readonly] submenus: up one level */
    int                 selected;           /* [readonly] only one menu has this property */
    int                 icon;               /* [init,opt] displayed icon; IT_SUBMENU identifies submenus (in the same "namespace", just hidden) */
    int16_t             submenu_width;      /* [auto-set,opt] width of the displayed submenu (copied from menu_entry; todo: autodetect?) */
    int16_t             submenu_height;     /* [auto-set,opt] height --"-- */
    int16_t             scroll_pos;         /* [internal] number of visible items to skip (because of scroll position) */
    int16_t             split_pos;          /* [override] the limit between name and value columns; negative values are internal, positive are user overrides */
    char                advanced;           /* [internal] whether this submenu shows advanced entries or not */
    char                has_placeholders;   /* [internal] whether this menu has placeholders (to force the location of certain menu entries) */
    char                no_name_lookup;     /* [override] use to disable name lookup for this entry (e.g. entries with duplicate names, or huge menus) */
                                            /*            note: this will disable all functionality depending on name look-up (such as usage counters, selecting for My Menu etc) */
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

/* useful to start tasks directly from menu (pass the routine as .priv) */
/* note: this disables powersaving while the task is running */
extern void run_in_separate_task(void* routine, int argument);

extern void menu_add( const char * name, struct menu_entry * new_entry, int count );

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

#define MENU_PLACEHOLDER(namae) { .name = namae, .placeholder = 1, .shidden = 1 }
#define MENU_IS_PLACEHOLDER(entry) ((entry)->placeholder == 1)

#define MENU_ADVANCED_TOGGLE { .name = "Advanced...", .select = menu_advanced_toggle, .update = menu_advanced_update }

extern MENU_SELECT_FUNC(menu_advanced_toggle);
extern MENU_UPDATE_FUNC(menu_advanced_update);

//~ #ifdef CONFIG_VXWORKS
#define MENU_WARNING_COLOR COLOR_RED
//~ #else
//~ #define MENU_WARNING_COLOR 254
//~ #endif

/* post a redraw event to menu task */
void menu_redraw();

/* should be obsolete, need to double-check */
void menu_set_dirty();

/* returns true if the specified tab is selected in menu (but menu itself may not be visible) */
int is_menu_selected(char* menu_name);

/* lookup a menu entry and tell whether it's selected or not (but menu itself may not be visible) */
int is_menu_entry_selected(char* menu_name, char* entry_name);

/* returns true if menu is visible and it shows the specified tab (without any help windows open or stuff like that) */
int is_menu_active(char* name);

/* returns true if the menu is showing LiveView behind it (transparent mode); to be renamed */
int menu_active_but_hidden();

/* true if the menu active in regular mode (without LiveView behind it) */
int menu_active_and_not_hidden();

/* set menu to show LiveView behind it (this name sounds better) */
void menu_enable_lv_transparent_mode();
void menu_disable_lv_transparent_mode();

/* private stuff, to be cleaned up somehow */
extern void crop_factor_menu_init();
extern void customize_menu_init();
extern void mem_menu_init();
extern void movie_tweak_menu_init();
extern void afp_menu_init();
extern int is_submenu_or_edit_mode_active();    /* used in joypress stuff, which should be moved to menu.c */
int get_menu_edit_mode();
extern void config_menu_save_flags();

/* call this to confirm the processing of a key-repeated event (when keeping arrow keys pressed in menu) */
void keyrepeat_ack(int button_code);

void menu_open_submenu();
void menu_close_submenu();
void menu_toggle_submenu();

int menu_get_value_from_script(const char* name, const char* entry_name);
char* menu_get_str_value_from_script(const char* name, const char* entry_name, struct menu_display_info * info);
int menu_set_value_from_script(const char* name, const char* entry_name, int value);
int menu_set_str_value_from_script(const char* name, const char* entry_name, char* value, int value_int);

extern void gui_stop_menu( void );
extern void gui_open_menu( void );

#endif
