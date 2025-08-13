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

/* do not use typeof in macros, use __typeof__ instead.
   see: http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Alternate-Keywords.html#Alternate-Keywords
*/
#define MIN(a,b) \
   ({ __typeof__ ((a)+(b)) _a = (a); \
      __typeof__ ((a)+(b)) _b = (b); \
     _a < _b ? _a : _b; })

#define MAX(a,b) \
   ({ __typeof__ ((a)+(b)) _a = (a); \
       __typeof__ ((a)+(b)) _b = (b); \
     _a > _b ? _a : _b; })

#define ABS(a) \
   ({ __typeof__ (a) _a = (a); \
     _a > 0 ? _a : -_a; })

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

#define STR_APPEND(orig,fmt,...) do { int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); } while(0)

struct raw_8pixels
{
   uint16_t a;
   uint16_t b;
   uint16_t c;
   uint16_t d;
   uint16_t e;
   uint16_t f;
   uint16_t g;
   uint16_t h;
};

#define PA (p->a)
#define PB (p->b)
#define PC (p->c)
#define PD (p->d)
#define PE (p->e)
#define PF (p->f)
#define PG (p->g)
#define PH (p->h)

#define SET_PA(v) (p->a = v)
#define SET_PB(v) (p->b = v)
#define SET_PC(v) (p->c = v)
#define SET_PD(v) (p->d = v)
#define SET_PE(v) (p->e = v)
#define SET_PF(v) (p->f = v)
#define SET_PG(v) (p->g = v)
#define SET_PH(v) (p->h = v)

#define RAW_MUL(p, x) ((((int)(p) - black_level) * (int)(x) / FIXP_ONE) + black_level)
#define F2H(ev) COERCE((int)(FIXP_RANGE/2 + ev * FIXP_RANGE/2), 0, FIXP_RANGE-1)
#define H2F(x) ((double)((x) - FIXP_RANGE/2) / (FIXP_RANGE/2))

static void add_pixel(int hist[8][FIXP_RANGE], int num[8], int offset, int pa, int pb, int32_t white_level)
{
    int a = pa;
    int b = pb;
    
    if (MIN(a,b) < 32)
        return; /* too noisy */

    if (MAX(a,b) > white_level / 1.1)
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
    int weight = log2(a);
    hist[offset][F2H(ev)] += weight;
    num[offset] += weight;
}


static void detect_vertical_stripes_coeffs(stripes_correction * correction,
                                           uint16_t * image_data,
                                           int32_t black_level,
                                           int32_t white_level,
                                           int32_t raw_info_frame_size,
                                           uint16_t width,
                                           uint16_t height)
{
    static int hist[8][FIXP_RANGE];
    static int num[8];
    memset(hist, 0, sizeof(hist));
    memset(num, 0, sizeof(num));

    int pitch = width * 2;

    /* compute 7 histograms: b./a, c./a ... h./a */
    /* that is, adjust all columns to make them as bright as a */
    /* process green pixels only, assuming the image is RGGB */
    struct raw_8pixels * row;
    for (row = (void*)image_data; (void*)row < (void*)image_data + pitch * height; row += 2 * pitch / sizeof(struct raw_8pixels))
    {
        /* first line is RG */
        struct raw_8pixels * rg;
        for (rg = row; (void*)rg < (void*)row + pitch - sizeof(struct raw_8pixels); rg++)
        {
            /* next line is GB */
            struct raw_8pixels * gb = rg + pitch / sizeof(struct raw_8pixels);

            struct raw_8pixels * p = rg;
            int pb = PB - black_level;
            int pd = PD - black_level;
            int pf = PF - black_level;
            int ph = PH - black_level;
            p++;
            int pb2 = PB - black_level;
            int pd2 = PD - black_level;
            int pf2 = PF - black_level;
            int ph2 = PH - black_level;
            p = gb;
            //int pa = PA - black_level;
            int pc = PC - black_level;
            int pe = PE - black_level;
            int pg = PG - black_level;
            p++;
            int pa2 = PA - black_level;
            int pc2 = PC - black_level;
            int pe2 = PE - black_level;
            int pg2 = PG - black_level;
            
            /**
             * verification: introducing strong banding in one column
             * should not affect the coefficients from the other columns
             **/

            //~ pe = pe * 1.1;
            //~ pe2 = pe2 * 1.1;
            
            /**
             * Make all columns as bright as a2
             * use linear interpolation, so when processing column b, for example,
             * let bi = (b * 1 + b2 * 7) / (7+1)
             * let ei = (e * 4 + e2 * 4) / (4+4)
             * and so on, to avoid getting tricked by smooth gradients.
             */

            add_pixel(hist, num, 1, pa2, (pb * 1 + pb2 * 7) / 8, white_level);
            add_pixel(hist, num, 2, pa2, (pc * 2 + pc2 * 6) / 8, white_level);
            add_pixel(hist, num, 3, pa2, (pd * 3 + pd2 * 5) / 8, white_level);
            add_pixel(hist, num, 4, pa2, (pe * 4 + pe2 * 4) / 8, white_level);
            add_pixel(hist, num, 5, pa2, (pf * 5 + pf2 * 3) / 8, white_level);
            add_pixel(hist, num, 6, pa2, (pg * 6 + pg2 * 2) / 8, white_level);
            add_pixel(hist, num, 7, pa2, (ph * 7 + ph2 * 1) / 8, white_level);
        }
    }

    int j,k;
    
    int max[8] = {0};
    for (j = 0; j < 8; j++)
        for (k = 1; k < FIXP_RANGE-1; k++)
            max[j] = MAX(max[j], hist[j][k]);

    /* compute the median correction factor (this will reject outliers) */
    for (j = 0; j < 8; j++)
    {
        if (num[j] < raw_info_frame_size / 128) continue;
        int t = 0;
        for (k = 0; k < FIXP_RANGE; k++)
        {
            t += hist[j][k];
            if (t >= num[j]/2)
            {
                int c = pow(2, H2F(k)) * FIXP_ONE;
                correction->coeffficients[j] = c;
                break;
            }
        }
    }

#if 0
    /* debug graphs */
    FILE* f = fopen("debug_graph.m", "w");
    fprintf(f, "h = {}; x = {}; c = \"rgbcmy\"; \n");
    for (j = 2; j < 8; j++)
    {
        fprintf(f, "h{end+1} = [");
        for (k = 1; k < FIXP_RANGE-1; k++)
        {
            fprintf(f, "%d ", hist[j][k]);
        }
        fprintf(f, "];\n");

        fprintf(f, "x{end+1} = [");
        for (k = 1; k < FIXP_RANGE-1; k++)
        {
            fprintf(f, "%f ", H2F(k) );
        }
        fprintf(f, "];\n");
        fprintf(f, "plot(log2(%d/%d) + [0 0], [0 %d], ['*-' c(%d)]); hold on;\n", correction[j], FIXP_ONE, max[j], j-1);
    }
    fprintf(f, "for i = 1:6, plot(x{i}, h{i}, c(i)); hold on; end;");
    fprintf(f, "axis([-0.05 0.05])");
    fclose(f);
    system("octave-cli --persist debug_graph.m");
#endif

    correction->coeffficients[0] = FIXP_ONE;

    /* do we really need stripe correction, or it won't be noticeable? or maybe it's just computation error? */
    correction->correction_needed = 0;
    for (j = 0; j < 8; j++)
    {
        double c = (double)correction->coeffficients[j] / FIXP_ONE;
        if (c < 0.998 || c > 1.002)
            correction->correction_needed = 1;
    }
}

static void apply_vertical_stripes_correction(stripes_correction * correction,
                                              uint16_t * image_data,
                                              int32_t black_level,
                                              int32_t white_level,
                                              uint16_t width,
                                              uint16_t height)
{
    /**
     * inexact white level will result in banding in highlights, especially if some channels are clipped
     * 
     * so... we'll try to use a better estimation of white level *for this particular purpose*
     * start with a gross under-estimation, then consider white = max(all pixels)
     * just in case the exif one is way off
     * reason: 
     *   - if there are no pixels above the true white level, it shouldn't hurt;
     *     worst case, the brightest pixel(s) will be underexposed by 0.1 EV or so
     *   - if there are, we will choose the true white level
     */
     
    int white = white_level * 2 / 3;
    int pitch = width * 2;

    struct raw_8pixels * row;
    for (row = (void*)image_data; (void*)row < (void*)image_data + pitch * height; row += pitch / sizeof(struct raw_8pixels))
    {
        struct raw_8pixels * p;
        for (p = row; (void*)p < (void*)row + pitch; p++)
        {
            white = MAX(white, PA);
            white = MAX(white, PB);
            white = MAX(white, PC);
            white = MAX(white, PD);
            white = MAX(white, PE);
            white = MAX(white, PF);
            white = MAX(white, PG);
            white = MAX(white, PH);
        }
    }
    
    int black = black_level;
    for (row = (void*)image_data; (void*)row < (void*)image_data + pitch * height; row += pitch / sizeof(struct raw_8pixels))
    {
        struct raw_8pixels * p;
        for (p = row; (void*)p < (void*)row + pitch; p++)
        {
            int pa = PA;
            int pb = PB;
            int pc = PC;
            int pd = PD;
            int pe = PE;
            int pf = PF;
            int pg = PG;
            int ph = PH;
            
            /**
             * Thou shalt not exceed the white level (the exact one, not the exif one)
             * otherwise you'll be blessed with banding instead of nice and smooth highlight recovery
             * 
             * At very dark levels, you will introduce roundoff errors, so don't correct there
             */
            
            if (correction->coeffficients[0] && pa && pa < white && pa > black + 64) SET_PA(MIN(white, RAW_MUL(pa, correction->coeffficients[0])));
            if (correction->coeffficients[1] && pb && pb < white && pa > black + 64) SET_PB(MIN(white, RAW_MUL(pb, correction->coeffficients[1])));
            if (correction->coeffficients[2] && pc && pc < white && pa > black + 64) SET_PC(MIN(white, RAW_MUL(pc, correction->coeffficients[2])));
            if (correction->coeffficients[3] && pd && pd < white && pa > black + 64) SET_PD(MIN(white, RAW_MUL(pd, correction->coeffficients[3])));
            if (correction->coeffficients[4] && pe && pe < white && pa > black + 64) SET_PE(MIN(white, RAW_MUL(pe, correction->coeffficients[4])));
            if (correction->coeffficients[5] && pf && pf < white && pa > black + 64) SET_PF(MIN(white, RAW_MUL(pf, correction->coeffficients[5])));
            if (correction->coeffficients[6] && pg && pg < white && pa > black + 64) SET_PG(MIN(white, RAW_MUL(pg, correction->coeffficients[6])));
            if (correction->coeffficients[7] && ph && ph < white && pa > black + 64) SET_PH(MIN(white, RAW_MUL(ph, correction->coeffficients[7])));
        }
    }
}

void fix_vertical_stripes(uint16_t * image_data,
                          int32_t black_level,
                          int32_t white_level,
                          int32_t raw_info_frame_size,
                          uint16_t width,
                          uint16_t height,
                          int vertical_stripes,
                          int show_progress)
{
    static stripes_correction correction;

    /* for speed: only detect correction factors from the first frame if not --force-stripes specified */
    static int first_time = 1;
    if (first_time || vertical_stripes == 2)
    {
        detect_vertical_stripes_coeffs(&correction, image_data, black_level, white_level, raw_info_frame_size, width, height);
        
        if (first_time && show_progress)
        {
            const char * method = NULL;
            if (vertical_stripes == 2)
            {
                method = "FORCED";
            }
            else if (correction.correction_needed)
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
                if (correction.coeffficients[j])
                    printf("  %.5f", (double)correction.coeffficients[j] / FIXP_ONE);
                else
                    printf("    1  ");
            }
            printf("\n\n");
        }

        first_time = 0;
    }

    apply_vertical_stripes_correction(&correction, image_data, black_level, white_level, width, height);
}
