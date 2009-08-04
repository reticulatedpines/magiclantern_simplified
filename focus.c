/** \file
 * Focus control.
 *
 * Support focus stacking and other focus controls.
 */
#include "dryos.h"
#include "menu.h"
#include "bmp.h"
#include "lens.h"
#include "config.h"

static unsigned	focus_mode = 1;

CONFIG_INT( "focus.step",	focus_stack_step, 100 );
CONFIG_INT( "focus.count",	focus_stack_count, 5 );

static struct semaphore * focus_stack_sem;




#if 0
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
)
{
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
#endif


static void
focus_stack_unlock( void * priv )
{
	gui_stop_menu();
	give_semaphore( focus_stack_sem );
}

static void
focus_near( void * priv )
{
	lens_focus( focus_mode, -8 );
}

static void
focus_far( void * priv )
{
	lens_focus( focus_mode, 8 );
}


static void
display_lens_hyperfocal(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	unsigned		font = FONT_LARGE;
	unsigned		height = fontspec_height( font );

	bmp_printf( font, x, y,
		//23456789012
		"Hyperfocal: %s",
		lens_format_dist( lens_info.hyperfocal )
	);

	y += height;
	bmp_printf( font, x, y,
		//23456789012
		"DOF Near:   %s",
		lens_format_dist( lens_info.dof_near )
	);

	y += height;
	bmp_printf( font, x, y,
		//23456789012
		"DOF Far:    %s",
		lens_info.dof_far >= 1000*1000
			? " Infnty"
			: lens_format_dist( lens_info.dof_far )
	);
}


static struct menu_entry focus_menu[] = {
	{
		.priv		= "Run Stack focus",
		.display	= menu_print,
		.select		= focus_stack_unlock,
	},
	{
		.priv		= "Focus Nearer",
		.display	= menu_print,
		.select		= focus_near,
	},
	{
		.priv		= "Focus Further",
		.display	= menu_print,
		.select		= focus_far,
	},
	{
		.display	= display_lens_hyperfocal,
	},
#if 0
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
#endif
};


static void
focus_init( void )
{
	focus_stack_sem = create_named_semaphore( "focus_stack_sem", 0 );

	menu_add( "Focus", focus_menu, COUNT(focus_menu) );
}


INIT_FUNC( __FILE__, focus_init );


void
focus_stack(
	unsigned		count,
	int			step
)
{
	if( count > 15 )
		count = 15;

	unsigned i;
	for( i=0 ; i < count ; i++ )
	{
		lens_take_picture( 2000 );
		if( count-1 == i )
			break;

		lens_focus( 0xD, step );
		lens_focus_wait();
		msleep( 50 );
	}

	// Restore to the starting focus position
	lens_focus( 0, -step * (count-1) );
}


static void
focus_stack_task( void )
{
	while(1)
	{
		take_semaphore( focus_stack_sem, 0 );
		DebugMsg( DM_MAGIC, 3, "%s: Awake", __func__ );
		bmp_printf( FONT_SMALL, 400, 30, "Focus stack" );

		msleep( 100 );
		focus_stack( focus_stack_count, focus_stack_step );
	}
}

TASK_CREATE( __FILE__, focus_stack_task, 0, 0x1f, 0x1000 );
