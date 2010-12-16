/** \file
 * Focus control.
 *
 * Support focus stacking and other focus controls.
 * \todo Figure out how to really tell if a focus event is over.  The
 * property PROP_LV_FOCUS_DONE doesn't seem to really indicate that it
 * is safe to send another one.
 */
#include "dryos.h"
#include "menu.h"
#include "bmp.h"
#include "lens.h"
#include "config.h"
#include "ptp.h"

CONFIG_INT( "focus.step",	focus_stack_step, 100 );
CONFIG_INT( "focus.count",	focus_stack_count, 5 );
static int focus_dir;
int get_focus_dir() { return focus_dir; }

#define FOCUS_MAX 1700
static int focus_position;

static struct semaphore * focus_stack_sem;




static void
focus_stack_unlock( void * priv )
{
	gui_stop_menu();
	give_semaphore( focus_stack_sem );
}


static void
display_lens_hyperfocal(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	unsigned		font = FONT_MED;
	unsigned		height = fontspec_height( font );

	bmp_printf( font, x, y,
		//23456789012
		"Focal dist: %s",
		lens_info.focus_dist == 0xFFFF
                        ? " Infnty"
                        : lens_format_dist( lens_info.focus_dist * 10 )
	);

	y += height;
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

TASK_CREATE( "focus_stack_task", focus_stack_task, 0, 0x1f, 0x1000 );

static struct semaphore * focus_task_sem;
static int focus_task_dir;
static int focus_task_delta;
static int focus_rack_delta;
CONFIG_INT( "focus.rack-speed", focus_rack_speed, 4 );


static void
focus_dir_display( 
	void *			priv,
	int			x,
	int			y,
	int			selected
) {

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Focus dir:  %s",
		focus_dir ? "FAR " : "NEAR"
	);
}

static void
focus_show_a( 
	void *			priv,
	int			x,
	int			y,
	int			selected
) {

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Focus A:    %+5d",
		focus_task_delta
	);
}


static void
focus_reset_a( void * priv )
{
	focus_task_delta = 0;
}


static void
focus_toggle( void * priv )
{
	focus_task_delta = -focus_task_delta;
	focus_rack_delta = focus_task_delta;
	give_semaphore( focus_task_sem );
}


static void
focus_rack_speed_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Rack speed: %2d",
		focus_rack_speed
	);
}

static void
focus_rack_speed_increment( void * priv )
{
	focus_rack_speed = ((focus_rack_speed + 1) * 5) / 4;
	if( focus_rack_speed > 40 )
		focus_rack_speed = 1;
}


void
lens_focus_start(
	int		dir
)
{
	if( dir == 0 )
		focus_task_dir = focus_dir ? 1 : -1;
	else
		focus_task_dir = dir;

	give_semaphore( focus_task_sem );
}


void
lens_focus_stop( void )
{
	focus_task_dir = 0;
}


static void
rack_focus(
	int		speed,
	int		delta
)
{
	DebugMsg( DM_MAGIC, 3,
		"%s: speed=%d delta=%d",
		__func__,
		speed,
		delta
	);

	if( speed <= 0 )
		speed = 1;

	int		speed_cmd = speed;

	// If we are moving closer, invert the speed command
	if( delta < 0 )
	{
		speed_cmd = -speed;
		delta = -delta;
	}

	while( delta )
	{
		if( speed > delta )
			speed = delta;

		delta -= speed;
		lens_focus( 0x7, speed_cmd );
	}
}


static void
focus_task( void )
{
	while(1)
	{
		take_semaphore( focus_task_sem, 0 );

		if( focus_rack_delta )
		{
			gui_hide_menu( 10 );
			rack_focus(
				focus_rack_speed,
				focus_rack_delta
			);

			focus_rack_delta = 0;
			continue;
		}

		int step = focus_task_dir * focus_rack_speed;

		DebugMsg( DM_MAGIC, 3,
			"%s: focus dir %d",
			__func__,
			step
		);

		while( focus_task_dir )
		{
			lens_focus( 1, step );
			focus_task_delta += step;
			//~ if( step > 0 && step < 1000 )
				//~ step = ((step+1) * 100) / 99;
			//~ else
			//~ if( step < 0 && step > -1000 )
				//~ step = ((step-1) * 100) / 99;

			msleep( 50 );
		}
	}
}

TASK_CREATE( "focus_task", focus_task, 0, 0x10, 0x1000 );


PROP_HANDLER( PROP_LV_FOCUS )
{
	return;
	static int16_t oldstep = 0;
	const struct prop_focus * const focus = (void*) buf;
	const int16_t step = (focus->step_hi << 8) | focus->step_lo;
	bmp_printf( FONT_SMALL, 200, 30,
		"FOCUS: %08x active=%02x dir=%+5d (%04x) mode=%02x",
			*(unsigned*)buf,
			focus->active,
			(int) step,
			(unsigned) step & 0xFFFF,
			focus->mode
		);
	return prop_cleanup( token, property );
}

static struct menu_entry focus_menu[] = {
	{
		.priv		= &focus_dir,
		.display	= focus_dir_display,
		.select		= menu_binary_toggle,
	},
	{
		.display	= focus_show_a,
		.select		= focus_reset_a,
	},
	{
		.display	= focus_rack_speed_display,
		.select		= focus_rack_speed_increment,
	},
	{
		.priv		= "Rack focus",
		.display	= menu_print,
		.select		= focus_toggle,
	},
	{
		.priv		= "Run Stack focus",
		.display	= menu_print,
		.select		= focus_stack_unlock,
	},
	{
		.display	= display_lens_hyperfocal,
	},
};


static void
focus_init( void )
{
	focus_stack_sem = create_named_semaphore( "focus_stack_sem", 0 );
	focus_task_sem = create_named_semaphore( "focus_task_sem", 1 );

	menu_add( "Focus", focus_menu, COUNT(focus_menu) );
}


PTP_HANDLER( 0x9998, 0 )
{
	int step = (int) param1;

	focus_position += step;
	if( focus_position < 0 )
		focus_position = 0;
	else
	if( focus_position > FOCUS_MAX )
		focus_position = FOCUS_MAX;

	lens_focus( 0x7, (int) param1 );
	bmp_printf( FONT_MED, 650, 35, "%04d", focus_position );

	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
		.param_count	= 2,
		.param		= { param1, focus_position },
	};

	context->send(
		context->handle,
		&msg
	);

	return 0;
}


INIT_FUNC( __FILE__, focus_init );

