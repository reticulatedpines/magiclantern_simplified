/** \file
 * Magic Lantern GUI
 */
#include "dryos.h"
#include "version.h"
#include "bmp.h"

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


static void
menu_task( void )
{
	msleep( 1000 );
	draw_version();

	while(1)
	{
		if( !gui_show_menu )
		{
			msleep( 500 );
			continue;
		}

		draw_version();
		draw_events();
		msleep( 100 );
	}
}

TASK_CREATE( "menu_task", menu_task, 0, 0x1e, 0x1000 );
