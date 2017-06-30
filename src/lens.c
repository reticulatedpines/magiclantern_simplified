/** \file
 * Lens focus and zoom related things
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */


#include "dryos.h"
#include "lens.h"
#include "property.h"
#include "bmp.h"
#include "config.h"
#include "menu.h"
#include "math.h"
#include "version.h"
#include "module.h"
#include "raw.h"
#include "zebra.h"
#include "cropmarks.h"
#include "battery.h"
#include "lens.h"
#include "shoot.h"
#include "hdr.h"
#include "fps.h"
#include "picstyle.h"
#include "focus.h"
#include "lvinfo.h"
#include "powersave.h"

// for movie logging
static char* mvr_logfile_buffer = 0;
/* delay to be waited after mirror is locked */
CONFIG_INT("mlu.lens.delay", lens_mlu_delay, 7);
static void update_stuff();

//~ extern struct semaphore * bv_sem;
void bv_update_lensinfo();
void bv_auto_update();
static void lensinfo_set_aperture(int raw);
static void bv_expsim_shift();

static CONFIG_INT("movie.log", movie_log, 0);
#ifdef CONFIG_FULLFRAME
#define SENSORCROPFACTOR 10
#define crop_info 0
#elif defined(CONFIG_600D)
static PROP_INT(PROP_DIGITAL_ZOOM_RATIO, digital_zoom_ratio);
#define DIGITAL_ZOOM ((is_movie_mode() && video_mode_crop && video_mode_resolution == 0) ? digital_zoom_ratio : 100)
#define SENSORCROPFACTOR (16 * DIGITAL_ZOOM / 100)
CONFIG_INT("crop.info", crop_info, 0);
#else
#define SENSORCROPFACTOR 16
CONFIG_INT("crop.info", crop_info, 0);
#endif

#define FOCUS_UNITS_METRIC 0
#define FOCUS_UNITS_IMPERIAL 1
CONFIG_INT("focus.units", focus_units, FOCUS_UNITS_METRIC);

//~ static struct semaphore * lens_sem;
static struct semaphore * focus_done_sem;
//~ static struct semaphore * job_sem;


struct lens_info lens_info = {
    .name        = "NO LENS NAME"
};

const char * lens_format_dist( unsigned mm)
{
   static char dist[ 32 ];
    
    if( mm > 100000 ) //100 m
    {
        snprintf( dist, sizeof(dist), SYM_INFTY);
    }
    else if(focus_units == FOCUS_UNITS_IMPERIAL)
    {
        int inches = (mm * 10 / 254);
        if( inches > 24 ) // 2 ft
        {
            snprintf( dist, sizeof(dist), "%dft", (inches + 6) / 12); //+6 to round properly
        }
        else
        {
            snprintf( dist, sizeof(dist),"%din", inches);
        }
    }
    else
    {
        if ( mm >= 10000 ) // 10 m
        {
            snprintf( dist, sizeof(dist), "%d"SYM_SMALL_M, mm / 1000);
        }
        else if( mm >= 1000 ) // 1 m
        {
            int meters_x100 = mm / 10;
            snprintf( dist, sizeof(dist), "%s%d.%02d"SYM_SMALL_M, FMT_FIXEDPOINT2(meters_x100));
        }
        else
        {
            snprintf( dist, sizeof(dist),"%d"SYM_SMALL_C SYM_SMALL_M, mm / 10 );
        }
    }

   return (dist);
} /* end of aj_lens_format_dist() */

void
update_lens_display(int top, int bottom)
{
    if (top) draw_ml_topbar();
    if (bottom) draw_ml_bottombar();
    //~ lvinfo_display(top, bottom);
    //~ info_print_screen();
}

int should_draw_bottom_bar()
{
    if (gui_menu_shown()) return 1;
    if (!get_global_draw()) return 0;
    //~ if (EXT_MONITOR_CONNECTED) return 1;
    if (canon_gui_front_buffer_disabled()) return 1;
    if (is_canon_bottom_bar_dirty())
    {
        crop_set_dirty(5);
        afframe_set_dirty();
        return 0;
    }
    if (lv_disp_mode == 0) return 1;
    return 0;
}

int raw2shutter_ms(int raw_shutter)
{
    if (!raw_shutter) return 0;
    return (int) roundf(powf(2.0, (56.0f - raw_shutter)/8.0f) * 1000.0f);
}
int shutter_ms_to_raw(int shutter_ms)
{
    if (shutter_ms == 0) return 160;
    return (int) roundf(56.0f - log2f((float)shutter_ms / 1000.0f) * 8.0f);
}
int shutterf_to_raw(float shutterf)
{
    if (shutterf == 0) return 160;
    return (int) roundf(56.0f - log2f(shutterf) * 8.0f);
}

float raw2shutterf(int raw_shutter)
{
    if (!raw_shutter) return 0.0;
    return powf(2.0, (56.0 - raw_shutter)/8.0);
}

int raw2iso(int raw_iso)
{
    int iso = (int) roundf(100.0f * powf(2.0f, (raw_iso - 72.0f)/8.0f));
    if (iso >= 100 && iso <= 6400)
        iso = values_iso[raw2index_iso(raw_iso)];
    else if (raw_iso == 123)
        iso = 8000;
    else if (raw_iso == 125)
        iso = 10000;
    else if (raw_iso == 131)
        iso = 16000;
    else if (raw_iso == 133)
        iso = 20000;
    else if (iso > 100000)
        iso = ((iso+500)/1000) * 1000;
    else if (iso > 10000)
        iso = ((iso+50)/100) * 100;
    else if (iso >= 70 && iso < 80)
        iso = ((iso+5)/10) * 10;
    else if (iso >= 15)
        iso = ((iso+2)/5) * 5;
    else if (iso > 5)
        iso = iso&~1;
    return iso;
}

int expo_value_rounding_ok(int raw, int is_aperture)
{
    if (is_aperture)
    {
        /* exception: aperture limits are allowed, even if they are not multiples of 1/2 or 1/3 EV */
        if (raw == lens_info.raw_aperture_min || raw == lens_info.raw_aperture_max)
            return 1;
    }
    
    int r = ABS(raw) % 8;
    if (r != 0 && r != 3 && r != 4 && r != 5)
        return 0;
    return 1;
}

int round_shutter(int tv, int slowest_shutter)
{
    int tvr;
    tv = MIN(tv, FASTEST_SHUTTER_SPEED_RAW);

    /* note: it's possible to get a valid shutter just by altering the requested value by 1 */
    /* ... unless we hit some limits */
    tvr = MAX(tv    , slowest_shutter); if (expo_value_rounding_ok(tvr, 0)) return tvr;
    tvr = MAX(tv - 1, slowest_shutter); if (expo_value_rounding_ok(tvr, 0)) return tvr;
    tvr = MAX(tv + 1, slowest_shutter); if (expo_value_rounding_ok(tvr, 0)) return tvr;
    tvr = MAX(tv - 2, slowest_shutter); if (expo_value_rounding_ok(tvr, 0)) return tvr;
    tvr = MAX(tv + 2, slowest_shutter); if (expo_value_rounding_ok(tvr, 0)) return tvr;
    return 0;
}

int round_aperture(int av)
{
    int avr;

    /* note: if we don't hit the limits, we can find a valid aperture just by altering the requested value by 1 */
    /* but if we do hit them... surprises may happen :) */
    avr = COERCE(av    , lens_info.raw_aperture_min, lens_info.raw_aperture_max); if (expo_value_rounding_ok(avr, 1)) return avr;
    avr = COERCE(av - 1, lens_info.raw_aperture_min, lens_info.raw_aperture_max); if (expo_value_rounding_ok(avr, 1)) return avr;
    avr = COERCE(av + 1, lens_info.raw_aperture_min, lens_info.raw_aperture_max); if (expo_value_rounding_ok(avr, 1)) return avr;
    avr = COERCE(av - 2, lens_info.raw_aperture_min, lens_info.raw_aperture_max); if (expo_value_rounding_ok(avr, 1)) return avr;
    avr = COERCE(av + 2, lens_info.raw_aperture_min, lens_info.raw_aperture_max); if (expo_value_rounding_ok(avr, 1)) return avr;
    return 0;
}

int round_expo_comp(int ae)
{
    int aer;
    /* note: it's possible to get a valid exposure compensation just by altering the requested value by 1 */
    aer = COERCE(ae    , -MAX_AE_EV * 8, MAX_AE_EV * 8); if (expo_value_rounding_ok(aer, 0)) return aer;
    aer = COERCE(ae - 1, -MAX_AE_EV * 8, MAX_AE_EV * 8); if (expo_value_rounding_ok(aer, 0)) return aer;
    aer = COERCE(ae + 1, -MAX_AE_EV * 8, MAX_AE_EV * 8); if (expo_value_rounding_ok(aer, 0)) return aer;
    return 0;
}

int round_flash_expo_comp(int fec)
{
    int fecr;
    /* note: it's possible to get a valid exposure compensation just by altering the requested value by 1 */
    fecr = COERCE(fec    , FLASH_MIN_EV * 8, FLASH_MAX_EV * 8); if (expo_value_rounding_ok(fecr, 0)) return fecr;
    fecr = COERCE(fec - 1, FLASH_MIN_EV * 8, FLASH_MAX_EV * 8); if (expo_value_rounding_ok(fecr, 0)) return fecr;
    fecr = COERCE(fec + 1, FLASH_MIN_EV * 8, FLASH_MAX_EV * 8); if (expo_value_rounding_ok(fecr, 0)) return fecr;
    return 0;
}

char* get_shootmode_name(int shooting_mode)
{
    return
        is_movie_mode() ?                       
            (
                shooting_mode == SHOOTMODE_C  ? "MovieC1" :
                shooting_mode == SHOOTMODE_C2 ? "MovieC2" :
                shooting_mode == SHOOTMODE_C3 ? "MovieC3" :
                                                "Movie"
            ) :
        shooting_mode == SHOOTMODE_P ?          "P" :
        shooting_mode == SHOOTMODE_M ?          "M" :
        shooting_mode == SHOOTMODE_TV ?         "Tv" :
        shooting_mode == SHOOTMODE_AV ?         "Av" :
        shooting_mode == SHOOTMODE_CA ?         "CA" :
        shooting_mode == SHOOTMODE_AP ?         "A+" :
        shooting_mode == SHOOTMODE_ADEP ?       "ADEP" :
        shooting_mode == SHOOTMODE_AUTO ?       "Auto" :
        shooting_mode == SHOOTMODE_LANDSCAPE ?  "Landscape" :
        shooting_mode == SHOOTMODE_PORTRAIT ?   "Portrait" :
        shooting_mode == SHOOTMODE_NOFLASH ?    "NoFlash" :
        shooting_mode == SHOOTMODE_MACRO ?      "Macro" :
        shooting_mode == SHOOTMODE_SPORTS ?     "Sports" :
        shooting_mode == SHOOTMODE_NIGHT ?      "Night" :
        shooting_mode == SHOOTMODE_NIGHTH ?     "Night Handheld" :
        shooting_mode == SHOOTMODE_HDR ?        "HDR Backlight" :
        shooting_mode == SHOOTMODE_BULB ?       "Bulb" :
        shooting_mode == SHOOTMODE_C ?          "C1" :
        shooting_mode == SHOOTMODE_C2 ?         "C2" :
        shooting_mode == SHOOTMODE_C3 ?         "C3" :
                                                "Unknown";
}

char* get_shootmode_name_short(int shooting_mode)
{
    return
        is_movie_mode() ?                       
            (
                shooting_mode == SHOOTMODE_C  ? "Mv1" :
                shooting_mode == SHOOTMODE_C2 ? "Mv2" :
                shooting_mode == SHOOTMODE_C3 ? "Mv3" :
                                                "Mv"
            ) :
        shooting_mode == SHOOTMODE_P ?          "P"  :
        shooting_mode == SHOOTMODE_M ?          "M"  :
        shooting_mode == SHOOTMODE_TV ?         "Tv" :
        shooting_mode == SHOOTMODE_AV ?         "Av" :
        shooting_mode == SHOOTMODE_CA ?         "CA" :
        shooting_mode == SHOOTMODE_AP ?         "A+" :
        shooting_mode == SHOOTMODE_ADEP ?       "AD" :
        shooting_mode == SHOOTMODE_AUTO ?       "[]" :
        shooting_mode == SHOOTMODE_LANDSCAPE ?  "LD" :
        shooting_mode == SHOOTMODE_PORTRAIT ?   ":)" :
        shooting_mode == SHOOTMODE_NOFLASH ?    "NF" :
        shooting_mode == SHOOTMODE_MACRO ?      "MC" :
        shooting_mode == SHOOTMODE_SPORTS ?     "SP" :
        shooting_mode == SHOOTMODE_NIGHT ?      "NI" :
        shooting_mode == SHOOTMODE_NIGHTH ?     "NH" :
        shooting_mode == SHOOTMODE_HDR ?        "HB" :
        shooting_mode == SHOOTMODE_BULB ?       "B"  :
        shooting_mode == SHOOTMODE_C ?          "C1" :
        shooting_mode == SHOOTMODE_C2 ?         "C2" :
        shooting_mode == SHOOTMODE_C3 ?         "C3" :
                                                "?"  ;
}

int FAST get_ml_topbar_pos()
{
    const int bar_height = 32;
    int bmp_ymax = (hdmi_code >= 5) ? 510 : 480;

    if (gui_menu_shown())
    {
        return (hdmi_code >= 5) ? 40 : 2; // force it at the top of menu
    }
    else
    {
        int screen_layout = get_screen_layout();

        switch (screen_layout)
        {
            case SCREENLAYOUT_16_9:
                return os.y0 + os.off_169 + 2; // meters just below 16:9 border

            case SCREENLAYOUT_16_10:
                return os.y0 + os.off_1610 + 2; // meters just below 16:9 border

            case SCREENLAYOUT_UNDER_3_2:
                return MIN(os.y_max + 2, bmp_ymax - 2*bar_height);

            case SCREENLAYOUT_UNDER_16_9:
                return MIN(os.y_max - os.off_169 + 4, bmp_ymax - 2*bar_height);

            default:
                return os.y0 + 2; // just above the 16:9 frame
        }
    }
}

int FAST get_ml_bottombar_pos()
{
    const int bar_height = 32;

    if (gui_menu_shown())
    {
        return 480 + (hdmi_code >= 5 ? 40 : 0) - bar_height; // force it at the bottom of menu
    }
    else
    {
        int screen_layout = get_screen_layout();

        switch (screen_layout)
        {
            case SCREENLAYOUT_16_9:
                return os.y_max - os.off_169 - bar_height;

            case SCREENLAYOUT_16_10:
                return os.y_max - os.off_1610 - bar_height;

            case SCREENLAYOUT_UNDER_3_2:
            case SCREENLAYOUT_UNDER_16_9:
                return get_ml_topbar_pos() + bar_height;

            default:
                return os.y_max - bar_height - 2; // just above the 16:9 frame
        }
    }
}

void draw_ml_bottombar()
{
    if (!should_draw_bottom_bar()) return;

    lvinfo_display(0,1);
}

static int round_nicely(int x, int digits)
{
    int x0 = x;
    
    /* round X to N significant digits */
    /* e.g. 1234 rounded to 2 digits => 1200 */
    int thr = powi(10, digits);
    
    if (x < thr/10) return x;
    
    int f = 1;
    while (x >= thr)
    {
        if ((x + 2) / 5 == 25)
        {
            /* exception: allow 125, 1250 and so on, because Canon does it as well */
            x = (x + 2) / 5;
            f *= 5;
        }
        else
        {
            x = (x + 5) / 10;
            f *= 10;
        }
    }
    
    /* re-round to cancel accumulated errors, if any */
    x = (x0 + f/2) / f;
    
    /* avoid ending in odd digits if the number is large (e.g. allow 11, 13, 19, 21, but don't allow 61 or 79) */
    int last_digit = x % 10;
    int next_digit = (x / 10) % 10;
    if ((last_digit == 1 || last_digit == 3 || last_digit == 7 || last_digit == 9) && next_digit >= 5)
    {
        f *= 2;
        x = (x0 + f/2) / f;
    }
    
    return x * f;
}

// Pretty prints the shutter speed given the shutter reciprocal (times 1000) as input
// To be used in movie mode; it doesn't try too hard to be consistent with Canon values
char* lens_format_shutter_reciprocal(int shutter_reciprocal_x1000, int digits)
{
    static char shutter[32];
    if (shutter_reciprocal_x1000 == 0)
    {
        snprintf(shutter, sizeof(shutter), "N/A");
    }
    else if (shutter_reciprocal_x1000 >= 10000000)
    {
        snprintf(shutter, sizeof(shutter), SYM_1_SLASH"%dK", (shutter_reciprocal_x1000+500000)/1000000);
    }
    else if (shutter_reciprocal_x1000 > 24000)
    {
        int shutter_rounded = round_nicely(shutter_reciprocal_x1000, digits);
        
        if (digits <= 2)
        {
            snprintf(shutter, sizeof(shutter), SYM_1_SLASH"%d", shutter_rounded/1000);
        }
        else
        {
            /* todo: compute how many digits should be after the decimal point? */
            shutter_rounded = (shutter_rounded + 5) / 10;
            snprintf(shutter, sizeof(shutter), SYM_1_SLASH"%s%d.%02d", FMT_FIXEDPOINT2(shutter_rounded));
        }
    }
    else if (shutter_reciprocal_x1000 > 3000)
    {
        snprintf(shutter, sizeof(shutter), SYM_1_SLASH"%d", (shutter_reciprocal_x1000+500)/1000);
    }
    else
    {
        int shutter_x10 = (100000/shutter_reciprocal_x1000+5)/10;
        if (shutter_x10 % 10 && shutter_x10 < 40)
            snprintf(shutter, sizeof(shutter), "%d.%d\"", shutter_x10 / 10, shutter_x10 % 10);
        else
            snprintf(shutter, sizeof(shutter), "%d\"", (shutter_x10+5) / 10);
    }
    return shutter;
}

// Pretty prints the shutter speed given the raw shutter value as input
// To be used in photo mode; it will try to be somewhat consistent with Canon values
char* lens_format_shutter(int tv)
{
    static char shutter[32];
    if(tv >= 70 && tv - 15 < COUNT(values_shutter))
    {
        snprintf(shutter, sizeof(shutter), SYM_1_SLASH"%d", values_shutter[tv-15]);
    }
    else if(tv >= 15 && tv < 70)
    {
        uint16_t value = values_shutter[tv-15];
        if(value % 10 != 0)
        {
            snprintf(shutter, sizeof(shutter), "%d.%d\"", value / 10, value % 10);
        }
        else
        {
            snprintf(shutter, sizeof(shutter), "%d\"", value / 10);
        }
    }
    else if (tv == SHUTTER_BULB)
    {
        snprintf(shutter, sizeof(shutter), "BULB");
    }
    else
    {
        //this should never happen, but if it does, just print the raw value
        snprintf(shutter, sizeof(shutter), "RAW:%d", tv);
    }
    return shutter;
}

char* lens_format_aperture(int raw_aperture)
{
    int f = RAW2VALUE(aperture, raw_aperture);
    
    static char aperture[32];
    if (f < 100)
    {
        snprintf(aperture, sizeof(aperture), SYM_F_SLASH"%d.%d", f / 10, f % 10);
    }
    else
    {
        snprintf(aperture, sizeof(aperture), SYM_F_SLASH"%d", f / 10);
    }
    return aperture;
}

void free_space_show_photomode()
{
    int free_space_32k = get_free_space_32k(get_shooting_card());

    int fsg = free_space_32k >> 15;
    int fsgr = free_space_32k - (fsg << 15);
    int fsgf = (fsgr * 10) >> 15;

    int time_indic_x = 720 - 160;
    int x = time_indic_x + 2 * font_med.width;
    int y =  452;
    bmp_printf(
        FONT(SHADOW_FONT(FONT_LARGE), COLOR_FG_NONLV, bmp_getpixel(x-10,y+10)),
        x, y,
        "%d.%dGB",
        fsg,
        fsgf
    );
}

void draw_ml_topbar()
{
    if (!get_global_draw()) return;
    
    lvinfo_display(1,0);
}

static volatile int lv_focus_requests = 0;
static volatile int lv_focus_done = 1;
static volatile int lv_focus_error = 0;

PROP_HANDLER( PROP_LV_FOCUS_DONE )
{
    /* turn off the LED we enabled in lens_focus */
    info_led_off();
    
    lv_focus_requests = 0;

    //~ bmp_printf(FONT_MED, 50, 100, "Focus status: 0x%x  ", buf[0]);
    
    if (buf[0] & 0x1000) 
    {
        NotifyBox(1000, "Focus: soft limit reached");
        lv_focus_error = 1;
    }
    else
    {
        /* assume all is fine (not sure if correct, but seems to work) */
        lv_focus_done = 1;
    }
}

static void
lens_focus_wait(void)
{
    for (int i = 0; i < 100; i++)
    {
        if (lv_focus_done) return;
        if (lv_focus_error) return;
        if (!lv) return;
        if (is_manual_focus()) return;
        msleep(10);
    }
    
    NotifyBox(1000, "Focus not confirmed");
    lv_focus_error = 1;
}

// this is compatible with all cameras so far, but allows only 3 speeds
int
lens_focus(
    int num_steps, 
    int stepsize, 
    int wait,
    int extra_delay
)
{
    lv_focus_done = 0;
    lv_focus_error = 0;

    if (!lv) return 0;
    if (is_manual_focus()) return 0;
    if (lens_info.job_state) return 0;

    if (num_steps < 0)
    {
        num_steps = -num_steps;
        stepsize = -stepsize;
    }

    stepsize = COERCE(stepsize, -3, 3);
    int focus_cmd = stepsize;
    if (stepsize < 0) focus_cmd = 0x8000 - stepsize;
    
    for (int i = 0; i < num_steps; i++)
    {
        if (lv && !mirror_down && lens_info.job_state == 0)
        {
            if (wait)
            {
                lv_focus_done = 0;
                
                /* request and wait for confirmation */
                info_led_on();
                prop_request_change_wait(PROP_LV_LENS_DRIVE_REMOTE, &focus_cmd, 4, 1000);
                
                /* also wait for confirmation from PROP_LV_FOCUS_DONE */
                lens_focus_wait();
                
                /* also wait a little more if user want so (for really stubborn lenses) */
                if (extra_delay)
                {
                    msleep(extra_delay);
                }
            }
            else
            {
                /* keep sending focus commands, but not more than 3
                 * from the last PROP_LV_FOCUS_DONE event
                 * this fixes some crashes that appear to be caused
                 * by filling Canon's focus request queue with our junk
                 * 
                 * (not sure what their buffer size is, but with 5, it crashes)
                 */
                lv_focus_requests++;
                
                if (lv_focus_requests < 3)
                {
                    info_led_on();
                    prop_request_change(PROP_LV_LENS_DRIVE_REMOTE, &focus_cmd, 4);
                }

                /* open-loop delay, without waiting for confirmation; at least 10ms */
                msleep(MAX(10, extra_delay));
            }
        }
    }

    #ifdef FEATURE_MAGIC_ZOOM
    if (get_zoom_overlay_trigger_by_focus_ring()) zoom_overlay_set_countdown(300);
    #endif

    idle_wakeup_reset_counters(-10);
    lens_display_set_dirty();
    
    /* return 1 on success, 0 on error */
    return lv_focus_error ? 0 : 1;
}

void lens_wait_readytotakepic(int wait)
{
    int i;
    for (i = 0; i < wait * 20; i++)
    {
        if (ml_shutdown_requested) return;
        if (sensor_cleaning) { msleep(50); continue; }
        if (shooting_mode == SHOOTMODE_M && lens_info.raw_shutter == 0) { msleep(50); continue; }
        if (job_state_ready_to_take_pic() && burst_count > 0 && ((icu_uilock & 0xFF) == 0)) break;
        msleep(50);
        if (NOT_RECORDING) info_led_on();
    }
    if (NOT_RECORDING) info_led_off();
}

static int mirror_locked = 0;
int mlu_lock_mirror_if_needed() // called by lens_take_picture; returns 0 if success, 1 if camera took a picture instead of locking mirror
{
    #ifdef CONFIG_5DC
    if (get_mlu()) set_mlu(0); // can't trigger shutter with MLU active, so just turn it off
    return 0;
    #endif
    
    if (drive_mode == DRIVE_SELFTIMER_2SEC || drive_mode == DRIVE_SELFTIMER_REMOTE || drive_mode == DRIVE_SELFTIMER_CONTINUOUS)
        return 0;
    
    if (get_mlu() && CURRENT_GUI_MODE)
    {
        SetGUIRequestMode(0);
        int iter = 20;
        while (iter-- && !display_idle())
            msleep(50); 
        msleep(500);
    }

    //~ NotifyBox(1000, "MLU locking");
    if (get_mlu() && !lv)
    {
        if (!mirror_locked)
        {
            int fn = get_shooting_card()->file_number;
            
            #if defined(CONFIG_5D2) || defined(CONFIG_50D)
            SW1(1,50);
            SW2(1,250);
            SW2(0,50);
            SW1(0,50);
            #elif defined(CONFIG_40D)
            call("FA_Release");
            #else
            call("Release");
            #endif
            
            msleep(500);
            if (get_shooting_card()->file_number != fn) // Heh... camera took a picture instead. Cool.
                return 1;

            if (lv) // we have somehow got into LiveView, where MLU does nothing... so, no need to wait
                return 0;

            mirror_locked = 1;
            
            msleep(MAX(0, get_mlu_delay(lens_mlu_delay) - 500));
        }
    }
    //~ NotifyBox(1000, "MLU locked");
    return 0;
}

#define AF_BUTTON_NOT_MODIFIED 100
static int orig_af_button_assignment = AF_BUTTON_NOT_MODIFIED;

// to preview AF patterns
void assign_af_button_to_halfshutter()
{
    if (ml_shutdown_requested) return;
    if (orig_af_button_assignment == AF_BTN_HALFSHUTTER) return;
    //~ take_semaphore(lens_sem, 0);
    lens_wait_readytotakepic(64);
    if (ml_shutdown_requested) return;
    if (orig_af_button_assignment == AF_BUTTON_NOT_MODIFIED) orig_af_button_assignment = cfn_get_af_button_assignment();
    cfn_set_af_button(AF_BTN_HALFSHUTTER);
    //~ give_semaphore(lens_sem);
}

// to prevent AF
void assign_af_button_to_star_button()
{
    if (ml_shutdown_requested) return;
    if (orig_af_button_assignment == AF_BTN_STAR) return;
    //~ take_semaphore(lens_sem, 0);
    lens_wait_readytotakepic(64);
    if (ml_shutdown_requested) return;
    if (orig_af_button_assignment == AF_BUTTON_NOT_MODIFIED) orig_af_button_assignment = cfn_get_af_button_assignment();
    cfn_set_af_button(AF_BTN_STAR);
    //~ give_semaphore(lens_sem);
}

void restore_af_button_assignment()
{
    if (orig_af_button_assignment != AF_BUTTON_NOT_MODIFIED)
        orig_af_button_assignment = COERCE(orig_af_button_assignment, 0, 10); // just in case, so we don't read invalid values from config file
    
    if (orig_af_button_assignment == AF_BUTTON_NOT_MODIFIED) return;
    //~ take_semaphore(lens_sem, 0);
    lens_wait_readytotakepic(64);
    cfn_set_af_button(orig_af_button_assignment);
    msleep(100);
    if (cfn_get_af_button_assignment() == (int)orig_af_button_assignment)
        orig_af_button_assignment = AF_BUTTON_NOT_MODIFIED; // success
    //~ give_semaphore(lens_sem);
}

// keep retrying until it succeeds, or until the 3-second timeout expires
void restore_af_button_assignment_at_shutdown()
{
    for (int i = 0; i < 30; i++)
    {
        if (orig_af_button_assignment == AF_BUTTON_NOT_MODIFIED)
            break;
        restore_af_button_assignment();
        info_led_blink(1,50,50);
    }
}

int ml_taking_pic = 0;

int lens_setup_af(int should_af)
{
    ASSERT(should_af != AF_DONT_CHANGE);
    
    if (!is_manual_focus())
    {
        if (should_af == AF_ENABLE) assign_af_button_to_halfshutter();
        else if (should_af == AF_DISABLE) assign_af_button_to_star_button();
        else return 0;
        
        return 1;
    }
    return 0;
}
void lens_cleanup_af()
{
    restore_af_button_assignment();
}

int
lens_take_picture(
    int wait, 
    int should_af
)
{
    if (ml_taking_pic) return -1;
    ml_taking_pic = 1;

    if (should_af != AF_DONT_CHANGE) lens_setup_af(should_af);
    //~ take_semaphore(lens_sem, 0);
    lens_wait_readytotakepic(64);
    
    // in some cases, the MLU setting is ignored; if ML can't detect this properly, this call will actually take a picture
    // if it happens (e.g. with LV active, but camera in QR mode), that's it, we won't try taking another one
    // side effects should be minimal
#if defined(CONFIG_EOSM)
    call("Release"); //EOSM is mirrorless no need to check for MLU
    goto end;
#else
    int took_pic = mlu_lock_mirror_if_needed();
    if (took_pic) goto end;
#endif
    
    #if defined(CONFIG_5D2) || defined(CONFIG_50D)
    if (get_mlu())
    {
        SW1(1,50);
        SW2(1,250);
        SW2(0,50);
        SW1(0,50);
    }
    else
    {
        #ifdef CONFIG_5D2
        int status = 0;
        PtpDps_remote_release_SW1_SW2_worker(&status);
        #else
        call("Release");
        #endif
    }
    #elif defined(CONFIG_5DC)
    call("rssRelease");
    #elif defined(CONFIG_40D)
    call("FA_Release");
    #else
    call("Release");
    #endif
    
    #if defined(CONFIG_7D)
    /* on EOS 7D the code to trigger SW1/SW2 is buggy that the metering somehow locks up when exposure time is >1.x seconds.
     * This causes the camera not to shut down when the card door is opened.
     * There is a workaround: Just wait until shooting is possible again and then reset SW1.
     * Then the camera will shut down clean.
     */
    lens_wait_readytotakepic(64);
    SW1(0,0);
    #endif

end:
    if( !wait )
    {
        //~ give_semaphore(lens_sem);
        if (should_af != AF_DONT_CHANGE) lens_cleanup_af();
        ml_taking_pic = 0;
        return 0;
    }
    else
    {
        msleep(200);

        if (drive_mode == DRIVE_SELFTIMER_2SEC) msleep(2000);
        if (drive_mode == DRIVE_SELFTIMER_REMOTE || drive_mode == DRIVE_SELFTIMER_CONTINUOUS) msleep(10000);

        lens_wait_readytotakepic(wait);
        //~ give_semaphore(lens_sem);
        if (should_af != AF_DONT_CHANGE) lens_cleanup_af();
        ml_taking_pic = 0;
        return lens_info.job_state;
    }
}

#ifdef FEATURE_MOVIE_LOGGING

/** Write the current lens info into the logfile */
static void
mvr_update_logfile(
    struct lens_info *    info,
    int            force
)
{
    if( mvr_logfile_buffer == 0 )
        return;

    static unsigned last_iso;
    static unsigned last_shutter;
    static unsigned last_aperture;
    static unsigned last_focal_len;
    static unsigned last_focus_dist;
    static int last_second;

    // Check if nothing changed and not forced.  Do not write.
    if( !force
    &&  last_iso        == info->iso
    &&  last_shutter    == info->shutter
    &&  last_aperture    == info->aperture
    &&  last_focal_len    == info->focal_len
    &&  last_focus_dist    == info->focus_dist
    )
        return;
    
    // Don't update more often than once per second
    if (!force
    && last_second == get_seconds_clock()
    )
        return;

    // Record the last settings so that we know if anything changes
    last_iso    = info->iso;
    last_shutter    = info->shutter;
    last_aperture    = info->aperture;
    last_focal_len    = info->focal_len;
    last_focus_dist    = info->focus_dist;
    last_second = get_seconds_clock();

    struct tm now;
    LoadCalendarFromRTC( &now );

    MVR_LOG_APPEND (
        "%02d:%02d:%02d,%d,%d,%d.%d,%d,%d\n",
        now.tm_hour,
        now.tm_min,
        now.tm_sec,
        info->iso,
        info->shutter,
        info->aperture / 10,
        info->aperture % 10,
        info->focal_len,
        info->focus_dist
    );
}

/** Create a logfile for each movie.
 * Record a logfile with the lens info for each movie.
 */
static void
mvr_create_logfile(
    unsigned        event
)
{
    if (!movie_log) return;

    if( event == 0 )
    {
        // Movie stopped - write the log file
        char name[100];
        snprintf(name, sizeof(name), "%s/MVI_%04d.LOG", get_dcim_dir(), get_shooting_card()->file_number);

        FILE * mvr_logfile = mvr_logfile = FIO_CreateFile( name );
        if (!mvr_logfile)
        {
            bmp_printf( FONT_MED, 0, 40,
                "Unable to create movie log! fd=%x\n%s",
                (unsigned) mvr_logfile,
                name
            );
            return;
        }

        FIO_WriteFile( mvr_logfile, mvr_logfile_buffer, strlen(mvr_logfile_buffer) );

        FIO_CloseFile( mvr_logfile );
        
        fio_free(mvr_logfile_buffer);
        mvr_logfile_buffer = 0;
        return;
    }

    if( event != 2 )
        return;

    // Movie starting
    mvr_logfile_buffer = fio_malloc(MVR_LOG_BUF_SIZE);

    snprintf( mvr_logfile_buffer, MVR_LOG_BUF_SIZE,
        "# Magic Lantern %s\n\n",
        build_version
    );

    struct tm now;
    LoadCalendarFromRTC( &now );

    MVR_LOG_APPEND (
        "Start          : %4d/%02d/%02d %02d:%02d:%02d\n",
        now.tm_year + 1900,
        now.tm_mon + 1,
        now.tm_mday,
        now.tm_hour,
        now.tm_min,
        now.tm_sec
    );

    MVR_LOG_APPEND (
        "Lens name      : %s\n", lens_info.name 
    );

    int sr_x1000 = get_current_shutter_reciprocal_x1000();

    MVR_LOG_APPEND (
        "ISO            : %d%s\n"
        "Shutter        : 1/%d.%03ds\n"
        "Aperture       : f/%d.%d\n"
        "Focal length   : %d mm\n"
        "Focus distance : %d mm\n",
        lens_info.iso, get_htp() ? " D+" : "",
        sr_x1000/1000, sr_x1000%1000,
        lens_info.aperture / 10, lens_info.aperture % 10,
        lens_info.focal_len,
        lens_info.focus_dist * 10
    );

    MVR_LOG_APPEND (
        "White Balance  : %d%s, %s %d, %s %d\n",
        lens_info.wb_mode == WB_KELVIN ? lens_info.kelvin : lens_info.wb_mode,
        lens_info.wb_mode == WB_KELVIN ? "K" : 
        lens_info.wb_mode == 0 ? " - Auto" : 
        lens_info.wb_mode == 1 ? " - Sunny" :
        lens_info.wb_mode == 2 ? " - Cloudy" : 
        lens_info.wb_mode == 3 ? " - Tungsten" : 
        lens_info.wb_mode == 4 ? " - Fluorescent" : 
        lens_info.wb_mode == 5 ? " - Flash" : 
        lens_info.wb_mode == 6 ? " - Custom" : 
        lens_info.wb_mode == 8 ? " - Shade" : " - unknown",
        lens_info.wbs_gm > 0 ? "Green" : "Magenta", ABS(lens_info.wbs_gm), 
        lens_info.wbs_ba > 0 ? "Amber" : "Blue", ABS(lens_info.wbs_ba)
        );

    #ifdef FEATURE_PICSTYLE
    MVR_LOG_APPEND (
        "Picture Style  : %s (%d,%d,%d,%d)\n", 
        get_picstyle_name(lens_info.raw_picstyle), 
        lens_get_sharpness(),
        lens_get_contrast(),
        ABS(lens_get_saturation()) < 10 ? lens_get_saturation() : 0,
        ABS(lens_get_color_tone()) < 10 ? lens_get_color_tone() : 0
        );
    #endif

    /* todo: refactor these with callbacks (these calls are private) */
    extern void fps_mvr_log(char* mvr_logfile_buffer);
    extern void hdr_mvr_log(char* mvr_logfile_buffer);
    extern void bitrate_mvr_log(char* mvr_logfile_buffer);
    fps_mvr_log(mvr_logfile_buffer);
    hdr_mvr_log(mvr_logfile_buffer);
    bitrate_mvr_log(mvr_logfile_buffer);
    
    MVR_LOG_APPEND (
        "\n\nCSV data:\n%s\n",
        "Time,ISO,Shutter,Aperture,Focal_Len,Focus_Dist"
    );

    // Force the initial values to be written
    mvr_update_logfile( &lens_info, 1 );
}
#endif

static inline uint16_t
bswap16(
    uint16_t        val
)
{
    return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}

PROP_HANDLER( PROP_MVR_REC_START )
{
    /* there might be a false trigger at startup - issue #1992 */
    extern int ml_started;
    if (!ml_started) return;

    mvr_rec_start_shoot(buf[0]);
    
    #ifdef FEATURE_MOVIE_LOGGING
    mvr_create_logfile( *(unsigned*) buf );
    #endif
}


PROP_HANDLER( PROP_LENS_NAME )
{
    if( len > sizeof(lens_info.name) )
        len = sizeof(lens_info.name);
    memcpy( (char*)lens_info.name, buf, len );
}

PROP_HANDLER(PROP_LENS)
{
    uint8_t* info = (uint8_t *) buf;
    
    #ifdef CONFIG_5DC
    lens_info.lens_exists = 0;
    lens_info.raw_aperture_min = info[2];
    lens_info.raw_aperture_max = info[3];
    lens_info.lens_id = 0;
    lens_info.lens_focal_min = 0;
    lens_info.lens_focal_max = 0;
    lens_info.lens_extender = 0;
    lens_info.lens_version = 0;
    lens_info.lens_capabilities = 0;
    #else
    lens_info.lens_exists = info[0];
    lens_info.raw_aperture_min = info[1];
    lens_info.raw_aperture_max = info[2];
    lens_info.lens_id = (info[3] << 8) | info[4];
    lens_info.lens_focal_min = (info[5] << 8) | info[6];
    lens_info.lens_focal_max = (info[7] << 8) | info[8];
    lens_info.lens_extender = info[0xE];
    
    /* not all models support this feature */
    if(len >= 0x1C)
    {
        lens_info.lens_version = (info[0x19] << 16) | (info[0x1A] << 8) | info[0x1B];
        lens_info.lens_capabilities = info[0x1C];
        
        /* not sure how big the lens serial is; exiftool shows 5 bytes in htmlDump */
        uint32_t lens_serial_lo = 
             info[0x18]        |
            (info[0x17] << 8)  |
            (info[0x16] << 16) |
            (info[0x15] << 24) ;
        uint32_t lens_serial_hi = 
             info[0x14]        ;
        lens_info.lens_serial = 
             (uint64_t) lens_serial_lo | 
            ((uint64_t) lens_serial_hi << 32);
    }
    else
    {
        lens_info.lens_version = 0;
        lens_info.lens_capabilities = 0;
    }
    #endif
    
    if (lens_info.raw_aperture < lens_info.raw_aperture_min || lens_info.raw_aperture > lens_info.raw_aperture_max)
    {
        int raw = COERCE(lens_info.raw_aperture, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        lensinfo_set_aperture(raw); // valid limits changed
    }
    
    //~ bv_update_lensinfo();
}

PROP_HANDLER(PROP_LV_LENS_STABILIZE)
{
    //~ NotifyBox(2000, "%x ", buf[0]);
    lens_info.IS = (buf[0] & 0x000F0000) >> 16; // not sure, but lower word seems to be AF/MF status
}

// it may be slow; if you need faster speed, replace this with a binary search or something better
#define RAWVAL_FUNC(param) \
int raw2index_##param(int raw) \
{ \
    int i; \
    for (i = 0; i < COUNT(codes_##param); i++) \
        if(codes_##param[i] >= raw) return i; \
    return 0; \
}\
\
int val2raw_##param(int val) \
{ \
    unsigned i; \
    for (i = 0; i < COUNT(codes_##param); i++) \
        if(values_##param[i] >= val) return codes_##param[i]; \
    return -1; \
}

RAWVAL_FUNC(iso)
RAWVAL_FUNC(shutter)
RAWVAL_FUNC(aperture)

static void lensinfo_set_iso(int raw)
{
    lens_info.raw_iso = raw;
    lens_info.iso = RAW2VALUE(iso, raw);
    update_stuff();
}

static void lensinfo_set_shutter(int raw)
{
    //~ bmp_printf(FONT_MED, 600, 100, "liss %d %d ", raw, caller);
    lens_info.raw_shutter = raw;
    lens_info.shutter = RAW2VALUE(shutter, raw);
    update_stuff();
}

static void lensinfo_set_aperture(int raw)
{
    if (raw)
    {
        if (lens_info.raw_aperture_min && lens_info.raw_aperture_max)
            raw = COERCE(raw, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        lens_info.raw_aperture = raw;
        lens_info.aperture = RAW2VALUE(aperture, raw);
    }
    else
    {
        lens_info.aperture = lens_info.raw_aperture = 0;
    }
    //~ BMP_LOCK( lens_info.aperture = (int)roundf(10.0 * sqrtf(powf(2.0, (raw-8.0)/8.0))); )
    update_stuff();
}

extern int bv_auto;

#if defined(CONFIG_NO_MANUAL_EXPOSURE_MOVIE) && !defined(CONFIG_NO_DEDICATED_MOVIE_MODE)
    /*
     * If we don't have manual exposure controls in movie mode, we need to use expo override (500D/1100D/50D).
     * 
     * In cameras with a dedicated movie mode (500D, 1100D), exposure properties are triggered
     * by Canon's auto exposure algorithm. Therefore, we can't sync expo override from properties.
     * 
     * If there is no dedicated movie mode on the mode dial (50D), we can simply enable ExpSim,
     * and this will not use any auto exposure algorithm from Canon.
     * Therefore, we can safely sync expo override from properties, just like on cameras with manual exposure controls.
     * 
     */

    #define CONFIG_MOVIE_EXPO_OVERRIDE_DISABLE_SYNC_WITH_PROPS
#endif

static int iso_ack = -1;
PROP_HANDLER( PROP_ISO )
{
    if (!CONTROL_BV) lensinfo_set_iso(buf[0]);
    #ifdef FEATURE_EXPO_OVERRIDE
    else if 
        (
            buf[0] && !gui_menu_shown()
            #if defined(ISO_ADJUSTMENT_ACTIVE) || defined(CONFIG_MOVIE_EXPO_OVERRIDE_DISABLE_SYNC_WITH_PROPS)
            && ISO_ADJUSTMENT_ACTIVE
            #endif
        )
    {
        /* when you adjust ISO from Canon menu, sync expo override too */
        /* this should work even on cameras without manual exposure control, since it's safeguarded by ISO_ADJUSTMENT_ACTIVE */
        /* (that's why ISO_ADJUSTMENT_ACTIVE is mandatory for cameras with CONFIG_NO_MANUAL_EXPOSURE_MOVIE, and optional on others) */
        bv_set_rawiso(buf[0]);
    }
    bv_auto_update();
    #endif
    lens_display_set_dirty();
    iso_ack = buf[0];
}

void iso_auto_restore_hack()
{
    if (iso_ack == 0) lensinfo_set_iso(0);
}

PROP_HANDLER( PROP_ISO_AUTO )
{
    uint32_t raw = *(uint32_t *) buf;

    #if defined(FRAME_ISO)
    if (lv && is_movie_mode()) raw = (uint8_t)FRAME_ISO;
    #endif

    lens_info.raw_iso_auto = raw;
    lens_info.iso_auto = RAW2VALUE(iso, raw);

    update_stuff();
}

#if defined(FRAME_ISO)
PROP_HANDLER( PROP_BV ) // camera-specific
{
    if (lv && is_movie_mode())
    {
        uint32_t raw_iso = (uint8_t)FRAME_ISO;

        if (raw_iso)
        {
            lens_info.raw_iso_auto = raw_iso;
            lens_info.iso_auto = RAW2VALUE(iso, raw_iso);
            update_stuff();
        }
    }
}
#endif

PROP_HANDLER( PROP_SHUTTER )
{
    if (!CONTROL_BV) 
    {
        if (shooting_mode != SHOOTMODE_AV && shooting_mode != SHOOTMODE_P)
            lensinfo_set_shutter(buf[0]);
    }
    #ifdef FEATURE_EXPO_OVERRIDE
    else if (buf[0]  // sync expo override to Canon values
            && (ABS(buf[0] - lens_info.raw_shutter) > 3) // some cameras may attempt to round shutter value to 1/2 or 1/3 stops
                                                       // especially when pressing half-shutter

        #ifdef CONFIG_MOVIE_EXPO_OVERRIDE_DISABLE_SYNC_WITH_PROPS
        && !is_movie_mode()
        #endif
        #ifdef CONFIG_6D
        && !(buf[0] == FASTEST_SHUTTER_SPEED_RAW )
        #endif

        )
    {
        bv_set_rawshutter(buf[0]);
    }
    bv_auto_update();
    #endif
    lens_display_set_dirty();
}

PROP_HANDLER( PROP_APERTURE )
{
    //~ NotifyBox(2000, "%x %x %x %x ", buf[0], CONTROL_BV, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
    if (!CONTROL_BV)
    {
        lensinfo_set_aperture(buf[0]);
    }
    #ifdef FEATURE_EXPO_OVERRIDE
    else if (buf[0] && !gui_menu_shown()
        #ifdef CONFIG_MOVIE_EXPO_OVERRIDE_DISABLE_SYNC_WITH_PROPS
        && !is_movie_mode()
        #endif
    )
    {
        bv_set_rawaperture(COERCE(buf[0], lens_info.raw_aperture_min, lens_info.raw_aperture_max));
    }
    bv_auto_update();
    #endif
    lens_display_set_dirty();
}

PROP_HANDLER( PROP_APERTURE_AUTO )
{
    /* this gets updated in Tv mode (where PROP_APERTURE is not updated); same for P, Auto and so on */
    /* it becomes 0 when camera is no longer metering */

    if (shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_AV)
    {
        /* in these modes, aperture is not automatic */
        /* however, this property sometimes becomes 0 in these modes as well, but this is not desired */
        if (buf[0] == 0)
            return;
    }

    if (!CONTROL_BV)
    {
        /* expo override turned off? */
        lensinfo_set_aperture(buf[0]);
    }

    lens_display_set_dirty();
}

PROP_HANDLER( PROP_SHUTTER_AUTO )
{
    /* this gets updated in Av mode (where PROP_SHUTTER is not updated); same for P, Auto and so on */
    /* it becomes 0 when camera is no longer metering */
    
    if (shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_TV)
    {
        /* in these modes, shutter is not automatic */
        /* however, this property sometimes becomes 0 in these modes as well, but this is not desired */
        if (buf[0] == 0)
            return;
    }
    
    if (!CONTROL_BV)
    {
        /* expo override turned off? */
        /* todo: double-check if it's still needed */
        if (ABS(buf[0] - lens_info.raw_shutter) > 3) 
            lensinfo_set_shutter(buf[0]);
    }
    
    lens_display_set_dirty();
}

PROP_HANDLER( PROP_AE )
{
    const uint32_t value = *(uint32_t *) buf;
    lens_info.ae = (int8_t)value;
    update_stuff();
}

PROP_HANDLER( PROP_WB_MODE_LV )
{
    const uint32_t value = *(uint32_t *) buf;
    lens_info.wb_mode = value;
}

PROP_HANDLER(PROP_WBS_GM)
{
    const int8_t value = *(int8_t *) buf;
    lens_info.wbs_gm = value;
}

PROP_HANDLER(PROP_WBS_BA)
{
    const int8_t value = *(int8_t *) buf;
    lens_info.wbs_ba = value;
}

PROP_HANDLER( PROP_WB_KELVIN_LV )
{
    const uint32_t value = *(uint32_t *) buf;
    lens_info.kelvin = value;
}

#if !defined(CONFIG_5DC) && !defined(CONFIG_40D)
static uint16_t custom_wb_gains[128];
PROP_HANDLER(PROP_CUSTOM_WB)
{
    ASSERT(len <= sizeof(custom_wb_gains));
    memcpy(custom_wb_gains, buf, len);
    const uint16_t * gains = (uint16_t *) buf;
    lens_info.WBGain_R = gains[16];
    lens_info.WBGain_G = gains[18];
    lens_info.WBGain_B = gains[19];
}
#endif

void lens_set_custom_wb_gains(int gain_R, int gain_G, int gain_B)
{
#if !defined(CONFIG_VXWORKS)
    // normalize: green gain should be always 1
    //~ gain_G = COERCE(gain_G, 4, 32000);
    //~ gain_R = COERCE(gain_R * 1024 / gain_G, 128, 32000);
    //~ gain_B = COERCE(gain_B * 1024 / gain_G, 128, 32000);
    //~ gain_G = 1024;

    gain_G = COERCE(gain_G, 128, 8192);
    gain_R = COERCE(gain_R, 128, 8192);
    gain_B = COERCE(gain_B, 128, 8192);

    // round off a bit to get nice values in menu
    gain_R = ((gain_R + 8) >> 4) << 4;
    gain_B = ((gain_B + 8) >> 4) << 4;

    custom_wb_gains[16] = gain_R;
    custom_wb_gains[18] = gain_G;
    custom_wb_gains[19] = gain_B;
    prop_request_change(PROP_CUSTOM_WB, custom_wb_gains, 0);

    int mode = WB_CUSTOM;
    prop_request_change(PROP_WB_MODE_LV, &mode, 4);
    prop_request_change(PROP_WB_MODE_PH, &mode, 4);
#endif
}

#define LENS_GET(param) \
int lens_get_##param() \
{ \
    return lens_info.param; \
} 

PROP_INT(PROP_WB_KELVIN_PH, wb_kelvin_ph);

void
lens_set_kelvin(int k)
{
    k = COERCE(k, KELVIN_MIN, KELVIN_MAX);
    int mode = WB_KELVIN;

    if (k > 10000 || k < 2500) // workaround for 60D; out-of-range values are ignored in photo mode
    {
        int lim = k > 10000 ? 10000 : 2500;
        if ((k > 10000 && (int)wb_kelvin_ph < lim) || (k < 2500 && (int)wb_kelvin_ph > lim))
        {
            prop_request_change(PROP_WB_KELVIN_PH, &lim, 4);
            msleep(20);
        }
    }

    prop_request_change(PROP_WB_MODE_LV, &mode, 4);
    prop_request_change(PROP_WB_KELVIN_LV, &k, 4);
    prop_request_change(PROP_WB_MODE_PH, &mode, 4);
    prop_request_change(PROP_WB_KELVIN_PH, &k, 4);
    msleep(20);
}

void
lens_set_kelvin_value_only(int k)
{
    k = COERCE(k, KELVIN_MIN, KELVIN_MAX);

    if (k > 10000 || k < 2500) // workaround for 60D; out-of-range values are ignored in photo mode
    {
        int lim = k > 10000 ? 10000 : 2500;
        prop_request_change(PROP_WB_KELVIN_PH, &lim, 4);
        msleep(10);
    }

    prop_request_change(PROP_WB_KELVIN_LV, &k, 4);
    prop_request_change(PROP_WB_KELVIN_PH, &k, 4);
    msleep(10);
}

void split_iso(int raw_iso, unsigned int* analog_iso, int* digital_gain)
{
    if (!raw_iso) { *analog_iso = 0; *digital_gain = 0; return; }
    int rounded = ((raw_iso+3)/8) * 8;
    if (get_htp()) rounded -= 8;
    *analog_iso = COERCE(rounded, 72, MAX_ANALOG_ISO); // analog ISO range: 100-3200 (100-25600 on 5D3)
    *digital_gain = raw_iso - *analog_iso;
}

void iso_components_update()
{
    split_iso(lens_info.raw_iso, &lens_info.iso_analog_raw, &lens_info.iso_digital_ev);

    lens_info.iso_equiv_raw = lens_info.raw_iso;

    int digic_gain = get_digic_iso_gain_movie();
    if (lens_info.iso_equiv_raw && digic_gain != 1024 && is_movie_mode())
    {
        lens_info.iso_equiv_raw = lens_info.iso_equiv_raw + (gain_to_ev_scaled(digic_gain, 8) - 80);
    }
}

static void update_stuff()
{
    focus_calc_dof();
    //~ if (gui_menu_shown()) lens_display_set_dirty();
    
    #ifdef FEATURE_MOVIE_LOGGING
    if (movie_log) mvr_update_logfile( &lens_info, 0 ); // do not force it
    #endif
    
    iso_components_update();
}

static void focus_ring_powersave_fix()
{
    if (!RECORDING && is_manual_focus())
    {
        /* Canon bug? turning the focus ring doesn't reset powersave timer */
        powersave_prolong();
    }
}

#if defined(CONFIG_EOSM)
PROP_HANDLER( PROP_LV_FOCAL_DISTANCE )
{
#ifdef FEATURE_MAGIC_ZOOM
    if (get_zoom_overlay_trigger_by_focus_ring()) zoom_overlay_set_countdown(300);
#endif
    
    idle_wakeup_reset_counters(-11);
    lens_display_set_dirty();
    focus_ring_powersave_fix();
    
#ifdef FEATURE_LV_ZOOM_SETTINGS
    zoom_focus_ring_trigger();
#endif
}
#endif
PROP_HANDLER( PROP_LV_LENS )
{
    const struct prop_lv_lens * const lv_lens = (void*) buf;
    lens_info.focal_len    = bswap16( lv_lens->focal_len );
    lens_info.focus_dist    = bswap16( lv_lens->focus_dist );
    
    if (lens_info.focal_len > 1000) // bogus values
        lens_info.focal_len = 0;

    //~ uint32_t lrswap = SWAP_ENDIAN(lv_lens->lens_rotation);
    //~ uint32_t lsswap = SWAP_ENDIAN(lv_lens->lens_step);

    //~ lens_info.lens_rotation = *((float*)&lrswap);
    //~ lens_info.lens_step = *((float*)&lsswap);
#if !defined(CONFIG_EOSM)  
    static unsigned old_focus_dist = 0;
    static unsigned old_focal_len = 0;
    if (lv && (old_focus_dist && lens_info.focus_dist != old_focus_dist) && (old_focal_len && lens_info.focal_len == old_focal_len))
    {
        #ifdef FEATURE_MAGIC_ZOOM
        if (get_zoom_overlay_trigger_by_focus_ring()) zoom_overlay_set_countdown(300);
        #endif
        
        idle_wakeup_reset_counters(-11);
        lens_display_set_dirty();
        focus_ring_powersave_fix();
        
        #ifdef FEATURE_LV_ZOOM_SETTINGS
        zoom_focus_ring_trigger();
        #endif
    }
    old_focus_dist = lens_info.focus_dist;
    old_focal_len = lens_info.focal_len;
#endif
    update_stuff();
}

/**
 * This tells whether the camera is ready to take a picture (or not)
 * 5D2: the sequence is: 0 11 10 8 0
 *      that means: 0 = idle, 11 = very busy (exposing), 10 = exposed, but processing (can take the next picture), 8 = done processing, just saving to card
 *      also, when job state is 11, we can't change camera settings, but when it's 10, we can
 * 5D3: the sequence is: 0 0x16 0x14 0x10 0
 * other cameras may have different values
 * 
 * => hypothesis: the general sequence is:
 * 
 *   0 max something_smaller something_even_smaller and so on
 * 
 *   so, we only want to avoid the situation when job_state == max_job_state
 * 
 */

static int max_job_state = 0;

int job_state_ready_to_take_pic()
{
    if (max_job_state == 0) return 1;
    return (int)lens_info.job_state < max_job_state;
}

PROP_HANDLER( PROP_LAST_JOB_STATE )
{
    const uint32_t state = *(uint32_t*) buf;
    lens_info.job_state = state;
    
    if (max_job_state == 0 && state != 0)
        max_job_state = state;
    
    if (max_job_state && (int)state == max_job_state)
    {
        mirror_locked = 0;
        hdr_flag_picture_was_taken();
    }

    #ifdef CONFIG_JOB_STATE_DEBUG
    static char jmsg[100] = "";
    STR_APPEND(jmsg, "%d ", state);
    bmp_printf(FONT_MED,0,0, jmsg);
    #endif
}

PROP_HANDLER(PROP_STROBO_AECOMP)
{
    lens_info.flash_ae = (int8_t) buf[0];
}

int lens_set_flash_ae(int fae)
{
    fae = round_flash_expo_comp(fae);
    prop_request_change_wait(PROP_STROBO_AECOMP, &fae, 4, 100);
    return lens_info.flash_ae == fae;
}

PROP_HANDLER(PROP_HALF_SHUTTER)
{
    update_stuff();
    lens_display_set_dirty();
    //~ bv_auto_update();
}

static struct menu_entry lens_menus[] = {
    #ifdef FEATURE_MOVIE_LOGGING
    {
        .name = "Movie Logging",
        .priv = &movie_log,
        .max = 1,
        .help = "Save metadata for each movie, e.g. MVI_1234.LOG",
        .depends_on = DEP_MOVIE_MODE,
    },
    #endif
};

static MENU_UPDATE_FUNC(lens_name_display)
{
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }
    MENU_SET_VALUE("%s", lens_info.name );
}

static MENU_UPDATE_FUNC(lens_id_display)
{
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }

    /* exiftool displays this as decimal */
    MENU_SET_VALUE("0x%04X (%d)", lens_info.lens_id, lens_info.lens_id);
}

static MENU_UPDATE_FUNC(lens_serial_display)
{
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }

    if(lens_info.lens_serial)
    {
        MENU_SET_VALUE(
            "%02x%08X", /* to match exiftool display */ 
            (uint32_t)(lens_info.lens_serial >> 32),
            (uint32_t)lens_info.lens_serial
        );
    }
    else
    {
        MENU_SET_VALUE("(none)");
    }
}

static MENU_UPDATE_FUNC(lens_extender_display)
{
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }
    MENU_SET_VALUE("0x%02X", lens_info.lens_extender );
}

static MENU_UPDATE_FUNC(lens_version_display)
{
    uint8_t v2 = lens_info.lens_version >> 16;
    uint8_t v1 = lens_info.lens_version >> 8;
    uint8_t v0 = lens_info.lens_version;
    
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }
    
    if(lens_info.lens_version)
    {
        MENU_SET_VALUE("v%d.%d.%d", v2, v1, v0);
    }
    else
    {
        MENU_SET_VALUE("(none)");
    }
}

static MENU_UPDATE_FUNC(lens_capabilities_display)
{
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }
    MENU_SET_VALUE("0x%02X", lens_info.lens_capabilities);
}

static MENU_UPDATE_FUNC(lens_focal_display)
{
    char *unit = "mm";
    float factor = 1.0f;
    
    if(!lens_info.lens_exists)
    {
        MENU_SET_VALUE("(no lens)");
        return;
    }
    
    if(focus_units == 1)
    {
        unit = "in";
        factor = 1/2.54;
    }
    
    if(lens_info.lens_focal_min == lens_info.lens_focal_max)
    {
        MENU_SET_VALUE("%d %s", (int)(lens_info.lens_focal_min * factor), unit);
    }
    else
    {
        MENU_SET_VALUE("%d-%d %s", (int)(lens_info.lens_focal_min * factor), (int)(lens_info.lens_focal_max * factor), unit);
    }
}

static struct menu_entry tweak_menus[] = {
   {
        .name = "Lens Info Prefs",
        .select   = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            #ifndef CONFIG_FULLFRAME
            {
                .name = "Crop Factor Display",
                .priv = &crop_info,
                .max  = 1,
                .choices = CHOICES("OFF", "ON, 35mm eq."),
                .help = "Display the 35mm equiv. focal length including crop factor.",
                .depends_on = DEP_LIVEVIEW | DEP_CHIPPED_LENS,
            },
            #endif
            {
                .name = "Focus Distance Units",
                .priv = &focus_units,
                .choices = CHOICES("mm/cm", "ft/in"),
                .max = 1,
                .help  = "Can select between Metric and Imperial focus distance units",
            },
            MENU_EOL
        }
    }
};

/* better place for this menu? */
static struct menu_entry lens_info_menus[] = {
   {
        .name = "Lens info",
        .select   = menu_open_submenu,
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Name",
                .update = &lens_name_display,
                .help  = "Show current lens name (as reported by your lens or adapter).",
                .help2 = "Read-only.",
            },
            {
                .name = "Focal len",
                .update = &lens_focal_display,
                .help  = "Show current lens focal length.",
                .help2 = "Read-only. Zoom lenses are only updated in LiveView.",
            },
            {
                .name = "Lens ID",
                .update = &lens_id_display,
                .help  = "Show current lens ID. Should match exiftool TEST.CR2 -LensType -b.",
                .help2 = "Read-only. Lenses from different manufacturers may have the same ID.",
            },
            {
                .name = "Serial num",
                .update = &lens_serial_display,
                .help  = "Show current lens serial number. Not all cameras report this.",
                .help2 = "Read-only. Should match exiftool TEST.CR2 -LensSerialNumber .",
            },
            {
                .name = "Version",
                .update = &lens_version_display,
                .help  = "Show current lens version string.",
                .help2 = "Read-only.",
            },
            {
                .name = "Capability",
                .update = &lens_capabilities_display,
                .help  = "Show current lens capability bits.",
                .help2 = "Read-only.",
            },
            {
                .name = "Extender",
                .update = &lens_extender_display,
                .help  = "Show current lens extender information byte.",
                .help2 = "Read-only.",
            },
            MENU_EOL
        },
    }
};

// hack to show this at the end of prefs menu
void
crop_factor_menu_init()
{
    menu_add("Prefs", tweak_menus, COUNT(tweak_menus));
    menu_add("Debug", lens_info_menus, COUNT(lens_info_menus));

    /* hack: lens name is usually long */
    /* force all submenu values to the left to maintain a nice layout */
    /* todo: better backend support? */
    lens_info_menus[0].children[0].parent_menu->split_pos = -10;
}

static void
lens_init( void* unused )
{
    focus_done_sem = create_named_semaphore( "focus_sem", 1 );
#ifndef CONFIG_5DC
    menu_add("Movie Tweaks", lens_menus, COUNT(lens_menus));
#endif
}

INIT_FUNC( "lens", lens_init );


// picture style, contrast...
// -------------------------------------------

PROP_HANDLER(PROP_PICTURE_STYLE)
{
    const uint32_t raw = *(uint32_t *) buf;
    lens_info.raw_picstyle = raw;
    lens_info.picstyle = get_prop_picstyle_index(raw);
}

extern struct prop_picstyle_settings picstyle_settings[];

// get contrast/saturation/etc from the current picture style

#define LENS_GET_FROM_PICSTYLE(param) \
int \
lens_get_##param() \
{ \
    int i = lens_info.picstyle; \
    if (!i) return -10; \
    return picstyle_settings[i].param; \
} \

#define LENS_GET_FROM_OTHER_PICSTYLE(param) \
int \
lens_get_from_other_picstyle_##param(int picstyle_index) \
{ \
    return picstyle_settings[picstyle_index].param; \
} \

// set contrast/saturation/etc in the current picture style (change is permanent!)
#define LENS_SET_IN_PICSTYLE(param,lo,hi) \
void \
lens_set_##param(int value) \
{ \
    if (value < lo || value > hi) return; \
    int i = lens_info.picstyle; \
    if (!i) return; \
    picstyle_settings[i].param = value; \
    prop_request_change(PROP_PICSTYLE_SETTINGS(i), &picstyle_settings[i], 24); \
} \

LENS_GET_FROM_PICSTYLE(contrast)
LENS_GET_FROM_PICSTYLE(sharpness)
LENS_GET_FROM_PICSTYLE(saturation)
LENS_GET_FROM_PICSTYLE(color_tone)

LENS_GET_FROM_OTHER_PICSTYLE(contrast)
LENS_GET_FROM_OTHER_PICSTYLE(sharpness)
LENS_GET_FROM_OTHER_PICSTYLE(saturation)
LENS_GET_FROM_OTHER_PICSTYLE(color_tone)

LENS_SET_IN_PICSTYLE(contrast, -4, 4)
LENS_SET_IN_PICSTYLE(sharpness, -1, 7)
LENS_SET_IN_PICSTYLE(saturation, -4, 4)
LENS_SET_IN_PICSTYLE(color_tone, -4, 4)


void SW1(int v, int wait)
{
    //~ int unused;
    //~ ptpPropButtonSW1(v, 0, &unused);
    prop_request_change(PROP_REMOTE_SW1, &v, 0);
    if (wait) msleep(wait);
}

void SW2(int v, int wait)
{
    //~ int unused;
    //~ ptpPropButtonSW2(v, 0, &unused);
    prop_request_change(PROP_REMOTE_SW2, &v, 0);
    if (wait) msleep(wait);
}

/** exposure primitives (the "clean" way, via properties) */

static int prop_set_rawaperture(unsigned aperture)
{
    // Canon likes only numbers in 1/3 or 1/2-stop increments
    int r = aperture % 8;
    if (r != 0 && r != 4 && r != 3 && r != 5 
        && aperture != lens_info.raw_aperture_min && aperture != lens_info.raw_aperture_max)
    {
        return 0;
    }

    lens_wait_readytotakepic(64);
    aperture = COERCE(aperture, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
    prop_request_change_wait(PROP_APERTURE, &aperture, 4, 200);
    return lens_info.raw_aperture == aperture;
}

static int prop_set_rawaperture_approx(unsigned new_av)
{
    /* aperture is very tricky; even if we respect the rounding rules
     * the values might be refused for no apparent reason
     * so we'll use trial and error, until a value gets accepted
     */
    
    /* first try to set it exactly */
    if (prop_set_rawaperture(new_av))
        return 1;

    /* then try to set a value close to the requested one, until it works */
    for (int d = 1; d < 4; d++)
    {
        if (prop_set_rawaperture(new_av + d))
            return 1;
        
        if (prop_set_rawaperture(new_av - d))
            return 1;
    }
    
    return 0;
}

static int prop_set_rawshutter(unsigned shutter)
{
    // Canon likes numbers in 1/3 or 1/2-stop increments
    if (is_movie_mode())
    {
        int r = shutter % 8;
        if (r != 0 && r != 4 && r != 3 && r != 5)
            return 0;
    }
    
    if (shutter < 16) return 0;
    if (shutter > FASTEST_SHUTTER_SPEED_RAW) return 0;
    
    lens_wait_readytotakepic(64);

    int s0 = shutter;
    prop_request_change_wait( PROP_SHUTTER, &shutter, 4, 100);
    
    if (lens_info.raw_shutter != s0 && !(CONTROL_BV && lv))
    {
        /* no confirmation? try set shutter 2 stops away from final value, and back */
        int sx = shutter > 128 ? shutter - 16 : shutter + 16;
        prop_request_change_wait( PROP_SHUTTER, &sx, 4, 100);
        prop_request_change_wait( PROP_SHUTTER, &shutter, 4, 100);
    }
    
    return lens_info.raw_shutter == s0;
}

static int prop_set_rawshutter_approx(unsigned shutter)
{
    lens_wait_readytotakepic(64);
    shutter = COERCE(shutter, 16, FASTEST_SHUTTER_SPEED_RAW); // 30s ... 1/8000 or 1/4000
    
    /* Some cameras accept any shutter speeds in 1/8 EV increments (in the valid range),
     * while others only accepts the following values modulo 8: 0,3,4,5.
     * Therefore, it is possible to get a rounding error of max 1 unit (1/8 EV)
     * 
     * Let's first see what Canon firmware gives us.
     */
    prop_request_change_wait( PROP_SHUTTER, &shutter, 4, 100);
    int delta = (int)lens_info.raw_shutter - (int)shutter;
    
    if (ABS(delta) == 2)
    {
        /* if we get a rounding error of 2, try altering the shutter speed by one;
         * it will most likely get it right this time */
        shutter -= SGN(delta);
        prop_request_change_wait( PROP_SHUTTER, &shutter, 4, 100);
        delta = (int)lens_info.raw_shutter - (int)shutter;
    }
    
    return ABS(delta) <= 1;
}

static int prop_set_rawiso(unsigned iso)
{
    lens_wait_readytotakepic(64);
    if (iso) iso = COERCE(iso, MIN_ISO, MAX_ISO); // ISO 100-25600
    prop_request_change_wait( PROP_ISO, &iso, 4, 100);
    return lens_info.raw_iso == iso;
}

static int prop_set_rawiso_approx(unsigned iso)
{
    /* first try to set it exactly */
    if (prop_set_rawiso(iso))
        return 1;

    if (iso)
    {
        /* then try to set a value close to the requested one, until it works */
        for (int d = 1; d < 4; d++)
        {
            if (prop_set_rawiso(iso + d))
                return 1;
            
            if (prop_set_rawiso(iso - d))
                return 1;
        }
    }
    
    return 0;
}

/** Exposure primitives (the "dirty" way, via BV control, bypasses protections) */

#ifdef FEATURE_EXPO_OVERRIDE

extern int bv_iso;
extern int bv_tv;
extern int bv_av;

int expo_override_active()
{
    return CONTROL_BV && lv;
}

void bv_update_lensinfo()
{
    if (CONTROL_BV) // sync lens info and camera properties with overriden values
    {
        lensinfo_set_iso(bv_iso + (get_htp() ? 8 : 0));
        lensinfo_set_shutter(bv_tv);
        lensinfo_set_aperture(bv_av);
    }
}

void bv_apply_tv(int tv)
{
    if (is_movie_mode())
        CONTROL_BV_TV = COERCE(tv, 0x5C, 0xA0); // try to extend shutter range, 1/24 ... 1/8000
    else
        CONTROL_BV_TV = COERCE(tv, 0x60, 0x98); // 600D: [LV] ERROR >> Tv:0x10, TvMax:0x98, TvMin:0x60
}

void bv_apply_av(int av)
{
    if (lens_info.raw_aperture_min == 0 && lens_info.raw_aperture_max == 0)
    {
        /* if this is 0, exposure override has no effect; use f2.8 as a dummy value */
        CONTROL_BV_AV = 32;
        return;
    }
    CONTROL_BV_AV = COERCE(av, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
}

void bv_apply_iso(int iso)
{
    CONTROL_BV_ISO = COERCE(iso, 72, MAX_ISO_BV);
}

int bv_set_rawshutter(unsigned shutter)
{
    //~ bmp_printf(FONT_MED, 600, 300, "bvsr %d ", shutter);
    bv_tv = shutter;
    bv_apply_tv(bv_tv);
    bv_update_lensinfo();
    bv_expsim_shift();
    //~ NotifyBox(2000, "%d > %d?", raw2shutter_ms(shutter), 1000/video_mode_fps); msleep(400);
    if (is_movie_mode() && raw2shutter_ms(shutter+1) > 1000/video_mode_fps) return 0;
    return shutter != 0;
}

int bv_set_rawiso(unsigned iso) 
{ 
    if (iso == 0) return 0;
    if (iso >= MIN_ISO && iso <= MAX_ISO_BV)
    {
        if (get_htp()) iso -= 8; // quirk: with exposure override and HTP, image is brighter by 1 stop than with Canon settings
        bv_iso = iso;
        bv_apply_iso(iso);
        bv_update_lensinfo();
        bv_expsim_shift();
        return 1;
    }
    else
    {
        return 0;
    }
}
int bv_set_rawaperture(unsigned aperture) 
{ 
    if (aperture >= lens_info.raw_aperture_min && aperture <= lens_info.raw_aperture_max) 
    { 
        bv_av = aperture; 
        bv_apply_av(bv_av);
        bv_update_lensinfo();
        bv_expsim_shift();
        return 1; 
    }
    else
    {
        return 0;
    }
}

static void bv_expsim_shift_try_iso(int newiso)
{
    #ifndef FEATURE_LV_DISPLAY_GAIN
    #error This requires FEATURE_LV_DISPLAY_GAIN.
    #endif
    
    #define MAX_GAIN_EV 6
    int e = 0;
    if (newiso < 72)
        e = 72 - newiso;
    else if (newiso > MAX_ISO_BV + MAX_GAIN_EV*8)
        e = MAX_ISO_BV + MAX_GAIN_EV*8 - newiso;
    e = e * 10/8;
    
    static int prev_e = 0;
    if (e != prev_e)
    {
        /*
        if (ABS(e) > 2)
        {
            NotifyBox(2000, "Preview %sexposed by %d.%d EV", e > 0 ? "over" : "under", ABS(e)/10, ABS(e)%10);
        }
        else NotifyBoxHide();
        */
    }
    prev_e = e;

    int g = 1024;
    while (newiso > MAX_ISO_BV && g < (1024 << MAX_GAIN_EV))
    {
        g *= 2;
        newiso -= 8;
    }

    bv_apply_iso(newiso);
    set_photo_digital_iso_gain_for_bv(g);
}
static void bv_expsim_shift()
{
    set_photo_digital_iso_gain_for_bv(1024);
    if (!lv) return;
    if (!get_expsim()) return;
    if (!CONTROL_BV) return;
   
    if (!is_movie_mode())
    {
        int tv_fps_shift = fps_get_shutter_speed_shift(bv_tv);
        
        if (is_bulb_mode()) // try to perform expsim in bulb mode, based on bulb timer setting
        {
            int tv = get_bulb_shutter_raw_equiv() + tv_fps_shift;
            if (tv < SHUTTER_1_30)
            {
                int delta = SHUTTER_1_30 - tv;
                bv_apply_tv(SHUTTER_1_30);
                bv_expsim_shift_try_iso(bv_iso + delta);
                return;
            }
            else
            {
                bv_apply_tv(tv);
                bv_apply_iso(bv_iso);
                return;
            }
        }
        else
        {
            bv_apply_tv(bv_tv);

            if (bv_tv < SHUTTER_1_30) // shutter speeds slower than 1/30 -> can't be obtained, raise ISO or open up aperture instead
            {
                int delta = SHUTTER_1_30 - bv_tv - tv_fps_shift;
                bv_apply_tv(SHUTTER_1_30);
                bv_expsim_shift_try_iso(bv_iso + delta);
                return;
            }
            else if (tv_fps_shift) // FPS override enabled
            {
                bv_expsim_shift_try_iso(bv_iso - tv_fps_shift);
                return;
            }
        }
        // no shifting, make sure we use unaltered values
        bv_apply_tv(bv_tv);
        bv_apply_av(bv_av);
        bv_apply_iso(bv_iso);
    }
    
    return;
}

static int bv_auto_should_enable()
{
    if (!bv_auto) return 0;
    if (!lv) return 0;

    extern int zoom_auto_exposure;
    if (zoom_auto_exposure && lv_dispsize > 1)
        return 0; // otherwise it would interfere with auto exposure
    
    if (LVAE_DISP_GAIN) // compatibility problem, disable it
        return 0;

    if (bv_auto == 1) // always enable (except for situations where it's known to cause problems)
    {
        return 1; // tricky situations were handled before these if's
    }

    return 0;
}

void bv_auto_update()
{
    //~ take_semaphore(bv_sem, 0);
    
    if (!bv_auto) return;
    //~ take_semaphore(lens_sem, 0);
    if (bv_auto_should_enable()) bv_enable();
    else bv_disable();
    bv_expsim_shift();
    lens_display_set_dirty();
    //~ give_semaphore(lens_sem);
    //~ give_semaphore(bv_sem);
}
#endif

/** Camera control functions */
int lens_set_rawaperture( int aperture)
{
    int ok = prop_set_rawaperture(aperture); // first try to set via property
    #ifdef FEATURE_EXPO_OVERRIDE
    bv_auto_update(); // auto flip between "BV" or "normal"
    if (CONTROL_BV) return bv_set_rawaperture(aperture);
    #endif
    return ok;
}

int lens_set_rawiso( int iso )
{
    int ok = prop_set_rawiso(iso); // first try to set via property
    #ifdef FEATURE_EXPO_OVERRIDE
    bv_auto_update(); // auto flip between "BV" or "normal"
    if (CONTROL_BV) return bv_set_rawiso(iso);
    #endif
    return ok;
}

int lens_set_rawshutter( int shutter )
{
    int ok = prop_set_rawshutter(shutter); // first try to set via property
    #ifdef FEATURE_EXPO_OVERRIDE
    bv_auto_update(); // auto flip between "BV" or "normal"
    if (CONTROL_BV) return bv_set_rawshutter(shutter);
    #endif
    return ok;
}

int lens_set_ae( int ae )
{
    ae = round_expo_comp(ae);
    prop_request_change_wait( PROP_AE, &ae, 4, 100);
    return lens_info.ae == ae;
}

void lens_set_drivemode( int dm )
{
    if (dm < 0) return;
    if (dm > 0x20) return;
    lens_wait_readytotakepic(64);
    prop_request_change( PROP_DRIVE, &dm, 4 );
    msleep(10);
}

void lens_set_wbs_gm(int value)
{
    value = COERCE(value, -9, 9);
    prop_request_change(PROP_WBS_GM, &value, 4);
}

void lens_set_wbs_ba(int value)
{
    value = COERCE(value, -9, 9);
    prop_request_change(PROP_WBS_BA, &value, 4);
}

// Functions to change camera settings during bracketing
// They will check the operation and retry if necessary
// Used for HDR bracketing
static int hdr_set_something(int (*set_something)(int), int arg)
{
    // first try to set it a few times...
    for (int i = 0; i < 5; i++)
    {
        if (ml_shutdown_requested)
            return 0;

        if (set_something(arg))
            return 1;
    }

    // didn't work, let's wait for job state...
    lens_wait_readytotakepic(64);

    for (int i = 0; i < 5; i++)
    {
        if (ml_shutdown_requested)
            return 0;

        if (set_something(arg))
            return 1;
    }

    // now this is really extreme... okay, one final try
    while (lens_info.job_state) msleep(100);

    for (int i = 0; i < 5; i++)
    {
        if (ml_shutdown_requested)
            return 0;

        if (set_something(arg))
            return 1;
    }

    // I give up    
    return 0;
}

int hdr_set_rawiso(int iso)
{
    return hdr_set_something((int(*)(int))prop_set_rawiso_approx, iso);
}

/* returns 0 on failure, -1 if it reached exposure limits, 1 otherwise */
int hdr_set_rawshutter(int shutter)
{
    /* fixme: check why 13 and not 16 */
    int in_range = shutter < FASTEST_SHUTTER_SPEED_RAW && shutter > 13;
    int ok = hdr_set_something((int(*)(int))prop_set_rawshutter_approx, shutter);
    return ok ? (in_range ? 1 : -1) : 0;
}

int hdr_set_rawaperture(int aperture)
{
    int in_range = aperture < lens_info.raw_aperture_max && aperture > lens_info.raw_aperture_min;
    int ok = hdr_set_something((int(*)(int))prop_set_rawaperture_approx, aperture);
    return ok ? (in_range ? 1 : -1) : 0;
}

int hdr_set_ae(int ae)
{
    int in_range = ABS(ae) < MAX_AE_EV * 8;
    int ok = hdr_set_something((int(*)(int))lens_set_ae, ae);
    return ok ? (in_range ? 1 : -1) : 0;
}

int hdr_set_flash_ae(int fae)
{
    int in_range = fae < FLASH_MAX_EV * 8 && fae > FLASH_MIN_EV * 8;
    int ok = hdr_set_something((int(*)(int))lens_set_flash_ae, fae);
    return ok ? (in_range ? 1 : -1) : 0;
}

int get_max_analog_iso() { return MAX_ANALOG_ISO; }
int get_max_ae_ev() { return MAX_AE_EV; }
#ifdef AE_VALUE
int get_ae_value() { return AE_VALUE; }
int get_bv() {
    return APEX10_RAW2TV(lens_info.raw_shutter)
         + APEX10_RAW2AV(lens_info.raw_aperture)
         - APEX10_RAW2SV(lens_info.iso_equiv_raw)
         + APEX10_RAW2EC(get_ae_value());
}
#endif
#ifdef AE_STATE
int get_ae_state() { return AE_STATE; }
#endif

#include "lvinfo.h"

static LVINFO_UPDATE_FUNC(clock_update)
{
    LVINFO_BUFFER(8);
    struct tm now;
    LoadCalendarFromRTC( &now );
    snprintf(buffer, sizeof(buffer), "%02d:%02d", now.tm_hour, now.tm_min);
}

static LVINFO_UPDATE_FUNC(disp_preset_update)
{
    LVINFO_BUFFER(8);

    /* only display this if the feature is enabled */
    extern int disp_profiles_0;
    if (disp_profiles_0)
    {
        snprintf(buffer, sizeof(buffer), 
            "DISP %d", get_disp_mode()
        );
    }
}

static LVINFO_UPDATE_FUNC(picq_update)
{
    LVINFO_BUFFER(16);

    if (!is_movie_mode())
    {
        int raw = pic_quality & 0x60000;
        int jpg = pic_quality & 0x10000;
        int rawsize = pic_quality & 0xF;
        int jpegtype = pic_quality >> 24;
        int jpegsize = (pic_quality >> 8) & 0xFF;
        snprintf(buffer, sizeof(buffer), "%s%s%s",
            rawsize == 1 ? "mRAW" : rawsize == 2 ? "sRAW" : rawsize == 7 ? "sRAW1" : rawsize == 8 ? "sRAW2" : raw ? "RAW" : "",
            jpg == 0 ? "" : (raw ? "+" : "JPG-"),
            jpg == 0 ? "" : (
                jpegsize == 0 ? (jpegtype == 3 ? "L" : "l") : 
                jpegsize == 1 ? (jpegtype == 3 ? "M" : "m") : 
                jpegsize == 2 ? (jpegtype == 3 ? "S" : "s") :
                jpegsize == 0x0e ? (jpegtype == 3 ? "S1" : "s1") :
                jpegsize == 0x0f ? (jpegtype == 3 ? "S2" : "s2") :
                jpegsize == 0x10 ? (jpegtype == 3 ? "S3" : "s3") :
                "err"
            )
        );
    }
    
    int raw_lv = raw_lv_is_enabled();
    if (raw_lv)
    {
        /* make it obvious that LiveView is in RAW mode */
        /* (primarily for troubleshooting the raw backend, proper raw_lv_request/release calls and Magic Zoom slowdowns) */
        if (is_movie_mode())
        {
            /* todo: icon? */
            snprintf(buffer, sizeof(buffer), "RAW");
        }
        item->color_fg = raw_lv == 1 ? COLOR_GREEN1 : COLOR_GRAY(20);
    }
}

static LVINFO_UPDATE_FUNC(alo_htp_update)
{
    LVINFO_BUFFER(8);
    int alo = get_alo();
    snprintf(buffer, sizeof(buffer),
        get_htp() ? "HTP" :
        alo == ALO_LOW ? "alo" :
        alo == ALO_STD ? "Alo" :
        alo == ALO_HIGH ? "ALO" : ""
    );
}

#ifdef FEATURE_PICSTYLE
static LVINFO_UPDATE_FUNC(picstyle_update)
{
    LVINFO_BUFFER(12);

    if (is_movie_mode())
    {
        /* picture style has no effect on raw video => don't display */
        if (raw_lv_is_enabled())
            return;
    }
    else
    {
        /* when shooting RAW photos, picture style only affects the preview => don't display */
        int jpg = pic_quality & 0x10000;
        if (!jpg)
            return;
    }

    snprintf(buffer, sizeof(buffer), "%s",
        (char*)get_picstyle_name(lens_info.raw_picstyle)
    );
}
#endif


static LVINFO_UPDATE_FUNC(temp_update)
{
  #ifdef EFIC_CELSIUS
    LVINFO_BUFFER(8);
    
    int t = EFIC_CELSIUS;
    snprintf(buffer, sizeof(buffer), "%d"SYM_DEGREE"C", t);
    if (t >= 60)
    {
        item->color_bg = COLOR_RED;
    }
    else if (t >= 50)
    {
        item->color_bg = COLOR_ORANGE;
    }
  #endif
}

static LVINFO_UPDATE_FUNC(mvi_number_update)
{
    LVINFO_BUFFER(12);
    
    if (is_movie_mode() && !raw_lv_is_enabled())
    {
        snprintf(buffer, sizeof(buffer), "MVI_%04d", get_shooting_card()->file_number);
    }
}

static LVINFO_UPDATE_FUNC(fps_update)
{
    LVINFO_BUFFER(8);

    if (is_movie_mode())
    {
        int f = fps_get_current_x1000();
        snprintf(buffer, sizeof(buffer), 
            "%2d.%03d", 
            f / 1000, f % 1000
        );
    }
}

static LVINFO_UPDATE_FUNC(free_space_update)
{
    LVINFO_BUFFER(8);
    
    if (RECORDING)
    {
        /* leave space for the recording indicators */
        return;
    }

    int free_space_32k = get_free_space_32k(get_shooting_card());

    int fsg = free_space_32k >> 15;
    int fsgr = free_space_32k - (fsg << 15);
    int fsgf = (fsgr * 10) >> 15;

    snprintf(buffer, sizeof(buffer), 
        "%d.%dGB",
        fsg,
        fsgf
    );
}

static LVINFO_UPDATE_FUNC(mode_update)
{
    LVINFO_BUFFER(8);
    snprintf(buffer, sizeof(buffer), get_shootmode_name_short(shooting_mode_custom));
}

static LVINFO_UPDATE_FUNC(focal_len_update)
{
    LVINFO_BUFFER(16);
    if (lens_info.name[0])
    {
        snprintf(buffer, sizeof(buffer), "%d%s",
               crop_info ? (lens_info.focal_len * SENSORCROPFACTOR + 5) / 10 : lens_info.focal_len,
               crop_info ? "eq" : SYM_SMALL_M SYM_SMALL_M
        );
    }
}

static LVINFO_UPDATE_FUNC(is_update)
{
    LVINFO_BUFFER(4);

    if (lens_info.IS)
    {
        int is_color =
            lens_info.IS == 0 ? COLOR_WHITE    :  // IS off
            lens_info.IS == 4 ? COLOR_GRAY(50) :  // IS active, but not engaged
            lens_info.IS == 8 ? COLOR_BLACK    :  // IS disabled on sigma lenses?
            lens_info.IS == 0xC ? COLOR_CYAN   :  // IS starting?
            lens_info.IS == 0xE ? COLOR_WHITE  :  // IS active and kicking
            COLOR_RED;                            // unknown
        snprintf(buffer, sizeof(buffer), "IS");
        item->color_fg = is_color;
    }
}

static LVINFO_UPDATE_FUNC(av_update)
{
    LVINFO_BUFFER(8);

    if (lens_info.raw_aperture && lens_info.name[0])
    {
        snprintf(buffer, sizeof(buffer), lens_format_aperture(lens_info.raw_aperture));
    }
    
    if (CONTROL_BV)
    {
        /* mark the "exposure override" mode */
        item->color_bg = 18;
    }
}

static LVINFO_UPDATE_FUNC(tv_update)
{
    LVINFO_BUFFER(16);

    if (is_bulb_mode())
    {
        snprintf(buffer, sizeof(buffer), "BULB");
    }
    else if (is_movie_mode())
    {
        snprintf(buffer, sizeof(buffer), "%s", lens_format_shutter_reciprocal(get_current_shutter_reciprocal_x1000(), 2));
    }
    else if (lens_info.raw_shutter)
    {
        snprintf(buffer, sizeof(buffer), "%s", lens_format_shutter(lens_info.raw_shutter));
    }

    if (CONTROL_BV)
    {
        /* mark the "exposure override" mode */
        item->color_bg = 18;
    }
}

static int (*dual_iso_is_active)() = MODULE_FUNCTION(dual_iso_is_active);
static int (*dual_iso_get_recovery_iso)() = MODULE_FUNCTION(dual_iso_get_recovery_iso);

static LVINFO_UPDATE_FUNC(iso_update)
{
    LVINFO_BUFFER(16);

    if (hdr_video_enabled())
    {
        int iso_low, iso_high;
        hdr_get_iso_range(&iso_low, &iso_high);
        iso_low = raw2iso(get_effective_hdr_iso_for_display(iso_low));
        iso_high = raw2iso(get_effective_hdr_iso_for_display(iso_high));
        snprintf(buffer, sizeof(buffer), SYM_ISO"%d/%d", iso_low, iso_high);
    }
    else if (dual_iso_is_active())
    {
        snprintf(buffer, sizeof(buffer), SYM_ISO"%d/%d", 
            raw2iso(lens_info.iso_analog_raw),
            raw2iso(dual_iso_get_recovery_iso())
        );
    }
    else if (is_movie_mode())
    {
        snprintf(buffer, sizeof(buffer), SYM_ISO);
        
        if (!lens_info.raw_iso)
        {
            /* Auto ISO? */
            STR_APPEND(buffer, "A");
        }

        /* this includes ML ISO digital gains, if any */
        int iso_equiv_raw = lens_info.iso_equiv_raw;
        
        #ifdef FEATURE_FPS_OVERRIDE
        iso_equiv_raw += fps_get_iso_correction_evx8();
        #endif
        
        int digital_gain = iso_equiv_raw - lens_info.raw_iso;
        
        if (digital_gain > 1)
        {
            /* avoid ISO 125, 250... */
            item->color_fg = COLOR_ORANGE;
        }

        int lv_iso = (FRAME_ISO & 0xFF) + (get_htp() ? 8 : 0);

        if (ABS(lv_iso - lens_info.raw_iso) > 3)
        {
            /* for some reason, the ISO being used is different from the one reported in properties */
            iso_equiv_raw += lv_iso - lens_info.raw_iso;
        }

        if (raw_lv_is_enabled())
        {
            /* the only ISOs used are the full-stop ones;
             * digital gain is only applied to display, not recorded */
            iso_equiv_raw = (lv_iso+3)/8*8;
            item->color_fg = COLOR_WHITE;
        }
        
        int iso = raw2iso(iso_equiv_raw);
        
        if (iso > 1600)
        {
            /* think twice before increasing ISO above this value */
            item->color_fg = COLOR_ORANGE;
        }
        
        STR_APPEND(buffer, "%d", iso);
    }
    else /* photo mode */
    {
        if (lens_info.raw_iso)
        {
            snprintf(buffer, sizeof(buffer), SYM_ISO"%d", raw2iso(lens_info.raw_iso));
        }
        else if (lens_info.iso_auto)
        {
            snprintf(buffer, sizeof(buffer), SYM_ISO"A%d", raw2iso(lens_info.raw_iso_auto));
        }
        else
        {
            snprintf(buffer, sizeof(buffer), SYM_ISO"Auto");
        }
    }

    if (get_htp())
    {
        STR_APPEND(buffer, "D+");
    }

    #ifdef ISO_ADJUSTMENT_ACTIVE
    if (ISO_ADJUSTMENT_ACTIVE)
    {
        item->color_bg = COLOR_LIGHT_BLUE;
    }
    else
    #endif
    if (CONTROL_BV)
    {
        /* mark the "exposure override" mode */
        item->color_bg = 18;
    }
}

static LVINFO_UPDATE_FUNC(wb_update)
{
    LVINFO_BUFFER(16);
    
    if( lens_info.wb_mode == WB_KELVIN )
    {
        snprintf(buffer, sizeof(buffer), "%dK", lens_info.kelvin);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%s",
            (uniwb_is_active()      ? "UniWB"  :
            (lens_info.wb_mode == 0 ? "AutoWB" : 
            (lens_info.wb_mode == 1 ? "Sunny"  :
            (lens_info.wb_mode == 2 ? "Cloudy" : 
            (lens_info.wb_mode == 3 ? "Tungst." : 
            (lens_info.wb_mode == 4 ? "Fluor."  : 
            (lens_info.wb_mode == 5 ? "Flash"   : 
            (lens_info.wb_mode == 6 ? "Custom"  : 
            (lens_info.wb_mode == 8 ? "Shade"   :
             "unk")))))))))
        );
    }
    
    int gm = lens_info.wbs_gm;
    int ba = lens_info.wbs_ba;
    
    if (gm || ba)
    {
        /* a dot is smaller than a space */
        if (buffer[strlen(buffer)-1] != '.')
        {
            STR_APPEND(buffer, ".");
        }
    }
    
    if (gm)
    {
        STR_APPEND(buffer, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
    }

    if (ba)
    {
        STR_APPEND(buffer, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
    }
}

/* in focus.c */
extern LVINFO_UPDATE_FUNC(focus_dist_update);

static LVINFO_UPDATE_FUNC(af_mf_update)
{
    LVINFO_BUFFER(4);
    snprintf(buffer, sizeof(buffer), is_manual_focus() ? "MF" : "AF");
}

static LVINFO_UPDATE_FUNC(batt_update)
{
    item->height = 30;

    #ifdef CONFIG_BATTERY_INFO
    item->width = 70;
    #else
    item->width = 20;
    #endif
    item->custom_drawing = 1;
    
    if (can_draw)
    {
        int xr = item->x - item->width/2;
        int y_origin = item->y;
        xr += 4;

        #ifdef CONFIG_BATTERY_INFO
        int bat = GetBatteryLevel();
        #else
        int bat = battery_level_bars == 0 ? 5 : battery_level_bars == 1 ? 30 : 100;
        #endif

        int col = 
            battery_level_bars == 0 ? COLOR_RED :
            battery_level_bars == 1 ? COLOR_YELLOW : 
            COLOR_WHITE;

        #ifdef CONFIG_BATTERY_INFO
        bmp_printf(SHADOW_FONT(FONT(FONT_MED, col, item->color_bg)), xr+16, y_origin + 30 - font_med.height, "%d%%", bat);
        #endif
        
        bat = bat * 20 / 100;
        bmp_fill(col, xr+2, y_origin-3, 8, 3);
        bmp_draw_rect(col, xr-2, y_origin, 16, 27);
        bmp_draw_rect(col, xr-1, y_origin + 1, 14, 25);
        bmp_fill(col, xr+2, y_origin + 23 - bat, 8, bat);
    }
}

static LVINFO_UPDATE_FUNC(ae_update)
{
    LVINFO_BUFFER(8);

    switch(shooting_mode)
    {
        /* in semi-auto modes, this shows the exposure compensation dialed by user (all fine) */
        case SHOOTMODE_P:
        case SHOOTMODE_AV:
        case SHOOTMODE_TV:
        {
            int ae = lens_info.ae * 10/8;
            snprintf(buffer, sizeof(buffer), "%s%d.%d", FMT_FIXEDPOINT1S(ae));
            /* note: it may be unclear what this is at first sight; maybe some symbol in the font can help? */
            break;
        }
        
        /* in M mode, the behavior is not consistent across cameras (on some it's 0, on others it's Canon metering) */
        /* it may be a better idea to use AE_VALUE, but for me, Canon metering is completely irrelevant */
        /* so I recommend looking at the ETTR indicator and histogram instead */
        
        /* in other modes, no idea */
        
        /* note: merging this indicator with the ETTR one may be a good idea (maybe also with a nice graphical meter) */
        
        default:
            break;
    }
}

static struct lvinfo_item info_items[] = {
    /* Top bar */
    {
        .name = "Clock",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = clock_update,
        .preferred_position = -50,
        .priority = -1,
    },
    {
        .name = "Disp preset",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = disp_preset_update,
    },
    {
        .name = "Pic Quality",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = picq_update,
    },
    {
        .name = "ALO/HTP",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = alo_htp_update,
        .priority = -1,
    },
    #ifdef FEATURE_PICSTYLE
    {
        .name = "Pic.Style",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = picstyle_update,
        .priority = -1,
    },
    #endif
    {
        .name = "Temperature",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = temp_update,
        .priority = 1,
    },
    {
        .name = "MVI number",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = mvi_number_update,
    },
    {
        .name = "FPS",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = fps_update,
    },
    {
        .name = "Free space",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = free_space_update,
    },
    /* Bottom bar */
    {
        .name = "Mode",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = mode_update,
        .priority = 1,
        .preferred_position = -128,
    },
    {
        .name = "Focal len",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = focal_len_update,
    },
    {
        .name = "IS",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = is_update,
        .priority = -1,
    },
    {
        .name = "Aperture",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = av_update,
        .priority = 1,
    },
    {
        .name = "Shutter",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = tv_update,
        .priority = 1,
    },
    {
        .name = "ISO",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = iso_update,
        .priority = 1,
    },
    {
        .name = "White Balance",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = wb_update,
        .priority = 1,
    },
    {
        .name = "Focus dist",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = focus_dist_update,
    },
    {
        .name = "AF/MF",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = af_mf_update,
        .priority = -1,
    },
    {
        .name = "Exposure Compensation",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = ae_update,
        .preferred_position = 50,
    },
    {
        .name = "Battery",
        .which_bar = LV_BOTTOM_BAR_ONLY,
        .update = batt_update,
        .preferred_position = 127,
    },
};

static void lens_info_init()
{
    lvinfo_add_items(info_items, COUNT(info_items));
}

INIT_FUNC("lens_info", lens_info_init);
