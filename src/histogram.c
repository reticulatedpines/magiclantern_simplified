#include "dryos.h"
#include "vram.h"

#include "bmp.h"
#include "version.h"
#include "config.h"
#include "lens.h"
#include "math.h"
#include "raw.h"
#include "menu.h"

#include "imgconv.h"

#include "histogram.h"


#if defined(FEATURE_HISTOGRAM)

extern unsigned int log_length(int x);
extern int FAST get_y_skip_offset_for_overlays();
extern int nondigic_zoom_overlay_enabled();


CONFIG_INT( "hist.draw", hist_draw,  1 );
CONFIG_INT( "hist.colorspace",   hist_colorspace,    1 );
CONFIG_INT( "hist.warn", hist_warn,  1 );
CONFIG_INT( "hist.log",  hist_log,   1 );
CONFIG_INT( "hist.meter", hist_meter,  2);

struct Histogram histogram;

#ifdef FEATURE_RAW_HISTOGRAM

#define HIST_METER_DYNAMIC_RANGE 1
#define HIST_METER_ETTR_HINT 2

void FAST hist_build_raw()
{
    if (!raw_update_params()) return;

    memset(&histogram, 0, sizeof(histogram));
    histogram.is_raw = 1;

    int step = lv ? 4 : 2;

    /* only show a 12-bit hisogram, since the rest is just noise */
    char r2ev[4096];
    for (int i = 0; i < 4095; i++)
        r2ev[i] = COERCE((raw_to_ev(i*4) + 12) * (HIST_WIDTH-1) / 12, 0, HIST_WIDTH-1);

    for (int i = os.y0; i < os.y_max; i += step)
    {
        int y = BM2RAW_Y(i);
        if (y < raw_info.active_area.y1 || y > raw_info.active_area.y2) continue;

        for (int j = os.x0; j < os.x_max; j += 8)
        {
            int x = BM2RAW_X(j);
            if (x < raw_info.active_area.x1 || x > raw_info.active_area.x2) continue;

            int r = raw_red_pixel_dark(x, y);
            int g = raw_green_pixel_dark(x, y);
            int b = raw_blue_pixel_dark(x, y);

            int ir = r2ev[(r>>2) & 4095];
            int ig = r2ev[(g>>2) & 4095];
            int ib = r2ev[(b>>2) & 4095];
            histogram.hist_r[ir]++;
            histogram.hist_g[ig]++;
            histogram.hist_b[ib]++;
            histogram.total_px++;
        }
    }
    for (int i = 0; i < HIST_WIDTH; i++)
    {
        histogram.max = MAX(histogram.max, histogram.hist_r[i]);
        histogram.max = MAX(histogram.max, histogram.hist_g[i]);
        histogram.max = MAX(histogram.max, histogram.hist_b[i]);
        histogram.hist[i] = (histogram.hist_r[i] + histogram.hist_g[i] + histogram.hist_b[i]) / 3;
    }
}

CONFIG_INT("raw.histo", raw_histogram_enable, 1);

MENU_UPDATE_FUNC(raw_histo_update)
{
    if (!can_use_raw_overlays_menu())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Set picture quality to RAW in Canon menu.");
    else if (raw_histogram_enable)
        MENU_SET_WARNING(MENU_WARN_INFO, "Will use RAW histogram in LiveView and after taking a pic.");
}
#endif

static int hist_rgb_color(int y, int sizeR, int sizeG, int sizeB)
{
    switch ((y > sizeR ? 0 : 1) |
            (y > sizeG ? 0 : 2) |
            (y > sizeB ? 0 : 4))
    {
        case 0b000: return COLOR_ALMOST_BLACK;
        case 0b001: return COLOR_RED;
        case 0b010: return COLOR_GREEN2;
        case 0b100: return COLOR_LIGHT_BLUE;
        case 0b011: return COLOR_YELLOW;
        case 0b110: return COLOR_CYAN;
        case 0b101: return COLOR_MAGENTA;
        case 0b111: return COLOR_WHITE;
    }
    return 0;
}

static void hist_dot(int x, int y, int fg_color, int bg_color, int radius, int label)
{
    x &= ~3;
    y &= ~3;
    for (int r = 0; r < radius; r++)
    {
        draw_circle(x, y, r, fg_color);
        draw_circle(x + 1, y, r, fg_color);
    }
    draw_circle(x, y, radius, bg_color);

    if (label)
    {
        if (label < 10)
            bmp_printf(
                SHADOW_FONT(FONT(FONT_MED, COLOR_WHITE, fg_color)),
                x - 4,
                y - font_med.height/2,
                "%d", label
            );
        else
            bmp_printf(
                SHADOW_FONT(FONT(FONT_SMALL, COLOR_WHITE, fg_color)),
                x - 8,
                y - font_small.height/2,
                "%d", label
            );
    }
}

static int hist_dot_radius(int over, int hist_total_px)
{
    // overexposures stronger than 1% are displayed at max radius (10)
    int p = 100 * over / hist_total_px;
    if (p > 1) return 10;

    // for smaller overexposure percentages, use dot radius to suggest the amount
    unsigned p1000 = 100 * 1000 * over / hist_total_px;
    int plog = p1000 ? (int)log2f(p1000) : 0;
    return MIN(plog, 10);
}

static int hist_dot_label(int over, int hist_total_px)
{
    return 100 * over / hist_total_px;
}

/** Draw the histogram image into the bitmap framebuffer.
 *
 * Draw one pixel at a time; it seems to be ok with err70.
 * Since there is plenty of math per pixel this doesn't
 * swamp the bitmap framebuffer hardware.
 */
void hist_draw_image(
    unsigned        x_origin,
    unsigned        y_origin,
    int highlight_level
)
{
    if (!PLAY_OR_QR_MODE)
    {
        if (!lv_luma_is_accurate()) return;
    }
    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    // Align the x origin, just in case
    x_origin &= ~3;

    uint8_t * row = bvram + x_origin + y_origin * BMPPITCH;
    if( histogram.max == 0 )
        histogram.max = 1;

    unsigned i, y;

    if (highlight_level >= 0)
        highlight_level = (highlight_level * HIST_WIDTH) >> 8;

    int log_max = log_length(histogram.max);

    #ifdef FEATURE_RAW_HISTOGRAM
    const int v = (1200 - raw_info.dynamic_range) * HIST_WIDTH / 1200;
    int underexposed_level = COERCE(v, 0, HIST_WIDTH-1);
    int stops_until_overexposure = 0;
    #endif

    for( i=0 ; i < HIST_WIDTH ; i++ )
    {
        // Scale by the maximum bin value
        const uint32_t size  = hist_log ? log_length(histogram.hist[i])   * hist_height / log_max : (histogram.hist[i]   * hist_height) / histogram.max;
        const uint32_t sizeR = hist_log ? log_length(histogram.hist_r[i]) * hist_height / log_max : (histogram.hist_r[i] * hist_height) / histogram.max;
        const uint32_t sizeG = hist_log ? log_length(histogram.hist_g[i]) * hist_height / log_max : (histogram.hist_g[i] * hist_height) / histogram.max;
        const uint32_t sizeB = hist_log ? log_length(histogram.hist_b[i]) * hist_height / log_max : (histogram.hist_b[i] * hist_height) / histogram.max;

        uint8_t * col = row + i;
        // vertical line up to the hist size
        for( y=hist_height ; y>0 ; y-- , col += BMPPITCH )
        {
            if (highlight_level >= 0)
            {
                int hilight = ABS(i-highlight_level) <= 1;
                *col = y > size + hilight ? COLOR_BG : (hilight ? COLOR_RED : COLOR_WHITE);
            }
            else if (hist_colorspace == 1 && !EXT_MONITOR_RCA) // RGB
                *col = hist_rgb_color(y, sizeR, sizeG, sizeB);
            else
                *col = y > size ? COLOR_BG :
#if defined(FEATURE_FALSE_COLOR)
                                             falsecolor_fordraw(((i << 8) / HIST_WIDTH) & 0xFF);
#else
                                             COLOR_WHITE;
#endif /* defined(FEATURE_FALSE_COLOR) */
        }

#if defined(FEATURE_HISTOGRAM)
        if (hist_warn && i == HIST_WIDTH - 1)
        {
            unsigned int thr = histogram.total_px / 100000; // start at 0.0001 with a tiny dot
            thr = MAX(thr, 1);
            int yw = y_origin + 12 + (hist_log ? hist_height - 24 : 0);
            int bg = (hist_log ? COLOR_WHITE : COLOR_BLACK);
            if (hist_colorspace == 1 && !EXT_MONITOR_RCA) // RGB
            {
                unsigned int over_r = histogram.hist_r[i] + histogram.hist_r[i-1];
                unsigned int over_g = histogram.hist_g[i] + histogram.hist_g[i-1];
                unsigned int over_b = histogram.hist_b[i] + histogram.hist_b[i-1];

                if (over_r > thr) hist_dot(x_origin + HIST_WIDTH/2 - 25, yw, COLOR_RED,        bg, hist_dot_radius(over_r, histogram.total_px), hist_dot_label(over_r, histogram.total_px));
                if (over_g > thr) hist_dot(x_origin + HIST_WIDTH/2     , yw, COLOR_GREEN1,     bg, hist_dot_radius(over_g, histogram.total_px), hist_dot_label(over_g, histogram.total_px));
                if (over_b > thr) hist_dot(x_origin + HIST_WIDTH/2 + 25, yw, COLOR_LIGHT_BLUE, bg, hist_dot_radius(over_b, histogram.total_px), hist_dot_label(over_b, histogram.total_px));
            }
            else
            {
                unsigned int over = histogram.hist[i] + histogram.hist[i-1];
                if (over > thr) hist_dot(x_origin + HIST_WIDTH/2, yw, COLOR_RED, bg, hist_dot_radius(over, histogram.total_px), hist_dot_label(over, histogram.total_px));
            }
        }
#endif
        #ifdef FEATURE_RAW_HISTOGRAM
        /* divide the histogram in 12 equal slices - each slice is 1 EV */
        if (histogram.is_raw)
        {
            static unsigned bar_pos;
            if (i == 0) bar_pos = 0;
            int h = hist_height - MAX(MAX(sizeR, sizeG), sizeB) - 1;

            if ((int)i <= underexposed_level + HIST_WIDTH/12)
            {
                draw_line(x_origin + i, y_origin, x_origin + i, y_origin + h, (int)i <= underexposed_level ? 4 : COLOR_GRAY(20));
            }

            if (i == bar_pos)
            {
                int dy = (i < font_med.width * 4) ? font_med.height : 0;
                draw_line(x_origin + i, y_origin + dy, x_origin + i, y_origin + h, COLOR_GRAY(50));
                bar_pos = (((bar_pos+1)*12/HIST_WIDTH) + 1) * HIST_WIDTH/12;
            }

            unsigned int thr = histogram.total_px / 10000;
            if (histogram.hist_r[i] > thr || histogram.hist_g[i] > thr || histogram.hist_b[i] > thr)
                stops_until_overexposure = 120 - (i * 120 / (HIST_WIDTH-1));
        }
        #endif

    }
    bmp_draw_rect(60, x_origin-1, y_origin-1, HIST_WIDTH+1, hist_height+1);

    #ifdef FEATURE_RAW_HISTOGRAM
    if (histogram.is_raw)
    {
        char msg[10];
        switch (hist_meter)
        {
            case HIST_METER_DYNAMIC_RANGE:
            {
                int dr = (raw_info.dynamic_range + 5) / 10;
                snprintf(msg, sizeof(msg), "D%d.%d", dr/10, dr%10);
                break;
            }

            case HIST_METER_ETTR_HINT:
            {
                if (!stops_until_overexposure)
                    stops_until_overexposure = INT_MIN;

                #ifdef CONFIG_MODULES
                int ettr_stops = INT_MIN;

                if (module_exec(NULL, "auto_ettr_export_correction", 1, &ettr_stops) == 1)
                    if (ettr_stops != INT_MIN)
                        stops_until_overexposure = (ettr_stops+5)/10;
                #endif

                if (stops_until_overexposure != INT_MIN)
                    snprintf(msg, sizeof(msg), "E%s%d.%d", FMT_FIXEDPOINT1(stops_until_overexposure));
                else
                    snprintf(msg, sizeof(msg), "OVER");
                break;
            }

            default:
                snprintf(msg, sizeof(msg), "RAW");
                break;
        }
        bmp_printf(SHADOW_FONT(FONT_MED), x_origin+4, y_origin, msg);
    }
    #endif
}

MENU_UPDATE_FUNC(hist_print)
{
#if defined(FEATURE_HISTOGRAM)
    if (hist_draw)
    {
        MENU_SET_VALUE(
            "%s%s%s",
            hist_colorspace == 0 ? "Luma" : "RGB",
            hist_log ? ",Log" : ",Lin",
            hist_warn ? ",dots" : ""
        );
    }
#endif
    #ifdef FEATURE_RAW_HISTOGRAM
    if (hist_draw && can_use_raw_overlays_menu())
    {
        raw_histo_update(entry, info);
        MENU_APPEND_VALUE(",RAW");
    }
    #endif
}

#endif /* defined(FEATURE_HISTOGRAM) */

MENU_UPDATE_FUNC(hist_warn_display)
{
#if defined(FEATURE_HISTOGRAM)
    MENU_SET_VALUE(
        "Clip warning  : %s",
        hist_warn == 0 ? "OFF" :
        hist_warn == 1 ? "0.001% px" :
        hist_warn == 2 ? "0.01% px" :
        hist_warn == 3 ? "0.1% px" :
        hist_warn == 4 ? "1% px" :
                         "Gradual"
    );
#endif /* defined(FEATURE_HISTOGRAM) */
}


void hist_highlight(int level)
{
#ifdef FEATURE_HISTOGRAM
    get_yuv422_vram();
    hist_draw_image( os.x_max - HIST_WIDTH, os.y0 + 100, level );
#endif
}

#ifdef FEATURE_RAW_HISTOGRAM

/* speed:
 * 0 = slowest, but 100% accurate (only for GRAY_PROJECTION_GREEN for now)
 * 1 = sample at LiveView resolution (720x480)
 * 2 = LiveView resolution downsampled by 2 on each axis
 * 3 = LiveView resolution downsampled by 3 on each axis
 * and so on, until 16
 */

int FAST raw_hist_get_percentile_levels(int* percentiles_x10, int* output_raw_values, int n, int gray_projection, int speed)
{
    if (!raw_update_params()) return -1;
    get_yuv422_vram();

    int* hist = SmallAlloc(16384*4);
    if (!hist) return -1;
    memset(hist, 0, 16384*4);

    int off = get_y_skip_offset_for_histogram();
    if (speed == 0 && gray_projection == GRAY_PROJECTION_GREEN)
    {
        /* time: 1-2 seconds on full raw 5D3 */
        //~ int t0 = get_ms_clock_value();
        for (struct raw_pixblock * row = (struct raw_pixblock *) raw_info.buffer + raw_info.active_area.y1 * raw_info.width / 8 + (raw_info.active_area.x1 + 7) / 8; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.active_area.y2; row += 2 * raw_info.width / 8)
        {
            struct raw_pixblock * row2 = row + raw_info.pitch / sizeof(struct raw_pixblock);

            struct raw_pixblock * p;
            struct raw_pixblock * q;
            for (p = row, q = row2; (void*)p < (void*)row + raw_info.jpeg.width * 14/8; p++, q++)
            {

                /**
                 *  p: abcdefgh abcdefgh
                 *  q: abcdefgh abcdefgh
                 *
                 *     rgrgrgrg rgrgrgrg
                 *     gbgbgbgb gbgbgbgb
                 */

                //~ int pa = ((int)(p->a));
                int pb = ((int)(p->b_lo | (p->b_hi << 12)));
                //~ int pc = ((int)(p->c_lo | (p->c_hi << 10)));
                int pd = ((int)(p->d_lo | (p->d_hi << 8)));
                //~ int pe = ((int)(p->e_lo | (p->e_hi << 6)));
                int pf = ((int)(p->f_lo | (p->f_hi << 4)));
                //~ int pg = ((int)(p->g_lo | (p->g_hi << 2)));
                int ph = ((int)(p->h));
                int qa = ((int)(q->a));
                //~ int qb = ((int)(q->b_lo | (q->b_hi << 12)));
                int qc = ((int)(q->c_lo | (q->c_hi << 10)));
                //~ int qd = ((int)(q->d_lo | (q->d_hi << 8)));
                int qe = ((int)(q->e_lo | (q->e_hi << 6)));
                //~ int qf = ((int)(q->f_lo | (q->f_hi << 4)));
                int qg = ((int)(q->g_lo | (q->g_hi << 2)));
                //~ int qh = ((int)(q->h));

                hist[pb]++;
                hist[pd]++;
                hist[pf]++;
                hist[ph]++;
                hist[qa]++;
                hist[qc]++;
                hist[qe]++;
                hist[qg]++;

                /* to check if we sample only the active area */
                //~ p->a = rand();
            }
        }
        //~ int t1 = get_ms_clock_value();
        //~ NotifyBox(5000, "%d ", t1 - t0);
        //~ save_dng("A:/foo.dng");
    }
    else
    {
        speed = COERCE(speed, 1, 16);
        for (int i = os.y0 + off; i < os.y_max - off; i += speed)
        {
            int y = BM2RAW_Y(i);
            for (int j = os.x0; j < os.x_max; j += speed)
            {
                int x = BM2RAW_X(j);
                int px = raw_get_gray_pixel(x, y, gray_projection);
                hist[px & 16383]++;
            }
        }
    }

    int total = 0;
    int i;
    for( i=0 ; i < 16384 ; i++ )
        total += hist[i];

    for (int k = 0; k < n; k++)
    {
        int thr = (uint64_t)total * percentiles_x10[k] / 1000 - 2;  // 50% => median; allow up to 2 stuck pixels
        int n = 0;
        int ans = -1;

        for( i=0 ; i < 16384; i++ )
        {
            n += hist[i];
            if (n >= thr)
            {
                ans = i;
                break;
            }
        }

        output_raw_values[k] = ans;
    }

    SmallFree(hist);
    return 1;
}

int raw_hist_get_percentile_level(int percentile_x10, int gray_projection, int speed)
{
    int ans;
    raw_hist_get_percentile_levels(&percentile_x10, &ans, 1, gray_projection, speed);
    return ans;
}


int raw_hist_get_overexposure_percentage(int gray_projection)
{
    if (!raw_update_params()) return -1;
    get_yuv422_vram();

    /* use some tolerance when checking for overexposure, because white level might vary a little */
    int white = raw_info.white_level * 80 / 100;
    int over = 0;
    int total = 0;

    int step = lv ? 4 : 2;

    for (int i = os.y0; i < os.y_max; i += step)
    {
        int y = BM2RAW_Y(i);
        for (int j = os.x0; j < os.x_max; j += step)
        {
            int x = BM2RAW_X(j);
            int px = raw_get_gray_pixel(x, y, gray_projection);
            if (px >= white) over++;
            total++;
        }
    }

    /* percentage x100 */
    return over * 10000 / total;
}

#endif
