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


static unsigned last_menu_event;
static struct gui_task * menu_task_ptr;
static struct menu * menus;


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
	struct menu *		menu = menus;

	for( ; menu ; menu = menu->next )
	{
		if( streq( menu->name, name ) )
			return menu;

		// Stop just before we get to the end
		if( !menu->next )
			break;
	}

	// Not found; create it
	struct menu * new_menu = malloc( sizeof(*new_menu) );
	if( !new_menu )
		return NULL;

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



void enable_full_hd( void * priv )
{
	DebugMsg( DM_MAGIC, 3, "Attempting to set HDMI to full HD" );

	thunk ChangeHDMIOutputSizeToFULLHD = (thunk) 0xFFA96260;
	void (*SetDisplayType)(int) = (void*) 0xFF8620DC;

	SetDisplayType( 3 );
	ChangeHDMIOutputSizeToFULLHD();

	DebugMsg( DM_MAGIC, 3, "Full HD done?" );
}

void call_dispcheck( void * priv )
{
	call( "dispcheck" );
}


struct property {
	unsigned	prop;
	unsigned	len;
	uint32_t	data[ 6 ]; // make it an even 32 bytes
};

#define MAX_PROP_LOG 1024
struct property prop_log[ MAX_PROP_LOG ];
int prop_head = 0;

void prop_log_display( void * priv, int x, int y, int selected )
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Dump prop log %04x",
		prop_head
	);
}

void prop_log_select( void * priv )
{
	write_debug_file(
		"property.log",
		prop_log,
		sizeof(prop_log[0])*prop_head
	);
	prop_head = 0;
}


static unsigned efic_temp;
static void * efic_temp_token;

static void
efic_temp_token_handler(
	void *			token,
	void *			arg1,
	void *			arg2,
	void *			arg3
)
{
	efic_temp_token = token;
	bmp_printf( FONT_SMALL, 100, 100,
		"args: %08x %08x %08x",
		(unsigned) arg1,
		(unsigned) arg2,
		(unsigned) arg3
	);
}
	

static void
efic_temp_property_handler(
	unsigned		property,
	void *			UNUSED( priv ),
	unsigned *		addr,
	unsigned		len
)
{
	efic_temp = *addr;
	prop_cleanup( efic_temp_token, property );
}


static void
efic_temp_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Sensor Temp %d",
		efic_temp
	);
}

static void
mvr_time_const_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	uint8_t * mvr_struct = (void*) 0x1ed4;
	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	struct state_object ** const mvr_state_object = (void*) 0x68a4;

	bmp_printf(
		FONT_MED, // selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"MVR %08x -> %08x %08x",
		(unsigned) *mvr_state_object,
		*(unsigned*)( 0x4c + (uintptr_t) mvr_state_object ),
		*(unsigned*)( 0x14c + (uintptr_t) mvr_state_object )
	);
}

static void
mvr_time_const_select( void * priv )
{
/*
	void (*mvr_set_time_const)(int *) = (void*) 0xff9716cc;
	void (*mvr_setd_fullhd)(int *) = (void*) 0xff9716cc;
	//int args[] = { 640, 480 };
	//DebugMsg( DM_MAGIC, 3, "calling mvr_setd_fullhd %d %d", args[0], args[1] );
	//mvr_setd_fullhd( args );

	uint32_t buf[] = { 8 };
	//prop_request_change( 0x207000c, buf, sizeof(buf) );
	void (*lv_magnify)( int * ) = (void*) 0xff83359c;
	lv_magnify( buf );
	void (*mvrSetBitRate)( int * ) = (void*) 0xff84f990;
	//int rate = 24;
	//mvrSetBitRate( &rate );
	mvr_struct->is_vga	= 0;
	mvr_struct->width	= 1920;
	mvr_struct->height	= 1080;
	mvr_struct->fps		= 24;
*/

	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	*(unsigned *)( mvr_hdr + 0x60 ) = 2400;
	*(unsigned *)( mvr_hdr + 0x64 ) = 100;
	*(unsigned *)( mvr_hdr + 0x68 ) = 24;
}



static int draw_prop = 1;

static void
draw_prop_select( void * priv )
{
	draw_prop = !draw_prop;
}


struct menu_entry debug_menus[] = {
	{
		.display	= efic_temp_display,
	},
	{
		.display	= mvr_time_const_display,
		.select		= mvr_time_const_select,
	},
	{
		.priv		= "Draw palette",
		.select		= bmp_draw_palette,
		.display	= menu_print,
	},
	{
		.priv		= "Toggle draw_prop",
		.select		= draw_prop_select,
		.display	= menu_print,
	},
	{
		.priv		= "Dump prop log",
		.select		= prop_log_select,
		.display	= prop_log_display,
	},
	{
		.priv		= "Dump dmlog",
		.select		= (void*) dumpf,
		.display	= menu_print,
	},
	{
		.priv		= "Screenshot",
		.select		= call_dispcheck,
		.display	= menu_print,
	},
};


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

	for( ; menu ; menu = menu->next )
	{
		unsigned fontspec = FONT(
			FONT_MED,
			COLOR_YELLOW,
			menu->selected ? 0x7F : COLOR_BG
		);
		bmp_printf( fontspec, x, y, "%7s", menu->name );
		x += fontspec_font( fontspec )->width * 7;

		if( menu->selected )
			menu_display(
				menu->children,
				orig_x,
				y + fontspec_font( fontspec )->height + 4,
				1
			);
	}
}


void
menu_entry_select(
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

	if( !entry || !entry->select )
		return;

	entry->select( entry->priv );
}

/** Scroll side to side in the list of menus */
void
menu_move(
	struct menu *		menu,
	gui_event_t		ev
)
{
	if( !menu )
		return;

	switch( ev )
	{
	case PRESS_JOY_LEFT:
		if( !menu->prev )
			break;
		menu->prev->selected	= 1;
		menu->selected		= 0;
		break;

	case PRESS_JOY_RIGHT:
		if( !menu->next )
			break;
		menu->next->selected	= 1;
		menu->selected		= 0;
		break;

	default:
		break;
	}
}


/** Scroll up or down in the currently displayed menu */
void
menu_entry_move(
	struct menu *		menu,
	gui_event_t		ev
)
{
	if( !menu )
		return;

	struct menu_entry *	entry = menu->children;

	for( ; entry ; entry = entry->next )
	{
		if( entry->selected )
			break;
	}

	// Nothing selected?
	if( !entry )
		return;

	switch( ev )
	{
	case PRESS_JOY_UP:
		// First and moving up?
		if( !entry->prev )
			break;
		entry->selected = 0;
		entry->prev->selected = 1;
		break;

	case PRESS_JOY_DOWN:
		// Last and moving down?
		if( !entry->next )
			break;
		entry->selected = 0;
		entry->next->selected = 1;
		break;
	default:
		break;
	}
}


static int
menu_handler(
	void *			priv,
	gui_event_t		event,
	int			arg2,
	int			arg3
)
{
	// Check if we should stop displaying
	if( !gui_show_menu
	|| event == TERMINATE_WINSYS
	|| event == DELETE_DIALOG_REQUEST )
	{
		DebugMsg( DM_MAGIC, 3, "Menu task shutting down: %d", event );
		//bmp_fill( COLOR_EMPTY, 90, 90, 720-180, 480-180 );
		return 1;
	}

	static uint32_t events[ MAX_GUI_EVENTS ][4];

	// Ignore periodic events
	if( event != GUI_TIMER )
	{
		// Store the event in the log
		events[ last_menu_event ][0] = event;
		events[ last_menu_event ][1] = arg2;
		events[ last_menu_event ][2] = arg3;
		last_menu_event = (last_menu_event + 1) % MAX_GUI_EVENTS;
	}


	// Find the selected menu
	struct menu * menu = menus;
	for( ; menu ; menu = menu->next )
		if( menu->selected )
			break;

	switch( event )
	{
	case INITIALIZE_CONTROLLER:
		DebugMsg( DM_MAGIC, 3, "Menu task INITIALIZE_CONTROLLER" );
		last_menu_event = 0;
		return 0;

	case GOT_TOP_OF_CONTROL:
		DebugMsg( DM_MAGIC, 3, "Menu task GOT_TOP_OF_CONTROL" );
		goto redraw_dialog;

	case PRESS_JOY_UP:
	case PRESS_JOY_DOWN:
		menu_entry_move( menu, event );
		break;

	case PRESS_JOY_LEFT:
	case PRESS_JOY_RIGHT:
		menu_move( menu, event );
		goto redraw_dialog;

	case PRESS_SET_BUTTON:
		menu_entry_select( menu );
		break;

	default:
		return 0;
	}
		
	// Something happened
	menus_display( menus, 100, 100 );
	return 0;

redraw_dialog:
	//bmp_fill( COLOR_BG, 90, 90, 720-180, 480-180 );
	menus_display( menus, 100, 100 );
	return 0;
}


static void * token;
void property_token(
	void *			new_token
)
{
	token = new_token;
}



void property_slave(
	unsigned		property,
	void *			UNUSED( priv ),
	unsigned *		addr,
	unsigned		len
)
{
/*
	if( property == PROP_LV_STATE )
		goto ack;

	if( property == PROP_LENS_SOMETHING )
		write_debug_file( "lensinfo.log", addr, len );
*/
	struct property * prop = &prop_log[ prop_head ];
	prop_head = (prop_head + 1) % MAX_PROP_LOG;

	DebugMsg( DM_MAGIC, 3, "Prop %08x: %d: %08x %08x %08x %08x",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0
	);
		

	unsigned i;
	prop->prop	= property;
	prop->len	= len;

	if( len > sizeof(prop->data) )
		len = sizeof(prop->data);
	unsigned word_len = (len + 3) / 4;
	for( i=0 ; i<sizeof(prop->data)/4 ; i++ )
		prop->data[i] =  i < word_len ? addr[i] : 0;

	if( !draw_prop )
		goto ack;

	const unsigned x = 80;
	static unsigned y = 32;

	bmp_printf( FONT_SMALL, x, y, "%08x %04x: %08x %08x %08x %08x %08x %08x",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0,
		len > 0x10 ? addr[4] : 0,
		len > 0x14 ? addr[5] : 0
	);
	y += font_small.height;

	bmp_fill( COLOR_RED, x, y, 100, 1 );

	if( y > 400 )
		y = 32;
	
ack:
	prop_cleanup( token, property );
}

#define num_properties 8192
unsigned property_list[ num_properties ];


void
call_init_funcs( void )
{
	// Call all of the init functions
	extern struct task_create _init_funcs_start[];
	extern struct task_create _init_funcs_end[];
	struct task_create * init_func = _init_funcs_start;

	for( ; init_func < _init_funcs_end ; init_func++ )
	{
		DebugMsg( DM_MAGIC, 3,
			"Calling init_func %s (%x)",
			init_func->name,
			(unsigned) init_func->entry
		);

		thunk entry = (thunk) init_func->entry;
		entry();
	}
}


static void
menu_task( void )
{
	menus = NULL;
	menu_task_ptr = NULL;

	msleep( 1000 );
	// Parse our config file
	global_config = config_parse_file( "A:/magiclantern.cfg" );
	draw_prop = config_int( global_config, "prop.draw", 0 );

	draw_version();

	// Only record important events for the display and face detect
	dm_set_store_level( DM_DISP, 4 );
	dm_set_store_level( DM_LVFD, 4 );
	dm_set_store_level( DM_LVCFG, 4 );
	dm_set_store_level( DM_LVCDEV, 4 );
	dm_set_store_level( DM_LV, 4 );
	dm_set_store_level( DM_RSC, 4 );
	dm_set_store_level( 0, 4 ); // catch all?

	call_init_funcs();

#if 1
	unsigned i, j, k;
	unsigned actual_num_properties = 0;
	//for( i=0 ; i<=0x8 ; i++ )
	i = 8;
	{
		for( j=0 ; j<=0x8 ; j++ )
		{
			for( k=0 ; k<0x40 ; k++ )
			{
				unsigned prop = 0
					| (i << 28) 
					| (j << 16)
					| (k <<  0);

				if( prop == 0x80030014
				||  prop == 0x80030015
				||  prop == 0x80050000
				||  prop == 0x80050004
				||  prop == 0x80050005
				||  prop == 0x80050010
				||  prop == 0x8005000f
				)
					continue;

				property_list[ actual_num_properties++ ] = prop;

				if( i != 0 )
				property_list[ actual_num_properties++ ] = 0
					| (i << 28) 
					| (j << 24)
					| (k <<  0);

				if( actual_num_properties > num_properties )
					goto thats_all;
			}
		}
	}

thats_all:
#else
	int actual_num_properties = 0;
	property_list[actual_num_properties++] = 0x44;
#endif

	prop_head = 0;
	prop_register_slave(
		//(void*) 0xffc509b0, 0xDA,
		property_list, actual_num_properties,
		property_slave,
		0,
		property_token
	);

	unsigned property = PROP_EFIC_TEMP;
	prop_register_slave(
		&property,
		1,
		efic_temp_property_handler,
		0xdeadbeef,
		efic_temp_token_handler
	);

	msleep( 3000 );

	int enable_liveview = config_int( global_config, "enable-liveview", 1 );
	if( enable_liveview )
		call( "FA_StartLiveView" );

	menu_add( "Debug", debug_menus, COUNT(debug_menus) );

	while(1)
	{
		if( !gui_show_menu )
		{
			if( menu_task_ptr )
			{
				gui_task_destroy( menu_task_ptr );
				menu_task_ptr = 0;
			}

			msleep( 500 );
			continue;
		}

		if( gui_show_menu == 1 )
		{
			DebugMsg( DM_MAGIC, 3, "Creating menu task" );
			last_menu_event = 0;
			menu_task_ptr = gui_task_create( menu_handler, 0 );
			gui_show_menu = 2;
			draw_version();
		}

		msleep( 100 );
	}
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1e, 0x1000 );
