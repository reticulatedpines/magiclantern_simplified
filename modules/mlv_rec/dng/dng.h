/*
 * Copyright (C) 2014 David Milligan
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

#ifndef _dng_h
#define _dng_h

#include <sys/types.h>
#include <raw.h>
#include "../mlv.h"

/* raw state: 
   0 - uncompressed/decompressed
   1 - compressed/recompressed
   2 - original uncompressed
   3 - original lossless
*/
enum raw_state
{
    UNCOMPRESSED_RAW    = 0,
    COMPRESSED_RAW      = 1,
    UNCOMPRESSED_ORIG   = 2,
    COMPRESSED_ORIG     = 3
};

/* all mlv block headers needed to generate a DNG frame plus extra parameters */
struct frame_info
{
    char * mlv_filename;
    char * dng_filename;
    int fps_override;

    /* flags */
    int deflicker_target;
    int vertical_stripes;
    int bad_pixels;
    int save_bpm;
    int dual_iso;
    int chroma_smooth;
    int pattern_noise;
    int show_progress;
    int raw_state;
    int pack_bits;

    /* block headers */
    mlv_vidf_hdr_t vidf_hdr;
    mlv_file_hdr_t file_hdr;
    mlv_rtci_hdr_t rtci_hdr;
    mlv_idnt_hdr_t idnt_hdr;
    mlv_rawi_hdr_t rawi_hdr;
    mlv_expo_hdr_t expo_hdr;
    mlv_lens_hdr_t lens_hdr;
    mlv_wbal_hdr_t wbal_hdr;
};

/* buffers of DNG header and image data */
struct dng_data
{
    size_t header_size;
    size_t image_size;
    size_t image_size_bitpacked;
    size_t image_size_bak;

    uint8_t * header_buf;
    uint16_t * image_buf;
    uint16_t * image_buf_bitpacked;
    uint16_t * image_buf_bak;
};

void dng_init_header(struct frame_info * frame_info, struct dng_data * dng_data);
void dng_init_data(struct frame_info * frame_info, struct dng_data * dng_data);
void dng_process_data(struct frame_info * frame_info, struct dng_data * dng_data);
void dng_free_data(struct dng_data * dng_data);

void dng_unpack_image_bits(uint16_t * input_buffer, uint16_t * output_buffer, size_t max_size, uint32_t bpp);
void dng_pack_image_bits(uint16_t * input_buffer, uint16_t * output_buffer, size_t max_size, uint32_t bpp);

int dng_save(struct frame_info * frame_info, struct dng_data * dng_data);

#endif
