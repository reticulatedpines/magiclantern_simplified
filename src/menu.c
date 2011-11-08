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
//~ static int menu_damage;
static int menu_hidden;
static int menu_timeout;
static int menu_shown = 0;
static int show_only_selected; // for ISO, kelvin...
static int edit_mode = 0;
static int config_dirty = 0;
static char* warning_msg = 0;
int menu_help_active = 0;

static CONFIG_INT("menu.advanced", advanced_mode, 0);

static int x0 = 0;
static int y0 = 0;

//void menu_set_dirty() { //~ menu_damage = 1; }

int is_menu_help_active() { return gui_menu_shown() && menu_help_active; }

int get_menu_font_sel() 
{
	if (recording) return FONT(FONT_LARGE,COLOR_WHITE,COLOR_RED);
	else if (edit_mode) return FONT(FONT_LARGE,COLOR_WHITE,0x12);
	else return FONT(FONT_LARGE,COLOR_WHITE,13);
}

static void menu_help_go_to_selected_entry();
//~ static void menu_init( void );
static void menu_show_version(void);

extern int gui_state;
void menu_show_only_selected()
{
	show_only_selected = 1;
	//~ menu_damage = 1;
	if (lv) edit_mode = 1;
}
int menu_active_but_hidden() { return gui_menu_shown() && ( show_only_selected || menu_hidden ); }
int menu_active_and_not_hidden() { return gui_menu_shown() && !( show_only_selected || menu_hidden ); }

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


struct gui_task * gui_menu_task;
static struct menu * menus;


void
menu_binary_toggle(
	void *			priv
)
{
	unsigned * val = priv;
	*val = !*val;
}

void menu_ternary_toggle(void* priv)
{
	unsigned * val = priv;
	*val = mod(*val + 1, 3);
}

void menu_ternary_toggle_reverse( void* priv)
{
	unsigned * val = priv;
	*val = mod(*val - 1, 3);
}

void menu_quaternary_toggle(void* priv)
{
	unsigned * val = priv;
	*val = mod(*val + 1, 4);
}

void menu_quaternary_toggle_reverse( void* priv)
{
	unsigned * val = priv;
	*val = mod(*val - 1, 4);
}

void menu_quinternary_toggle(void* priv)
{
	unsigned * val = priv;
	*val = mod(*val + 1, 5);
}

void menu_quinternary_toggle_reverse( void* priv)
{
	unsigned * val = priv;
	*val = mod(*val - 1, 5);
}

void
menu_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
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


static struct menu *
menu_find_by_name(
	const char *		name,
	int icon
)
{
	take_semaphore( menu_sem, 0 );

	struct menu *		menu = menus;

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

	new_menu->name		= name;
	new_menu->icon		= icon;
	new_menu->prev		= menu;
	new_menu->next		= NULL; // Inserting at end
	new_menu->children	= NULL;

	// menu points to the last entry or NULL if there are none
	if( menu )
	{
		// We are adding to the end
		menu->next		= new_menu;
		new_menu->selected	= 0;
	} else {
		// This is the first one
		menus			= new_menu;
		new_menu->selected	= 1;
	}

	give_semaphore( menu_sem );
	return new_menu;
}


void
menu_add(
	const char *		name,
	struct menu_entry *	new_entry,
	int			count
)
{
#if 1
	// Walk the menu list to find a menu
	struct menu *		menu = menu_find_by_name( name, 0);
	if( !menu )
		return;

	take_semaphore( menu_sem, 0 );

	struct menu_entry *	head = menu->children;
	if( !head )
	{
		// First one -- insert it as the selected item
		head = menu->children	= new_entry;
		new_entry->next		= NULL;
		new_entry->prev		= NULL;
		new_entry->selected	= 1;
		new_entry++;
		count--;
	}

	// Find the end of the entries on the menu already
	while( head->next )
		head = head->next;

	while( count-- )
	{
		new_entry->selected	= 0;
		new_entry->next		= head->next;
		new_entry->prev		= head;
		head->next		= new_entry;

		head			= new_entry;
		new_entry++;
	}
	give_semaphore( menu_sem );
#else
	// Maybe later...
	struct menu_entry * child = head->child;
	if( !child )
	{
		// No other child entries; add this one
		// and select it
		new_entry->highlighted	= 1;
		new_entry->prev		= NULL;
		new_entry->next		= NULL;
		head->child		= new_entry;
		return;
	}

	// Walk the child list to find the end
	while( child->next )
		child = child->next;

	// Push the new entry onto the end of the list
	new_entry->selected	= 0;
	new_entry->prev		= child;
	new_entry->next		= NULL;
	child->next		= new_entry;
#endif
}

static void batsu(int x, int y)
{
	int i;
	for (i = 1; i < 5; i++)
	{
		draw_line(x + 5 + i, y + 5, x + 22 + i, y + 22, COLOR_RED);
		draw_line(x + 22 + i, y + 5, x + 5 + i, y + 22, COLOR_RED);
	}
}

void maru(int x, int y, int color)
{
	int r;
	for (r = 0; r < 10; r++)
	{
		draw_circle(x + 16, y + 16, r, color);
		draw_circle(x + 17, y + 16, r, color);
	}
}

static void percent(int x, int y, int value)
{
	int i;
	y -= 2;
	value = value * 28 / 100;
	for (i = 0; i < 28; i++)
		draw_line(x + 2 + i, y + 30, x + 2 + i, y + 30 - i,  i <= value ? 9 : 60);
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

// By default, icon type is MNI_BOOL(*(int*)priv)
// To override, call menu_draw_icon from the display functions

// Icon is only drawn once for each menu item, even if this is called multiple times
// Only the first call is executed

int icon_drawn = 0;
void menu_draw_icon(int x, int y, int type, int arg)
{
	#if !CONFIG_DEBUGMSG
	if (icon_drawn) return;
	icon_drawn = 1;
	x -= 40;
	if (type >= 0) bmp_printf(FONT_LARGE, x, y, "  "); // cleanup background
	warning_msg = 0;
	switch(type)
	{
		case MNI_OFF: batsu(x, y); return;
		case MNI_ON: maru(x, y, COLOR_GREEN1); return;
		case MNI_WARNING: maru(x, y, COLOR_RED); warning_msg = arg; return;
		case MNI_AUTO: maru(x, y, 9); return;
		case MNI_PERCENT: percent(x, y, arg); return;
		case MNI_ACTION: playicon(x, y); return;
	}
	#endif
}

static int
menu_has_visible_items(struct menu_entry *	menu)
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
	struct menu_entry *	menu,
	int			x,
	int			y,
	int			selected
)
{
	while( menu )
	{
		if (advanced_mode || IS_ESSENTIAL(menu))
		{
			icon_drawn = 0;
			
			if (!show_only_selected || menu->selected)
			{
				menu->display(
					menu->priv,
					x,
					y,
					menu->selected
				);
			}
			
			// this should be after menu->display, in order to allow it to override the icon
			if (menu->priv && !show_only_selected)
			{
				menu_draw_icon(x, y, MNI_BOOL(*(int*)menu->priv), 0);
			}
			
			if (menu->selected && menu->help && !show_only_selected)
				bmp_printf(
					FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 
					x0 + 10 /* + ((700/font_med.width) - strlen(menu->help)) * font_med.width / 2*/, y0 + 450, 
					menu->help
				);
			
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

			y += font_large.height - 1;
			
			if (y > vram_bm.height - font_large.height) return;
		}
		menu = menu->next;
	}
}


static void
menus_display(
	struct menu *		menu,
	int			orig_x,
	int			y
)
{
	int			x = orig_x;

	take_semaphore( menu_sem, 0 );

	extern int override_zoom_buttons; // from focus.c
	override_zoom_buttons = 0; // will override them only if rack focus items are selected

	if (!show_only_selected)
		bmp_printf(
			FONT(FONT_MED, 55, COLOR_BLACK), // gray
			x0 + 10, y0 + 430, 
				MENU_NAV_HELP_STRING
		);

	bmp_fill(40, orig_x, y, 720, 42);
	bmp_fill(70, orig_x, y+42, 720, 1);
	for( ; menu ; menu = menu->next )
	{
		if (!menu_has_visible_items(menu->children))
			continue; // empty menu
		
		int fg = menu->selected ? COLOR_WHITE : 70;
		int bg = menu->selected ? 13 : 40;
		unsigned fontspec = FONT(
			menu->selected ? FONT_LARGE : FONT_MED,
			fg,
			bg
		);
		if (!show_only_selected)
		{
			int w = fontspec_font( fontspec )->width * 6 + 10;
			int h = fontspec_font( fontspec )->height;
			int icon_w = 0;
			if (menu->icon)
			{
				bmp_fill(bg, x, y, 200, 40);
				if (menu->icon == ICON_ML_PLAY) icon_w = playicon_square(x,y,fg);
				else icon_w = bfnt_draw_char(menu->icon, x, y, fg, bg);
			}
			if (!menu->icon || menu->selected)
			{
				bfnt_puts(menu->name, x + icon_w + 5, y, fg, bg);
				//~ bmp_printf( fontspec, x + icon_w + 5, y + (40 - h)/2, "%6s", menu->name );
				x += w;
			}
			x += 49;
			//~ if (menu->selected)
			//~ {
				//~ bmp_printf( FONT(FONT_LARGE,fg,40), orig_x + 700 - font_large.width * strlen(menu->name), y + 4, menu->name );
			//~ }
		}

		if( menu->selected )
			menu_display(
				menu->children,
				orig_x + 40,
				y + 45,
				1
			);
	}
	give_semaphore( menu_sem );
}


static void
menu_entry_select(
	struct menu *	menu,
	int mode // 0 = normal, 1 = reverse, 2 = auto setting
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

	if(mode == 1)
	{
		if( entry->select_reverse ) entry->select_reverse( entry->priv );
		else if (entry->select) entry->select( entry->priv );
	}
	else if (mode == 2)
	{
		if( entry->select_auto ) entry->select_auto( entry->priv );
		else if (entry->select) entry->select( entry->priv );
	}
	else 
	{
		if( entry->select ) entry->select( entry->priv );
	}
	
	config_dirty = 1;
}

/** Scroll side to side in the list of menus */
static void
menu_move(
	struct menu *		menu,
	int			direction
)
{
	//~ menu_damage = 1;

	if( !menu )
		return;

	int rc = take_semaphore( menu_sem, 1000 );
	if( rc != 0 )
		return;

	// Deselect the current one
	menu->selected		= 0;

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
	menu->selected		= 1;
	give_semaphore( menu_sem );
	
	if (!menu_has_visible_items(menu->children))
		menu_move(menu, direction); // this menu is hidden, skip it (try again)
		// will fail if no menus are displayed!
}


/** Scroll up or down in the currently displayed menu */
static void
menu_entry_move(
	struct menu *		menu,
	int			direction
)
{
	if( !menu )
		return;

	int rc = take_semaphore( menu_sem, 1000 );
	if( rc != 0 )
		return;
	
	if (!menu_has_visible_items(menu->children))
		return;

	struct menu_entry *	entry = menu->children;

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

static void menu_select_current(int reverse)
{
	struct menu * menu = menus;
	for( ; menu ; menu = menu->next )
		if( menu->selected )
			break;
	menu_entry_select(menu,reverse);
}

static void 
menu_redraw_if_damaged()
{
	//~ if( menu_damage )
	{
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
				// draw to mirror buffer to avoid flicker
				//~ bmp_mirror_copy(0); // no need, drawing is fullscreen anyway
				bmp_draw_to_mirror(1);
				
				bmp_fill( show_only_selected ? 0 : COLOR_BLACK, 0, 0, 960, 540 ); 
				menus_display( menus, x0, y0 ); 
				if (is_menu_active("Help")) menu_show_version();
				//~ draw_ml_topbar();

				if (show_only_selected) 
				{
					draw_ml_topbar();
					draw_ml_bottombar(0);
				}

				// copy image to main buffer
				bmp_draw_to_mirror(0);
				int screen_layout = get_screen_layout();
				if (hdmi_code == 2) // copy at a smaller scale to fit the screen
				{
					if (screen_layout == SCREENLAYOUT_16_10)
						bmp_zoom(bmp_vram(), get_bvram_mirror(), x0 + 360, y0 + 150, /* 128 div */ 143, /* 128 div */ 169);
					else if (screen_layout == SCREENLAYOUT_16_9)
						bmp_zoom(bmp_vram(), get_bvram_mirror(), x0 + 360, y0 + 150, /* 128 div */ 143, /* 128 div */ 185);
					else
						bmp_mirror_copy(1);
				}
				else if (ext_monitor_rca)
					bmp_zoom(bmp_vram(), get_bvram_mirror(), x0 + 360, y0 + 200, /* 128 div */ 135, /* 128 div */ 135);
				else
					bmp_mirror_copy(1);
				bvram_mirror_clear();
			)
			//~ update_stuff();

			update_disp_mode_bits_from_params();
		}
	}
}

void menu_send_event(int event)
{
	ctrlman_dispatch_event(gui_menu_task, event, 0, 0);
}

static struct menu * get_selected_menu()
{
	struct menu * menu = menus;
	for( ; menu ; menu = menu->next )
		if( menu->selected )
			break;
	return menu;
}

static int
menu_handler(
	void *			priv,
	gui_event_t		event,
	int			arg2,
	int			arg3,
	unsigned		arg4
)
{
#if CONFIG_DEBUGMSG
	if( event > 1 && event < 0x10000000)
	{
		bmp_printf( FONT_SMALL, 400, 40,
			"evt %8x(%8x,%8x,%8x)",
			event, arg2, arg3, arg4
		);
	}
#endif

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
	
	switch( event )
	{
	case INITIALIZE_CONTROLLER:
		//~ NotifyBox(2000, "INITIALIZE_CONTROLLER");
		return 0;

	case GOT_TOP_OF_CONTROL:
		//~ NotifyBox(2000, "GOT_TOP_OF_CONTROL");
		menu_redraw_if_damaged();
		return 0;

	case LOST_TOP_OF_CONTROL:
		//~ NotifyBox(2000, "LOST_TOP_OF_CONTROL");
		gui_stop_menu();
		return 0;

	case TERMINATE_WINSYS:
		// Must propagate to all gui elements
		//~ NotifyBox(2000, "TERMINATE_WINSYS");
		gui_stop_menu();
		return 1;

	case DELETE_DIALOG_REQUEST:
		// Must not propagate
		//~ NotifyBox(2000, "DELETE_DIALOG_REQUEST");
		gui_stop_menu();
		return 0;


	case PRESS_MENU_BUTTON:
		advanced_mode = !advanced_mode;
		show_only_selected = 0;
		menu_help_active = 0;
		edit_mode = 0;
		break;

	case EVENTID_METERING_START: // If they press the shutter halfway
		gui_stop_menu();
		return 1;
	
	case EVENTID_94:
		// Generated when buttons are pressed?  Forward it on
		return 1;
	
	case PRESS_ZOOM_IN_BUTTON:
		edit_mode = !edit_mode;
		//~ menu_damage = 1;
		menu_help_active = 0;
		break;

#ifdef CONFIG_50D
	case PRESS_JOY_UP:
#else
	case PRESS_UP_BUTTON:
#endif
		edit_mode = 0;
	case ELECTRONIC_SUB_DIAL_LEFT:
		//~ menu_damage = 1;
		show_only_selected = 0;
		if (menu_help_active) { menu_help_prev_page(); break; }
		if (edit_mode) { int i; for (i = 0; i < 5; i++) { menu_entry_select( menu, 1 ); msleep(10); }}
		else menu_entry_move( menu, -1 );
		break;

#ifdef CONFIG_50D
	case PRESS_JOY_DOWN:
#else
	case PRESS_DOWN_BUTTON:
#endif
		edit_mode = 0;
	case ELECTRONIC_SUB_DIAL_RIGHT:
		//~ menu_damage = 1;
		show_only_selected = 0;
		if (menu_help_active) { menu_help_next_page(); break; }
		if (edit_mode) { int i; for (i = 0; i < 5; i++) { menu_entry_select( menu, 0 ); msleep(10); }}
		else menu_entry_move( menu, 1 );
		break;

#ifdef CONFIG_50D
	case PRESS_JOY_RIGHT:
#else
	case PRESS_RIGHT_BUTTON:
#endif
		edit_mode = 0;
	case DIAL_RIGHT:
		//~ menu_damage = 1;
		show_only_selected = 0;
		if (menu_help_active) { menu_help_next_page(); break; }
		if (edit_mode) menu_entry_select( menu, 0 );
		else menu_move( menu, 1 );
		break;

#ifdef CONFIG_50D
	case PRESS_JOY_LEFT:
#else
	case PRESS_LEFT_BUTTON:
#endif
		edit_mode = 0;
	case DIAL_LEFT:
		//~ menu_damage = 1;
		show_only_selected = 0;
		if (menu_help_active) { menu_help_prev_page(); break; }
		if (edit_mode) menu_entry_select( menu, 1 );
		else menu_move( menu, -1 );
		break;

	case PRESS_SET_BUTTON:
		if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
		else
		{
			edit_mode = 1;
			#ifdef CONFIG_60D
			if (lv) edit_mode = 0; // in LiveView, UNPRESS SET event is not sent => can't detect when SET is being held
			#endif
			menu_entry_select( menu, 0 ); // normal select
		}
		//~ menu_damage = 1;
		break;
	case UNPRESS_SET_BUTTON:
		edit_mode = 0;
		break;

	case PRESS_INFO_BUTTON:
		menu_help_active = !menu_help_active;
		show_only_selected = 0;
		if (menu_help_active) menu_help_go_to_selected_entry(menu);
		//~ menu_damage = 1;
		break;

    case PRESS_PLAY_BUTTON:
		if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
		menu_entry_select( menu, 1 ); // reverse select
		//~ menu_damage = 1;
		break;

	case PRESS_DIRECT_PRINT_BUTTON:
#ifdef CONFIG_50D
	case PRESS_FUNC_BUTTON:
	case JOY_CENTER:
#endif
		if (menu_help_active) { menu_help_active = 0; /* menu_damage = 1; */ break; }
		menu_entry_select( menu, 2 ); // auto setting select
		//~ menu_damage = 1;
		break;

#ifdef CONFIG_50D
	case PRESS_JOY_LEFTUP:
	case PRESS_JOY_LEFTDOWN:
	case PRESS_JOY_RIGHTUP:
	case PRESS_JOY_RIGHTDOWN:
		break; // ignore
#endif

	case EVENT_1:          // Synthetic redraw event
		break;

	default:
		DebugMsg( DM_MAGIC, 3, "%s: unknown event %08x? %08x %08x %x08",
			__func__,
			event,
			arg2,
			arg3,
			arg4
		);
		return 1;
	}

	// If we end up here, something has been changed.
	// Reset the timeout
	menu_timeout = menu_timeout_time;

	// If we are hidden or no longer exit, do not redraw
	if( menu_hidden || !gui_menu_task )
		return 0;

	menu_redraw_if_damaged();

	return 0;
}





void
menu_init( void )
{
	menus = NULL;
	gui_menu_task = NULL;
	menu_sem = create_named_semaphore( "menus", 1 );
	gui_sem = create_named_semaphore( "gui", 0 );

#if defined(CONFIG_550D) || defined(CONFIG_60D)
	menu_find_by_name( "Audio", ICON_MIC);
#endif
	menu_find_by_name( "LiveV", ICON_LV);
	menu_find_by_name( "Expo", ICON_AE);
#if defined(CONFIG_500D)
	menu_find_by_name( "Movie", ICON_FILM );
#endif
	menu_find_by_name( "Movie", ICON_VIDEOCAM );
	menu_find_by_name( "Shoot", ICON_PHOTOCAM );
	//~ menu_find_by_name( "Brack" );
	menu_find_by_name( "Focus", ICON_SHARPNESS );
	//~ menu_find_by_name( "LUA" );
	//menu_find_by_name( "Games" );
	menu_find_by_name( "Tweaks", ICON_SMILE );
	menu_find_by_name( "Play", ICON_ML_PLAY );
	menu_find_by_name( "Config", ICON_CF );
	menu_find_by_name( "Power", ICON_P_SQUARE );
	menu_find_by_name( "Debug", ICON_HEAD_WITH_RAYS );
	//~ menu_find_by_name( "Config" );
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


CONFIG_INT("guimode.ml.menu", guimode_ml_menu, 2);

static void
guimode_ml_menu_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
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

// this function should be called only from gui event handlers
void
gui_stop_menu( void )
{
	menu_hidden = 0;
	//~ menu_damage = 0;

	if( !gui_menu_task )
		return;

	gui_task_destroy( gui_menu_task );
	gui_menu_task = NULL;

	stop_killing_flicker_do();

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

	menu_shown = 0;

	if (!PLAY_MODE) {}//redraw_after(300);
	else draw_livev_for_playback();
}


void
gui_hide_menu(
	int			redisplay_time
)
{
	if (!menu_hidden)
		bmp_fill( 0, 0, 0, 720, 480 );
	menu_hidden = redisplay_time;
	//~ menu_damage = 1;
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
/*
static void
about_print_0(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (!selected) return;
	show_logo();
}

static void
about_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	y -= font_large.height;
	if (!selected) return;
	
}*/


/*static struct menu_entry draw_prop_menus[] = {
	{
		.priv		= "Toggle draw-event",
		.display	= menu_print,
		.select		= toggle_draw_event,
	},
};*/

static struct menu_entry dbg_menu[] = {
	{
		.priv = &guimode_ml_menu,
		.display = guimode_ml_menu_print,
		.select = guimode_ml_menu_inc,
		.select_reverse = guimode_ml_menu_dec,
	},
};


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

static void
menu_task( void* unused )
{
	//~ int x, y;
	DebugMsg( DM_MAGIC, 3, "%s: Starting up\n", __func__ );

	// Add the draw_prop menu
	#if 0
	menu_add( "Debug", dbg_menu, COUNT(dbg_menu) );
	#endif
	//~ menu_add( " (i)", about_menu, COUNT(about_menu));
	
	msleep(500);
	while(1)
	{
		int rc = take_semaphore( gui_sem, 250 );
		if( rc != 0 )
		{
			// We woke up after 1 second
			if( !gui_menu_task )
				continue;

			// Count down the menu timeout
			if( --menu_timeout == 0 )
			{
				gui_stop_menu();
				continue;
			}

			// Count down the menu_hidden timer
			if( menu_hidden )
			{
				if( --menu_hidden != 0 )
					continue;
				// Force an update on timer expiration
				ctrlman_dispatch_event(
					gui_menu_task,
					GOT_TOP_OF_CONTROL,
					0,
					0
				);
			} else if (!menu_help_active && !show_only_selected) {
				// Inject a synthetic timing event
				ctrlman_dispatch_event(
					gui_menu_task,
					1,
					0,
					0
				);
			}

			continue;
		}

		if( gui_menu_task )
		{
			gui_stop_menu();
			continue;
		}
		
		if (recording && !lv) continue;
		
		menu_shown = 1;
		
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
		idle_kill_flicker();
		bmp_on();

		x0 = hdmi_code == 5 ? 120 : 0;
		y0 = hdmi_code == 5 ? 40 : 0;

		DebugMsg( DM_MAGIC, 3, "Creating menu task" );
		//~ menu_damage = 1;
		menu_hidden = 0;
		edit_mode = 0;
		menu_help_active = 0;
		gui_menu_task = gui_task_create( menu_handler, 0 );

		//~ zebra_pause();
		//~ display_on(); // ensure the menu is visible even if display was off
		//~ bmp_on();
		show_only_selected = 0;
	}
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1d, 0x1000 );

int is_menu_active(char* name)
{
	if (!gui_menu_task) return 0;
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
			struct menu_entry *	entry = menu->children;
			
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
			struct menu_entry *	entry = menu->children;
			
			int i;
			for(i = 0 ; entry ; entry = entry->next, i++ )
				entry->selected = !strcmp(entry->name, entry_name);
		}
	}
	//~ menu_damage = 1;
}

static void
menu_help_go_to_selected_entry(
	struct menu *	menu
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
	void *			priv,
	int			x,
	int			y,
	int			selected
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

	if (event->param == BGMT_MENU && PLAY_MODE)
	{
		give_semaphore( gui_sem );
		return 0;
	}

	return 1;
}

// this can be called from any task
void menu_stop()
{
	if (gui_menu_shown())
		give_semaphore( gui_sem );
}

#ifndef CONFIG_50D
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
