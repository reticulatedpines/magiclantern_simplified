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

CONFIG_INT("brack.ae-count", ae_count, 3);
CONFIG_INT("brack.ae-step", ae_step, 8);
CONFIG_INT("brack.delay", brack_delay, 1000);

static void
ae_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (priv == &ae_step)
	{
		int steps = *(int*)priv;
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"AE Step:    %d%s eV",
			steps/8,
			((steps/4) % 2) ? ".5" : ""
		);
		return;
	}
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"AE %s   %d",
		priv == &ae_count  ? "Count:" :
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
ae_adjust_count_reverse(
	void *			priv
)
{
	ae_count -= 2;

		if( ae_count < 3 )
			ae_count = 13;
}



static void
ae_adjust_step(
	void *			priv
)
{
	ae_step += 4;
	if( ae_step > 32 )
		ae_step = 4;
}

static void
ae_adjust_step_reverse(
	void *			priv
)
{
	ae_step -= 4;
	if( ae_step < 4 )
		ae_step = 32;
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
		.select_reverse = ae_adjust_count_reverse,
	},
	{
		.priv		= &ae_step,
		.display	= ae_display,
		.select		= ae_adjust_step,
		.select_reverse = ae_adjust_step_reverse,
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
		bmp_printf( FONT_MED, 0, 30, "%s: Awake", __func__ );

		// Get the current value for a starting point
		const int ae = lens_get_ae();
		DebugMsg( DM_MAGIC, 3,
			"%s: Current AE %d.  Count=%d step=%d",
			__func__,
			ae,
			ae_count,
			ae_step
		);
		bmp_printf( FONT_MED, 3, 30, "%s: AE=%d, count=%d, steps=%d                          ", __func__, ae, ae_count, ae_step);
	
		msleep( 100 );

		//~ bmp_printf( FONT_MED, 3, 50, "%s: i = %d to %d                        ", __func__, -((int)(ae_count/2)), ae_count/2);
		int i, a, b;
		a = -((int)(ae_count/2)); // signed/unsigned workaround... 
		b = (int)(ae_count/2);
		for( i = a; i <= b; i++ )
		{
			//~ bmp_printf( FONT_MED, 3, 50, "%s: %d!!!!                        ", __func__, i);
			int new_ae = ae + (int)(ae_step) * i;
			bmp_printf( FONT_MED, 3, 30, "%s: Frame %d: newAE=%d                            ", __func__, i, new_ae);
			lens_set_ae( new_ae );
			lens_take_picture( 1000 );
			bmp_printf( FONT_MED, 3, 30, "%s: Took picture                                  ", __func__, i, new_ae);
			msleep(brack_delay);
		}

		lens_set_ae( ae );
	}
}

TASK_CREATE( "bracket_task", bracket_task, 0, 0x10, 0x1000 ); 

