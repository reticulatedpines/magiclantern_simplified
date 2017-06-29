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

enum { TYPE_FOCUS, TYPE_BAD };

struct parameter_list
{
	char * mlv_name;
	int dual_iso;
	int aggressive;
	int save_bpm;
	int show_progress;	
	
	uint32_t camera_id;
	uint16_t width;
	uint16_t height;
	uint16_t pan_x;
    uint16_t pan_y;
	int32_t raw_width;
	int32_t raw_height;
	int32_t black_level;
};

void chroma_smooth(uint16_t * image_data, int width, int height, int black, int method);
void fix_pixels(int map_type, uint16_t * image_data, struct parameter_list par);
void free_pixel_maps();

#endif
