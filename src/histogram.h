#ifndef _histogram_h_
#define _histogram_h_

// those colors will not be considered for histogram (so they should be very unlikely to appear in real situations)
#define MZ_WHITE 0xFA12FA34 
#define MZ_BLACK 0x00120034
#define MZ_GREEN 0x80808080

#define hist_height         54
#define HIST_WIDTH          128

struct Histogram
{
/** Store the histogram data for each of the "HIST_WIDTH" bins */
    uint32_t hist[HIST_WIDTH];
    uint32_t hist_r[HIST_WIDTH];
    uint32_t hist_g[HIST_WIDTH];
    uint32_t hist_b[HIST_WIDTH];

    /** Maximum value in the histogram so that at least one entry fills
     * the box */
    uint32_t max;

    /** total number of pixels analyzed by histogram */
    uint32_t total_px;

    int is_raw;
};

extern struct Histogram histogram;

void hist_build_raw();

/** Draw the histogram image into the bitmap framebuffer.
 *
 * Draw one pixel at a time; it seems to be ok with err70.
 * Since there is plenty of math per pixel this doesn't
 * swamp the bitmap framebuffer hardware.
 */
void
hist_draw_image(
    unsigned        x_origin,
    unsigned        y_origin,
    int highlight_level
);

void hist_highlight(int level);

int raw_hist_get_percentile_level(int percentile, int gray_projection, int speed);
int raw_hist_get_overexposure_percentage(int gray_projection);

extern struct menu_entry hist_menu_entry;

extern int hist_colorspace;
extern int hist_draw;
extern int hist_meter;
extern int hist_warn;
extern int hist_log;

MENU_UPDATE_FUNC(hist_print);
MENU_UPDATE_FUNC(hist_warn_display);

MENU_UPDATE_FUNC(raw_histo_update);

extern int raw_histogram_enable;

#endif /* _histogram_h_ */
