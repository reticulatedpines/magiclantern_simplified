/*
 * Copyright (C) 2013 Magic Lantern Team
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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "lv_rec.h"
#include <raw.h>
#include <chdk-dng.h>
#include "qsort.h"  /* much faster than standard C qsort */
#include "../dual_iso/optmed.h"

lv_rec_file_footer_t lv_rec_footer;
struct raw_info raw_info;

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!ok) FAIL(fmt, ## __VA_ARGS__); }

static void fix_vertical_stripes();
static void chroma_smooth();

#define EV_RESOLUTION 32768

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf(
            "\n"
            "usage:\n"
            "\n"
            "%s file.raw [prefix]\n"
            "\n"
            " => will create prefix000000.dng, prefix0000001.dng and so on.\n"
            "\n",
            argv[0]
        );
        return 1;
    }
    
    FILE* fi = fopen(argv[1], "rb");
    CHECK(fi, "could not open %s", argv[1]);
    if (sizeof(lv_rec_file_footer_t) != 192) FAIL("sizeof(lv_rec_file_footer_t) = %d, should be 192", sizeof(lv_rec_file_footer_t));
    
    fseeko(fi, -192, SEEK_END);
    int r = fread(&lv_rec_footer, 1, sizeof(lv_rec_file_footer_t), fi);
    CHECK(r == sizeof(lv_rec_file_footer_t), "footer");
    raw_info = lv_rec_footer.raw_info;
    fseek(fi, 0, SEEK_SET);

    if (strncmp((char*)lv_rec_footer.magic, "RAWM", 4))
        FAIL("This ain't a lv_rec RAW file\n");
    
    if (raw_info.api_version != 1)
        FAIL("API version mismatch: %d\n", raw_info.api_version);
    
    /* override params here (e.g. when the footer is from some other file) */
    //~ lv_rec_footer.xRes=2048;
    //~ lv_rec_footer.yRes=1024;
    //~ lv_rec_footer.frameSize = lv_rec_footer.xRes * lv_rec_footer.yRes * 14/8;
    //~ lv_rec_footer.raw_info.white_level = 16383;
    
    printf("Resolution  : %d x %d\n", lv_rec_footer.xRes, lv_rec_footer.yRes);
    printf("Frames      : %d\n", lv_rec_footer.frameCount);
    printf("Frame size  : %d bytes\n", lv_rec_footer.frameSize);
    printf("FPS         : %d.%03d\n", lv_rec_footer.sourceFpsx1000/1000, lv_rec_footer.sourceFpsx1000%1000);
    printf("Black level : %d\n", lv_rec_footer.raw_info.black_level);
    printf("White level : %d\n", lv_rec_footer.raw_info.white_level);
    
    char* raw = malloc(lv_rec_footer.frameSize);
    CHECK(raw, "malloc");
    
    /* override the resolution from raw_info with the one from lv_rec_footer, if they don't match */
    if (lv_rec_footer.xRes != raw_info.width)
    {
        raw_info.width = lv_rec_footer.xRes;
        raw_info.pitch = raw_info.width * 14/8;
        raw_info.active_area.x1 = 0;
        raw_info.active_area.x2 = raw_info.width;
        raw_info.jpeg.x = 0;
        raw_info.jpeg.width = raw_info.width;
    }

    if (lv_rec_footer.yRes != raw_info.height)
    {
        raw_info.height = lv_rec_footer.yRes;
        raw_info.active_area.y1 = 0;
        raw_info.active_area.y2 = raw_info.height;
        raw_info.jpeg.y = 0;
        raw_info.jpeg.height = raw_info.height;
    }
    
    raw_info.frame_size = lv_rec_footer.frameSize;
    
    char* prefix = argc > 2 ? argv[2] : "";

    int i;
    for (i = 0; i < lv_rec_footer.frameCount; i++)
    {
        printf("\rProcessing frame %d of %d...", i+1, lv_rec_footer.frameCount);
        fflush(stdout);
        int r = fread(raw, 1, lv_rec_footer.frameSize, fi);
        CHECK(r == lv_rec_footer.frameSize, "fread");
        raw_info.buffer = raw;
        
        /* uncomment if the raw file is recovered from a DNG with dd */
        //~ reverse_bytes_order(raw, lv_rec_footer.frameSize);
        
        char fn[100];
        snprintf(fn, sizeof(fn), "%s%06d.dng", prefix, i);
        fix_vertical_stripes();
        chroma_smooth();
        dng_set_framerate(lv_rec_footer.sourceFpsx1000);
        save_dng(fn, &raw_info);
    }
    fclose(fi);
    printf("\nDone.\n");
    printf("\nTo convert to jpg, you can try: \n");
    printf("    ufraw-batch --out-type=jpg %s*.dng\n", prefix);
    printf("\nTo get a mjpeg video: \n");
    printf("    ffmpeg -i %s%%6d.jpg -vcodec mjpeg -qscale 1 video.avi\n\n", prefix);
    return 0;
}

int raw_get_pixel(int x, int y) {
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: return p->a;
        case 1: return p->b_lo | (p->b_hi << 12);
        case 2: return p->c_lo | (p->c_hi << 10);
        case 3: return p->d_lo | (p->d_hi << 8);
        case 4: return p->e_lo | (p->e_hi << 6);
        case 5: return p->f_lo | (p->f_hi << 4);
        case 6: return p->g_lo | (p->g_hi << 2);
        case 7: return p->h;
    }
    return p->a;
}

int raw_set_pixel(int x, int y, int value)
{
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: p->a = value; break;
        case 1: p->b_lo = value; p->b_hi = value >> 12; break;
        case 2: p->c_lo = value; p->c_hi = value >> 10; break;
        case 3: p->d_lo = value; p->d_hi = value >> 8; break;
        case 4: p->e_lo = value; p->e_hi = value >> 6; break;
        case 5: p->f_lo = value; p->f_hi = value >> 4; break;
        case 6: p->g_lo = value; p->g_hi = value >> 2; break;
        case 7: p->h = value; break;
    }
    return p->a;
}

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

static int stripes_coeffs[8] = {0};
static int stripes_correction_needed = 0;

#define MIN(a,b) \
   ({ typeof ((a)+(b)) _a = (a); \
      typeof ((a)+(b)) _b = (b); \
     _a < _b ? _a : _b; })

#define MAX(a,b) \
   ({ typeof ((a)+(b)) _a = (a); \
       typeof ((a)+(b)) _b = (b); \
     _a > _b ? _a : _b; })

#define ABS(a) \
   ({ __typeof__ (a) _a = (a); \
     _a > 0 ? _a : -_a; })

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

#define STR_APPEND(orig,fmt,...) ({ int _len = strlen(orig); snprintf(orig + _len, sizeof(orig) - _len, fmt, ## __VA_ARGS__); });

#define PA ((int)(p->a))
#define PB ((int)(p->b_lo | (p->b_hi << 12)))
#define PC ((int)(p->c_lo | (p->c_hi << 10)))
#define PD ((int)(p->d_lo | (p->d_hi << 8)))
#define PE ((int)(p->e_lo | (p->e_hi << 6)))
#define PF ((int)(p->f_lo | (p->f_hi << 4)))
#define PG ((int)(p->g_lo | (p->g_hi << 2)))
#define PH ((int)(p->h))

#define SET_PA(x) { int v = (x); p->a = v; }
#define SET_PB(x) { int v = (x); p->b_lo = v; p->b_hi = v >> 12; }
#define SET_PC(x) { int v = (x); p->c_lo = v; p->c_hi = v >> 10; }
#define SET_PD(x) { int v = (x); p->d_lo = v; p->d_hi = v >> 8; }
#define SET_PE(x) { int v = (x); p->e_lo = v; p->e_hi = v >> 6; }
#define SET_PF(x) { int v = (x); p->f_lo = v; p->f_hi = v >> 4; }
#define SET_PG(x) { int v = (x); p->g_lo = v; p->g_hi = v >> 2; }
#define SET_PH(x) { int v = (x); p->h = v; }

#define RAW_MUL(p, x) ((((int)(p) - raw_info.black_level) * (int)(x) / FIXP_ONE) + raw_info.black_level)
#define F2H(ev) COERCE((int)(FIXP_RANGE/2 + ev * FIXP_RANGE/2), 0, FIXP_RANGE-1)
#define H2F(x) ((double)((x) - FIXP_RANGE/2) / (FIXP_RANGE/2))

static void add_pixel(int hist[8][FIXP_RANGE], int num[8], int offset, int pa, int pb)
{
    int a = pa;
    int b = pb;
    
    if (MIN(a,b) < 32)
        return; /* too noisy */

    if (MAX(a,b) > raw_info.white_level / 1.5)
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
    hist[offset][F2H(ev)] += weight;
    num[offset] += weight;
}


static void detect_vertical_stripes_coeffs()
{
    static int hist[8][FIXP_RANGE];
    static int num[8];
    
    memset(hist, 0, sizeof(hist));
    memset(num, 0, sizeof(num));

    /* compute 8 little histograms */
    struct raw_pixblock * row;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.height; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch - sizeof(struct raw_pixblock);)
        {
            int pa = PA - raw_info.black_level;
            int pb = PB - raw_info.black_level;
            int pc = PC - raw_info.black_level;
            int pd = PD - raw_info.black_level;
            int pe = PE - raw_info.black_level;
            int pf = PF - raw_info.black_level;
            int pg = PG - raw_info.black_level;
            int ph = PH - raw_info.black_level;
            p++;
            int pa2 = PA - raw_info.black_level;
            int pb2 = PB - raw_info.black_level;
            //~ int pc2 = PC - raw_info.black_level;
            //~ int pd2 = PD - raw_info.black_level;
            //~ int pe2 = PE - raw_info.black_level;
            //~ int pf2 = PF - raw_info.black_level;
            //~ int pg2 = PG - raw_info.black_level;
            //~ int ph2 = PH - raw_info.black_level;
            
            /**
             * verification: introducing strong banding in one column
             * should not affect the coefficients from the other columns
             **/

            //~ pe = pe * 1.1;
            //~ pe2 = pe2 * 1.1;
            
            /**
             * weight according to distance between corrected and reference pixels
             * e.g. pc is 2px away from pa, but 6px away from pa2, so pa/pc gets stronger weight than pa2/p3
             * the improvement is visible in horizontal gradients
             */
            
            add_pixel(hist, num, 2, pa, pc);
            add_pixel(hist, num, 2, pa, pc);
            add_pixel(hist, num, 2, pa, pc);
            add_pixel(hist, num, 2, pa2, pc);

            add_pixel(hist, num, 3, pb, pd);
            add_pixel(hist, num, 3, pb, pd);
            add_pixel(hist, num, 3, pb, pd);
            add_pixel(hist, num, 3, pb2, pd);

            add_pixel(hist, num, 4, pa, pe);
            add_pixel(hist, num, 4, pa, pe);
            add_pixel(hist, num, 4, pa2, pe);
            add_pixel(hist, num, 4, pa2, pe);

            add_pixel(hist, num, 5, pb, pf);
            add_pixel(hist, num, 5, pb, pf);
            add_pixel(hist, num, 5, pb2, pf);
            add_pixel(hist, num, 5, pb2, pf);

            add_pixel(hist, num, 6, pa, pg);
            add_pixel(hist, num, 6, pa2, pg);
            add_pixel(hist, num, 6, pa2, pg);
            add_pixel(hist, num, 6, pa2, pg);

            add_pixel(hist, num, 7, pb, ph);
            add_pixel(hist, num, 7, pb2, ph);
            add_pixel(hist, num, 7, pb2, ph);
            add_pixel(hist, num, 7, pb2, ph);
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
        if (num[j] < raw_info.frame_size / 128) continue;
        int t = 0;
        for (k = 0; k < FIXP_RANGE; k++)
        {
            t += hist[j][k];
            if (t >= num[j]/2)
            {
                int c = pow(2, H2F(k)) * FIXP_ONE;
                stripes_coeffs[j] = c;
                break;
            }
        }
    }

#if 0
    /* debug graphs */
    FILE* f = fopen("raw2dng.m", "w");
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
        fprintf(f, "plot(log2(%d/%d) + [0 0], [0 %d], ['*-' c(%d)]); hold on;\n", stripes_coeffs[j], FIXP_ONE, max[j], j-1);
    }
    fprintf(f, "for i = 1:6, plot(x{i}, h{i}, c(i)); hold on; end;");
    fclose(f);
    system("octave --persist raw2dng.m");
#endif

    stripes_coeffs[0] = FIXP_ONE;
    stripes_coeffs[1] = FIXP_ONE;

    /* do we really need stripe correction, or it won't be noticeable? or maybe it's just computation error? */
    stripes_correction_needed = 0;
    for (j = 0; j < 8; j++)
    {
        double c = (double)stripes_coeffs[j] / FIXP_ONE;
        if (c < 0.998 || c > 1.002)
            stripes_correction_needed = 1;
    }
    
    if (stripes_correction_needed)
    {
        printf("\nVertical stripes correction:\n");
        for (j = 0; j < 8; j++)
        {
            if (stripes_coeffs[j])
                printf("  %.3f", (double)stripes_coeffs[j] / FIXP_ONE);
            else
                printf("    1  ");
        }
        printf("\n");
    }
}

static void apply_vertical_stripes_correction()
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
     
    int white = raw_info.white_level * 2 / 3;
    
    struct raw_pixblock * row;
    
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.height; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch; p++)
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
    
    int black = raw_info.black_level;
    for (row = raw_info.buffer; (void*)row < (void*)raw_info.buffer + raw_info.pitch * raw_info.height; row += raw_info.pitch / sizeof(struct raw_pixblock))
    {
        struct raw_pixblock * p;
        for (p = row; (void*)p < (void*)row + raw_info.pitch; p++)
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
            
            if (stripes_coeffs[0] && pa && pa < white && pa > black + 64) SET_PA(MIN(white, RAW_MUL(pa, stripes_coeffs[0])));
            if (stripes_coeffs[1] && pb && pb < white && pa > black + 64) SET_PB(MIN(white, RAW_MUL(pb, stripes_coeffs[1])));
            if (stripes_coeffs[2] && pc && pc < white && pa > black + 64) SET_PC(MIN(white, RAW_MUL(pc, stripes_coeffs[2])));
            if (stripes_coeffs[3] && pd && pd < white && pa > black + 64) SET_PD(MIN(white, RAW_MUL(pd, stripes_coeffs[3])));
            if (stripes_coeffs[4] && pe && pe < white && pa > black + 64) SET_PE(MIN(white, RAW_MUL(pe, stripes_coeffs[4])));
            if (stripes_coeffs[5] && pf && pf < white && pa > black + 64) SET_PF(MIN(white, RAW_MUL(pf, stripes_coeffs[5])));
            if (stripes_coeffs[6] && pg && pg < white && pa > black + 64) SET_PG(MIN(white, RAW_MUL(pg, stripes_coeffs[6])));
            if (stripes_coeffs[7] && ph && ph < white && pa > black + 64) SET_PH(MIN(white, RAW_MUL(ph, stripes_coeffs[7])));
        }
    }
}

static void fix_vertical_stripes()
{
    /* for speed: only detect correction factors from the first frame */
    static int first_time = 1;
    if (first_time)
    {
        detect_vertical_stripes_coeffs();
        first_time = 0;
    }
    
    /* only apply stripe correction if we need it, since it takes a little CPU time */
    if (stripes_correction_needed)
    {
        apply_vertical_stripes_correction();
    }
}

/* median of 4 numbers */
static int median4(int a, int b, int c, int d)
{
    int x[4] = {a,b,c,d};

    /* compute median */
    int aux;
    int i,j;
    for (i = 0; i < 3; i++)
        for (j = i+1; j < 4; j++)
            if (x[i] > x[j])
                aux = x[i], x[i] = x[j], x[j] = aux;
    int median = (x[1] + x[2]) / 2;
    return median;
}

/* AHD-like selection (if I understood the paper well)
 * choose the interpolation direction (L-R or T-B) based on some criteria (what looks best or something like this) */
static int ahd_like(int a, int b, int c, int d)
{
    int d1 = ABS(a-b);
    int d2 = ABS(c-d);
    return d1 < d2 ? (a+b)/2 : (c+d)/2;
}

static void chroma_smooth_3x3(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x,y;

    for (y = 4; y < h-5; y += 2)
    {
        for (x = 4; x < w-4; x += 2)
        {
            int i,j;
            int k = 0;
            int med_r[9];
            int med_b[9];
            int eh = 0;
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                    int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                    int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                    int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                    int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1]; g2 = raw2ev[g2]; g3 = raw2ev[g3]; g4 = raw2ev[g4]; g5 = raw2ev[g5]; g6 = raw2ev[g6];
                    
                    int gr = (g1+g3)/2;
                    int gb = (g2+g5)/2;
                    eh += ABS(g1-g3) + ABS(g2-g5);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            int drh = opt_med9(med_r);
            int dbh = opt_med9(med_b);
            

            int ev = 0;
            k = 0;
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                                                        /*  for R      for B      */
                    int g1 = inp[x+i+1 +   (y+j) * w];  /*  Right      Top        */
                    int g2 = inp[x+i   + (y+j+1) * w];  /*  Bottom     Left       */
                    int g3 = inp[x+i-1 +   (y+j) * w];  /*  Left                  */ 
                    int g4 = inp[x+i   + (y+j-1) * w];  /*  Top                   */
                    int g5 = inp[x+i+2 + (y+j+1) * w];  /*             Right      */
                    int g6 = inp[x+i+1 + (y+j+2) * w];  /*             Bottom     */
                    
                    g1 = raw2ev[g1]; g2 = raw2ev[g2]; g3 = raw2ev[g3]; g4 = raw2ev[g4]; g5 = raw2ev[g5]; g6 = raw2ev[g6];
                    
                    int gr = (g2+g4)/2;
                    int gb = (g1+g6)/2;
                    ev += ABS(g2-g4) + ABS(g1-g6);
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }

            int drv = opt_med9(med_r);
            int dbv = opt_med9(med_b);

            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int g3 = inp[x-1 +   (y) * w];
            int g4 = inp[x   + (y-1) * w];
            int g5 = inp[x+2 + (y+1) * w];
            int g6 = inp[x+1 + (y+2) * w];
            
            g1 = raw2ev[g1]; g2 = raw2ev[g2]; g3 = raw2ev[g3]; g4 = raw2ev[g4]; g5 = raw2ev[g5]; g6 = raw2ev[g6];
            
            int ev0 = ABS(g2-g4) + ABS(g1-g6);
            int eh0 = ABS(g1-g3) + ABS(g2-g5);

            int gr = ev0 < eh0 ? (g2+g4)/2 : (g1+g3)/2;
            int gb = ev0 < eh0 ? (g1+g6)/2 : (g2+g5)/2;
            int dr = ev0 < eh0 ? drv : drh;
            int db = ev0 < eh0 ? dbv : dbh;

            out[x   +     y * w] = ev2raw[COERCE(gr + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
            out[x+1 + (y+1) * w] = ev2raw[COERCE(gb + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
        }
    }
}

static void chroma_smooth_5x5(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x,y;

    for (y = 6; y < h-7; y += 2)
    {
        for (x = 4; x < w-4; x += 2)
        {
            int i,j;
            int k = 0;
            int med_r[25];
            int med_b[25];
            for (i = -4; i <= 4; i += 2)
            {
                for (j = -4; j <= 4; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];

                    int g1 = inp[x+i+1 +   (y+j) * w];
                    int g2 = inp[x+i   + (y+j+1) * w];
                    int g3 = inp[x+i-1 +   (y+j) * w];
                    int g4 = inp[x+i   + (y+j-1) * w];
                    int g5 = inp[x+i+2 + (y+j+1) * w];
                    int g6 = inp[x+i+1 + (y+j+2) * w];
                    //~ int g7 = inp[x+i   + (y+j+1) * w];
                    //~ int g8 = inp[x+i+1 +   (y+j) * w];
                    
                    int gr = (raw2ev[g1] + raw2ev[g2] + raw2ev[g3] + raw2ev[g4]) / 4;
                    int gb = (raw2ev[g1] + raw2ev[g2] + raw2ev[g5] + raw2ev[g6]) / 4;
                    med_r[k] = raw2ev[r] - gr;
                    med_b[k] = raw2ev[b] - gb;
                    k++;
                }
            }
            int dr = opt_med25(med_r);
            int db = opt_med25(med_b);

            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int g3 = inp[x-1 +   (y) * w];
            int g4 = inp[x   + (y-1) * w];
            int g5 = inp[x+2 + (y+1) * w];
            int g6 = inp[x+1 + (y+2) * w];
            int gr = (raw2ev[g1] + raw2ev[g2] + raw2ev[g3] + raw2ev[g4]) / 4;
            int gb = (raw2ev[g1] + raw2ev[g2] + raw2ev[g5] + raw2ev[g6]) / 4;

            out[x   +     y * w] = ev2raw[COERCE(gr + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
            out[x+1 + (y+1) * w] = ev2raw[COERCE(gb + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
        }
    }
}

static void chroma_smooth()
{
    int black = raw_info.black_level;
    static int raw2ev[16384];
    static int _ev2raw[24*EV_RESOLUTION];
    int* ev2raw = _ev2raw + 10*EV_RESOLUTION;
    
    int i;
    for (i = 0; i < 16384; i++)
    {
        raw2ev[i] = log2(MAX(1, i - black)) * EV_RESOLUTION;
    }

    for (i = -10*EV_RESOLUTION; i < 14*EV_RESOLUTION; i++)
    {
        ev2raw[i] = black + pow(2, (float)i / EV_RESOLUTION);
    }

    int w = raw_info.width;
    int h = raw_info.height;

    unsigned short * aux = malloc(w * h * sizeof(short));
    unsigned short * aux2 = malloc(w * h * sizeof(short));

    int x,y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            aux[x + y*w] = aux2[x + y*w] = raw_get_pixel(x, y);
    
    chroma_smooth_3x3(aux, aux2, raw2ev, ev2raw);
    
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            raw_set_pixel(x, y, aux2[x + y*w]);

    free(aux);
    free(aux2);
}
