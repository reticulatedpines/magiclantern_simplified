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

CONFIG_INT( "focus.follow", follow_focus, 1 );
CONFIG_INT( "focus.follow.rev.h", follow_focus_reverse_h, 0); // for left/right buttons
CONFIG_INT( "focus.follow.rev.v", follow_focus_reverse_v, 0); // for up/down buttons

static int focus_dir;
int get_focus_dir() { return focus_dir; }
int is_follow_focus_active() { return follow_focus; }
int get_follow_focus_stop_on_focus() { return follow_focus == 2; }
int get_follow_focus_dir_v() { return follow_focus_reverse_v ? -1 : 1; }
int get_follow_focus_dir_h() { return follow_focus_reverse_h ? -1 : 1; }

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
		"Focal dist: %s",
		lens_info.focus_dist == 0xFFFF
                        ? " Infnty"
                        : lens_format_dist( lens_info.focus_dist * 10 )
	);

	y += height;
	bmp_printf( font, x, y,
		"Hyperfocal: %s",
		lens_format_dist( lens_info.hyperfocal )
	);

	y += height;
	bmp_printf( font, x, y,
		"DOF Near:   %s",
		lens_format_dist( lens_info.dof_near )
	);

	y += height;
	bmp_printf( font, x, y,
		"DOF Far:    %s",
		lens_info.dof_far >= 1000*1000
			? " Infnty"
			: lens_format_dist( lens_info.dof_far )
	);
}


void focus_stack_ensure_preconditions()
{
	while (lens_info.job_state) msleep(100);
	if (!lv_drawn())
	{
		msleep(200);
		SW1(1,0);
		SW1(0,0);
		msleep(200);
		while (!lv_drawn())
		{
			bmp_printf(FONT_LARGE, 10, 30, "Please switch to LiveView");
			msleep(100);
		}
		msleep(200);
	}

	while (is_manual_focus())
	{
		bmp_printf(FONT_LARGE, 10, 30, "Please enable autofocus");
		msleep(100);
	}
}

void
focus_stack(
	unsigned		count,
	int			step
)
{
	if( count > 15 )
		count = 15;

	bmp_printf( FONT_LARGE, 10, 30, "Focus stack: %dx%d", count, step );
	msleep(1000);
	
	int focus_moved_total = 0;

	unsigned i;
	for( i=0 ; i < count ; i++ )
	{
		if (gui_menu_shown()) break;
		
		bmp_printf( FONT_LARGE, 10, 30, "Focus stack: %d of %d", i+1, count );
		msleep( 500 );
		
		focus_stack_ensure_preconditions();
		lens_take_picture( 64 );
		
		if( count-1 == i )
			break;
		
		focus_stack_ensure_preconditions();

		lens_focus( 1, step );
		lens_focus_wait();
		focus_moved_total += step;
	}

	msleep(1000);
	bmp_printf( FONT_LARGE, 10, 30, "Focus stack done!         " );

	// Restore to the starting focus position
	focus_stack_ensure_preconditions();
	lens_focus( 0, -focus_moved_total );
}


static void
focus_stack_task( void )
{
	while(1)
	{
		take_semaphore( focus_stack_sem, 0 );
		msleep( 100 );
		focus_stack( focus_stack_count, focus_stack_step );
	}
}

TASK_CREATE( "fstack_task", focus_stack_task, 0, 0x1f, 0x1000 );

static struct semaphore * focus_task_sem;
static int focus_task_dir;
static int focus_task_delta;
static int focus_rack_delta;
CONFIG_INT( "focus.rack-speed", focus_rack_speed, 4 );

void follow_focus_reverse_dir()
{
	focus_task_dir = -focus_task_dir;
}

void plot_focus_status()
{
	if (gui_menu_shown()) return;
	bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 160, "%s        ", focus_task_dir > 0 ? "FAR " : focus_task_dir < 0 ? "NEAR" : "    ");
}

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
		"Focus dir     : %s",
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
		"Focus A       : %d",
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
		"Focus speed   : %d",
		focus_rack_speed
	);
}


unsigned rack_speed_values[] = {1,2,3,4,5,7,10,13,17,22,28,36,50,75,100,200,300,500,1000};

int current_speed_index(speed)
{
	int i;
	for (i = 0; i < COUNT(rack_speed_values); i++)
		if (speed == rack_speed_values[i]) return i;
	return 0;
}

static void
focus_rack_speed_increment( void * priv )
{
	int i = current_speed_index(focus_rack_speed);
	focus_rack_speed = rack_speed_values[mod(i + 1, COUNT(rack_speed_values))];
}

static void
focus_rack_speed_decrement( void * priv )
{
	int i = current_speed_index(focus_rack_speed);
	focus_rack_speed = rack_speed_values[mod(i - 1, COUNT(rack_speed_values))];
}

static void
focus_stack_step_increment( void * priv )
{
	int i = current_speed_index(focus_stack_step);
	focus_stack_step = rack_speed_values[mod(i + 1, COUNT(rack_speed_values))];
}

static void
focus_stack_count_increment( void * priv )
{
	focus_stack_count = mod(focus_stack_count + 1, 16);
	if (focus_stack_count < 2) focus_stack_count = 2;
}

static void
focus_stack_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Stack focus   : %dx%d",
		focus_stack_count, focus_stack_step
	);
	bmp_printf(FONT_MED, x + 450, y-1, "PLAY: Run\nSET/Q: Adjust");
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
		msleep(10);
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

		while( focus_task_dir )
		{
			int step = focus_task_dir * focus_rack_speed;
			lens_focus( 1, step );
			focus_task_delta += step;
		}
	}
}

TASK_CREATE( "focus_task", focus_task, 0, 0x1d, 0x1000 );


//~ PROP_HANDLER( PROP_LV_FOCUS )
//~ {
	//~ return prop_cleanup( token, property );
	//~ static int16_t oldstep = 0;
	//~ const struct prop_focus * const focus = (void*) buf;
	//~ const int16_t step = (focus->step_hi << 8) | focus->step_lo;
	//~ bmp_printf( FONT_SMALL, 200, 30,
		//~ "FOCUS: %08x active=%02x dir=%+5d (%04x) mode=%02x",
			//~ *(unsigned*)buf,
			//~ focus->active,
			//~ (int) step,
			//~ (unsigned) step & 0xFFFF,
			//~ focus->mode
		//~ );
	//~ return prop_cleanup( token, property );
//~ }

static void
follow_focus_toggle_dir_h( void * priv )
{
	follow_focus_reverse_h = !follow_focus_reverse_h;
}
static void
follow_focus_toggle_dir_v( void * priv )
{
	follow_focus_reverse_v = !follow_focus_reverse_v;
}

static void
follow_focus_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Follow Focus  : %s",
		follow_focus == 1 ? "Manual" : follow_focus == 2 ? "AutoLock" : "OFF"
	);
	if (follow_focus)
	{
		bmp_printf(FONT_MED, x + 480, y+5, follow_focus_reverse_h ? "- +" : "+ -");
		bmp_printf(FONT_MED, x + 480 + font_med.width, y-4, follow_focus_reverse_v ? "-\n+" : "+\n-");
	}
}


CONFIG_INT("movie.af", movie_af, 0);
CONFIG_INT("movie.af.aggressiveness", movie_af_aggressiveness, 4);
CONFIG_INT("movie.af.noisefilter", movie_af_noisefilter, 7); // 0 ... 9
int movie_af_stepsize = 10;



int focus_value = 0; // heuristic from 0 to 100
int focus_value_delta = 0;

volatile int focus_done = 0;
volatile uint32_t focus_done_raw = 0;

PROP_HANDLER(PROP_LV_FOCUS_DONE)
{
        focus_done_raw = buf[0];
        focus_done = 1;
        return prop_cleanup(token, property);
}


int get_focus_graph() 
{ 
	if (movie_af || get_follow_focus_stop_on_focus())
		return !is_manual_focus() && zebra_should_run();

	if (get_trap_focus() && can_lv_trap_focus_be_active())
		return zebra_should_run() || get_halfshutter_pressed();

	return 0;
}

int lv_focus_confirmation = 0;
static int hsp_countdown = 0;
int can_lv_trap_focus_be_active()
{
	if (!lv_drawn()) return 0;
	if (hsp_countdown) return 0; // half-shutter can be mistaken for DOF preview, but DOF preview property triggers a bit later
	if (dofpreview) return 0;
	if (shooting_mode == SHOOTMODE_MOVIE) return 0;
	if (gui_state != GUISTATE_IDLE) return 0;
	if (get_silent_pic_mode()) return 0;
	if (!is_manual_focus()) return 0;
	return 1;
}

int movie_af_active()
{
	return shooting_mode == SHOOTMODE_MOVIE && lv_drawn() && !is_manual_focus() && (focus_done || movie_af==3);
}

static int hsp = 0;
int movie_af_reverse_dir_request = 0;
PROP_HANDLER(PROP_HALF_SHUTTER)
{
	if (buf[0] && !hsp) movie_af_reverse_dir_request = 1;
	hsp = buf[0];
	hsp_countdown = 3;
	if (get_zoom_overlay_z()) zoom_overlay_set_countdown(0);
	
	return prop_cleanup(token, property);
}


int get_lv_focus_confirmation() 
{ 
	if (!can_lv_trap_focus_be_active()) return 0;
	if (!get_halfshutter_pressed()) return 0;
	int ans = lv_focus_confirmation;
	lv_focus_confirmation = 0;
	return ans; 
}

int is_manual_focus()
{
	return (af_mode & 0xF) == 3;
}

static void movie_af_step(int mag)
{
	if (!movie_af_active()) return;
	
	#define MAXSTEPSIZE 64
	#define NP ((int)movie_af_noisefilter)
	#define NQ (10 - NP)
	
	int dirchange = 0;
	static int dir = 1;
	static int prev_mag = 0;
	static int target_focus_rate = 1;
	if (mag == prev_mag) return;
	
	bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "    ");

	static int dmag = 0;
	dmag = ((mag - prev_mag) * NQ + dmag * NP) / 10; // focus derivative is filtered (it's noisy)
	int dmagp = dmag * 10000 / prev_mag;
	static int dmagp_acc = 0;
	static int acc_num = 0;
	dmagp_acc += dmagp;
	acc_num++;
	
	if (focus_done_raw & 0x1000) // bam! focus motor has hit something
	{
		dirchange = 1;
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "BAM!");
	}
	else if (movie_af_reverse_dir_request)
	{
		dirchange = 1;
		movie_af_reverse_dir_request = 0;
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "REV ");
	}
	else
	{
		if (dmagp_acc < -500 && acc_num >= 2) dirchange = 1;
		if (ABS(dmagp_acc) < 500)
		{
			bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, " !! "); // confused
		}
		else
		{
			dmagp_acc = 0;
			acc_num = 0;
			bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, " :) "); // it knows exactly if it's going well or not
		}

		if (ABS(dmagp) > target_focus_rate) movie_af_stepsize /= 2;       // adjust step size in order to maintain a preset rate of change in focus amount
		else movie_af_stepsize = movie_af_stepsize * 3 / 2;               // when focus is "locked", derivative of focus amount is very high => step size will be very low
		movie_af_stepsize = COERCE(movie_af_stepsize, 2, MAXSTEPSIZE);    // when OOF, derivative is very small => will increase focus speed
	}
	
	if (dirchange)
	{
		dir = -dir;
		dmag = 0;
		target_focus_rate /= 4;
	}
	else
	{
		target_focus_rate = target_focus_rate * 11/10;
	}
	target_focus_rate = COERCE(target_focus_rate, movie_af_aggressiveness * 20, movie_af_aggressiveness * 100);

	focus_done = 0;	
	static int focus_pos = 0;
	int focus_delta = movie_af_stepsize * SGN(dir);
	focus_pos += focus_delta;
	lens_focus(7, focus_delta);  // send focus command

	//~ bmp_draw_rect(7, COERCE(350 + focus_pos, 100, 620), COERCE(380 - mag/200, 100, 380), 2, 2);
	
	if (get_global_draw())
	{
		bmp_fill(0, 8, 151, 128, 10);                                          // display focus info
		bmp_fill(COLOR_RED, 8, 151, movie_af_stepsize, 5);
		bmp_fill(COLOR_BLUE, 8, 156, 64 * target_focus_rate / movie_af_aggressiveness / 100, 5);
		bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 160, "%s %d%%   ", dir > 0 ? "FAR " : "NEAR", dmagp/100);
	}
	prev_mag = mag;
}

static void plot_focus_mag(int mag)
{
	if (gui_state != GUISTATE_IDLE) return;
	if (!lv_drawn()) return;
	#define NMAGS 64
	#define FH COERCE(mags[i] * 45 / maxmagf, 0, 50)
	static int mags[NMAGS] = {0};
	int maxmag = 1;
	int i;
	#define WEIGHT(i) 1
	for (i = 0; i < NMAGS-1; i++)
		if (mags[i] * WEIGHT(i) > maxmag) maxmag = mags[i] * WEIGHT(i);

	static int maxmagf = 1;
	maxmagf = (maxmagf * 4 + maxmag * 1) / 5;
	
	for (i = 0; i < NMAGS-1; i++)
	{
		if (get_global_draw()) bmp_draw_rect(COLOR_BLACK, 8 + i, 100, 0, 50);
		mags[i] = mags[i+1];
		if (get_global_draw()) bmp_draw_rect(COLOR_YELLOW, 8 + i, 150 - FH, 0, FH);
	}

	// i = NMAGS-1
	mags[i] = mag;

	focus_value_delta = FH * 2 - focus_value;
	focus_value = FH * 2;
	lv_focus_confirmation = (focus_value + focus_value_delta*3 > 110);
	
	static int rev_countdown = 0;
	static int stop_countdown = 0;
	if (is_follow_focus_active())
	{
		if (get_global_draw())
		{
			plot_focus_status();
			bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "    ");
		}
		if (get_follow_focus_stop_on_focus() && !stop_countdown)
		{
			if (!rev_countdown)
			{
				if (focus_value - focus_value_delta*5 > 110)
				{
					follow_focus_reverse_dir();
					rev_countdown = 5;
					bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "PEAK");
				}
			}
			else
			{
				rev_countdown--;
				bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "PEAK");
				if (focus_value + focus_value_delta*5 > 110) rev_countdown = 0;
				if (!rev_countdown)
				{
					bmp_printf(FONT(FONT_MED, COLOR_WHITE, 0), 30, 180, "LOCK");
					lens_focus_stop();
					stop_countdown = 10;
				}
			}
		}
		if (stop_countdown) stop_countdown--;
	}
	#undef FH
	#undef NMAGS
}

int focus_mag_a = 0;
int focus_mag_b = 0;
int focus_mag_c = 0;
PROP_HANDLER(PROP_LV_FOCUS_DATA)
{
	focus_mag_a = buf[2];
	focus_mag_b = buf[3];
	focus_mag_c = buf[4];
	
	if (movie_af != 3)
	{
		if (get_focus_graph()) plot_focus_mag(focus_mag_a + focus_mag_b);
		if ((movie_af == 2) || (movie_af == 1 && get_halfshutter_pressed())) 
			movie_af_step(focus_mag_a + focus_mag_b);
	}
	return prop_cleanup(token, property);
}

static void
movie_af_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (movie_af)
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Movie AF      : %s A%d N%d",
			movie_af == 1 ? "Hold" : movie_af == 2 ? "Cont" : movie_af == 3 ? "CFPk" : "Err",
			movie_af_aggressiveness,
			movie_af_noisefilter
		);
	else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"Movie AF      : OFF"
		);
}

void movie_af_aggressiveness_bump(void* priv)
{
	movie_af_aggressiveness = movie_af_aggressiveness * 2;
	if (movie_af_aggressiveness > 64) movie_af_aggressiveness = 1;
}
void movie_af_noisefilter_bump(void* priv)
{
	movie_af_noisefilter = (movie_af_noisefilter + 1) % 10;
}

static void
focus_misc_task()
{
	while(1)
	{
		msleep(50);
		
		if (hsp_countdown) hsp_countdown--;

		if (movie_af == 3)
		{
			int fm = get_spot_focus(100);
			if (get_focus_graph()) plot_focus_mag(fm);
			movie_af_step(fm);
		}
	}
}

TASK_CREATE( "focus_misc_task", focus_misc_task, 0, 0x1f, 0x1000 );

static void 
trap_focus_display( void * priv, int x, int y, int selected )
{
	int t = (*(int*)priv);
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Trap Focus    : %s",
		t == 1 ? "Hold" : t == 2 ? "Cont." : "OFF"
	);
}


extern int trap_focus;


static struct menu_entry focus_menu[] = {
	{
		.priv		= &trap_focus,
		.select		= menu_binary_toggle,
		.display	= trap_focus_display,
	},
	{
		.display	= focus_stack_print,
		.select		= focus_stack_count_increment,
		.select_auto		= focus_stack_step_increment,
		.select_reverse		= focus_stack_unlock,
	},
	{
		.display	= focus_rack_speed_display,
		.select		= focus_rack_speed_increment,
		.select_reverse	= focus_rack_speed_decrement
	},
	{
		.priv = &follow_focus,
		.display	= follow_focus_print,
		.select		= menu_ternary_toggle,
		.select_reverse = follow_focus_toggle_dir_v,
		.select_auto = follow_focus_toggle_dir_h,
	},
	{
		.priv = &movie_af,
		.display	= movie_af_print,
		.select		= menu_quaternary_toggle,
		.select_reverse = movie_af_noisefilter_bump,
		.select_auto = movie_af_aggressiveness_bump,
	},
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
		.priv		= "Rack focus",
		.display	= menu_print,
		.select		= focus_toggle,
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

/*
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
*/

INIT_FUNC( __FILE__, focus_init );

