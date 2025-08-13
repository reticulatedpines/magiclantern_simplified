#ifndef _zebra_h_
#define _zebra_h_

#include "dryos.h"

/* return true if Canon display is in idle state (ready for shooting) */
/* to be cleaned up and moved together */
int liveview_display_idle();        /* applies only to LiveView */
int display_idle();                 /* applies to both LiveView and photo */

/* returns true if you may draw most overlays, info bars and so on
 * (setting enabled and most preconditions valid; can be queried from menu too) */
int get_global_draw();

/* returns true if you should draw zebras and other overlays right now */
int zebra_should_run();

/* returns true if the setting is enabled (does not check preconditions) */
int get_global_draw_setting();

/* returns true if the luma data from LiveView matches the final output */
/* (that is, histogram and clip warnings make sense */
int lv_luma_is_accurate();

/* for display presets: copy settings to presets */
void update_disp_mode_bits_from_params();

/* current display preset (to be renamed) */
int get_disp_mode();

/* private stuff */
void draw_histogram_and_waveform(int allow_play);
int histogram_or_small_waveform_enabled();
int should_draw_bottom_graphs();

/* true if should draw Magic Zoom (setting + preconditions) */
int should_draw_zoom_overlay();

/* get MZ setting for trigger mode */
int get_zoom_overlay_trigger_mode();

/* trigger Magic Zoom by moving the focus ring (called from prop handler in lens.c) */
void zoom_focus_ring_trigger();

/* true if MZ is configured to be triggered by focus ring */
int get_zoom_overlay_trigger_by_focus_ring();

/* set auto-hide countdown for focus ring trigger (to be renamed) */
void zoom_overlay_set_countdown(int countdown);

/* is the full-screen version of MZ running (which tweaks DIGIC registers), or the plain old memcpy-based one? */
int digic_zoom_overlay_enabled();
int nondigic_zoom_overlay_enabled();

/* MZ vsync in vsync-lite.c (to be cleaned up somehow) */
void _lv_vsync(int mz);

/* todo: move to separate file */
int arrow_keys_shortcuts_active();

/* todo: move to powersave.c/h */
void idle_wakeup_reset_counters(int reason);
void idle_force_powersave_now();
void idle_force_powersave_in_1s();
void idle_globaldraw_dis();
void idle_globaldraw_en();
int get_last_time_active();

int handle_livev_playback(struct event * event);

/* focus peaking */
int is_focus_peaking_enabled();
int focus_peaking_as_display_filter();
void peak_disp_filter();    /* CBR called from tweaks.c */

/* spotmeter */
int get_spot_motion(int dxb, int xcb, int ycb, int draw);
void get_spot_yuv(int dxb, int* Y, int* U, int* V);
void get_spot_yuv_ex(int size_dxb, int dx, int dy, int* Y, int* U, int* V, int draw, int erase);

/* private */
void spotmeter_erase();

/* yuv zebra analysis */
int get_under_and_over_exposure(int thr_lo, int thr_hi, int* under, int* over);

/* to be implemented in a nicer way */
int get_y_skip_offset_for_histogram();
int get_y_skip_offset_for_overlays();

/* from bitrate code, which needs some massive cleanup, maybe move to a module and keep only some basic info in core */
extern int is_mvr_buffer_almost_full();

struct raw_highlight_info
{
    int gray_projection;
    int raw_level_lo;
    int raw_level_hi;
    int color;
    int line_type;
    int fill_type;
};

#define RAW_HIGHLIGHT_END {0,0,0,0,0,0}
    
#define ZEBRA_LINE_NONE 0
#define ZEBRA_LINE_SIMPLE 1

#define ZEBRA_FILL_NONE 0
#define ZEBRA_FILL_DIAG 1
#define ZEBRA_FILL_50_PERCENT 2
#define ZEBRA_FILL_SOLID 3

#define FAST_ZEBRA_GRID_COLOR 4 // invisible diagonal grid for zebras; must be unused and only from 0-15

/**
 * Custom highlighting of raw zones / levels
 * Parameter: array of struct raw_highlight_info (where you can define highlight zones and customize pretty much anything)
 * The array *must* be terminated with RAW_HIGHLIGHT_END.
 * 
 * It's not very fast, but it's very powerful
 * e.g. you can sample any channel or combination of channels via gray_projection
 * and you can define as many highlight zones as you want (with custom appearance, hatching patterns and so on)
 * 
 * Exercise: re-create the RAW RGB zebras with a call to this function.
 * You may define new gray projections if needed.
 */
extern void zebra_highlight_raw_advanced(struct raw_highlight_info * raw_highlight_info);

#endif //_zebra_h_
