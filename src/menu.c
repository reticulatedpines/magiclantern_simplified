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


#define DOUBLE_BUFFERING 1

#define MENU_KEYHELP_Y_POS (menu_lv_transparent_mode ? 425 : 430)
#define MENU_HELP_Y_POS 453
#define MENU_WARNING_Y_POS (menu_lv_transparent_mode ? 425 : 453)

//for vscroll
#define MENU_LEN_DEFAULT 11
#define MENU_LEN_AUDIO 10 // at len=11, audio meters would overwrite menu entries on 600D
//~ #define MENU_LEN_FOCUS 8

int get_menu_len(struct menu * menu)
{
    if (menu->icon == ICON_MIC) // that's the Audio menu
        return MENU_LEN_AUDIO;
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
static int menu_hidden_dirty = 0;
//~ static int menu_hidden_should_display_help = 0;
static int menu_zebras_mirror_dirty = 0; // to clear zebras from mirror (avoids display artifacts if, for example, you enable false colors in menu, then you disable them, and preview LV)
static char* warning_msg = 0;
int menu_help_active = 0;
int submenu_mode = 0;
int g_submenu_width = 0;
static int menu_id_increment = 1;
static int redraw_flood_stop = 0;

static int quick_redraw = 0; // don't redraw the full menu, because user is navigating quickly
static int redraw_in_progress = 0;
#define MENU_REDRAW_FULL 1
#define MENU_REDRAW_QUICK 2

static int hist_countdown = 3; // histogram is slow, so draw it less often

void menu_close_post_delete_dialog_box();
void menu_close_gmt();

int is_submenu_mode_active() { return gui_menu_shown() && submenu_mode; }

//~ static CONFIG_INT("menu.transparent", semitransparent, 0);

static CONFIG_INT("menu.first", menu_first_by_icon, ICON_i);
int advanced_hidden_edit_mode = 0;

void menu_set_dirty() { menu_damage = 1; }

int is_menu_help_active() { return gui_menu_shown() && menu_help_active; }

//~ int get_menu_font_sel() 
//~ {
    //~ if (recording) return FONT(FONT_LARGE,COLOR_WHITE,12); // dark red
    //~ else return FONT(FONT_LARGE,COLOR_WHITE,13);
//~ }

void select_menu_by_name(char* name, char* entry_name);
static void select_menu_by_icon(int icon);
static void menu_help_go_to_selected_entry(struct menu * menu);
//~ static void menu_init( void );
static void menu_show_version(void);
static struct menu * get_current_submenu();
static struct menu * get_selected_menu();
static void menu_make_sure_selection_is_valid();
static void menu_save_hidden_items();
static void menu_load_hidden_items();

extern int gui_state;
void menu_enable_lv_transparent_mode()
{
    menu_lv_transparent_mode = 1;
    menu_damage = 1;
}
void menu_disable_lv_transparent_mode()
{
    menu_lv_transparent_mode = 0;
}
int menu_active_but_hidden() { return gui_menu_shown() && ( menu_lv_transparent_mode ); }
int menu_active_and_not_hidden() { return gui_menu_shown() && !( menu_lv_transparent_mode && hist_countdown < 2 ); }

int draw_event = 0;

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
static unsigned get_beta_timestamp()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    return now.tm_mday;
}
int beta_should_warn()
{
    unsigned t = get_beta_timestamp();
    return beta_warn != t;
}

void beta_set_warned()
{
    unsigned t = get_beta_timestamp();
    beta_warn = t;
}
#endif

//~ struct dialog * menu_dialog = 0;
static struct menu * menus;

struct menu * menu_get_root() {
  return menus;
}

void
menu_binary_toggle(
    void *          priv,
    int unused
)
{
    unsigned * val = priv;
    *val = !*val;
}

void menu_ternary_toggle(void* priv, int delta)
{
    unsigned * val = priv;
    *val = mod(*val + delta, 3);
}

void menu_quaternary_toggle(void* priv, int delta)
{
    unsigned * val = priv;
    *val = mod(*val + delta, 4);
}

void menu_quinternary_toggle(void* priv, int delta)
{
    unsigned * val = priv;
    *val = mod(*val + delta, 5);
}

void menu_numeric_toggle(int* val, int delta, int min, int max)
{
    *val = mod(*val - min + delta, max - min + 1) + min;
}

void
menu_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "%s",
        (const char*) priv
    );
    menu_draw_icon(x, y, MNI_ACTION, 0);
}


static void entry_draw_icon(
    struct menu_entry * entry,
    int         x,
    int         y
)
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
            const char* first_choice = entry->choices[0];
            if (streq(first_choice, "OFF") || streq(first_choice, "Hide"))
                entry->icon_type = IT_BOOL;
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
    
    switch (entry->icon_type)
    {
        case IT_BOOL:
            menu_draw_icon(x, y, MNI_BOOL(MEM(entry->priv)), 0);
            break;

        case IT_BOOL_NEG:
            menu_draw_icon(x, y, MNI_BOOL(!MEM(entry->priv)), 0);
            break;

        case IT_ACTION:
            menu_draw_icon(x, y, MNI_ACTION, 0);
            break;

        case IT_ALWAYS_ON:
            menu_draw_icon(x, y, MNI_ON, 0);
            break;
            
        case IT_SIZE:
            menu_draw_icon(x, y, MNI_SIZE, MEM(entry->priv) | ((entry->max+1) << 16));
            break;

        case IT_DICE:
            menu_draw_icon(x, y, MNI_DICE, MEM(entry->priv) | ((entry->max+1) << 16));
            break;
        
        case IT_PERCENT:
            menu_draw_icon(x, y, MNI_PERCENT, (MEM(entry->priv) - entry->min) * 100 / (entry->max - entry->min));
            break;

        case IT_NAMED_COLOR:
            menu_draw_icon(x, y, MNI_NAMED_COLOR, (intptr_t) entry->choices[MEM(entry->priv)]);
            break;
        
        case IT_DISABLE_SOME_FEATURE:
            menu_draw_icon(x, y, MEM(entry->priv) ? MNI_DISABLE : MNI_NEUTRAL, 0);
            break;

        case IT_DISABLE_SOME_FEATURE_NEG:
            menu_draw_icon(x, y, MEM(entry->priv) ? MNI_NEUTRAL : MNI_DISABLE, 0);
            break;

        case IT_REPLACE_SOME_FEATURE:
            menu_draw_icon(x, y, MEM(entry->priv) ? MNI_ON : MNI_NEUTRAL, 0);
            break;
        
        case IT_SUBMENU:
        {
            int value = 0;
            if (entry->priv) value = MEM(entry->priv); // if priv field is present, use it as boolean value
            else 
            {   // otherwise, look in the children submenus; if one is true, then submenu icon is drawn as "true"
                struct menu_entry * e = entry->children;
                for( ; e ; e = e->next )
                {
                    if( e->priv && MEM(e->priv))
                    {
                        value = 1;
                        break;
                    }
                }

            }
            menu_draw_icon(x, y, MNI_SUBMENU, value);
            break;
        }
    }
}

void
submenu_print(
    struct menu_entry * entry,
    int         x,
    int         y
)
{
    static char msg[200];
    msg[0] = '\0';
    STR_APPEND(msg, "%s", entry->name);
    if (entry->priv && entry->select != (void(*)(void*,int))run_in_separate_task)
    {
        int l = strlen(entry->name);
        for (int i = 0; i < 14 - l; i++)
            STR_APPEND(msg, " ");
        if (entry->choices && MEM(entry->priv) <= entry->max)
        {
            STR_APPEND(msg, ": %s", entry->choices[MEM(entry->priv)]);
        }
        else if (entry->min == 0 && entry->max == 1)
        {
            STR_APPEND(msg, ": %s", MEM(entry->priv) ? "ON" : "OFF");
        }
        else
        {
            switch (entry->unit)
            {
                case UNIT_1_8_EV:
                case UNIT_x10:
                case UNIT_PERCENT_x10:
                {
                    int v = MEM(entry->priv);
                    int den = entry->unit == UNIT_1_8_EV ? 8 : 10;
                    STR_APPEND(msg, ": %s%d", v < 0 ? "-" : "", ABS(v)/den);
                    int r = (ABS(v)%den)*10/den;
                    if (r) STR_APPEND(msg, ".%d", r);
                    STR_APPEND(msg, "%s",
                        entry->unit == UNIT_1_8_EV ? " EV" :
                        entry->unit == UNIT_PERCENT_x10 ? "%%" : ""
                    );
                    break;
                }
                case UNIT_PERCENT:
                {
                    STR_APPEND(msg, ": %d%%", MEM(entry->priv));
                    break;
                }
                case UNIT_ISO:
                {
                    if (!MEM(entry->priv)) { STR_APPEND(msg, ": Auto"); }
                    else { STR_APPEND(msg, ": %d", raw2iso(MEM(entry->priv))); }
                    break;
                }
                case UNIT_HEX:
                {
                    STR_APPEND(msg, ": 0x%x", MEM(entry->priv));
                    break;
                }
                default:
                {
                    STR_APPEND(msg, ": %d", MEM(entry->priv));
                    break;
                }
            }
        }
    }
    bmp_printf(
        entry->selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        msg
    );

    entry_draw_icon(entry, x, y);
}

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
            give_semaphore( menu_sem );
            return menu;
        }

        // Stop just before we get to the end
        if( !menu->next )
            break;
    }

    // Not found; create it
    struct menu * new_menu = AllocateMemory( sizeof(*new_menu) );
    if( !new_menu )
    {
        give_semaphore( menu_sem );
        return NULL;
    }

    new_menu->id        = menu_id_increment++;
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
menu_has_visible_items(struct menu_entry *  menu)
{
    while( menu )
    {
        if (advanced_hidden_edit_mode || IS_VISIBLE(menu))
        {
            return 1;
        }
        menu = menu->next;
    }
    return 0;
}

static int
are_there_any_visible_menus()
{
    struct menu * menu = menus;
    while( menu )
    {
        if (advanced_hidden_edit_mode || (!IS_SUBMENU(menu) && menu_has_visible_items(menu->children)))
        {
            return 1;
        }
        menu = menu->next;
    }
    return 0;
}


void
menu_add(
    const char *        name,
    struct menu_entry * new_entry,
    int         count
)
{
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
        if (new_entry->id == 0) new_entry->id = menu_id_increment++;
        new_entry->next     = NULL;
        new_entry->prev     = NULL;
        new_entry->selected = 1;
         menu->pos           = 1;
         menu->childnum      = 1; 
         menu->childnummax   = 1;
        //~ if (IS_SUBMENU(menu)) new_entry->essential = FOR_SUBMENU;
        new_entry++;
        count--;
    }

    // Find the end of the entries on the menu already
    while( head->next )
        head = head->next;

    for (int i = 0; i < count; i++)
    {
        if (new_entry->id == 0) new_entry->id = menu_id_increment++;

        if(IS_VISIBLE(new_entry)) menu->childnum++;
        menu->childnummax++;

        new_entry->selected = 0;
        //~ if (IS_SUBMENU(menu)) new_entry->essential = FOR_SUBMENU;
        new_entry->next     = head->next;
        new_entry->prev     = head;
        head->next      = new_entry;
        head            = new_entry;
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
            while (!MENU_IS_EOL(child)) { count++; child++; }
            struct menu * submenu = menu_find_by_name( entry->name, ICON_ML_SUBMENU);
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

void dot(int x, int y, int color, int radius)
{
    int r;
    for (r = 0; r < radius; r++)
    {
        draw_circle(x + 16, y + 16, r, color);
        draw_circle(x + 17, y + 16, r, color);
    }
}


void maru(int x, int y, int color)
{
    dot(x, y, color, 10);
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

static int playicon_square(int x, int y, int color)
{
    bmp_draw_rect(color,x+1,y+4,38,32);
    bmp_draw_rect(color,x+2,y+5,36,30);
    int i;
    for (i = 12; i < 40-12; i++)
    {
        draw_line(x + 10, y + i, x + 30, y + 20, color);
        draw_line(x + 10, y + i, x + 30, y + 20, color);
    }
    return 40;
}

void submenu_icon(int x, int y)
{
    //~ int color = COLOR_WHITE;
    x -= 40;
    bmp_draw_rect(45, x+2, y+5, 32-3, 32-10+1);
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

void submenu_only_icon(int x, int y, int value)
{
    //~ bmp_draw_rect(50, x+2, y+5, 32-3, 32-10);
    int color = value ? COLOR_GREEN1 : COLOR_GRAY45;
    for (int r = 0; r < 3; r++)
    {
        draw_circle(x + 8, y + 10, r, color);
        draw_circle(x + 8, y + 16, r, color);
        draw_circle(x + 8, y + 22, r, color);
        draw_circle(x + 9, y + 10, r, color);
        draw_circle(x + 9, y + 16, r, color);
        draw_circle(x + 9, y + 22, r, color);
    }
    
    color = value ? COLOR_WHITE : COLOR_GRAY45;
    bmp_draw_rect(color, x + 15, y + 10, 10, 1);
    bmp_draw_rect(color, x + 15, y + 16, 10, 1);
    bmp_draw_rect(color, x + 15, y + 22, 10, 1);
}

void selection_bar(int x0, int y0)
{
    if (menu_lv_transparent_mode) return; // only one menu, no need to highlight, and this routine conflicts with RGB zebras

    int w = submenu_mode == 1 ? x0 + g_submenu_width - 50 : 720;
    
    extern int bmp_color_scheme;
    
    uint8_t* B = bmp_vram();
    int c = advanced_hidden_edit_mode ? COLOR_DARK_RED : submenu_mode || bmp_color_scheme ? COLOR_LIGHTBLUE : COLOR_BLUE;
    int black = COLOR_BLACK;
    #ifdef CONFIG_VXWORKS
    c = D2V(c);
    black = D2V(black);
    #endif
    for (int y = y0; y < y0 + 31; y++)
    {
        for (int x = x0-5; x < w; x++)
        {
            if (B[BM(x,y)] == black)
                B[BM(x,y)] = c;
        }
    }
}

void dim_hidden_menu(int x0, int y0, int selected)
{
    int w = submenu_mode == 1 ? x0 + g_submenu_width - 50 : 720;
    
    uint8_t* B = bmp_vram();
    int new_color = selected ? COLOR_ALMOST_BLACK : COLOR_GRAY50;
    int black = COLOR_BLACK;
    #ifdef CONFIG_VXWORKS
    new_color = D2V(selected ? COLOR_BG : COLOR_GRAY50);
    black = D2V(black);
    #endif
    for (int y = y0; y < y0 + 31; y++)
    {
        for (int x = x0-5; x < w; x++)
        {
            if (B[BM(x,y)] != black)
                B[BM(x,y)] = new_color;
        }
    }
}

void size_icon(int x, int y, int current, int nmax)
{
    dot(x, y, COLOR_GREEN1, COERCE(current * (nmax > 2 ? 9 : 7) / (nmax-1) + 3, 1, 12));
}

void dice_icon(int x, int y, int current, int nmax)
{
    #define C(i) (current == (i) ? COLOR_GREEN1 : COLOR_GRAY50), (current == (i) ? 6 : 4)
    //~ x -= 40;
    //~ x += 16; y += 16;
    switch (nmax)
    {
        case 2:
            dot(x - 6, y + 6, C(0)+2);
            dot(x + 6, y - 6, C(1)+2);
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
            dot(x + 10, y + 10, C(10));
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

// By default, icon type is MNI_BOOL(*(int*)priv)
// To override, call menu_draw_icon from the display functions

// Icon is only drawn once for each menu item, even if this is called multiple times
// Only the first call is executed

int icon_drawn = 0;
void menu_draw_icon(int x, int y, int type, intptr_t arg)
{
    #if !CONFIG_DEBUGMSG
    if (icon_drawn) return;
    icon_drawn = type;
    x -= 40;
    if (type != MNI_NONE) bmp_printf(FONT_LARGE, x, y, "  "); // cleanup background; don't call this for LCD remote icons
    warning_msg = 0;
    switch(type)
    {
        case MNI_OFF: maru(x, y, COLOR_GRAY40); return;
        case MNI_ON: maru(x, y, COLOR_GREEN1); return;
        case MNI_DISABLE: batsu(x, y, COLOR_RED); return;
        case MNI_NEUTRAL: maru(x, y, COLOR_GRAY60); return;
        case MNI_WARNING: maru(x, y, COLOR_RED); warning_msg = (char *) arg; return;
        case MNI_AUTO: maru(x, y, COLOR_LIGHTBLUE); return;
        case MNI_PERCENT: percent(x, y, arg); return;
        case MNI_ACTION: playicon(x, y); return;
        case MNI_DICE: dice_icon(x, y, arg & 0xFFFF, arg >> 16); return;
        case MNI_SIZE: size_icon(x, y, arg & 0xFFFF, arg >> 16); return;
        case MNI_NAMED_COLOR: color_icon(x, y, (char *)arg); return;
        case MNI_SUBMENU: submenu_only_icon(x, y, arg); return;
    }
    #endif
}


static void
menu_display(
    struct menu * parentmenu,
    int         x,
    int         y, 
    int only_selected
)
{
    struct menu_entry *menu = parentmenu->children;
    //hide upper menu for vscroll
    int menu_len = get_menu_len(parentmenu); 

    int delnum = parentmenu->delnum; // how many menu entries to skip
    delnum = MAX(delnum, parentmenu->pos - menu_len);
    delnum = MIN(delnum, parentmenu->pos - 1);
    parentmenu->delnum = delnum;
    
    for(int i=0;i<delnum;i++){
        if(advanced_hidden_edit_mode){
            menu = menu->next;
        }else{                
            while(!IS_VISIBLE(menu)) menu = menu->next;
            menu = menu->next;
        }
    }
    //<== vscroll

    int menu_entry_num = 0;
    while( menu )
    {

        if (advanced_hidden_edit_mode || IS_VISIBLE(menu))
        {
            // display help (should be first; if there are too many items in menu, the main text should overwrite the help, not viceversa)
            if (menu->selected && menu->help)
            {
                bmp_printf(
                    FONT(FONT_MED, 0xC, COLOR_BLACK), // red
                     10,  MENU_HELP_Y_POS, 
                        "                                                           "
                );
                bmp_printf(
                    FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 
                     10,  MENU_HELP_Y_POS, 
                    menu->help
                );
            }

            // display icon (only the first icon is drawn)
            icon_drawn = 0;
            if ((!menu_lv_transparent_mode && !only_selected) || menu->selected)
            {
                if (quick_redraw && menu->selected) // selected menu was not erased, so there may be leftovers on the screen
                    bmp_fill(menu_lv_transparent_mode ? 0 : COLOR_BLACK, x, y, g_submenu_width-50, font_large.height);
                
                if (menu->display)
                    menu->display(
                        menu->priv,
                        x,
                        y,
                        menu->selected
                    );
                else
                    submenu_print(menu, x, y);
                
                if (menu->hidden && menu->hidden != MENU_ENTRY_NEVER_HIDE)
                    dim_hidden_menu(x, y, menu->selected);
            }
            
            // this should be after menu->display, in order to allow it to override the icon
            if (menu->selected || (!menu_lv_transparent_mode && !only_selected))
            {
                entry_draw_icon(menu, x, y);
            }

            // display key help
            if (menu->selected && !is_menu_active("Help") && (menu->priv || menu->select) && y + font_large.height <  430)
            {
                char msg[100] = "";

                // this should follow exactly the same logic as in menu_entry_select
                // todo: remove duplicate code
                
                
                // exception for action and submenu items
                if (icon_drawn == MNI_ACTION && !submenu_mode)
                {
                    STR_APPEND(msg, "SET: run action         ");
                }
                else if (menu->select == menu_open_submenu)
                {
                    STR_APPEND(msg, "SET: open submenu       ");
                }
                // exception end
                
                else if (submenu_mode == 2)
                {
                    STR_APPEND(msg, "SET: toggle edit mode   ");
                }
                else if (menu_lv_transparent_mode)
                {
                    STR_APPEND(msg, "SET: toggle LiveView    ");
                }
                else if (menu->edit_mode == EM_FEW_VALUES) // SET increments
                {
                    STR_APPEND(msg, "SET: change value       ");
                }
                else if (menu->edit_mode == EM_MANY_VALUES)
                {
                    STR_APPEND(msg, "SET: toggle edit mode   ");
                }
                else if (menu->edit_mode == EM_MANY_VALUES_LV)
                {
                    if (lv)
                    {
                        STR_APPEND(msg, "SET: toggle LiveView    ");
                    }
                    else if (submenu_mode != 1)
                    {
                        STR_APPEND(msg, "SET: toggle edit mode   ");
                    }
                    else // increment
                    {
                        STR_APPEND(msg, "SET: change value       ");
                    }
                }


                if (submenu_mode || menu_lv_transparent_mode || only_selected)
                {
                    STR_APPEND(msg, "      ");
                    #ifdef CONFIG_EOSM
                    if (!CURRENT_DIALOG_MAYBE)
                    #else
                    if (CURRENT_DIALOG_MAYBE) // GUIMode nonzero => wheel events working
                    #endif
                    {
                        #ifdef CONFIG_EOSM
                        STR_APPEND(msg, "Left/Right: ");
                        #else
                        STR_APPEND(msg, "L/R/Wheel : ");
                        #endif
                    }
                    else
                    {
                        #ifdef CONFIG_5DC
                        STR_APPEND(msg, "Main Dial: ");
                        #else
                        STR_APPEND(msg, "Left/Right: ");
                        #endif
                    }
                    if (icon_drawn == MNI_ACTION)
                    {
                        STR_APPEND(msg, "run action  ");
                    }
                    else
                    {
                        STR_APPEND(msg, "change value");
                    }

                    #ifdef CONFIG_EOSM
                    if (!CURRENT_DIALOG_MAYBE)
                    #else
                    if (CURRENT_DIALOG_MAYBE) // we can use scrollwheel
                    #endif
                        bfnt_draw_char(ICON_MAINDIAL, 680, 415, COLOR_CYAN, COLOR_BLACK);
                    else
                        leftright_sign(690, 415);
                }
                else if (menu->children && !submenu_mode && !menu_lv_transparent_mode)
                {
                    char *button = Q_BTN_NAME;
                    
#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_7D) // Q not working while recording, use INFO instead
                    if (recording)
                    {
                        button = "[INFO]";
                    }
#endif
                    
                    int nspaces = 16 - strlen(button);
                    for (int i = 0; i < nspaces; i++) { STR_APPEND(msg, " "); }
                    
                    STR_APPEND(msg, "%s: open submenu ", button);
                }
                
                //~ while (strlen(msg) < 60) { STR_APPEND(msg, " "); }

                
                bmp_printf(
                    FONT(FONT_MED, COLOR_CYAN, COLOR_BLACK), 
                     10,  MENU_KEYHELP_Y_POS, 
                    msg
                );
                
            #if !defined(CONFIG_5DC) && !defined(CONFIG_EOSM)
                if (!submenu_mode && !menu_lv_transparent_mode) // we can use scrollwheel for navigation
                {
                    bfnt_draw_char(ICON_MAINDIAL, 680, 415, COLOR_GRAY50, COLOR_BLACK);
                    if (!CURRENT_DIALOG_MAYBE) // wait, we CAN'T use it... 
                                               // and you need to be careful because you will change shooting settings while recording!
                    {
                        draw_line(720, 430, 680, 445, COLOR_WHITE);
                        draw_line(720, 431, 680, 446, COLOR_WHITE);
                    }
                }
            #endif
            }

            // if there's a warning message set, display it
            if (menu->selected && warning_msg)
            {
                bmp_printf(
                    FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK),
                     10,  MENU_WARNING_Y_POS, 
                        "                                                            "
                );

                bmp_printf(
                    FONT(FONT_MED, MENU_WARNING_COLOR, COLOR_BLACK),
                     10,  MENU_WARNING_Y_POS, 
                        warning_msg
                );
            }

            // if you have hidden some menus, display help about how to bring them back
            if (advanced_hidden_edit_mode)
            {
                bmp_printf(
                    FONT(FONT_MED, MENU_WARNING_COLOR, COLOR_BLACK),
                     10,  MENU_HELP_Y_POS, 
                        "Press MENU to hide items. Press MENU to show them again.   "
                );
            }

            // display submenu marker if this item has a submenu
            if (menu->children && !menu_lv_transparent_mode)
                submenu_icon(x, y);
            
            // display selection bar
            if (menu->selected)
                selection_bar(x, y);

            // move down for next item
            y += font_large.height;
            
            // stop before attempting to display things outside the screen
            if ((unsigned)y > 480 - font_large.height 
                //~ #if CONFIG_DEBUGMSG
                && !is_menu_active("VRAM")
                //~ #endif
            ) 
                return;
        }
                                         
        //hide buttom menu for vscroll
        if(advanced_hidden_edit_mode) menu_entry_num++;
        else                          if(IS_VISIBLE(menu)) menu_entry_num++;

        if(menu_entry_num >= menu_len) break;
        //<== vscroll

        menu = menu->next;
    }
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
            if (!IS_VISIBLE(entry))
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
        STR_APPEND(hidden_msg, advanced_hidden_edit_mode ? "." : " (press MENU).");
        
        if (strlen(hidden_msg) > 59)
        {
            hidden_msg[58] = hidden_msg[57] = hidden_msg[56] = '.';
            hidden_msg[59] = '\0';
        }

        int hidden_pos_y = MENU_KEYHELP_Y_POS - font_med.height - 5;
        if (is_menu_active("Help")) hidden_pos_y -= font_med.height;
        if (hidden_count || force_clear)
        {
            bmp_printf(
                FONT(FONT_MED, COLOR_GRAY45, COLOR_BLACK), 
                 10,  hidden_pos_y, 
                "                                                            "
            );
        }
        if (hidden_count)
        {
            bmp_printf(
                FONT(FONT_MED, advanced_hidden_edit_mode ? MENU_WARNING_COLOR : COLOR_ORANGE , COLOR_BLACK), 
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

    if(advanced_hidden_edit_mode) max = parent->childnummax;
    else                          max = parent->childnum;

    int menu_len = get_menu_len(parent);
    
    if(max > menu_len){
        bmp_draw_rect(COLOR_GRAY70, 715, 42, 4, 350);
        int16_t posx = 42 + (300 * (pos-1) / (max-1));
        bmp_fill(COLOR_GRAY70, 717, posx, 4, 50);
    }
}

static void
menus_display(
    struct menu *       menu,
    int         orig_x,
    int         y
)
{
    int         x = orig_x;

    take_semaphore( menu_sem, 0 );

    extern int override_zoom_buttons; // from focus.c
    override_zoom_buttons = 0; // will override them only if rack focus items are selected

    //~ if (!menu_lv_transparent_mode)
        //~ bmp_printf(
            //~ FONT(FONT_MED, 55, COLOR_BLACK), // gray
            //~  10,  430, 
                //~ MENU_NAV_HELP_STRING
        //~ );

    bmp_fill(COLOR_GRAY40, orig_x, y, 720, 42);
    bmp_fill(COLOR_GRAY70, orig_x, y+42, 720, 1);
    for( ; menu ; menu = menu->next )
    {
        if (!menu_has_visible_items(menu->children) && !menu->selected)
            continue; // empty menu
        if (IS_SUBMENU(menu))
            continue;
        int color_selected = advanced_hidden_edit_mode ? COLOR_DARK_RED : COLOR_BLUE;
        int fg = menu->selected ? COLOR_WHITE : 70;
        int bg = menu->selected ? color_selected : 40;
        unsigned fontspec = FONT(
            menu->selected ? FONT_LARGE : FONT_MED,
            fg,
            bg
        );
        
        if (!menu_lv_transparent_mode)
        {
            int w = fontspec_font( fontspec )->width * 6;
            //int h = fontspec_font( fontspec )->height;
            int icon_w = 0;
            if (menu->icon)
            {
                bmp_fill(bg, x+1, y, 200, 40);
                if (menu->icon == ICON_ML_PLAY) icon_w = playicon_square(x,y,fg);
                else icon_w = bfnt_draw_char(menu->icon, x, y, fg, bg);
            }
            if (!menu->icon || menu->selected)
            {
                bfnt_puts(menu->name, x + icon_w, y, fg, bg);
                //~ bmp_printf( fontspec, x + icon_w + 5, y + (40 - h)/2, "%6s", menu->name );
                x += w;
            }
            x += 62;
            #ifdef CONFIG_5DC
            x += 50;
            #endif
            //~ if (menu->selected)
            //~ {
                //~ bmp_printf( FONT(FONT_LARGE,fg,40), orig_x + 700 - font_large.width * strlen(menu->name), y + 4, menu->name );
            //~ }
        }

        if( menu->selected )
        {
            show_hidden_items(menu, 0);

            menu_display(
                menu,
                orig_x + 40,
                y + 45, 
                0
            );
            
            show_vscroll(menu);
        }
    }
    give_semaphore( menu_sem );
}


static void
implicit_submenu_display()
{
    struct menu * menu = get_selected_menu();
    menu_display(
        menu,
         40,
         45,
         1
    );
}

static void
submenu_display(struct menu * submenu)
{
    if (!submenu) return;

    int count = 0;
    struct menu_entry * child = submenu->children;
    while (child) { if (advanced_hidden_edit_mode || IS_VISIBLE(child)) count++; child = child->next; }
    int h = submenu->submenu_height ? submenu->submenu_height : (int)MIN((count + 3) * font_large.height, 400);
    int w = submenu->submenu_width  ? submenu->submenu_width : 600;
    g_submenu_width = w;
    int bx = (720 - w)/2;
    int by = (480 - h)/2 - 30;
    
    if (!menu_lv_transparent_mode)
    {
        bmp_fill(COLOR_GRAY40,  bx,  by, 720-2*bx+4, 50);
        bmp_fill(COLOR_BLACK,  bx,  by + 50, 720-2*bx+4, h-50);
        bmp_draw_rect(COLOR_GRAY70,  bx,  by, 720-2*bx, 50);
        bmp_draw_rect(COLOR_WHITE,  bx,  by, 720-2*bx, h);
        bfnt_puts(submenu->name,  bx + 15,  by + 5, COLOR_WHITE, 40);
    }

    show_hidden_items(submenu, 1);
    menu_display(submenu,  bx + 50,  by + 50 + 20, 0);
}

static void
menu_entry_showhide_toggle(
    struct menu *   menu
)
{
    if( !menu )
        return;

    take_semaphore( menu_sem, 0 );
    struct menu_entry * entry = menu->children;

    for( ; entry ; entry = entry->next )
    {
        if( entry->selected )
            break;
    }
    give_semaphore( menu_sem );

    if (entry->hidden != MENU_ENTRY_NEVER_HIDE)
    {
        entry->hidden = entry->hidden ? MENU_ENTRY_NOT_HIDDEN : MENU_ENTRY_HIDDEN;
        if(entry->hidden == MENU_ENTRY_HIDDEN){
            menu->childnum--;
        }else{
            menu->childnum++;
        }
        menu_make_sure_selection_is_valid();
        menu_hidden_dirty = 1;
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

    take_semaphore( menu_sem, 0 );
    struct menu_entry * entry = menu->children;

    for( ; entry ; entry = entry->next )
    {
        if( entry->selected )
            break;
    }
    give_semaphore( menu_sem );

    if( !entry )
        return;
    
    //~ if (entry->show_liveview)
        //~ menu_show_only_selected();
    
    // don't perform actions on empty items (can happen on empty submenus)
    if (!IS_VISIBLE(entry) && !advanced_hidden_edit_mode)
    {
        submenu_mode = 0;
        menu_lv_transparent_mode = 0;
        return;
    }

    if(mode == 1) // decrement
    {
        if( entry->select_reverse ) entry->select_reverse( entry->priv, -1 );
        else if (entry->select) entry->select( entry->priv, -1);
        else menu_numeric_toggle(entry->priv, -1, entry->min, entry->max);
    }
    else if (mode == 2) // Q
    {
        if ( entry->select_Q ) entry->select_Q( entry->priv, 1);
        else { submenu_mode = !submenu_mode; menu_lv_transparent_mode = 0; }
    }
    else if (mode == 3) // SET
    {
        if (submenu_mode == 2) submenu_mode = 0;
        else if (menu_lv_transparent_mode && entry->icon_type != IT_ACTION) menu_lv_transparent_mode = 0;
        else if (entry->edit_mode == EM_FEW_VALUES) // SET increments
        {
            if( entry->select ) entry->select( entry->priv, 1);
            else menu_numeric_toggle(entry->priv, 1, entry->min, entry->max);
        }
        else if (entry->edit_mode == EM_MANY_VALUES)
        {
            submenu_mode = (!submenu_mode)*2;
            menu_lv_transparent_mode = 0;
        }
        else if (entry->edit_mode == EM_MANY_VALUES_LV)
        {
            if (lv) menu_lv_transparent_mode = !menu_lv_transparent_mode;
            else if (submenu_mode != 1) submenu_mode = (!submenu_mode)*2;
            else // increment
            {
                if( entry->select ) entry->select( entry->priv, 1);
                else menu_numeric_toggle(entry->priv, 1, entry->min, entry->max);
            }
        }
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

    else if (!menu_has_visible_items(menu->children) && are_there_any_visible_menus())
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
    
    if (!menu_has_visible_items(menu->children))
    {
        give_semaphore( menu_sem );
        return;
    }

    struct menu_entry * entry = menu->children;

    int selectedpos= 0;
    for( ; entry ; entry = entry->next )
    {
        if(advanced_hidden_edit_mode) selectedpos++;
        else                          if(IS_VISIBLE(entry)) selectedpos++;

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
    
    if (!advanced_hidden_edit_mode && !IS_VISIBLE(entry) && menu_has_visible_items(menu->children))
        menu_entry_move(menu, direction); // try again, skip hidden items
        // warning: would block if the menu is empty
}


// Make sure we will not display an empty menu
// If the menu or the selection is empty, move back and forth to restore a valid selection
static void menu_make_sure_selection_is_valid()
{
    if (advanced_hidden_edit_mode) return; // all menus displayed
    
    struct menu * menu = get_selected_menu();
    if (submenu_mode)
    {
        struct menu * main_menu = menu;
        menu = get_current_submenu();
        if (!menu) menu = main_menu; // no submenu, operate on same item
    }
 
    // current menu has any valid items in current mode?
    if (!menu_has_visible_items(menu->children))
    {
        if (submenu_mode == 1) return; // empty submenu
        menu_move(menu, -1); menu = get_selected_menu();
        menu_move(menu, 1); menu = get_selected_menu();
    }

    // currently selected menu entry is visible?
    struct menu_entry * entry = menu->children;
    for( ; entry ; entry = entry->next )
    {
        if( entry->selected )
            break;
    }
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
        g_submenu_width = 720;
        
        if (!DISPLAY_IS_ON) return;
        if (sensor_cleaning) return;
        if (gui_state == GUISTATE_MENUDISP) return;
        
        if (menu_help_active)
        {
            BMP_LOCK( menu_help_redraw(); )
            menu_damage = 0;
        }
        else
        {
            if (!lv) menu_lv_transparent_mode = 0;
            //~ if (menu_lv_transparent_mode) quick_redraw = false;
            //~ if (MENU_MODE || lv) clrscr();

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
                    if (!quick_redraw || !submenu_mode)
                        bmp_fill(COLOR_BLACK, 0, 0, 720, 480 );
                }
                prev_z = z;

                // this part needs to know which items are selected - don't run it in the middle of selection changing
                //~ take_semaphore(menu_redraw_sem, 0);
                menu_make_sure_selection_is_valid();
                
                if (quick_redraw)
                {
                    if (!submenu_mode)
                        menus_display( menus, 0, 0 ); // only the selected item will be redrawn
                }
                else
                {
                    if (!menu_lv_transparent_mode || !submenu_mode)
                        menus_display( menus, 0, 0 ); 
                }

                if (!menu_lv_transparent_mode && !submenu_mode)
                {
                    if (is_menu_active("Help")) menu_show_version();
                    if (is_menu_active("Focus")) display_lens_hyperfocal();
                }

                if (submenu_mode)
                {
                    if (!menu_lv_transparent_mode && !quick_redraw) bmp_dim();
                    struct menu * submenu = get_current_submenu();
                    if (submenu) submenu_display(submenu);
                    else implicit_submenu_display();
                }
                
                //~ give_semaphore(menu_redraw_sem);

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
            menu_redraw_do();
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

/*void menu_inject_redraw_event()
{
    menu_redraw();
}*/

static struct menu * get_selected_menu()
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    return menu;
}

static struct menu * get_current_submenu()
{
    if (submenu_mode == 2) return 0;
    
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    struct menu_entry * entry = menu->children;
    for( ; entry ; entry = entry->next )
        if( entry->selected )
            break;
    if (entry->children)
        return menu_find_by_name(entry->name, 0);

    // no submenu
    submenu_mode = 2;
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
        {
            beta_set_warned();
            menu_redraw();
        }
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
/*        if (submenu_mode) submenu_mode = 0;
        else advanced_hidden_edit_mode = !advanced_hidden_edit_mode;
        menu_lv_transparent_mode = 0;
        menu_help_active = 0;
*/
        if (!menu_lv_transparent_mode && submenu_mode != 2)
        {
            if (!advanced_hidden_edit_mode) advanced_hidden_edit_mode = 2;
            else menu_entry_showhide_toggle(menu);
            menu_needs_full_redraw = 1;
            //~ menu_hidden_should_display_help = 1;
        }
        
        break;
    
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
        
    case BGMT_PRESS_ZOOMIN_MAYBE:
        if (lv) menu_lv_transparent_mode = !menu_lv_transparent_mode;
        else submenu_mode = (!submenu_mode)*2;
        menu_damage = 1;
        menu_help_active = 0;
        menu_needs_full_redraw = 1;
        break;

    case BGMT_PRESS_UP:
    case BGMT_WHEEL_UP:
        if (menu_help_active) { menu_help_prev_page(); break; }
        menu_entry_move( menu, -1 );
         if (submenu_mode == 2 || menu_lv_transparent_mode) menu_needs_full_redraw = 1;
        //~ if (!submenu_mode) menu_lv_transparent_mode = 0;
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_PRESS_DOWN:
    case BGMT_WHEEL_DOWN:
        if (menu_help_active) { menu_help_next_page(); break; }
        menu_entry_move( menu, 1 );
         if (submenu_mode == 2 || menu_lv_transparent_mode) menu_needs_full_redraw = 1;
        //~ if (!submenu_mode) menu_lv_transparent_mode = 0;
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_PRESS_RIGHT:
    case BGMT_WHEEL_RIGHT:
        menu_damage = 1;
        if (menu_help_active) { menu_help_next_page(); break; }
        if (submenu_mode || menu_lv_transparent_mode) menu_entry_select( menu, 0 );
        else { menu_move( menu, 1 ); menu_lv_transparent_mode = 0; }
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_PRESS_LEFT:
    case BGMT_WHEEL_LEFT:
        menu_damage = 1;
        if (menu_help_active) { menu_help_prev_page(); break; }
        if (submenu_mode || menu_lv_transparent_mode) menu_entry_select( menu, 1 );
        else { menu_move( menu, -1 ); menu_lv_transparent_mode = 0; }
        //~ menu_hidden_should_display_help = 0;
        break;

    case BGMT_UNPRESS_SET:
        return 0; // block Canon menu redraws

#if defined(CONFIG_5D3) || defined(CONFIG_7D)
    case BGMT_JOY_CENTER:
#endif
    case BGMT_PRESS_SET:
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
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
        menu_entry_select( menu, 1 ); // reverse select
        menu_needs_full_redraw = 1;
        //~ menu_damage = 1;
        //~ menu_hidden_should_display_help = 0;
        break;

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
#ifdef CONFIG_5D2
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
    static int prev_submenu_mode = 0;
    if (submenu_mode != prev_submenu_mode) menu_needs_full_redraw = 1;
    prev_submenu_mode = submenu_mode;
    
    if (menu_needs_full_redraw) menu_redraw_full();
    else menu_redraw();
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
    menu_redraw_sem = create_named_semaphore( "menu_r", 1);

#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_5D2) || defined(CONFIG_500D) || defined(CONFIG_600D) || defined(CONFIG_1100D) || defined(CONFIG_5D3)
    menu_find_by_name( "Audio", ICON_MIC);
#endif
    menu_find_by_name( "Expo", ICON_AE);
    menu_find_by_name( "Overlay", ICON_LV);
#if defined(CONFIG_500D)
    menu_find_by_name( "Movie", ICON_FILM );
#endif
#ifndef CONFIG_5DC
    menu_find_by_name( "Movie", ICON_VIDEOCAM );
#endif
    menu_find_by_name( "Shoot", ICON_PHOTOCAM );
    //~ menu_find_by_name( "Brack" );
    menu_find_by_name( "Focus", ICON_SHARPNESS );
    //~ menu_find_by_name( "LUA" );
    //menu_find_by_name( "Games" );
#ifndef CONFIG_5DC
    menu_find_by_name( "Display", ICON_MONITOR );
#endif
    menu_find_by_name( "Prefs", ICON_SMILE );
    //~ menu_find_by_name( "Play", ICON_ML_PLAY );
    //~ menu_find_by_name( "Power", ICON_P_SQUARE );
    menu_find_by_name( "Debug", ICON_HEAD_WITH_RAYS );
    //~ menu_find_by_name( "Config" );
    //~ menu_find_by_name( "Config", ICON_CF );
    menu_find_by_name( "Help", ICON_i );
    //~ menu_find_by_name( "Boot" );

/*
    bmp_printf( FONT_LARGE, 0, 40, "Yes, use this battery" );
    gui_control( ELECTRONIC_SUB_DIAL_RIGHT, 1, 0 );
    msleep( 2000 );
    gui_control( PRESS_SET_BUTTON, 1, 0 );
    msleep( 2000 );

    // Try to defeat the battery message
    //GUI_SetErrBattery( 1 );
    //msleep( 100 );
    //StopErrBatteryApp();

    msleep( 1000 );
*/
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


int
gui_menu_shown( void )
{
    return menu_shown;
}

int get_draw_event() { return draw_event; }

void toggle_draw_event( void * priv )
{
    draw_event = !draw_event;
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
    if (!lv) msleep(200);
    else if (EXT_MONITOR_CONNECTED) msleep(300);
    for (int i = 0; i < 5; i++)
    {
        if (redraw_flood_stop) break;
        if (!CURRENT_DIALOG_MAYBE) break;
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
    
#ifdef CONFIG_5DC
    //~ forces the 5dc screen to turn on for ML menu.
    if (!DISPLAY_IS_ON) fake_simple_button(BGMT_MENU);
    msleep(50);
#endif
    
    menu_lv_transparent_mode = 0;
    submenu_mode = 0;
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

    advanced_hidden_edit_mode = 0;
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
    
    menu_load_hidden_items();
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
                if (config_autosave && (config_dirty || menu_hidden_dirty) && !recording && !ml_shutdown_requested)
                {
                    save_config(0);
                    config_dirty = 0;
                    menu_hidden_dirty = 0;
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
        struct menu_entry * entry = menu->children;
        for( ; entry ; entry = entry->next)
            if (entry->selected)
                break;
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

void hide_menu_by_name(char* name, char* entry_name)
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        if (streq(menu->name, name))
        {
            struct menu_entry * entry = menu->children;
            
            int i;
            for(i = 0 ; entry ; entry = entry->next, i++ )
            {
                if (streq(entry->name, entry_name))
                {
                    entry->hidden = 1;
                    menu->childnum--;
                }
            }
        }
    }
    //~ menu_damage = 1;
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

    take_semaphore(menu_sem, 0);

    struct menu_entry * entry = menu->children;

    for( ; entry ; entry = entry->next )
    {
        if( entry->selected )
            break;
    }
    
    menu_help_go_to_label(entry->name);
    
    give_semaphore(menu_sem);
}

static void menu_show_version(void)
{
    big_bmp_printf(FONT_MED,  10,  410,
        "Magic Lantern version : %s\n"
        "Mercurial changeset   : %s\n"
        "Built on %s by %s.",
        build_version,
        build_id,
        build_date,
        build_user);
}

void
menu_title_hack_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    unsigned fontspec = FONT(
        FONT_MED,
        selected ? COLOR_WHITE : 60,
        selected ? 13 : COLOR_BLACK
    );

    menu_draw_icon(x, y, -1, 0);
    bmp_printf(
        fontspec,
        x - 35, y + 10,
        (char*)priv
    );
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
}

void menu_close_submenu()
{
    submenu_mode = 0;
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

void config_menu_save_hidden_items()
{
    if (!menu_hidden_dirty) return;

    #define MAX_SIZE 10240
    char* msg = alloc_dma_memory(MAX_SIZE);
    msg[0] = '\0';

    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        struct menu_entry * entry = menu->children;
        
        int i;
        for(i = 0 ; entry ; entry = entry->next, i++ )
        {
            if (!IS_VISIBLE(entry))
            {
                snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1, "%s\\%s\n", menu->name, entry->name);
            }
        }
    }
    
    FILE * file = FIO_CreateFileEx( CARD_DRIVE "ML/SETTINGS/HIDDEN.CFG" );
    if( file == INVALID_PTR )
        return;
    
    FIO_WriteFile(file, msg, strlen(msg));

    FIO_CloseFile( file );
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

static void menu_load_hidden_items()
{
    int size = 0;
    char* buf = (char*)read_entire_file( CARD_DRIVE "ML/SETTINGS/HIDDEN.CFG", &size);
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
                char* menu_name = &buf[prev+1];
                char* entry_name = &buf[sep+1];
                //~ NotifyBox(2000, "%s -> %s", menu_name, entry_name); msleep(2000);
                hide_menu_by_name(menu_name, entry_name);
            }
            prev = i;
        }
    }
    free_dma_memory(buf);
}
