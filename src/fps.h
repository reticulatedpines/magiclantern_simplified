#ifndef _fps_h_
#define _fps_h_
/* todo: get rid of old fps.c and rename fps-engio.c */

/* current LiveView FPS, as read from the timers */
int fps_get_current_x1000();

/* current shutter speed, maybe modified by FPS */
int get_current_shutter_reciprocal_x1000();

/* compute shutter speed from timer value (FRAME_SHUTTER_TIMER) */
int get_shutter_speed_us_from_timer(int timer);

/* how would current FPS settings change some shutter speed? (in 1/8 EV increments) */
int fps_get_shutter_speed_shift(int raw_shutter);

/* slowest shutter speed */
int get_max_shutter_timer();

/* from DebugMsg hack */
void fps_override_shutter_blanking();

/* from movtweaks (gradual expo) */
void fps_expo_iso_step();

/* instant ISO/shutter overrides in LiveView */
/* to be moved, but where? */
int can_set_frame_iso();
void set_frame_iso(int iso);
int get_frame_iso();

int can_set_frame_shutter_timer();
int get_frame_shutter_timer();
void set_frame_shutter_timer(int timer);
void set_frame_shutter(int shutter_reciprocal);

int get_frame_aperture();

int fps_get_iso_correction_evx8();

#endif
