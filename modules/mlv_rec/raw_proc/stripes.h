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

#ifndef _stripes_h
#define _stripes_h

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <raw.h>

typedef struct {
    int correction_needed;
    int coeffficients[8];
} stripes_correction;

void fix_vertical_stripes(uint16_t * image_data,
                          int32_t black_level,
                          int32_t white_level,
                          int32_t raw_info_frame_size,
                          uint16_t width,
                          uint16_t height,
                          int vertical_stripes,
                          int show_progress);
#endif
