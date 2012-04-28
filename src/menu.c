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

static struct semaphore * menu_sem;
extern struct semaphore * gui_sem;
static struct semaphore * menu_redraw_sem;
static int menu_damage;
static int menu_timeout;
static bool menu_shown = false;
static int show_only_selected; // for ISO, kelvin...
static int config_dirty = 0;
static char* warning_msg = 0;
int menu_help_active = 0;
int submenu_mode = 0;
static int menu_id_increment = 1;

int is_submenu_mode_active() { return gui_menu_shown() && submenu_mode; }

//~ static CONFIG_INT("menu.transparent", semitransparent, 0);

static CONFIG_INT("menu.first", menu_first_by_icon, ICON_i);
static CONFIG_INT("menu.advanced", advanced_mode, 1);

int get_menu_advanced_mode() { return advanced_mode; }

static int x0 = 0;
static int y0 = 0;

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

extern int gui_state;
void menu_show_only_selected()
{
    show_only_selected = 1;
    menu_damage = 1;
}
int menu_active_but_hidden() { return gui_menu_shown() && ( show_only_selected ); }
int menu_active_and_not_hidden() { return gui_menu_shown() && !( show_only_selected ); }

int draw_event = 0;
CONFIG_INT( "debug.menu-timeout", menu_timeout_time, 1000 ); // doesn't work and breaks rack focus

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
        "http://magiclantern.wikia.com/"
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


struct dialog * menu_dialog = 0;
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
        else if (!entry->priv || entry->select == run_in_separate_task)
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
    
    
    char msg[100] = "";
    STR_APPEND(msg, "%s", entry->name);
    if (entry->priv && entry->select != run_in_separate_task)
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
        if (IS_SUBMENU(menu)) new_entry->essential = FOR_SUBMENU;
        new_entry++;
        count--;
    }

    // Find the end of the entries on the menu already
    while( head->next )
        head = head->next;

    for (int i = 0; i < count; i++)
    {
        if (new_entry->id == 0) new_entry->id = menu_id_increment++;
        new_entry->selected = 0;
        if (IS_SUBMENU(menu)) new_entry->essential = FOR_SUBMENU;
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
            menu_find_by_name( entry->name, ICON_ML_SUBMENU);
            menu_add(entry->name, entry->children, count);
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
        draw_line(x + 3, y + i, x + 18 + 3, y + 16, COLOR_WHITE);
        draw_line(x + 3, y + i, x + 18 + 3, y + 16, COLOR_WHITE);

        draw_line(x - 3, y + i, x - 18 - 3, y + 16, COLOR_WHITE);
        draw_line(x - 3, y + i, x - 18 - 3, y + 16, COLOR_WHITE);
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
    int color = value ? COLOR_GREEN1 : 45;
    for (int r = 0; r < 3; r++)
    {
        draw_circle(x + 8, y + 10, r, color);
        draw_circle(x + 8, y + 16, r, color);
        draw_circle(x + 8, y + 22, r, color);
        draw_circle(x + 9, y + 10, r, color);
        draw_circle(x + 9, y + 16, r, color);
        draw_circle(x + 9, y + 22, r, color);
    }
    
    color = value ? COLOR_WHITE : 45;
    bmp_draw_rect(color, x + 15, y + 10, 10, 1);
    bmp_draw_rect(color, x + 15, y + 16, 10, 1);
    bmp_draw_rect(color, x + 15, y + 22, 10, 1);
}

void selection_bar(int x0, int y0)
{
    int w = x0 + 720 - 40 - 10;
    if (submenu_mode==1) w -= 90;

    extern int bmp_color_scheme;
    
    uint8_t* B = bmp_vram();
    for (int y = y0; y < y0 + 31; y++)
    {
        for (int x = x0-5; x < w; x++)
        {
            if (B[BM(x,y)] == COLOR_BLACK)
                B[BM(x,y)] = submenu_mode || bmp_color_scheme ? COLOR_LIGHTBLUE : COLOR_BLUE;
        }
    }
}

void size_icon(int x, int y, int current, int nmax)
{
    dot(x, y, COLOR_GREEN1, current * (nmax > 2 ? 9 : 7) / (nmax-1) + 3);
}

void dice_icon(int x, int y, int current, int nmax)
{
    #define C(i) (current == (i) ? COLOR_GREEN1 : 50), (current == (i) ? 6 : 4)
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
    else if (streq(color, "Luma"))
        maru(x, y, 60);
    else if (streq(color, "RGB"))
    {
        dot(x,     y - 7, COLOR_RED, 5);
        dot(x - 7, y + 3, COLOR_GREEN2, 5);
        dot(x + 7, y + 3, COLOR_LIGHTBLUE, 5);
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
    if (type >= 0) bmp_printf(FONT_LARGE, x, y, "  "); // cleanup background
    warning_msg = 0;
    switch(type)
    {
        case MNI_OFF: maru(x, y, 40); return;
        case MNI_ON: maru(x, y, COLOR_GREEN1); return;
        case MNI_DISABLE: batsu(x, y, COLOR_RED); return;
        case MNI_NEUTRAL: maru(x, y, 60); return;
        case MNI_WARNING: maru(x, y, COLOR_RED); warning_msg = (char *) arg; return;
        case MNI_AUTO: maru(x, y, 9); return;
        case MNI_PERCENT: percent(x, y, arg); return;
        case MNI_ACTION: playicon(x, y); return;
        case MNI_DICE: dice_icon(x, y, arg & 0xFFFF, arg >> 16); return;
        case MNI_SIZE: size_icon(x, y, arg & 0xFFFF, arg >> 16); return;
        case MNI_NAMED_COLOR: color_icon(x, y, (char *)arg); return;
        case MNI_SUBMENU: submenu_only_icon(x, y, arg); return;
    }
    #endif
}

static int
menu_has_visible_items(struct menu_entry *  menu)
{
    while( menu )
    {
        if (advanced_mode || IS_ESSENTIAL(menu))
        {
            return 1;
        }
        menu = menu->next;
    }
    return 0;
}

static void
menu_display(
    struct menu_entry * menu,
    int         x,
    int         y
)
{
    while( menu )
    {
        if (advanced_mode || IS_ESSENTIAL(menu))
        {
            // display help (should be first; if there are too many items in menu, the main text should overwrite the help, not viceversa)
            if (menu->selected && menu->help)
            {
                bmp_printf(
                    FONT(FONT_MED, 0xC, COLOR_BLACK), // red
                    x0 + 10, y0 + 450, 
                        "                                                           "
                );
                bmp_printf(
                    FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 
                    x0 + 10 /* + ((700/font_med.width) - strlen(menu->help)) * font_med.width / 2*/, y0 + 450, 
                    menu->help
                );
            }

            // display icon (only the first icon is drawn)
            icon_drawn = 0;
            if (!show_only_selected || menu->selected)
            {
                if (menu->display)
                    menu->display(
                        menu->priv,
                        x,
                        y,
                        menu->selected
                    );
                else
                    submenu_print(menu, x, y);
            }
            
            // this should be after menu->display, in order to allow it to override the icon
            if (menu->selected || !show_only_selected)
            {
                entry_draw_icon(menu, x, y);
            }

            // display key help
            if (menu->selected && !is_menu_active("Help") && (menu->priv || menu->select))
            {
                char msg[100] = "";

                // this should follow exactly the same logic as in menu_entry_select
                // todo: remove duplicate code
                
                
                // exception for action and submenu items
                if (icon_drawn == MNI_ACTION) 
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
                else if (show_only_selected)
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


                STR_APPEND(msg, "        ", Q_BTN_NAME);
                if (submenu_mode || show_only_selected)
                {
                    if (CURRENT_DIALOG_MAYBE) // GUIMode nonzero => wheel events working
                    {
                        STR_APPEND(msg, "L/R/Wheel : ");
                    }
                    else
                    {
                        STR_APPEND(msg, "Left/Right: ");
                    }
                    if (icon_drawn == MNI_ACTION)
                    {
                        STR_APPEND(msg, "run action  ");
                    }
                    else
                    {
                        STR_APPEND(msg, "change value");
                    }
                    leftright_sign(x0+690, y0+400);
                }
                else if (menu->children && !submenu_mode && !show_only_selected)
                {
                    STR_APPEND(msg, "%s: open submenu ", Q_BTN_NAME);
                }
                
                bmp_printf(
                    FONT(FONT_MED, 60, COLOR_BLACK), 
                    x0 + 10, y0 + 425, 
                    msg
                );
            }
            
            // if there's a warning message set, display it
            if (menu->selected && warning_msg)
            {
                bmp_printf(
                    FONT(FONT_MED, 0xC, COLOR_BLACK), // red
                    x0 + 10, y0 + 450, 
                        "                                                           "
                );
                bmp_printf(
                    FONT(FONT_MED, 0xC, COLOR_BLACK), // red
                    x0 + 10, y0 + 450, 
                        warning_msg
                );
            }

            // display submenu marker if this item has a submenu
            if (menu->children && !show_only_selected)
                submenu_icon(x, y);
            
            // display selection bar
            if (menu->selected)
                selection_bar(x, y);

            // move down for next item
            y += font_large.height-1;
            
            // stop before attempting to display things outside the screen
            if ((unsigned)y > vram_bm.height - font_large.height 
                #if CONFIG_DEBUGMSG
                && !is_menu_active("VRAM")
                #endif
            ) 
                return;
        }
        menu = menu->next;
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

    //~ if (!show_only_selected)
        //~ bmp_printf(
            //~ FONT(FONT_MED, 55, COLOR_BLACK), // gray
            //~ x0 + 10, y0 + 430, 
                //~ MENU_NAV_HELP_STRING
        //~ );

    bmp_fill(40, orig_x, y, 720, 42);
    bmp_fill(70, orig_x, y+42, 720, 1);
    for( ; menu ; menu = menu->next )
    {
        if (!menu_has_visible_items(menu->children))
            continue; // empty menu
        if (IS_SUBMENU(menu))
            continue;

        int fg = menu->selected ? COLOR_WHITE : 70;
        int bg = menu->selected ? 13 : 40;
        unsigned fontspec = FONT(
            menu->selected ? FONT_LARGE : FONT_MED,
            fg,
            bg
        );
        if (!show_only_selected)
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
            x += 47;
            //~ if (menu->selected)
            //~ {
                //~ bmp_printf( FONT(FONT_LARGE,fg,40), orig_x + 700 - font_large.width * strlen(menu->name), y + 4, menu->name );
            //~ }
        }

        if( menu->selected )
            menu_display(
                menu->children,
                orig_x + 40,
                y + 45
            );
    }
    give_semaphore( menu_sem );
}

static void
implicit_submenu_display()
{
    struct menu * menu = get_selected_menu();
    int sos = show_only_selected;
    show_only_selected = 1;
    menu_display(
        menu->children,
        x0 + 40,
        y0 + 45
    );
    show_only_selected = sos;
}

static void
submenu_display(struct menu * submenu)
{
    if (!submenu) return;

    int count = 0;
    struct menu_entry * child = submenu->children;
    while (child) { count++; child = child->next; }
    int h = MIN((count + 4) * font_large.height, 400);

    int bx = 45;
    int by = (480 - h)/2 - 30;
    if (!show_only_selected)
    {
        bmp_fill(40, x0 + bx, y0 + by, 720-2*bx+4, 50);
        bmp_fill(COLOR_BLACK, x0 + bx, y0 + by + 50, 720-2*bx+4, h-50);
        bmp_draw_rect(70, x0 + bx, y0 + by, 720-2*bx, 50);
        bmp_draw_rect(COLOR_WHITE, x0 + bx, y0 + by, 720-2*bx, h);
        bfnt_puts(submenu->name, x0 + bx + 15, y0 + by + 5, COLOR_WHITE, 40);
    }

    menu_display(submenu->children, x0 + bx + 50, y0 + by + 50 + 20);
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

    if(mode == 1) // decrement
    {
        if( entry->select_reverse ) entry->select_reverse( entry->priv, -1 );
        else if (entry->select) entry->select( entry->priv, -1);
        else menu_numeric_toggle(entry->priv, -1, entry->min, entry->max);
    }
    else if (mode == 2) // Q
    {
        if ( entry->select_Q ) entry->select_Q( entry->priv, 1);
        else { submenu_mode = !submenu_mode; show_only_selected = 0; }
    }
    else if (mode == 3) // SET
    {
        if (submenu_mode == 2) submenu_mode = 0;
        else if (show_only_selected) show_only_selected = 0;
        else if (entry->edit_mode == EM_FEW_VALUES) // SET increments
        {
            if( entry->select ) entry->select( entry->priv, 1);
            else menu_numeric_toggle(entry->priv, 1, entry->min, entry->max);
        }
        else if (entry->edit_mode == EM_MANY_VALUES)
        {
            submenu_mode = (!submenu_mode)*2;
            show_only_selected = 0;
        }
        else if (entry->edit_mode == EM_MANY_VALUES_LV)
        {
            if (lv) show_only_selected = !show_only_selected;
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

    int rc = take_semaphore( menu_sem, 1000 );
    if( rc != 0 )
        return;

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
    
    if (IS_SUBMENU(menu) || !menu_has_visible_items(menu->children))
        menu_move(menu, direction); // this menu is hidden, skip it (try again)
        // will fail if no menus are displayed!
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

    int rc = take_semaphore( menu_sem, 1000 );
    if( rc != 0 )
        return;
    
    if (!menu_has_visible_items(menu->children))
        return;

    struct menu_entry * entry = menu->children;

    for( ; entry ; entry = entry->next )
    {
        if( entry->selected )
            break;
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
        if( entry->prev )
            entry = entry->prev;
        else {
            // Go to the last one
            while( entry->next )
                entry = entry->next;
        }
    } else {
        // Last and moving down?
        if( entry->next )
            entry = entry->next;
        else {
            // Go to the first one
            while( entry->prev )
                entry = entry->prev;
        }
    }

    // Select the new one, which might be the same as the old one
    entry->selected = 1;
    give_semaphore( menu_sem );
    
    if (!advanced_mode && !IS_ESSENTIAL(entry))
        menu_entry_move(menu, direction); // try again, skip hidden items
        // warning: would block if the menu is empty
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
        
        int double_buffering = 1;
        
        if (menu_help_active)
        {
            BMP_LOCK( menu_help_redraw(); )
            //~ menu_damage = 0;
        }
        else
        {
            if (!lv) show_only_selected = 0;
            //~ if (MENU_MODE || lv) clrscr();

            //~ menu_damage = 0;
            BMP_LOCK (
                if (double_buffering)
                {
                    // draw to mirror buffer to avoid flicker
                    //~ bmp_idle_copy(0); // no need, drawing is fullscreen anyway
                    bmp_draw_to_idle(1);
                }

                static int prev_so = 0;
                if (show_only_selected)
                {
                    bmp_fill( 0, 0, 0, 960, 540 );
                    if (zebra_should_run())
                    {
                        if (prev_so) copy_zebras_from_mirror();
                        else cropmark_clear_cache(); // will clear BVRAM mirror and reset cropmarks
                    }
                    //~ draw_histogram_and_waveform(); // too slow
                }
                else
                {
                    bmp_fill(COLOR_BLACK, 0, 0, 960, 540 );
                }
                prev_so = show_only_selected;

                if (!show_only_selected || !submenu_mode)
                    menus_display( menus, x0, y0 ); 

                if (!show_only_selected && !submenu_mode)
                    if (is_menu_active("Help")) menu_show_version();

                if (submenu_mode)
                {
                    if (!show_only_selected) bmp_dim();
                    struct menu * submenu = get_current_submenu();
                    if (submenu) submenu_display(submenu);
                    else implicit_submenu_display();
                }

                if (show_only_selected) 
                {
                    draw_ml_topbar(0, 1);
                    draw_ml_bottombar(0, 1);
                }

                if (recording)
                    bmp_make_semitransparent();

                if (double_buffering)
                {
                    // copy image to main buffer
                    bmp_draw_to_idle(0);

                    int screen_layout = get_screen_layout();
                    if (hdmi_code == 2) // copy at a smaller scale to fit the screen
                    {
                        if (screen_layout == SCREENLAYOUT_16_10)
                            bmp_zoom(bmp_vram(), bmp_vram_idle(), x0 + 360, y0 + 150, /* 128 div */ 143, /* 128 div */ 169);
                        else if (screen_layout == SCREENLAYOUT_16_9)
                            bmp_zoom(bmp_vram(), bmp_vram_idle(), x0 + 360, y0 + 150, /* 128 div */ 143, /* 128 div */ 185);
                        else
                        {
                            if (menu_upside_down) bmp_flip(bmp_vram(), bmp_vram_idle(), 0);
                            else bmp_idle_copy(1);
                        }
                    }
                    else if (ext_monitor_rca)
                        bmp_zoom(bmp_vram(), bmp_vram_idle(), x0 + 360, y0 + 200, /* 128 div */ 135, /* 128 div */ 135);
                    else
                    {
                        if (menu_upside_down) bmp_flip(bmp_vram(), bmp_vram_idle());
                        else bmp_idle_copy(1);
                    }
                    //~ bmp_idle_clear();
                }
            )
            //~ update_stuff();
            
            lens_display_set_dirty();
            update_disp_mode_bits_from_params();
        }
}

struct msg_queue * menu_redraw_queue = 0;

static void
menu_redraw_task()
{
    menu_redraw_queue = msg_queue_create("menu_redraw_mq", 1);
    while(1)
    {
        int msg;
        msg_queue_receive(menu_redraw_queue, &msg, 0);
        
        take_semaphore(menu_redraw_sem, 0);
        menu_redraw_do();
        give_semaphore(menu_redraw_sem);
    }
}
TASK_CREATE( "menu_redraw_task", menu_redraw_task, 0, 0x1d, 0x4000 );

static void
menu_redraw()
{
    if (menu_redraw_queue) msg_queue_post(menu_redraw_queue, 1);
}

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

// only for checking correct usage of dialog box API
int menu_minimal_handler(void * dialog, int tmpl, gui_event_t event, int arg3, void* arg4, int arg5, int arg6, int code) 
{
    switch (event) {

    case TERMINATE_WINSYS:
        menu_dialog = NULL;
        menu_close_post_delete_dialog_box();
        return 1;

    case DELETE_DIALOG_REQUEST:
        menu_close_gmt();
        return dialog != arg4;  // ?!

    default:
        break;
    }
    return 1;
}

static int
menu_handler(void * dialog, int tmpl, gui_event_t event, int arg3, void* arg4, int arg5, int arg6, int code) 
{
#if 0
    if( event > 1 && event < 0x10000000)
    {
        bmp_printf( FONT_SMALL, 400, 40,
            "evt %8x(%8x,%8x,%8x)",
            event, arg2, arg3, arg4
        );
    }
#endif

    // the first steps may temporarily change the selected menu item - don't redraw in the middle of this
    take_semaphore(menu_redraw_sem, 0);

    // Find the selected menu (should be cached?)
    struct menu * menu = get_selected_menu();
    
    // Make sure we are not displaying an empty menu
    if (!menu_has_visible_items(menu->children))
    {
        menu_move(menu, -1); menu = get_selected_menu();
        menu_move(menu, 1); menu = get_selected_menu();
    }
    
    menu_entry_move(menu, -1);
    menu_entry_move(menu, 1);
    
    struct menu * help_menu = menu;
    if (submenu_mode)
    {
        help_menu = menu;
        menu = get_current_submenu();
        if (!menu) menu = help_menu; // no submenu, operate on same item
    }

    give_semaphore(menu_redraw_sem);
    
    switch( event )
    {
    //~ bmp_printf(FONT_MED, 0, 0, "dlg=%x template=%x btn=%x %x %x %x\ncode=%x", dialog, template, event, arg3, arg4, arg5, arg6, code);

    case GOT_TOP_OF_CONTROL:
        //~ NotifyBox(2000, "GOT_TOP_OF_CONTROL");
        menu_redraw();
        return 1;

    case TERMINATE_WINSYS:
        menu_dialog = NULL;
        menu_close_post_delete_dialog_box();
        return 1;

    case LOST_TOP_OF_CONTROL:
        menu_close_gmt();
        return 0;

    case DELETE_DIALOG_REQUEST:
        menu_close_gmt();
        return dialog != arg4;  // ?!


    case PRESS_MENU_BUTTON:
        if (submenu_mode) submenu_mode = 0;
        else advanced_mode = !advanced_mode;
        show_only_selected = 0;
        menu_help_active = 0;
        break;

    case EVENTID_METERING_START: // If they press the shutter halfway
        menu_close_gmt();
        return 1;
    
    case EVENTID_94:
        // Generated when buttons are pressed?  Forward it on
        return 1;
    
    case PRESS_ZOOM_IN_BUTTON:
        if (lv) show_only_selected = !show_only_selected;
        else submenu_mode = (!submenu_mode)*2;
        menu_damage = 1;
        menu_help_active = 0;
        break;

#if defined(CONFIG_50D) || defined(CONFIG_5D2)
    case PRESS_JOY_UP:
#else
    case PRESS_UP_BUTTON:
#endif
    case ELECTRONIC_SUB_DIAL_LEFT:
        if (menu_help_active) { menu_help_prev_page(); break; }
        menu_entry_move( menu, -1 );
        //~ if (!submenu_mode) show_only_selected = 0;
        break;

#if defined(CONFIG_50D) || defined(CONFIG_5D2)
    case PRESS_JOY_DOWN:
#else
    case PRESS_DOWN_BUTTON:
#endif
    case ELECTRONIC_SUB_DIAL_RIGHT:
        if (menu_help_active) { menu_help_next_page(); break; }
        menu_entry_move( menu, 1 );
        //~ if (!submenu_mode) show_only_selected = 0;
        break;

#if defined(CONFIG_50D) || defined(CONFIG_5D2)
    case PRESS_JOY_RIGHT:
#else
    case PRESS_RIGHT_BUTTON:
#endif
        //~ if (!submenu_mode) show_only_selected = 0;
    case DIAL_RIGHT:
        menu_damage = 1;
        if (menu_help_active) { menu_help_next_page(); break; }
        if (submenu_mode || show_only_selected) menu_entry_select( menu, 0 );
        else { menu_move( menu, 1 ); show_only_selected = 0; }
        break;

#if defined(CONFIG_50D) || defined(CONFIG_5D2)
    case PRESS_JOY_LEFT:
#else
    case PRESS_LEFT_BUTTON:
#endif
        //~ if (!submenu_mode) show_only_selected = 0;
    case DIAL_LEFT:
        menu_damage = 1;
        if (menu_help_active) { menu_help_prev_page(); break; }
        if (submenu_mode || show_only_selected) menu_entry_select( menu, 1 );
        else { menu_move( menu, -1 ); show_only_selected = 0; }
        break;

    case PRESS_SET_BUTTON:
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        else
        {
            menu_entry_select( menu, 3 ); // "SET" select
        }
        //~ menu_damage = 1;
        break;
    case UNPRESS_SET_BUTTON:
        break;

    case PRESS_INFO_BUTTON:
        menu_help_active = !menu_help_active;
        show_only_selected = 0;
        if (menu_help_active) menu_help_go_to_selected_entry(help_menu);
        //~ menu_damage = 1;
        break;

    case PRESS_PLAY_BUTTON:
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        menu_entry_select( menu, 1 ); // reverse select
        //~ menu_damage = 1;
        break;

    case PRESS_DIRECT_PRINT_BUTTON:
#ifdef PRESS_DIRECT_PRINT_BUTTON_ALT
    case PRESS_DIRECT_PRINT_BUTTON_ALT:
#endif
#ifdef CONFIG_50D
    case PRESS_FUNC_BUTTON:
    case JOY_CENTER:
#endif
#ifdef CONFIG_5D2
    case PRESS_PICTURE_STYLE_BUTTON:
    case JOY_CENTER:
#endif
        if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
        menu_entry_select( menu, 2 ); // auto setting select
        //~ menu_damage = 1;
        break;

#if defined(CONFIG_50D) || defined(CONFIG_5D2)
    case PRESS_JOY_LEFTUP:
    case PRESS_JOY_LEFTDOWN:
    case PRESS_JOY_RIGHTUP:
    case PRESS_JOY_RIGHTDOWN:
        break; // ignore
#endif

    case EVENT_1:          // Synthetic redraw event
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
    menu_timeout = menu_timeout_time;

    // If we no longer exist, do not redraw
    if( !menu_dialog )
        return 0;

    menu_redraw();

    return 0;
}





void
menu_init( void )
{
    menus = NULL;
    menu_dialog = NULL;
    menu_sem = create_named_semaphore( "menus", 1 );
    gui_sem = create_named_semaphore( "gui", 0 );
    menu_redraw_sem = create_named_semaphore( "menu_r", 1);

#if defined(CONFIG_550D) || defined(CONFIG_60D) || defined(CONFIG_5D2) || defined(CONFIG_500D)
    menu_find_by_name( "Audio", ICON_MIC);
#endif
    menu_find_by_name( "Expo", ICON_AE);
    menu_find_by_name( "LiveV", ICON_LV);
#if defined(CONFIG_500D)
    menu_find_by_name( "Movie", ICON_FILM );
#endif
    menu_find_by_name( "Movie", ICON_VIDEOCAM );
    menu_find_by_name( "Shoot", ICON_PHOTOCAM );
    //~ menu_find_by_name( "Brack" );
    menu_find_by_name( "Focus", ICON_SHARPNESS );
    //~ menu_find_by_name( "LUA" );
    //menu_find_by_name( "Games" );
    menu_find_by_name( "Display", ICON_MONITOR );
    menu_find_by_name( "Tweaks", ICON_SMILE );
    menu_find_by_name( "Play", ICON_ML_PLAY );
    menu_find_by_name( "Power", ICON_P_SQUARE );
    menu_find_by_name( "Debug", ICON_HEAD_WITH_RAYS );
    //~ menu_find_by_name( "Config" );
    menu_find_by_name( "Config", ICON_CF );
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
    fake_simple_button(MLEV_MENU_CLOSE);
}


bool
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

void piggyback_canon_menu()
{
    #ifdef GUIMODE_ML_MENU
    if (!PLAY_MODE) SetGUIRequestMode(GUIMODE_ML_MENU);
    if (GUIMODE_ML_MENU == 2) msleep(100);
    #else
    if (!lv && !MENU_MODE && !is_movie_mode())
    {
        if (!PLAY_MODE) open_canon_menu();
    }
    #endif
    msleep(100);
}

void menu_close_post_delete_dialog_box()
{
    canon_gui_enable_front_buffer(0);
 
    #ifdef GUIMODE_ML_MENU
    if (!PLAY_MODE) SetGUIRequestMode(0);
    #endif

    lens_focus_stop();
    show_only_selected = 0;

    #ifndef GUIMODE_ML_MENU
    if (MENU_MODE && !get_halfshutter_pressed())
    {
        fake_simple_button(BGMT_MENU);
    }
    #endif
    
    extern int config_autosave;
    if (config_autosave && config_dirty && !recording)
    {
        save_config(0);
        config_dirty = 0;
    }

    menu_shown = false;

    if (!PLAY_MODE) { redraw(); }
    else draw_livev_for_playback();
}

void menu_open_gmt() {

        show_only_selected = 0;
        submenu_mode = 0;
        menu_help_active = 0;

        bmp_on(); // just to be sure the BMP overlay is enabled (maybe it was disabled by ClearScreen function)
        //~ call("TurnOnDisplay");

        //~ info_led_blink(2, 50, 50);
        if (menu_dialog != NULL) {
                DeleteDialogBox(menu_dialog);
                menu_dialog = NULL;
        }

        canon_gui_disable_front_buffer(0);
        menu_dialog = (void*)CreateDialogBox(0, 0, menu_handler, 1, 0);
        dialog_redraw(menu_dialog);
        clrscr();
        //~ bmp_printf(FONT_LARGE, 100, 100, "menu");
        menu_redraw();
}

void menu_close_gmt() {
        //~ info_led_blink(3, 50, 50);
    if (menu_dialog != NULL) {
            DeleteDialogBox(menu_dialog);
            menu_dialog = NULL;
    }
    menu_close_post_delete_dialog_box();
}

void menu_open() { fake_simple_button(MLEV_MENU_OPEN); }
void menu_close() { fake_simple_button(MLEV_MENU_CLOSE); }

void menu_inject_redraw_event()
{
    if (menu_dialog)
    {
        ctrlman_dispatch_event(
            menu_dialog->gui_task,
            1,
            0,
            0
        );
    }
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
    select_menu_by_icon(menu_first_by_icon);
    while(1)
    {
        int rc = take_semaphore( gui_sem, 500 );
        if( rc != 0 )
        {
            // We woke up after 1 second
            if( !menu_dialog )
                continue;

            // Count down the menu timeout
            if( --menu_timeout == 0 )
            {
                menu_close();
                continue;
            }

            if ((!menu_help_active && !show_only_selected) || menu_damage) {
                fake_simple_button(MLEV_MENU_REDRAW);
            }

            continue;
        }

        if( menu_dialog )
        {
            menu_close();
            continue;
        }
        
        if (recording && !lv) continue;
        
        // Set this flag a bit earlier in order to pause LiveView tasks.
        // Otherwise, high priority tasks such as focus peaking might delay the menu a bit.
        menu_shown = true; 
        
        // ML menu needs to piggyback on Canon menu, in order to receive wheel events
        piggyback_canon_menu();

        x0 = hdmi_code == 5 ? 120 : 0;
        y0 = hdmi_code == 5 ? 40 : 0;

        menu_open();
    }
}

static void
menu_task_minimal( void* unused )
{
    select_menu_by_icon(menu_first_by_icon);

    while(1)
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
        //~ menu_dialog = test_dialog;

        if( !menu_dialog )
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

TASK_CREATE( "menu_task", menu_task, 0, 0x1d, 0x1000 );

int is_menu_active(char* name)
{
    if (!menu_dialog) return 0;
    if (menu_help_active) return 0;
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
        if( menu->selected )
            break;
    return !strcmp(menu->name, name);
}

void select_menu(char* name, int entry_index)
{
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        menu->selected = !strcmp(menu->name, name);
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
    struct menu * menu = menus;
    for( ; menu ; menu = menu->next )
    {
        menu->selected = !strcmp(menu->name, name);
        if (menu->selected)
        {
            struct menu_entry * entry = menu->children;
            
            int i;
            for(i = 0 ; entry ; entry = entry->next, i++ )
                entry->selected = !strcmp(entry->name, entry_name);
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
    bmp_printf(FONT_MED, x0 + 10, y0 + 410,
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
    
    if (event->param == BGMT_TRASH)
    {
        if (gui_menu_shown() || gui_state == GUISTATE_IDLE) 
        {
            give_semaphore( gui_sem );
            return 0;
        }
    }

#ifndef CONFIG_5D2
    if (event->param == BGMT_MENU && PLAY_MODE)
    {
        give_semaphore( gui_sem );
        return 0;
    }
#endif

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

#if !defined(CONFIG_50D) && !defined(CONFIG_5D2)
int handle_quick_access_menu_items(struct event * event)
{
    // quick access to some menu items
    if (event->param == BGMT_Q_ALT && !gui_menu_shown())
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
    return 1;
}
#endif
