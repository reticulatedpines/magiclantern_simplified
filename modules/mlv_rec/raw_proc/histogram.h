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

#ifndef mlvfs_histogram_h
#define mlvfs_histogram_h

#include <stdio.h>

#pragma pack(push,1)

struct histogram
{
    uint16_t white;
    uint32_t count;
    uint16_t * data;
};

#pragma pack(pop)

struct histogram * hist_create(uint16_t white);
void hist_add(struct histogram * hist, uint16_t * data, uint32_t size, uint16_t skip);
uint16_t hist_median(struct histogram * hist);
void hist_destroy(struct histogram * hist);

#endif
