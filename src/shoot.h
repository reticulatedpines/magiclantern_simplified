#ifndef __SHOOT_H_
#define __SHOOT_H_
void hdr_shot(int skip0, int wait);
int expo_value_rounding_ok(int raw, int is_aperture);
int round_shutter(int tv, int slowest_shutter);
int round_aperture(int av);
void redraw_after(int msec);
#ifdef FEATURE_INTERVALOMETER
int get_interval_count();
int get_interval_time();
void set_interval_time(int seconds);
void set_interval_index(int index);
#endif
#endif // __SHOOT_H_
