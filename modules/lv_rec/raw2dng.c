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

lv_rec_file_footer_t lv_rec_footer;
struct raw_info raw_info;

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!ok) FAIL(fmt, ## __VA_ARGS__); }

static void fix_vertical_stripes();
static void hdr_process();

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
        hdr_process();
        set_framerate(lv_rec_footer.sourceFpsx1000);
        save_dng(fn);
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

/** HDR video interpolation */
/** Technical details: https://dl.dropboxusercontent.com/u/4124919/bleeding-edge/isoless/dual_iso.pdf */

/* choose interpolation method (define only one of these) */
#undef INTERP_MEAN_2RB_3G
#define INTERP_MEDIAN_6

/* quick check to see if this looks like a HDR frame */
static int hdr_check()
{
    int black = raw_info.black_level;
    int white = raw_info.white_level;

    int w = raw_info.width;
    int h = raw_info.height;

    static double raw2ev[16384];
    
    int i;
    for (i = 0; i < 16384; i++)
        raw2ev[i] = log2(MAX(1, i - black));

    int x, y;
    double avg_ev[4] = {0, 0, 0, 0};
    int num[4] = {0, 0, 0, 0};
    for (y = 2; y < h-2; y ++)
    {
        for (x = 2; x < w-2; x ++)
        {
            int p = raw_get_pixel(x, y);
            int p2 = raw_get_pixel(x, y+2);
            if (p < white && p2 < white)
            {
                avg_ev[y%4] += raw2ev[p];
                num[y%4]++;
            }
        }
    }

    double min_ev = 100;
    double max_ev = 0;
    for (i = 0; i < 4; i++)
    {
        avg_ev[i] /= num[i];
        min_ev = MIN(min_ev, avg_ev[i]);
        max_ev = MAX(max_ev, avg_ev[i]);
    }
    
    if (max_ev - min_ev > 0.5)
        return 1;
    
    return 0;
}

static int median_short(short* x, int n)
{
    short* aux = malloc(n * sizeof(x[0]));
    CHECK(aux, "malloc");
    memcpy(aux, x, n * sizeof(aux[0]));
    //~ qsort(aux, n, sizeof(aux[0]), compare_short);
    #define short_lt(a,b) ((*a)<(*b))
    QSORT(short, aux, n, short_lt);
    int ans = aux[n/2];
    free(aux);
    return ans;
}

static int estimate_iso(short* dark, short* bright, float* corr_ev, int* black_delta)
{
    /* guess ISO - find the factor and the offset for matching the bright and dark images */
    /* method: for each X (dark image) level, compute median of corresponding Y (bright image) values */
    /* then do a straight line fitting */

    int black = raw_info.black_level;
    int white = raw_info.white_level;

    int w = raw_info.width;
    int h = raw_info.height;
    
    int* order = malloc(w * h * sizeof(order[0]));
    CHECK(order, "malloc");
    
    int i, j, k;
    for (i = 0; i < w * h; i++)
        order[i] = i;

    /* sort the low ISO tones and process them as RLE */
    #define darkidx_lt(a,b) (dark[(*a)]<dark[(*b)])
    QSORT(int, order, w*h, darkidx_lt);
    
    int* medians_x = malloc(white * sizeof(medians_x[0]));
    int* medians_y = malloc(white * sizeof(medians_y[0]));
    int num_medians = 0;

    for (i = 0; i < w*h; )
    {
        int ref = dark[order[i]];
        for (j = i+1; j < w*h && dark[order[j]] == ref; j++);

        /* same dark value from i to j (without j) */
        int num = (j - i);
        
        if (num > 1000 && ref > black + 32)
        {
            short* aux = malloc(num * sizeof(aux[0]));
            for (k = 0; k < num; k++)
                aux[k] = bright[order[k+i]];
            int m = median_short(aux, num);
            if (m > black + 32 && m < white - 1000)
            {
                medians_x[num_medians] = ref - black;
                medians_y[num_medians] = m - black;
                num_medians++;
            }
            free(aux);
        }
        
        i = j;
    }

#if 0
    FILE* f = fopen("iso-curve.m", "w");

    fprintf(f, "x = [");
    for (i = 0; i < num_medians; i++)
        fprintf(f, "%d ", medians_x[i]);
    fprintf(f, "];\n");
    
    fprintf(f, "y = [");
    for (i = 0; i < num_medians; i++)
        fprintf(f, "%d ", medians_y[i]);
    fprintf(f, "];\n");

    fprintf(f, "plot(x, y);\n");
    fclose(f);
    
    system("octave --persist iso-curve.m");
#endif

    /**
     * plain least squares
     * y = ax + b
     * a = (mean(xy) - mean(x)mean(y)) / (mean(x^2) - mean(x)^2)
     * b = mean(y) - a mean(x)
     */
    
    double mx = 0, my = 0, mxy = 0, mx2 = 0;
    for (i = 0; i < num_medians; i++)
    {
        mx += medians_x[i];
        my += medians_y[i];
        mxy += medians_x[i] * medians_y[i];
        mx2 += medians_x[i] * medians_x[i];
    }
    mx /= num_medians;
    my /= num_medians;
    mxy /= num_medians;
    mx2 /= num_medians;
    double a = (mxy - mx*my) / (mx2 - mx*mx);
    double b = my - a * mx;

    free(medians_x);
    free(medians_y);
    free(order);

    double factor = a;
    if (factor < 1.2 || !isfinite(factor))
    {
        printf("Doesn't look like interlaced ISO\n");
        return 0;
    }
    
    *corr_ev = log2(factor);
    *black_delta = -(int)round(b / a);

    printf("ISO difference : %.2f EV (%d)\n", log2(factor), (int)round(factor*100));
    printf("Black delta    : %d\n", *black_delta);

    return 1;
}

#ifdef INTERP_MEDIAN_6
#define INTERP_METHOD_NAME "median6"
#define interp6 median6
/* median of 6 numbers */
static int median6(int a, int b, int c, int d, int e, int f, int white)
{
    int x[6] = {a,b,c,d,e,f};

    /* compute median */
    int aux;
    int i,j;
    for (i = 0; i < 5; i++)
        for (j = i+1; j < 6; j++)
            if (x[i] > x[j])
                aux = x[i], x[i] = x[j], x[j] = aux;
    if ((x[2] >= white) || (x[3] >= white))
        return white;
    int median = (x[2] + x[3]) / 2;
    return median;
}
#endif

#ifdef INTERP_MEAN_2RB_3G
#define INTERP_METHOD_NAME "mean2rb3g"
#define EV_MEAN2(a,b) ev2raw[(raw2ev[a & 16383] + raw2ev[b & 16383]) / 2]
#define EV_MEAN3(a,b,c) ev2raw[(raw2ev[a & 16383] + raw2ev[b & 16383] + raw2ev[c & 16383]) / 3]
#endif

#define EV_RESOLUTION 1000

static int hdr_interpolate()
{
    static int first_frame = 1;

    int black = raw_info.black_level;
    int white = raw_info.white_level;

    int w = raw_info.width;
    int h = raw_info.height;

    int x, y;

    /* for fast EV - raw conversion */
    static int raw2ev[16384];   /* EV x EV_RESOLUTION */
    static int ev2raw[14*EV_RESOLUTION];
    
    int i;
    for (i = 0; i < 16384; i++)
        raw2ev[i] = (int)round(log2(MAX(1, i - black)) * EV_RESOLUTION);
    for (i = 0; i < 14*EV_RESOLUTION; i++)
        ev2raw[i] = COERCE(black + pow(2, ((double)i/EV_RESOLUTION)), black, white),

    /* first we need to know which lines are dark and which are bright */
    /* the pattern is not always the same, so we need to autodetect it */

    /* it may look like this */                       /* or like this */
    /*
               ab cd ef gh  ab cd ef gh               ab cd ef gh  ab cd ef gh
                                       
            0  RG RG RG RG  RG RG RG RG            0  rg rg rg rg  rg rg rg rg
            1  gb gb gb gb  gb gb gb gb            1  gb gb gb gb  gb gb gb gb
            2  rg rg rg rg  rg rg rg rg            2  RG RG RG RG  RG RG RG RG
            3  GB GB GB GB  GB GB GB GB            3  GB GB GB GB  GB GB GB GB
            4  RG RG RG RG  RG RG RG RG            4  rg rg rg rg  rg rg rg rg
            5  gb gb gb gb  gb gb gb gb            5  gb gb gb gb  gb gb gb gb
            6  rg rg rg rg  rg rg rg rg            6  RG RG RG RG  RG RG RG RG
            7  GB GB GB GB  GB GB GB GB            7  GB GB GB GB  GB GB GB GB
            8  RG RG RG RG  RG RG RG RG            8  rg rg rg rg  rg rg rg rg
    */

    double acc_bright[4] = {0, 0, 0, 0};
    for (y = 2; y < h-2; y ++)
    {
        for (x = 2; x < w-2; x ++)
        {
            acc_bright[y % 4] += raw_get_pixel(x, y);
        }
    }
    double avg_bright = (acc_bright[0] + acc_bright[1] + acc_bright[2] + acc_bright[3]) / 4;
    int is_bright[4] = { acc_bright[0] > avg_bright, acc_bright[1] > avg_bright, acc_bright[2] > avg_bright, acc_bright[3] > avg_bright};

    if (first_frame)
        printf("\nISO pattern    : %c%c%c%c\n", is_bright[0] ? 'B' : 'd', is_bright[1] ? 'B' : 'd', is_bright[2] ? 'B' : 'd', is_bright[3] ? 'B' : 'd');
    else
        printf(" [HDR %c%c%c%c]", is_bright[0] ? 'B' : 'd', is_bright[1] ? 'B' : 'd', is_bright[2] ? 'B' : 'd', is_bright[3] ? 'B' : 'd');
    
    if (is_bright[0] + is_bright[1] + is_bright[2] + is_bright[3] != 2)
    {
        printf("Bright/dark detection error\n");
        return 0;
    }

    if (is_bright[0] == is_bright[2] || is_bright[1] == is_bright[3])
    {
        printf("Interlacing method not supported\n");
        return 0;
    }

    /* dark and bright exposures, interpolated */
    short* dark   = malloc(w * h * sizeof(short));
    CHECK(dark, "malloc");
    short* bright = malloc(w * h * sizeof(short));
    CHECK(bright, "malloc");

    memset(dark, 0, w * h * sizeof(short));
    memset(bright, 0, w * h * sizeof(short));

    #define BRIGHT_ROW (is_bright[y % 4])

    if (first_frame)
        printf("Interpolation  : %s\n", INTERP_METHOD_NAME);

#ifdef INTERP_MEAN_2RB_3G
    for (y = 2; y < h-2; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        int is_rg = (y % 2 == 0); /* RG or GB? */
        
        for (x = 2; x < w-2; x += 2)
        {
        
            /* red/blue: interpolate from (x,y+2) and (x,y-2) */
            /* green: interpolate from (x+1,y+1),(x-1,y+1),(x,y-2) or (x+1,y-1),(x-1,y-1),(x,y+2), whichever has the correct brightness */
            
            int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;
            
            if (is_rg)
            {
                int ra = raw_get_pixel(x, y-2);
                int rb = raw_get_pixel(x, y+2);
                int ri = (ra < white && rb < white) ? EV_MEAN2(ra, rb) : white;
                
                int ga = raw_get_pixel(x+1+1, y+s);
                int gb = raw_get_pixel(x+1-1, y+s);
                int gc = raw_get_pixel(x+1, y-2*s);
                int gi = (ga < white && gb < white && gc < white) ? EV_MEAN3(ga, gb, gc) : white;

                interp[x   + y * w] = ri;
                interp[x+1 + y * w] = gi;
            }
            else
            {
                int ba = raw_get_pixel(x+1  , y-2);
                int bb = raw_get_pixel(x+1  , y+2);
                int bi = (ba < white && bb < white) ? EV_MEAN2(ba, bb) : white;

                int ga = raw_get_pixel(x+1, y+s);
                int gb = raw_get_pixel(x-1, y+s);
                int gc = raw_get_pixel(x, y-2*s);
                int gi = (ga < white && gb < white && gc < white) ? EV_MEAN3(ga, gb, gc) : white;

                interp[x   + y * w] = gi;
                interp[x+1 + y * w] = bi;
            }

            native[x   + y * w] = raw_get_pixel(x, y);
            native[x+1 + y * w] = raw_get_pixel(x+1, y);
        }
    }
#else
    for (y = 2; y < h-2; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 2; x < w-2; x ++)
        {
            int ra = raw_get_pixel(x, y-2);
            int rb = raw_get_pixel(x, y+2);
            int ral = raw_get_pixel(x-2, y-2);
            int rbl = raw_get_pixel(x-2, y+2);
            int rar = raw_get_pixel(x+2, y-2);
            int rbr = raw_get_pixel(x+2, y+2);

            int ri = ev2raw[
                interp6(
                    raw2ev[ra & 16383], 
                    raw2ev[rb & 16383],
                    raw2ev[ral & 16383],
                    raw2ev[rbl & 16383],
                    raw2ev[rar & 16383],
                    raw2ev[rbr & 16383],
                    raw2ev[white]
                )
            ];
            
            interp[x   + y * w] = ri;
            native[x   + y * w] = raw_get_pixel(x, y);
        }
    }
#endif
    
    /* border interpolation */
    for (y = 0; y < 2; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel(x, y+2);
            native[x + y * w] = raw_get_pixel(x, y);
        }
    }

    for (y = h-2; y < h; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel(x, y-2);
            native[x + y * w] = raw_get_pixel(x, y);
        }
    }

    for (y = 2; y < h; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < 2; x ++)
        {
            interp[x + y * w] = raw_get_pixel(x, y-2);
            native[x + y * w] = raw_get_pixel(x, y);
        }

        for (x = w-2; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel(x, y-2);
            native[x + y * w] = raw_get_pixel(x, y);
        }
    }

    /* estimate ISO and black difference between bright and dark exposures */
    static float corr_ev = 0;
    static int black_delta = 0;
    
    /* only do the math for first frame, for speed reasons */
    if (first_frame)
    {
        int ok = estimate_iso(dark, bright, &corr_ev, &black_delta);
        if (!ok) goto err;
    }
    int corr = (int)roundf(corr_ev * EV_RESOLUTION);

#if 0 /* for debugging only */
    save_dng("normal.dng");
    for (y = 0; y < h; y ++)
        for (x = 0; x < w; x ++)
            raw_set_pixel(x, y, bright[x + y*w]);
    save_dng("bright.dng");
    for (y = 0; y < h; y ++)
        for (x = 0; x < w; x ++)
            raw_set_pixel(x, y, dark[x + y*w]);
    save_dng("dark.dng");
#endif

    /* mix the two images */
    /* highlights:  keep data from dark image only */
    /* shadows:     keep data from bright image only */
    /* midtones:    mix data from both, to bring back the resolution */
    
    /* estimate ISO overlap */
    /*
      ISO 100:       ###...........  (11 stops)
      ISO 1600:  ####..........      (10 stops)
      Combined:  XX##..............  (14 stops)
    */
    double clipped_ev = corr_ev;
    double lowiso_dr = 11;
    double overlap = lowiso_dr - clipped_ev;

    /* you get better colors, less noise, but a little more jagged edges if we underestimate the overlap amount */
    /* maybe expose a tuning factor? (preference towards resolution or colors) */
    overlap -= MIN(2, overlap - 2);
    
    if (first_frame)
        printf("ISO overlap    : %.1f EV (approx)\n", overlap);
    
    if (overlap < 0.5)
    {
        printf("Overlap error\n");
        goto err;
    }
    else if (overlap < 2)
    {
        printf("Overlap too small, use a smaller ISO difference for better results.\n");
    }

    /* mixing curves */
    double max_ev = log2(white - black);
    static int mix_curve[16384];
    static int fullres_curve[16384];
    for (i = 0; i < 16384; i++)
    {
        double ev = log2(MAX(i - black, 1));
        double c = -cos(MAX(MIN(ev-(max_ev-overlap),overlap),0)*M_PI/overlap);
        double k = (c+1)/2;
        double f = 1 - pow(c, 4);
        
        /* this looks very ugly at iso > 1600 */
        if (corr_ev > 4.5)
            f = 0;
        
        mix_curve[i] = FIXP_ONE * k;
        fullres_curve[i] = FIXP_ONE * f;
    }

#if 0
    FILE* f = fopen("mix-curve.m", "w");
    fprintf(f, "x = 0:16383; \n");

    fprintf(f, "ev = [");
    for (i = 0; i < 16384; i++)
        fprintf(f, "%f ", log2(MAX(i - black, 1)));
    fprintf(f, "];\n");
    
    fprintf(f, "k = [");
    for (i = 0; i < 16384; i++)
        fprintf(f, "%d ", mix_curve[i]);
    fprintf(f, "];\n");
    
    fprintf(f, "plot(ev, k);\n");
    fclose(f);
    
    system("octave --persist mix-curve.m");
#endif

    int hot_pixels = 0;

    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            /* bright and dark source pixels  */
            /* they may be real or interpolated */
            int b0 = bright[x + y*w];
            int d = dark[x + y*w] - black_delta;
            
            /* go from linear to EV space */
            int bev = raw2ev[b0 & 16383];
            int dev = raw2ev[d & 16383];
            
            /* darken bright pixel so it looks like its darker sibling  */
            bev -= corr;
            
            /* blending factors */
            int k = mix_curve[b0 & 16383];
            int f = fullres_curve[b0 & 16383];

            /* beware of hot pixels */
            int is_hot = (k > 0 && b0 < white && dev > bev + 2 * EV_RESOLUTION);
            if (is_hot)
            {
                hot_pixels++;
                k = f = 0;
            }

            /* mix bright and dark exposures */
            double mixed = (bev*(FIXP_ONE-k) + dev*k) / FIXP_ONE;
            
            /* recover the full resolution where possible */
            double fullres = BRIGHT_ROW ? bev : dev;
            
            /* blend "mixed" and "full-res" images smoothly to avoid banding*/
            int output = (mixed*(FIXP_ONE-f) + fullres*f) / FIXP_ONE;
            
            /* safeguard */
            output = COERCE(output, 0, 14*EV_RESOLUTION-1);
            
            /* back to linear space and commit */
            raw_set_pixel(x, y, ev2raw[output] + black_delta/8);
            
            /* fixme: why black_delta/8? it looks good for the Batman shot, but why? */
        }
    }

    if (hot_pixels)
    {
        if (first_frame)
            printf("Hot pixels     : %d", hot_pixels);
        else
            printf(" [%d hot pixels]  ", hot_pixels);
    }

    first_frame = 0;
    free(dark);
    free(bright);
    return 1;

err:
    free(dark);
    free(bright);
    return 0;
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


static void hdr_process()
{
    static int first_time = 1;
    static int hdr_needed = 0;
    if (first_time)
    {
        hdr_needed = hdr_check();
        first_time = 0;
    }
    
    if (hdr_needed)
    {
        hdr_interpolate();
    }
}
