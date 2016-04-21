#ifndef _powersave_h_
#define _powersave_h_

/* reset the powersave timer (as if you would press a button) */
void powersave_prolong();

/* disable powersave timer */
void powersave_prohibit();

/* re-enable powersave timer */
void powersave_permit();

/* stop LiveView activity, leave shutter open */
/* the sensor is turned off, and the LV image will freeze */
/* method similar to what Canon code does when entering PLAY mode from LV */
void PauseLiveView();

/* back to LiveView from paused state */
int ResumeLiveView();

/* turn display on/off */
void display_on();
void display_off();

/* ML powersave in LV */
int idle_is_powersave_enabled();
int idle_is_powersave_active();
int idle_is_powersave_enabled_on_info_disp_key();

/* internal hooks (to be refactored as CBRs) */
void idle_led_blink_step(int k);
void idle_powersave_step();
int handle_powersave_key(struct event * event);
void idle_kill_flicker();
#endif
