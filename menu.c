/** \file
 * Magic Lantern GUI
 */
#include "dryos.h"
#include "version.h"
#include "bmp.h"
#include "gui.h"
#include "config.h"


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

unsigned audio_mgain = 0;
void audio_mgain_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x1) & 0x7;
}

void audio_mgain_display( void * priv, int x, int y, int selected )
{
	bmp_printf( x, y, "%sMGAIN reg: %x",
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
}

void audio_dgain_display( void * priv, int x, int y, int selected )
{
	bmp_printf( x, y, "%sDGAIN reg: %2d",
		selected ? "->" : "  ",
		*(unsigned*) priv
	);
}


struct menu_entry main_menu[] = {
	{
		.selected	= 1,
		.priv		= &zebra_level,
		.select		= zebra_toggle,
		.display	= zebra_display,
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

	menu_display( main_menu, 0, 100, 1 );

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
		

	switch( event )
	{
	case INITIALIZE_CONTROLLER:
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


static void
menu_task( void )
{
	msleep( 1000 );
	// Parse our config file
	global_config = config_parse_file( "A:/ML.CFG" );

	draw_version();

	while(1)
	{
		if( !gui_show_menu )
		{
			msleep( 500 );
			continue;
		}

		if( gui_show_menu == 1 )
		{
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
