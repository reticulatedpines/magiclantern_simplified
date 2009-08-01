/** \file
 * Test bracketing
 */

#include "dryos.h"
#include "tasks.h"
#include "bmp.h"
#include "menu.h"
#include "lens.h"

static struct semaphore * bracket_sem;

static void
bracket_start( void * priv )
{
	DebugMsg( DM_MAGIC, 3, "%s: Starting bracket task", __func__ );
	gui_stop_menu();
	give_semaphore( bracket_sem );
}



static struct menu_entry bracket_menu[] = {
	{
		.priv		= "Test bracket",
		.display	= menu_print,
		.select		= bracket_start,
	},
};



static void
bracket_task( void * priv )
{
	bracket_sem = create_named_semaphore( "bracket_sem", 0 );

	menu_add( "Brack", bracket_menu, COUNT(bracket_menu) );

	while( 1 )
	{
		take_semaphore( bracket_sem, 0 );
		DebugMsg( DM_MAGIC, 3, "%s: Awake", __func__ );
		bmp_printf( FONT_SMALL, 400, 30, "%s: Awake", __func__ );

		msleep( 100 );

		lens_set_shutter( SHUTTER_100 );
		lens_take_picture( 1000 );

		lens_set_shutter( SHUTTER_125 );
		lens_take_picture( 1000 );

		lens_set_shutter( SHUTTER_160 );
		lens_take_picture( 1000 );

		lens_set_shutter( SHUTTER_200 );
		lens_take_picture( 1000 );

		lens_set_shutter( SHUTTER_250 );
		lens_take_picture( 1000 );
	}
}

TASK_CREATE( "bracket_task", bracket_task, 0, 0x10, 0x1000 ); 

