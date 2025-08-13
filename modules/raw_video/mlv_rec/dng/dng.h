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
    int fps_override;     // switch "-A fpsx1000"

    /* buffer where the image is stored as the camera-written RAWI isn't suitable for x64 systems */
    void *frame_buffer;
    int frame_buffer_size;

    /* flags */
    int deflicker_target; // "--deflicker=value"
    int vertical_stripes; // 0 - "--no-stripes", 1 - no switch (default), 2 - "--force-stripes"
    int focus_pixels;     // 0 - "--no-fixfp", 1 - no switch (default)
    int bad_pixels;       // 0 - "--no-fixcp", 1 - no switch (default), 2 - "--fixcp2" (makes algorithm aggresive to reveal more bad pixels)
    int save_bpm;         // "--save-bpm" (saves bad pixel map to file)
    int dual_iso;         // "--is-dualiso" (means RAW data is dual iso process bad/focus pixels correctly (can be removed if DISO block parsing implemented)
    int chroma_smooth;    // 2 - "--cs2x2", 3 "--cs3x3", 5 - "--cs5x5"
    int pattern_noise;    // "--fixpn" (fixes pattern noise)
    int show_progress;    // "--show-progress" (verbose mode for 'dng.c')
    int raw_state;        // see 'enum raw_state' above
    int pack_bits;        // 0 - "--no-bitpack" (saves 16bit dngs), 1 - bit packing will be done (default)
    int fpi_method;       // 0 - "--fpi 0" mlvfs focus pixel interpolation  (default), "--fpi 1" raw2dng focus pixel interpolation
    int bpi_method;       // 0 - "--bpi 0" mlvfs bad pixel interpolation (default), "--bpi 1" raw2dng bad pixel interpolation
    int crop_rec;         // 0 - no crop_rec, 1 - crop_rec

    /* block headers */
    mlv_vidf_hdr_t vidf_hdr;
    mlv_file_hdr_t file_hdr;
    mlv_rtci_hdr_t rtci_hdr;
    mlv_idnt_hdr_t idnt_hdr;
    mlv_rawi_hdr_t rawi_hdr;
    mlv_rawc_hdr_t rawc_hdr;
    mlv_expo_hdr_t expo_hdr;
    mlv_lens_hdr_t lens_hdr;
    mlv_wbal_hdr_t wbal_hdr;
    char *info_str;
};

/* buffers of DNG header and image data */
struct dng_data
{
    size_t header_size;             // dng header size
    size_t image_size;              // raw image buffer size
    size_t image_size_bitpacked;    // bit packed raw image buffer size
    size_t image_size_bak;          // image size backup (needed to restore original size)

    uint8_t * header_buf;           // pointer to header buffer
    uint16_t * image_buf;           // pointer to image buffer
    uint16_t * image_buf_bitpacked; // pointer to bit packed image buffer
    uint16_t * image_buf_bak;       // backup of pointer to image buffer (needed to restore pointer to original buffer)
};

/* routines to initialize, process and free raw image buffers of 'dng_data' struct */
void dng_init_header(struct frame_info * frame_info, struct dng_data * dng_data);
void dng_init_data(struct frame_info * frame_info, struct dng_data * dng_data);
void dng_process_data(struct frame_info * frame_info, struct dng_data * dng_data);
void dng_free_data(struct dng_data * dng_data);

/* routines to unpack and pack bits */
void dng_unpack_image_bits(uint16_t * input_buffer, uint16_t * output_buffer, size_t max_size, uint32_t bpp);
void dng_pack_image_bits(uint16_t * input_buffer, uint16_t * output_buffer, size_t max_size, uint32_t bpp);

/* routine to save cdng file */
int dng_save(struct frame_info * frame_info, struct dng_data * dng_data);

#endif
