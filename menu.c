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

CONFIG_INT( "debug.draw-event", draw_event, 0 );
CONFIG_INT( "debug.menu-timeout", menu_timeout_time, 15 );

static void
draw_version( void )
{
	bmp_printf(
		FONT( FONT_SMALL, COLOR_WHITE, COLOR_BLUE ),
		0, 32,
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
	struct menu * new_menu = malloc( sizeof(*new_menu) );
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
		menu->display(
			menu->priv,
			x,
			y,
			menu->selected
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
			COLOR_YELLOW,
			menu->selected ? 0x7F : COLOR_BG
		);
		bmp_printf( fontspec, x, y, "%6s", menu->name );
		x += fontspec_font( fontspec )->width * 6;

		if( menu->selected )
			menu_display(
				menu->children,
				orig_x,
				y + fontspec_font( fontspec )->height + 4,
				1
			);
	}

	give_semaphore( menu_sem );
}


void
menu_entry_select(
	struct menu *	menu
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

	if( !entry || !entry->select )
		return;

	entry->select( entry->priv );
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


static int
menu_handler(
	void *			priv,
	gui_event_t		event,
	int			arg2,
	int			arg3,
	unsigned		arg4
)
{
	// Ignore periodic events (pass them on)
	if( 0
	||  event == GUI_TIMER2
	||  event == GUI_TIMER3
	||  event == GUI_TIMER4
	)
		return 0;

	if( event == GUI_PROP_EVENT )
	{
		if(0) bmp_printf( FONT_SMALL, 400, 40,
			"prop %08x => %08x",
			arg4,
			*(unsigned*) arg4
		);

		// Mine!  No one else gets it
		return 0;
	}

	if( event != 1 )
	{
		if( draw_event )
			bmp_printf( FONT_SMALL, 400, 40,
				"event %08x args %08x %08x %08x",
				event,
				arg2,
				arg3,
				arg4
			);

		DebugMsg( DM_MAGIC, 3, "%s: event %x", __func__, event );
	}


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
		break;

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
		gui_stop_menu();
		return 1;

	case JOY_CENTER:
		// We don't process it, but dont' let anyone else, either
		return 0;

	case EVENTID_94:
		// Generated when buttons are pressed?  Forward it on
		return 1;

	case PRESS_JOY_UP:
	case ELECTRONIC_SUB_DIAL_LEFT:
		menu_entry_move( menu, -1 );
		menu_damage = 1;
		break;

	case PRESS_JOY_DOWN:
	case ELECTRONIC_SUB_DIAL_RIGHT:
		menu_entry_move( menu, 1 );
		menu_damage = 1;
		break;

	case PRESS_JOY_RIGHT:
	case DIAL_RIGHT:
		menu_move( menu, 1 );
		break;

	case PRESS_JOY_LEFT:
	case DIAL_LEFT:
		menu_move( menu, -1 );
		break;

	case PRESS_SET_BUTTON:
		menu_entry_select( menu );
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

	case 1:
		// Synthetic redraw event
		break;

	default:
		// We consume any unknown events
		DebugMsg( DM_MAGIC, 3, "%s: unknown event %x", __func__, event );
		return 0;
	}

	// If we end up here, something has been changed.
	// Reset the timeout
	menu_timeout = menu_timeout_time;

	// If we are hidden or no longer exit, do not redraw
	if( menu_hidden || !gui_menu_task )
		return 0;

	if( menu_damage )
		bmp_fill( COLOR_BG, 90, 90, 720-160, 480-180 );
	menu_damage = 0;
	menus_display( menus, 100, 100 );

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
	menu_find_by_name( "Video" );
	menu_find_by_name( "Brack" );
	menu_find_by_name( "Focus" );
	//menu_find_by_name( "Games" );
	menu_find_by_name( "Debug" );

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

	gui_task_destroy( gui_menu_task );
	gui_menu_task = NULL;
	bmp_fill( 0, 90, 90, 720-160, 480-180 );
}


void
gui_hide_menu(
	int			redisplay_time
)
{
	menu_hidden = redisplay_time;
	menu_damage = 1;
	bmp_fill( 0, 90, 90, 720-160, 480-180 );
}


int
gui_menu_shown( void )
{
	return (int) gui_menu_task;
}


static void
toggle_draw_event( void * priv )
{
	draw_event = !draw_event;
}

static struct menu_entry draw_prop_menus[] = {
	{
		.priv		= "Toggle draw-event",
		.display	= menu_print,
		.select		= toggle_draw_event,
	},
};


static void
menu_task( void )
{
	int x, y;
	DebugMsg( DM_MAGIC, 3, "%s: Starting up\n", __func__ );

	// Add the draw_prop menu
	menu_add( "Debug", draw_prop_menus, COUNT(draw_prop_menus) );

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

		DebugMsg( DM_MAGIC, 3, "Creating menu task" );
		menu_damage = 1;
		menu_hidden = 0;
		gui_menu_task = gui_task_create( menu_handler, 0 );
	}
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1e, 0x1000 );
