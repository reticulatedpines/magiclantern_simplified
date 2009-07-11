/** \file
 * Magic Lantern GUI
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


void
menu_add(
	struct menu_entry *	head,
	struct menu_entry *	new_entry,
	int			count
)
{
#if 1
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

void set_aperture( void * priv )
{
	DebugMsg( DM_MAGIC, 3, "Trying to set aperture to f/22" );
	unsigned value = 88;

		thunk stop_quick_review = (thunk) 0xffaadb9c;
	//for( value=APERTURE_1_8 ; value<APERTURE_8_0 ; value++ )
	{
		DebugMsg( DM_MAGIC, 3, "%s: 1.8", __func__ );
		lens_set_aperture( APERTURE_1_8 );
		msleep( 100 );
		DebugMsg( DM_MAGIC, 3, "%s: take photo", __func__ );
		take_photo();
		DebugMsg( DM_MAGIC, 3, "%s: sleep", __func__ );
		msleep(2000);
		DebugMsg( DM_MAGIC, 3, "%s: stop review.8", __func__ );
		stop_quick_review();

		DebugMsg( DM_MAGIC, 3, "%s: 22", __func__ );
		lens_set_aperture( APERTURE_22 );
		msleep( 100 );
		DebugMsg( DM_MAGIC, 3, "%s: take photo", __func__ );
		take_photo();
		DebugMsg( DM_MAGIC, 3, "%s: sleep", __func__ );
		msleep(2000);
		DebugMsg( DM_MAGIC, 3, "%s: stop review.8", __func__ );
		stop_quick_review();
	}

	DebugMsg( DM_MAGIC, 3, "%s: Done!", __func__ );
}


struct menu_entry main_menu = {
	.priv		= 0,
	.selected	= 1,
	.select		= 0,
	.display	= efic_temp_display,
};


/*
	{
		.priv		= "HDMI FullHD",
		.select		= enable_full_hd,
		.display	= menu_print,
	},
	{
		.priv		= "Debug Lens info",
		.select		= debug_lens_info,
		.display	= menu_print,
	},
*/

struct menu_entry debug_menus[] = {
/*
	{
		.priv		= "Draw palette",
		.select		= bmp_draw_palette,
		.display	= menu_print,
	},
*/

	{
		.priv		= "Set aperture",
		.select		= set_aperture,
		.display	= menu_print,
	},
	{
		.priv		= "Dump prop log",
		.select		= prop_log_select,
		.display	= prop_log_display,
	},
	{
		.priv		= "Dump dmlog",
		.select		= dumpf,
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
menu_select(
	struct menu_entry *	menu
)
{
	for( ; menu ; menu = menu->next )
	{
		if( !menu->selected )
			continue;

		if( menu->select )
			menu->select( menu->priv );
		break;
	}
}


void
menu_move(
	struct menu_entry *	menu_top,
	gui_event_t		ev
)
{
	struct menu_entry *	menu = menu_top;

	for( ; menu ; menu = menu->next )
	{
		if( !menu->selected )
			continue;

		if( ev == PRESS_JOY_UP )
		{
			// First and moving up?
			if( !menu->prev )
				break;
			menu->selected = 0;
			menu->prev->selected = 1;
			break;
		}

		if( ev == PRESS_JOY_DOWN )
		{
			// Last and moving down?
			if( !menu->next )
				break;
			menu->selected = 0;
			menu->next->selected = 1;
			break;
		}
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
	|| event == TERMINATE_WINSYS )
	{
		gui_task_destroy( menu_task_ptr );
		menu_task_ptr = 0;
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

	menu_display( &main_menu, 100, 100, 1 );

	switch( event )
	{
	case INITIALIZE_CONTROLLER:
		DebugMsg( DM_MAGIC, 3, "Menu task INITIALIZE_CONTROLLER" );
		last_menu_event = 0;
		bmp_fill( COLOR_BG, 90, 90, 720-180, 480-180 );
		break;

	case PRESS_JOY_UP:
	case PRESS_JOY_DOWN:
		menu_move( &main_menu, event );
		break;

	case PRESS_SET_BUTTON:
		menu_select( &main_menu );
		break;

	default:
		break;
	}
		
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
	if( property == PROP_EFIC_TEMP )
		efic_temp = *addr;

	struct property * prop = &prop_log[ prop_head ];
	prop_head = (prop_head + 1) % MAX_PROP_LOG;

	int i;
	prop->prop	= property;
	prop->len	= len;

	if( len > sizeof(prop->data) )
		len = sizeof(prop->data);
	unsigned word_len = (len + 3) / 4;
	for( i=0 ; i<sizeof(prop->data)/4 ; i++ )
		prop->data[i] =  i < word_len ? addr[i] : 0;

	int draw_prop = 1;
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
	msleep( 1000 );
	// Parse our config file
	global_config = config_parse_file( "A:/magiclantern.cfg" );

	draw_version();

	// Only record important events for the display and face detect
	dm_set_store_level( DM_DISP, 4 );
	dm_set_store_level( DM_LVFD, 4 );
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

				if( j == 5
				|| prop == 0x80030014
				|| prop == 0x80030015
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
	property_list[actual_num_properties++] = 0x80050000;
	property_list[actual_num_properties++] = PROP_EFIC_TEMP;
#endif

	prop_head = 0;
	prop_register_slave(
		//(void*) 0xffc509b0, 0xDA,
		property_list, actual_num_properties,
		property_slave,
		0,
		property_token
	);

	msleep( 3000 );

	int enable_liveview = config_int( global_config, "enable-liveview", 1 );
	if( enable_liveview )
		call( "FA_StartLiveView" );

	menu_add( &main_menu, debug_menus, COUNT(debug_menus) );

	while(1)
	{
		if( !gui_show_menu )
		{
			msleep( 500 );
			continue;
		}

		if( gui_show_menu == 1 )
		{
			DebugMsg( DM_MAGIC, 3, "Creating menu task" );
			last_menu_event = 0;
			menu_task_ptr = gui_task_create( menu_handler, 0 );
			gui_show_menu = 2;
		}

		draw_version();
		msleep( 100 );
	}
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1e, 0x1000 );
