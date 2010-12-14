/** \file
 * Test bracketing
 */

#include "dryos.h"
#include "tasks.h"
#include "bmp.h"
#include "menu.h"
#include "lens.h"
#include "config.h"

static struct semaphore * bracket_sem;

static void
bracket_start( void * priv )
{
	DebugMsg( DM_MAGIC, 3, "%s: Starting bracket task", __func__ );
	gui_stop_menu();
	give_semaphore( bracket_sem );
}

CONFIG_INT("brack.ae-count", ae_count, 5);
CONFIG_INT("brack.ae-step", ae_step, 3);

static void
ae_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//2349012
		"AE %s   %2d",
		priv == &ae_count  ? "Count:" :
		priv == &ae_step   ? "Step: " :
		"?????:",
		*(int*) priv
	);
}


static void
ae_adjust_count(
	void *			priv
)
{
	ae_count += 2;

	if( ae_count > 13 )
		ae_count = 3;
}


static void
ae_adjust_step(
	void *			priv
)
{
	ae_step++;
	if( ae_step > 18 )
		ae_step = 1;
}


static struct menu_entry bracket_menu[] = {
	{
		.priv		= "Test bracket",
		.display	= menu_print,
		.select		= bracket_start,
	},
	{
		.priv		= &ae_count,
		.display	= ae_display,
		.select		= ae_adjust_count,
	},
	{
		.priv		= &ae_step,
		.display	= ae_display,
		.select		= ae_adjust_step,
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

		// Get the current value for a starting point
		const int ae = lens_get_ae();
		DebugMsg( DM_MAGIC, 3,
			"%s: Current AE %d.  Count=%d step=%d",
			__func__,
			ae,
			ae_count,
			ae_step
		);
	
		msleep( 100 );

		int i;
		for( i=-ae_count/2 ; i<=ae_count/2 ; i++ )
		{
			int new_ae = ae + ae_step * i;
			DebugMsg( DM_MAGIC, 3,
				"%s: Exposure %d: ae %d",
				__func__,
				i,
				new_ae
			);
			lens_set_ae( new_ae );
			lens_take_picture( 1000 );
		}

		lens_set_ae( ae );
	}
}

TASK_CREATE( "bracket_task", bracket_task, 0, 0x10, 0x1000 ); 

