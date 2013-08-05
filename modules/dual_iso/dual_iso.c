/**
 * Dual ISO trick
 * Codenames: ISO-less mode, Nikon mode.
 * 
 * Technical details: https://dl.dropboxusercontent.com/u/4124919/bleeding-edge/isoless/dual_iso.pdf
 * 
 * 5D3 and 7D only.
 * Requires a camera with two analog amplifiers (cameras with 8-channel readout seem to have this).
 * 
 * Samples half of sensor lines at ISO 100 and the other half at ISO 1600 (or other user-defined values)
 * This trick cleans up shadow noise, resulting in a dynamic range improvement of around 3 stops on 5D3,
 * at the cost of reduced vertical resolution, aliasing and moire.
 * 
 * Requires a camera with two separate analog amplifiers that can be configured separately.
 * At the time of writing, the only two cameras known to have this are 5D Mark III and 7D.
 * 
 * Works for both raw photos (CR2 - postprocess with cr2hdr) and raw videos (raw2dng).
 * 
 * After postprocessing, you get a DNG that looks like an ISO 100 shot,
 * with much cleaner shadows (you can boost the exposure in post at +6EV without getting a lot of noise)
 * 
 * You will not get any radioactive HDR look by default;
 * you will get a normal picture with less shadow noise.
 * 
 * To get the HDR look, you need to use the "HDR from a single raw" trick:
 * develop the DNG file at e.g. 0, +3 and +6 EV and blend the resulting JPEGs with your favorite HDR software
 * (mine is Enfuse)
 * 
 * This technique is very similar to http://www.guillermoluijk.com/article/nonoise/index_en.htm
 * 
 * and Guillermo Luijk's conclusion applies here too:
 * """
 *     But differently as in typical HDR tools that apply local microcontrast and tone mapping procedures,
 *     the described technique seeks to rescue all possible information providing it in a resulting image 
 *     with the same overall and local brightness, contrast and tones as the original. It is now a decision
 *     of each user to choose the way to process and make use of it in the most convenient way.
 * """
 */

/*
 * Copyright (C) 2013 Magic Lantern Team
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

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <raw.h>
#include <lens.h>
#include <math.h>
#include <fileprefix.h>

static CONFIG_INT("isoless.hdr", isoless_hdr, 0);
static CONFIG_INT("isoless.iso", isoless_recovery_iso, 4);
static CONFIG_INT("isoless.alt", isoless_alternate, 0);
static CONFIG_INT("isoless.prefix", isoless_file_prefix, 0);

#define ISOLESS_AUTO (isoless_recovery_iso == 8)

extern WEAK_FUNC(ret_0) int raw_lv_is_enabled();
extern WEAK_FUNC(ret_0) int get_dxo_dynamic_range();
extern WEAK_FUNC(ret_0) int is_play_or_qr_mode();
extern WEAK_FUNC(ret_0) int raw_hist_get_percentile_level();
extern WEAK_FUNC(ret_0) int raw_hist_get_overexposure_percentage();
extern WEAK_FUNC(ret_0) void raw_lv_request();
extern WEAK_FUNC(ret_0) void raw_lv_release();
extern WEAK_FUNC(ret_0) float raw_to_ev(int ev);

/* camera-specific constants */

static int is_7d = 0;

static uint32_t FRAME_CMOS_ISO_START = 0;
static uint32_t FRAME_CMOS_ISO_COUNT = 0;
static uint32_t FRAME_CMOS_ISO_SIZE = 0;

static uint32_t PHOTO_CMOS_ISO_START = 0;
static uint32_t PHOTO_CMOS_ISO_COUNT = 0;
static uint32_t PHOTO_CMOS_ISO_SIZE = 0;

static uint32_t CMOS_ISO_BITS = 0;
static uint32_t CMOS_FLAG_BITS = 0;
static uint32_t CMOS_EXPECTED_FLAG = 0;

#define CMOS_ISO_MASK ((1 << CMOS_ISO_BITS) - 1)
#define CMOS_FLAG_MASK ((1 << CMOS_FLAG_BITS) - 1)


static int isoless_auto_iso_index;

static int isoless_recovery_iso_index()
{
    /* CHOICES("-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV", "+1 EV", "+2 EV", "+3 EV", "+4 EV", "+5 EV", "+6 EV", "100", "200", "400", "800", "1600", "3200", "6400", "12800") */

    int max_index = MAX(FRAME_CMOS_ISO_COUNT, PHOTO_CMOS_ISO_COUNT) - 1;
    
    /* auto mode */
    if (ISOLESS_AUTO)
        return COERCE(isoless_auto_iso_index, 0, max_index);
    
    /* absolute mode */
    if (isoless_recovery_iso >= 0)
        return COERCE(isoless_recovery_iso, 0, max_index);
    
    /* relative mode */

    /* auto ISO? idk, fall back to 100 */
    if (lens_info.raw_iso == 0)
        return 0;
    
    int delta = isoless_recovery_iso < -6 ? isoless_recovery_iso + 6 : isoless_recovery_iso + 7;
    int canon_iso_index = (lens_info.iso_analog_raw - 72) / 8;
    return COERCE(canon_iso_index + delta, 0, max_index);
}

/* 7D: transfer data to/from master memory */
extern WEAK_FUNC(ret_0) uint32_t BulkOutIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t*, uint32_t, uint32_t), uint32_t cb_parm);
extern WEAK_FUNC(ret_0) uint32_t BulkInIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t*, uint32_t, uint32_t), uint32_t cb_parm);

static uint8_t* local_buf = 0;

static void bulk_cb(uint32_t *parm, uint32_t address, uint32_t length)
{
    *parm = 0;
}

static int isoless_enable(uint32_t start_addr, int size, int count, uint16_t* backup)
{
        /* for 7D */
        int start_addr_0 = start_addr;
        
        if (is_7d) /* start_addr is on master */
        {
            volatile uint32_t wait = 1;
            BulkInIPCTransfer(0, local_buf, size * count, start_addr, &bulk_cb, (uint32_t) &wait);
            while(wait) msleep(20);
            start_addr = (uint32_t) local_buf + 2; /* our numbers are aligned at 16 bits, but not at 32 */
        }
    
        /* sanity check first */
        
        int prev_iso = 0;
        for (int i = 0; i < count; i++)
        {
            uint16_t raw = *(uint16_t*)(start_addr + i * size);
            uint32_t flag = raw & CMOS_FLAG_MASK;
            int iso1 = (raw >> CMOS_FLAG_BITS) & CMOS_ISO_MASK;
            int iso2 = (raw >> (CMOS_FLAG_BITS + CMOS_ISO_BITS)) & CMOS_ISO_MASK;
            int reg  = (raw >> 12) & 0xF;

            if (reg != 0)
                return 1;
            
            if (flag != CMOS_EXPECTED_FLAG)
                return 2;
            
            if (iso1 != iso2)
                return 3;
            
            if (iso1 < prev_iso) /* the list should be ascending */
                return 4;
            
            prev_iso = iso1;
        }
        
        if (prev_iso < 10 && !is_7d)
            return 5;
        
        /* backup old values */
        for (int i = 0; i < count; i++)
        {
            uint16_t raw = *(uint16_t*)(start_addr + i * size);
            backup[i] = raw;
        }
        
        /* apply our custom amplifier gains */
        for (int i = 0; i < count; i++)
        {
            uint16_t raw = *(uint16_t*)(start_addr + i * size);
            int my_raw = backup[COERCE(isoless_recovery_iso_index(), 0, count-1)];

            int my_iso2 = (my_raw >> (CMOS_FLAG_BITS + CMOS_ISO_BITS)) & CMOS_ISO_MASK;
            raw &= ~(CMOS_ISO_MASK << (CMOS_FLAG_BITS + CMOS_ISO_BITS));
            raw |= (my_iso2 << (CMOS_FLAG_BITS + CMOS_ISO_BITS));
            
            *(uint16_t*)(start_addr + i * size) = raw;
        }

        if (is_7d) /* commit the changes on master */
        {
            volatile uint32_t wait = 1;
            BulkOutIPCTransfer(0, (uint8_t*)start_addr - 2, size * count, start_addr_0, &bulk_cb, (uint32_t) &wait);
            while(wait) msleep(20);
        }

        /* success */
        return 0;
}

static int isoless_disable(uint32_t start_addr, int size, int count, uint16_t* backup)
{
    /* for 7D */
    int start_addr_0 = start_addr;
    
    if (is_7d) /* start_addr is on master */
    {
        volatile uint32_t wait = 1;
        BulkInIPCTransfer(0, local_buf, size * count, start_addr, &bulk_cb, (uint32_t) &wait);
        while(wait) msleep(20);
        start_addr = (uint32_t) local_buf + 2;
    }

    /* just restore saved values */
    for (int i = 0; i < count; i++)
    {
        *(uint16_t*)(start_addr + i * size) = backup[i];
    }

    if (is_7d) /* commit the changes on master */
    {
        volatile uint32_t wait = 1;
        BulkOutIPCTransfer(0, (uint8_t*)start_addr - 2, size * count, start_addr_0, &bulk_cb, (uint32_t) &wait);
        while(wait) msleep(20);
    }

    /* success */
    return 0;
}

/* Photo mode: always enable */
/* LiveView: only enable in movie mode */
/* Refresh the parameters whenever you change something from menu */
static unsigned int isoless_refresh(unsigned int ctx)
{
    if (!job_state_ready_to_take_pic())
        return 0;
    
    static uint16_t backup_lv[20];
    static uint16_t backup_ph[20];
    static int enabled_lv = 0;
    static int enabled_ph = 0;
    int mv = is_movie_mode() ? 1 : 0;
    int lvi = lv ? 1 : 0;
    int raw = (mv ? raw_lv_is_enabled() : ((pic_quality & 0xFE00FF) == (PICQ_RAW & 0xFE00FF))) ? 1 : 0;
    
    if (FRAME_CMOS_ISO_COUNT > COUNT(backup_ph)) return 0;
    if (PHOTO_CMOS_ISO_COUNT > COUNT(backup_lv)) return 0;
    
    static int prev_sig = 0;
    int sig = isoless_recovery_iso + (lvi << 16) + (mv << 17) + (raw << 18) + (isoless_hdr << 24) + (isoless_alternate << 25) + file_number + lens_info.raw_iso * 1234 + isoless_auto_iso_index * 315;
    int setting_changed = (sig != prev_sig);
    prev_sig = sig;
    
    if (enabled_lv && setting_changed)
    {
        isoless_disable(FRAME_CMOS_ISO_START, FRAME_CMOS_ISO_SIZE, FRAME_CMOS_ISO_COUNT, backup_lv);
        enabled_lv = 0;
    }
    
    if (enabled_ph && setting_changed)
    {
        isoless_disable(PHOTO_CMOS_ISO_START, PHOTO_CMOS_ISO_SIZE, PHOTO_CMOS_ISO_COUNT, backup_ph);
        enabled_ph = 0;
    }

    if (isoless_hdr && raw)
    {
        if (!enabled_ph && PHOTO_CMOS_ISO_START && ((file_number % 2) || !isoless_alternate))
        {
            enabled_ph = 1;
            int err = isoless_enable(PHOTO_CMOS_ISO_START, PHOTO_CMOS_ISO_SIZE, PHOTO_CMOS_ISO_COUNT, backup_ph);
            if (err) { NotifyBox(10000, "ISOless PH err(%d)", err); enabled_ph = 0; }
        }
        
        if (!enabled_lv && lv && mv && FRAME_CMOS_ISO_START)
        {
            enabled_lv = 1;
            int err = isoless_enable(FRAME_CMOS_ISO_START, FRAME_CMOS_ISO_SIZE, FRAME_CMOS_ISO_COUNT, backup_lv);
            if (err) { NotifyBox(10000, "ISOless LV err(%d)", err); enabled_lv = 0; }
        }
    }

    if (isoless_file_prefix && setting_changed)
    {
        /* hack: this may when file_number is updated;
         * if so, it will rename the previous picture, captured with the old setting,
         * so it will mis-label the pics */
        if (lens_info.job_state)
            msleep(500);
        
        static int prefix_key = 0;
        if (enabled_ph)
        {
            prefix_key = file_prefix_set("DUAL");
        }
        else
        {
            file_prefix_reset(prefix_key);
        }
    }

    return 0;
}

static unsigned int isoless_playback_fix(unsigned int ctx)
{
    if (is_7d)
        return 0; /* seems to cause problems, figure out why */
    
    if (!isoless_hdr) return 0;
    if (!is_play_or_qr_mode()) return 0;
    
    static int aux = INT_MIN;
    if (!should_run_polling_action(1000, &aux))
        return 0;

    uint32_t* lv = (uint32_t*)get_yuv422_vram()->vram;

    /* try to guess the period of alternating lines */
    int avg[5];
    int best_score = 0;
    int period = 0;
    int max_i = 0;
    int min_i = 0;
    int max_b = 0;
    int min_b = 0;
    for (int rep = 2; rep <= 5; rep++)
    {
        /* compute average brightness for each line group */
        for (int i = 0; i < rep; i++)
            avg[i] = 0;
        
        int num = 0;
        for(int y = os.y0; y < os.y_max; y ++ )
        {
            for (int x = os.x0; x < os.x_max; x += 32)
            {
                uint32_t uyvy = lv[BM2LV(x,y)/4];
                int luma = (((((uyvy) >> 24) & 0xFF) + (((uyvy) >> 8) & 0xFF)) >> 1);
                avg[y % rep] += luma;
                num++;
            }
        }
        
        /* choose the group with max contrast */
        int min = INT_MAX;
        int max = INT_MIN;
        int mini = 0;
        int maxi = 0;
        for (int i = 0; i < rep; i++)
        {
            avg[i] = avg[i] * rep / num;
            if (avg[i] < min)
            {
                min = avg[i];
                mini = i;
            }
            if (avg[i] > max)
            {
                max = avg[i];
                maxi = i;
            }
        }

        int score = max - min;
        if (score > best_score)
        {
            period = rep;
            best_score = score;
            min_i = mini;
            max_i = maxi;
            max_b = max;
            min_b = min;
        }
    }
    
    if (best_score < 5)
        return 0;

    /* alternate between bright and dark exposures */
    static int show_bright = 0;
    show_bright = !show_bright;
    
    /* one exposure too bright or too dark? no point in showing it */
    int forced = 0;
    if (min_b < 10)
        show_bright = 1, forced = 1;
    if (max_b > 245)
        show_bright = 0, forced = 1;

    bmp_printf(FONT_MED, 0, 0, "%s%s", show_bright ? "Bright" : "Dark", forced ? " only" : "");

    /* only keep one line from each group (not optimal for resolution, but doesn't have banding) */
    for(int y = os.y0; y < os.y_max; y ++ )
    {
        uint32_t* bright = &(lv[BM2LV_R(y)/4]);
        int dark_y = y/period*period + (show_bright ? max_i : min_i);
        if (dark_y < 0) continue;
        if (y == dark_y) continue;
        uint32_t* dark = &(lv[BM2LV_R(dark_y)/4]);
        memcpy(bright, dark, vram_lv.pitch);
    }
    return 0;
}

static unsigned int isoless_auto_step(unsigned int ctx)
{
    if (is_7d)
        return 0;
    
    if (isoless_hdr && ISOLESS_AUTO && lv && !recording && lv_dispsize == 1 && !is_movie_mode())
    {
        static int aux = INT_MIN;
        if (!should_run_polling_action(liveview_display_idle() ? 1000 : 200, &aux))
            return 0;
        
        raw_lv_request();
        
        /* target: 5% percentile above (DR-3) EV */
        int p = raw_hist_get_percentile_level(50, GRAY_PROJECTION_AVERAGE_RGB, 8);
        float ev = raw_to_ev(p);
        
        int dxo_dr = get_dxo_dynamic_range(lens_info.raw_iso);
        int target_ev = -((dxo_dr + 50) / 100 - 3);
        
        int under = target_ev - (int) roundf(ev);
        int canon_iso_index = MAX((lens_info.iso_analog_raw - 72) / 8, 0);

        if (canon_iso_index > 0 && under <= 2)
        {
            /* does it look grossly overexposed? screw shadows and protect the highlights instead */
            float overexposed = raw_hist_get_overexposure_percentage(GRAY_PROJECTION_AVERAGE_RGB) / 100.0;
            if (overexposed > 1)
            {
                isoless_auto_iso_index = 0;
                goto after_shadow;
            }
        }

        /* recover the shadows */
        int max_auto_iso = auto_iso_range & 0xFF;
        int max_gain = MAX((max_auto_iso - 72) / 8 - canon_iso_index, 0);
        isoless_auto_iso_index = canon_iso_index + COERCE(under, 0, max_gain);

    after_shadow:
        
        /* hacked status display */
        if (zebra_should_run())
        {
            int iso1 = 72 + isoless_recovery_iso_index() * 8;
            bmp_printf(FONT(FONT_MED, COLOR_WHITE, bmp_getpixel(359, os.y_max - 30)), 360, os.y_max - 33, "     ");
            bmp_printf(SHADOW_FONT(FONT_MED), 360, os.y_max - 33, "/%d ", raw2iso(iso1));
        }
        
        raw_lv_release();
    }
    return 0;
}

static MENU_UPDATE_FUNC(isoless_check)
{
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = lens_info.raw_iso/8*8;
    
    if (!iso2)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Auto ISO => cannot estimate dynamic range.");
    
    if (!iso2 && iso1 < 0)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ISO => cannot use relative recovery ISO.");
    
    if (ISOLESS_AUTO)
    {
        int dxo_dr = get_dxo_dynamic_range(lens_info.raw_iso);
        int target_ev = -((dxo_dr + 50) / 100 - 3);
        if (!lv || is_movie_mode()) MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto option only works in photo mode LiveView.");
        else MENU_SET_WARNING(MENU_WARN_INFO, "Auto shadow recovery: 5th percentile at %d EV.", target_ev);
    }

    if (iso1 == iso2)
        MENU_SET_WARNING(MENU_WARN_INFO, "Both ISOs are identical, nothing to do.");

    if (!get_dxo_dynamic_range(72))
        MENU_SET_WARNING(MENU_WARN_ADVICE, "No dynamic range info available.");

    int mvi = is_movie_mode() && FRAME_CMOS_ISO_START;

    int raw = mvi ? raw_lv_is_enabled() : ((pic_quality & 0xFE00FF) == (PICQ_RAW & 0xFE00FF));

    if (!raw)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "[%s] You must shoot RAW in order to use this.", mvi ? "MOVIE" : "PHOTO");
}

static MENU_UPDATE_FUNC(isoless_dr_update)
{
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = lens_info.raw_iso/8*8;
    
    isoless_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
    {
        MENU_SET_VALUE("N/A");
        return;
    }
    
    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);

    int dr_hi = get_dxo_dynamic_range(iso_hi);
    int dr_lo = get_dxo_dynamic_range(iso_lo);
    int dr_gained = (iso_hi - iso_lo) / 8 * 100;
    int dr_lost = dr_lo - dr_hi;
    int dr_total = dr_gained - dr_lost - 1;
    dr_total /= 10;
    
    MENU_SET_VALUE("%d.%d EV", dr_total/10, dr_total%10);
}

static MENU_UPDATE_FUNC(isoless_overlap_update)
{
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = (lens_info.raw_iso+3)/8*8;

    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);
    
    isoless_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
    {
        MENU_SET_VALUE("N/A");
        return;
    }
    
    int iso_diff = (iso_hi - iso_lo) / 8;
    int dr_lo = (get_dxo_dynamic_range(iso_lo)+50)/100;
    int overlap = dr_lo - iso_diff;
    
    MENU_SET_VALUE("%d EV", overlap);
}

static MENU_UPDATE_FUNC(isoless_update)
{
    if (!isoless_hdr)
        return;
    
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = lens_info.raw_iso/8*8;
    MENU_SET_VALUE("%d/%d", raw2iso(iso2), raw2iso(iso1));

    isoless_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
        return;
    
    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);

    int dr_hi = get_dxo_dynamic_range(iso_hi);
    int dr_lo = get_dxo_dynamic_range(iso_lo);
    int dr_gained = (iso_hi - iso_lo) / 8 * 100;
    int dr_lost = dr_lo - dr_hi;
    int dr_total = dr_gained - dr_lost;
    dr_total /= 10;
    
    MENU_SET_RINFO("DR+%d.%d", dr_total/10, dr_total%10);
}

static struct menu_entry isoless_menu[] =
{
    {
        .name = "Dual ISO",
        .priv = &isoless_hdr,
        .update = isoless_update,
        .max = 1,
        .help  = "Alternate ISO for every 2 sensor scan lines.",
        .help2 = "With some clever post, you get less shadow noise (more DR).",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Recovery ISO",
                .priv = &isoless_recovery_iso,
                .min = -12,
                .max = 8,
                .unit = UNIT_ISO,
                .choices = CHOICES("-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV", "+1 EV", "+2 EV", "+3 EV", "+4 EV", "+5 EV", "+6 EV", "100", "200", "400", "800", "1600", "3200", "6400", "12800", "Auto shadow"),
                .help  = "ISO for half of the scanlines (usually to recover shadows).",
                .help2 = "Can be absolute or relative to primary ISO from Canon menu.",
            },
            {
                .name = "Dynamic range gained",
                .update = isoless_dr_update,
                .icon_type = IT_ALWAYS_ON,
                .help  = "[READ-ONLY] How much more DR you get with current settings",
                .help2 = "(upper theoretical limit, estimated from DxO measurements)",
            },
            {
                .name = "Midtone overlapping",
                .update = isoless_overlap_update,
                .icon_type = IT_ALWAYS_ON,
                .help  = "[READ-ONLY] How much of midtones will get better resolution",
                .help2 = "Highlights/shadows will be half res, with aliasing/moire.",
            },
            {
                .name = "Alternate frames only",
                .priv = &isoless_alternate,
                .max = 1,
                .help = "Shoot one image with the hack, one without.",
            },
            {
                .name = "Custom file prefix",
                .priv = &isoless_file_prefix,
                .max = 1,
                .choices = CHOICES("OFF", "DUAL"),
                .help  = "Change file prefix for dual ISO photos (e.g. DUAL0001.CR2).",
            },
            MENU_EOL,
        },
    },
};

static unsigned int isoless_init()
{
    if (streq(camera_model_short, "5D3"))
    {
        FRAME_CMOS_ISO_START = 0x40452C72; // CMOS register 0000 - for LiveView, ISO 100 (check in movie mode, not photo!)
        FRAME_CMOS_ISO_COUNT =          9; // from ISO 100 to 25600
        FRAME_CMOS_ISO_SIZE  =         30; // distance between ISO 100 and ISO 200 addresses, in bytes

        PHOTO_CMOS_ISO_START = 0x40451120; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          8; // from ISO 100 to 12800
        PHOTO_CMOS_ISO_SIZE  =         18; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 4;
        CMOS_FLAG_BITS = 4;
        CMOS_EXPECTED_FLAG = 3;
    }
    else if (streq(camera_model_short, "7D"))
    {
        is_7d = 1;
        
        PHOTO_CMOS_ISO_START = 0x406944f4; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        PHOTO_CMOS_ISO_SIZE  =         14; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 0;
        
        local_buf = alloc_dma_memory(PHOTO_CMOS_ISO_COUNT * PHOTO_CMOS_ISO_SIZE + 4);
    }
    
    if (FRAME_CMOS_ISO_START || PHOTO_CMOS_ISO_START)
    {
        menu_add("Expo", isoless_menu, COUNT(isoless_menu));
    }
    else
    {
        isoless_hdr = 0;
    }
    return 0;
}

static unsigned int isoless_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(isoless_init)
    MODULE_DEINIT(isoless_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "ISO 100/1600")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "a1ex")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, isoless_refresh, 0)
    MODULE_CBR(CBR_SHOOT_TASK, isoless_playback_fix, 0)
    MODULE_CBR(CBR_SHOOT_TASK, isoless_auto_step, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(isoless_hdr)
    MODULE_CONFIG(isoless_recovery_iso)
    MODULE_CONFIG(isoless_alternate)
    MODULE_CONFIG(isoless_file_prefix)
MODULE_CONFIGS_END()
