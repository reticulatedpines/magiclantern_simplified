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
static int menu_damage;
static int menu_hidden;
static int menu_timeout;
static int menu_shown = 0;
static int show_only_selected; // for ISO, kelvin...
static int edit_mode = 0;
static int config_dirty = 0;
int menu_help_active = 0;

int is_menu_help_active() { return gui_menu_shown() && menu_help_active; }

int get_menu_font_sel() 
{
	if (edit_mode) return FONT(FONT_LARGE,COLOR_WHITE,COLOR_RED);
	else return FONT(FONT_LARGE,COLOR_WHITE,13);
}

extern int gui_state;
void menu_show_only_selected()
{
	show_only_selected = 1;
	menu_damage = 1;
}

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
	const char *		name
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
	struct menu *		menu = menu_find_by_name( name );
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

static void maru(int x, int y, int color)
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
	switch(type)
	{
		case MNI_OFF: batsu(x, y); return;
		case MNI_ON: maru(x, y, COLOR_GREEN1); return;
		case MNI_WARNING: maru(x, y, COLOR_RED); return;
		case MNI_AUTO: maru(x, y, 9); return;
		case MNI_PERCENT: percent(x, y, arg); return;
		case MNI_ACTION: playicon(x, y); return;
	}
	#endif
}


void
menu_display(
	struct menu_entry *	menu,
	int			x,
	int			y,
	int			selected
)
{
	while( menu )
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
		if (menu->priv)
		{
			menu_draw_icon(x, y, MNI_BOOL(*(int*)menu->priv), 0);
		}
		
		if (menu->selected && menu->help)
			bmp_printf(
				FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 
				10 /* + ((700/font_med.width) - strlen(menu->help)) * font_med.width / 2*/, 450, 
				menu->help
			);

		y += font_large.height;
		menu = menu->next;
	}
}


void
menus_display(
	struct menu *		menu,
	int			orig_x,
	int			y
)
{
	int			x = orig_x;

	take_semaphore( menu_sem, 0 );

	for( ; menu ; menu = menu->next )
	{
		unsigned fontspec = FONT(
			FONT_MED,
			menu->selected ? COLOR_WHITE : COLOR_YELLOW,
			menu->selected ? 13 : COLOR_BLACK
		);
		if (!show_only_selected) bmp_printf( fontspec, x, y, "%6s", menu->name );
		x += fontspec_font( fontspec )->width * 6;

		if( menu->selected )
			menu_display(
				menu->children,
				orig_x + 40,
				y + fontspec_font( fontspec )->height + 4,
				1
			);
	}

	give_semaphore( menu_sem );
}


void
menu_entry_select(
	struct menu *	menu,
	int mode // 0 = normal, 1 = reverse, 2 = auto setting
)
{
	if( !menu )
		return;

	show_only_selected = 0;
	menu_help_active = 0;
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
void
menu_move(
	struct menu *		menu,
	int			direction
)
{
	menu_damage = 1;

	if( !menu )
		return;

	show_only_selected = 0;
	menu_help_active = 0;
	int rc = take_semaphore( menu_sem, 100 );
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
}


/** Scroll up or down in the currently displayed menu */
void
menu_entry_move(
	struct menu *		menu,
	int			direction
)
{
	if( !menu )
		return;

	show_only_selected = 0;
	menu_help_active = 0;
	int rc = take_semaphore( menu_sem, 100 );
	if( rc != 0 )
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
}

void menu_select_current(int reverse)
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
	if( menu_damage )
	{
		if (menu_help_active)
		{
			menu_help_redraw();
			menu_damage = 0;
		}
		else
		{
			if (!lv) show_only_selected = 0;
			//~ if (MENU_MODE || lv) clrscr();
			bmp_fill( show_only_selected ? 0 : COLOR_BLACK, 0, 0, 720, 480 );
			menu_damage = 0;
			BMP_SEM( menus_display( menus, 10, 40 ); )
			update_stuff();
			update_disp_mode_bits_from_params();
		}
	}
}

void menu_send_event(int event)
{
	ctrlman_dispatch_event(gui_menu_task, event, 0, 0);
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
	static int k = 0;
	// Ignore periodic events (pass them on)
	if( 0
	||  event == GUI_TIMER2
	||  event == GUI_TIMER3
	||  event == GUI_TIMER4
	||  event == 0x1000007c
	||  event == 0x10000078
	)
		return 1; // 0 is too aggressive :)

#if 0
	if( event > 1 && event < 0x10000000)
	{
		bmp_printf( FONT_SMALL, 400, 40,
			"evt %8x(%8x,%8x,%8x)",
			event, arg2, arg3, arg4
		);
	}
#endif

		// Mine!  No one else gets it
		//~ return 0;
	//~ }

	//~ if( event != 1 )
	//~ {
		DebugMsg( DM_MAGIC, 3, "%s: event %x", __func__, event );
		if( draw_event )
        {
			bmp_printf( FONT_SMALL, 20, 10 + ((k) % 8) * 10, "EVENT%2d: %x args %8x/%8x; %8x/%8x; %8x/%8x", k % 100, event, arg2, arg2 ? (*(int*)arg2) : 0, arg3, arg3 ? (*(int*)arg3) : 0, arg4, arg4 ? (*(int*)arg4) : 0);
			bmp_printf( FONT_SMALL, 20, 10 + ((k+1) % 8) * 10, "                                             ");
            k += 1;
            // not dangerous any more :)
			//~ if (event != PRESS_LEFT_BUTTON && event != PRESS_RIGHT_BUTTON && event != PRESS_UP_BUTTON && event != PRESS_DOWN_BUTTON && event != PRESS_SET_BUTTON) return 0;
        }
	//~ }

	// Find the selected menu (should be cached?)
	struct menu * menu = menus;
	for( ; menu ; menu = menu->next )
		if( menu->selected )
			break;
	switch( event )
	{
	case INITIALIZE_CONTROLLER:
		DebugMsg( DM_MAGIC, 3, "Menu task INITIALIZE_CONTROLLER" );
		return 0;

	case GOT_TOP_OF_CONTROL:
		DebugMsg( DM_MAGIC, 3, "Menu task GOT_TOP_OF_CONTROL" );
		menu_damage = 1;
		menu_redraw_if_damaged();
		menu_damage = 1;
		break;
	case LOST_TOP_OF_CONTROL:
		gui_stop_menu();

	case TERMINATE_WINSYS:
		// Must propagate to all gui elements
		DebugMsg( DM_MAGIC, 3, "%s: TERMINATE_WINSYS", __func__ );
		gui_stop_menu();
		return 1;

	case DELETE_DIALOG_REQUEST:
		// Must not propagate
		DebugMsg( DM_MAGIC, 3, "%s: DELETE_DIALOG", __func__ );
		gui_stop_menu();
		return 0;

	case PRESS_MENU_BUTTON:
	case EVENTID_METERING_START: // If they press the shutter halfway
	case 0x10000048:
	case 0x10000062:
		gui_stop_menu();
		return 1;
	
	case EVENTID_94:
		// Generated when buttons are pressed?  Forward it on
		return 1;
	
	case PRESS_ZOOM_IN_BUTTON:
		edit_mode = !edit_mode;
		menu_damage = 1;
		menu_help_active = 0;
		break;

	case PRESS_UP_BUTTON:
		edit_mode = 0;
	case ELECTRONIC_SUB_DIAL_LEFT:
		menu_damage = 1;
		if (menu_help_active) { menu_help_prev_page(); break; }
		if (edit_mode) { int i; for (i = 0; i < 5; i++) { menu_entry_select( menu, 1 ); msleep(10); }}
		else menu_entry_move( menu, -1 );
		break;

	case PRESS_DOWN_BUTTON:
		edit_mode = 0;
	case ELECTRONIC_SUB_DIAL_RIGHT:
		menu_damage = 1;
		if (menu_help_active) { menu_help_next_page(); break; }
		if (edit_mode) { int i; for (i = 0; i < 5; i++) { menu_entry_select( menu, 0 ); msleep(10); }}
		else menu_entry_move( menu, 1 );
		break;

	case DIAL_RIGHT:
	case PRESS_RIGHT_BUTTON:
		menu_damage = 1;
		if (menu_help_active) { menu_help_next_page(); break; }
		if (edit_mode) menu_entry_select( menu, 0 );
		else menu_move( menu, 1 );
		break;

	case DIAL_LEFT:
	case PRESS_LEFT_BUTTON:
		menu_damage = 1;
		if (menu_help_active) { menu_help_prev_page(); break; }
		if (edit_mode) menu_entry_select( menu, 1 );
		else menu_move( menu, -1 );
		break;

	case PRESS_SET_BUTTON:
		if (menu_help_active) { menu_help_active = 0; menu_damage = 1; break; }
		if (edit_mode) edit_mode = 0;
		else menu_entry_select( menu, 0 ); // normal select
		menu_damage = 1;
		break;

	case PRESS_INFO_BUTTON:
		menu_help_active = !menu_help_active;
		if (menu_help_active) menu_help_go_to_selected_entry(menu);
		menu_damage = 1;
		break;

    case PRESS_PLAY_BUTTON:
		if (menu_help_active) { menu_help_active = 0; menu_damage = 1; break; }
		menu_entry_select( menu, 1 ); // reverse select
		menu_damage = 1;
		break;

	case PRESS_DIRECT_PRINT_BUTTON:
		if (menu_help_active) { menu_help_active = 0; menu_damage = 1; break; }
		menu_entry_select( menu, 2 ); // auto setting select
		menu_damage = 1;
		break;

#if 0
	case PRESS_ZOOM_IN_BUTTON:
		gui_hide_menu( 100 );
		lens_focus_start( 0 );
		break;


#if 0
	// This breaks playback if enabled; figure out why!
	case PRESS_ZOOM_OUT_BUTTON:
		gui_hide_menu( 100 );
		lens_focus_start( -1 );
		break;
#endif

	case UNPRESS_ZOOM_IN_BUTTON:
	//case UNPRESS_ZOOM_OUT_BUTTON:
		gui_hide_menu( 2 );
		lens_focus_stop();
		break;
#endif

	case EVENT_1:          // Synthetic redraw event
		break;

	//~ case 0x10000097: // canon code might have drawn over menu
	case 0x100000e8: // when you press Q on ISO
		menu_damage = 1;
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

	menu_find_by_name( "Audio" );
	menu_find_by_name( "LiveV" );
	menu_find_by_name( "Movie" );
	menu_find_by_name( "Shoot" );
	menu_find_by_name( "Expo" );
	//~ menu_find_by_name( "Brack" );
	menu_find_by_name( "Focus" );
	//~ menu_find_by_name( "LUA" );
	//menu_find_by_name( "Games" );
	menu_find_by_name( "Tweak" );
	menu_find_by_name( "Debug" );
	menu_find_by_name( "Config" );
	menu_find_by_name( " (i)" );
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


void
gui_stop_menu( void )
{
	menu_hidden = 0;
	menu_damage = 0;

	if( !gui_menu_task )
		return;
	
	//~ while (gui_menu_task == 1) msleep(100);

	gui_task_destroy( gui_menu_task );
	gui_menu_task = NULL;

	//workaround, otherwise screen does not refresh after closing menu
	/*if (!lv)
	{
		while (get_halfshutter_pressed()) msleep(100);
		fake_simple_button(BGMT_Q);
	}*/
	
	lens_focus_stop();
	show_only_selected = 0;
	//~ powersave_set_config_for_menu(); // revert to your preferred setting for powersave

	extern int config_autosave;
	if (config_autosave && config_dirty)
	{
		save_config(0);
		config_dirty = 0;
	}

	if (MENU_MODE && !get_halfshutter_pressed())
	{
		fake_simple_button(BGMT_MENU);
	}
	else
	{
		redraw();
	}

	menu_shown = 0;
}


void
gui_hide_menu(
	int			redisplay_time
)
{
	menu_hidden = redisplay_time;
	menu_damage = 1;
	bmp_fill( 0, 0, 0, 720, 480 );
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
/*
static struct menu_entry about_menu[] = {
	{
		.display = about_print_0
	},
	{
		.display = about_print
	}
};*/

static void
open_canon_menu()
{
	while(1)
	{
		fake_simple_button(BGMT_MENU);
		#ifdef CONFIG_600D
		msleep(500);
		return;
		#endif
		int i;
		for (i = 0; i < 10; i++)
		{
			if (MENU_MODE) return;
			msleep(100);
		}
	}
}

static void
menu_task( void* unused )
{
	//~ int x, y;
	DebugMsg( DM_MAGIC, 3, "%s: Starting up\n", __func__ );

	// Add the draw_prop menu
	//~ menu_add( "Debug", draw_prop_menus, COUNT(draw_prop_menus) );
	//~ menu_add( " (i)", about_menu, COUNT(about_menu));
	
	msleep(500);
	while(1)
	{
		int rc = take_semaphore( gui_sem, 500 );
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
			} else {
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
		
		menu_shown = 1;
		
		if (!lv && !MENU_MODE)
		{
			open_canon_menu();
		}
		msleep(100);
		
		DebugMsg( DM_MAGIC, 3, "Creating menu task" );
		menu_damage = 1;
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

TASK_CREATE( "menu_task", menu_task, 0, 0x1e, 0x1000 );

int is_menu_active(char* name)
{
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
				entry->selected = !strcmp(menu->name, name);
		}
	}
}

void
menu_help_go_to_selected_entry(
	struct menu *	menu
)
{
	if( !menu )
		return;

	struct menu_entry * entry = menu->children;

	for( ; entry ; entry = entry->next )
	{
		if( entry->selected )
			break;
	}
	
	menu_help_go_to_label(entry->name);
}
