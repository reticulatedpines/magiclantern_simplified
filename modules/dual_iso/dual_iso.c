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
#include <raw.h>
#include <patch.h>

static CONFIG_INT("isoless.hdr", isoless_hdr, 0);
static CONFIG_INT("isoless.iso", isoless_recovery_iso, 3);
static CONFIG_INT("isoless.alt", isoless_alternate, 0);
static CONFIG_INT("isoless.prefix", isoless_file_prefix, 0);

extern WEAK_FUNC(ret_0) int raw_lv_is_enabled();
extern WEAK_FUNC(ret_0) int get_dxo_dynamic_range();
extern WEAK_FUNC(ret_0) int is_play_or_qr_mode();
extern WEAK_FUNC(ret_0) int raw_hist_get_percentile_level();
extern WEAK_FUNC(ret_0) int raw_hist_get_overexposure_percentage();
extern WEAK_FUNC(ret_0) void raw_lv_request();
extern WEAK_FUNC(ret_0) void raw_lv_release();
extern WEAK_FUNC(ret_0) float raw_to_ev(int ev);

int dual_iso_set_enabled(bool enabled);
int dual_iso_is_enabled();
int dual_iso_is_active();

/* camera-specific constants */

static int is_7d = 0;
static int is_5d2 = 0;
static int is_50d = 0;
static int is_6d = 0;
static int is_60d = 0;
static int is_100d = 0; 
static int is_500d = 0;
static int is_550d = 0;
static int is_600d = 0;
static int is_650d = 0;
static int is_700d = 0;
static int is_eosm = 0;
static int is_1100d = 0;

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

/* CBR macros (to know where they were called from) */
#define CTX_SHOOT_TASK 0
#define CTX_SET_RECOVERY_ISO 1

static int isoless_recovery_iso_index()
{
    /* CHOICES("-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV", "+1 EV", "+2 EV", "+3 EV", "+4 EV", "+5 EV", "+6 EV", "100", "200", "400", "800", "1600", "3200", "6400", "12800") */

    int max_index = MAX(FRAME_CMOS_ISO_COUNT, PHOTO_CMOS_ISO_COUNT) - 1;
    
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

int dual_iso_calc_dr_improvement(int iso1, int iso2)
{
    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);

    int dr_hi = get_dxo_dynamic_range(iso_hi);
    int dr_lo = get_dxo_dynamic_range(iso_lo);
    int dr_gained = (iso_hi - iso_lo) / 8 * 100;
    int dr_lost = dr_lo - dr_hi;
    int dr_total = dr_gained - dr_lost;
    
    return dr_total;
}

int dual_iso_get_dr_improvement()
{
    if (!dual_iso_is_active())
        return 0;
    
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = lens_info.iso_analog_raw/8*8;
    return dual_iso_calc_dr_improvement(iso1, iso2);
}

/* 7D: transfer data to/from master memory */
extern WEAK_FUNC(ret_0) uint32_t BulkOutIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t*, uint32_t, uint32_t), uint32_t cb_parm);
extern WEAK_FUNC(ret_0) uint32_t BulkInIPCTransfer(int type, uint8_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t*, uint32_t, uint32_t), uint32_t cb_parm);

static uint8_t* local_buf = 0;

static void bulk_cb(uint32_t *parm, uint32_t address, uint32_t length)
{
    *parm = 0;
}

static int isoless_enable(uint32_t start_addr, int size, int count, uint32_t* backup)
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
        
        /* dummy call to get Canon values */
        patch_memory_array(start_addr, count, size, 0, 0, 0, 0, 0, backup, "dual_iso: CMOS[0] gains");
        unpatch_memory(start_addr);
    
        /* sanity check first */
        int prev_iso = 0;
        for (int i = 0; i < count; i++)
        {
            uint16_t raw = backup[i];
            uint32_t flag = raw & CMOS_FLAG_MASK;
            int iso1 = (raw >> CMOS_FLAG_BITS) & CMOS_ISO_MASK;
            int iso2 = (raw >> (CMOS_FLAG_BITS + CMOS_ISO_BITS)) & CMOS_ISO_MASK;
            int reg  = (raw >> 12) & 0xF;

            if (reg != 0 && !is_6d)
                return reg;
            
            if (flag != CMOS_EXPECTED_FLAG)
                return 2;
            
            if (is_5d2)
                iso2 += iso1; /* iso2 is 0 by default */
            
            if (iso1 != iso2)
                return 3;
            
            if ( (iso1 < prev_iso) && !is_50d && !is_500d) /* the list should be ascending */
                return 4;
            
            prev_iso = iso1;
        }

        int my_raw = backup[COERCE(isoless_recovery_iso_index(), 0, count-1)];

        /* take one of the ISO fields from recovery index */
        uint32_t patch_mask = ((1 << CMOS_ISO_BITS) - 1) << CMOS_FLAG_BITS;
        uint32_t patch_value = my_raw & patch_mask;

        if (is_5d2)
        {
            /* iso2 is 0 by default, let's just patch that one */
            patch_mask = ((1 << CMOS_ISO_BITS) - 1) << (CMOS_FLAG_BITS + CMOS_ISO_BITS);
            patch_value = (my_raw << CMOS_ISO_BITS) & patch_mask;
            
            /* enable the dual ISO flag */
            patch_mask  |= 1 << (CMOS_FLAG_BITS + CMOS_ISO_BITS + CMOS_ISO_BITS);
            patch_value |= 1 << (CMOS_FLAG_BITS + CMOS_ISO_BITS + CMOS_ISO_BITS);
        }

        if (is_eosm || is_650d || is_700d || is_100d) //TODO: This hack is probably needed on EOSM
        {
            /* Clear the MSB to fix line-skipping. 1 -> 8 lines, 0 -> 4 lines */
            patch_mask |= 0x800;
        }  
        
        /* apply our custom amplifier gains */
        patch_memory_array(start_addr, count, size, 0, 0, patch_mask, 0, patch_value, backup, "dual_iso: CMOS[0] gains");

        if (is_7d) /* commit the changes on master */
        {
            volatile uint32_t wait = 1;
            BulkOutIPCTransfer(0, (uint8_t*)start_addr - 2, size * count, start_addr_0, &bulk_cb, (uint32_t) &wait);
            while(wait) msleep(20);
        }

        /* success */
        return 0;
}

static int isoless_disable(uint32_t start_addr, int size, int count, uint32_t* backup)
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

    /* just undo our patch */
    unpatch_memory(start_addr);
    
    if (is_7d) /* commit the changes on master */
    {
        volatile uint32_t wait = 1;
        BulkOutIPCTransfer(0, (uint8_t*)start_addr - 2, size * count, start_addr_0, &bulk_cb, (uint32_t) &wait);
        while(wait) msleep(20);
    }

    /* success */
    return 0;
}

static struct semaphore * isoless_sem = 0;

/* Photo mode: always enable */
/* LiveView: only enable in movie mode */
/* Refresh the parameters whenever you change something from menu */
static int enabled_lv = 0;
static int enabled_ph = 0;

/* thread safe */
static unsigned int isoless_refresh(unsigned int ctx)
{
    if (!job_state_ready_to_take_pic())
        return 0;

    take_semaphore(isoless_sem, 0);

    static uint32_t backup_lv[20];
    static uint32_t backup_ph[20];
    int mv = is_movie_mode() ? 1 : 0;
    int lvi = lv ? 1 : 0;
    int raw_mv = mv && lv && raw_lv_is_enabled();
    int raw_ph = (pic_quality & 0xFE00FF) == (PICQ_RAW & 0xFE00FF);
    
    if (FRAME_CMOS_ISO_COUNT > COUNT(backup_ph)) goto end;
    if (PHOTO_CMOS_ISO_COUNT > COUNT(backup_lv)) goto end;
    
    static int prev_sig = 0;
    int sig = isoless_recovery_iso + (lvi << 16) + (raw_mv << 17) + (raw_ph << 18) + (isoless_hdr << 24) + (isoless_alternate << 25) + (isoless_file_prefix << 26) + get_shooting_card()->file_number * isoless_alternate + lens_info.raw_iso * 1234;
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

    if (isoless_hdr && raw_ph && !enabled_ph && PHOTO_CMOS_ISO_START && ((get_shooting_card()->file_number % 2) || !isoless_alternate))
    {
        enabled_ph = 1;
        int err = isoless_enable(PHOTO_CMOS_ISO_START, PHOTO_CMOS_ISO_SIZE, PHOTO_CMOS_ISO_COUNT, backup_ph);
        if (err) { NotifyBox(10000, "ISOless PH err(%d)", err); enabled_ph = 0; }
    }
    
    if (isoless_hdr && raw_mv && !enabled_lv && FRAME_CMOS_ISO_START)
    {
        enabled_lv = 1;
        int err = isoless_enable(FRAME_CMOS_ISO_START, FRAME_CMOS_ISO_SIZE, FRAME_CMOS_ISO_COUNT, backup_lv);
        if (err) { NotifyBox(10000, "ISOless LV err(%d)", err); enabled_lv = 0; }
    }

    if (setting_changed)
    {
        /* hack: this may be executed when file_number is updated;
         * if so, it will rename the previous picture, captured with the old setting,
         * so it will mis-label the pics */
        int file_prefix_needs_delay = (ctx == CTX_SHOOT_TASK && lens_info.job_state);

        int iso1 = 72 + isoless_recovery_iso_index() * 8;
        int iso2 = lens_info.iso_analog_raw/8*8;

        static int prefix_key = 0;
        if (isoless_file_prefix && enabled_ph && iso1 != iso2)
        {
            if (!prefix_key)
            {
                //~ NotifyBox(1000, "DUAL");
                if (file_prefix_needs_delay) msleep(500);
                prefix_key = file_prefix_set("DUAL");
            }
        }
        else if (prefix_key)
        {
            if (file_prefix_needs_delay) msleep(500);
            if (file_prefix_reset(prefix_key))
            {
                //~ NotifyBox(1000, "IMG_");
                prefix_key = 0;
            }
        }
    }

end:
    give_semaphore(isoless_sem);
    return 0;
}

int dual_iso_set_enabled(bool enabled)
{
    if (enabled)
        isoless_hdr = 1; 
    else
        isoless_hdr = 0;

    return 1; // module is loaded & responded != ret_0
}

int dual_iso_is_enabled()
{
    return isoless_hdr;
}

int dual_iso_is_active()
{
    return is_movie_mode() ? enabled_lv : enabled_ph;
}

int dual_iso_get_recovery_iso()
{
    if (!dual_iso_is_active())
        return 0;
    
    return 72 + isoless_recovery_iso_index() * 8;
}

int dual_iso_set_recovery_iso(int iso)
{
    if (!dual_iso_is_active())
        return 0;
    
    int max_index = MAX(FRAME_CMOS_ISO_COUNT, PHOTO_CMOS_ISO_COUNT) - 1;
    isoless_recovery_iso = COERCE((iso - 72)/8, 0, max_index);

    /* apply the new settings right now */
    isoless_refresh(CTX_SET_RECOVERY_ISO);
    return 1;
}

static unsigned int isoless_playback_fix(unsigned int ctx)
{
    if (is_7d || is_1100d)
        return 0; /* seems to cause problems, figure out why */
    
    if (!isoless_hdr) return 0;
    if (!is_play_or_qr_mode()) return 0;
    
    static int aux = INT_MIN;
    if (!should_run_polling_action(1000, &aux))
        return 0;

    uint32_t* lv = (uint32_t*)get_yuv422_vram()->vram;
    if (!lv) return 0;

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

static MENU_UPDATE_FUNC(isoless_check)
{
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = lens_info.iso_analog_raw/8*8;
    
    if (!iso2)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Auto ISO => cannot estimate dynamic range.");
    
    if (!iso2 && iso1 < 0)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ISO => cannot use relative recovery ISO.");
    
    if (iso1 == iso2)
        MENU_SET_WARNING(MENU_WARN_INFO, "Both ISOs are identical, nothing to do.");
    
    if (iso1 && iso2 && ABS(iso1 - iso2) > 8 * (is_movie_mode() ? MIN(FRAME_CMOS_ISO_COUNT-2, 3) : MIN(PHOTO_CMOS_ISO_COUNT-2, 4)))
        MENU_SET_WARNING(MENU_WARN_INFO, "Consider using a less aggressive setting (e.g. 100/800).");

    if (!get_dxo_dynamic_range(72))
        MENU_SET_WARNING(MENU_WARN_ADVICE, "No dynamic range info available.");

    int mvi = is_movie_mode();

    /* default checks will not work here - we need full-sized raw in photo mode */
    int raw = mvi ? raw_lv_is_enabled() : ((pic_quality & 0xFE00FF) == (PICQ_RAW & 0xFE00FF));

    if (mvi && !FRAME_CMOS_ISO_START)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Dual ISO does not work in movie mode on your camera.");
    
    if (!raw)
        menu_set_warning_raw(entry, info);
}

static MENU_UPDATE_FUNC(isoless_dr_update)
{
    isoless_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
    {
        MENU_SET_VALUE("N/A");
        return;
    }
    
    int dr_improvement = dual_iso_get_dr_improvement() / 10;
    
    MENU_SET_VALUE("%d.%d EV", dr_improvement/10, dr_improvement%10);
}

static MENU_UPDATE_FUNC(isoless_overlap_update)
{
    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = (lens_info.iso_analog_raw)/8*8;

    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);
    
    isoless_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
    {
        MENU_SET_VALUE("N/A");
        return;
    }
    
    int iso_diff = (iso_hi - iso_lo) * 10/ 8;
    int dr_lo = (get_dxo_dynamic_range(iso_lo)+5)/10;
    int overlap = dr_lo - iso_diff;
    
    MENU_SET_VALUE("%d.%d EV", overlap/10, overlap%10);
}

static MENU_UPDATE_FUNC(isoless_update)
{
    if (!isoless_hdr)
        return;

    int iso1 = 72 + isoless_recovery_iso_index() * 8;
    int iso2 = (lens_info.iso_analog_raw)/8*8;

    MENU_SET_VALUE("%d/%d", raw2iso(iso2), raw2iso(iso1));

    isoless_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
        return;
    
    int dr_improvement = dual_iso_get_dr_improvement() / 10;
    
    MENU_SET_RINFO("DR+%d.%d", dr_improvement/10, dr_improvement%10);
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
                .update = isoless_check,
                .min = -12,
                .max = 6,
                .unit = UNIT_ISO,
                .choices = CHOICES("-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV", "+1 EV", "+2 EV", "+3 EV", "+4 EV", "+5 EV", "+6 EV", "100", "200", "400", "800", "1600", "3200", "6400"),
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
                .choices = CHOICES("OFF", "DUAL (unreliable!)"),
                .help  = "Change file prefix for dual ISO photos (e.g. DUAL0001.CR2).",
                .help2 = "Will not sync properly in burst mode or when taking pics quickly."
            },
            MENU_EOL,
        },
    },
};

static unsigned int isoless_init()
{
    if (is_camera("5D3", "1.1.3") || is_camera("5D3", "1.2.3"))
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
    else if (is_camera("7D", "2.0.3"))
    {
        is_7d = 1;
        
        PHOTO_CMOS_ISO_START = 0x406944f4; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        PHOTO_CMOS_ISO_SIZE  =         14; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 0;
        
        local_buf = fio_malloc(PHOTO_CMOS_ISO_COUNT * PHOTO_CMOS_ISO_SIZE + 4);
    }
    else if (is_camera("5D2", "2.1.2"))
    {
        is_5d2 = 1;
        
        PHOTO_CMOS_ISO_START = 0x404b3b5c; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          5; // from ISO 100 to 1600
        PHOTO_CMOS_ISO_SIZE  =         14; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 3;
    }
    else if (is_camera("6D", "1.1.6"))
    {
        is_6d = 1;

        FRAME_CMOS_ISO_START = 0x40452196; // CMOS register 0003 - for LiveView, ISO 100 (check in movie mode, not photo!)
        FRAME_CMOS_ISO_COUNT =          7; // from ISO 100 to 6400
        FRAME_CMOS_ISO_SIZE  =         32; // distance between ISO 100 and ISO 200 addresses, in bytes

        PHOTO_CMOS_ISO_START = 0x40450E08; // CMOS register 0003 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          7; // from ISO 100 to 6400 (last real iso!)
        PHOTO_CMOS_ISO_SIZE  =         18; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 4;
        CMOS_FLAG_BITS = 0;
        CMOS_EXPECTED_FLAG = 0;
    }
    else if (is_camera("50D", "1.0.9"))
    {  
        // 100 - 0x04 - 160 - 0x94
        /* 00:00:04.078911     100   0004 404B548E */
        /* 00:00:14.214376     160   0094 404B549C */
        /* 00:00:26.551116     320   01B4 404B54AA */
        /*                     640   01FC 404B54B8 */
        /* 00:00:47.349194     1250+ 016C 404B54C6 */

        is_50d = 1;    

        PHOTO_CMOS_ISO_START = 0x404B548E; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          5; // from ISO 100 to 1600
        PHOTO_CMOS_ISO_SIZE  =         14; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 3;
        CMOS_EXPECTED_FLAG = 4;
    }
    else if (is_camera("60D", "1.1.1"))
    {  
        /*
        100 - 0
        200 - 0x024
        400 - 0x048
        800 - 0x06c
        1600 -0x090
        3200 -0x0b4
        */
        is_60d = 1;    

        FRAME_CMOS_ISO_START = 0x407458fc; // CMOS register 0000 - for LiveView, ISO 100 (check in movie mode, not photo!)
        FRAME_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        FRAME_CMOS_ISO_SIZE  =         30; // distance between ISO 100 and ISO 200 addresses, in bytes

        PHOTO_CMOS_ISO_START = 0x4074464c; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        PHOTO_CMOS_ISO_SIZE  =         18; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 0; 
    }
    else if (is_camera("500D", "1.1.1"))
    {  
        is_500d = 1;    

        PHOTO_CMOS_ISO_START = 0x405C56C2; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          5; // from ISO 100 to 1600
        PHOTO_CMOS_ISO_SIZE  =         14; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 3;
        CMOS_EXPECTED_FLAG = 0;
    }
    else if (is_camera("550D", "1.0.9"))
    {
    	is_550d = 1;
		
		FRAME_CMOS_ISO_START = 0x40695494; // CMOS register 0000 - for LiveView, ISO 100 (check in movie mode, not photo!)
		FRAME_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
		FRAME_CMOS_ISO_SIZE  =         30; // distance between ISO 100 and ISO 200 addresses, in bytes
		
		//  00 0000 406941E4  = 100
		//  00 0024 406941F6  = 200
		//  00 0048 40694208  = 400
		//  00 006C 4069421A  = 800
		//  00 0090 4069422C  = 1600
		//  00 00B4 4069423E  = 3200
		
		PHOTO_CMOS_ISO_START = 0x406941E4; // CMOS register 0000 - for photo mode, ISO 100
		PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
		PHOTO_CMOS_ISO_SIZE  =         18; // distance between ISO 100 and ISO 200 addresses, in bytes
		
		CMOS_ISO_BITS = 3;
		CMOS_FLAG_BITS = 2;
		CMOS_EXPECTED_FLAG = 0;
    }
    else if (is_camera("600D", "1.0.2"))
    {  
        /*
        100 - 0
        200 - 0x024
        400 - 0x048
        800 - 0x06c
        1600 -0x090
        3200 -0x0b4
        */
        is_600d = 1;    

        FRAME_CMOS_ISO_START = 0x406957C8; // CMOS register 0000 - for LiveView, ISO 100 (check in movie mode, not photo!)
        FRAME_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        FRAME_CMOS_ISO_SIZE  =         30; // distance between ISO 100 and ISO 200 addresses, in bytes

        PHOTO_CMOS_ISO_START = 0x4069464C; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        PHOTO_CMOS_ISO_SIZE  =         18; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 0;
    }
    else if (is_camera("100D", "1.0.1"))
    {
        is_100d = 1;    

        FRAME_CMOS_ISO_START = 0x416990c4;
        FRAME_CMOS_ISO_COUNT =          6;
        FRAME_CMOS_ISO_SIZE  =         34;

        PHOTO_CMOS_ISO_START = 0x4169743e;
        PHOTO_CMOS_ISO_COUNT =          6;
        PHOTO_CMOS_ISO_SIZE  =         20;

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 3;
    }
    else if (is_camera("700D", "1.1.5"))
    {
        is_700d = 1;    

        FRAME_CMOS_ISO_START = 0x4045328E;
        FRAME_CMOS_ISO_COUNT =          6;
        FRAME_CMOS_ISO_SIZE  =         34;

        PHOTO_CMOS_ISO_START = 0x40452044;
        PHOTO_CMOS_ISO_COUNT =          6;
        PHOTO_CMOS_ISO_SIZE  =         16;

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 3;
    }
    else if (is_camera("650D", "1.0.4"))
    {
        is_650d = 1;    

        FRAME_CMOS_ISO_START = 0x404a038e;
        FRAME_CMOS_ISO_COUNT =          6;
        FRAME_CMOS_ISO_SIZE  =       0x22;

        PHOTO_CMOS_ISO_START = 0x4049f144;
        PHOTO_CMOS_ISO_COUNT =          6;
        PHOTO_CMOS_ISO_SIZE  =       0x10;

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 3;
    }

    else if (is_camera("EOSM", "2.0.2"))
    {
        is_eosm = 1;    
        
        /*   00 0803 40502516 */
		/*   00 0827 40502538 */
		/*   00 084B 4050255A */
		/*   00 086F 4050257C */
		/*   00 0893 4050259E */
		/*   00 08B7 405025C0 */


        FRAME_CMOS_ISO_START = 0x40482516;
        FRAME_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        FRAME_CMOS_ISO_SIZE  =         34;


        /*
        00 0803 4050124C
        00 0827 4050125C
        00 084B 4050126C
        00 086F 4050127C
        00 0893 4050128C
        00 08B7 4050129C
        */

        PHOTO_CMOS_ISO_START = 0x4048124C;
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        PHOTO_CMOS_ISO_SIZE  =         16;

        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 2;
        CMOS_EXPECTED_FLAG = 3;
    }
    else if (is_camera("1100D", "1.0.5"))
    {
        is_1100d = 1;
        /*
         100 - 0     0x407444B2
         200 - 0x120 0x407444C6
         400 - 0x240 0x407444DA
         800 - 0x360 0x407444EE
         1600 -0x480 0x40744502
         3200 -0x5A0 0x40744516
         */
        
        PHOTO_CMOS_ISO_START = 0x407444B2; // CMOS register 00    00 - for photo mode, ISO
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to     3200
        PHOTO_CMOS_ISO_SIZE  =         20; // distance between     ISO 100 and ISO 200 addresses, in bytes
        
        CMOS_ISO_BITS = 3;
        CMOS_FLAG_BITS = 5;
        CMOS_EXPECTED_FLAG = 0;
    }




    if (FRAME_CMOS_ISO_START || PHOTO_CMOS_ISO_START)
    {
        menu_add("Expo", isoless_menu, COUNT(isoless_menu));
    }
    else
    {
        isoless_hdr = 0;
        return 1;
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

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, isoless_refresh, CTX_SHOOT_TASK)
    MODULE_CBR(CBR_SHOOT_TASK, isoless_playback_fix, CTX_SHOOT_TASK)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(isoless_hdr)
    MODULE_CONFIG(isoless_recovery_iso)
    MODULE_CONFIG(isoless_alternate)
    MODULE_CONFIG(isoless_file_prefix)
MODULE_CONFIGS_END()
