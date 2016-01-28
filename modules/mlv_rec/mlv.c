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

#include <dryos.h>
#include <stdint.h>
#include <lens.h>
#include <property.h>
#include <propvalues.h>
#include <picstyle.h>
#include <raw.h>
#include <fps.h>

#include "mlv.h"
#include "../trace/trace.h"

extern uint32_t raw_rec_trace_ctx;

extern uint64_t get_us_clock_value();
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, int n);
extern const char* get_picstyle_name(int raw_picstyle);

extern struct prop_picstyle_settings picstyle_settings[];

void mlv_fill_lens(mlv_lens_hdr_t *hdr, uint64_t start_timestamp)
{
    /* prepare header */
    mlv_set_type((mlv_hdr_t *)hdr, "LENS");
    mlv_set_timestamp((mlv_hdr_t *)hdr, start_timestamp);
    hdr->blockSize = sizeof(mlv_lens_hdr_t);

    hdr->focalLength = lens_info.focal_len;
    hdr->focalDist = lens_info.focus_dist;
    hdr->aperture = lens_info.aperture * 10;
    hdr->stabilizerMode = lens_info.IS;
    hdr->lensID = lens_info.lens_id;
    hdr->autofocusMode = af_mode;
    hdr->flags = 0;

    strncpy((char *)hdr->lensName, lens_info.name, 32);
    strncpy((char *)hdr->lensSerial, "", 32);
}

void mlv_fill_wbal(mlv_wbal_hdr_t *hdr, uint64_t start_timestamp)
{
    /* prepare header */
    mlv_set_type((mlv_hdr_t *)hdr, "WBAL");
    mlv_set_timestamp((mlv_hdr_t *)hdr, start_timestamp);
    hdr->blockSize = sizeof(mlv_wbal_hdr_t);

    hdr->wb_mode = lens_info.wb_mode;
    hdr->kelvin = lens_info.kelvin;
    hdr->wbgain_r = lens_info.WBGain_R;
    hdr->wbgain_g = lens_info.WBGain_G;
    hdr->wbgain_b = lens_info.WBGain_B;
    hdr->wbs_gm = lens_info.wbs_gm;
    hdr->wbs_ba = lens_info.wbs_ba;
}

void mlv_fill_styl(mlv_styl_hdr_t *hdr, uint64_t start_timestamp)
{
    /* prepare header */
    mlv_set_type((mlv_hdr_t *)hdr, "STYL");
    mlv_set_timestamp((mlv_hdr_t *)hdr, start_timestamp);
    hdr->blockSize = sizeof(mlv_styl_hdr_t);

    hdr->picStyleId = lens_info.raw_picstyle;
    hdr->contrast = lens_get_contrast();
    hdr->sharpness = lens_get_sharpness();
    hdr->saturation = lens_get_saturation();
    hdr->colortone = lens_get_color_tone();

    strncpy((char *)hdr->picStyleName, get_picstyle_name(lens_info.raw_picstyle), sizeof(hdr->picStyleName));
}

void mlv_fill_expo(mlv_expo_hdr_t *hdr, uint64_t start_timestamp)
{
    /* prepare header */
    mlv_set_type((mlv_hdr_t *)hdr, "EXPO");
    mlv_set_timestamp((mlv_hdr_t *)hdr, start_timestamp);
    hdr->blockSize = sizeof(mlv_expo_hdr_t);

    /* iso is zero when auto-iso is enabled */
    if(lens_info.iso == 0)
    {
        hdr->isoMode = 1;
        hdr->isoValue = lens_info.iso_auto;
    }
    else
    {
        hdr->isoMode = 0;
        hdr->isoValue = lens_info.iso;
    }
    hdr->isoAnalog = lens_info.iso_analog_raw;
    hdr->digitalGain = lens_info.iso_digital_ev;
    hdr->shutterValue = (uint32_t)(1000.0f * (1000000.0f / (float)get_current_shutter_reciprocal_x1000()));
}

void mlv_fill_rtci(mlv_rtci_hdr_t *hdr, uint64_t start_timestamp)
{
    struct tm now;
    memset(&now, 0x00, sizeof(struct tm));

    /* prepare header */
    mlv_set_type((mlv_hdr_t *)hdr, "RTCI");
    mlv_set_timestamp((mlv_hdr_t *)hdr, start_timestamp);
    hdr->blockSize = sizeof(mlv_rtci_hdr_t);

    /* get calendar time from real time clock */
    LoadCalendarFromRTC(&now);

    hdr->tm_sec = now.tm_sec;
    hdr->tm_min = now.tm_min;
    hdr->tm_hour = now.tm_hour;
    hdr->tm_mday = now.tm_mday;
    hdr->tm_mon = now.tm_mon;
    hdr->tm_year = now.tm_year;
    hdr->tm_wday = now.tm_wday;
    hdr->tm_yday = now.tm_yday;
    hdr->tm_isdst = now.tm_isdst;
    hdr->tm_gmtoff = now.tm_gmtoff;

    memset(hdr->tm_zone, 0x00, 8);
    strncpy((char *)hdr->tm_zone, now.tm_zone, 8);
}

void mlv_fill_idnt(mlv_idnt_hdr_t *hdr, uint64_t start_timestamp)
{
    /* prepare header */
    mlv_set_type((mlv_hdr_t *)hdr, "IDNT");
    mlv_set_timestamp((mlv_hdr_t *)hdr, start_timestamp);
    hdr->blockSize = sizeof(mlv_idnt_hdr_t);
    
    hdr->cameraModel = camera_model_id;
    memcpy(hdr->cameraName, camera_model, 32);
    memcpy(hdr->cameraSerial, camera_serial, 32);

    trace_write(raw_rec_trace_ctx, "[IDNT] cameraName: '%s' cameraModel: 0x%08X cameraSerial: '%s'", hdr->cameraName, hdr->cameraModel, hdr->cameraSerial);
}

uint64_t mlv_prng_lfsr(uint64_t value)
{
    uint64_t lfsr = value;
    int max_clocks = 512;

    for(int clocks = 0; clocks < max_clocks; clocks++)
    {
        /* maximum length LFSR according to http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf */
        int bit = ((lfsr >> 63) ^ (lfsr >> 62) ^ (lfsr >> 60) ^ (lfsr >> 59)) & 1;
        lfsr = (lfsr << 1) | bit;
    }

    return lfsr;
}

uint64_t mlv_generate_guid()
{
    struct tm now;
    uint64_t guid = get_us_clock_value();
    LoadCalendarFromRTC(&now);

    /* now run through prng once to shuffle bits */
    guid = mlv_prng_lfsr(guid);

    /* then seed shuffled bits with rtc time */
    guid ^= now.tm_sec;
    guid ^= now.tm_min << 7;
    guid ^= now.tm_hour << 12;
    guid ^= now.tm_yday << 17;
    guid ^= now.tm_year << 26;
    guid ^= get_us_clock_value() << 37;

    /* now run through final prng pass */
    return mlv_prng_lfsr(guid);
}

void mlv_init_fileheader(mlv_file_hdr_t *hdr)
{
    mlv_set_type((mlv_hdr_t*)hdr, "MLVI");
    hdr->blockSize = sizeof(mlv_file_hdr_t);
    strncpy((char*)hdr->versionString, MLV_VERSION_STRING, sizeof(hdr->versionString));
}

void mlv_set_type(mlv_hdr_t *hdr, char *type)
{
    memcpy(hdr->blockType, type, 4);
}

uint64_t mlv_set_timestamp(mlv_hdr_t *hdr, uint64_t start)
{
    uint64_t timestamp = get_us_clock_value();

    if(hdr)
    {
        hdr->timestamp = timestamp - start;
    }
    return timestamp;
}
