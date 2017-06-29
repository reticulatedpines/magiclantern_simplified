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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "histogram.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

/**
 * Initialize a histogram
 */
struct histogram * hist_create(uint16_t white)
{
    struct histogram * hist = (struct histogram *)malloc(sizeof(struct histogram));
    if(hist != NULL)
    {
        hist->white = white;
        hist->count = 0;
        hist->data = (uint16_t *)malloc((white + 1) * sizeof(uint16_t));
        if(hist->data != NULL)
        {
            memset(hist->data, 0, (white + 1) * sizeof(uint16_t));
        }
    }
    return hist;
}

/**
 * Add data to a histogram
 */
void hist_add(struct histogram * hist, uint16_t * data, uint32_t size, uint16_t skip)
{
    for(uint32_t i = 0; i < size; i += (skip + 1))
    {
        hist->data[MIN(hist->white, data[i])]++;
    }
    hist->count += size / (skip + 1);
}

/**
 * Compute the median
 */
uint16_t hist_median(struct histogram * hist)
{
    uint32_t middle = hist->count / 2;
    uint32_t current = 0;
    
    for(uint16_t i = 0; i <= hist->white; i++)
    {
        current += hist->data[i];
        if(current > middle) return i;
    }
    return 0;
}

/**
 * Free memory resources for a histogram
 */
void hist_destroy(struct histogram * hist)
{
    free(hist->data);
    free(hist);
}
