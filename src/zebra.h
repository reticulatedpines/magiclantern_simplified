#ifndef _zebra_h_
#define _zebra_h_
#include "dryos.h"
int liveview_display_idle();
int get_global_draw();
void NotifyBox( int timeout, char* fmt, ...);

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
