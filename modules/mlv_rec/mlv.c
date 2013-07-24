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
#include "raw.h"
#include "mlv.h"

extern uint64_t get_us_clock_value();
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, int n);


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
    for(int pos = 0; pos < 64; pos += 8)
    {
        guid = guid ^ now.tm_sec;
        guid = guid ^ now.tm_min;
        guid = guid ^ now.tm_hour;
        guid = guid ^ now.tm_yday;
        guid = guid ^ now.tm_year;
    }
    
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
