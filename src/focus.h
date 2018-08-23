#ifndef __focus_h__
#define __focus_h__

#include "lens.h"

/* returns true if the AF/MF switch is in the MF position, or you have a manual lens */
int is_manual_focus();

/* returns true in LiveView, if AF is enabled and
 * Continuous AF (photo mode) or Movie Servo AF (movie mode) is selected in Canon menu */
int is_continuous_af();

/* enqueue some focus steps in the focus task */
void lens_focus_enqueue_step(int dir);

/* Starts rack focusing in the given direction */
void lens_focus_start(int dir);

/* abort the current rack focus operation, if any */
void lens_focus_stop( void );

/* for rack focus menu */
int handle_rack_focus_menu_overrides(struct event * event);
void reset_override_zoom_buttons();

/* DOF info calculation */
void focus_calc_dof();

int is_follow_focus_active();
int get_follow_focus_mode();
int get_follow_focus_dir_h();
int get_follow_focus_dir_v();

/* trap focus */
int can_lv_trap_focus_be_active();
int get_lv_focus_confirmation();

int get_focus_confirmation();

/* focus stacking */
int is_focus_stack_enabled();

/* private stuff (focus stacking code tigtly coupled with HDR bracketing stuff */
void focus_stack_run(int skip_frame);

/* focus racking */
void rack_focus_start_now( void * priv, int delta );
int is_rack_focus_enabled();
#endif
