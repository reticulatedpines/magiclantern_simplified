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
	msleep( 4000 );
	bracket_sem = create_named_semaphore( "bracket_sem", 0 );

	menu_add( "Brack", bracket_menu, COUNT(bracket_menu) );

	//thunk stop_quick_review = (thunk) 0xffaadb9c;
	void (*stop_quick_review)(int) = (void*) 0xffaab3c4;

	while( 1 )
	{
		take_semaphore( bracket_sem, 0 );
		DebugMsg( DM_MAGIC, 3, "%s: Awake", __func__ );
		bmp_printf( FONT_SMALL, 400, 30, "%s: Awake", __func__ );

		msleep( 5000 );
#if 0
		int i;
		for( i=0 ; i<focus_steps && lens_info.focus_dist != 0xFFFF ; i++ )
		{
			if( lens_info.focus_dist >= 288 )
				break;

			send_focus_cmd( NULL );
		}

		bmp_printf( FONT_SMALL, 400, 30, "%s: Done!", __func__ );

		continue;
#endif

		DebugMsg( DM_MAGIC, 3, "%s: 1.8", __func__ );
		call( "FA_ClearReleaseModeForSR" );

		lens_set_aperture( APERTURE_1_8 );
		call( "Release" );
		msleep( 200 );

		lens_set_aperture( APERTURE_2_8 );
		call( "Release" );
		msleep( 200 );

		lens_set_aperture( APERTURE_5_6 );
		call( "Release" );
		msleep( 200 );

		lens_set_aperture( APERTURE_8_0 );
		call( "Release" );
		msleep( 200 );

		lens_set_aperture( APERTURE_11 );
		call( "Release" );
		msleep( 200 );

#if 0
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
#endif
	}
}

TASK_CREATE( "bracket_task", bracket_task, 0, 0x10, 0x1000 ); 

