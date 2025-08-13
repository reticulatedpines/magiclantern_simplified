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
#include "../raw_video/mlv_rec/mlv.h"
#include "../raw_video/mlv_rec/mlv_rec_interface.h"

static CONFIG_INT("dual_iso.hdr", dual_iso_hdr, 0);
static CONFIG_INT("dual_iso.iso", dual_iso_alternate_iso, 3);
static CONFIG_INT("dual_iso.alt", dual_iso_every_other, 0);
static CONFIG_INT("dual_iso.prefix", dual_iso_file_prefix, 0);

extern WEAK_FUNC(ret_0) int raw_lv_is_enabled();
extern WEAK_FUNC(ret_0) int get_dxo_dynamic_range();
extern WEAK_FUNC(ret_0) int is_play_or_qr_mode();
extern WEAK_FUNC(ret_0) int raw_hist_get_percentile_level();
extern WEAK_FUNC(ret_0) int raw_hist_get_overexposure_percentage();
extern WEAK_FUNC(ret_0) void raw_lv_request();
extern WEAK_FUNC(ret_0) void raw_lv_release();
extern WEAK_FUNC(ret_0) float raw_to_ev(int ev);
extern WEAK_FUNC(ret_0) void mlv_set_type(mlv_hdr_t *, char *);

int dual_iso_set_enabled(bool enabled);
int dual_iso_is_enabled();
int dual_iso_is_active();

/* camera-specific constants */

static int is_7d = 0;
static int is_5d2 = 0;
static int is_50d = 0;
static int is_6d = 0;
static int is_6d2 = 0;
static int is_60d = 0;
static int is_70d = 0;
static int is_100d = 0;
static int is_200d = 0;
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

// ML code often uses the term "register", in an overloaded, confusing manner.
// This is doubly true for Dual ISO code.
//
// "Register" may be used to mean an ARM register.
// It can mean any MMIO mapped address, regardless of whether this actually is
// a register on the mapped hardware.  And, for dual ISO specifically, there
// is an ADTG part that has control registers.  See:
// https://magiclantern.fandom.com/wiki/ADTG
//
// NB ADTG is the part, and it has subparts.  I'm not clear on this yet,
// there are mentions of a CMOS and ADC sub part.
//
// Here we care primarily about ADTG control.  We alter cam mem so when DryOS
// configures ADTG, it will send our control messages, and select
// our desired hybrid ISO.  The control message is packed in a 32-bit value.
//
// There are different encoding schemes in different camera gens.
// 200D example:
// |_____|__________|______|______|______|______|_______|
//   unk   ADTG reg   ISO3   ISO2   ISO1   ISO0   flags
//
// An example value: 0x0b400220.  This splits into regions like so:
// 0_b_4_0022_0, or:
// unk = 0, ADTG reg = b, unk = 4, ISO3-0 = 0, 0, 2, 2, flags = 0.
// On 200D, 0 => ISO 100, 2 => ISO 400.
//
// Existing ML docs are quite vague and confusing on usage of CMOS vs ADTG.
// I think the CMOS sub-part is expected to have a small number of
// internal registers, 7 or 8 on D45 cams.  This changes to 15 or 16
// on later cams.
//
// Logging CMOS_write() shows the kinds of messages described here.
//
// ADTG reg is b on 200d, meaning that ADTG takes the whole message,
// uses b for internal addressing or config, and applies the command
// portion to b.  Probably to CMOS subpart?
//
// ML code treats the 4 ISO values as 2 combined values.  I presume
// this is because of bayer pattern meaning it's easier to think about
// two rows of sensor as one row of pixels.
//
// ML code makes some assumptions about the layout of bits in the
// control message.  It assumes flags are in the bottom bits,
// followed by two blocks, of equal size, holding ISO values,
// then an ADTG register value.
static uint32_t CMOS_ISO_BITS = 0; // length, in bits, of each of 2 ISO fields
static uint32_t CMOS_FLAG_BITS = 0; // length, in bits, of the flag field
static uint32_t CMOS_EXPECTED_FLAG = 0; // bit field, used for sanity check against flag field

#define CMOS_ISO_MASK ((1 << CMOS_ISO_BITS) - 1)
#define CMOS_FLAG_MASK ((1 << CMOS_FLAG_BITS) - 1)

/* CBR macros (to know where they were called from) */
#define CTX_SHOOT_TASK 0
#define CTX_SET_RECOVERY_ISO 1

// Returns a small integer between 0 and max allowed by
// ADTG control value array length, which represents
// full stops of ISO.  0: 100, 1: 200, 2: 400, etc.
static int get_alternate_iso_index()
{
    // CHOICES("-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV",
    //         "+1 EV", "+2 EV", "+3 EV", "+4 EV", "+5 EV", "+6 EV",
    //         "100", "200", "400", "800", "1600", "3200", "6400")
    //
    // The menu struct maps these to between -12 and 6, inclusive,
    // and dual_iso_alternate_iso should be set to one of those values.
    // Therefore, positive values are absolute ISO, else relative to
    // the ISO Canon thinks it's using.

    int max_index = MAX(FRAME_CMOS_ISO_COUNT, PHOTO_CMOS_ISO_COUNT) - 1;
    
    /* absolute mode */
    if (dual_iso_alternate_iso >= 0)
        return COERCE(dual_iso_alternate_iso, 0, max_index);
    
    /* relative mode */

    /* auto ISO? idk, fall back to 100 */
    if (lens_info.raw_iso == 0)
        return 0;
    
    // map -12:-7 -> -6:-1, -6:-1 -> 1:6,
    // that is, to number of stops different from base ISO
    int delta = dual_iso_alternate_iso < -6 ? dual_iso_alternate_iso + 6 : dual_iso_alternate_iso + 7;

    // SJE FIXME stop using these magic numbers that aren't explained at all.
    // Many refs to 72 and 8 in this code.  What's going on here is that Canon uses
    // a value in the "lens" struct that is mapped to an ISO value.  72 means ISO 100.
    // Full ISO stops are separated by 8.  E.g. ISO 200 is mapped to 80.
    // Why is this held in something we call lens_info?  Unknown.
    // Anyway, see lens.h for "codes_iso" and "values_iso", which store the mapping.
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
    
    int iso1 = 72 + get_alternate_iso_index() * 8;
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

// Return value is non-zero for error, but errors can be positive or negative.
static int patch_cmos_iso_values_200d(uint32_t start_addr, int item_size, int count)
{
    uint32_t table_size = item_size * count;
    int32_t result = 0;

    // save an alloc by getting space for both at once
    uint8_t *new_values = malloc(table_size * 2);
    if (new_values == NULL)
        return -2;
    uint8_t *old_values = new_values + table_size;

    memcpy(new_values, (uint8_t *)start_addr, table_size);
    memcpy(old_values, (uint8_t *)start_addr, table_size);

    // ISO bits are 0x000f_fff0, we will be preserving half and replacing half.
    uint32_t iso_mask = 0x00000ff0;

    uint32_t other_iso_i = get_alternate_iso_index();
    // could use CMOS_ISO_BITS for the mask, with some shifts, like dual_iso_enable() does:
    uint32_t other_iso_bits = *(uint32_t *)(old_values + item_size * other_iso_i) & iso_mask;
    if (other_iso_bits != 0x000
        && other_iso_bits != 0x110
        && other_iso_bits != 0x220
        && other_iso_bits != 0x330
        && other_iso_bits != 0x440
        && other_iso_bits != 0x550
        && other_iso_bits != 0x660)
    {
        // ISO values are unexpected, we don't want to patch using them
        DryosDebugMsg(0, 15, "weird ISO bits: 0x%x", other_iso_bits);
        DryosDebugMsg(0, 15, "from: 0x%x", (start_addr + item_size * other_iso_i));
        result = -1;
        goto end;
    }

    for (int i = 0; i < count; i++)
    {
        // field is 0xRRR ABCD 0, middle 4 are ISO values, RRR is CMOS register
        // It seems that "unusual" patterns aren't accepted, somehow.
        // 4400 or 4040 both work.  4440 results in all lines appearing the same brightness.
        // 6420 seems to make two bright, two dark.
        // So, this is not fully understood.
        // Here, we choose to use 0x00000ff0 above as the mask for our alternate ISO bits.
        //
        // Old cams calculate this in a slightly more generic way, using
        // CMOS_FLAG_BITS, CMOS_ISO_BITS, and some shifting.  This is less
        // clear, and so far this function is 200D specific, so I haven't bothered.

        // We patch all full stop ISO entries: because this is what the old code does...
        // Probably works badly with Auto ISO if it picks a non-full ISO.

        uint32_t val = *(uint32_t *)(old_values + item_size * i);
        if ((val & 0xfff00000) == 0x0b400000) // sanity check.  b4 is, apparently, the ADTG command.
                                              // The top 4 bits are meaningful, but always 0 in the array.
                                              // Purpose is unknown, they are non-zero in some different contexts.
        {
            val &= (~iso_mask); // mask out old other ISO
            val |= other_iso_bits; // mask in new
            *(uint32_t *)(new_values + item_size * i) = val;
        }
    }
    struct patch patch =
    {
        .addr = (uint8_t *)(start_addr),
        .old_values = old_values,
        .new_values = new_values,
        .size = table_size,
        .description = "dual_iso: CMOS[0] gains",
        .is_instruction = 0
    };
    // NB this won't apply patch if location is already patched
    result = apply_patches(&patch, 1);

end:
    free(new_values);
    return result;
}

// Return value is non-zero for error, but errors can be positive or negative.
static int patch_cmos_iso_values_6d2(uint32_t start_addr, int item_size, int count)
{
    uint32_t table_size = item_size * count;
    int32_t result = 0;

    // save an alloc by getting space for both at once
    uint8_t *new_values = malloc(table_size * 2);
    if (new_values == NULL)
        return -2;
    uint8_t *old_values = new_values + table_size;

    memcpy(new_values, (uint8_t *)start_addr, table_size);
    memcpy(old_values, (uint8_t *)start_addr, table_size);

    // ISO bits are 0x0000_0ff0, we will be preserving half and replacing half.
    uint32_t iso_mask = 0x000000f0;

    uint32_t other_iso_i = get_alternate_iso_index();
    // could use CMOS_ISO_BITS for the mask, with some shifts, like dual_iso_enable() does:
    uint32_t other_iso_bits = *(uint32_t *)(old_values + item_size * other_iso_i) & iso_mask;
    if (other_iso_bits != 0x00
        && other_iso_bits != 0x10
        && other_iso_bits != 0x20
        && other_iso_bits != 0x30
        && other_iso_bits != 0x40
        && other_iso_bits != 0x50
        && other_iso_bits != 0x60
        && other_iso_bits != 0x70) // one more "native" ISO than 200D
    {
        // ISO values are unexpected, we don't want to patch using them
        DryosDebugMsg(0, 15, "weird ISO bits: 0x%x", other_iso_bits);
        DryosDebugMsg(0, 15, "from: 0x%x", (start_addr + item_size * other_iso_i));
        result = other_iso_bits;
        goto end;
    }

    for (int i = 0; i < count; i++)
    {
        // 0d03a000 == 100, 0d03a440 == 1600
        // field is 0xURUUU AB F, AB are ISO values, R is probably CMOS register,
        // U are unknown.  F is probably a flag, purpose unknown.
        //
        // Old cams calculate this in a slightly more generic way, using
        // CMOS_FLAG_BITS, CMOS_ISO_BITS, and some shifting.  The specific
        // logic used doesn't work for the 6D2 data structure.

        uint32_t val = *(uint32_t *)(old_values + item_size * i);
        if ((val & 0xffff0000) == 0x0d030000) // Sanity check.  You can log the commands with adtglog2.mo
                                              // to find patterns.
        {
            val &= (~iso_mask); // mask out old other ISO
            val |= other_iso_bits; // mask in new
            *(uint32_t *)(new_values + item_size * i) = val;
        }
    }
    struct patch patch =
    {
        .addr = (uint8_t *)(start_addr),
        .old_values = old_values,
        .new_values = new_values,
        .size = table_size,
        .description = "dual_iso: CMOS[d] gains",
        .is_instruction = 0
    };
    // NB this won't apply patch if location is already patched
    result = apply_patches(&patch, 1);

end:
    free(new_values);
    return result;
}

// start_addr should be the address of one of the arrays of ADTG command values,
// which encode, amongst other things, the ISO values.
// There is a different array for photo and video.
//
// size is the length, in bytes, of an element in the array.
// However, we cheat on some cams.  We only want to patch full ISO stops,
// (I don't know why, it's just how the code worked when I got here...),
// and some cams store 1/3 stops.  So we lie about size and make it 3x larger,
// skipping the intermediate values.
//
// count is the number of items in the array.
//
// We don't patch all items in the array.  We only patch full stops,
// and some cams (at least 200D), store 1/3 stops.
static int dual_iso_enable(uint32_t start_addr, int size, int count, uint32_t* backup)
{
        /* for 7D */
        int start_addr_0 = start_addr;

        if (start_addr == 0)
            return 5;
        
        if (is_7d) /* start_addr is on master */
        {
            volatile uint32_t wait = 1;
            BulkInIPCTransfer(0, local_buf, size * count, start_addr, &bulk_cb, (uint32_t) &wait);
            while(wait)
                msleep(20);
            start_addr = (uint32_t) local_buf + 2; /* our numbers are aligned at 16 bits, but not at 32 */
        }

        // SJE FIXME this is rather ugly, making 200D special and skipping the function body.
        //
        // However, we don't want to use all the code below, it makes lots of undocumented
        // assumptions that I don't care to test & replicate, and we don't need as many
        // checks since we patch rom; can't get the wrong location due to heap layout.
        //
        // When more MMU cams are added we can find the correct place to split.
        if (is_200d)
        {
            return patch_cmos_iso_values_200d(start_addr, size, count);
        }

        // SJE FIXME now we have two special cases for modern cams...
        // The below logic assumes the CMOS data buf contains 16 bit items,
        // and the target CMOS reg is at a specific offset.  Neither of
        // these things holds for 6d2.  We also can't trivially adapt
        // the 200D logic, because the protocol is quite different there.
        // 4 apparent ISO channels and a different command structure.
        //
        // It's looking like the best solution is move this stuff into
        // the cam, making the code simpler in this module; just call the cam
        // func which would have a fixed name.
        // Or, give each cam a patch_cmos_iso_values_XXd() func and make this
        // func closer to a switch/case.
        if (is_6d2)
        {
            return patch_cmos_iso_values_6d2(start_addr, size, count);
        }

        // Get original values, used for sanity testing the address points at
        // tables of CMOS / ADTG values.
        // On some cams, FRAME_CMOS_ISO_START is not a fixed address so this will
        // work intermittently, we want to detect when this happens so we
        // only patch when it points to ISO table info.
        // I think 650d and 700d are known to do this.  The real fix is find
        // one level higher and get the pointer to the table, rather than
        // the variable table address directly.

        // We can't read values directly on ARMv5 due to unaligned read behaviour
        // interacting with apply_patches() / read_value() behaviour.
        for (int i = 0; i < count; i++)
            backup[i] = read_value((uint8_t *)(start_addr + size * i), 0);

        /* sanity check first */
        int prev_iso = 0;
        for (int i = 0; i < count; i++)
        {
            uint16_t raw = backup[i]; // NB this assumes both ISO values are encoded
                                      // in the low 16 bits, which is not true for modern cams.
                                      // Doesn't seem to be any point using a u16, but modern
                                      // cams currently don't reach this code (or use "backup" at all),
                                      // 200D returns earlier, so I'm not changing it yet.
                                      // See comment near CMOS_ISO_BITS for some ML assumptions.
                                      // The code below looks like it always extracts bits using
                                      // an AND mask, so we could safely make this u32?
            uint32_t flag = raw & CMOS_FLAG_MASK;
            int iso1 = (raw >> CMOS_FLAG_BITS) & CMOS_ISO_MASK;
            int iso2 = (raw >> (CMOS_FLAG_BITS + CMOS_ISO_BITS)) & CMOS_ISO_MASK;
            int reg  = (raw >> 12) & 0xF;

            if (reg != 0 && !is_6d)
                return reg;
            
            if (flag != CMOS_EXPECTED_FLAG)
                return 2;
            
            if (is_5d2)
                iso2 += iso1; // iso2 is 0 by default
            
            if (iso1 != iso2)
                return 3;
            
            if ( (iso1 < prev_iso) && !is_50d && !is_500d) // the list should be ascending
                return 4;
            
            prev_iso = iso1;
        }

        uint32_t cmos_bits_mask = ((1 << CMOS_ISO_BITS) - 1) << CMOS_FLAG_BITS;
        if (is_eosm || is_650d || is_700d || is_100d)
        {
            // Clear the MSB to fix line-skipping. 1 -> 8 lines, 0 -> 4 lines
            cmos_bits_mask |= 0x800;
        }

        if (is_5d2)
        {
            // iso2 is 0 by default, let's just patch that one
            cmos_bits_mask = cmos_bits_mask << CMOS_ISO_BITS;
        }

        // patch the stored ISO table to use our dual-iso values
        uint32_t patch_value;
        uint32_t cmos_bits;
        for (int i = 0; i < count; i++)
        {
            // get the CMOS bits from our target ISO
            cmos_bits = backup[COERCE(get_alternate_iso_index(), 0, count-1)] & cmos_bits_mask;

            if (is_5d2) // enable the dual ISO flag
                cmos_bits |= 1 << (CMOS_FLAG_BITS + CMOS_ISO_BITS + CMOS_ISO_BITS);

            // keep the bits that aren't CMOS bits
            patch_value = backup[i] & (~cmos_bits_mask);

            patch_value |= cmos_bits; // add the CMOS bits from our target ISO
            struct patch patch =
            {
                .addr = (uint8_t *)(start_addr + i * size),
                .old_value = backup[i],
                .new_value = patch_value,
                .size = 4,
                .description = "dual_iso: CMOS[0] gains",
                .is_instruction = 1
            };
            apply_patches(&patch, 1);
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

static int dual_iso_disable(uint32_t start_addr, int size, int count, uint32_t* backup)
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

    // undo our patches
    if (is_200d)
    {
        // This should be all CONFIG_MMU_REMAP cams but we don't currently
        // have a nice way of detecting that.

        //Here we do one large patch, thus only one unpatch
        unpatch_memory(start_addr);
    }
    else
    {
        for (int i = 0; i < count; i++)
            unpatch_memory(start_addr + i * size);
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

static struct semaphore *dual_iso_sem = NULL;

/* Photo mode: always enable */
/* LiveView: only enable in movie mode */
/* Refresh the parameters whenever you change something from menu */
static int enabled_lv = 0;
static int enabled_ph = 0;

/* thread safe */
static unsigned int dual_iso_refresh(unsigned int ctx)
{
    if (!job_state_ready_to_take_pic())
        return 0;

    if (dual_iso_sem == NULL)
        return 0;
    int err = take_semaphore(dual_iso_sem, 0);
    if (err)
        return 0; // this func is called by module_exec_cbr(),
                  // we return 0 on error, to allow later cbrs to run

    // SJE FIXME - have these backup arrays become redundant with patchmgr?
    // I believe so, patches store the old content at time of patch.
    // dual_iso_disable() should be able to revert to pre-patch values
    // and these arrays can be removed.
    static uint32_t backup_lv[20];
    static uint32_t backup_ph[20];
    int mv = is_movie_mode() ? 1 : 0;
    int lvi = lv ? 1 : 0;
    int raw_mv = mv && lv && raw_lv_is_enabled();
    int raw_ph = (pic_quality & 0xFE00FF) == (PICQ_RAW & 0xFE00FF);
    
    if (FRAME_CMOS_ISO_COUNT > COUNT(backup_ph)) goto end;
    if (PHOTO_CMOS_ISO_COUNT > COUNT(backup_lv)) goto end;
    
    static int prev_sig = 0;
    int sig = dual_iso_alternate_iso + (lvi << 16) + (raw_mv << 17) + (raw_ph << 18)
            + (dual_iso_hdr << 24) + (dual_iso_every_other << 25) + (dual_iso_file_prefix << 26)
            + get_shooting_card()->file_number * dual_iso_every_other + lens_info.raw_iso * 1234;
    int setting_changed = (sig != prev_sig);
    prev_sig = sig;
    
    if (enabled_lv && setting_changed)
    {
        dual_iso_disable(FRAME_CMOS_ISO_START, FRAME_CMOS_ISO_SIZE, FRAME_CMOS_ISO_COUNT, backup_lv);
        enabled_lv = 0;
    }
    
    if (enabled_ph && setting_changed)
    {
        dual_iso_disable(PHOTO_CMOS_ISO_START, PHOTO_CMOS_ISO_SIZE, PHOTO_CMOS_ISO_COUNT, backup_ph);
        enabled_ph = 0;
    }

    if (dual_iso_hdr && raw_ph && !enabled_ph && PHOTO_CMOS_ISO_START
        && ((get_shooting_card()->file_number % 2) || !dual_iso_every_other))
    {
        enabled_ph = 1;
        int err = dual_iso_enable(PHOTO_CMOS_ISO_START, PHOTO_CMOS_ISO_SIZE, PHOTO_CMOS_ISO_COUNT, backup_ph);
        if (err) { NotifyBox(10000, "ISOless PH err(%d)", err); enabled_ph = 0; }
    }
    
    if (dual_iso_hdr && raw_mv && !enabled_lv && FRAME_CMOS_ISO_START)
    {
        enabled_lv = 1;
        int err = dual_iso_enable(FRAME_CMOS_ISO_START, FRAME_CMOS_ISO_SIZE, FRAME_CMOS_ISO_COUNT, backup_lv);
        if (err) { NotifyBox(10000, "ISOless LV err(%d)", err); enabled_lv = 0; }
    }

    if (setting_changed)
    {
        /* hack: this may be executed when file_number is updated;
         * if so, it will rename the previous picture, captured with the old setting,
         * so it will mis-label the pics */
        int file_prefix_needs_delay = (ctx == CTX_SHOOT_TASK && lens_info.job_state);

        int iso1 = 72 + get_alternate_iso_index() * 8;
        int iso2 = lens_info.iso_analog_raw/8*8;

        static int prefix_key = 0;
        if (dual_iso_file_prefix && enabled_ph && iso1 != iso2)
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
    give_semaphore(dual_iso_sem);
    return 0;
}

int dual_iso_set_enabled(bool enabled)
{
    if (enabled)
        dual_iso_hdr = 1; 
    else
        dual_iso_hdr = 0;

    return 1; // module is loaded & responded != ret_0
}

int dual_iso_is_enabled()
{
    return dual_iso_hdr;
}

int dual_iso_is_active()
{
    return is_movie_mode() ? enabled_lv : enabled_ph;
}

int dual_iso_get_alternate_iso()
{
    if (!dual_iso_is_active())
        return 0;
    
    return 72 + get_alternate_iso_index() * 8;
}

int dual_iso_set_alternate_iso(int iso)
{
    if (!dual_iso_is_active())
        return 0;
    
    int max_index = MAX(FRAME_CMOS_ISO_COUNT, PHOTO_CMOS_ISO_COUNT) - 1;
    dual_iso_alternate_iso = COERCE((iso - 72)/8, 0, max_index);

    /* apply the new settings right now */
    dual_iso_refresh(CTX_SET_RECOVERY_ISO);
    return 1;
}

static unsigned int dual_iso_playback_fix(unsigned int ctx)
{
    if (is_7d || is_1100d)
        return 0; /* seems to cause problems, figure out why */
    
    if (!dual_iso_hdr) return 0;
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

static MENU_UPDATE_FUNC(dual_iso_check)
{
    int iso1 = 72 + get_alternate_iso_index() * 8;
    int iso2 = lens_info.iso_analog_raw/8*8;
    
    if (!iso2)
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Auto ISO => cannot estimate dynamic range.");
    
    if (!iso2 && iso1 < 0)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Auto ISO => cannot use relative alternate ISO.");
    
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

static MENU_UPDATE_FUNC(dual_iso_dr_update)
{
    dual_iso_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
    {
        MENU_SET_VALUE("N/A");
        return;
    }
    
    int dr_improvement = dual_iso_get_dr_improvement() / 10;
    
    MENU_SET_VALUE("%d.%d EV", dr_improvement/10, dr_improvement%10);
}

static MENU_UPDATE_FUNC(dual_iso_overlap_update)
{
    int iso1 = 72 + get_alternate_iso_index() * 8;
    int iso2 = (lens_info.iso_analog_raw)/8*8;

    int iso_hi = MAX(iso1, iso2);
    int iso_lo = MIN(iso1, iso2);
    
    dual_iso_check(entry, info);
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

static MENU_UPDATE_FUNC(dual_iso_update)
{
    if (!dual_iso_hdr)
        return;

    int iso1 = 72 + get_alternate_iso_index() * 8;
    int iso2 = (lens_info.iso_analog_raw)/8*8;

    MENU_SET_VALUE("%d/%d", raw2iso(iso2), raw2iso(iso1));

    dual_iso_check(entry, info);
    if (info->warning_level >= MENU_WARN_ADVICE)
        return;
    
    int dr_improvement = dual_iso_get_dr_improvement() / 10;
    
    MENU_SET_RINFO("DR+%d.%d", dr_improvement/10, dr_improvement%10);
}

static struct menu_entry dual_iso_menu[] =
{
    {
        .name = "Dual ISO",
        .priv = &dual_iso_hdr,
        .update = dual_iso_update,
        .max = 1,
        .help  = "Alternate ISO for every 2 sensor scan lines.",
        .help2 = "With some clever post, you get less shadow noise (more DR).",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Recovery ISO",
                .priv = &dual_iso_alternate_iso,
                .update = dual_iso_check,
                .min = -12,
                .max = 6,
                .unit = UNIT_ISO,
                .choices = CHOICES("-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV", "+1 EV", "+2 EV", "+3 EV", "+4 EV", "+5 EV", "+6 EV", "100", "200", "400", "800", "1600", "3200", "6400"),
                .help  = "ISO for half of the scanlines (usually to recover shadows).",
                .help2 = "Can be absolute or relative to primary ISO from Canon menu.",
            },
            {
                .name = "Dynamic range gained",
                .update = dual_iso_dr_update,
                .icon_type = IT_ALWAYS_ON,
                .help  = "[READ-ONLY] How much more DR you get with current settings",
                .help2 = "(upper theoretical limit, estimated from DxO measurements)",
            },
            {
                .name = "Midtone overlapping",
                .update = dual_iso_overlap_update,
                .icon_type = IT_ALWAYS_ON,
                .help  = "[READ-ONLY] How much of midtones will get better resolution",
                .help2 = "Highlights/shadows will be half res, with aliasing/moire.",
            },
            {
                .name = "Every other frame",
                .priv = &dual_iso_every_other,
                .max = 1,
                .help = "Shoot one image with the hack, one without.",
            },
            {
                .name = "Custom file prefix",
                .priv = &dual_iso_file_prefix,
                .max = 1,
                .choices = CHOICES("OFF", "DUAL (unreliable!)"),
                .help  = "Change file prefix for dual ISO photos (e.g. DUAL0001.CR2).",
                .help2 = "Will not sync properly in burst mode or when taking pics quickly."
            },
            MENU_EOL,
        },
    },
};

// ISO is applied to sensor data via ADTG / CMOS chip.
// Many cams allow setting this per line, so we can capture two ISOs
// simultaneously.
//
// The CMOS ISO table is held in ram and used to map
// ISO selected by the OS into amplification per sensor line.
//
// We want the location of the ram copy, to modify it,
// but this is held in heap, so the address varies sometimes
// (depending on different factors including Canon mode
// settings, prefs etc).
//
// Early in boot on 550D (and others?), a DMA copy happens from
// the asset rom, specifically 0xf8910000.  There is some kind of
// linked-list struct associated with the copy, which holds
// the DMA src addr as well as the dst addr.  Therefore, we
// can find the ram addr by searching for the fixed rom addr.
//
// An even nicer way might be to walk the linked list of
// structs, but I don't know where they start.
static uint32_t get_photo_cmos_iso_start_550d(void)
{
    // The linked-lists seem to start near 0x3d0000.
    // It can be found around 0x2dd0, seem to be quite
    // a few system related pointers there.  0x800000
    // seems easily enough to always find our target.
    uint32_t addr = 0x3d0000;
    uint32_t max_search_addr = 0x800000;

    uint32_t rom_copy_start = 0xf8910000;
    uint32_t ram_copy_start = 0;

    // search for DMA src addr, to find our dst addr
    for (; addr < max_search_addr; addr += 4)
    {
        if (*(uint32_t *)addr == rom_copy_start)
        {
            // A bunch of checks to give us higher confidence
            // we found the right value.  So far, none of these
            // have been required; the first hit is the correct
            // one.  But these are cheap checks, and should avoid
            // ever finding a random match on the 32-bit DMA addr value.

            uint32_t *probe = (uint32_t *)addr;
            // we expect to find 2 copies of the DMA src addr nearby
            if (probe[0] != probe[4])
                continue;
            if (probe[0] != probe[5])
                continue;

            ram_copy_start = probe[6] + 0xde4;
            // we expect this to be Uncacheable
            if (ram_copy_start == (uint32_t)CACHEABLE(ram_copy_start))
                continue;

            // we expect the next field to be the original addr
            // before it was rounded up to meet DMA alignment,
            // which looks to be 0x100 though I'm not sure.
            uint32_t aligned_val = probe[7] + (-probe[7] & 0xff);
            if (probe[6] != aligned_val)
                continue;

            // passed all checks, stop search
            qprintf("Found ram_copy_start, 0x%08x: 0x%08x\n",
                    &probe[6], ram_copy_start);
            printf("r_c_s %08x: %08x\n",
                   &probe[6], ram_copy_start);
            break;
        }
    }
    if (*(uint32_t *)addr != rom_copy_start || addr >= max_search_addr)
    {
        qprintf("Failed to find rom_copy_start!\n");
        printf("Failed to find r_c_s!\n");
        return 0; // failed to find target
    }

    return ram_copy_start;
}

// See the variant for 550d above, that largely applies here.
// 650d however holds the tables in SFDATA.BIN / serial flash.
// The copy actions are visible in qemu logs with EEPROM-DMA prefix.
// I see a copy to 40470f00 of len bb860 covering the ISO tables
// (and I assume a lot of other stuff).
static uint32_t get_photo_cmos_iso_start_650d(void)
{
    // 650D has different structs related to DMA transfers
    // (because it's SF transfer?).  The struct for this transfer
    // ends up persistently in I think heap, somewhere in the
    // 0x4XXXXX region.  I've seen anywhere from 450000 to 4a0000.
    //
    // That struct contains the src addr, always 340000 in my testing
    // (I assume this is fixed for a given rom version but don't know,
    // the value is logged in the EEPROM-DMA string in qemu).
    //
    // ROM function ff122548() builds the struct.

    uint32_t addr = 0x300000;
    uint32_t max_search_addr = 0x600000;

    uint32_t rom_copy_start = 0x340000;
    uint32_t ram_copy_start = 0;

    // search for DMA src addr, to find our dst addr
    for (; addr < max_search_addr; addr += 4)
    {
        if (*(uint32_t *)addr == rom_copy_start)
        {
            // A bunch of checks to give us higher confidence
            // we found the right value.  So far, none of these
            // have been required; the first hit is the correct
            // one.  But these are cheap checks, and should avoid
            // ever finding a random match on the 32-bit DMA addr value.

            uint32_t *probe = (uint32_t *)addr;
            // we expect to find 4 copies of the DMA src addr nearby
            if (probe[0] != probe[1])
                continue;
            if (probe[0] != probe[4])
                continue;
            if (probe[0] != probe[5])
                continue;

            // probe[6] is expected to hold the aligned start
            // of the dest heap address.  PHOTO_CMOS_ISO_START
            // is offset by 0x1244.
            ram_copy_start = probe[6] + 0x1244;
            // we expect this to be Uncacheable
            if (ram_copy_start == (uint32_t)CACHEABLE(ram_copy_start))
                continue;

            // we expect the next field to be the original addr
            // before it was rounded up to meet DMA alignment
            // (0x100 aligned on this 650D version)
            uint32_t aligned_val = probe[7] + (-probe[7] & 0xff);
            if (probe[6] != aligned_val)
                continue;

            // passed all checks, stop search
            qprintf("Found ram_copy_start, 0x%08x: 0x%08x\n",
                    &probe[6], ram_copy_start);
            break;
        }
    }
    if (*(uint32_t *)addr != rom_copy_start || addr >= max_search_addr)
    {
        qprintf("Failed to find rom_copy_start!\n");
        printf("Failed to find r_c_s!\n");
        return 0; // failed to find target
    }

    return ram_copy_start;
}

// 6d2 has some differences to earlier cams.  Unlike 200d,
// I couldn't find a table of fixed commands in ROM.
// It seems that a template command is copied rom -> ram,
// as well as a short table of just the different ISO command words.
// At runtime, the template command is patched with the relevant
// ISO, then sent.
//
// Patching the rom copy doesn't work, presumably this happens too late.
// Patching the heap copy is a bad idea, these addresses, while
// much more stable than you might guess, are not fully stable.
//
// Here we search for what I believe to be heap or DMA memcpy metadata,
// which holds the src and dst addr of the DMA transfer used for
// the rom -> ram copy.  The offset from that start addr is fixed,
// and the search is reliable.
static uint32_t get_photo_cmos_iso_start_6d2(void)
{
    // The struct for the rom -> ram transfer ends up in the
    // 0x7b_0000 region.  How much this can vary hasn't been tested much.
    //
    // That struct contains the src addr, e198_0000.

    uint32_t addr = 0x780000;
    uint32_t max_search_addr = 0x880000;

    uint32_t rom_copy_start = 0xe1980000;
    uint32_t ram_copy_start = 0;

    // search for DMA src addr, to find our dst addr
    for (; addr < max_search_addr; addr += 4)
    {
        if (*(uint32_t *)addr == rom_copy_start)
        {
            // A bunch of checks to give us higher confidence
            // we found the right value.  So far, none of these
            // have been required; the first hit is the correct
            // one.  But these are cheap checks, and should avoid
            // ever finding a random match on the 32-bit DMA addr value.

            uint32_t *probe = (uint32_t *)addr;
            // we expect to find 4 copies of the DMA src addr nearby
            if (probe[0] != probe[1])
                continue;
            if (probe[0] != probe[4])
                continue;
            if (probe[0] != probe[5])
                continue;

            // probe[6] is expected to hold the aligned start
            // of the dest heap address (often, 0x415a_7000).
            // PHOTO_CMOS_ISO_START is offset by 0xb30.
            ram_copy_start = probe[6] + 0xb30;
            // we expect this to be Uncacheable
            if (ram_copy_start == (uint32_t)CACHEABLE(ram_copy_start))
                continue;

            // passed all checks, stop search
            qprintf("Found ram_copy_start, 0x%08x: 0x%08x\n",
                    &probe[6], ram_copy_start);
            break;
        }
    }
    if (*(uint32_t *)addr != rom_copy_start || addr >= max_search_addr)
    {
        qprintf("Failed to find rom_copy_start!\n");
        printf("Failed to find r_c_s!\n");
        return 0; // failed to find target
    }

    return ram_copy_start;
}

/* callback routine for mlv_rec to add a custom DISO block after recording started (which already was specified in mlv.h in definition phase) */
static void dual_iso_mlv_rec_cbr (uint32_t event, void *ctx, mlv_hdr_t *hdr)
{
    /* construct a free-able pointer to later pass it to mlv_rec_queue_block */
    mlv_diso_hdr_t *dual_iso_block = malloc(sizeof(mlv_diso_hdr_t));
    
    /* set the correct type and size */
    mlv_set_type((mlv_hdr_t *)dual_iso_block, "DISO");
    dual_iso_block->blockSize = sizeof(mlv_diso_hdr_t);
    
    /* and fill with data */
    dual_iso_block->dualMode = dual_iso_is_active();
    dual_iso_block->isoValue = dual_iso_alternate_iso;
    
    /* finally pass it to mlv_rec which will free the block when it has been processed */
    mlv_rec_queue_block((mlv_hdr_t *)dual_iso_block);
}

static unsigned int dual_iso_init()
{
    dual_iso_sem = create_named_semaphore("dual_iso_sem", SEM_CREATE_UNLOCKED);

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
    else if (is_camera("6D2", "1.1.1"))
    {
        is_6d2 = 1;
//        PHOTO_CMOS_ISO_START = 0xe1980b30; // array of 8 ISO change commands; doesn't change photo ISO
//        PHOTO_CMOS_ISO_COUNT = 8;
//        PHOTO_CMOS_ISO_SIZE  = 0x4;

        // this works, but I want a less janky way of locating the array,
        // these heap addresses are not reliable on some cams
//        PHOTO_CMOS_ISO_START = 0x415a7b30; // array of 8 ISO change commands, ram copy
        PHOTO_CMOS_ISO_START = get_photo_cmos_iso_start_6d2();
        PHOTO_CMOS_ISO_COUNT = 8;
        PHOTO_CMOS_ISO_SIZE  = 4;

        CMOS_ISO_BITS = 4; // bit size of *each* ISO field
        CMOS_FLAG_BITS = 4; // bit size of all flags
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
    else if (is_camera("70D", "1.1.2"))
    {
        /* Movie Mode CMOS[0]
        100 - 0x404e77d6 value (0x3)
        200 - 0x404e7804 value (0x27)
        400 - 0x404e7832 value (0x4b)
        800 - 0x404e7860 value (0x6f)
        1600 -0x404e788e value (0x93)
        3200 -0x404e78bc value (0xb7)
        6400 -0x404e78ea value (0xdb) */

        is_70d = 1;

        /* FRAME_CMOS_ISO_START = 0x404e77d6; // CMOS register 0000 - for LiveView, ISO 100 (check in movie mode, not photo!)
        FRAME_CMOS_ISO_COUNT =          7; // from ISO 100 to 6400 (last real iso!)
        FRAME_CMOS_ISO_SIZE  =         46; // distance between ISO 100 and ISO 200 addresses, in bytes */

        /* WE DO NOT SEEM TO BE ABLE TO USE DUAL ISO IN MOVIE MODE */
        /* MORE CONFUSING IS THAT WE ARE INDEED ABLE TO USE */
        /* CMOS[0] OR CMOS[3]. BOTH SEEM TO WORK WELL IN PHOTO MODE */
        /* FOR NOW LET US OPT FOR CMOS[0] BUT THE QUESTION IS: */
        /* COULD WE TAKE ADVANTAGE OF USING BOTH AT THE SAME TIME (TRIPLE ISO)? */

        /* Photo Mode CMOS[0]
        100 - 0x404e5664 value (0x3)
        200 - 0x404e5678 value (0x27)
        400 - 0x404e568c value (0x4b)
        800 - 0x404e56a0 value (0x6f)
        1600 -0x404e56b4 value (0x93)
        3200 -0x404e56c8 value (0xb7)
        6400 -0x404e56dc value (0xdb) */

        PHOTO_CMOS_ISO_START = 0x404e5664; // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          7; // from ISO 100 to 6400 (last real iso!)
        PHOTO_CMOS_ISO_SIZE  =         20; // distance between ISO 100 and ISO 200 addresses, in bytes

        CMOS_ISO_BITS = 3;      // unverified
        CMOS_FLAG_BITS = 2;     // unverified
        CMOS_EXPECTED_FLAG = 3; // unverified
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

        //  00 0000 406941E4  = 100
        //  00 0024 406941F6  = 200
        //  00 0048 40694208  = 400
        //  00 006C 4069421A  = 800
        //  00 0090 4069422C  = 1600
        //  00 00B4 4069423E  = 3200

        PHOTO_CMOS_ISO_START = get_photo_cmos_iso_start_550d(); // CMOS register 0000 - for photo mode, ISO 100
        PHOTO_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        PHOTO_CMOS_ISO_SIZE  =         18; // distance between ISO 100 and ISO 200 addresses, in bytes

        FRAME_CMOS_ISO_START = 0;
        if (PHOTO_CMOS_ISO_START != 0)
            FRAME_CMOS_ISO_START = PHOTO_CMOS_ISO_START + 0x12b0; // CMOS register 0000 - for LiveView, ISO 100 (check in movie mode, not photo!)
        FRAME_CMOS_ISO_COUNT =          6; // from ISO 100 to 3200
        FRAME_CMOS_ISO_SIZE  =         30; // distance between ISO 100 and ISO 200 addresses, in bytes

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
    else if (is_camera("200D", "1.0.1"))
    {
        is_200d = 1;
//        PHOTO_CMOS_ISO_START = 0xe1984538; // This array doesn't change stills, purpose unknown
//        PHOTO_CMOS_ISO_START = 0xe19819c0; // This one doesn't change stills either
        PHOTO_CMOS_ISO_START = 0xe0aaa2fc; // changes stills, not video
        // This array starts with 0b400000, repeating that 2 times.
        // So, ISO 100 is the base ISO twice?  From then on, it's ISO 200 3 times
        // for all ISO up to 6400 / 0x0b466660; this one repeats 7 times.
        // The pattern seems to be for a given base ISO, e.g. ISO 100, the +2/3
        // is "pulled down" from ISO 200 somehow, and ISO 100 + 1/3 is "pushed up" from 100.
        //
        // This suggests the top 7 are 3200 + 2/3, 6400, +1/3, +2/3,
        // then 12800, 25600, maybe 51200?  No third stops for the very high ISOs?
        // And presumably the extra amplification above 6400 is configured elsewhere.

        // ML code assumes we're only interested in full stops, and it assumes they're evenly spaced.

        // So, we lie in the following values (maybe in the same way as old cams?
        // haven't verified), and the "count" is only full stops, meaning
        // the "size" is 3x larger than reality.  Perhaps this was always
        // the intention, if so these are just bad names.
        PHOTO_CMOS_ISO_COUNT = 7; // 6400, 12800, 25600 all use 0b466660,
                                  // and maybe don't have the 1/3 stops,
                                  // so this is probably the last one we can use,
                                  // without understanding how the extra boost is configured.
        PHOTO_CMOS_ISO_SIZE  = 24; // 0x18, 0x8 * 3

        // The same array has 4 sets of values (or 4 contiguous arrays, more likely).
        // +2fc changes stills, but not photo LV
        // +3bc changes video and video LV (but not photo LV),
        //      since 200D uses line skipping this doesn't work with the current
        //      un-dual-iso code, you get odd patterns of bright/dark lines
        // +47c changes photo LV (but not photos), and neither vid LV nor video
        // +53c changes x5 and x10 zoom modes

        FRAME_CMOS_ISO_START = 0xe0aaa53c;
        FRAME_CMOS_ISO_COUNT = 7; // SJE FIXME: as above, we are cheating here,
                                  // skipping patching intermediate ISOs.  Does
                                  // this matter for auto ISO video?
        FRAME_CMOS_ISO_SIZE  = 24; // 0x18, 0x8 * 3

        CMOS_ISO_BITS = 8;
        CMOS_FLAG_BITS = 4;
        CMOS_EXPECTED_FLAG = 0;
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
        // ISO values are 0x0803, 0x0827 style
        is_650d = 1;

        PHOTO_CMOS_ISO_START = get_photo_cmos_iso_start_650d();
        PHOTO_CMOS_ISO_COUNT =          6;
        PHOTO_CMOS_ISO_SIZE  =       0x10;

        FRAME_CMOS_ISO_START = 0;
        if (PHOTO_CMOS_ISO_START != 0)
            FRAME_CMOS_ISO_START = PHOTO_CMOS_ISO_START + 0x124a;
        FRAME_CMOS_ISO_COUNT =          6;
        FRAME_CMOS_ISO_SIZE  =       0x22;

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
        menu_add("Expo", dual_iso_menu, COUNT(dual_iso_menu));
    }
    else
    {
        dual_iso_hdr = 0;
        return 1;
    }
    
    mlv_rec_register_cbr(MLV_REC_EVENT_PREPARING, &dual_iso_mlv_rec_cbr, NULL);
    
    return 0;
}

static unsigned int dual_iso_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(dual_iso_init)
    MODULE_DEINIT(dual_iso_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_SHOOT_TASK, dual_iso_refresh, CTX_SHOOT_TASK)
    MODULE_CBR(CBR_SHOOT_TASK, dual_iso_playback_fix, CTX_SHOOT_TASK)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(dual_iso_hdr)
    MODULE_CONFIG(dual_iso_alternate_iso)
    MODULE_CONFIG(dual_iso_every_other)
    MODULE_CONFIG(dual_iso_file_prefix)
MODULE_CONFIGS_END()
