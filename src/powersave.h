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

#endif
