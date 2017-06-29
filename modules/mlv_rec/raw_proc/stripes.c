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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stripes.h"

/* Vertical stripes correction code from raw2dng, credits: a1ex */

/**
 * Fix vertical stripes (banding) from 5D Mark III (and maybe others).
 *
 * These stripes are periodic, they repeat every 8 pixels.
 * It looks like some columns have different luma amplification;
 * correction factors are somewhere around 0.98 - 1.02, maybe camera-specific, maybe depends on
 * certain settings, I have no idea. So, this fix compares luma values within one pixel block,
 * computes the correction factors (using median to reject outliers) and decides
 * whether to apply the correction or not.
 *
 * For speed reasons:
 * - Correction factors are computed from the first frame only.
 * - Only channels with error greater than 0.2% are corrected.
 */

#define FIXP_ONE 65536
#define FIXP_RANGE 65536

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))
#define F2H(ev) COERCE((int)(FIXP_RANGE/2 + ev * FIXP_RANGE/2), 0, FIXP_RANGE-1)
#define H2F(x) ((double)((x) - FIXP_RANGE/2) / (FIXP_RANGE/2))

static void add_pixel(int * hist, int num[8], int offset, int pa, int pb, struct raw_info * raw_info)
{
    int a = pa;
    int b = pb;
    
    if (MIN(a,b) < 32)
        return; /* too noisy */
    
    if (MAX(a,b) > raw_info->white_level / 1.5)
        return; /* too bright */
    
    /**
     * compute correction factor for b, that makes it as bright as a
     *
     * first, work around quantization error (which causes huge spikes on histogram)
     * by adding a small random noise component
     * e.g. if raw value is 13, add some uniformly distributed noise,
     * so the value will be between -12.5 and 13.5.
     *
     * this removes spikes on the histogram, thus canceling bias towards "round" values
     */
    double af = a + (rand() % 1024) / 1024.0 - 0.5;
    double bf = b + (rand() % 1024) / 1024.0 - 0.5;
    double factor = af / bf;
    double ev = log2(factor);
    
    /**
     * add to histogram (for computing the median)
     */
    int weight = 1;
    hist[offset * FIXP_RANGE + F2H(ev)] += weight;
    num[offset] += weight;
}


void stripes_compute_correction(struct stripes_correction * correction, uint16_t * image_data, struct raw_info * raw_info, uint16_t width, uint16_t height)
{
    int * hist = malloc(sizeof(int) * 8 * FIXP_RANGE);
    int num[8];
    
    memset(hist, 0, sizeof(int) * 8 * FIXP_RANGE);
    memset(num, 0, sizeof(num));
    
    /* compute 8 little histograms */
    for (int y = 0; y < height; y++)
    {
        int row_start = y * width;
        for (int x = row_start; x < row_start + width - 10; x += 8)
        {
            int pa = image_data[x] - raw_info->black_level;
            int pb = image_data[x + 1] - raw_info->black_level;
            int pc = image_data[x + 2] - raw_info->black_level;
            int pd = image_data[x + 3] - raw_info->black_level;
            int pe = image_data[x + 4] - raw_info->black_level;
            int pf = image_data[x + 5] - raw_info->black_level;
            int pg = image_data[x + 6] - raw_info->black_level;
            int ph = image_data[x + 7] - raw_info->black_level;
            int pa2 = image_data[x + 8] - raw_info->black_level;
            int pb2 = image_data[x + 9] - raw_info->black_level;
            
            /**
             * weight according to distance between corrected and reference pixels
             * e.g. pc is 2px away from pa, but 6px away from pa2, so pa/pc gets stronger weight than pa2/p3
             * the improvement is visible in horizontal gradients
             */
            
            add_pixel(hist, num, 2, pa, pc, raw_info);
            add_pixel(hist, num, 2, pa, pc, raw_info);
            add_pixel(hist, num, 2, pa, pc, raw_info);
            add_pixel(hist, num, 2, pa2, pc, raw_info);
            
            add_pixel(hist, num, 3, pb, pd, raw_info);
            add_pixel(hist, num, 3, pb, pd, raw_info);
            add_pixel(hist, num, 3, pb, pd, raw_info);
            add_pixel(hist, num, 3, pb2, pd, raw_info);
            
            add_pixel(hist, num, 4, pa, pe, raw_info);
            add_pixel(hist, num, 4, pa, pe, raw_info);
            add_pixel(hist, num, 4, pa2, pe, raw_info);
            add_pixel(hist, num, 4, pa2, pe, raw_info);
            
            add_pixel(hist, num, 5, pb, pf, raw_info);
            add_pixel(hist, num, 5, pb, pf, raw_info);
            add_pixel(hist, num, 5, pb2, pf, raw_info);
            add_pixel(hist, num, 5, pb2, pf, raw_info);
            
            add_pixel(hist, num, 6, pa, pg, raw_info);
            add_pixel(hist, num, 6, pa2, pg, raw_info);
            add_pixel(hist, num, 6, pa2, pg, raw_info);
            add_pixel(hist, num, 6, pa2, pg, raw_info);
            
            add_pixel(hist, num, 7, pb, ph, raw_info);
            add_pixel(hist, num, 7, pb2, ph, raw_info);
            add_pixel(hist, num, 7, pb2, ph, raw_info);
            add_pixel(hist, num, 7, pb2, ph, raw_info);
        }
    }
    
    int j,k;
    
    int max[8] = {0};
    for (j = 0; j < 8; j++)
    {
        for (k = 1; k < FIXP_RANGE-1; k++)
        {
            max[j] = MAX(max[j], hist[j * FIXP_RANGE + k]);
        }
    }
    
    /* compute the median correction factor (this will reject outliers) */
    for (j = 0; j < 8; j++)
    {
        if (num[j] < raw_info->frame_size / 128) continue;
        int t = 0;
        for (k = 0; k < FIXP_RANGE; k++)
        {
            t += hist[j * FIXP_RANGE + k];
            if (t >= num[j]/2)
            {
                int c = (int)(pow(2, H2F(k)) * FIXP_ONE);
                correction->coeffficients[j] = c;
                break;
            }
        }
    }
    
    correction->coeffficients[0] = FIXP_ONE;
    correction->coeffficients[1] = FIXP_ONE;
    
    /* do we really need stripe correction, or it won't be noticeable? or maybe it's just computation error? */
    correction->correction_needed = 0;
    for (j = 0; j < 8; j++)
    {
        double c = (double)correction->coeffficients[j] / FIXP_ONE;
        if (c < 0.998 || c > 1.002)
            correction->correction_needed = 1;
    }
    
    free(hist);
}

void stripes_apply_correction(struct stripes_correction * correction, uint16_t * image_data, off_t offset, size_t size, struct raw_info * raw_info, uint16_t width)
{
    /* only apply stripe correction if we need it */
    if(correction == NULL || !correction->correction_needed) return;
    if(width % 8 != 0) return;

    uint16_t black = raw_info->black_level;
    uint16_t white = raw_info->white_level;
    size_t start = offset % 8;
    for(size_t i = 0; i < size; i++)
    {
        double correction_coeffficient = correction->coeffficients[(i + start) % 8];
        if(correction_coeffficient && image_data[i] > black + 64)
        {
            image_data[i] = (uint16_t)MIN(white, (image_data[i] - black) * correction_coeffficient / FIXP_ONE + black);
        }
    }
}

void fix_vertical_stripes(uint16_t * image_data, off_t offset, size_t size, struct raw_info * raw_info, uint16_t width, uint16_t height, int vertical_stripes, int show_progress)
{
    static struct stripes_correction corrections;

    /* for speed: only detect correction factors from the first frame if not --force-stripes specified */
    static int first_time = 1;
    if (first_time || vertical_stripes == 2)
    {
        stripes_compute_correction(&corrections, image_data, raw_info, width, height);
        
        if (first_time && show_progress)
        {
            const char * method = NULL;
            if (vertical_stripes == 2)
            {
                method = "FORCED";
            }
            else if (corrections.correction_needed)
            {
                method = "NEEDED";
            }
            else
            {
                method = "UNNEEDED";
            }
            
            printf("\nVertical stripes correction: '%s'\n", method);
            for (int j = 0; j < 8; j++)
            {
                if (corrections.coeffficients[j])
                    printf("  %.5f", (double)corrections.coeffficients[j] / FIXP_ONE);
                else
                    printf("    1  ");
            }
            printf("\n");
        }
        
        first_time = 0;
    }
    
    stripes_apply_correction(&corrections, image_data, offset, size, raw_info, width);
}
