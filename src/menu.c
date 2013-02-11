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
#include "version.h"
#include "bmp.h"
#include "gui.h"
#include "config.h"
#include "property.h"
#include "lens.h"
#include "font.h"
#include "menu.h"

#define CONFIG_MENU_ICONS
//~ #define CONFIG_MENU_DIM_HACKS

#define DOUBLE_BUFFERING 1

//~ #define MENU_KEYHELP_Y_POS (menu_lv_transparent_mode ? 425 : 430)
#define MENU_HELP_Y_POS 435
#define MENU_HELP_Y_POS_2 458
#define MENU_WARNING_Y_POS (menu_lv_transparent_mode ? 425 : 458)

#define MENU_BG_COLOR_HEADER_FOOTER COLOR_GRAY40

extern int bmp_color_scheme;
#define MENU_BAR_COLOR (bmp_color_scheme ? COLOR_LIGHTBLUE : COLOR_BLUE)

#ifdef CONFIG_MENU_ICONS
#define SUBMENU_OFFSET 50
#define MENU_OFFSET 40
#else
#define SUBMENU_OFFSET 30
#define MENU_OFFSET 20
#endif

#define MY_MENU_NAME "MyMenu"

#define CAN_HAVE_PICKBOX(entry) ((entry)->max > (entry)->min && (entry)->max - (entry)->min < 25 && (entry)->priv)
#define SHOULD_HAVE_PICKBOX(entry) ((entry)->max > (entry)->min + 1 && (entry)->max - (entry)->min < 10 && (entry)->priv)
#define IS_BOOL(entry) ((entry)->max - (entry)->min == 1 && (entry)->priv)
#define IS_ACTION(entry) ((entry)->icon_type == IT_ACTION || (entry)->icon_type == IT_SUBMENU)
#define SHOULD_USE_EDIT_MODE(entry) (!IS_BOOL(entry) && !IS_ACTION(entry))

//for vscroll
#define MENU_LEN_DEFAULT 11
#define MENU_LEN_AUDIO 10 // at len=11, audio meters would overwrite menu entries on 600D
//~ #define MENU_LEN_FOCUS 8

#define MNI_WARNING 3 //private

int get_menu_len(struct menu * menu)
{
    //~ if (menu->icon == ICON_ML_AUDIO) // that's the Audio menu
        //~ return MENU_LEN_AUDIO;
    return MENU_LEN_DEFAULT;
}

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

static int menu_flags_dirty = 0;

//~ static int menu_hidden_should_display_help = 0;
static int menu_zebras_mirror_dirty = 0; // to clear zebras from mirror (avoids display artifacts if, for example, you enable false colors in menu, then you disable them, and preview LV)

int menu_help_active = 0; // also used in menuhelp.c
int menu_redraw_blocked = 0; // also used in flexinfo

static int submenu_mode = 0;
static int edit_mode = 0;
static int customize_mode = 0;

#define SUBMENU_OR_EDIT (submenu_mode || edit_mode)

static CONFIG_INT("menu.junkie", junkie_mode, 0);

static int is_customize_selected();

#define CUSTOMIZE_MODE_MYMENU (customize_mode == 1)
#define CUSTOMIZE_MODE_HIDING (customize_mode == 2)

static int g_submenu_width = 0;
//~ #define g_submenu_width 720
static int redraw_flood_stop = 0;

static int quick_redraw = 0; // don't redraw the full menu, because user is navigating quickly
static int redraw_in_progress = 0;
#define MENU_REDRAW_FULL 1
#define MENU_REDRAW_QUICK 2

static int hist_countdown = 3; // histogram is slow, so draw it less often

void menu_close_post_delete_dialog_box();
void menu_close_gmt();

int is_submenu_or_edit_mode_active() { return gui_menu_shown() && SUBMENU_OR_EDIT; }

//~ static CONFIG_INT("menu.transparent", semitransparent, 0);

static CONFIG_INT("menu.first", menu_first_by_icon, ICON_i);

void menu_set_dirty() { menu_damage = 1; }

int is_menu_help_active() { return gui_menu_shown() && menu_help_active; }

void select_menu_by_name(char* name, char* entry_name);
static void select_menu_by_icon(int icon);
static void menu_help_go_to_selected_entry(struct menu * menu);
//~ static void menu_init( void );
static void menu_show_version(void);
static struct menu * get_current_submenu();
static struct menu * get_selected_menu();
static void menu_make_sure_selection_is_valid();
static void config_menu_load_flags();
static int guess_submenu_enabled(struct menu_entry * entry);
static void menu_draw_icon(int x, int y, int type, intptr_t arg); // private
static struct menu_entry * entry_find_by_name(const char* name, const char* entry_name);
static struct menu_entry * get_selected_entry(struct menu * menu);
static void submenu_display(struct menu * submenu);

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
int beta_should_warn()
{
    int t = get_beta_timestamp();
    return beta_warn != t;
}

void beta_set_warned()
{
    unsigned t = get_beta_timestamp();
    beta_warn = t;
}
#endif

static struct menu_entry customize_menu[] = {
    {
        .name = "Customize Menus",
        .priv = &customize_mode,
        .max = 2,
        .choices = CHOICES("OFF", "MyMenu items", "Hide items"),
    }
};

static int is_customize_selected(struct menu * menu) // argument is optional, just for speedup
{
    struct menu_entry * selected_entry = get_selected_entry(menu);
    if (selected_entry == &customize_menu[0])
        return 1;
    return 0;
}

#define MY_MENU_ENTRY \
        { \
            .hidden = 1, \
            .jhidden = 1, \
        },

static MENU_UPDATE_FUNC(menu_placeholder_unused_update)
{
    info->custom_drawing = CUSTOM_DRAW_THIS_ENTRY; // do not draw it at all
    if (entry->selected && !junkie_mode)
        bmp_printf(FONT(FONT_LARGE, COLOR_GRAY45, COLOR_BLACK), 250, info->y, "(empty)");
}

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

void customize_menu_init()
{
    menu_add("Prefs", customize_menu, COUNT(customize_menu));
    menu_add(MY_MENU_NAME, my_menu_placeholders, COUNT(my_menu_placeholders));
}


static struct menu * menus;

struct menu * menu_get_root() {
  return menus;
}

void menu_numeric_toggle(int* val, int delta, int min, int max)
{
    *val = mod(*val - min + delta, max - min + 1) + min;
}


static void entry_guess_icon_type(struct menu_entry * entry)
{
    if (entry->icon_type == IT_AUTO)
    {
        if (entry->select == menu_open_submenu)
        {
            entry->icon_type = IT_SUBMENU;
        }
        else if (!entry->priv || entry->select == (void(*)(void*,int))run_in_separate_task)
        {
            entry->icon_type = IT_ACTION;
        }
        else if(entry->choices)
        {
            const char* first_choice = entry->choices[entry->min];
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
            entry->icon_type = entry->max == 1 && entry->min == 0 ? IT_BOOL : IT_PERCENT;
        }
        else
            entry->icon_type = IT_BOOL;
    }
}

static int entry_guess_enabled(struct menu_entry * entry)
{
    if (entry->icon_type == IT_BOOL || entry->icon_type == IT_DICE_OFF)
        return MENU_INT(entry);
    else if (entry->icon_type == IT_BOOL_NEG)
        return !MENU_INT(entry);
    else if (entry->icon_type == IT_SUBMENU)
        return guess_submenu_enabled(entry);
    else return 1;
}

static int guess_submenu_enabled(struct menu_entry * entry)
{
    if (entry->priv) // if it has a priv field, use it as truth value for the entire group
    {
        return MENU_INT(entry);
    }
    else 
    {   // otherwise, look in the children submenus; if one is true, then submenu icon is drawn as "true"
        struct menu_entry * e = entry->children;
        for( ; e ; e = e->next )
        {
            if (MENU_INT(e))
                return 1;
        }
        return 0;
    }
}

static void entry_draw_icon(
    struct menu_entry * entry,
    int         x,
    int         y
)
{
    entry_guess_icon_type(entry);
    
    switch (entry->icon_type)
    {
        case IT_BOOL:
            menu_draw_icon(x, y, MNI_BOOL(MENU_INT(entry)), 0);
            break;

        case IT_BOOL_NEG:
            menu_draw_icon(x, y, MNI_BOOL(!MENU_INT(entry)), 0);
            break;

        case IT_ACTION:
            menu_draw_icon(x, y, MNI_ACTION, 0);
            break;

        case IT_ALWAYS_ON:
            menu_draw_icon(x, y, MNI_ON, 0);
            break;
            
        case IT_SIZE:
            menu_draw_icon(x, y, MNI_SIZE, SELECTED_INDEX(entry) | (NUM_CHOICES(entry) << 16));
            break;

        case IT_DICE:
            menu_draw_icon(x, y, MNI_DICE, SELECTED_INDEX(entry) | (NUM_CHOICES(entry) << 16));
            break;
        
        case IT_DICE_OFF:
            menu_draw_icon(x, y, MNI_DICE_OFF, SELECTED_INDEX(entry) | (NUM_CHOICES(entry) << 16));
            break;
        
        case IT_PERCENT:
            menu_draw_icon(x, y, MNI_PERCENT, SELECTED_INDEX(entry) * 100 / (NUM_CHOICES(entry)-1));
            break;

        case IT_NAMED_COLOR:
            menu_draw_icon(x, y, MNI_NAMED_COLOR, (intptr_t) entry->choices[SELECTED_INDEX(entry)]);
            break;
        
        case IT_DISABLE_SOME_FEATURE:
            menu_draw_icon(x, y, MENU_INT(entry) ? MNI_DISABLE : MNI_NEUTRAL, 0);
            break;

        case IT_DISABLE_SOME_FEATURE_NEG:
            menu_draw_icon(x, y, MENU_INT(entry) ? MNI_NEUTRAL : MNI_DISABLE, 0);
            break;

        case IT_REPLACE_SOME_FEATURE:
            menu_draw_icon(x, y, MENU_INT(entry) ? MNI_ON : MNI_NEUTRAL, 0);
            break;
        
        case IT_SUBMENU:
        {
            int value = guess_submenu_enabled(entry);
            menu_draw_icon(x, y, MNI_SUBMENU, value);
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
menu_find_by_name(
    const char *        name,
    int icon
)
{
    take_semaphore( menu_sem, 0 );

    struct menu *       menu = menus;

    for( ; menu ; menu = menu->next )
    {
        if( streq( menu->name, name ) )
        {
            if (icon && !menu->icon) menu->icon = icon;
            give_semaphore( menu_sem );
            return menu;
        }

        // Stop just before we get to the end
        if( !menu->next )
            break;
    }

    // Not found; create it
    struct menu * new_menu = SmallAlloc( sizeof(*new_menu) );
    if( !new_menu )
    {
        give_semaphore( menu_sem );
        return NULL;
    }

    new_menu->name      = name;
    new_menu->icon      = icon;
    new_menu->prev      = menu;
    new_menu->next      = NULL; // Inserting at end
    new_menu->children  = NULL;
    new_menu->submenu_width = 0;
    new_menu->submenu_height = 0;
    new_menu->pos       = 1;
    new_menu->childnum  = 1;
    new_menu->childnummax = 1;
    new_menu->split_pos = -12;
    new_menu->delnum    = 0;
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

    give_semaphore( menu_sem );
    return new_menu;
}

static int
menu_has_visible_items(struct menu * menu)
{
    if (junkie_mode) // hide Debug and Help
    {
        if (
            streq(menu->name, "Debug") ||
            streq(menu->name, "Help") ||
            //~ streq(menu->name, "Scripts") ||
           0)
            return 0;
    }

    if (junkie_mode == 2) // also hide My Menu, since everything else is displayed
    {
        if (
            streq(menu->name, MY_MENU_NAME) ||
           0)
            return 0;
    }
    
    struct menu_entry * entry = menu->children;
    while( entry )
    {
        if (entry == &customize_menu[0]) // hide the Customize menu if everything else from Prefs is also hidden
            goto next;

        if (IS_VISIBLE(entry))
        {
            return 1;
        }
        
        next:
        entry = entry->next;
    }
    return 0;
}

static int
are_there_any_visible_menus()
{
    struct menu * menu = menus;
    while( menu )
    {
        if (!IS_SUBMENU(menu) && menu_has_visible_items(menu))
        {
            return 1;
        }
        menu = menu->next;
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
    if (entry->name && menu->split_pos < 0 && entry->priv)
    {
        menu->split_pos = -MAX(-menu->split_pos, strlen(entry->name) + 2);
        if (-menu->split_pos > 25) menu->split_pos = -25;
    }
}

void
menu_add(
    const char *        name,
    struct menu_entry * new_entry,
    int         count
)
{
#if defined(POSITION_INDEPENDENT)
    menu_fixup_pic(new_entry, count);
#endif

#if 1
    // There is nothing to display. Sounds crazy (but might result from ifdef's)
    if ( count == 0 )
        return;

    // Walk the menu list to find a menu
    struct menu *       menu = menu_find_by_name( name, 0);
    if( !menu )
        return;
    
    int count0 = count; // for submenus

    take_semaphore( menu_sem, 0 );

    struct menu_entry * head = menu->children;
    if( !head )
    {
        // First one -- insert it as the selected item
        head = menu->children   = new_entry;
        //~ if (new_entry->id == 0) new_entry->id = menu_id_increment++;
        new_entry->next     = NULL;
        new_entry->prev     = NULL;
        new_entry->selected = 1;
        menu->pos           = 1;
        menu->childnum      = 1; 
        menu->childnummax   = 1;
        //~ if (IS_SUBMENU(menu)) new_entry->essential = FOR_SUBMENU;
        menu_update_split_pos(menu, new_entry);
        new_entry++;
        count--;
    }

    // Find the end of the entries on the menu already
    while( head->next )
        head = head->next;

    for (int i = 0; i < count; i++)
    {
        //~ if (new_entry->id == 0) new_entry->id = menu_id_increment++;

        if(!HAS_HIDDEN_FLAG(new_entry)) menu->childnum++;
        menu->childnummax++;

        new_entry->selected = 0;
        //~ if (IS_SUBMENU(menu)) new_entry->essential = FOR_SUBMENU;
        new_entry->next     = head->next;
        new_entry->prev     = head;
        head->next      = new_entry;
        head            = new_entry;
        menu_update_split_pos(menu, new_entry);
        new_entry++;
    }
    give_semaphore( menu_sem );


    // create submenus

    struct menu_entry * entry = head;
    for (int i = 0; i < count0; i++)
    {
        if (entry->children)
        {
            int count = 0;
            struct menu_entry * child = entry->children;
            while (!MENU_IS_EOL(child)) 
            { 
                child->depends_on |= entry->depends_on; // inherit dependencies
                child->works_best_in |= entry->works_best_in;
                count++; 
                child++;
            }
            struct menu * submenu = menu_find_by_name( entry->name, ICON_ML_SUBMENU);
            if (submenu->children != entry->children) // submenu is reused, do not add it twice
                menu_add(entry->name, entry->children, count);
            submenu->submenu_width = entry->submenu_width;
            submenu->submenu_height = entry->submenu_height;
        }
        entry = entry->prev;
        if (!entry) break;
    }

#else
    // Maybe later...
    struct menu_entry * child = head->child;
    if( !child )
    {
        // No other child entries; add this one
        // and select it
        new_entry->highlighted  = 1;
        new_entry->prev     = NULL;
        new_entry->next     = NULL;
        head->child     = new_entry;
        return;
    }

    // Walk the child list to find the end
    while( child->next )
        child = child->next;

    // Push the new entry onto the end of the list
    new_entry->selected = 0;
    new_entry->prev     = child;
    new_entry->next     = NULL;
    child->next     = new_entry;
#endif
}

void dot(int x, int y, int color, int radius)
{
    int r;
    for (r = 0; r < radius; r++)
    {
        draw_circle(x + 16, y + 16, r, color);
        draw_circle(x + 17, y + 16, r, color);
    }
}

#ifdef CONFIG_MENU_ICONS

void maru(int x, int y, int color)
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

static void percent(int x, int y, int value)
{
    int i;
    y -= 2;
    value = value * 28 / 100;
    for (i = 0; i < 28; i++)
        draw_line(x + 2 + i, y + 25, x + 2 + i, y + 25 - i/3 - 5,
            i <= value ? 9 : 60
        );
}

static void playicon(int x, int y)
{
    int i;
    for (i = 5; i < 32-5; i++)
    {
        draw_line(x + 7, y + i, x + 25, y + 16, COLOR_YELLOW);
        draw_line(x + 7, y + i, x + 25, y + 16, COLOR_YELLOW);
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

void submenu_icon(int x, int y)
{
    //~ int color = COLOR_WHITE;
    x -= 40;
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

void submenu_only_icon(int x, int y, int color)
{
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
}

void size_icon(int x, int y, int current, int nmax)
{
    dot(x, y, COLOR_GREEN1, COERCE(current * (nmax > 2 ? 9 : 7) / (nmax-1) + 3, 1, 12));
}

void dice_icon(int x, int y, int current, int nmax, int color_on, int color_off)
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
            size_icon(x, y, current, nmax);
            break;
    }
    #undef C
}

void color_icon(int x, int y, const char* color)
{
    if (streq(color, "Red"))
        maru(x, y, COLOR_RED);
    else if (streq(color, "Green"))
        maru(x, y, COLOR_GREEN2);
    else if (streq(color, "Blue"))
        maru(x, y, COLOR_LIGHTBLUE);
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
        maru(x, y, COLOR_GRAY60);
    else if (streq(color, "RGB"))
    {
        dot(x,     y - 7, COLOR_RED, 5);
        dot(x - 7, y + 3, COLOR_GREEN2, 5);
        dot(x + 7, y + 3, COLOR_LIGHTBLUE, 5);
    }
    else if (streq(color, "ON"))
        maru(x, y, COLOR_GREEN1);
    else if (streq(color, "OFF"))
        maru(x, y, COLOR_GRAY40);
    else
    {
        dot(x,     y - 7, COLOR_CYAN, 5);
        dot(x - 7, y + 3, COLOR_RED, 5);
        dot(x + 7, y + 3, COLOR_YELLOW, 5);
    }
}

#endif // CONFIG_MENU_ICONS

void FAST selection_bar_backend(int c, int black, int x0, int y0, int w, int h)
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

int icon_drawn = 0;
static void menu_draw_icon(int x, int y, int type, intptr_t arg)
{
    if (icon_drawn) return;
    
    icon_drawn = type;

#ifdef CONFIG_MENU_ICONS

    x -= 40;
    //~ if (type != MNI_NONE && type != MNI_STOP_DRAWING) 
        //~ bmp_printf(FONT_LARGE, x, y, "  "); // cleanup background; don't call this for LCD remote icons

    switch(type)
    {
        case MNI_OFF: maru(x, y, COLOR_GRAY40); return;
        case MNI_ON: maru(x, y, COLOR_GREEN1); return;
        case MNI_DISABLE: batsu(x, y, COLOR_RED); return;
        case MNI_NEUTRAL: maru(x, y, COLOR_GRAY60); return;
        case MNI_WARNING: maru(x, y, COLOR_ORANGE); return;
        case MNI_AUTO: maru(x, y, COLOR_LIGHTBLUE); return;
        case MNI_PERCENT: percent(x, y, arg); return;
        case MNI_ACTION: playicon(x, y); return;
        case MNI_DICE: dice_icon(x, y, arg & 0xFFFF, arg >> 16, COLOR_GREEN1, COLOR_GRAY50); return;
        case MNI_DICE_OFF:
        {
            int i = arg & 0xFFFF;
            int N = arg >> 16;
            if (i == 0) dice_icon(x, y, i-1, N-1, COLOR_GRAY40, COLOR_GRAY40);
            else dice_icon(x, y, i-1, N-1, COLOR_GREEN1, COLOR_GRAY50);
            return;
        }
        case MNI_SIZE: size_icon(x, y, arg & 0xFFFF, arg >> 16); return;
        case MNI_NAMED_COLOR: color_icon(x, y, (char *)arg); return;
        case MNI_SUBMENU: submenu_only_icon(x, y, arg ? COLOR_GREEN1 : COLOR_GRAY45); return;
    }
#endif
}

// if the help text contains more lines (separated by '\n'), display the line indicated by "priv" field
// if *priv is too high, display the first line


static char* menu_help_get_line(const char* help, void* priv)
{
    char * p = strchr(help, '\n');
    if (!p) return (char*) help; // help text contains a single line, no more fuss

    // help text contains more than one line, choose the i'th one
    static char buf[70];
    int i = 0;
    if (priv) i = *(int*)priv;
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
    int len = MIN((int)sizeof(buf), end - start + 1);
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
    int w = 200;
    for (int i = lo; i <= hi; i++)
    {
        w = MAX(w, font_large.width * strlen(pickbox_string(entry, i)));
    }

    // don't draw the pickbox out of the screen
    int h = 31 * (hi-lo+1);
    
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
    
    x0 &= ~3;

    w = 720-x0+16; // extend it till the right edge

    // draw the pickbox
    bmp_fill(COLOR_GRAY45, x0-16, y0, w, h+1);
    for (int i = lo; i <= hi; i++)
    {
        int y = y0 + (i-lo) * 31;
        if (i == sel)
            selection_bar_backend(MENU_BAR_COLOR, COLOR_GRAY45, x0-16, y, w, 32);
        bmp_printf(fnt, x0, y, pickbox_string(entry, i));
    }
    
    /*
    if (entry->children)
        bmp_printf(
            SHADOW_FONT(FONT(FONT_MED, COLOR_CYAN, COLOR_GRAY45)), 
            x0, y0 + (hi-lo+1) * 31 + 5, 
            "%s" SUBMENU_HINT_SUFFIX,
            Q_BTN_NAME
        );*/
}

static void submenu_key_hint(int x, int y, int bg)
{
    bmp_fill(bg, x+12, y, 30, 30);
    bfnt_draw_char(ICON_ML_SUBMENU_KEY, x, y-5, COLOR_CYAN, COLOR_BLACK);
}

// draw submenu dots (for non-selected items)
static void submenu_marker(int x, int y)
{
    if (SUBMENU_OR_EDIT) return;
    int fnt = SHADOW_FONT(FONT(FONT_MED, COLOR_CYAN, COLOR_BLACK));
    bmp_printf(fnt, 685, y+14, "...");
}

static void menu_clean_footer()
{
    int h = 50;
    if (is_menu_active("Help")) h += 10;
    int bgu = MENU_BG_COLOR_HEADER_FOOTER;
    int fgu = COLOR_GRAY50;
    bmp_fill(fgu, 0, 480-h-1, 720, 1);
    bmp_fill(bgu, 0, 480-h, 720, h);
}

static int check_default_warnings(struct menu_entry * entry, char* warning)
{
    warning[0] = 0;
    
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
    else if (DEPENDS_ON(DEP_CFN_AF_HALFSHUTTER) && cfn_get_af_button_assignment() != AF_BTN_HALFSHUTTER)
        snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to Half-Shutter from Canon menu (CFn / custom ctrl).");
    else if (DEPENDS_ON(DEP_CFN_AF_BACK_BUTTON) && cfn_get_af_button_assignment() != AF_BTN_STAR)
        snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to back btn (*) from Canon menu (CFn / custom ctrl).");
    else if (DEPENDS_ON(DEP_EXPSIM) && !lv_luma_is_accurate())
        snprintf(warning, MENU_MAX_WARNING_LEN, EXPSIM_WARNING_MSG);
    else if (DEPENDS_ON(DEP_NOT_EXPSIM) && lv_luma_is_accurate())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires ExpSim disabled.");
    else if (DEPENDS_ON(DEP_MANUAL_FOCUS) && !is_manual_focus())
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires manual focus.");
    else if (DEPENDS_ON(DEP_CHIPPED_LENS) && !lens_info.name[0])
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires a chipped (electronic) lens.");
    else if (DEPENDS_ON(DEP_M_MODE) && shooting_mode != SHOOTMODE_M)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires Manual (M) mode.");
    else if (DEPENDS_ON(DEP_MANUAL_ISO) && !lens_info.raw_iso)
        snprintf(warning, MENU_MAX_WARNING_LEN, "This feature requires manual ISO.");
    else if (DEPENDS_ON(DEP_SOUND_RECORDING) && !SOUND_RECORDING_ENABLED)
        snprintf(warning, MENU_MAX_WARNING_LEN, (was_sound_recording_disabled_by_fps_override() && !fps_should_record_wav()) ? 
            "Sound recording was disabled by FPS override." :
            "Sound recording is disabled. Enable it from Canon menu."
        );
    else if (DEPENDS_ON(DEP_NOT_SOUND_RECORDING) && SOUND_RECORDING_ENABLED)
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
        else if (WORKS_BEST_IN(DEP_CFN_AF_HALFSHUTTER) && cfn_get_af_button_assignment() != AF_BTN_HALFSHUTTER)
            snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to Half-Shutter from Canon menu (CFn / custom ctrl).");
        else if (WORKS_BEST_IN(DEP_CFN_AF_BACK_BUTTON) && cfn_get_af_button_assignment() != AF_BTN_STAR)
            snprintf(warning, MENU_MAX_WARNING_LEN, "Set AF to back btn (*) from Canon menu (CFn / custom ctrl).");
        else if (WORKS_BEST_IN(DEP_EXPSIM) && !lv_luma_is_accurate())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with ExpSim enabled.");
        else if (WORKS_BEST_IN(DEP_NOT_EXPSIM) && lv_luma_is_accurate())
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with ExpSim disabled.");
        else if (WORKS_BEST_IN(DEP_CHIPPED_LENS) && !lens_info.name[0])
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with a chipped (electronic) lens.");
        else if (WORKS_BEST_IN(DEP_M_MODE) && shooting_mode != SHOOTMODE_M)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best in Manual (M) mode.");
        else if (WORKS_BEST_IN(DEP_MANUAL_ISO) && !lens_info.raw_iso)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with manual ISO.");
        else if (WORKS_BEST_IN(DEP_SOUND_RECORDING) && !SOUND_RECORDING_ENABLED)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with sound recording enabled.");
        else if (WORKS_BEST_IN(DEP_NOT_SOUND_RECORDING) && SOUND_RECORDING_ENABLED)
            snprintf(warning, MENU_MAX_WARNING_LEN, "This feature works best with sound recording disabled.");
        
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
    info->icon = 0;
    info->icon_arg = 0;

    entry_guess_icon_type(entry);
    info->enabled = entry_guess_enabled(entry);
    info->warning_level = check_default_warnings(entry, warning);
    
    snprintf(name, sizeof(name), "%s", entry->name);
    
    /* for junkie mode, short_name will get copied, short_value is empty by default */
    if(entry->short_name && strlen(entry->short_name))
    {
        snprintf(short_name, sizeof(short_name), "%s", entry->short_name);
    }

    if (entry->choices && SELECTED_INDEX(entry) >= 0 && SELECTED_INDEX(entry) < NUM_CHOICES(entry))
    {
        STR_APPEND(value, "%s", entry->choices[SELECTED_INDEX(entry)]);
    }

    else if (entry->priv && entry->select != (void(*)(void*,int))run_in_separate_task)
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
                    if (!MEM(entry->priv)) { STR_APPEND(value, ": Auto"); }
                    else { STR_APPEND(value, "%d", raw2iso(MEM(entry->priv))); }
                    break;
                }
                case UNIT_HEX:
                {
                    STR_APPEND(value, "0x%x", MEM(entry->priv));
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

static int get_customize_color()
{
    if (CUSTOMIZE_MODE_HIDING) return junkie_mode ? COLOR_ORANGE : COLOR_RED;
    else if (CUSTOMIZE_MODE_MYMENU) return COLOR_GREEN1;
    return 0;
}

static void
entry_print(
    int x,
    int y,
    int w,
    struct menu_entry * entry,
    struct menu_display_info * info,
    int in_submenu
)
{
    int fnt = MENU_FONT;

    if (info->warning_level == MENU_WARN_NOT_WORKING) 
        fnt = MENU_FONT_GRAY;
    
    if (submenu_mode && !in_submenu)
        fnt = MENU_FONT_GRAY;
    
    bmp_printf(
        fnt,
        x, y,
        info->name
    );

    // debug
    if (0)
        bmp_printf(FONT_SMALL, x, y, "name(%s)(%d) value(%s)(%d)", info->name, strlen(info->name), info->value, strlen(info->value));

    if (info->enabled == 0) 
        fnt = MENU_FONT_GRAY;

    // far right end
    int x_end = in_submenu ? x + g_submenu_width - SUBMENU_OFFSET : 720;
    
    w = MAX(w, strlen(info->name)+1);
    
    // value string too big? move it to the left
    int end = w + strlen(info->value);
    int wmax = (x_end - x) / font_large.width;

    // right-justified info field?
    int rlen = strlen(info->rinfo);
    int rinfo_x = x_end - font_large.width * (rlen + 1);
    if (rlen) wmax -= rlen + 2;
    
    // no right info? then make sure there's room for the Q symbol
    else if (entry->children && !in_submenu && !menu_lv_transparent_mode && (entry->priv || entry->select))
    {
        wmax--;
    }
    
    if (end > wmax)
        w -= (end - wmax);

    // print value field
    bmp_printf(
        fnt,
        x + font_large.width * w, y,
        "%s",
        info->value
    );

    // print right-justified info, if any
    if (info->rinfo[0])
    {
        bmp_printf(
            MENU_FONT_GRAY,
            rinfo_x, y,
            "%s",
            info->rinfo
        );
    }

    // selection bar params

    // bar middle
    int xc = x - 5;
    if ((in_submenu || edit_mode) && info->value[0])
        xc = x + font_large.width * w - 15;
    
    // customization markers
    if (customize_mode)
    {
        // reserve space for icons
        if (entry != &customize_menu[0] && !(is_menu_active(MY_MENU_NAME) && !in_submenu))
            xc = x_end - 70;
        else
            xc = x_end;

        // star marker
        if (entry->starred)
            bfnt_draw_char(ICON_ML_MYMENU, xc, y-4, COLOR_GREEN1, COLOR_BLACK);
        
        // hidden marker
        if (HAS_CURRENT_HIDDEN_FLAG(entry))
            batsu(xc+37, y+2, junkie_mode ? COLOR_ORANGE : COLOR_RED);
    }

    // selection bar
    if (entry->selected)
    {
        int color_left = COLOR_GRAY45;
        int color_right = MENU_BAR_COLOR;
        if (junkie_mode && !in_submenu) color_left = color_right = COLOR_BLACK;
        if (customize_mode) { color_left = get_customize_color(); color_right = COLOR_GRAY40; }

        selection_bar_backend(color_left, COLOR_BLACK, x-5, y, xc-x+5, 31);
        selection_bar_backend(color_right, COLOR_BLACK, xc, y, x_end-xc, 31);
        
        // use a pickbox if possible
        if (edit_mode && CAN_HAVE_PICKBOX(entry))
        {
            int px = x + font_large.width * w;
            pickbox_draw(entry, px, y);
        }
    }

    // display help
    if (entry->selected && entry->help && !menu_lv_transparent_mode)
    {
        bmp_printf(
            FONT(FONT_MED, COLOR_WHITE, MENU_BG_COLOR_HEADER_FOOTER), 
             10,  MENU_HELP_Y_POS, 
            "%s",
            entry->help
        );

        if (entry->help2)
        {
            bmp_printf(
                FONT(FONT_MED, COLOR_WHITE, MENU_BG_COLOR_HEADER_FOOTER), 
                 10,  MENU_HELP_Y_POS_2, 
                 "%s",
                 menu_help_get_line(entry->help2, entry->priv)
            );
        }
    }

    // if there's a warning message set, display it
    if (entry->selected && info->warning[0])
    {
        bmp_fill(MENU_BG_COLOR_HEADER_FOOTER, 10, MENU_WARNING_Y_POS, 720, font_med.height);
        bmp_printf(
            FONT(FONT_MED, COLOR_YELLOW, MENU_BG_COLOR_HEADER_FOOTER),
             10,  MENU_WARNING_Y_POS, 
                info->warning
        );
    }

    // warning icon, if any
    if (info->warning_level == MENU_WARN_NOT_WORKING && info->enabled)
    {
        if (entry->icon_type == IT_SUBMENU)
            submenu_only_icon(x-40, y, COLOR_ORANGE);
        else
            menu_draw_icon(x, y, MNI_WARNING, 0);
    }
    else // no warning, draw normal icons
    {
        // overriden icon has the highest priority
        if (info->icon)
            menu_draw_icon(x, y, info->icon, info->icon_arg);
        
        if (!info->enabled && entry->icon_type == IT_PERCENT)
            menu_draw_icon(x, y, MNI_OFF, 0);
        
        if (entry->icon_type == IT_BOOL)
            menu_draw_icon(x, y, info->enabled ? MNI_ON : MNI_OFF, 0);
        
        entry_draw_icon(entry, x, y);
    }

    // display submenu marker if this item has a submenu
    if (entry->children && !menu_lv_transparent_mode)
        submenu_icon(x, y);

    // and also a Q sign for selected item
    if (entry->selected && (entry->priv || entry->select) && !customize_mode)
    {
        if (entry->children && !SUBMENU_OR_EDIT && !menu_lv_transparent_mode)
        {
            if (icon_drawn != MNI_SUBMENU)
                submenu_key_hint(720-35, y, junkie_mode ? COLOR_BLACK : MENU_BAR_COLOR);
        }
    }
}

static void
menu_post_display()
{

    if (!CURRENT_DIALOG_MAYBE)
    {
        // we can't use the scrollwheel
        // and you need to be careful because you will change shooting settings while recording!
        bfnt_draw_char(ICON_MAINDIAL, 680, 395, MENU_WARNING_COLOR, MENU_BG_COLOR_HEADER_FOOTER);
        draw_line(720, 405, 680, 427, MENU_WARNING_COLOR);
        draw_line(720, 406, 680, 428, MENU_WARNING_COLOR);
    }

    // display help about how to customize the menu
    if (customize_mode)
    {
        bmp_printf(
            FONT(FONT_MED, get_customize_color(), MENU_BG_COLOR_HEADER_FOOTER),
             10,  MENU_HELP_Y_POS_2, 
                CUSTOMIZE_MODE_HIDING ? "Press SET to show/hide items you don't use.                 " :
                CUSTOMIZE_MODE_MYMENU ? "Press SET to choose your favorite items for MyMenu.         " : ""
        );
    }
}

static int
menu_entry_process(
    struct menu * menu,
    struct menu_entry * entry,
    int         x,
    int         y, 
    int only_selected
)
{
    //~ if (quick_redraw && !entry->selected)
        //~ return 1;

    // fill in default text, warning checks etc 
    static struct menu_display_info info;
    entry_default_display_info(entry, &info);
    info.x = x;
    info.y = y;
    info.x_val = x + font_large.width * ABS(menu->split_pos);

    // display icon (only the first icon is drawn)
    icon_drawn = 0;

    if ((!menu_lv_transparent_mode && !only_selected) || entry->selected)
    {
        if (quick_redraw) // menu was not erased, so there may be leftovers on the screen
            bmp_fill(menu_lv_transparent_mode ? 0 : COLOR_BLACK, x-8, y, g_submenu_width-x+8, font_large.height);
        
        // should we override some things?
        if (entry->update)
            entry->update(entry, &info);

        // menu->update asked to draw the entire screen by itself? stop drawing right now
        if (info.custom_drawing == CUSTOM_DRAW_THIS_MENU)
            return 0;
        
        // print the menu on the screen
        if (info.custom_drawing == CUSTOM_DRAW_DISABLE)
            entry_print(x, y, ABS(menu->split_pos), entry, &info, IS_SUBMENU(menu));
    }
    return 1;
}

static int
my_menu_update()
{
    struct menu * my_menu = menu_find_by_name(MY_MENU_NAME, 0);

    my_menu->split_pos = -12;

    int i = 0;
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        if (menu == my_menu)
            continue;

        struct menu_entry * entry = menu->children;
        
        for(; entry ; entry = entry->next)
        {
            if (entry->starred)
            {
                if (i >= COUNT(my_menu_placeholders)) // too many starred items
                    return 0; // whoops
                
                struct menu_entry * my_entry = &(my_menu_placeholders[i]);
                i++;
                
                // copy most things from old menu structure to this one
                // except for some essential things :P
                void* next = my_entry->next;
                void* prev = my_entry->prev;
                int selected = my_entry->selected;
                my_memcpy(my_entry, entry, sizeof(struct menu_entry));
                my_entry->next = next;
                my_entry->prev = prev;
                my_entry->selected = selected;
                my_entry->shidden = 0;
                my_entry->hidden = 0;
                my_entry->jhidden = 0;
                my_entry->starred = 0;
                
                // update split position
                menu_update_split_pos(my_menu, my_entry);
            }
        }
    }
    
    for ( ; i < COUNT(my_menu_placeholders); i++)
    {
        struct menu_entry * my_entry = &(my_menu_placeholders[i]);
        my_entry->shidden = 1;
        my_entry->hidden = 1;
        my_entry->jhidden = 1;
        my_entry->name = 0;
        my_entry->priv = 0;
        my_entry->select = 0;
        my_entry->select_Q = 0;
        my_entry->update = menu_placeholder_unused_update;
    }
    
    return 1; // success
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
    int menu_len = get_menu_len(menu); 

    int delnum = menu->delnum; // how many menu entries to skip
    delnum = MAX(delnum, menu->pos - menu_len);
    delnum = MIN(delnum, menu->pos - 1);
    menu->delnum = delnum;
    
    //~ NotifyBox(1000, "%d %d ", delnum, menu->pos);

    for(int i=0;i<delnum;i++){
        while(!IS_VISIBLE(entry)) entry = entry->next;
        entry = entry->next;
    }
    //<== vscroll

    if (!menu_lv_transparent_mode)
        menu_clean_footer();

    int menu_entry_num = 0;
    while( entry )
    {
        if (IS_VISIBLE(entry))
        {
            // display current entry
            int ok = menu_entry_process(menu, entry, x, y, only_selected);
            
            // entry asked for custom draw? stop here
            if (!ok) break;
            
            // move down for next item
            y += font_large.height;

            //hide buttom menu for vscroll
            menu_entry_num++;
            if(menu_entry_num >= menu_len) break;
            //<== vscroll
        }

        entry = entry->next;
    }

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
static char* junkie_get_shortname(struct menu_display_info * info, int maxlen)
{
    static char tmp[30];
    static char sname[20];
    if (maxlen > 19) maxlen = 19;

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

    int i,j;
    for (i = 0, j = 0; i < maxlen && j < N; j++)
    {
        char c = tmp[j];
        if (c == ' ') { tmp[j+1] = toupper(tmp[j+1]); continue; }
        if (c == '.') continue;
        if (c == '(') break;
        if (maxlen < 5 && islower(c)) continue;
        sname[i] = c;
        i++;
    }
    sname[i] = 0;
    
    return sname;
}

static char* junkie_get_shortvalue(struct menu_display_info * info, int maxlen)
{
    static char tmp[30];
    static char svalue[20];
    if (maxlen > 19) maxlen = 19;

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

    int i,j;
    for (i = 0, j = 0; i < maxlen && j < N; j++)
    {
        char c = tmp[j];
        if (c == ' ') continue;
        if (c == '(') break;
        svalue[i] = c;
        i++;
    }
    svalue[i] = 0;
    
    return svalue;
}

static char* junkie_get_shorttext(struct menu_display_info * info, int maxlen)
{
    // print name or value?
    if (streq(info->value, "ON") || streq(info->value, "Default") || startswith(info->value, "OFF") || streq(info->value, "Normal") || (!info->value[0] && !info->short_value[0]))
    {
        // ON/OFF is obvious by color; print just the name
        return junkie_get_shortname(info, maxlen);
    }
    else // print value only
    {
        char* svalue = junkie_get_shortvalue(info, maxlen);
        int len = strlen(svalue);
        if (maxlen - len >= 4) // still plenty of space? try to print part of name too
        {
            static char nv[30];
            char* sname = junkie_get_shortname(info, maxlen - len - 1);
            if (strlen(sname) > 1)
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

    int fg = COLOR_GRAY60;
    int bg = COLOR_GRAY40;

    if (info->warning_level == MENU_WARN_NOT_WORKING) 
    {
        if (info->enabled)
        {
            bg = COLOR_GRAY50;
            fg = COLOR_BLACK;
        }
        else
        {
            fg = COLOR_GRAY45;
            bg = COLOR_GRAY40;
        }
    }
    else if (info->enabled)
    {
        bg = COLOR_GREEN1;
        fg = COLOR_BLACK;
    }
    
    w -= 2;
    x += 1;
    h -= 1;

    if (sel) // display the full selected entry normally
    {
        entry_print(40, 395, 10, entry, info, 0);
        
        // brighten the selection
        if (bg == COLOR_GREEN1) bg = COLOR_GREEN2;
        //~ else if (bg == COLOR_ORANGE) bg = COLOR_PINK; // modified to look like bright orange
        else if (bg == COLOR_GRAY40) bg = 42;
        else if (bg == COLOR_GRAY50) bg = 55;

        if (fg == COLOR_GRAY60) fg = COLOR_WHITE;
        else if (fg == COLOR_GRAY45) fg = 55;
    }

    int sh = bg;
    if (bg == COLOR_GRAY40 || bg == 42) sh = COLOR_BLACK;

    int fnt = SHADOW_FONT(FONT(FONT_MED, fg, sh));

    if (h > 30 && w > 130) // we can use large font when we have 5 or fewer tabs
        fnt = SHADOW_FONT(FONT(FONT_LARGE, fg, sh));

    int maxlen = (w - 8) / fontspec_width(fnt);

    bmp_fill(bg, x+2, y+2, w-4+1, h-4+1);
    //~ bmp_draw_rect(bg, x+2, y+2, w-4, h-4);

    char* shorttext = junkie_get_shorttext(info, maxlen);
    
    bmp_printf(
        fnt,
        x + (w - fontspec_width(fnt) * strlen(shorttext)) / 2 + 2, 
        y + (h - fontspec_height(fnt)) / 2,
        shorttext
    );

    // selection bar params
    
    // selection bar
    int selc = sel ? COLOR_WHITE : COLOR_BLACK; //menu_selected ? COLOR_BLUE : entry->selected ? COLOR_BLACK : COLOR_BLACK;
    bmp_draw_rect(selc, x, y, w, h);
    bmp_draw_rect(selc, x+1, y+1, w-2, h-2);
    bmp_draw_rect(selc, x+2, y+2, w-4, h-4);
    draw_line(x, y+h+1, x+w, y+h+1, selc);
    
    // round corners
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

    // customization markers
    if (customize_mode)
    {
        // star marker
        if (entry->starred)
            bmp_printf(SHADOW_FONT(FONT(FONT_LARGE, COLOR_GREEN2, COLOR_BLACK)), x+5, y, "*");
        
        // hidden marker
        if (entry->jhidden)
            bmp_printf(SHADOW_FONT(FONT(FONT_LARGE, COLOR_ORANGE, COLOR_BLACK)), x+w-20, y, "x");
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

    // display icon (only the first icon is drawn)
    icon_drawn = 0;

    //~ if ((!menu_lv_transparent_mode && !only_selected) || entry->selected)
    {
        //~ if (quick_redraw) // menu was not erased, so there may be leftovers on the screen
            //~ bmp_fill(menu_lv_transparent_mode ? 0 : COLOR_BLACK, x-8, y, g_submenu_width-x+8, font_large.height);
        
        // should we override some things?
        if (entry->update)
            entry->update(entry, &info);

        // menu->update asked to draw the entire screen by itself? stop drawing right now
        if (info.custom_drawing == CUSTOM_DRAW_THIS_MENU)
            return 0;
        
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
    struct menu_entry * entry = menu->children;
    int num = 0;
    while( entry )
    {
        if (IS_VISIBLE(entry)) num++;
        entry = entry->next;
    }
    entry = menu->children;
    
    int space_left = 330;
    *h = space_left / num;
    
    int y = 0;

    while( entry )
    {
        if (IS_VISIBLE(entry))
        {
            // move down for next item
            int dh = space_left / num;
            if (entry->selected) // found!
                return y + dh/2;
            y += dh;
            space_left -= dh;
            num--;
        }
        entry = entry->next;
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
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
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
    struct menu_entry * entry = menu->children;

    int num = 0;
    while( entry )
    {
        if (IS_VISIBLE(entry)) num++;
        entry = entry->next;
    }
    entry = menu->children;
    
    int h = 330 / num;
    int space_left = 330;

    if (!menu_lv_transparent_mode && menu->selected)
        menu_clean_footer();

    while( entry )
    {
        if (IS_VISIBLE(entry))
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

        entry = entry->next;
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
        char hidden_msg[70];
        snprintf(hidden_msg, sizeof(hidden_msg), "Hidden: ");
        int hidden_count = 0;

        struct menu_entry * entry = menu->children;
        while( entry )
        {
            if (HAS_HIDDEN_FLAG(entry) && entry->name)
            {
                if (hidden_count) { STR_APPEND(hidden_msg, ", "); }
                int len = strlen(hidden_msg);
                STR_APPEND(hidden_msg, "%s", entry->name);
                while (isspace(hidden_msg[strlen(hidden_msg)-1])) hidden_msg[strlen(hidden_msg)-1] = '\0';
                while (ispunct(hidden_msg[strlen(hidden_msg)-1])) hidden_msg[strlen(hidden_msg)-1] = '\0';
                hidden_msg[MIN(len+15, (int)sizeof(hidden_msg))] = '\0';
                hidden_count++;
            }
            entry = entry->next;
        }
        STR_APPEND(hidden_msg, CUSTOMIZE_MODE_HIDING ? "." : " (Prefs->Customize).");
        
        if (strlen(hidden_msg) > 60)
        {
            hidden_msg[59] = hidden_msg[58] = hidden_msg[57] = '.';
            hidden_msg[60] = '\0';
        }

        int hidden_pos_y = 410;
        if (is_menu_active("Help")) hidden_pos_y -= font_med.height;
        if (hidden_count)
        {
            bmp_fill(COLOR_BLACK, 0, hidden_pos_y, 720, 19);
            bmp_printf(
                SHADOW_FONT(FONT(FONT_MED, CUSTOMIZE_MODE_HIDING ? MENU_WARNING_COLOR : COLOR_ORANGE , MENU_BG_COLOR_HEADER_FOOTER)), 
                 10, hidden_pos_y, 
                 hidden_msg
            );
        }
    }
}

static void
show_vscroll(struct menu* parent){
    int16_t pos = parent->pos; // from 1 to max
    int16_t max;

    if(customize_mode) max = parent->childnummax;
    else max = parent->childnum;

    int menu_len = get_menu_len(parent);
    
    if(max > menu_len){
        bmp_draw_rect(COLOR_GRAY50, 718, 43, 1, 385);
        int16_t posx = 43 + (335 * (pos-1) / (max-1));
        bmp_fill(COLOR_WHITE, 717, posx, 4, 50);
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

    struct menu * submenu = 0;
    if (submenu_mode)
        submenu = get_current_submenu();

    if (junkie_mode) junkie_sync_selection();

    take_semaphore( menu_sem, 0 );

    extern int override_zoom_buttons; // from focus.c
    override_zoom_buttons = 0; // will override them only if rack focus items are selected

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
    
    int x = orig_x + junkie_mode ? 0 : 150;
    int icon_spacing = junkie_mode ? 720 / num_tabs : (720 - 150) / num_tabs;
    
    int bgs = COLOR_BLACK;
    int bgu = MENU_BG_COLOR_HEADER_FOOTER;
    int fgu = COLOR_GRAY50;
    int fgs = COLOR_WHITE;

    if (customize_mode) fgs = get_customize_color();

    bmp_fill(bgu, orig_x, y, 720, 42);
    bmp_fill(fgu, orig_x, y+42, 720, 1);
    
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
                bmp_fill(bg, x, y+2, icon_spacing, 38);

            int icon_char = menu->icon ? menu->icon : menu->name[0];
            int icon_width = bfnt_char_get_width(icon_char);
            int x_ico = (x & ~3) + (icon_spacing - icon_width) / 2;
            bfnt_draw_char(icon_char, x_ico, y + 2, fg, bg);
            //~ bmp_printf(FONT_MED, x_ico, 40, "%d ", menu->delnum);

            if (menu->selected)
            {
                    //~ bmp_printf(FONT_MED, 720 - strlen(menu->name)*font_med.width, 50, menu->name);
                //~ else
                if (!junkie_mode)
                    bfnt_puts(menu->name, 5, y, fg, bg);
                int x1 = (x & ~3);
                int x2 = ((x1 + icon_spacing) & ~3);

                draw_line(x1, y+42-4, x1, y+5, fgu);
                draw_line(x2, y+42-4, x2, y+5, fgu);
                draw_line(x1+4, y+1, x2-4, y+1, fgu);
                draw_line(x1-1, y+40, x2+1, y+40, bgs);
                draw_line(x1-2, y+41, x2+2, y+41, bgs);
                draw_line(x1-3, y+42, x2+3, y+42, bgs);

                draw_line(x1-4, y+42, x1, y+42-4, fgu);
                draw_line(x2+4, y+42, x2, y+42-4, fgu);

                draw_line(x1, y+5, x1+4, y+1, fgu);
                draw_line(x2, y+5, x2-4, y+1, fgu);
                
                draw_line(x1, y+2, x1, y+4, bgu);
                draw_line(x1+1, y+2, x1+1, y+3, bgu);
                draw_line(x1+2, y+2, x1+2, y+2, bgu);

                draw_line(x2, y+2, x2, y+4, bgu);
                draw_line(x2-1, y+2, x2-1, y+3, bgu);
                draw_line(x2-2, y+2, x2-2, y+2, bgu);
            }
            x += icon_spacing;
        }

        int skip_this = 0; 
        if (submenu && (quick_redraw || menu_lv_transparent_mode || edit_mode))
            skip_this = 1;
        
        if (junkie_mode && !edit_mode && !menu_lv_transparent_mode)
        {
            menu_display_junkie(
                menu,
                x - icon_spacing,
                y + 55,
                icon_spacing
            );
        }
        else if( menu->selected && !skip_this)
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
        if (!quick_redraw && !menu_lv_transparent_mode && !edit_mode)
            bmp_dim(45, 480-50);
        
        submenu_display(submenu);
    }
    
    give_semaphore( menu_sem );
}


static void
implicit_submenu_display()
{
    struct menu * menu = get_selected_menu();
    menu_display(
        menu,
        MENU_OFFSET,
         55,
         1
    );
}

static void
submenu_display(struct menu * submenu)
{
    if (!submenu) return;

    int count = 0;
    struct menu_entry * child = submenu->children;
    while (child) { if (IS_VISIBLE(child)) count++; child = child->next; }
    int h = submenu->submenu_height ? submenu->submenu_height : 
        (int) MIN(420, count * font_large.height + 40 + 50 - (count > 7 ? 30 : 0));
                       /* body + titlebar + padding - smaller padding for large submenus */
        
    int w = submenu->submenu_width  ? submenu->submenu_width : 600;
    g_submenu_width = w;
    int bx = (720 - w)/2;
    int by = (480 - h)/2 - 30;
    
    // submenu header
    if (!menu_lv_transparent_mode && !edit_mode)
    {
        bmp_fill(MENU_BG_COLOR_HEADER_FOOTER,  bx,  by, 720-2*bx+4, 40);
        bmp_fill(COLOR_BLACK,  bx,  by + 40, 720-2*bx+4, h-40);
        //~ bmp_draw_rect(COLOR_GRAY70,  bx,  by, 720-2*bx, 40);
        draw_line(bx, by+40, bx+w, by+40, 39);
        draw_line(bx, by+41, bx+w, by+41, 39);
        draw_line(bx, by+42, bx+w, by+42, 38);
        draw_line(bx, by+43, bx+w, by+43, 38);
        bmp_draw_rect(COLOR_GRAY60,  bx,  by, 720-2*bx, h);
        bfnt_puts(submenu->name,  bx + 15,  by+2, COLOR_WHITE, 40);
        //~ bfnt_printf(bx + 5,  by, COLOR_WHITE, 40, "%s - %s", get_selected_menu()->name, submenu->name);

        submenu_key_hint(720-bx-45, by+5, MENU_BG_COLOR_HEADER_FOOTER);

        int xl = 720-bx-50;
        int yl = by+15;
        draw_line(xl, yl+2, xl, yl+20, COLOR_CYAN);
        draw_line(xl+1, yl+2, xl+1, yl+20, COLOR_CYAN);
        draw_line(xl, yl+20, xl+40, yl+20, COLOR_CYAN);
        draw_line(xl+1, yl+21, xl+40, yl+21, COLOR_CYAN);
        for (int i = -5; i <= 5; i++)
            draw_line(xl, yl, xl+i, yl+7, COLOR_CYAN);
    }

                                                   /* titlebar + padding difference for large submenus */
    menu_display(submenu,  bx + SUBMENU_OFFSET,  by + 40 + (count > 7 ? 10 : 25), edit_mode ? 1 : 0);
    show_hidden_items(submenu, 1);
}

static void
menu_entry_showhide_toggle(
    struct menu *   menu
)
{
    if( !menu )
        return;
    
    if (streq(menu->name, MY_MENU_NAME)) return; // special menu

    struct menu_entry * entry = get_selected_entry(menu);
    if (!entry) return;

    if (junkie_mode)
    {
        entry->jhidden = !entry->jhidden;
    }
    else
    {
        entry->hidden = !entry->hidden;
        if(entry->hidden){
            menu->childnum--;
        }else{
            menu->childnum++;
        }
    }
    menu_make_sure_selection_is_valid();
    menu_flags_dirty = 1;
    my_menu_update();
}

static void
menu_entry_star_toggle(
    struct menu *   menu
)
{
    if( !menu )
        return;

    struct menu_entry * entry = get_selected_entry(menu);
    if (!entry) return;

    if (streq(menu->name, MY_MENU_NAME))
    {
        // lookup the corresponding entry in normal menus, and toggle that one instead
        char* name = (char*) entry->name;   // trick so we don't find the same menu
        entry->name = 0;                    // (this menu will be rebuilt anyway, so... no big deal)
        entry = entry_find_by_name(0, name);
        if (!entry) { beep(); return; }
    }

    entry->starred = !entry->starred;
    menu_flags_dirty = 1;
    int ok = my_menu_update();
    if (!ok)
    {
        beep();
        NotifyBox(2000, "Too many favorites");
        entry->starred = 0;
        my_menu_update();
    }
}

static void
menu_entry_select(
    struct menu *   menu,
    int mode // 0 = increment, 1 = decrement, 2 = Q, 3 = SET
)
{
    if( !menu )
        return;

    struct menu_entry * entry = get_selected_entry(menu);
    if( !entry ) return;
    
    // don't perform actions on empty items (can happen on empty submenus)
    if (!IS_VISIBLE(entry))
    {
        submenu_mode = edit_mode = 0;
        menu_lv_transparent_mode = 0;
        return;
    }

    if(mode == 1) // decrement
    {
        if (entry->select) entry->select( entry->priv, -1);
        else menu_numeric_toggle(entry->priv, -1, entry->min, entry->max);
    }
    else if (mode == 2) // Q
    {
        if ( entry->select_Q ) entry->select_Q( entry->priv, 1);
        else if (edit_mode) edit_mode = 0;
        else menu_toggle_submenu();
    }
    else if (mode == 3) // SET
    {
        if (edit_mode) edit_mode = 0;
        else if (menu_lv_transparent_mode && entry->icon_type != IT_ACTION) menu_lv_transparent_mode = 0;
        else if (entry->edit_mode == EM_MANY_VALUES) edit_mode = !edit_mode;
        else if (entry->edit_mode == EM_MANY_VALUES_LV && lv) menu_lv_transparent_mode = !menu_lv_transparent_mode;
        else if (entry->edit_mode == EM_MANY_VALUES_LV && !lv) edit_mode = !edit_mode;
        else if (SHOULD_USE_EDIT_MODE(entry)) edit_mode = !edit_mode;
        else if( entry->select ) entry->select( entry->priv, 1);
        else menu_numeric_toggle(entry->priv, 1, entry->min, entry->max);
    }
    else // increment
    {
        if( entry->select ) entry->select( entry->priv, 1);
        else menu_numeric_toggle(entry->priv, 1, entry->min, entry->max);
    }
    
    config_dirty = 1;
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

    // Select the new one (which might be the same)
    menu->selected      = 1;
    menu_first_by_icon = menu->icon;
    give_semaphore( menu_sem );
    

    if (IS_SUBMENU(menu))
        menu_move(menu, direction); // always skip submenus

    else if (!menu_has_visible_items(menu) && are_there_any_visible_menus())
        menu_move(menu, direction); // this menu is hidden, skip it (try again)
            // would fail if no menus are displayed, so we double check before trying
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

    int selectedpos= 0;
    for( ; entry ; entry = entry->next )
    {
        if(IS_VISIBLE(entry)) selectedpos++;
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

    if( direction < 0 )
    {
        // First and moving up?
        if( entry->prev ){
            entry = entry->prev;
            menu->pos = selectedpos - 1;
        }else {
            // Go to the last one
            while( entry->next ) entry = entry->next;
            menu->pos = menu->childnum;
        }
    } else {
        // Last and moving down?
        if( entry->next ){
            entry = entry->next;
            menu->pos = selectedpos + 1;
        }else {
            // Go to the first one
            while( entry->prev ) entry = entry->prev;
            menu->pos = 1;
        }
    }

    // Select the new one, which might be the same as the old one
    entry->selected = 1;
    give_semaphore( menu_sem );
    
    if (!IS_VISIBLE(entry) && menu_has_visible_items(menu))
        menu_entry_move(menu, direction); // try again, skip hidden items
        // warning: would block if the menu is empty

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
    struct menu * menu = get_selected_menu();
    if (submenu_mode)
    {
        struct menu * main_menu = menu;
        menu = get_current_submenu();
        if (!menu) menu = main_menu; // no submenu, operate on same item
    }
 
    // current menu has any valid items in current mode?
    if (!menu_has_visible_items(menu))
    {
        if (submenu_mode) return; // empty submenu
        menu_move(menu, -1); menu = get_selected_menu();
        menu_move(menu, 1); menu = get_selected_menu();
    }

    // currently selected menu entry is visible?
    struct menu_entry * entry = get_selected_entry(menu);
    if (!entry) return;

    if (entry->selected && !IS_VISIBLE(entry))
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
    //~ in EOS M, force dialog change when canon dialog times out. Not sure how else to do this at the moment.
#ifdef CONFIG_EOSM
        if (!CURRENT_DIALOG_MAYBE)
            SetGUIRequestMode(GUIMODE_ML_MENU);
#endif

        menu_damage = 0;
        //~ g_submenu_width = 720;
        
        if (!DISPLAY_IS_ON) return;
        if (sensor_cleaning) return;
        if (gui_state == GUISTATE_MENUDISP) return;
        
        if (junkie_mode) quick_redraw = 0;
        
        if (menu_help_active)
        {
            BMP_LOCK( menu_help_redraw(); )
            menu_damage = 0;
        }
        else
        {
            if (!lv) menu_lv_transparent_mode = 0;
            if (menu_lv_transparent_mode && edit_mode) edit_mode = 0;

            //~ menu_damage = 0;
            BMP_LOCK (
                if (DOUBLE_BUFFERING)
                {
                    // draw to mirror buffer to avoid flicker
                    //~ bmp_idle_copy(0); // no need, drawing is fullscreen anyway
                    bmp_draw_to_idle(1);
                }
                
                int z = zebra_should_run();
                if (menu_zebras_mirror_dirty && !z)
                {
                    clear_zebras_from_mirror();
                    menu_zebras_mirror_dirty = 0;
                }

                static int prev_z = 0;
                if (menu_lv_transparent_mode)
                {
                    if (!quick_redraw)
                        bmp_fill( 0, 0, 0, 720, 480 );
                    if (z)
                    {
                        if (prev_z) copy_zebras_from_mirror();
                        else cropmark_clear_cache(); // will clear BVRAM mirror and reset cropmarks
                        menu_zebras_mirror_dirty = 1;
                    }
                    if (hist_countdown == 0 && !should_draw_zoom_overlay())
                        draw_histogram_and_waveform(); // too slow
                    else
                        hist_countdown--;
                }
                else
                {
                    if (!quick_redraw)
                        bmp_fill(COLOR_BLACK, 0, 0, 720, 480 );
                }
                prev_z = z;

                menu_make_sure_selection_is_valid();
                
                menus_display( menus, 0, 0 ); 

                if (!menu_lv_transparent_mode && !SUBMENU_OR_EDIT && !junkie_mode)
                {
                    if (is_menu_active("Help")) menu_show_version();
                    if (is_menu_active("Focus")) display_lens_hyperfocal();
                }
                
                if (menu_lv_transparent_mode) 
                {
                    draw_ml_topbar(0, 1);
                    draw_ml_bottombar(0, 1);
                }

                if (recording)
                    bmp_make_semitransparent();

                if (beta_should_warn()) draw_beta_warning();

                if (DOUBLE_BUFFERING)
                {
                    // copy image to main buffer
                    bmp_draw_to_idle(0);

                    int screen_layout = get_screen_layout();
                    if (hdmi_code == 2) // copy at a smaller scale to fit the screen
                    {
                        if (screen_layout == SCREENLAYOUT_16_10)
                            bmp_zoom(bmp_vram(), bmp_vram_idle(),  360,  150, /* 128 div */ 143, /* 128 div */ 169);
                        else if (screen_layout == SCREENLAYOUT_16_9)
                            bmp_zoom(bmp_vram(), bmp_vram_idle(),  360,  165, /* 128 div */ 143, /* 128 div */ 185);
                        else
                        {
                            if (menu_upside_down) bmp_flip(bmp_vram(), bmp_vram_idle(), 0);
                            else bmp_idle_copy(1,0);
                        }
                    }
                    else if (EXT_MONITOR_RCA)
                        bmp_zoom(bmp_vram(), bmp_vram_idle(),  360,  200, /* 128 div */ 135, /* 128 div */ 135);
                    else
                    {
                        if (menu_upside_down) bmp_flip(bmp_vram(), bmp_vram_idle(), 0);
                        else bmp_idle_copy(1,0);
                    }
                    //~ bmp_idle_clear();
                }
            )
            //~ update_stuff();
            
            lens_display_set_dirty();
        }
    
    bmp_on();

    // adjust some colors for better contrast
    alter_bitmap_palette_entry(COLOR_GREEN1, COLOR_GREEN1, 160, 256);
    alter_bitmap_palette_entry(COLOR_GREEN2, COLOR_GREEN2, 300, 256);
    //~ alter_bitmap_palette_entry(COLOR_ORANGE, COLOR_ORANGE, 160, 160);
    //~ alter_bitmap_palette_entry(COLOR_PINK,   COLOR_ORANGE, 256, 256);

    #ifdef CONFIG_VXWORKS
    set_ml_palette();
    #endif

}

static int _t = 0;
static int _get_timestamp(struct tm * t)
{
    return t->tm_sec + t->tm_min * 60 + t->tm_hour * 3600 + t->tm_mday * 3600 * 24;
}
static void _tic()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    _t = _get_timestamp(&now);
}
static int _toc()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    return _get_timestamp(&now) - _t;
}


void menu_benchmark()
{
    _tic();
    for (int i = 0; i < 500; i++) menu_redraw_do();
    int t = _toc();
    clrscr();
    NotifyBox(20000, "Elapsed time: %d seconds", t);
}

struct msg_queue * menu_redraw_queue = 0;

static void
menu_redraw_task()
{
    menu_redraw_queue = (struct msg_queue *) msg_queue_create("menu_redraw_mq", 1);
    TASK_LOOP
    {
        //~ msleep(30);
        int msg;
        int err = msg_queue_receive(menu_redraw_queue, (struct event**)&msg, 500);
        if (err) continue;
        if (gui_menu_shown())
        {
            redraw_in_progress = 1;
            quick_redraw = (msg == MENU_REDRAW_QUICK);
            
            if (!menu_redraw_blocked)
            {
                menu_redraw_do();
            }
            msleep(20);
            redraw_in_progress = 0;
        }
        //~ else redraw();
    }
}

TASK_CREATE( "menu_redraw_task", menu_redraw_task, 0, 0x1a, 0x2000 );

void
menu_redraw()
{
    if (!DISPLAY_IS_ON) return;
    if (ml_shutdown_requested) return;
    if (menu_help_active) bmp_draw_request_stop();
    if (menu_redraw_queue) msg_queue_post(menu_redraw_queue, redraw_in_progress ? MENU_REDRAW_QUICK : MENU_REDRAW_FULL);
}

void
menu_redraw_full()
{
    if (!DISPLAY_IS_ON) return;
    if (ml_shutdown_requested) return;
    if (menu_help_active) bmp_draw_request_stop();
    if (menu_redraw_queue) msg_queue_post(menu_redraw_queue, MENU_REDRAW_FULL);
}


static struct menu * get_selected_menu()
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    return menu;
}

static struct menu_entry * get_selected_entry(struct menu * menu)  // argument is optional, just for speedup
{
    if (!menu)
    {
        menu = menus;
        for( ; menu ; menu = menu->next )
            if( menu->selected )
                break;
    }
    struct menu_entry * entry = menu->children;
    for( ; entry ; entry = entry->next )
        if( entry->selected )
            return entry;
    return 0;
}

static struct menu * get_current_submenu()
{
    struct menu_entry * entry = get_selected_entry(0);
    if (!entry) return 0;

    if (entry->children)
        return menu_find_by_name(entry->name, 0);

    // no submenu, fall back to edit mode
    submenu_mode = 0;
    edit_mode = 1;
    return 0;
}

static int keyrepeat = 0;
static int keyrep_countdown = 4;
static int keyrep_ack = 1;
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
                break;
        }
    }
    return 1;
}

void keyrepeat_ack(int button_code)
{
    if (button_code == keyrepeat) keyrep_ack = 1;
}

int
handle_ml_menu_keys(struct event * event) 
{
    if (menu_shown || arrow_keys_shortcuts_active())
        handle_ml_menu_keyrepeat(event);

    if (!menu_shown) return 1;
    if (!DISPLAY_IS_ON)
        if (event->param != BGMT_PRESS_HALFSHUTTER) return 1;
    
    // rack focus may override some menu keys
    if (handle_rack_focus_menu_overrides(event)==0) return 0;
    
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
    
    // Find the selected menu (should be cached?)
    struct menu * menu = get_selected_menu();

    struct menu * main_menu = menu;
    if (submenu_mode)
    {
        main_menu = menu;
        menu = get_current_submenu();
        if (!menu) menu = main_menu; // no submenu, operate on same item
    }
    
    int button_code = event->param;
#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_7D) // Q not working while recording, use INFO instead
    if (button_code == BGMT_INFO && recording) button_code = BGMT_Q;
#endif

    int menu_needs_full_redraw = 0; // if true, do not allow quick redraws
    
    switch( button_code )
    {
    case BGMT_MENU:
    {
        if (SUBMENU_OR_EDIT || menu_lv_transparent_mode || menu_help_active)
        {
            submenu_mode = 0;
            edit_mode = 0;
            menu_lv_transparent_mode = 0;
            menu_help_active = 0;
        }
        else
        {
            // double click will go to "extra junkie" mode (nothing hidden)
            static int last_t = 0;
            int t = get_ms_clock_value();
            if (t > last_t && t < last_t + 300)
                junkie_mode = !junkie_mode*2;
            else
                junkie_mode = !junkie_mode;
            last_t = t;
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
    
    #ifndef CONFIG_500D // LV is Q
    case BGMT_LV:
        if (!lv) return 1;
        // else fallthru
    #endif
    case BGMT_PRESS_ZOOMIN_MAYBE:
        if (lv) menu_lv_transparent_mode = !menu_lv_transparent_mode;
        else edit_mode = !edit_mode;
        menu_damage = 1;
        menu_help_active = 0;
        break;

    case BGMT_PRESS_UP:
    case BGMT_WHEEL_UP:
        if (menu_help_active) { menu_help_prev_page(); break; }

        if (edit_mode && !menu_lv_transparent_mode)
            menu_entry_select( menu, 1 );
        else
        {
            menu_entry_move( menu, -1 );
            if (menu_lv_transparent_mode) menu_needs_full_redraw = 1;
        }

        break;

    case BGMT_PRESS_DOWN:
    case BGMT_WHEEL_DOWN:
        if (menu_help_active) { menu_help_next_page(); break; }
        
        if (edit_mode && !menu_lv_transparent_mode)
            menu_entry_select( menu, 0 );
        else
        {
            menu_entry_move( menu, 1 );
            if (menu_lv_transparent_mode) menu_needs_full_redraw = 1;
        }

        break;

    case BGMT_PRESS_RIGHT:
    case BGMT_WHEEL_RIGHT:
        menu_damage = 1;
        if (menu_help_active) { menu_help_next_page(); break; }
        if (SUBMENU_OR_EDIT || menu_lv_transparent_mode) menu_entry_select( menu, 0 );
        else { menu_move( menu, 1 ); menu_lv_transparent_mode = 0; menu_needs_full_redraw = 1; }
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_PRESS_LEFT:
    case BGMT_WHEEL_LEFT:
        menu_damage = 1;
        if (menu_help_active) { menu_help_prev_page(); break; }
        if (SUBMENU_OR_EDIT || menu_lv_transparent_mode) menu_entry_select( menu, 1 );
        else { menu_move( menu, -1 ); menu_lv_transparent_mode = 0;  menu_needs_full_redraw = 1; }
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_UNPRESS_SET:
        return 0; // block Canon menu redraws

#if defined(CONFIG_5D3) || defined(CONFIG_7D)
    case BGMT_JOY_CENTER:
#endif
    case BGMT_PRESS_SET:
        if (menu_help_active) // pel, don't touch this!
        { 
            menu_help_active = 0;
            break; 
        }
        else if (CUSTOMIZE_MODE_HIDING && !is_customize_selected(menu))
        {
            menu_entry_showhide_toggle(menu);
        }
        else if (CUSTOMIZE_MODE_MYMENU && !is_customize_selected(menu))
        {
            menu_entry_star_toggle(menu);
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
        if (menu_help_active) menu_help_go_to_selected_entry(main_menu);
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_PLAY:
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        menu_entry_select( menu, 1 ); // decrement
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;
#ifdef CONFIG_TOUCHSCREEN
    case TOUCH_1_FINGER:
    case TOUCH_2_FINGER:
    case UNTOUCH_1_FINGER:
    case UNTOUCH_2_FINGER:
        return handle_ml_menu_touch(event);
#endif
#ifdef BGMT_RATE
    case BGMT_RATE:
#endif
#if defined(BGMT_Q)
    case BGMT_Q:
#endif
#ifdef BGMT_Q_ALT
    case BGMT_Q_ALT:
#endif
//~ #ifdef BGMT_JOY_CENTER
    //~ case BGMT_JOY_CENTER:
//~ #endif
#if defined(CONFIG_5D2) || defined(CONFIG_7D)
    case BGMT_PICSTYLE:
#endif
#ifdef CONFIG_50D
    case BGMT_FUNC:
    //~ case BGMT_LV:
#endif
#ifdef CONFIG_5DC
    case BGMT_JUMP:
    case BGMT_PRESS_DIRECT_PRINT:
#endif
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        menu_entry_select( menu, 2 ); // auto setting select
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;

    default:
        /*DebugMsg( DM_MAGIC, 3, "%s: unknown event %08x? %08x %08x %x08",
            __func__,
            event,
            arg2,
            arg3,
            arg4
        );*/
        return 1;
    }

    // If we end up here, something has been changed.
    // Reset the timeout
    
    // if submenu mode was changed, force a full redraw
    static int prev_menu_mode = 0;
    int menu_mode = submenu_mode | edit_mode*2 | menu_lv_transparent_mode*4 | customize_mode*8 | junkie_mode*16;
    if (menu_mode != prev_menu_mode) menu_needs_full_redraw = 1;
    prev_menu_mode = menu_mode;
    
    if (menu_needs_full_redraw) menu_redraw_full();
    else menu_redraw();
    keyrepeat_ack(button_code);
    hist_countdown = 3;
    return 0;
}

#ifdef CONFIG_TOUCHSCREEN
int handle_ml_menu_touch(struct event * event)
{
    int button_code = event->param;
    switch (button_code) {
        case TOUCH_1_FINGER:
            fake_simple_button(BGMT_Q);
            return 0;
        case TOUCH_2_FINGER:
        case UNTOUCH_1_FINGER:
        case UNTOUCH_2_FINGER:
            return 0;
        default:
            return 1;
    }
    return 1;
}
#endif


void
menu_init( void )
{
    menus = NULL;
    menu_sem = create_named_semaphore( "menus", 1 );
    gui_sem = create_named_semaphore( "gui", 0 );
    menu_redraw_sem = create_named_semaphore( "menu_r", 1);

    menu_find_by_name( "Audio",     ICON_ML_AUDIO   )->split_pos = 16;
    menu_find_by_name( "Expo",      ICON_ML_EXPO    )->split_pos = 14;
    menu_find_by_name( "Overlay",   ICON_ML_OVERLAY );
    menu_find_by_name( "Movie",     ICON_ML_MOVIE   )->split_pos = 16;
    menu_find_by_name( "Shoot",     ICON_ML_SHOOT   );
    menu_find_by_name( "Focus",     ICON_ML_FOCUS   )->split_pos = 17;
    menu_find_by_name( "Display",   ICON_ML_DISPLAY )->split_pos = 17;
    menu_find_by_name( "Prefs",     ICON_ML_PREFS   );
    menu_find_by_name( "Scripts",   ICON_ML_SCRIPT  )->split_pos = 10;
    menu_find_by_name( "Debug",     ICON_ML_DEBUG   )->split_pos = 15;
    menu_find_by_name( "Help",      ICON_ML_INFO    )->split_pos = 13;
    menu_find_by_name( MY_MENU_NAME,ICON_ML_MYMENU  );

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


int FAST
gui_menu_shown( void )
{
    return menu_shown;
}

void
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
    for (int i = 0; i < 10; i++)
    {
        if (redraw_flood_stop) break;
        if (!menu_shown) break;
        canon_gui_enable_front_buffer(0);
        menu_redraw_full();
        msleep(20);
    }
}

void piggyback_canon_menu()
{
#ifdef GUIMODE_ML_MENU
    #ifdef CONFIG_500D
    if (is_movie_mode()) return; // doesn'tworkstation
    #endif
    if (recording) return;
    if (sensor_cleaning) return;
    if (gui_state == GUISTATE_MENUDISP) return;
    NotifyBoxHide();
    int new_gui_mode = GUIMODE_ML_MENU;
    if (new_gui_mode) { redraw_flood_stop = 0; task_create("menu_redraw_flood", 0x1c, 0, menu_redraw_flood, 0); }
    if (new_gui_mode != CURRENT_DIALOG_MAYBE) 
    { 
        if (lv) bmp_off(); // mask out the underlying Canon menu :)
        SetGUIRequestMode(new_gui_mode); msleep(200); 
        // bmp will be enabled after first redraw
    }
#endif
}

void close_canon_menu()
{
#ifdef GUIMODE_ML_MENU
    #ifdef CONFIG_500D
    if (is_movie_mode()) return; // doesn'tworkstation
    #endif
    if (recording) return;
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
    if (menu_shown) return;

/* not ergonomic
    // always start in my menu, if configured
    struct menu * my_menu = menu_find_by_name(MY_MENU_NAME, 0);
    if (menu_has_visible_items(my_menu))
        select_menu_by_icon(ICON_ML_MYMENU);
*/

#ifdef CONFIG_5DC
    //~ forces the 5dc screen to turn on for ML menu.
    if (!DISPLAY_IS_ON) fake_simple_button(BGMT_MENU);
    msleep(50);
#endif
    
    menu_lv_transparent_mode = 0;
    submenu_mode = 0;
    edit_mode = 0;
    customize_mode = 0;
    menu_help_active = 0;
    keyrepeat = 0;
    menu_shown = 1;
    //~ menu_hidden_should_display_help = 0;
    if (lv) menu_zebras_mirror_dirty = 1;

    piggyback_canon_menu();
    canon_gui_disable_front_buffer(0);
    if (lv && EXT_MONITOR_CONNECTED) clrscr();
    menu_redraw_full();
}
static void menu_close() 
{ 
    if (!menu_shown) return;
    menu_shown = false;

    customize_mode = 0;
    update_disp_mode_bits_from_params();

    lens_focus_stop();
    menu_lv_transparent_mode = 0;
    
    close_canon_menu();
    canon_gui_enable_front_buffer(0);
    redraw();
    if (lv) bmp_on();
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
    while (!ml_started) msleep(100);
    config_menu_init();
    
    int initial_mode = 0; // shooting mode when menu was opened (if changed, menu should close)
    
    config_menu_load_flags();
    select_menu_by_icon(menu_first_by_icon);
    menu_make_sure_selection_is_valid();
    
    TASK_LOOP
    {
        int menu_or_shortcut_menu_shown = (menu_shown || arrow_keys_shortcuts_active());
        int dt = (menu_or_shortcut_menu_shown && keyrepeat) ? COERCE(100 + keyrep_countdown*5, 20, 100) : should_draw_zoom_overlay() && menu_lv_transparent_mode ? 2000 : 500;
        int rc = take_semaphore( gui_sem, dt );
        if( rc != 0 )
        {
            if (keyrepeat && menu_or_shortcut_menu_shown)
            {
                keyrep_countdown--;
                if (keyrep_countdown <= 0 && keyrep_ack) { keyrep_ack = 0; fake_simple_button(keyrepeat); }
                continue;
            }

            // We woke up after 1 second
            if( !menu_shown )
            {
                extern int config_autosave;
                if (config_autosave && (config_dirty || menu_flags_dirty) && !recording && !ml_shutdown_requested)
                {
                    save_config(0);
                    config_dirty = 0;
                    menu_flags_dirty = 0;
                }
                
                continue;
            }

            if ((!menu_help_active && !menu_lv_transparent_mode) || menu_damage) {
                menu_redraw();
            }

            if (sensor_cleaning && menu_shown)
                menu_close();

            if (initial_mode != shooting_mode && menu_shown)
                menu_close();

            if (gui_state == GUISTATE_MENUDISP && menu_shown)
                menu_close();

            if (!DISPLAY_IS_ON && menu_shown)
                menu_close();
            
            continue;
        }

        if( menu_shown )
        {
            menu_close();
            continue;
        }
        
        if (recording && !lv) continue;
        
        // Set this flag a bit earlier in order to pause LiveView tasks.
        // Otherwise, high priority tasks such as focus peaking might delay the menu a bit.
        //~ menu_shown = true; 
        
        // ML menu needs to piggyback on Canon menu, in order to receive wheel events
        //~ piggyback_canon_menu();

        //~ fake_simple_button(BGMT_PICSTYLE);
        menu_open();
        initial_mode = shooting_mode;
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
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    if (streq(menu->name, menu_name))
    {
        struct menu_entry * entry = get_selected_entry(menu);
        if (!entry) return 0;
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
    return is_menu_selected(name);
}

void select_menu(char* name, int entry_index)
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        menu->selected = streq(menu->name, name);
        if (menu->selected)
        {
            struct menu_entry * entry = menu->children;
            
            int i;
            for(i = 0 ; entry ; entry = entry->next, i++ )
                entry->selected = (i == entry_index);
        }
    }
    //~ menu_damage = 1;
}

void select_menu_by_name(char* name, char* entry_name)
{
    struct menu * menu_that_was_selected = 0;
    int entry_was_selected = 0;
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        menu->selected = streq(menu->name, name) && !menu_that_was_selected;
        if (menu->selected) menu_that_was_selected = menu;
        if (menu->selected)
        {
            struct menu_entry * entry = menu->children;
            
            int i;
            for(i = 0 ; entry ; entry = entry->next, i++ )
            {
                entry->selected = streq(entry->name, entry_name) && !entry_was_selected;
                if (entry->selected) entry_was_selected = 1;
            }
        }
    }
    
    if (!menu_that_was_selected) { menus->selected = 1; menu_that_was_selected = menus; }// name not found, just select the first one one
    if (!entry_was_selected) menu_that_was_selected->children->selected = 1;
    //~ menu_damage = 1;
}

static struct menu_entry * entry_find_by_name(const char* name, const char* entry_name)
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        if (!name || streq(menu->name, name))
        {
            struct menu_entry * entry = menu->children;
            
            int i;
            for(i = 0 ; entry ; entry = entry->next, i++ )
            {
                if (streq(entry->name, entry_name))
                {
                    return entry;
                }
            }
        }
    }
    return 0;
}

static void hide_menu_by_name(char* name, char* entry_name)
{
    struct menu * menu = menu_find_by_name(name, 0);
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (menu && entry)
    {
        entry->hidden = 1;
        menu->childnum--;
    }
}

static void jhide_menu_by_name(char* name, char* entry_name)
{
    struct menu * menu = menu_find_by_name(name, 0);
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (menu && entry)
    {
        entry->jhidden = 1;
    }
}
static void star_menu_by_name(char* name, char* entry_name)
{
    struct menu_entry * entry = entry_find_by_name(name, entry_name);
    if (entry)
    {
        entry->starred = 1;
    }
}

static void select_menu_by_icon(int icon)
{
    take_semaphore(menu_sem, 0);
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        if (menu->icon == icon) // found!
        {
            struct menu * menu = menus;
            for( ; menu ; menu = menu->next )
                menu->selected = menu->icon == icon;
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

    struct menu_entry * entry = get_selected_entry(menu);
    if (!entry) return;
    menu_help_go_to_label(entry->name);
    give_semaphore(menu_sem);
}

static void menu_show_version(void)
{
    big_bmp_printf(FONT(FONT_MED, COLOR_GRAY60, MENU_BG_COLOR_HEADER_FOOTER),  10,  420,
        "Magic Lantern version : %s\n"
        "Mercurial changeset   : %s\n"
        "Built on %s by %s.",
        build_version,
        build_id,
        build_date,
        build_user);
}


// this should work on most cameras
int handle_ml_menu_erase(struct event * event)
{
    if (dofpreview) return 1; // don't open menu when DOF preview is locked
    
#ifdef CONFIG_EOSM
    if (recording)
        return 1;
#endif
    
    if (event->param == BGMT_TRASH)
    {
        if (gui_menu_shown() || gui_state == GUISTATE_IDLE)
        {
            give_semaphore( gui_sem );
            return 0;
        }
        //~ else bmp_printf(FONT_LARGE, 100, 100, "%d ", gui_state);
    }

    return 1;
}

// this can be called from any task
void menu_stop()
{
    if (gui_menu_shown())
        give_semaphore( gui_sem );
}

void menu_open_submenu(struct menu_entry * entry)
{
    submenu_mode = 1;
    edit_mode = 0;
    menu_lv_transparent_mode = 0;
}

void menu_close_submenu()
{
    submenu_mode = 0;
    edit_mode = 0;
    menu_lv_transparent_mode = 0;
}

void menu_toggle_submenu()
{
    submenu_mode = !submenu_mode;
    edit_mode = 0;
    menu_lv_transparent_mode = 0;
}

int handle_quick_access_menu_items(struct event * event)
{
#ifdef BGMT_Q
    // quick access to some menu items
    #ifdef BGMT_Q_ALT
    if (event->param == BGMT_Q_ALT && !gui_menu_shown())
    #else
    if (event->param == BGMT_Q && !gui_menu_shown())
    #endif
    {
        if (ISO_ADJUSTMENT_ACTIVE)
        {
            select_menu("Expo", 0);
            give_semaphore( gui_sem ); 
            return 0;
        }
#ifdef CURRENT_DIALOG_MAYBE_2
        else if (CURRENT_DIALOG_MAYBE_2 == DLG2_FOCUS_MODE)
#else
        else if (CURRENT_DIALOG_MAYBE == DLG_FOCUS_MODE)
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

static int menu_get_flags(struct menu_entry * entry)
{
    return entry->starred + entry->hidden*2 + entry->jhidden*4;
}

static void menu_set_flags(char* menu_name, char* entry_name, int flags)
{
    if (flags & 1)
        star_menu_by_name(menu_name, entry_name);
    if (flags & 2)
        hide_menu_by_name(menu_name, entry_name);
    if (flags & 4)
        jhide_menu_by_name(menu_name, entry_name);
}

static void menu_save_flags(char* filename)
{
    #define MAX_SIZE 10240
    char* msg = alloc_dma_memory(MAX_SIZE);
    msg[0] = '\0';

    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        if (streq(menu->name, MY_MENU_NAME)) continue;
        
        struct menu_entry * entry = menu->children;
        
        int i;
        for(i = 0 ; entry ; entry = entry->next, i++ )
        {
            if (!entry->name) continue;
            if (!entry->name[0]) continue;

            int flags = menu_get_flags(entry);
            if (flags)
            {
                snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1, "%d %s\\%s\n", flags, menu->name, entry->name);
            }
        }
    }
    
    FILE * file = FIO_CreateFileEx(filename);
    if( file == INVALID_PTR )
        return;
    
    FIO_WriteFile(file, msg, strlen(msg));

    FIO_CloseFile( file );

}

static void menu_load_flags(char* filename)
{
    int size = 0;
    char* buf = (char*)read_entire_file(filename , &size);
    if (!size) return;
    if (!buf) return;
    int prev = -1;
    int sep = 0;
    for (int i = 0; i < size; i++)
    {
        if (buf[i] == '\\') sep = i;
        else if (buf[i] == '\n')
        {
            //~ NotifyBox(2000, "%d %d %d ", prev, sep, i);
            if (prev < sep-2 && sep < i-2)
            {
                buf[i] = 0;
                buf[sep] = 0;
                char* menu_name = &buf[prev+3];
                char* entry_name = &buf[sep+1];
                int flags = buf[prev+1] - '0';
                //~ NotifyBox(2000, "%s -> %s", menu_name, entry_name); msleep(2000);
                
                menu_set_flags(menu_name, entry_name, flags);
            }
            prev = i;
        }
    }
    free_dma_memory(buf);
}


static void config_menu_load_flags()
{
    menu_load_flags(CARD_DRIVE "ML/SETTINGS/MENU.CFG");
    my_menu_update();
}

void config_menu_save_flags()
{
    if (!menu_flags_dirty) return;
    menu_save_flags(CARD_DRIVE "ML/SETTINGS/MENU.CFG");
}


/*void menu_save_all_items_dbg()
{
    #define MAX_SIZE 10240
    char* msg = alloc_dma_memory(MAX_SIZE);
    msg[0] = '\0';

    int unnamed = 0;
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        struct menu_entry * entry = menu->children;
        
        int i;
        for(i = 0 ; entry ; entry = entry->next, i++ )
        {
            snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1, "%s\\%s\n", menu->name, entry->name);
            if (strlen(entry->name) == 0 || strlen(menu->name) == 0) unnamed++;
        }
    }
    
    FILE * file = FIO_CreateFileEx( CARD_DRIVE "ML/LOGS/MENUS.LOG" );
    if( file == INVALID_PTR )
        return;
    
    FIO_WriteFile(file, msg, strlen(msg));

    FIO_CloseFile( file );
    
    NotifyBox(5000, "Menu items: %d unnamed.", unnamed);
}*/
