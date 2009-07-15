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
	give_semaphore( bracket_sem );
}


static void sel( void * priv )
{
	prop_request_change( 0x80050001, &priv, 4 );
}

static void show( 
	void *			priv,
	int			x,
	int			y,
	int			selected
) {
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Focus %08x",
		(unsigned) priv
	);
}

static struct menu_entry bracket_menu[] = {
	{
		.priv		= "Test rack",
		.display	= menu_print,
		.select		= bracket_start,
	},
	{ .priv = (void*) 0x07ddff01, .display=show, .select=sel },
	{ .priv = (void*) 0x07ffff01, .display=show, .select=sel },
	{ .priv = (void*) 0x00010001, .display=show, .select=sel },
	{ .priv = (void*) 0x00100001, .display=show, .select=sel },
	{ .priv = (void*) 0x00ff0001, .display=show, .select=sel },
	{ .priv = (void*) 0x01000001, .display=show, .select=sel },
	{ .priv = (void*) 0x07000001, .display=show, .select=sel },
};



static void
bracket_task( void * priv )
{
	msleep( 4000 );
	bracket_sem = create_named_semaphore( "bracket_sem", 0 );

	menu_add( "Bracket", bracket_menu, COUNT(bracket_menu) );

	//thunk stop_quick_review = (thunk) 0xffaadb9c;
	void (*stop_quick_review)(int) = (void*) 0xffaab3c4;

	while( 1 )
	{
		take_semaphore( bracket_sem, 0 );
		DebugMsg( DM_MAGIC, 3, "%s: Awake", __func__ );
		bmp_printf( FONT_SMALL, 400, 30, "%s: Awake", __func__ );

		int i;
		for( i=0 ; i<400; i++ )
		{
			//unsigned value = 0x000dff01; // big steps
			//unsigned value = 0x07ddff01; // small steps
			unsigned value = 0x00010001; // 
			prop_request_change( 0x80050001, &value, 4 );
			msleep( 50 );
		}

		continue;

		DebugMsg( DM_MAGIC, 3, "%s: 1.8", __func__ );
		lens_set_aperture( APERTURE_1_8 );
		msleep( 100 );
		DebugMsg( DM_MAGIC, 3, "%s: take photo", __func__ );
		take_photo();
		DebugMsg( DM_MAGIC, 3, "%s: sleep", __func__ );
		msleep(2000);
		DebugMsg( DM_MAGIC, 3, "%s: stop review.8", __func__ );
		stop_quick_review(1);

		DebugMsg( DM_MAGIC, 3, "%s: 22", __func__ );
		lens_set_aperture( APERTURE_22 );
		msleep( 100 );
		DebugMsg( DM_MAGIC, 3, "%s: take photo", __func__ );
		take_photo();
		DebugMsg( DM_MAGIC, 3, "%s: sleep", __func__ );
		msleep(2000);
		DebugMsg( DM_MAGIC, 3, "%s: stop review.8", __func__ );
		stop_quick_review(1);


		msleep( 100 );
	}
}

TASK_CREATE( "bracket_task", bracket_task, 0, 0x1d, 0x1000 ); 

