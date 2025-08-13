/*
* Copyright (C) 2014 David Milligan
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

#ifndef mlvfs_dng_h
#define mlvfs_dng_h

#include "mlv.h"

#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))

//all the mlv block headers corresponding to a particular frame, needed to generate a DNG for that frame
struct frame_headers
{
	uint32_t fileNumber;
	uint64_t position;
	mlv_vidf_hdr_t vidf_hdr;
	mlv_file_hdr_t file_hdr;
	mlv_rtci_hdr_t rtci_hdr;
	mlv_idnt_hdr_t idnt_hdr;
	mlv_rawi_hdr_t rawi_hdr;
	mlv_expo_hdr_t expo_hdr;
	mlv_lens_hdr_t lens_hdr;
	mlv_wbal_hdr_t wbal_hdr;
};

__declspec(dllexport) size_t dng_get_header_data(struct frame_headers * frame_headers, uint8_t * output_buffer, off_t offset, size_t max_size);
__declspec(dllexport) size_t dng_get_header_size(struct frame_headers * frame_headers);
__declspec(dllexport) size_t dng_get_image_data(struct frame_headers * frame_headers, uint8_t * packed_buffer, uint8_t * output_buffer, off_t offset, size_t max_size);
__declspec(dllexport) size_t dng_get_image_size(struct frame_headers * frame_headers);
__declspec(dllexport) size_t dng_get_size(struct frame_headers * frame_headers);

#endif
