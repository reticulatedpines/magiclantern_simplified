/*
 * Copyright (C) 2014 The Magic Lantern Team
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

#ifndef _pixel_proc_h
#define _pixel_proc_h

#include <stdio.h>

/* struct of parameters to pass to fix_pixels() routine */
struct parameter_list
{
	char * mlv_name;
	int dual_iso;
	int aggressive;
	int save_bpm;
	int show_progress;
	int fpi_method;
	int bpi_method;
	int crop_rec;
	int unified; // If raw data is restricted to 8-12bit lossless use unified, all in one aggressive focus pixel map
	
	uint32_t camera_id;
	uint16_t width;
	uint16_t height;
	uint16_t pan_x;
	uint16_t pan_y;
	int32_t raw_width;
	int32_t raw_height;
	int32_t black_level;
};

/* do chroma smoothing with methods: 2x2, 3x3 and 5x5 */
void chroma_smooth(uint16_t * image_data, int width, int height, int black, int white, int method);
/* fix focus raw pixels */
void fix_focus_pixels(uint16_t * image_data, struct parameter_list par);
/* fix all kind of bad raw pixels */
void fix_bad_pixels(uint16_t * image_data, struct parameter_list par);
/* free bufers used for raw processing */
void free_pixel_maps();

#endif
