/**
 * Auto ETTR (Expose To The Right).
 * 
 * Optimize the exposure for raw shooting (photo + video).
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <raw.h>
#include <lens.h>
#include <math.h>
#include <zebra.h>
#include <shoot.h>
#include <fps.h>
#include <focus.h>
#include <beep.h>
#include <histogram.h>
#include <console.h>

/* interface with dual ISO */
#include "../dual_iso/dual_iso.h" 

static CONFIG_INT("auto.ettr", auto_ettr, 0);
static CONFIG_INT("auto.ettr.trigger", auto_ettr_trigger, 2);
static CONFIG_INT("auto.ettr.ignore", auto_ettr_ignore, 1);
static CONFIG_INT("auto.ettr.level", auto_ettr_target_level, -1);
static CONFIG_INT("auto.ettr.max.tv", auto_ettr_max_shutter, 88);
static CONFIG_INT("auto.ettr.clip", auto_ettr_clip, 0);
static CONFIG_INT("auto.ettr.mode", auto_ettr_adjust_mode, 0);
static CONFIG_INT("auto.ettr.midtone.snr", auto_ettr_midtone_snr_limit, 6);
static CONFIG_INT("auto.ettr.shadow.snr", auto_ettr_shadow_snr_limit, 2);
static CONFIG_INT("auto.ettr.dual.iso", auto_ettr_dual_iso_link, 1);
static CONFIG_INT("auto.ettr.allow.beeps", auto_ettr_allow_beeps, 1);

static int debug_info = 0;
static int show_metered_areas = 0;

#define AUTO_ETTR_TRIGGER_ALWAYS_ON (auto_ettr_trigger == 0 || is_intervalometer_running())
#define AUTO_ETTR_TRIGGER_AUTO_SNAP (auto_ettr_trigger == 1)
#define AUTO_ETTR_TRIGGER_PHOTO (AUTO_ETTR_TRIGGER_ALWAYS_ON || AUTO_ETTR_TRIGGER_AUTO_SNAP)
#define AUTO_ETTR_TRIGGER_BY_SET (auto_ettr_trigger == 2)
#define AUTO_ETTR_TRIGGER_BY_HALFSHUTTER_DBLCLICK (auto_ettr_trigger == 3)

/* status codes */
#define ETTR_EXPO_PRECOND_TIMEOUT -2
#define ETTR_EXPO_LIMITS_REACHED -1
#define ETTR_NEED_MORE_SHOTS 0
#define ETTR_SETTLED 1

/** Some cameras do not have raw liveview **/
extern WEAK_FUNC(ret_0) void raw_lv_request();
extern WEAK_FUNC(ret_0) void raw_lv_release();
extern WEAK_FUNC(ret_0) int  raw_lv_is_enabled();
// allow compiling module if FEATURE_RAW_ZEBRAS is undefined
extern WEAK_FUNC(ret_0) void zebra_highlight_raw_advanced(struct raw_highlight_info * raw_highlight_info);

/* optional beeps */
static void ettr_beep()
{
    if (auto_ettr_allow_beeps)
    {
        beep();
    }
}

static void ettr_beep_times(int n)
{
    if (auto_ettr_allow_beeps)
    {
        beep_times(n);
    }
}

static int ettr_get_current_long_exposure_time()
{
    int seconds = menu_get_value_from_script("Bulb Timer", "Exposure duration");
    return seconds;
}

static int ettr_get_current_raw_shutter()
{
    if (is_bulb_mode())
    {
        return shutterf_to_raw(ettr_get_current_long_exposure_time());
    }
    else
    {
        return lens_info.raw_shutter;
    }
}

static int auto_ettr_get_long_exposure_time(int raw_shutter)
{
    /* full-stops will be rounded to minutes, otherwise we get funny times like 91 seconds */
    int seconds = (int)roundf(30.0 * powf(2.0, (16.0 - raw_shutter)/8.0));
    
    /* things like 21 or 19 get rounded */
    int s = (seconds % 60) % 10;
    if (s == 1 || s == 3) {
        seconds--;
    } else if (s == 7 || s == 9) {
        seconds++;
    }
    
    return seconds;
}

static const char * ettr_format_shutter(int raw_shutter)
{
    if (raw_shutter >= SHUTTER_30s)
    {
        return lens_format_shutter(raw_shutter);
    }
    else
    {
        int seconds = auto_ettr_get_long_exposure_time(raw_shutter);
        return format_time_hours_minutes_seconds(seconds);
    }
}


static char* get_current_exposure_settings()
{
    static char msg[50];
    int iso1 = lens_info.iso_analog_raw;
    snprintf(msg, sizeof(msg), "ISO %d", raw2iso(iso1));
    int iso2 = dual_iso_get_recovery_iso();
    if (iso2 && iso2 != iso1)
    {
        STR_APPEND(msg, "/%d", raw2iso(iso2));
    }
    
    if (is_bulb_mode())
    {
        /* note: using ettr_format_shutter here introduces roundoff errors of a few seconds */
        int seconds = ettr_get_current_long_exposure_time();
        STR_APPEND(msg, " %s", format_time_hours_minutes_seconds(seconds));
    }
    else
    {
        STR_APPEND(msg, " %s", lens_format_shutter(lens_info.raw_shutter));
    }
    return msg;
}

static int extra_snr_needed = 0;

/* metering on dual ISO images can be affected by black level delta */
/* ideally, ev_hi = ev_lo + ev_delta, so we'll try to find a black level delta that matches this */
/* => solve this: raw_to_ev(raw_value_hi - black_delta) = raw_to_ev(raw_value_lo + black_delta) + ev_delta */
static int guess_black_delta(int raw_value_lo, int raw_value_hi, float ev_delta)
{
    float best_err = 100000;
    int best_black_delta = 0;
    for (int black_delta = -40; black_delta <= 40; black_delta++)
    {
        float err = ABS(raw_to_ev(raw_value_hi - black_delta) - raw_to_ev(raw_value_lo + black_delta) - ev_delta);
        if (err < best_err)
        {
            best_err = err;
            best_black_delta = black_delta;
        }
    }
    return best_black_delta;
}

/* also used for display on histogram */
static int auto_ettr_get_correction()
{
    static int last_value = INT_MIN;
    
    /* this is kinda slow, don't run it very often */
    static int aux = INT_MIN;
    if (!lv && !should_run_polling_action(100, &aux) && last_value != INT_MIN)
        return last_value;
    
    int gray_proj = 
        auto_ettr_clip == 0 ? GRAY_PROJECTION_MAX_RGB :
        auto_ettr_clip == 1 ? GRAY_PROJECTION_MAX_RB :
        auto_ettr_clip == 2 ? GRAY_PROJECTION_MEDIAN_RGB : -1;
    
    /* compute the raw levels for more percentile values; will help if the image is overexposed */
    /* if it's not, we'll use only the first value (the one from menu) */
    int percentiles[13] = {(1000 - auto_ettr_ignore), 950, 900, 800, 750, 700, 600, 500, 300, 200, 150, 100, 50};

    int raw_values[COUNT(percentiles)];
    static float diff_from_lower_percentiles[COUNT(percentiles)-1] = {0};

    int speed = 1; /* 1 = examine each LiveView pixel (720x480); 2 = downsample by 2 and so on */
    if (lv)
    {
        /* if highlight ignore is off, we have to look carefully */
        /* otherwise, the meter is not that sensitive and can be a little faster */
        speed = auto_ettr_ignore ? 4 : 2;
    }

    int ok = raw_hist_get_percentile_levels(percentiles, raw_values, COUNT(percentiles), gray_proj | GRAY_PROJECTION_DARK_ONLY, speed);
    
    if (ok != 1)
    {
        last_value = INT_MIN;
        return last_value;
    }
    
    float ev = raw_to_ev(raw_values[0]);
    int raw_median_lo = raw_values[7];  /* 50th percentile (median) */
    int raw_shadow_lo = raw_values[12]; /* 5th percentile */
    int raw_highlight_lo = raw_values[0]; /* "highlight ignore" percentile */
    float ev_median_lo = raw_to_ev(raw_median_lo);
    float ev_shadow_lo = raw_to_ev(raw_shadow_lo);
    
    int dual_iso = auto_ettr_dual_iso_link && dual_iso_is_active();
    float ev_median_hi = ev_median_lo;
    float ev_shadow_hi = ev_shadow_lo; /* for dual ISO: for the bright exposure */
    
    if (dual_iso)
    {
        /* for dual ISO only:*/
        /* we have metered the dark exposure (since ETTR is pushing that to the right), now meter the bright one too */

        /* EV difference between the two ISOs (from settings) */
        float dual_iso_spacing = ABS(dual_iso_get_recovery_iso() - lens_info.iso_analog_raw) / 8.0;

        if (lv && !is_movie_mode())
        {
            /* photo LV (only one exposure) */
            /* estimate it from settings */
            int rec_iso = dual_iso_get_recovery_iso();
            if (rec_iso > (int)lens_info.iso_analog_raw) /* we are looking at the dark exposure */
            {
                ev_median_hi = MIN(ev_median_lo + dual_iso_spacing, 0); /* you can't get whiter than white */
                ev_shadow_hi = MIN(ev_shadow_lo + dual_iso_spacing, 0);
            }
            else /* we are looking at the bright exposure */
            {
                ev_median_hi = ev_median_lo - dual_iso_spacing;
                ev_shadow_hi = ev_shadow_lo - dual_iso_spacing;
                float aux = ev_median_hi; ev_median_hi = ev_median_lo; ev_median_lo = aux;
                aux = ev_shadow_hi; ev_shadow_hi = ev_shadow_lo; ev_shadow_lo = aux;
            }
        }
        else
        {
            /* photo non-LV and movie */
            int percentiles_hi[2] = {500, 50};
            int raw_values_hi[2];
            raw_hist_get_percentile_levels(percentiles_hi, raw_values_hi, COUNT(percentiles_hi), gray_proj | GRAY_PROJECTION_BRIGHT_ONLY, 4);
            int raw_median_hi = raw_values_hi[0];  /* 50th percentile (median) */
            int raw_shadow_hi = raw_values_hi[1]; /* 5th percentile */

            /* signal level for the higher exposure must be equal to signal level for the lower exposure plus dual ISO spacing (EV) */
            /* if it's not, it's very likely to be a large black level difference messing with our formulas */
            /* let's try to fight it! */

            /* compute it from shadow levels, because this is where black delta has the largest effect */
            /* if you compute it from median, shadow may be still wrong by 1-2 EV */
            /* if you compute it from shadow, median may be wrong by only 0.1 - 0.2 EV - much better! */
            int black_delta = guess_black_delta(raw_shadow_lo, raw_shadow_hi, dual_iso_spacing);

            ev_median_lo = raw_to_ev(raw_median_lo + black_delta);
            ev_shadow_lo = raw_to_ev(raw_shadow_lo + black_delta);

            ev_median_hi = raw_to_ev(raw_median_hi - black_delta);
            ev_shadow_hi = raw_to_ev(raw_shadow_hi - black_delta);

            if (debug_info)
            {
                int gap_med = (ev_median_hi - ev_median_lo) * 100;
                int gap_shad = (ev_shadow_hi - ev_shadow_lo) * 100;
                printf("Black delta  : %d (EV gap mid:%s%d.%02d shad:%s%d.%02d)\n", black_delta, FMT_FIXEDPOINT2(gap_med), FMT_FIXEDPOINT2(gap_shad));
            }
        }
    }

    if (show_metered_areas)
    {
        /* show where exactly are those percentiles */
        bmp_printf(FONT(FONT_SMALL, COLOR_WHITE, COLOR_BLUE),   0, 20, "Shadows    5%%   ");
        bmp_printf(FONT(FONT_SMALL, COLOR_WHITE, COLOR_ORANGE), 0, 32, "Midtones   50%%  ");
        int hp = (1000 - auto_ettr_ignore);
        bmp_printf(FONT(FONT_SMALL, COLOR_WHITE, COLOR_RED),    0, 44, "Highlights%3d.%d%%", hp/10, hp%10);
        zebra_highlight_raw_advanced(
            (struct raw_highlight_info [])
            {
                {
                    .raw_level_lo = 0,
                    .raw_level_hi = raw_shadow_lo,
                    .color = COLOR_BLUE,
                    .line_type = ZEBRA_LINE_SIMPLE,
                    .fill_type = ZEBRA_FILL_DIAG,
                    .gray_projection = gray_proj | GRAY_PROJECTION_DARK_ONLY,
                },
                {
                    .raw_level_lo = raw_median_lo,
                    .raw_level_hi = raw_median_lo,
                    .color = COLOR_ORANGE,
                    .line_type = ZEBRA_LINE_SIMPLE,
                    .gray_projection = gray_proj | GRAY_PROJECTION_DARK_ONLY,
                },
                {
                    .raw_level_lo = raw_highlight_lo,
                    .raw_level_hi = 16384,
                    .color = COLOR_RED,
                    .line_type = ZEBRA_LINE_SIMPLE,
                    .fill_type = ZEBRA_FILL_DIAG,
                    .gray_projection = gray_proj | GRAY_PROJECTION_DARK_ONLY,
                },
                RAW_HIGHLIGHT_END
            }
        );
    }

    //~ bmp_printf(FONT_MED, 50, 200, "%d ", MEMX(0xc0f08030));
    float target = MIN(auto_ettr_target_level, -0.5);
    float correction = target - ev;
    float overexposed_percentage = 0;
    if (ev < -0.1)
    {
        /* cool, we know exactly how much to correct, we'll return "correction" */
        
        /* save data for helping with future overexposed shots */
        for (int k = 0; k < COUNT(percentiles)-1; k++)
            diff_from_lower_percentiles[k] = ev - raw_to_ev(raw_values[k+1]);
        
        if (debug_info) printf("overexposure hints: %d %d %d\n", (int)(diff_from_lower_percentiles[0] * 100), (int)(diff_from_lower_percentiles[1] * 100), (int)(diff_from_lower_percentiles[2] * 100));
    }
    else
    {
        /* image is overexposed */
        /* and we don't know how much to go back in order to fix the overexposure */

        /* we can find out how many pixels are clipped, but this doesn't help much in knowing how many stops we should go back */
        overexposed_percentage = raw_hist_get_overexposure_percentage(GRAY_PROJECTION_AVERAGE_RGB | GRAY_PROJECTION_DARK_ONLY) / 100.0;
        if (debug_info) printf("overexposure area: %s%d.%d%%\n", FMT_FIXEDPOINT2((int)(overexposed_percentage * 100)));

        /* from the previous shot, we know where the highlights were, compared to some lower percentiles */
        /* let's assume this didn't change; meter at those percentiles and extrapolate the result */

        int num = 0;
        float sum = 0;
        float min = 100000;
        float max = -100000;
        for (int k = 0; k < COUNT(percentiles)-1; k++)
        {
            if (diff_from_lower_percentiles[k] > 0)
            {
                float lower_ev = raw_to_ev(raw_values[k+1]);
                if (lower_ev < -0.1)
                {
                    /* if the scene didn't change, we should be spot on */
                    /* don't update the correction hints, since we don't know exactly where we are */
                    ev = lower_ev + diff_from_lower_percentiles[k];
                    
                    /* we need to get a stronger correction than with the overexposed metering */
                    /* otherwise, the scene probably changed */
                    if (target - ev < correction)
                    {
                        float corr = target - ev;
                        min = MIN(min, corr);
                        max = MAX(max, corr);
                        
                        /* first estimations are more reliable, weight them a bit more */
                        sum += corr * (COUNT(percentiles) - k);
                        num += (COUNT(percentiles) - k);
                        //~ msleep(500);
                        printf("overexposure fix: k=%d diff=%d ev=%d corr=%d\n", k, (int)(diff_from_lower_percentiles[k] * 100), (int)(ev * 100), (int)(corr * 100));
                    }
                }
            }
        }

        /* use the average value for correction */
        correction = sum / num;
        
        if (num < 3 || max - correction > 1 || correction - min > 1 || correction > -1)
        {
            /* scene changed? measurements from previous shot not confirmed or vary too much?
             * 
             * we'll use a heuristic: for 1% of blown out image, go back 1EV, for 100% go back 13EV */
            printf("fail info: (%d %d %d %d) (%d %d %d)\n", raw_values[0], raw_values[1], raw_values[2], raw_values[3], (int)(diff_from_lower_percentiles[0] * 100), (int)(diff_from_lower_percentiles[1] * 100), (int)(diff_from_lower_percentiles[2] * 100));
            float corr = - log2f(1 + overexposed_percentage*overexposed_percentage);
            
            /* with dual ISO, the cost of underexposing is not that high, so prefer it to improve convergence */
            if (dual_iso)
                corr *= 3;
            
            correction = MIN(correction, corr);
            
            /* we can't really meter more than 10 EV */
            correction = MAX(correction, -10);
        }
    }

    int iso1 = lens_info.iso_analog_raw;
    int iso2 = iso1;
    if (dual_iso) iso2 = dual_iso_get_recovery_iso();
    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);
    float dr_lo = get_dxo_dynamic_range(iso_lo) / 100.0;
    float dr_hi = get_dxo_dynamic_range(iso_hi) / 100.0;

    if (debug_info)
    {
        if (dual_iso)
        {
            float midtone_snr_lo = dr_lo + ev_median_lo;
            float shadow_snr_lo = dr_lo + ev_shadow_lo;
            int mid_snr_lo = (int)roundf(midtone_snr_lo * 10);
            int shad_snr_lo = (int)roundf(shadow_snr_lo * 10);
            float midtone_snr_hi = dr_hi + ev_median_hi;
            float shadow_snr_hi = dr_hi + ev_shadow_hi;
            int mid_snr_hi = (int)roundf(midtone_snr_hi * 10);
            int shad_snr_hi = (int)roundf(shadow_snr_hi * 10);
            printf("Midtone SNR  : %s%d.%d / %s%d.%d EV\n", FMT_FIXEDPOINT1(mid_snr_lo), FMT_FIXEDPOINT1(mid_snr_hi));
            printf("Shadows SNR  : %s%d.%d / %s%d.%d EV\n", FMT_FIXEDPOINT1(shad_snr_lo), FMT_FIXEDPOINT1(shad_snr_hi));
        }
        else
        {
            float midtone_snr = dr_lo + ev_median_lo;
            float shadow_snr = dr_lo + ev_shadow_lo;
            int mid_snr = (int)roundf(midtone_snr * 10);
            int shad_snr = (int)roundf(shadow_snr * 10);
            printf("Midtone SNR  : %s%d.%d EV\n", FMT_FIXEDPOINT1(mid_snr));
            printf("Shadows SNR  : %s%d.%d EV\n", FMT_FIXEDPOINT1(shad_snr));
        }
        int clipped = raw_hist_get_overexposure_percentage(GRAY_PROJECTION_AVERAGE_RGB | GRAY_PROJECTION_DARK_ONLY);
        printf("Clipped highs: %s%d.%02d%%\n", FMT_FIXEDPOINT2(clipped));
    }

    if (overexposed_percentage > 0 && (auto_ettr_midtone_snr_limit || auto_ettr_shadow_snr_limit) && !dual_iso)
    {
        /* if the image is overexposed and we have SNR limits, we could meter for those instead */
        /* don't underexpose by more than 2 EV in one step though */
        correction -= 2;
    }

    /* are we underexposing too much? */
    float correction0 = correction;
    if (lens_info.raw_iso && (auto_ettr_midtone_snr_limit || auto_ettr_shadow_snr_limit))
    {
        float midtone_snr = dr_lo + ev_median_lo;
        float shadow_snr = dr_lo + ev_shadow_lo;

        if (auto_ettr_midtone_snr_limit)
        {
            float midtone_expected_snr = midtone_snr + correction0;
            int midtone_desired_snr = auto_ettr_midtone_snr_limit;

            if (midtone_expected_snr < midtone_desired_snr)
            {
                correction = MAX(correction, correction0 + midtone_desired_snr - midtone_expected_snr);
            }
        }

        if (auto_ettr_shadow_snr_limit)
        {
            float shadow_expected_snr = shadow_snr + correction0;
            int shadow_desired_snr = auto_ettr_shadow_snr_limit;

            if (shadow_expected_snr < shadow_desired_snr)
            {
                correction = MAX(correction, correction0 + shadow_desired_snr - shadow_expected_snr);
            }
        }
    }
    
    /* exposure difference with and without SNR limits */
    int expo_delta_snr = (correction - correction0) * 100.0;
    
    if (debug_info)
    {
        int expo_hi = correction0 * 100.0;
        int expo_snr = correction * 100.0;
        printf("Expo highlight: %s%d.%02d EV\n", FMT_FIXEDPOINT2S(expo_hi));
        printf("Expo SNR limit: %s%d.%02d EV\n", FMT_FIXEDPOINT2S(expo_snr));
        printf("Expo delta SNR: %s%d.%02d EV\n", FMT_FIXEDPOINT2S(expo_delta_snr));
    }

    /* exposure correction so it doesn't clip anything more than allowed by highlight ignore */
    int corr_without_clipping = (int)(correction * 100) - expo_delta_snr;

    if (dual_iso)
    {
        /* with dual ISO: expose without clipping */
        /* auto_ettr_work will have to do something and recover the SNR */
        last_value = corr_without_clipping;
        extra_snr_needed = expo_delta_snr;
    }
    else
    {
        /* without dual ISO: expose with clipping in order to meet the SNR */
        /* no more SNR correction needed */
        last_value = corr_without_clipping + expo_delta_snr;
        extra_snr_needed = 0;
    }
    
    if (debug_info)
    {
        printf("Expo correction: %s%d.%02d EV\n", FMT_FIXEDPOINT2S(last_value));
    }
    return last_value;
}

int auto_ettr_export_correction(int* out)
{
    int value = auto_ettr_get_correction();
    if (value == INT_MIN) return -1;
    if (out) *out = value;
    return 1;
}

static char prev_exposure_settings[50];

/* returns: 0 = nothing changed, 1 = OK, -1 = exposure limits reached */
static int auto_ettr_work(int corr)
{
    if (debug_info) printf("\nauto_ettr_work(%d)\n", corr);
    /* wait until shutter speed is reported by Canon firmware */
    int iter = 0;
    while (lens_info.raw_shutter == 0)
    {
        if (iter > 100)
        {
            return ETTR_EXPO_PRECOND_TIMEOUT;
        }
        msleep(50);
        iter += 50;
    }
    
    /* save initial exposure settings so we can print them */
    char* expo_settings = get_current_exposure_settings();
    snprintf(prev_exposure_settings, sizeof(prev_exposure_settings), "%s", expo_settings);
    
    int tv = ettr_get_current_raw_shutter();
    int iso = lens_info.raw_iso;
    
    /* to detect whether it settled or not */
    int tv_before = tv;
    int iso_before = iso;
    int iso2_before = dual_iso_get_recovery_iso();
    
    if (!tv || !iso) return 0;
    //~ int old_expo = tv - iso;

    /* note: expo compensation will not clip with dual ISO, but will clip highlights without it */
    int dual_iso = auto_ettr_dual_iso_link && dual_iso_is_active();
    int delta = -corr * 8 / 100;
    
    int expected_expo = tv - iso + delta;               /* will clip without dual ISO */

    static int prev_tv = 0;
    if (auto_ettr_adjust_mode == 1)
    {
        if (prev_tv != tv)
        {
            auto_ettr_max_shutter = tv;
            if (lv)
            {
                NotifyBox(2000, "ETTR: Tv <= %s ", lens_format_shutter(tv));
                prev_tv = tv;
                return 0; /* wait for next iteration */
            }
            else
            {
                msleep(1000);
                bmp_printf(FONT_MED, 0, os.y0, "ETTR: Tv <= %s ", lens_format_shutter(tv));
            }
        }
    }
    else
    {
        if (lv && prev_tv != tv && AUTO_ETTR_TRIGGER_ALWAYS_ON)
        {
            prev_tv = tv;
            return 0; /* small pause when you change exposure manually */
        }
    }

    int shutter_lim = auto_ettr_max_shutter;

    /* if intervalometer is enabled, limit longest exposures
     * to interval time minus 2 seconds */
    if (is_intervalometer_running())
    {
        int intervalometer_lim = MAX(200, 1000 * (get_interval_time() - 2));
        shutter_lim = MAX(shutter_lim, shutter_ms_to_raw(intervalometer_lim));
    }

    /* can't go slower than 1/fps in movie mode */
    if (is_movie_mode())
    {
        shutter_lim = MAX(shutter_lim, shutter_ms_to_raw(1000 / video_mode_fps));
        if (!expo_override_active())
        {
            /* without expo override, in movie mode we can't set exposures longer than 1/30 */
            shutter_lim = MAX(shutter_lim, SHUTTER_1_30);
        }
    }

    /* apply exposure correction */
    tv += delta;

    if (debug_info) printf("expo after comp: %d\n", tv - iso);

    /* use the lowest ISO for which we can get shutter = shutter_lim or higher */
    int offset = MIN(tv - shutter_lim, iso - MIN_ISO);
    tv -= offset;
    iso -= offset;

    /* some shutter values are not accepted by Canon firmware */
    int tvr = (MIN(tv, shutter_lim) >= SHUTTER_30s)
        ? round_shutter(tv, shutter_lim)
        : MAX(tv, shutter_lim);
    
    iso += tvr - tv;

    if (debug_info) printf("tv rounding: %d -> %d limit=%d\n", tv, tvr, shutter_lim);
    
    /* analog iso can be only in 1 EV increments */
    /* prefer rounding towards lower ISOs */
    int max_auto_iso = auto_iso_range & 0xFF;
    int isor = COERCE(iso / 8 * 8, MIN_ISO, max_auto_iso);
    if (debug_info) printf("iso rounding: %d -> %d (expo %d -> %d)\n", iso, isor, tvr - iso, tvr - isor);
    
    /* can we use dual ISO to recover the highlights? (HR = highlight recovery) */
    if (dual_iso)
    {
        int base_iso = isor;
        int recovery_iso = base_iso;

        /* bring back the SNR */
        int snr_delta = -extra_snr_needed;
        while (snr_delta < 0 && recovery_iso < max_auto_iso)
        {
            int old_rec_iso = recovery_iso;
            recovery_iso += 8;
            int dr_gained = dual_iso_calc_dr_improvement(old_rec_iso, recovery_iso);
            snr_delta += dr_gained;
        }
        
        if (snr_delta + extra_snr_needed < 100) /* snr_delta + extra_snr_needed = SUM(dr_gained) */
        {
            /* too little gain? just shoot at base ISO */
            recovery_iso = base_iso;
            snr_delta = -extra_snr_needed;
        }
        else if (base_iso > MIN_ISO)
        {
            /* shooting at high ISO? go back one stop to protect some more highlights, because the cost is next to none */
            base_iso -= 8;
            expected_expo += 8;
        }

        /* apply dual ISO settings */
        isor = base_iso;
        dual_iso_set_recovery_iso(recovery_iso);
        extra_snr_needed = -snr_delta;
    }

    /* apply the new settings */
    int oki = 0, oks = 0;
    if (tvr < SHUTTER_30s)
    {
        /* use BULB for long exposures */
        ensure_bulb_mode();
        int seconds = auto_ettr_get_long_exposure_time(tvr);
        
        if (is_intervalometer_running())
        {
            /* in BULB mode, limit longest exposures to interval time minus 3 seconds */
            int intervalometer_lim = MAX(1, get_interval_time() - 3);
            seconds = MIN(seconds, intervalometer_lim);
        }
        
        /* configure bulb timer with the new exposure */
        menu_set_value_from_script("Bulb Timer", "Exposure duration", seconds);
        oks = 1;

        /* set ISO */
        oki = hdr_set_rawiso(isor);
    }
    else
    {
        if (is_bulb_mode())
        {
            /* back from BULB */
            set_shooting_mode(SHOOTMODE_M);
        }
        
        oki = lens_set_rawiso(isor);    /* for expo overide */
        oks = lens_set_rawshutter(tvr);
        if (!expo_override_active())
        {
            oks = hdr_set_rawshutter(tvr);  /* for confirmation and retrying if needed */
            oki = hdr_set_rawiso(isor);
        }
    }

    /* don't let expo lock undo our changes */
    expo_lock_update_value();

    if (debug_info)
    {
        printf("Adjusted expo: %s (SNR lost: %s%d.%02d)\n", get_current_exposure_settings(), FMT_FIXEDPOINT2(extra_snr_needed));
    }

    /* to know when the user changed shutter speed */
    prev_tv = ettr_get_current_raw_shutter();
    
    /* did it converge or not? */
    int tv_after = prev_tv;
    int iso_after = lens_info.raw_iso;
    int new_expo = tv_after - iso_after;

    if (dual_iso)
    {
        int iso2_after = dual_iso_get_recovery_iso();
        int dr2_before = dual_iso_calc_dr_improvement(iso_before, iso2_before);
        int dr2_after = dual_iso_calc_dr_improvement(iso_after, iso2_after);

        if (debug_info)
        {
            printf( 
                "iso2 %d->%d dr %d->%d\n",
                raw2iso(iso2_before), raw2iso(iso2_after), dr2_before, dr2_after
            );
        }

        if (ABS(dr2_after - dr2_before) >= 40)
            return ETTR_NEED_MORE_SHOTS;
        
        //~ if (highlight_headroom_needed > 50)
            //~ return ETTR_EXPO_LIMITS_REACHED;
    }

    if (debug_info)
    {
        printf(
            "iso %d->%d %s\ntv %d->%d %s\nexpo expected %d got %d\n",
            raw2iso(iso_before), raw2iso(iso_after), oki ? "OK" : "err",
            tv_before, tv_after, oks ? "OK" : "err",
            expected_expo, new_expo
        );
    }
    
    /* anything changed? consider it OK, better than nothing */
    if (ABS(tv_before - tv_after) >= 4)
        return ETTR_NEED_MORE_SHOTS;

    if (ABS(iso_before - iso_after) >= 4)
        return ETTR_NEED_MORE_SHOTS;

    /* did we fully correct the exposure? */
    if (ABS(new_expo - expected_expo) > 8)
        return ETTR_EXPO_LIMITS_REACHED;

    return oks && oki ? ETTR_SETTLED : ETTR_EXPO_LIMITS_REACHED;
}

static volatile int auto_ettr_running = 0;
static volatile int ettr_pics_took = 0;

static void auto_ettr_step_task(int corr)
{
    lens_wait_readytotakepic(64);
    int status = auto_ettr_work(corr);
    
    if (status == ETTR_SETTLED)
    {
        /* cool, we got the ideal exposure */
        ettr_beep();
        ettr_pics_took = 0;
        
        msleep(1000);
        bmp_printf(FONT_MED, 0, os.y0, "ETTR: settled at %s", get_current_exposure_settings());

        //~ int blown_highlights = (highlight_headroom_needed - highlight_headroom_recovered) / 10;
        //~ if (blown_highlights > 2)
        //~ {
            //~ msleep(1000);
            //~ bmp_printf(FONT_MED, 0, os.y0, "ETTR: clipped %s%d.%d EV of highlights", FMT_FIXEDPOINT1(blown_highlights));
        //~ }
    }
    else if (ettr_pics_took >= 3)
    {
        /* I give up */
        ettr_beep_times(3);
        ettr_pics_took = 0;
        msleep(1000);
        bmp_printf(FONT_MED, 0, os.y0, "ETTR: giving up\n%s", get_current_exposure_settings());
    }
    else if (status == ETTR_EXPO_LIMITS_REACHED)
    {
        ettr_beep_times(3);
        ettr_pics_took = 0;
        msleep(1000);
        bmp_printf(FONT_MED, 0, os.y0, "ETTR: expo limits reached\n%s", get_current_exposure_settings());
    }
    else if (status == ETTR_EXPO_PRECOND_TIMEOUT)
    {
        ettr_beep_times(3);
        ettr_pics_took = 0;
        msleep(1000);
        bmp_printf(FONT_MED, 0, os.y0, "ETTR: timeout while waiting for preconditions\n");
    }
    else if (AUTO_ETTR_TRIGGER_AUTO_SNAP)
    {
        /* take another pic */
        auto_ettr_running = 0;
        schedule_remote_shot();
        ettr_pics_took++;
    }
    else if (AUTO_ETTR_TRIGGER_ALWAYS_ON)
    {
        ettr_beep_times(2);
        msleep(1000);
        bmp_printf(FONT_MED, 0, os.y0, "ETTR: next %s (was %s)", get_current_exposure_settings(), prev_exposure_settings);

        //~ int blown_highlights = (highlight_headroom_needed - highlight_headroom_recovered) / 10;
        //~ if (blown_highlights > 2)
        //~ {
            //~ bmp_printf(FONT_MED, 0, os.y0+20, "Clipped %s%d.%d EV of highlights", FMT_FIXEDPOINT1(blown_highlights));
        //~ }
    }
    auto_ettr_running = 0;
}

/* photo mode only, no LV */
static void auto_ettr_step()
{
    if (!auto_ettr) return;
    if (shooting_mode != SHOOTMODE_M && !is_movie_mode() && !is_bulb_mode()) return;
    if (lens_info.raw_iso == 0) return;
    if (auto_ettr_running) return;
    if (is_hdr_bracketing_enabled() && !AUTO_ETTR_TRIGGER_BY_SET) return;

    if (!raw_update_params())
    {
        NotifyBox(5000, "Raw error");
        return;
    }

    int corr = auto_ettr_get_correction();
    if (corr != INT_MIN)
    {
        /* we'd better not change expo settings from prop task (we won't get correct confirmations) */
        auto_ettr_running = 1;
        task_create("ettr_task", 0x1c, 0x1000, auto_ettr_step_task, (void*) corr);
    }
}

static int auto_ettr_check_pre_lv()
{
    if (!auto_ettr) return 0;
    if (shooting_mode != SHOOTMODE_M && !is_movie_mode()) return 0;
    if (lens_info.raw_iso == 0) return 0;
    if (lens_info.raw_shutter == 0) return 0;
    if (is_hdr_bracketing_enabled() && !AUTO_ETTR_TRIGGER_BY_SET) return 0;
    int raw = is_movie_mode() ? raw_lv_is_enabled() : pic_quality & 0x60000;
    return raw;
}

static int auto_ettr_check_in_lv()
{
    if (AUTO_ETTR_TRIGGER_ALWAYS_ON && !get_expsim()) return 0;
    if (AUTO_ETTR_TRIGGER_ALWAYS_ON && lv_dispsize != 1) return 0;
    if (LV_PAUSED) return 0;
    if (!liveview_display_idle()) return 0;
    return 1;
}

static int auto_ettr_check_lv()
{
    if (!auto_ettr_check_pre_lv()) return 0;
    if (!auto_ettr_check_in_lv()) return 0;
    return 1;
}

static volatile int auto_ettr_vsync_active = 0;
static volatile int auto_ettr_vsync_delta = 0;
static volatile int auto_ettr_vsync_counter = 0;

/* instead of changing settings via properties, we can override them very quickly */
static unsigned int auto_ettr_vsync_cbr(unsigned int ctx)
{
    auto_ettr_vsync_counter++;

    if (auto_ettr_vsync_active)
    {
        int delta = auto_ettr_vsync_delta;
        int current_iso = get_frame_iso();
        int current_shutter = get_frame_shutter_timer();
        int altered_iso = current_iso;
        int altered_shutter = current_shutter;

        int max_shutter = get_max_shutter_timer();
        if (current_shutter > max_shutter) max_shutter = current_shutter;

        /* first increase shutter speed, since it gives the cleanest signal */
        while (delta > 0 && altered_shutter * 2 <= max_shutter)
        {
            altered_shutter *= 2;
            delta -= 8;
        }

        int max_iso = get_max_analog_iso();

        /* then try to increase ISO if we need more */
        while (delta > 0 && altered_iso + 8 <= max_iso)
        {
            altered_iso += 8;
            delta -= 8;
        }

        /* then try to decrease ISO until ISO 100, raw 72 (even with HTP, FRAME_ISO goes to 100) */
        while (delta < -8 && altered_iso - 8 >= 72)
        {
            altered_iso -= 8;
            delta += 8;
        }

        /* commit iso */
        set_frame_iso(altered_iso);

        /* adjust shutter with the remaining delta */
        altered_shutter = COERCE((int)roundf(powf(2, delta/8.0) * (float)altered_shutter), 2, max_shutter);
        
        /* commit shutter */
        set_frame_shutter_timer(altered_shutter);
        
        //~ bmp_printf(FONT_MED, 50, 70, "delta %d iso %d->%d shutter %d->%d max %d ",  auto_ettr_vsync_delta, current_iso, altered_iso, current_shutter, altered_shutter, get_max_shutter_timer());
        return 1;
    }
    
    return 0;
}

static int auto_ettr_wait_lv_frames(int num_frames)
{
    auto_ettr_vsync_counter = 0;
    int count = 0;
    int frame_duration = 1000000 / fps_get_current_x1000();
    while (auto_ettr_vsync_counter < num_frames)
    {
        frame_duration = MAX(frame_duration, 1000000 / fps_get_current_x1000());
        msleep(20);
        count++;
        if (count > num_frames * frame_duration * 2 / 20)
        {
            /* timeout */
            if (debug_info) printf("wait_lv_frames: timeout\n");
            return 0;
        }
        if (!lv)
        {
            /* outside lv */
            if (debug_info) printf("wait_lv_frames: LV closed\n");
            return 0;
        }
    }
    return 1;
}

/* wait until LiveView exposure changes from the old values to something else (with timeout on number of frames) */
static int auto_ettr_wait_lv_expo_change(int max_frames, int old_iso, int old_shutter)
{
    /* todo: also look at aperture changes */
    for (int i = 0; i < max_frames; i++)
    {
        int current_iso = get_frame_iso();
        int current_shutter = get_frame_shutter_timer();
        if (debug_info) printf("wait lv expo change: %x %x\n", current_iso, current_shutter);
        if (current_iso != old_iso || current_shutter != old_shutter)
        {
            if (debug_info) printf("exposure changed to: %x %x\n", current_iso, current_shutter);
            /* exposure changed */
            return 1;
        }
        if (!auto_ettr_wait_lv_frames(1))
        {
            /* whoops */
            return 0;
        }
    }

    /* timeout */
    if (debug_info) printf("lv expo change timeout\n");
    return 0;
}

/* wait until LiveView exposure settles (identical on two consecutive frames) */
static int auto_ettr_wait_lv_expo_settle(int max_frames)
{
    /* todo: also look at aperture changes */
    int old_iso = -1;
    int old_shutter = -1;
    for (int i = 0; i < max_frames; i++)
    {
        int current_iso = get_frame_iso();
        int current_shutter = get_frame_shutter_timer();
        if (debug_info) printf("wait lv expo settle: %x %x\n", current_iso, current_shutter);
        if (current_iso == old_iso && current_shutter == old_shutter)
        {
            /* looks like it settled */
            if (debug_info) printf("lv expo maybe settled at: %x %x\n", current_iso, current_shutter);
            
            /* wait one more frame, just in case */
            if (auto_ettr_wait_lv_frames(2) == 0)
            {
                return 0;
            }
            
            current_iso = get_frame_iso();
            current_shutter = get_frame_shutter_timer();
            if (current_iso == old_iso && current_shutter == old_shutter)
            {
                if (debug_info) printf("lv expo settled at: %x %x\n", current_iso, current_shutter);
                /* looks like it did settle */
                return 1;
            }
        }
        if (!auto_ettr_wait_lv_frames(1))
        {
            /* whoops */
            return 0;
        }
        old_iso = current_iso;
        old_shutter = current_shutter;
    }
    
    /* timeout */
    if (debug_info) printf("lv expo settle timeout\n");
    return 0;
}

static int auto_ettr_prepare_lv(int reset, int force_expsim_and_zoom)
{
    static int was_in_lv = 1;
    static int old_expsim = -1;
    static int old_zoom = -1;
    static int should_clear_bv = 0;
    
    if (!reset)
    {
        was_in_lv = lv;
        old_expsim = -1;

        if (!lv) force_liveview();
        if (!lv) return 0; /* fail */

        /* force 1x zoom */
        if (force_expsim_and_zoom && lv_dispsize != 1)
        {
            old_zoom = lv_dispsize;
            set_lv_zoom(1);
            if (!auto_ettr_wait_lv_frames(10)) return 0;
        }

        /* temporarily enable get_expsim() while metering */
        if (force_expsim_and_zoom)
        {
            if (shooting_mode == SHOOTMODE_M && !lens_info.lens_exists)
            {
                /* workaround for Canon's manual lens underexposure bug */
                /* use expo override instead of ExpSim */
                extern int bv_auto;
                if (!bv_auto)
                {
                    should_clear_bv = 1;
                    bv_toggle(0, 1);
                    if (!auto_ettr_wait_lv_frames(10)) return 0;
                }
            }
            else if (!get_expsim())
            {
                /* ExpSim should work well */
                old_expsim = get_expsim();
                set_expsim(1);
                if (!auto_ettr_wait_lv_frames(10)) return 0;
            }
        }
    }
    else /* undo all that stuff */
    {
        if (should_clear_bv)
        {
            extern int bv_auto;
            if (bv_auto)
            {
                bv_toggle(0, -1);
                auto_ettr_wait_lv_frames(5);
            }
            should_clear_bv = 0;
        }
        
        if (old_expsim >= 0)
        {
            set_expsim(old_expsim);
            old_expsim = -1;
        }
        
        if (old_zoom > 0)
        {
            set_lv_zoom(old_zoom);
            old_zoom = -1;
        }
        
        if (lv && !was_in_lv)
        {
            msleep(200);
            close_liveview();
            was_in_lv = 1;
        }
    }
    return 1; /* ok */
}

static void auto_ettr_on_request_task_fast()
{
    ettr_beep();
    int raw_requested = 0;
    
    char* err_msg = "ETTR failed";
    
    /* requires LiveView and ExpSim */
    if (!auto_ettr_prepare_lv(0, 1)) goto err;
    if (!auto_ettr_check_lv()) goto err;
    
    if (get_halfshutter_pressed())
    {
        msleep(500);
        if (get_halfshutter_pressed()) goto err;
    }

#undef AUTO_ETTR_DEBUG
#ifdef AUTO_ETTR_DEBUG
    auto_ettr_vsync_active = 1;
    int raw0 = raw_hist_get_percentile_level(500, GRAY_PROJECTION_GREEN, 2);
    float ev0 = raw_to_ev(raw0);
    int y0 = 100 - ev0 * 20;
    for (int i = 0; i < 100; i++)
    {
        int delta = rand() % 160 - 80;
        auto_ettr_vsync_delta = delta;
        if (!auto_ettr_wait_lv_frames(2)) break;
        
        int raw = raw_hist_get_percentile_level(500, GRAY_PROJECTION_GREEN, 2);
        float ev = raw_to_ev(raw);
        int x = 360 + delta * 3;
        int y = 100 - ev * 24; /* multiplier must be 8 x the one from delta */
        draw_circle(x, y, 2, COLOR_BLUE);
        draw_angled_line(360, y0, 300, 1800-450, COLOR_RED);
        draw_angled_line(360, y0, 300, -450, COLOR_RED);
        draw_angled_line(0, 100, 720, 0, COLOR_RED);
    }
    auto_ettr_vsync_delta = 0;
    auto_ettr_wait_lv_frames(100);
#endif


    NotifyBox(100000, "ETTR...");
    raw_lv_request(); raw_requested = 1;

    if (!raw_update_params())
    {
        err_msg = "Raw error";
        goto err;
    }

    for (int i = 0; i < 5; i++)
    {
        NotifyBox(100000, "ETTR (%d)...", i+1);

        /* make sure the LiveView exposure is settled before reading */
        if (!auto_ettr_wait_lv_expo_settle(30)) break;

        if (fps_get_shutter_speed_shift(160) == 0)
        {
            auto_ettr_vsync_active = 1;
            auto_ettr_vsync_delta = 0;
            for (int k = 0; k < 5; k++)
            {
                if (debug_info) printf("ETTR (%d.%d)\n", i+1, k+1);
                
                /* see how far we are from the ideal exposure */
                int corr = auto_ettr_get_correction();
                if (corr == INT_MIN) break;
                
                /* override the liveview parameters via auto_ettr_vsync_cbr (much faster than via properties) */
                auto_ettr_vsync_delta += corr * 8 / 100;

                /* I'm confident the last iteration was accurate */
                if (corr >= -20 && corr <= 200)
                    break;
                
                /* wait for 2 frames before trying again */
                if (!auto_ettr_wait_lv_frames(2)) goto err;
            }
            auto_ettr_vsync_active = 0;
        }
        else /* FPS override is messing up our plans? fall back to the slow method */
        {
            int corr = auto_ettr_get_correction();
            if (corr == INT_MIN) break;
            auto_ettr_vsync_delta = corr * 8 / 100;
        }

        /* apply the correction via properties */
        int corr = auto_ettr_vsync_delta * 100 / 8;
        int old_iso = get_frame_iso();
        int old_shutter = get_frame_shutter_timer();
        int status = auto_ettr_work(corr);
    
        if (status == ETTR_SETTLED)
        {
            /* looks like it settled */
            break;
        }
        else
        {
            if (i < 4 && status != ETTR_EXPO_LIMITS_REACHED)
            {
                /* here we go again... */
                if (!auto_ettr_wait_lv_expo_change(30, old_iso, old_shutter)) goto err;
            }
            else
            {
                /* or... not? */
                err_msg = status == ETTR_EXPO_LIMITS_REACHED ? "Expo limits reached" : "Whoops";
                goto err;
            }
        }
    }

/* ok: */
    ettr_beep();
    NotifyBoxHide();
    goto cleanup;

err:
    ettr_beep();
    ettr_beep();
    NotifyBox(2000, err_msg);
    goto cleanup;

cleanup:
    auto_ettr_running = 0;
    auto_ettr_vsync_active = 0;
    if (raw_requested) raw_lv_release();
    auto_ettr_prepare_lv(1, 1);
}

static void auto_ettr_step_lv_fast()
{
    if (!auto_ettr || !AUTO_ETTR_TRIGGER_ALWAYS_ON)
        return;
    
    if (!auto_ettr_prepare_lv(0, 0))
        goto end;
    
    if (!auto_ettr_check_lv())
        goto end;
    
    if (get_halfshutter_pressed())
        goto end;

    /* only poll exposure once per second */
    static int aux = INT_MIN;
    if (!should_run_polling_action(1000, &aux))
        goto end;
    
    raw_lv_request();

    if (!raw_update_params())
    {
        NotifyBox(5000, "Raw error");
        goto skip;
    }

    /* make sure the LiveView exposure is settled before reading */
    if (!auto_ettr_wait_lv_expo_settle(30)) goto skip;

    /* get exposure correction */
    int corr = auto_ettr_get_correction();
    
    /* only correct if the image is overexposed by more than 0.2 EV or underexposed by more than 1 EV */
    if (corr != INT_MIN && (corr < -20 || corr > 100))
    {
        if (fps_get_shutter_speed_shift(160) == 0)
        {
            auto_ettr_vsync_active = 1;
            auto_ettr_vsync_delta = 0;
            int k;
            for (k = 0; k < 5; k++)
            {
                /* see how far we are from the ideal exposure */
                if (k > 0) corr = auto_ettr_get_correction();
                if (corr == INT_MIN) break;
                
                /* override the liveview parameters via auto_ettr_vsync_cbr (much faster than via properties) */
                auto_ettr_vsync_delta += corr * 8 / 100;

                /* I'm confident the last iteration was accurate */
                if (corr >= -20 && corr <= 200)
                    break;
                
                /* wait for 3 frames before trying again */
                if (!auto_ettr_wait_lv_frames(2)) break;
            }
            auto_ettr_vsync_active = 0;
        }
        else /* FPS override is messing up our plans? fall back to the slow method */
        {
            auto_ettr_vsync_delta = corr * 8 / 100;
        }

        /* apply the final correction via properties */
        int old_iso = get_frame_iso();
        int old_shutter = get_frame_shutter_timer();

        auto_ettr_work(auto_ettr_vsync_delta * 100 / 8);

        auto_ettr_wait_lv_expo_change(30, old_iso, old_shutter);
    }

skip:
    raw_lv_release();
    
end:
    auto_ettr_prepare_lv(1, 0);
}

static void auto_ettr_on_request_task_slow()
{
    ettr_beep();
    char* err_msg = "ETTR failed";
    
    /* requires LiveView and ExpSim */
    if (!auto_ettr_prepare_lv(0, 1)) goto err;
    if (!auto_ettr_check_lv()) goto err;

    if (get_halfshutter_pressed())
    {
        msleep(500);
        if (get_halfshutter_pressed()) goto err;
    }

    NotifyBox(100000, "ETTR...");
    for (int k = 0; k < 5; k++)
    {
        msleep(500);
        
        raw_lv_request();

        if (!raw_update_params())
        {
            err_msg = "Raw error";
            goto err;
        }

        int corr = auto_ettr_get_correction();
        raw_lv_release();

        if (corr == INT_MIN)
            break;
        
        int status = auto_ettr_work(corr);
        msleep(1000);
        
        if (status == ETTR_SETTLED)
            break;
        
        if (k == 4 || status == ETTR_EXPO_LIMITS_REACHED)
        {
            err_msg = status == ETTR_EXPO_LIMITS_REACHED ? "Expo limits reached" : "Whoops";
            goto err;
        }
    }

/* ok: */
    ettr_beep();
    NotifyBoxHide();
    goto cleanup;

err:
    ettr_beep();
    ettr_beep();
    NotifyBox(2000, err_msg);
    goto cleanup;

cleanup:
    auto_ettr_prepare_lv(1, 1);
    auto_ettr_running = 0;
}

static void auto_ettr_step_lv_slow()
{
    if (!auto_ettr || !AUTO_ETTR_TRIGGER_ALWAYS_ON)
        return;
    
    if (!auto_ettr_check_lv())
        return;
    
    if (get_halfshutter_pressed())
        return;

    /* only update once per 1.5 seconds, so the exposure has a chance to be updated on the LCD */
    static int aux = INT_MIN;
    if (!should_run_polling_action(1500, &aux))
        return;
    
    int corr = INT_MIN;
    raw_lv_request();
    
    if (raw_update_params())
    {
        corr = auto_ettr_get_correction();
    }
    
    raw_lv_release();
    
    if (corr == INT_MIN)
        return;
    
    /* only correct if the image is overexposed by more than 0.2 EV or underexposed by more than 1 EV */
    if (corr >= -20 && corr < 100)
        return;

    int status = auto_ettr_work(corr);

    if (status == ETTR_SETTLED)
    {
        ettr_beep();
    }
}

static void auto_ettr_step_lv()
{
    if (can_set_frame_iso() && can_set_frame_shutter_timer())
        auto_ettr_step_lv_fast();
    else
        auto_ettr_step_lv_slow();
}

static void auto_ettr_on_request_task(int unused)
{
    if (can_set_frame_iso() && can_set_frame_shutter_timer())
        auto_ettr_on_request_task_fast();
    else
        auto_ettr_on_request_task_slow();
}

static unsigned int auto_ettr_keypress_cbr(unsigned int key)
{
    if (!auto_ettr) return 1;
    if (AUTO_ETTR_TRIGGER_PHOTO) return 1;
    if (!display_idle()) return 1;
    if (!auto_ettr_check_pre_lv()) return 1;
    if (lv && !auto_ettr_check_in_lv()) return 1;
    
    if (
            (AUTO_ETTR_TRIGGER_BY_SET && key == MODULE_KEY_PRESS_SET) ||
            (AUTO_ETTR_TRIGGER_BY_HALFSHUTTER_DBLCLICK && detect_double_click(key, MODULE_KEY_PRESS_HALFSHUTTER, MODULE_KEY_UNPRESS_HALFSHUTTER)) ||
       0)
    {
        if (!auto_ettr_running)
        {
            auto_ettr_running = 1;
            task_create("ettr_task", 0x1c, 0x1000, auto_ettr_on_request_task, (void*) 0);
        }
        if (AUTO_ETTR_TRIGGER_BY_SET) return 0;
    }
    return 1;
}

static MENU_UPDATE_FUNC(auto_ettr_update)
{
    if (lv && ((void*)&raw_lv_request == (void*)&ret_0))
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ETTR Does not work in LV on this camera.");

    if (shooting_mode != SHOOTMODE_M && !is_movie_mode() && !is_bulb_mode())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ETTR only works in M, BULB and RAW MOVIE modes.");

    if (lens_info.raw_iso == 0)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ETTR requires manual ISO.");

    if (!lv && !can_use_raw_overlays_photo() && AUTO_ETTR_TRIGGER_PHOTO)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Photo RAW data not available, try in LiveView.");

    menu_checkdep_raw(entry, info);

    if (image_review_time == 0 && AUTO_ETTR_TRIGGER_PHOTO)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Enable image review from Canon menu.");

    if (is_hdr_bracketing_enabled() && !AUTO_ETTR_TRIGGER_BY_SET)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Not compatible with HDR bracketing. Use trigger mode SET.");

    if (lv && AUTO_ETTR_TRIGGER_ALWAYS_ON && !get_expsim())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "In LiveView, this requires ExpSim enabled.");
    
    if (is_continuous_drive() && AUTO_ETTR_TRIGGER_PHOTO)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Not fully compatible with continuous drive.");

    if (auto_ettr)
    {
        MENU_SET_VALUE(
            AUTO_ETTR_TRIGGER_ALWAYS_ON ? "Always ON" : 
            AUTO_ETTR_TRIGGER_AUTO_SNAP ? "Auto Snap" : 
            AUTO_ETTR_TRIGGER_BY_SET ? "Press SET" : 
            AUTO_ETTR_TRIGGER_BY_HALFSHUTTER_DBLCLICK ? "HalfS DBC" : "err"
        );
    }

    if (!AUTO_ETTR_TRIGGER_PHOTO)
    {
        MENU_SET_HELP("Press the shortcut key to optimize the exposure (ETTR).");
    }
    else if (AUTO_ETTR_TRIGGER_ALWAYS_ON)
    {
        if (lv) MENU_SET_HELP("In LiveView, just wait for exposure to settle, then shoot.");
        else MENU_SET_HELP("Take a test picture (underexposed). Next pic will be ETTR.");
    }
    else if (AUTO_ETTR_TRIGGER_AUTO_SNAP)
    {
        MENU_SET_HELP("Press shutter once. ML will take a pic and retry if needed.");
    }
    
    /* recommended: move AF to back button */
    if (auto_ettr && AUTO_ETTR_TRIGGER_BY_HALFSHUTTER_DBLCLICK && !is_manual_focus())
        entry->works_best_in = DEP_CFN_AF_BACK_BUTTON;
    else
        entry->works_best_in = 0;
}

static MENU_UPDATE_FUNC(auto_ettr_max_shutter_update)
{
    MENU_SET_VALUE("%s", ettr_format_shutter(auto_ettr_max_shutter));
    
    if (auto_ettr_max_shutter < SHUTTER_30s)
    {
        MENU_SET_RINFO("BULB");
        MENU_SET_WARNING(MENU_WARN_INFO, "For long exposures, enable bulb timer (maybe also intervalometer).");
    }
    
    if (is_intervalometer_running())
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "Slowest shutter will be limited by interval time minus 2 seconds.");
    }
    
    if (auto_ettr_adjust_mode == 1)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Adjust shutter speed from top scrollwheel, outside menu.");
    }
}

static MENU_SELECT_FUNC(auto_ettr_max_shutter_toggle)
{
    if (auto_ettr_adjust_mode == 0)
    {
        /* adjust in 0.5 EV steps, from 1/4096 to 4096 seconds */
        const int tv_max = SHUTTER_1_4000;
        const int tv_min = SHUTTER_1s - EXPO_FULL_STOP * 12;
        auto_ettr_max_shutter = MOD(auto_ettr_max_shutter/4*4 - tv_min + delta * 4, tv_max - tv_min + 4) + tv_min;
    }
}

PROP_HANDLER(PROP_GUI_STATE)
{
    if (buf[0] == GUISTATE_QR)
    {
        if (AUTO_ETTR_TRIGGER_PHOTO)
            auto_ettr_step();
    }
}

MENU_UPDATE_FUNC(auto_ettr_intervalometer_warning)
{
    if (auto_ettr && image_review_time == 0 && AUTO_ETTR_TRIGGER_PHOTO)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ETTR: enable image review from Canon menu.");
}

void auto_ettr_intervalometer_wait()
{
    if (auto_ettr && image_review_time)
    {
        /* make sure auto ETTR has a chance to run (it's triggered by prop handler on QR mode) */
        /* timeout: a bit more than exposure time, to handle long expo noise reduction */
        for (int i = 0; i < raw2shutter_ms(ettr_get_current_raw_shutter())/100; i++)
        {
            if (gui_state == GUISTATE_PLAYMENU || gui_state == GUISTATE_QR) break;
            msleep(150);
        }
        msleep(500);
    }
}

static unsigned int auto_ettr_polling_cbr()
{
    if (lv && NOT_RECORDING && ((void*)&raw_lv_request != (void*)&ret_0))
        auto_ettr_step_lv();
    return 0;
}

static MENU_SELECT_FUNC(debug_info_toggle)
{
    debug_info = !debug_info;
    /* fixme: kinda ugly */
    if (debug_info) console_show();
    else console_hide();
}

static struct menu_entry ettr_menu[] =
{
    {
        .name = "Auto ETTR", 
        .priv = &auto_ettr, 
        .update = auto_ettr_update,
        .max = 1,
        .help  = "Auto expose to the right when you shoot RAW.",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Trigger mode",
                .priv = &auto_ettr_trigger,
                .max = 3, // NOTE: Modifed by the module init task to disable ETTR in LV if not supported
                .choices = CHOICES("Always ON", "Auto Snap", "Press SET", "HalfS DblClick"),
                .help  = "When should the exposure be adjusted for ETTR:",
                .help2 = "Always ON: when you take a pic, or continuously in LiveView\n"
                         "Auto Snap: after u take a pic,trigger another pic if needed\n"
                         "Press SET: meter for ETTR when you press SET (LiveView)\n"
                         "HalfS DblClick: meter for ETTR when pressing halfshutter 2x\n"
            },
            {
                .name = "Slowest shutter",
                .priv = &auto_ettr_max_shutter,
                .select = auto_ettr_max_shutter_toggle,
                .update = auto_ettr_max_shutter_update,
                .min = 16,
                .max = 152,
                .icon_type = IT_PERCENT,
                .help = "Slowest shutter speed for ETTR (longest exposure time)."
            },
            {
                .name = "Exposure target",
                .priv = &auto_ettr_target_level,
                .min = -4,
                .max = 0,
                .choices = CHOICES("-4 EV", "-3 EV", "-2 EV", "-1 EV", "-0.5 EV"),
                .help = "Exposure target for ETTR. Recommended: -0.5 or -1 EV.",
                .advanced = 1,
            },
            {
                .name = "Highlight ignore",
                .priv = &auto_ettr_ignore,
                .min = 0,
                .max = 500,
                .unit = UNIT_PERCENT_x10,
                .icon_type = IT_PERCENT,
                .help  = "How many bright pixels are allowed above the target level.",
                .help2 = "Use this to allow spec(ta)cular highlights to be clipped.",
            },
            {
                .name = "Allow clipping",
                .priv = &auto_ettr_clip,
                .max = 2,
                .choices = CHOICES("OFF", "Green channel", "Any channel"),
                .help = "Choose what color channels are allowed to be clipped.",
                .advanced = 1,
            },
            {
                .name = "Midtone SNR limit",
                .priv = &auto_ettr_midtone_snr_limit,
                .min = 0,
                .max = 8,
                .choices = CHOICES("OFF", "1 EV", "2 EV", "3 EV", "4 EV", "5 EV", "6 EV", "7 EV", "8 EV"),
                .help  = "Stop underexposing when at least half of the image gets",
                .help2 = "noisier than selected SNR => will clip more highlights.",
                .depends_on = DEP_MANUAL_ISO,
            },
            {
                .name = "Shadow SNR limit",
                .priv = &auto_ettr_shadow_snr_limit,
                .min = 0,
                .max = 6,
                .choices = CHOICES("OFF", "1 EV", "2 EV", "3 EV", "4 EV", "5 EV", "6 EV"),
                .help  = "Stop underexposing when at least 5% of the image gets",
                .help2 = "noisier than selected SNR => will clip more highlights.",
                .depends_on = DEP_MANUAL_ISO,
            },
            {
                .name = "Link to Canon shutter",
                .priv = &auto_ettr_adjust_mode,
                .max = 1,
                .help = "Hack to adjust slowest shutter from main dial.",
                .advanced = 1,
            },
            {
                .name = "Link to Dual ISO",
                .priv = &auto_ettr_dual_iso_link,
                .max = 1,
                .help  = "Let ETTR change DualISO settings so you get the SNR values",
                .help2 = "in mids & shadows. It will disable dual ISO if not needed.",
            },
            {
                .name = "Show metered areas",
                .priv = &show_metered_areas,
                .max = 1,
                .help =  "Show where the white point and the SNR levels are metered",
                .help2 = "(what exactly is considered highlight, midtone and shadow).",
                .advanced = 1,
            },
            {
                .name = "Allow beeps",
                .priv = &auto_ettr_allow_beeps,
                .max = 1,
                .help =  "Make status beeps (1 = OK, 2 = need more pictures, 3 = error).",
                .advanced = 1,
            },
            {
                .name = "Show debug info",
                .priv = &debug_info,
                .select = debug_info_toggle,
                .max = 1,
                .help = "For camera nerds.",
                .advanced = 1,
            },
            MENU_ADVANCED_TOGGLE,
            MENU_EOL,
        },
    },
};

static unsigned int ettr_init()
{
    if ((void*)&raw_lv_request == (void*)&ret_0)
    {
        auto_ettr_trigger  = auto_ettr_trigger > 1 ? 0 : auto_ettr_trigger;
        ettr_menu[0].children[0].max = 1;
    }
    menu_add("Expo", ettr_menu, COUNT(ettr_menu));
    return 0;
}

static unsigned int ettr_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(ettr_init)
    MODULE_DEINIT(ettr_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_VSYNC_SETPARAM, auto_ettr_vsync_cbr, 0)
    MODULE_CBR(CBR_KEYPRESS, auto_ettr_keypress_cbr, 0)
    MODULE_CBR(CBR_SHOOT_TASK, auto_ettr_polling_cbr, 0)
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_GUI_STATE)
MODULE_PROPHANDLERS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(auto_ettr)
    MODULE_CONFIG(auto_ettr_trigger)
    MODULE_CONFIG(auto_ettr_ignore)
    MODULE_CONFIG(auto_ettr_target_level)
    MODULE_CONFIG(auto_ettr_max_shutter)
    MODULE_CONFIG(auto_ettr_clip)
    MODULE_CONFIG(auto_ettr_adjust_mode)
    MODULE_CONFIG(auto_ettr_midtone_snr_limit)
    MODULE_CONFIG(auto_ettr_shadow_snr_limit)
    MODULE_CONFIG(auto_ettr_dual_iso_link)
    MODULE_CONFIG(auto_ettr_allow_beeps)
MODULE_CONFIGS_END()
