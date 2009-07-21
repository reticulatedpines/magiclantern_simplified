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
	gui_show_menu = 0;
	give_semaphore( bracket_sem );
}

uint8_t focus_mode = 7;
int focus_cmd;
unsigned focus_steps = 1000;

struct semaphore * focus_done_sem;

static void
focus_done( void )
{
	give_semaphore( focus_done_sem );
}


static void
send_focus_cmd( void * priv )
{
	// Should we timeout to avoid hanging?
	take_semaphore( focus_done_sem, 0 );

	struct prop_focus focus = {
		.active		= 1,
		.mode		= focus_mode,
		.step_hi	= (focus_cmd >> 8) & 0xFF,
		.step_lo	= (focus_cmd >> 0) & 0xFF,
	};

	prop_request_change( PROP_LV_FOCUS, &focus, sizeof(focus) );
}

static void sel( void * priv )
{
	unsigned shift = (unsigned) priv;
	unsigned bits = (focus_mode >> shift) & 0xF;
	bits = (bits + 1) & 0xF;
	focus_mode &= ~(0xF  << shift);
	focus_mode |=   bits << shift;
}

static void show( 
	void *			priv,
	int			x,
	int			y,
	int			selected
) {
	unsigned shift = (unsigned) priv;
	unsigned bits = (focus_mode >> shift) & 0xF;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"%1x %08x",
		bits,
		focus_mode
	);
}

static void show_cmd( 
	void *			priv,
	int			x,
	int			y,
	int			selected
) {
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"%+5d %04x",
		focus_cmd,
		(unsigned) focus_cmd & 0xFFFF
	);
}

static void sel_cmd( void * priv )
{
	focus_cmd = (focus_cmd * 3) / 2 + 1;
	if( ((unsigned) focus_cmd) > 0x8000 )
		focus_cmd = 1;
}


static struct menu_entry bracket_menu[] = {
	{
		.priv		= "Test bracket",
		.display	= menu_print,
		.select		= bracket_start,
	},
	{
		.priv		= "Test focus",
		.display	= menu_print,
		.select		= send_focus_cmd,
	},
	{ .priv = &focus_cmd, .display=show_cmd, .select=sel_cmd },
	//{ .priv = &focus_steps, .display=show_steps, .select=sel_steps },
	//{ .priv = (void*) 28, .display=show, .select=sel },
	//{ .priv = (void*) 24, .display=show, .select=sel },
	//{ .priv = (void*) 20, .display=show, .select=sel },
	//{ .priv = (void*) 16, .display=show, .select=sel },
	//{ .priv = (void*) 12, .display=show, .select=sel },
	//{ .priv = (void*) 8, .display=show, .select=sel },
	{ .priv = (void*) 4, .display=show, .select=sel },
	{ .priv = (void*) 0, .display=show, .select=sel },
};



static void
bracket_task( void * priv )
{
	msleep( 4000 );
	bracket_sem = create_named_semaphore( "bracket_sem", 0 );
	focus_done_sem = create_named_semaphore( "focus_sem", 1 );
	focus_cmd = 1;

	unsigned prop_lv_focus = PROP_LV_FOCUS_DONE;
	prop_register_slave(
		&prop_lv_focus,
		1,
		focus_done,
		0,
		0
	);

	menu_add( "Bracket", bracket_menu, COUNT(bracket_menu) );

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

