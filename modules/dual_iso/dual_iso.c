/**
 * Dual ISO trick
 * Codenames: ISO-less mode, Nikon mode. 5D3 only for now, with potential for 7D.
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

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <raw.h>
#include <lens.h>
#include <math.h>

static CONFIG_INT("isoless.hdr", isoless_hdr, 0);
static CONFIG_INT("isoless.iso", isoless_recovery_iso, 4);

/* camera-specific constants */
uint32_t FRAME_CMOS_ISO_START = 0;
uint32_t FRAME_CMOS_ISO_COUNT = 0;
uint32_t FRAME_CMOS_ISO_SIZE = 0;

uint32_t PHOTO_CMOS_ISO_START = 0;
uint32_t PHOTO_CMOS_ISO_COUNT = 0;
uint32_t PHOTO_CMOS_ISO_SIZE = 0;

static int isoless_enable(uint32_t start_addr, int size, int count, uint16_t* backup)
{
        /* sanity check first */
        
        int prev_iso = 0;
        for (int i = 0; i < count; i++)
        {
            uint16_t raw = *(uint16_t*)(start_addr + i * size);
            int flag = raw & 0xF;
            int iso1 = (raw >> 4) & 0xF;
            int iso2 = (raw >> 8) & 0xF;
            int reg = (raw >> 12) & 0xF;
            
            if (reg != 0)
                return 1;
            
            if (flag != 3)
                return 2; /* important? no idea */
            
            if (iso1 != iso2)
                return 3;
            
            if (iso1 < prev_iso) /* the list should be ascending */
                return 4;
            
            prev_iso = iso1;
        }
        
        if (prev_iso < 10)
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
            int my_raw = backup[COERCE(isoless_recovery_iso, 0, count-1)];
            int my_iso2 = (my_raw >> 8) & 0xF;

            raw &= ~(0xF << 8);
            raw |= (my_iso2 << 8);
            
            *(uint16_t*)(start_addr + i * size) = raw;
        }

        /* success */
        return 0;
}

static int isoless_disable(uint32_t start_addr, int size, int count, uint16_t* backup)
{
    /* just restore saved values */
    for (int i = 0; i < count; i++)
    {
        *(uint16_t*)(start_addr + i * size) = backup[i];
    }

    /* success */
    return 0;
}

/* Photo mode: always enable */
/* LiveView: only enable in movie mode */
/* Refresh the parameters whenever you change something from menu */
static unsigned int isoless_refresh(unsigned int ctx)
{
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
    int sig = isoless_recovery_iso + (lvi << 16) + (mv << 17) + (raw << 18) + (isoless_hdr << 24);
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
        if (!enabled_ph)
        {
            enabled_ph = 1;
            int err = isoless_enable(PHOTO_CMOS_ISO_START, PHOTO_CMOS_ISO_SIZE, PHOTO_CMOS_ISO_COUNT, backup_ph);
            if (err) { NotifyBox(10000, "ISOless PH err(%d)", err); enabled_ph = 0; }
        }
        
        if (!enabled_lv && lv && mv)
        {
            enabled_lv = 1;
            int err = isoless_enable(FRAME_CMOS_ISO_START, FRAME_CMOS_ISO_SIZE, FRAME_CMOS_ISO_COUNT, backup_lv);
            if (err) { NotifyBox(10000, "ISOless LV err(%d)", err); enabled_lv = 0; }
        }
    }
    
    return 0;
}

static unsigned int isoless_playback_fix(unsigned int ctx)
{
    if (!isoless_hdr) return 0;
    if (!is_play_or_qr_mode()) return 0;

    uint32_t* lv = (uint32_t*)get_yuv422_vram()->vram;

    int avg[4] = {0,0,0,0};
    int num = 0;
    for(int y = os.y0; y < os.y_max; y ++ )
    {
        for (int x = os.x0; x < os.x_max; x += 2)
        {
            uint32_t uyvy = lv[BM2LV(x,y)/4];
            int luma = (((((uyvy) >> 24) & 0xFF) + (((uyvy) >> 8) & 0xFF)) >> 1);
            avg[y%8] += luma;
            num++;
        }
    }
    for (int i = 0; i < COUNT(avg); i++)
        avg[i] /= num;

    int idx[4] = {0,1,2,3};
    for (int i = 0; i < 3; i++)
        for (int j = i+1; j < 4; j++)
            if (avg[idx[i]] > avg[idx[j]])
                { int aux = idx[i]; idx[i] = idx[j]; idx[j] = aux; }
    int lo1 = avg[idx[0]];
    int lo2 = avg[idx[1]];
    int hi1 = avg[idx[2]];
    int hi2 = avg[idx[3]];

    if (ABS(lo1 - lo2) > 3) return 0;
    if (ABS(hi1 - hi2) > 3) return 0;
    if (ABS(hi1 - lo2) < 5) return 0;
    
    int is_bright[4] = {avg[0] >= hi1, avg[1] >= hi1, avg[2] >= hi1, avg[3] >= hi1};
    
    bmp_printf(FONT_MED, 0, 0, "%d%d%d%d", is_bright[0], is_bright[1], is_bright[2], is_bright[3]);
    
    /* replace bright lines with dark ones */
    for(int y = os.y0; y < os.y_max; y ++ )
    {
        if (is_bright[y%4])
        {
            uint32_t* bright = &(lv[BM2LV_R(y)/4]);
            int dark_y = !is_bright[(y+1)%4] ? y+1 : !is_bright[(y-1)%4] ? y-1 : -1;
            if (dark_y < 0) continue;
            uint32_t* dark = &(lv[BM2LV_R(dark_y)/4]);
            memcpy(bright, dark, vram_lv.pitch);
        }
    }
    return 0;
}

static MENU_UPDATE_FUNC(isoless_check)
{
    int iso1 = 72 + isoless_recovery_iso * 8;
    int iso2 = lens_info.raw_iso/8*8;
    
    if (!iso2)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Auto ISO => cannot estimate dynamic range.");

    if (iso1 == iso2)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Both ISOs are identical, nothing to do.");

    int raw = is_movie_mode() ? raw_lv_is_enabled() : ((pic_quality & 0xFE00FF) == (PICQ_RAW & 0xFE00FF));

    if (!raw)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "[%s] You must shoot RAW in order to use this.", is_movie_mode() ? "MOVIE" : "PHOTO");
}

static MENU_UPDATE_FUNC(isoless_dr_update)
{
    int iso1 = 72 + isoless_recovery_iso * 8;
    int iso2 = lens_info.raw_iso/8*8;
    
    isoless_check(entry, info);
    if (info->warning_level)
        return;
    
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
    int iso1 = 72 + isoless_recovery_iso * 8;
    int iso2 = (lens_info.raw_iso+3)/8*8;

    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);
    
    isoless_check(entry, info);
    if (info->warning_level)
        return;
    
    int iso_diff = (iso_hi - iso_lo) / 8;
    int dr_lo = (get_dxo_dynamic_range(iso_lo)+50)/100;
    int overlap = dr_lo - iso_diff;
    
    MENU_SET_VALUE("%d EV", overlap);
}

static MENU_UPDATE_FUNC(isoless_update)
{
    if (!isoless_hdr)
        return;
    
    int iso1 = 72 + isoless_recovery_iso * 8;
    int iso2 = lens_info.raw_iso/8*8;
    
    isoless_check(entry, info);
    if (info->warning_level)
        return;
    
    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);

    int dr_hi = get_dxo_dynamic_range(iso_hi);
    int dr_lo = get_dxo_dynamic_range(iso_lo);
    int dr_gained = (iso_hi - iso_lo) / 8 * 100;
    int dr_lost = dr_lo - dr_hi;
    int dr_total = dr_gained - dr_lost;
    dr_total /= 10;
    
    MENU_SET_VALUE("%d/%d", raw2iso(iso2), raw2iso(iso1));
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
                .max = 6,
                .unit = UNIT_ISO,
                .choices = CHOICES("100", "200", "400", "800", "1600", "3200", "6400"),
                .help  = "ISO for half of the scanlines (usually to recover shadows).",
                .help2 = "Select the primary ISO from Canon menu.",
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
    }
    
    if (FRAME_CMOS_ISO_START && PHOTO_CMOS_ISO_START)
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
    MODULE_CBR(CBR_SECONDS_CLOCK, isoless_refresh, 0)
    MODULE_CBR(CBR_SECONDS_CLOCK, isoless_playback_fix, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(isoless_hdr)
    MODULE_CONFIG(isoless_recovery_iso)
MODULE_CONFIGS_END()
