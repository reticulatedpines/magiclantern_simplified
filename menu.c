/** \file
 * Magic Lantern GUI
 */
#include "dryos.h"
#include "version.h"
#include "bmp.h"
#include "gui.h"
#include "config.h"
#include "property.h"

static void
draw_version( void )
{
	bmp_printf( 0, 32,
		"Magic Lantern Firmware version %s (%s)\nBuilt on%s by %s\n%s",
		build_version,
		build_id,
		build_date,
		build_user,
		"http://magiclantern.wikia.com/"
	);

	//thunk debug_lens_info = (void*) 0xff8efde8;
	//debug_lens_info();
	//bmp_hexdump( 0, 200, (void*)( 0x1D88 ), 0x40 );

	int y = 200;
	struct config * config = global_config;
	bmp_printf( 0, y, "Config: %x", (unsigned) global_config );
	y += font_height;

	while( config )
	{
		bmp_printf( 0, y, "'%s' => '%s'", config->name, config->value );
		config = config->next;
		y += font_height;
	}
}


static void
draw_events( void )
{
	int i;
	for( i=0 ; i<MAX_GUI_EVENTS ; i++ )
	{
		const struct event * const ev = &gui_events[ (i + gui_events_index) % MAX_GUI_EVENTS ];
		bmp_printf( 0, 100 + i*font_height,
			"Ev %d %08x %08x %08x",
			(unsigned) ev->type,
			(unsigned) ev->param,
			(unsigned) ev->obj,
			(unsigned) ev->arg
		);
	}
}


static unsigned last_menu_event;
static struct gui_task * menu_task_ptr;


struct menu_entry
{
	int			selected;
	void *			priv;
	void			(*select)( void * priv );
	void			(*display)(
		void *			priv,
		int			x,
		int			y,
		int			selected
	);
};


void
menu_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf( x, y, "%s%s",
		selected ? "->" : "  ",
		(const char*) priv
	);
}


unsigned zebra_level = 0xF000;
void zebra_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x4000) & 0xF000;
}

void zebra_display( void * priv, int x, int y, int selected )
{
	bmp_printf( x, y, "%sZebra level: %04x",
		selected ? "->" : "  ",
		*(unsigned*) priv
	);
}

unsigned zebra_draw = 1;
void zebra_draw_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = !*ptr;
}

void zebra_draw_display( void * priv, int x, int y, int selected )
{
	bmp_printf( x, y, "%sZebras %s",
		selected ? "->" : "  ",
		*(unsigned*) priv ? "on" : "off"
	);
}

unsigned audio_mgain = 0;
void audio_mgain_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x1) & 0x7;
	audio_configure();
}

void audio_mgain_display( void * priv, int x, int y, int selected )
{
	bmp_printf( x, y, "%sMGAIN reg: 0x%x",
		selected ? "->" : "  ",
		*(unsigned*) priv
	);
}

unsigned audio_dgain = 0;
void audio_dgain_toggle( void * priv )
{
	unsigned dgain = *(unsigned*) priv;
	dgain += 6;
	if( dgain > 40 )
		dgain = 0;
	*(unsigned*) priv = dgain;
	audio_configure();
}

void audio_dgain_display( void * priv, int x, int y, int selected )
{
	bmp_printf( x, y, "%sDGAIN reg: %2d dB",
		selected ? "->" : "  ",
		*(unsigned*) priv
	);
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

void debug_lens_info( void * priv )
{
	//thunk focusinfo = (thunk) 0xff8a3344;
	//dm_set_store_level( 0x9f, 1 );
	//DebugMsg( DM_MAGIC, 3, "Calling rmt_focusinfo %x", (unsigned) focusinfo );
	//focusinfo();
	//bmp_hexdump( 300, 100, (void*) 0x39e4, 0x80 );
	call( "FA_MovieStart" );
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
	bmp_printf( x, y, "%sDump prop log %04x",
		selected ? "->" : "  ",
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



struct menu_entry main_menu[] = {
	{
		.selected	= 1,
		.priv		= &zebra_draw,
		.select		= zebra_draw_toggle,
		.display	= zebra_draw_display,
	},
	{
		.priv		= &zebra_level,
		.select		= zebra_toggle,
		.display	= zebra_display,
	},
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
	{
		.priv		= &audio_mgain,
		.select		= audio_mgain_toggle,
		.display	= audio_mgain_display,
	},
	{
		.priv		= &audio_dgain,
		.select		= audio_dgain_toggle,
		.display	= audio_dgain_display,
	},
	{
		.priv		= "Draw palette",
		.select		= bmp_draw_palette,
		.display	= menu_print,
	},
	{
		.select		= prop_log_select,
		.display	= prop_log_display,
	},
	{
		.priv		= "Dump dmlog",
		.select		= dumpf,
		.display	= menu_print,
	},
	{
		.selected	= -1,
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
	for( ; menu->selected >= 0 ; menu++, y += font_height )
	{
		menu->display(
			menu->priv,
			x,
			y,
			menu->selected
		);
	}
}


void
menu_select(
	struct menu_entry *	menu
)
{
	for( ; menu->selected >= 0 ; menu++ )
	{
		if( !menu->selected )
			continue;

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

	for( ; menu->selected >= 0 ; menu++ )
	{
		if( !menu->selected )
			continue;

		if( ev == PRESS_JOY_UP )
		{
			// First and moving up?
			if( menu == menu_top )
				break;
			menu[-1].selected = 1;
			menu[ 0].selected = 0;
			break;
		}

		if( ev == PRESS_JOY_DOWN )
		{
			// Last and moving down?
			if( menu[1].selected < 0 )
				break;
			menu[+1].selected = 1;
			menu[ 0].selected = 0;
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
	if( !gui_show_menu )
	{
		gui_task_destroy( menu_task_ptr );
		//bmp_fill( 0x00, 0, 0, 640, 480 );
		menu_task_ptr = 0;
		return 1;
	}

	static uint32_t events[ MAX_GUI_EVENTS ][4];

	// Ignore periodic events
	if( event == GUI_TIMER )
		return 1;

	// Store the event in the log
	events[ last_menu_event ][0] = event;
	events[ last_menu_event ][1] = arg2;
	events[ last_menu_event ][2] = arg3;
	last_menu_event = (last_menu_event + 1) % MAX_GUI_EVENTS;

	menu_display( main_menu, 100, 100, 1 );

#if 0
	unsigned i;
	for( i=0 ; i < MAX_GUI_EVENTS ; i++ )
	{
		uint32_t * ev = events[ (i + last_menu_event) % MAX_GUI_EVENTS ];
		bmp_printf( 300, 100 + i*font_height, "GUI: %08x %08x %08x",
			(unsigned) ev[0],
			(unsigned) ev[1],
			(unsigned) ev[2]
		);
	}
#endif
	extern char current_lens_name[];
	bmp_printf( 300, 88, "Lens: '%s'", current_lens_name );
	//bmp_hexdump( 300, 100, (void*) 0x39e4, 0x80 );

	switch( event )
	{
	case INITIALIZE_CONTROLLER:
		DebugMsg( DM_MAGIC, 3, "Menu task INITIALIZE_CONTROLLER" );
		last_menu_event = 0;
		break;

	case PRESS_JOY_UP:
	case PRESS_JOY_DOWN:
		menu_move( main_menu, event );
		break;

	case PRESS_SET_BUTTON:
		menu_select( main_menu );
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

	int draw_prop = 0;
	if( !draw_prop )
		goto ack;

	const unsigned x = 150;
	static unsigned y = 32;

/*
	DebugMsg( DM_MAGIC, 3, "Prop %08x: %08x @ %02d: %08x",
		property,
		(unsigned) addr,
		len,
		addr[0]
	);
*/

	bmp_printf( x, y, "Prop: %08x: %d %08x",
		property,
		len,
		addr[0]
	);
	y += font_height;

	if( len != 4 )
	{
		bmp_hexdump( x, y, addr, len );
		y += ((len+15) / 16) * font_height;
	}

	bmp_fill( RED_COLOR, x, y, 100, 1 );

	if( y > 400 )
		y = 32;
	
ack:
	prop_cleanup( token, property );
}

#define num_properties 1024
unsigned property_list[ num_properties ];


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
	dm_set_store_level( 0, 4 ); // catch all?

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
				property_list[ actual_num_properties++ ] = 0
					| (i << 28) 
					| (j << 16)
					| (k <<  0);

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
	property_list[actual_num_properties++] = 0x80030002;
#endif

	prop_head = 0;
	prop_register_slave(
		(void*) 0xffc509b0, 0xDA,
		//property_list, actual_num_properties,
		property_slave,
		0,
		property_token
	);

	msleep( 3000 );

	int enable_liveview = config_int( global_config, "enable-liveview", 1 );
	if( enable_liveview )
		call( "FA_StartLiveView" );

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
			bmp_printf( 0, 400, "Creating menu task" );
			last_menu_event = 0;
			menu_task_ptr = gui_task_create( menu_handler, 0 );
			gui_show_menu = 2;
		}

		draw_version();
		//draw_events();
		msleep( 100 );
	}
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1e, 0x1000 );
