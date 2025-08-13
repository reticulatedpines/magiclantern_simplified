/*
 * Copyright (C) 2017 Magic Lantern Team
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

#ifndef _wav_h_
#define _wav_h_

static const char * iXML =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<BWFXML>"
"<IXML_VERSION>1.5</IXML_VERSION>"
"<PROJECT>%s</PROJECT>"
"<NOTE>%s</NOTE>"
"<CIRCLED>FALSE</CIRCLED>"
"<BLACKMAGIC-KEYWORDS>%s</BLACKMAGIC-KEYWORDS>"
"<TAPE>%d</TAPE>"
"<SCENE>%d</SCENE>"
"<BLACKMAGIC-SHOT>%d</BLACKMAGIC-SHOT>"
"<TAKE>%d</TAKE>"
"<BLACKMAGIC-ANGLE>ms</BLACKMAGIC-ANGLE>"
"<SPEED>"
"<MASTER_SPEED>%d/%d</MASTER_SPEED>"
"<CURRENT_SPEED>%d/%d</CURRENT_SPEED>"
"<TIMECODE_RATE>%d/%d</TIMECODE_RATE>"
"<TIMECODE_FLAG>NDF</TIMECODE_FLAG>"
"</SPEED>"
"</BWFXML>";

#pragma pack(push,1)

struct wav_bext {
    char description[256];
    char originator[32];
    char originator_reference[32];
    char origination_date[10];      //yyyy:mm:dd
    char origination_time[8];       //hh:mm:ss
    uint64_t time_reference;
    uint16_t version;
    uint8_t umid[64];
    int16_t loudness_value;
    int16_t loudness_range;
    int16_t max_true_peak_level;
    int16_t max_momentary_loudness;
    int16_t max_short_term_loudness;
    uint8_t reserved[180];
    char coding_history[4];
};

struct wav_header {
    //file header
    char RIFF[4];               // "RIFF"
    uint32_t file_size;
    char WAVE[4];               // "WAVE"
    //bext subchunk
    char bext_id[4];
    uint32_t bext_size;
    struct wav_bext bext;
    //iXML subchunk
    char iXML_id[4];
    uint32_t iXML_size;
    char iXML[1024];
    //subchunk1
    char fmt[4];                // "fmt"
    uint32_t subchunk1_size;    // 16
    uint16_t audio_format;      // 1 (PCM)
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;       // ???
    uint16_t bits_per_sample;
    //subchunk2
    char data[4];               // "data"
    uint32_t subchunk2_size;
    //audio data start
};

#pragma pack(pop)

#endif