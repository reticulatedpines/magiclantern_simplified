/**
 * Post-process CR2 images obtained with the Dual ISO module
 * (deinterlace, blend the two exposures, output a 16-bit DNG with much cleaner shadows)
 * 
 * Technical details: https://dl.dropboxusercontent.com/u/4124919/bleeding-edge/isoless/dual_iso.pdf
 */
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

/* choose interpolation method (define only one of these) */
#define INTERP_MEAN23
#undef INTERP_MEAN_6
#undef INTERP_MEAN_4_OUT_OF_6
#undef INTERP_MEAN_5_OUT_OF_6
#undef INTERP_MEDIAN_6
#undef INTERP_MEAN23_EDGE

/* post interpolation enhancements */
#define CHROMA_SMOOTH
#define ALIAS_BLEND

/* minimizes aliasing while ignoring the other factors (e.g. shadow noise, banding) */
/* useful for debugging */
//~ #define FULLRES_ONLY


#define EV_RESOLUTION 32768

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "../../src/raw.h"
#include "qsort.h"  /* much faster than standard C qsort */

#include "optmed.h"

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!ok) FAIL(fmt, ## __VA_ARGS__); }

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))
#define COUNT(x)        ((int)(sizeof(x)/sizeof((x)[0])))

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

#define CAM_COLORMATRIX1                       \
 6722, 10000,     -635, 10000,    -963, 10000, \
-4287, 10000,    12460, 10000,    2028, 10000, \
 -908, 10000,     2162, 10000,    5668, 10000

struct raw_info raw_info = {
    .api_version = 1,
    .bits_per_pixel = 16,
    .black_level = 2048,
    .white_level = 15000,
    .cfa_pattern = 0x02010100,          // Red  Green  Green  Blue
    .calibration_illuminant1 = 1,       // Daylight
    .color_matrix1 = {CAM_COLORMATRIX1},
};

static int hdr_check();
static int hdr_interpolate();
static int black_subtract(int left_margin, int top_margin);
static int black_subtract_simple(int left_margin, int top_margin);
static int white_detect();

static inline int raw_get_pixel16(int x, int y) {
    unsigned short * buf = raw_info.buffer;
    int value = buf[x + y * raw_info.width];
    return value;
}

static inline int raw_set_pixel16(int x, int y, int value)
{
    unsigned short * buf = raw_info.buffer;
    buf[x + y * raw_info.width] = value;
    return value;
}

int raw_get_pixel(int x, int y) {
    return raw_get_pixel16(x,y);
}

/* from 14 bit to 16 bit */
int raw_get_pixel_14to16(int x, int y) {
    return (raw_get_pixel16(x,y) << 2) & 0xFFFF;
}

static int startswith(char* str, char* prefix)
{
    char* s = str;
    char* p = prefix;
    for (; *p; s++,p++)
        if (*s != *p) return 0;
    return 1;
}

static void reverse_bytes_order(char* buf, int count)
{
    unsigned short* buf16 = (unsigned short*) buf;
    int i;
    for (i = 0; i < count/2; i++)
    {
        unsigned short x = buf16[i];
        buf[2*i+1] = x;
        buf[2*i] = x >> 8;
    }
}

int main(int argc, char** argv)
{
    int k;
    int r;
    for (k = 1; k < argc; k++)
    {
        char* filename = argv[k];

        printf("\nInput file     : %s\n", filename);

        char dcraw_cmd[100];
        snprintf(dcraw_cmd, sizeof(dcraw_cmd), "dcraw -v -i -t 0 \"%s\" > tmp.txt", filename);
        int exit_code = system(dcraw_cmd);
        CHECK(exit_code == 0, "%s", filename);
        
        FILE* t = fopen("tmp.txt", "rb");
        CHECK(t, "tmp.txt");
        int raw_width = 0, raw_height = 0;
        int out_width = 0, out_height = 0;
        
        char line[100];
        while (fgets(line, sizeof(line), t))
        {
            if (startswith(line, "Full size: "))
            {
                r = sscanf(line, "Full size: %d x %d\n", &raw_width, &raw_height);
                CHECK(r == 2, "sscanf");
            }
            else if (startswith(line, "Output size: "))
            {
                r = sscanf(line, "Output size: %d x %d\n", &out_width, &out_height);
                CHECK(r == 2, "sscanf");
            }
        }
        fclose(t);

        printf("Full size      : %d x %d\n", raw_width, raw_height);
        printf("Active area    : %d x %d\n", out_width, out_height);
        
        int left_margin = raw_width - out_width;
        int top_margin = raw_height - out_height;

        snprintf(dcraw_cmd, sizeof(dcraw_cmd), "dcraw -4 -E -c -t 0 \"%s\" > tmp.pgm", filename);
        exit_code = system(dcraw_cmd);
        CHECK(exit_code == 0, "%s", filename);
        
        FILE* f = fopen("tmp.pgm", "rb");
        CHECK(f, "tmp.pgm");
        
        char magic0, magic1;
        r = fscanf(f, "%c%c\n", &magic0, &magic1);
        CHECK(r == 2, "fscanf");
        CHECK(magic0 == 'P' && magic1 == '5', "pgm magic");
        
        int width, height;
        r = fscanf(f, "%d %d\n", &width, &height);
        CHECK(r == 2, "fscanf");
        CHECK(width == raw_width, "pgm width");
        CHECK(height == raw_height, "pgm height");
        
        int max_value;
        r = fscanf(f, "%d\n", &max_value);
        CHECK(r == 1, "fscanf");
        CHECK(max_value == 65535, "pgm max");

        void* buf = malloc(width * (height+1) * 2); /* 1 extra line for handling GBRG easier */
        fseek(f, -width * height * 2, SEEK_END);
        int size = fread(buf, 1, width * height * 2, f);
        CHECK(size == width * height * 2, "fread");
        fclose(f);

        /* PGM is big endian, need to reverse it */
        reverse_bytes_order(buf, width * height * 2);

        raw_info.buffer = buf;
        
        /* did we read the PGM correctly? (right byte order etc) */
        //~ int i;
        //~ for (i = 0; i < 10; i++)
            //~ printf("%d ", raw_get_pixel16(i, 0));
        //~ printf("\n");
        
        raw_info.black_level = 2048;
        raw_info.white_level = 15000;

        raw_info.width = width;
        raw_info.height = height;
        raw_info.pitch = width * 2;
        raw_info.frame_size = raw_info.height * raw_info.pitch;

        raw_info.active_area.x1 = left_margin;
        raw_info.active_area.x2 = raw_info.width;
        raw_info.active_area.y1 = top_margin;
        raw_info.active_area.y2 = raw_info.height;
        raw_info.jpeg.x = 0;
        raw_info.jpeg.y = 0;
        raw_info.jpeg.width = raw_info.width - left_margin;
        raw_info.jpeg.height = raw_info.height - top_margin;

        if (hdr_check())
        {
            white_detect();
            if (!black_subtract(left_margin, top_margin))
                printf("Black subtract didn't work\n");

            /* will use 16 bit processing and output, instead of 14 */
            raw_info.black_level *= 4;
            raw_info.white_level *= 4;

            if (hdr_interpolate())
            {
                char out_filename[1000];
                snprintf(out_filename, sizeof(out_filename), "%s", filename);
                int len = strlen(out_filename);
                out_filename[len-3] = 'D';
                out_filename[len-2] = 'N';
                out_filename[len-1] = 'G';

                /* run a second black subtract pass, to fix whatever our funky processing may do to blacks */
                black_subtract_simple(left_margin, top_margin);

                reverse_bytes_order(raw_info.buffer, raw_info.frame_size);

                printf("Output file    : %s\n", out_filename);
                save_dng(out_filename);

                char exif_cmd[100];
                snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -tagsFromFile \"%s\" -all:all \"%s\" -overwrite_original", filename, out_filename);
                int r = system(exif_cmd);
                if (r != 0)
                    printf("Exiftool didn't work\n");
            }
            else
            {
                printf("ISO blending didn't work\n");
            }

            raw_info.black_level /= 4;
            raw_info.white_level /= 4;
        }
        else
        {
            printf("Doesn't look like interlaced ISO\n");
        }
        
        unlink("tmp.pgm");
        unlink("tmp.txt");
        
        free(buf);
    }
    
    return 0;
}

static int white_detect_brute_force()
{
    /* sometimes the white level is much lower than 15000; this would cause pink highlights */
    /* workaround: consider the white level as a little under the maximum pixel value from the raw file */
    /* caveat: bright and dark exposure may have different white levels, so we'll take the minimum value */
    /* side effect: if the image is not overexposed, it may get brightened a little; shouldn't hurt */
    
    /* ignore hot pixels when finding white level (at least 10 pixels should confirm it) */
    
    int white = raw_info.white_level * 5 / 6;
    int whites[8] = {white, white, white, white, white, white, white, white};
    int maxies[8] = {white, white, white, white, white, white, white, white};
    int counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int x,y;
    for (y = raw_info.active_area.y1; y < raw_info.active_area.y2; y ++)
    {
        for (x = raw_info.active_area.x1; x < raw_info.active_area.x2; x ++)
        {
            int pix = raw_get_pixel16(x, y);
            #define BIN_IDX ((y%4) + (x%2)*4)
            if (pix > maxies[BIN_IDX])
            {
                maxies[BIN_IDX] = pix;
                counts[BIN_IDX] = 1;
            }
            else if (pix == maxies[BIN_IDX])
            {
                counts[BIN_IDX]++;
                if (counts[BIN_IDX] > 10)
                {
                    whites[BIN_IDX] = maxies[BIN_IDX];
                }
            }
            #undef BIN_IDX
        }
    }

    //~ printf("%8d %8d %8d %8d %8d %8d %8d %8d\n", whites[0], whites[1], whites[2], whites[3], whites[4], whites[5], whites[6], whites[7]);
    //~ printf("%8d %8d %8d %8d %8d %8d %8d %8d\n", counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7]);
    
    int white1 = MIN(MIN(whites[0], whites[1]), MIN(whites[2], whites[3]));
    int white2 = MIN(MIN(whites[4], whites[5]), MIN(whites[6], whites[7]));
    white = MIN(white1, white2);
    raw_info.white_level = white - 100;
    printf("White level    : %d\n", raw_info.white_level);
    return 1;
}

static int white_detect()
{
    return white_detect_brute_force();

#if 0
    int w = raw_info.width;
    int p0 = raw_get_pixel16(0, 0);
    if (p0 < 10000) return 0;
    int x;
    for (x = 0; x < w; x++)
        if (raw_get_pixel16(x, 0) != p0)
            return 0;

    /* first line is white level, cool! */
    raw_info.white_level = p0 - 1000;       /* pink pixels at aggressive values */
    printf("White level    : %d\n", raw_info.white_level);
    return 1;
#endif
}


static int black_subtract(int left_margin, int top_margin)
{
    if (left_margin < 10) return 0;
    if (top_margin < 10) return 0;

#if 0
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("untouched.dng");
#endif

    printf("Black borders  : %d left, %d top\n", left_margin, top_margin);

    int w = raw_info.width;
    int h = raw_info.height;
    
    int* vblack = malloc(h * sizeof(int));
    int* hblack = malloc(w * sizeof(int));
    int* aux = malloc(MAX(w,h) * sizeof(int));
    unsigned short * blackframe = malloc(w * h * sizeof(unsigned short));
    
    CHECK(vblack, "malloc");
    CHECK(hblack, "malloc");
    CHECK(blackframe, "malloc");

    /* data above this may be gibberish */
    int ymin = (top_margin*3/4) & ~3;

    /* estimate vertical correction for each line */
    int x,y;
    for (y = 0; y < h; y++)
    {
        int avg = 0;
        int num = 0;
        for (x = 2; x < left_margin - 8; x++)
        {
            avg += raw_get_pixel16(x, y);
            num++;
        }
        vblack[y] = avg / num;
    }
    
    /* perform some slight filtering (averaging) so we don't add noise to the image */
    for (y = 0; y < h; y++)
    {
        int y2;
        int avg = 0;
        int num = 0;
        for (y2 = y - 10*4; y2 < y + 10*4; y2 += 4)
        {
            if (y2 < ymin) continue;
            if (y2 >= h) continue;
            avg += vblack[y2];
            num++;
        }
        if (num > 0)
        {
            avg /= num;
            aux[y] = avg;
        }
        else
        {
            aux[y] = vblack[y];
        }
    }
    
    memcpy(vblack, aux, h * sizeof(vblack[0]));

    /* update the dark frame */
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            blackframe[x + y*w] = vblack[y];
    
    /* estimate horizontal drift for each channel */
    int k;
    for (k = 0; k < 4; k++)
    {
        int y0 = ymin + k;
        int offset = 0;
        {
            int num = 0;
            for (y = y0; y < top_margin-4; y += 4)
            {
                offset += blackframe[y*w];
                num++;
            }
            offset /= num;
        }
        
        /* try to fix banding that repeats every 8 pixels */
        int xg;
        for (xg = 0; xg < 8; xg++)
        {
            for (x = xg; x < w; x += 8)
            {
                int num = 0;
                int avg = 0;
                for (y = y0; y < top_margin-4; y += 4)
                {
                    avg += raw_get_pixel16(x, y) - offset;
                    num++;
                }
                hblack[x] = avg / num;
            }
            
            /* perform some stronger filtering (averaging), since this data is a lot noisier */
            /* if we don't do that, we will add some strong FPN to the image */
            for (x = xg; x < w; x += 8)
            {
                int x2;
                int avg = 0;
                int num = 0;
                for (x2 = x - 1024; x2 < x + 1024; x2 += 8)
                {
                    if (x2 < 0) continue;
                    if (x2 >= w) continue;
                    avg += hblack[x2];
                    num++;
                }
                avg /= num;
                aux[x] = avg;
            }
            memcpy(hblack, aux, w * sizeof(hblack[0]));

            /* update the dark frame */
            for (y = y0; y < h; y += 4)
                for (x = xg; x < w; x += 8)
                    blackframe[x + y*w] += hblack[x];
        }
    }
    

#if 0 /* for debugging only */
    void* old_buffer = raw_info.buffer;
    raw_info.buffer = blackframe;
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("black.dng");
    raw_info.buffer = old_buffer;
#endif

    /* subtract the dark frame, keeping the average black level */
    double avg_black = 0;
    for (y = top_margin; y < h; y++)
    {
        for (x = left_margin; x < w; x++)
        {
            avg_black += blackframe[x + y*w];
        }
    }
    avg_black /= (w * h);

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            int p = raw_get_pixel16(x, y);
            int black_delta = avg_black - blackframe[x + y*w];
            p += black_delta;
            p = COERCE(p, 0, 16383);
            raw_set_pixel16(x, y, p);
        }
    }

    raw_info.black_level = round(avg_black);
    printf("Black level    : %d\n", raw_info.black_level);

#if 0
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("subtracted.dng");
#endif

    free(vblack);
    free(hblack);
    free(blackframe);
    free(aux);
    return 1;
}


static int black_subtract_simple(int left_margin, int top_margin)
{
    if (left_margin < 10) return 0;
    if (top_margin < 10) return 0;
    
    int w = raw_info.width;
    int h = raw_info.height;

    /* average left bar */
    int x,y;
    long long avg = 0;
    int num = 0;
    for (y = 0; y < h; y++)
    {
        for (x = 2; x < left_margin - 8; x++)
        {
            int p = raw_get_pixel16(x, y);
            if (p > 0)
            {
                avg += p;
                num++;
            }
        }
    }
    
    int new_black = avg / num;
        
    int black_delta = raw_info.black_level - new_black;
    
    printf("Black adjust   : %d\n", (int)black_delta);

    /* "subtract" the dark frame, keeping the exif black level and preserving the white level */
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            double p = raw_get_pixel16(x, y);
            if (p > 0)
            {
                p += black_delta;
                p = p * raw_info.white_level / (raw_info.white_level + black_delta);
                p = COERCE(p, 0, 65535);
                raw_set_pixel16(x, y, p);
            }
        }
    }
    
    return 1;
}

static void compute_black_noise(int x1, int x2, int y1, int y2, int dx, int dy, double* out_mean, double* out_stdev)
{
    double black = 0;
    int num = 0;
    /* compute average level */
    int x, y;
    for (y = y1; y < y2; y += dy)
    {
        for (x = x1; x < x2; x += dx)
        {
            black += raw_get_pixel(x, y);
            num++;
        }
    }

    double mean = black / num;

    /* compute standard deviation */
    double stdev = 0;
    for (y = y1; y < y2; y += dy)
    {
        for (x = x1; x < x2; x += dx)
        {
            int dif = raw_get_pixel(x, y) - mean;
            stdev += dif * dif;
        }
    }
    stdev /= num;
    stdev = sqrt(stdev);

    *out_mean = mean;
    *out_stdev = stdev;
}

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
    double avg_ev = 0;
    int num = 0;
    for (y = 2; y < h-2; y ++)
    {
        for (x = 2; x < w-2; x ++)
        {
            int p = raw_get_pixel16(x, y);
            int p2 = raw_get_pixel16(x, y+2);
            if (p > black+32 && p2 > black+32 && p < white && p2 < white)
            {
                avg_ev += ABS(raw2ev[p2] - raw2ev[p]);
                num++;
            }
        }
    }
    
    avg_ev /= num;

    if (avg_ev > 0.5)
        return 1;
    
    return 0;
}

static int median_short(unsigned short* x, int n)
{
    unsigned short* aux = malloc(n * sizeof(x[0]));
    CHECK(aux, "malloc");
    memcpy(aux, x, n * sizeof(aux[0]));
    //~ qsort(aux, n, sizeof(aux[0]), compare_short);
    #define short_lt(a,b) ((*a)<(*b))
    QSORT(unsigned short, aux, n, short_lt);
    int ans = aux[n/2];
    free(aux);
    return ans;
}

static int median_int(int* x, int n)
{
    int* aux = malloc(n * sizeof(x[0]));
    CHECK(aux, "malloc");
    memcpy(aux, x, n * sizeof(aux[0]));
    #define int_lt(a,b) ((*a)<(*b))
    QSORT(int, aux, n, int_lt);
    int ans = aux[n/2];
    free(aux);
    return ans;
}

static int estimate_iso(unsigned short* dark, unsigned short* bright, double* corr_ev)
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
    
    /* sort the high ISO tones and process them as RLE */
    #define darkidx_lt(a,b) (bright[(*a)]<bright[(*b)])
    QSORT(int, order, w*h, darkidx_lt);
    
    int* medians_x = malloc(16384 * sizeof(medians_x[0]));
    int* medians_y = malloc(16384 * sizeof(medians_y[0]));
    int num_medians = 0;

    int* all_medians = malloc(16384 * sizeof(all_medians[0]));
    memset(all_medians, 0, 16384 * sizeof(all_medians[0]));

    for (i = 0; i < w*h; )
    {
        int ref = bright[order[i]];
        for (j = i+1; j < w*h && bright[order[j]] == ref; j++);

        /* same dark value from i to j (without j) */
        int num = (j - i);
        
        unsigned short* aux = malloc(num * sizeof(aux[0]));
        for (k = 0; k < num; k++)
            aux[k] = dark[order[k+i]];
        
        int m = median_short(aux, num);
        all_medians[ref] = m;
        
        if (num > 100 && ref > black && ref < white - 1000 && m > black && m < white - 1000)
        {
            medians_x[num_medians] = ref - black;
            medians_y[num_medians] = m - black;
            num_medians++;
        }
        
        free(aux);
        
        i = j;
    }

    /* estimate ISO (median angle) */
    int* medians_ang = malloc(num_medians * sizeof(medians_ang[0]));
    for (i = 0; i < num_medians; i++)
    {
        double ang = atan2(medians_y[i], medians_x[i]);
        while (ang < 0) ang += M_PI;
        medians_ang[i] = (int)round(ang * 1000000);
    }
    int ma = median_int(medians_ang, num_medians);

    /* y = ax */
    double a = tan(ma / 1000000.0);

    /* adjust ISO 100 nonlinearly so it matches the y = ax */
    int x, y;
    for (y = 0; y < h-1; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int ref = bright[x + y*w];
            ref = MIN(ref, white);
            int med = all_medians[ref];
            if (med == 0)
                continue;
            int ideal = (ref - black) * a + black;
            int corr = ideal - med;
            dark[x + y*w] += corr;
        }
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

    fprintf(f, "plot(x, y); hold on;\n");
    fprintf(f, "a = %f;\n", a);
    fprintf(f, "b = %f;\n", 0.0);
    fprintf(f, "plot(x, a * x + b, 'r');\n");
    fclose(f);
    
    system("octave --persist iso-curve.m");
#endif

    free(medians_ang);
    free(medians_x);
    free(medians_y);
    free(order);
    free(all_medians);

    double factor = 1/a;
    if (factor < 1.2 || !isfinite(factor))
    {
        printf("Doesn't look like interlaced ISO\n");
        return 0;
    }
    
    *corr_ev = log2(factor);

    printf("ISO difference : %.2f EV (%d)\n", log2(factor), (int)round(factor*100));

    return 1;
}

#ifdef INTERP_MEAN_5_OUT_OF_6
#define interp6 mean5outof6
#define INTERP_METHOD_NAME "mean5/6"

/* mean of 5 numbers out of 6 (with one outlier removed) */
static int mean5outof6(int a, int b, int c, int d, int e, int f, int white)
{
    int x[6] = {a,b,c,d,e,f};

    /* compute median */
    int aux;
    int i,j;
    for (i = 0; i < 5; i++)
        for (j = i+1; j < 6; j++)
            if (x[i] > x[j])
                aux = x[i], x[i] = x[j], x[j] = aux;
    int median = (x[2] + x[3]) / 2;
    
    /* remove 1 outlier */
    int l = 0;
    int r = 5;
    if (median - x[l] > x[r] - median) l++;
    else r--;
    
    /* mean of remaining numbers */
    int sum = 0;
    for (i = l; i <= r; i++)
    {
        if (x[i] >= white) return white;
        sum += x[i];
    }
    return sum / (r - l + 1);
}
#endif

#ifdef INTERP_MEAN_4_OUT_OF_6
#define INTERP_METHOD_NAME "mean4/6"
#define interp6 mean4outof6
/* mean of 4 numbers out of 6 (with two outliers removed) */
static int mean4outof6(int a, int b, int c, int d, int e, int f, int white)
{
    int x[6] = {a,b,c,d,e,f};

    /* compute median */
    int aux;
    int i,j;
    for (i = 0; i < 5; i++)
        for (j = i+1; j < 6; j++)
            if (x[i] > x[j])
                aux = x[i], x[i] = x[j], x[j] = aux;
    int median = (x[2] + x[3]) / 2;
    
    /* remove 2 outliers */
    int l = 0;
    int r = 5;
    if (median - x[l] > x[r] - median) l++;
    else r--;
    if (median - x[l] > x[r] - median) l++;
    else r--;
    
    /* mean of remaining numbers */
    int sum = 0;
    for (i = l; i <= r; i++)
    {
        if (x[i] >= white) return white;
        sum += x[i];
    }
    return sum / (r - l + 1);
}
#endif

#ifdef INTERP_MEAN_6
#define INTERP_METHOD_NAME "mean6"
#define interp6 mean6
static int mean6(int a, int b, int c, int d, int e, int f, int white)
{
    return (a + b + c + d + e + f) / 6;
}
#endif

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

#ifdef INTERP_MEAN23
#define INTERP_METHOD_NAME "mean23"
#endif

#ifdef INTERP_MEAN23_EDGE
#define INTERP_METHOD_NAME "mean23-edge"

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

#if defined(INTERP_MEAN23) || defined(INTERP_MEAN23_EDGE)
static int mean2(int a, int b, int white, int* err)
{
    if (a >= white || b >= white)
        return white;
    
    int m = (a + b) / 2;

    if (err)
        *err = MAX(ABS(a - m), ABS(b - m));

    return m;
}

static int mean3(int a, int b, int c, int white, int* err)
{
    if (a >= white || b >= white || c >= white)
        return white;

    int m = (a + b + c) / 3;

    if (err)
        *err = MAX(MAX(ABS(a - m), ABS(b - m)), ABS(c - m));

    return m;
}
#endif

#ifdef CHROMA_SMOOTH
#if 0
static void chroma_smooth_3x3(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x, y;

    for (y = 2; y < h-2; y += 2)
    {
        for (x = 2; x < w-2; x += 2)
        {
            int i,j;
            int k = 0;
            int med_r[9];
            int med_b[9];
            for (i = -2; i <= 2; i += 2)
            {
                for (j = -2; j <= 2; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int g1 = inp[x+i+1 +   (y+j) * w];
                    int g2 = inp[x+i   + (y+j+1) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                    
                    int ge = (raw2ev[g1] + raw2ev[g2]) / 2;
                    med_r[k] = raw2ev[r] - ge;
                    med_b[k] = raw2ev[b] - ge;
                    k++;
                }
            }
            int dr = opt_med9(med_r);
            int db = opt_med9(med_b);

            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int ge = (raw2ev[g1] + raw2ev[g2]) / 2;

            out[x   +     y * w] = ev2raw[COERCE(ge + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
            out[x+1 + (y+1) * w] = ev2raw[COERCE(ge + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
        }
    }
}
#endif

static void chroma_smooth_5x5(unsigned short * inp, unsigned short * out, int* raw2ev, int* ev2raw)
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
            int med_r[25];
            int med_b[25];
            for (i = -4; i <= 4; i += 2)
            {
                for (j = -4; j <= 4; j += 2)
                {
                    int r  = inp[x+i   +   (y+j) * w];
                    int g1 = inp[x+i+1 +   (y+j) * w];
                    int g2 = inp[x+i   + (y+j+1) * w];
                    int b  = inp[x+i+1 + (y+j+1) * w];
                    
                    int ge = (raw2ev[g1] + raw2ev[g2]) / 2;
                    med_r[k] = raw2ev[r] - ge;
                    med_b[k] = raw2ev[b] - ge;
                    k++;
                }
            }
            int dr = opt_med25(med_r);
            int db = opt_med25(med_b);

            int g1 = inp[x+1 +     y * w];
            int g2 = inp[x   + (y+1) * w];
            int ge = (raw2ev[g1] + raw2ev[g2]) / 2;

            out[x   +     y * w] = ev2raw[COERCE(ge + dr, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
            out[x+1 + (y+1) * w] = ev2raw[COERCE(ge + db, -10*EV_RESOLUTION, 14*EV_RESOLUTION)];
        }
    }
}
#endif

static int hdr_interpolate()
{
    int ret = 1;
    
    int black = raw_info.black_level;
    int white = raw_info.white_level;

    int w = raw_info.width;
    int h = raw_info.height;

    int x, y;

    /* for fast EV - raw conversion */
    static int raw2ev[65536];   /* EV x EV_RESOLUTION */
    static int ev2raw_0[24*EV_RESOLUTION];
    
    /* handle sub-black values (negative EV) */
    int* ev2raw = ev2raw_0 + 10*EV_RESOLUTION;
    
    /* RGGB or GBRG? */
    double rggb_err = 0;
    double gbrg_err = 0;
    for (y = 2; y < h-2; y += 2)
    {
        for (x = 2; x < w-2; x += 2)
        {
            int tl = raw_get_pixel16(x, y);
            int tr = raw_get_pixel16(x+1, y);
            int bl = raw_get_pixel16(x, y+1);
            int br = raw_get_pixel16(x+1, y+1);
            int pl = raw_get_pixel16(x, y-1);
            int pr = raw_get_pixel16(x+1, y-1);
            rggb_err += MIN(ABS(tr-bl), ABS(tr-pl));
            gbrg_err += MIN(ABS(tl-br), ABS(tl-pr));
        }
    }
    
    /* which one looks more likely? */
    int rggb = (rggb_err < gbrg_err);
    
    if (!rggb) /* this code assumes RGGB, so we need to skip one line */
    {
        raw_info.buffer += raw_info.pitch;
        raw_info.active_area.y1++;
        raw_info.active_area.y2--;
        raw_info.jpeg.y++;
        raw_info.jpeg.height -= 3;
        raw_info.height--;
        h--;
    }

    int i;
    for (i = 0; i < 65536; i++)
    {
        double signal = MAX(i/4.0 - black/4.0, -1023);
        if (signal > 0)
            raw2ev[i] = (int)round(log2(1+signal) * EV_RESOLUTION);
        else
            raw2ev[i] = -(int)round(log2(1-signal) * EV_RESOLUTION);
    }

    for (i = -10*EV_RESOLUTION; i < 0; i++)
    {
        ev2raw[i] = COERCE(black+4 - round(4*pow(2, ((double)-i/EV_RESOLUTION))), 0, black);
    }

    for (i = 0; i < 14*EV_RESOLUTION; i++)
    {
        ev2raw[i] = COERCE(black-4 + round(4*pow(2, ((double)i/EV_RESOLUTION))), black, white);
        
        if (i >= raw2ev[white])
        {
            ev2raw[i] = white;
        }
    }
    
    /* keep "bad" pixels, if any */
    ev2raw[raw2ev[0]] = 0;
    ev2raw[raw2ev[0]] = 0;
    
    /* check raw <--> ev conversion */
    //~ printf("%d %d %d %d %d %d %d *%d* %d %d %d %d %d\n", raw2ev[0], raw2ev[1000], raw2ev[2000], raw2ev[8188], raw2ev[8189], raw2ev[8190], raw2ev[8191], raw2ev[8192], raw2ev[8193], raw2ev[8194], raw2ev[8195], raw2ev[8196], raw2ev[8200]);
    //~ printf("%d %d %d %d %d %d %d *%d* %d %d %d %d %d\n", ev2raw[raw2ev[0]], ev2raw[raw2ev[1000]], ev2raw[raw2ev[2000]], ev2raw[raw2ev[8188]], ev2raw[raw2ev[8189]], ev2raw[raw2ev[8190]], ev2raw[raw2ev[8191]], ev2raw[raw2ev[8192]], ev2raw[raw2ev[8193]], ev2raw[raw2ev[8194]], ev2raw[raw2ev[8195]], ev2raw[raw2ev[8196]], ev2raw[raw2ev[8200]]);

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
            acc_bright[y % 4] += raw_get_pixel16(x, y);
        }
    }
    double avg_bright = (acc_bright[0] + acc_bright[1] + acc_bright[2] + acc_bright[3]) / 4;
    int is_bright[4] = { acc_bright[0] > avg_bright, acc_bright[1] > avg_bright, acc_bright[2] > avg_bright, acc_bright[3] > avg_bright};

    printf("ISO pattern    : %c%c%c%c %s\n", is_bright[0] ? 'B' : 'd', is_bright[1] ? 'B' : 'd', is_bright[2] ? 'B' : 'd', is_bright[3] ? 'B' : 'd', rggb ? "RGGB" : "GBRG");
    
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

    #define BRIGHT_ROW (is_bright[y % 4])
    
    double noise_std[4];
    double noise_avg;
    for (y = 0; y < 4; y++)
        compute_black_noise(8, raw_info.active_area.x1 - 8, 20 + y, raw_info.active_area.y2 - 20, 1, 4, &noise_avg, &noise_std[y]);

    printf("Noise levels   : %.02f %.02f %.02f %.02f (14-bit)\n", noise_std[0], noise_std[1], noise_std[2], noise_std[3]);
    double dark_noise = MIN(MIN(noise_std[0], noise_std[1]), MIN(noise_std[2], noise_std[3]));
    double bright_noise = MAX(MAX(noise_std[0], noise_std[1]), MAX(noise_std[2], noise_std[3]));
    double dark_noise_ev = log2(dark_noise);
    double bright_noise_ev = log2(bright_noise);

    /* dark and bright exposures, interpolated */
    unsigned short* dark   = malloc(w * h * sizeof(unsigned short));
    CHECK(dark, "malloc");
    unsigned short* bright = malloc(w * h * sizeof(unsigned short));
    CHECK(bright, "malloc");
    memset(dark, 0, w * h * sizeof(unsigned short));
    memset(bright, 0, w * h * sizeof(unsigned short));
    
    /* fullres image (minimizes aliasing) */
    unsigned short* fullres = malloc(w * h * sizeof(unsigned short));
    CHECK(fullres, "malloc");
    memset(fullres, 0, w * h * sizeof(unsigned short));
    unsigned short* fullres_smooth = 0;

    /* halfres image (minimizes noise and banding) */
    unsigned short* halfres = malloc(w * h * sizeof(unsigned short));
    CHECK(halfres, "malloc");
    memset(halfres, 0, w * h * sizeof(unsigned short));
    unsigned short* halfres_smooth = 0;

    #ifdef ALIAS_BLEND
    unsigned short* alias_map = malloc(w * h * sizeof(unsigned short));
    CHECK(alias_map, "malloc");
    unsigned short* overexposed = malloc(w * h * sizeof(unsigned short));
    CHECK(overexposed, "malloc");
    memset(alias_map, 0, w * h * sizeof(unsigned short));
    memset(overexposed, 0, w * h * sizeof(unsigned short));
    #endif

    printf("Estimating ISO difference...\n");
    /* use a simple interpolation in 14-bit space (the 16-bit one will trick the algorithm) */
    for (y = 2; y < h-2; y ++)
    {
        unsigned short* native = BRIGHT_ROW ? bright : dark;
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = (raw_get_pixel(x, y+2) + raw_get_pixel(x, y-2)) / 2;
            native[x + y * w] = raw_get_pixel(x, y);
        }
    }
    /* estimate ISO difference between bright and dark exposures */
    double corr_ev = 0;
    
    /* don't forget that estimate_iso only works on 14-bit data, but we are working on 16 */
    raw_info.black_level /= 4;
    raw_info.white_level /= 4;
    int ok = estimate_iso(dark, bright, &corr_ev);
    raw_info.black_level *= 4;
    raw_info.white_level *= 4;
    if (!ok) goto err;

    /* propagate the adjustments for black image, performed by estimate_iso */
    for (y = 2; y < h-2; y ++)
    {
        if (BRIGHT_ROW)
            continue;
        for (x = 0; x < w; x ++)
            raw_set_pixel16(x, y, dark[x + y*w]);
    }

    printf("Interpolation  : %s\n", INTERP_METHOD_NAME
        #ifdef CHROMA_SMOOTH
        "-chroma5x5"
        #endif
        #ifdef ALIAS_BLEND
        "-alias"
        #endif
    );

#if defined(INTERP_MEAN23) || defined(INTERP_MEAN23_EDGE)
    for (y = 2; y < h-2; y ++)
    {
        unsigned short* native = BRIGHT_ROW ? bright : dark;
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        int is_rg = (y % 2 == 0); /* RG or GB? */
        
        for (x = 2; x < w-3; x += 2)
        {
        
            /* red/blue: interpolate from (x,y+2) and (x,y-2) */
            /* green: interpolate from (x+1,y+1),(x-1,y+1),(x,y-2) or (x+1,y-1),(x-1,y-1),(x,y+2), whichever has the correct brightness */
            
            int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;
            
            if (is_rg)
            {
                int ra = raw_get_pixel_14to16(x, y-2);
                int rb = raw_get_pixel_14to16(x, y+2);
                int er=0; int ri = mean2(raw2ev[ra], raw2ev[rb], raw2ev[white], &er);
                
                int ga = raw_get_pixel_14to16(x+1+1, y+s);
                int gb = raw_get_pixel_14to16(x+1-1, y+s);
                int gc = raw_get_pixel_14to16(x+1, y-2*s);
                int eg=0; int gi = mean3(raw2ev[ga], raw2ev[gb], raw2ev[gc], raw2ev[white], &eg);

                interp[x   + y * w] = ev2raw[ri];
                interp[x+1 + y * w] = ev2raw[gi];
            }
            else
            {
                int ba = raw_get_pixel_14to16(x+1  , y-2);
                int bb = raw_get_pixel_14to16(x+1  , y+2);
                int eb=0; int bi = mean2(raw2ev[ba], raw2ev[bb], raw2ev[white], &eb);

                int ga = raw_get_pixel_14to16(x+1, y+s);
                int gb = raw_get_pixel_14to16(x-1, y+s);
                int gc = raw_get_pixel_14to16(x, y-2*s);
                int eg=0; int gi = mean3(raw2ev[ga], raw2ev[gb], raw2ev[gc], raw2ev[white], &eg);

                interp[x   + y * w] = ev2raw[gi];
                interp[x+1 + y * w] = ev2raw[bi];
            }

            native[x   + y * w] = raw_get_pixel_14to16(x, y);
            native[x+1 + y * w] = raw_get_pixel_14to16(x+1, y);
        }
    }
#else
    for (y = 2; y < h-2; y ++)
    {
        unsigned short* native = BRIGHT_ROW ? bright : dark;
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 2; x < w-2; x ++)
        {
            int ra = raw_get_pixel_14to16(x, y-2);
            int rb = raw_get_pixel_14to16(x, y+2);
            int ral = raw_get_pixel_14to16(x-2, y-2);
            int rbl = raw_get_pixel_14to16(x-2, y+2);
            int rar = raw_get_pixel_14to16(x+2, y-2);
            int rbr = raw_get_pixel_14to16(x+2, y+2);

            int ri = ev2raw[
                interp6(
                    raw2ev[ra], 
                    raw2ev[rb],
                    raw2ev[ral],
                    raw2ev[rbl],
                    raw2ev[rar],
                    raw2ev[rbr],
                    raw2ev[white]
                )
            ];
            
            interp[x   + y * w] = ri;
            native[x   + y * w] = raw_get_pixel_14to16(x, y);
        }
    }
#endif

#ifdef INTERP_MEAN23_EDGE /* second step, for detecting edges */
    for (y = 2; y < h-2; y ++)
    {
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 2; x < w-2; x ++)
        {
            int Ra = raw_get_pixel_14to16(x, y-2);
            int Rb = raw_get_pixel_14to16(x, y+2);
            int Ral = raw_get_pixel_14to16(x-2, y-2);
            int Rbl = raw_get_pixel_14to16(x-2, y+2);
            int Rar = raw_get_pixel_14to16(x+2, y-2);
            int Rbr = raw_get_pixel_14to16(x+2, y+2);
            
            int rae = raw2ev[Ra];
            int rbe = raw2ev[Rb];
            int rale = raw2ev[Ral];
            int rble = raw2ev[Rbl];
            int rare = raw2ev[Rar];
            int rbre = raw2ev[Rbr];
            int whitee = raw2ev[white];

            /* median */
            int ri = ev2raw[interp6(rae, rbe, rale, rble, rare, rbre, whitee)];
            
            /* mean23, computed at previous step */
            int ri0 = interp[x + y * w];
            
            /* mean23 overexposed? use median without thinking twice */
            if (ri0 >= white)
            {
                interp[x + y * w] = ri;
                continue;
            }
            
            int ri0e = raw2ev[ri0];
            
            /* threshold for edges */
            int thr = EV_RESOLUTION / 8;
            
            /* detect diagonal edge patterns, where median looks best:
             *       
             *       . . *          . * *           * * *
             *         ?              ?               ?            and so on
             *       * * *          * * *           * . . 
             */
            
            if (rbe < ri0e - thr && rble < ri0e - thr && rbre < ri0e - thr) /* bottom dark */
            {
                if ((rale < ri0e - thr && rare > ri0e + thr) || (rare < ri0e - thr && rale > ri0e - thr))
                    interp[x + y * w] = ri;
            }
            else if (rbe > ri0e + thr && rble > ri0e + thr && rbre > ri0e + thr) /* bottom bright */
            {
                if ((rale > ri0e + thr && rare < ri0e - thr) || (rare > ri0e + thr && rale < ri0e - thr))
                    interp[x + y * w] = ri;
            }
            else if (rae < ri0e - thr && rale < ri0e - thr && rare < ri0e - thr) /* top dark */
            {
                if ((rble > ri0e + thr && rbre < ri0e - thr) || (rbre > ri0e + thr && rble < ri0e - thr))
                    interp[x + y * w] = ri;
            }
            else if (rae > ri0e + thr && rale > ri0e + thr && rare > ri0e + thr) /* top bright */
            {
                if ((rble > ri0e + thr && rbre < ri0e - thr) || (rbre > ri0e + thr && rble < ri0e - thr))
                    interp[x + y * w] = ri;
            }
        }
    }
#endif

    /* border interpolation */
    for (y = 0; y < 3; y ++)
    {
        unsigned short* native = BRIGHT_ROW ? bright : dark;
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel_14to16(x, y+2);
            native[x + y * w] = raw_get_pixel_14to16(x, y);
        }
    }

    for (y = h-2; y < h; y ++)
    {
        unsigned short* native = BRIGHT_ROW ? bright : dark;
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel_14to16(x, y-2);
            native[x + y * w] = raw_get_pixel_14to16(x, y);
        }
    }

    for (y = 2; y < h; y ++)
    {
        unsigned short* native = BRIGHT_ROW ? bright : dark;
        unsigned short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < 2; x ++)
        {
            interp[x + y * w] = raw_get_pixel_14to16(x, y-2);
            native[x + y * w] = raw_get_pixel_14to16(x, y);
        }

        for (x = w-3; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel_14to16(x-2, y-2);
            native[x + y * w] = raw_get_pixel_14to16(x-2, y);
        }
    }
    
    /* we have now switched to 16-bit, update noise numbers */
    dark_noise *= 4;
    bright_noise *= 4;
    dark_noise_ev += 2;
    bright_noise_ev += 2;

    double lowiso_dr = log2(white - black) - dark_noise_ev;
    double highiso_dr = log2(white - black) - bright_noise_ev;
    printf("Dynamic range  : %.02f (+) %.02f => %.02f EV (in theory)\n", lowiso_dr, highiso_dr, highiso_dr + corr_ev);
    printf("Matching brightness...\n");
    double corr = pow(2, corr_ev);
    int white_darkened = (white - black) / corr + black;
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            {
                /* darken bright image so it looks like the low-ISO one */
                /* this is best done in linear space, to handle values under black level */
                /* but it's important to work in 16-bit or more, to minimize quantization errors */
                int b = bright[x + y*w];
                int bd = (b - black) / corr + black;
                bright[x + y*w] = bd;
            }
        }
    }
    
    /* update bright noise measurements, so they can be compared after scaling */
    bright_noise /= corr;
    bright_noise_ev -= corr_ev;
    
#if 1
    {
        printf("Looking for hot/cold pixels...\n");
        int hot_pixels = 0;
        int cold_pixels = 0;
        unsigned short* hot = malloc(w * h * sizeof(unsigned short));
        CHECK(hot, "malloc");
        memset(hot, 0, w * h * sizeof(unsigned short));
        for (y = 6; y < h-6; y ++)
        {
            for (x = 6; x < w-6; x ++)
            {
                {
                    int d = dark[x + y*w];
                    int b = bright[x + y*w];

                    /* don't check hot pixels near overexposed areas */
                    int i,j;
                    int over = 0;
                    for (i = -3; i <= 3; i++)
                    {
                        for (j = -3; j <= 3; j++)
                        {
                            if (bright[x+j + (y+i)*w] >= white_darkened || dark[x+j + (y+i)*w] >= white_darkened)
                            {
                                over = 1;
                                break;
                            }
                        }
                    }
                    if (over)
                        continue;
                    
                    /* for speedup */
                    int maybe_hot = (raw2ev[d] - raw2ev[b] > EV_RESOLUTION) && (d - b > dark_noise);
                    int maybe_cold = (raw2ev[d] - raw2ev[b] < -EV_RESOLUTION) && (d - b < -dark_noise);
                    if (!maybe_hot && !maybe_cold)
                        continue;

                    /* let's look at the neighbours: is this pixel clearly brigher? (isolated) */
                    int neighbours[50];
                    int k = 0;
                    for (i = -3; i <= 3; i++)
                    {
                        for (j = -3; j <= 3; j++)
                        {
                            if (ABS(i) <= 1 && ABS(j) <= 1)
                                continue;
                            
                            int d = dark[x+j*2 + (y+i*2)*w];
                            int b = bright[x+j*2 + (y+i*2)*w];
                            int p = BRIGHT_ROW ? b : d;
                            neighbours[k++] = p;
                        }
                    }
                    int max = 0;
                    int imax = 0;
                    int min = 1000000;
                    int imin = 0;
                    for (i = 0; i < k; i++)
                    {
                        if (neighbours[i] > max)
                        {
                            max = neighbours[i];
                            imax = i;
                        }
                        if (neighbours[i] < min)
                        {
                            min = neighbours[i];
                            imin = i;
                        }
                    }

                    int second_max = 0;
                    int second_min = 1000000;
                    for (i = 0; i < k; i++)
                    {
                        if (neighbours[i] > second_max && i != imax)
                        {
                            second_max = neighbours[i];
                        }
                        if (neighbours[i] < second_min && i != imin)
                        {
                            second_min = neighbours[i];
                        }
                    }

                    int is_hot =
                        (raw2ev[d] - raw2ev[max] > -EV_RESOLUTION/2) &&
                        (raw2ev[d] - raw2ev[second_max] > EV_RESOLUTION/2) &&
                        (d - second_max > dark_noise/2);

                    int is_cold = 
                        (raw2ev[d] - raw2ev[min] < EV_RESOLUTION/2) &&
                        (raw2ev[d] - raw2ev[second_min] < -EV_RESOLUTION/2) &&
                        (d - second_min < -dark_noise);
                    
                    //~ int is_cold = d < black + dark_noise;

                    if (is_hot)
                    {
                        hot_pixels++;
                        hot[x + y*w] = 1;
                    }

                    if (is_cold)
                    {
                        cold_pixels++;
                        hot[x + y*w] = -1;
                    }
                }
            }
        }

        /* correct all hot pixels from the high-ISO image, which is usually clean */
        for (y = 0; y < h; y ++)
        {
            for (x = 0; x < w; x ++)
            {
                if (hot[x + y*w])
                {
                    dark[x + y*w] = bright[x + y*w];
                }
            }
        }
        free(hot);

        if (hot_pixels)
            printf("Hot pixels     : %d\n", hot_pixels);

        if (cold_pixels)
            printf("Cold pixels    : %d\n", cold_pixels);
    }
#endif

    /* reconstruct a full-resolution image (discard interpolated fields whenever possible) */
    /* this has full detail and lowest possible aliasing, but it has high shadow noise and color artifacts when high-iso starts clipping */

    printf("Full-res reconstruction...\n");
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            if (BRIGHT_ROW)
            {
                int f = bright[x + y*w];
                /* if the brighter copy is overexposed, the guessed pixel for sure has higher brightness */
                fullres[x + y*w] = f < white_darkened ? f : MAX(f, dark[x + y*w]);
            }
            else
            {
                fullres[x + y*w] = dark[x + y*w]; 
            }
        }
    }
 
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
    double overlap = lowiso_dr - clipped_ev;

    /* you get better colors, less noise, but a little more jagged edges if we underestimate the overlap amount */
    /* maybe expose a tuning factor? (preference towards resolution or colors) */
    overlap -= MIN(3, overlap - 3);
    
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

    printf("Half-res blending...\n");

    /* mixing curve */
    double max_ev = log2(white/4 - black/4);
    static double mix_curve[65536];
    
    for (i = 0; i < 65536; i++)
    {
        double ev = log2(MAX(i/4.0 - black/4.0, 1)) + corr_ev;
        double c = -cos(MAX(MIN(ev-(max_ev-overlap),overlap),0)*M_PI/overlap);
        double k = (c+1) / 2;
        mix_curve[i] = k;
    }

#if 0
    FILE* f = fopen("mix-curve.m", "w");
    fprintf(f, "x = 0:65535; \n");

    fprintf(f, "ev = [");
    for (i = 0; i < 65536; i++)
        fprintf(f, "%f ", log2(MAX(i/4.0 - black/4.0, 1)));
    fprintf(f, "];\n");
    
    fprintf(f, "k = [");
    for (i = 0; i < 65536; i++)
        fprintf(f, "%f ", mix_curve[i]);
    fprintf(f, "];\n");
    
    fprintf(f, "plot(ev, k);\n");
    fclose(f);
    
    system("octave --persist mix-curve.m");
#endif
    
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            /* bright and dark source pixels  */
            /* they may be real or interpolated */
            /* they both have the same brightness (they were adjusted before this loop), so we are ready to mix them */ 
            int b = bright[x + y*w];
            int d  = dark[x + y*w];

            /* go from linear to EV space */
            int bev = raw2ev[b];
            int dev = raw2ev[d];

            /* blending factor */
            double k = COERCE(mix_curve[b & 65535], 0, 1);
            
            /* mix bright and dark exposures */
            int mixed = bev * (1-k) + dev * k;
            halfres[x + y*w] = ev2raw[mixed];
        }
    }

#ifdef CHROMA_SMOOTH
    printf("Chroma filtering...\n");
    fullres_smooth = malloc(w * h * sizeof(unsigned short));
    CHECK(fullres_smooth, "malloc");
    halfres_smooth = malloc(w * h * sizeof(unsigned short));
    CHECK(halfres_smooth, "malloc");

    memcpy(fullres_smooth, fullres, w * h * sizeof(unsigned short));
    memcpy(halfres_smooth, halfres, w * h * sizeof(unsigned short));
#endif

#ifdef CHROMA_SMOOTH
    chroma_smooth_5x5(fullres, fullres_smooth, raw2ev, ev2raw);
    chroma_smooth_5x5(halfres, halfres_smooth, raw2ev, ev2raw);
#endif

#if 0 /* for debugging only */
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("normal.dng");

    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, bright[x + y*w]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("bright.dng");

    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, dark[x + y*w]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("dark.dng");

    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, fullres[x + y*w]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("fullres.dng");

    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, halfres[x + y*w]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("halfres.dng");

#ifdef CHROMA_SMOOTH
    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, fullres_smooth[x + y*w]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("fullres_smooth.dng");

    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, halfres_smooth[x + y*w]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("halfres_smooth.dng");
#endif

    int yb = 0;
    int yd = h/2;
    for (y = 0; y < h; y ++)
    {
        if (BRIGHT_ROW)
        {
            for (x = 0; x < w; x ++)
                raw_set_pixel16(x, yb, bright[x + y*w]);
            yb++;
        }
        else
        {
            for (x = 0; x < w; x ++)
                raw_set_pixel16(x, yd, dark[x + y*w]);
            yd++;
        }
    }
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("split.dng");
#endif

#ifdef ALIAS_BLEND
    printf("Building alias map...\n");

    unsigned short* alias_aux = malloc(w * h * sizeof(unsigned short));
    CHECK(alias_aux, "malloc");
    
    /* build the aliasing maps (where it's likely to get aliasing) */
    /* do this by comparing fullres and halfres images */
    /* if the difference is small, we'll prefer halfres for less noise, otherwise fullres for less aliasing */
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int f = fullres_smooth[x + y*w];
            int h = halfres_smooth[x + y*w];
            int fe = raw2ev[f];
            int he = raw2ev[h];
            int e_lin = ABS(f - h); /* error in linear space, for shadows (downweights noise) */
            e_lin = MAX(e_lin - dark_noise, 0);
            int e_log = ABS(fe - he); /* error in EV space, for highlights (highly sensitive to noise) */
            alias_map[x + y*w] = MIN(e_lin*16, e_log/8);
        }
    }

#if 0
    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, ev2raw[COERCE(alias_map[x + y*w] * 1024, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("alias.dng");
#endif

    memcpy(alias_aux, alias_map, w * h * sizeof(unsigned short));
    
    /* trial and error - too high = aliasing, too low = noisy */
    int ALIAS_MAP_MAX = 15000;

    printf("Dilating alias map...\n");
    for (y = 6; y < h-6; y ++)
    {
        for (x = 6; x < w-6; x ++)
        {
            /* optimizing... the brute force way */
            int c0 = alias_map[x+0 + (y+0) * w];
            int c1 = MAX(MAX(alias_map[x+0 + (y-2) * w], alias_map[x-2 + (y+0) * w]), MAX(alias_map[x+2 + (y+0) * w], alias_map[x+0 + (y+2) * w]));
            int c2 = MAX(MAX(alias_map[x-2 + (y-2) * w], alias_map[x+2 + (y-2) * w]), MAX(alias_map[x-2 + (y+2) * w], alias_map[x+2 + (y+2) * w]));
            int c3 = MAX(MAX(alias_map[x+0 + (y-4) * w], alias_map[x-4 + (y+0) * w]), MAX(alias_map[x+4 + (y+0) * w], alias_map[x+0 + (y+4) * w]));
            //~ int c4 = MAX(MAX(alias_map[x-2 + (y-4) * w], alias_map[x+2 + (y-4) * w]), MAX(alias_map[x-4 + (y-2) * w], alias_map[x+4 + (y-2) * w]));
            //~ int c5 = MAX(MAX(alias_map[x-4 + (y+2) * w], alias_map[x+4 + (y+2) * w]), MAX(alias_map[x-2 + (y+4) * w], alias_map[x+2 + (y+4) * w]));
            //~ int c = MAX(MAX(MAX(c0, c1), MAX(c2, c3)), MAX(c4, c5));
            int c = MAX(MAX(c0, c1), MAX(c2, c3));
            
            alias_aux[x + y * w] = c;
        }
    }

#if 0
    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, ev2raw[COERCE(alias_aux[x + y*w] * 1024, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("alias-dilated.dng");
#endif

    printf("Smoothing alias map...\n");
    /* gaussian blur */
    for (y = 6; y < h-6; y ++)
    {
        for (x = 6; x < w-6; x ++)
        {

/* code generation
            const int blur[4][4] = {
                {1024,  820,  421,  139},
                { 820,  657,  337,  111},
                { 421,  337,  173,   57},
                { 139,  111,   57,    0},
            };
            const int blur_unique[] = {1024, 820, 657, 421, 337, 173, 139, 111, 57};

            int k;
            for (k = 0; k < COUNT(blur_unique); k++)
            {
                int dx, dy;
                int c = 0;
                printf("(");
                for (dy = -3; dy <= 3; dy++)
                {
                    for (dx = -3; dx <= 3; dx++)
                    {
                        c += alias_aux[x + dx + (y + dy) * w] * blur[ABS(dx)][ABS(dy)] / 1024;
                        if (blur[ABS(dx)][ABS(dy)] == blur_unique[k])
                            printf("alias_aux[x%+d + (y%+d) * w] + ", dx, dy);
                    }
                }
                printf("\b\b\b) * %d / 1024 + \n", blur_unique[k]);
            }
            exit(1);
*/
            /* optimizing... the brute force way */
            int c = 
                (alias_aux[x+0 + (y+0) * w])+ 
                (alias_aux[x+0 + (y-2) * w] + alias_aux[x-2 + (y+0) * w] + alias_aux[x+2 + (y+0) * w] + alias_aux[x+0 + (y+2) * w]) * 820 / 1024 + 
                (alias_aux[x-2 + (y-2) * w] + alias_aux[x+2 + (y-2) * w] + alias_aux[x-2 + (y+2) * w] + alias_aux[x+2 + (y+2) * w]) * 657 / 1024 + 
                (alias_aux[x+0 + (y-2) * w] + alias_aux[x-2 + (y+0) * w] + alias_aux[x+2 + (y+0) * w] + alias_aux[x+0 + (y+2) * w]) * 421 / 1024 + 
                (alias_aux[x-2 + (y-2) * w] + alias_aux[x+2 + (y-2) * w] + alias_aux[x-2 + (y-2) * w] + alias_aux[x+2 + (y-2) * w] + alias_aux[x-2 + (y+2) * w] + alias_aux[x+2 + (y+2) * w] + alias_aux[x-2 + (y+2) * w] + alias_aux[x+2 + (y+2) * w]) * 337 / 1024 + 
                //~ (alias_aux[x-2 + (y-2) * w] + alias_aux[x+2 + (y-2) * w] + alias_aux[x-2 + (y+2) * w] + alias_aux[x+2 + (y+2) * w]) * 173 / 1024 + 
                //~ (alias_aux[x+0 + (y-6) * w] + alias_aux[x-6 + (y+0) * w] + alias_aux[x+6 + (y+0) * w] + alias_aux[x+0 + (y+6) * w]) * 139 / 1024 + 
                //~ (alias_aux[x-2 + (y-6) * w] + alias_aux[x+2 + (y-6) * w] + alias_aux[x-6 + (y-2) * w] + alias_aux[x+6 + (y-2) * w] + alias_aux[x-6 + (y+2) * w] + alias_aux[x+6 + (y+2) * w] + alias_aux[x-2 + (y+6) * w] + alias_aux[x+2 + (y+6) * w]) * 111 / 1024 + 
                //~ (alias_aux[x-2 + (y-6) * w] + alias_aux[x+2 + (y-6) * w] + alias_aux[x-6 + (y-2) * w] + alias_aux[x+6 + (y-2) * w] + alias_aux[x-6 + (y+2) * w] + alias_aux[x+6 + (y+2) * w] + alias_aux[x-2 + (y+6) * w] + alias_aux[x+2 + (y+6) * w]) * 57 / 1024;
                0;
            alias_map[x + y * w] = c;
        }
    }

#if 0
    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, ev2raw[COERCE(alias_map[x + y*w] * 128, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("alias-smooth.dng");
#endif

    /* make it grayscale */
    for (y = 2; y < h-2; y += 2)
    {
        for (x = 2; x < w-2; x += 2)
        {
            int a = alias_map[x   +     y * w];
            int b = alias_map[x+1 +     y * w];
            int c = alias_map[x   + (y+1) * w];
            int d = alias_map[x+1 + (y+1) * w];
            int C = MAX(MAX(a,b), MAX(c,d));
            
            C = MIN(C, ALIAS_MAP_MAX);

            alias_map[x   +     y * w] = 
            alias_map[x+1 +     y * w] = 
            alias_map[x   + (y+1) * w] = 
            alias_map[x+1 + (y+1) * w] = C;
        }
    }

#if 0
    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel16(x, y, ev2raw[(long long)alias_map[x + y*w] * 13*EV_RESOLUTION / ALIAS_MAP_MAX]);
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("alias-filtered.dng");
#endif

    /* where the image is overexposed? */
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            overexposed[x + y * w] = bright[x + y * w] >= white_darkened || dark[x + y * w] >= white ? 100 : 0;
        }
    }
    
    /* "blur" the overexposed map */
    memcpy(alias_aux, overexposed, w * h * sizeof(unsigned short));

    for (y = 3; y < h-3; y ++)
    {
        for (x = 3; x < w-3; x ++)
        {
            overexposed[x + y * w] = 
                (alias_aux[x+0 + (y+0) * w])+ 
                (alias_aux[x+0 + (y-1) * w] + alias_aux[x-1 + (y+0) * w] + alias_aux[x+1 + (y+0) * w] + alias_aux[x+0 + (y+1) * w]) * 820 / 1024 + 
                (alias_aux[x-1 + (y-1) * w] + alias_aux[x+1 + (y-1) * w] + alias_aux[x-1 + (y+1) * w] + alias_aux[x+1 + (y+1) * w]) * 657 / 1024 + 
                //~ (alias_aux[x+0 + (y-2) * w] + alias_aux[x-2 + (y+0) * w] + alias_aux[x+2 + (y+0) * w] + alias_aux[x+0 + (y+2) * w]) * 421 / 1024 + 
                //~ (alias_aux[x-1 + (y-2) * w] + alias_aux[x+1 + (y-2) * w] + alias_aux[x-2 + (y-1) * w] + alias_aux[x+2 + (y-1) * w] + alias_aux[x-2 + (y+1) * w] + alias_aux[x+2 + (y+1) * w] + alias_aux[x-1 + (y+2) * w] + alias_aux[x+1 + (y+2) * w]) * 337 / 1024 + 
                //~ (alias_aux[x-2 + (y-2) * w] + alias_aux[x+2 + (y-2) * w] + alias_aux[x-2 + (y+2) * w] + alias_aux[x+2 + (y+2) * w]) * 173 / 1024 + 
                //~ (alias_aux[x+0 + (y-3) * w] + alias_aux[x-3 + (y+0) * w] + alias_aux[x+3 + (y+0) * w] + alias_aux[x+0 + (y+3) * w]) * 139 / 1024 + 
                //~ (alias_aux[x-1 + (y-3) * w] + alias_aux[x+1 + (y-3) * w] + alias_aux[x-3 + (y-1) * w] + alias_aux[x+3 + (y-1) * w] + alias_aux[x-3 + (y+1) * w] + alias_aux[x+3 + (y+1) * w] + alias_aux[x-1 + (y+3) * w] + alias_aux[x+1 + (y+3) * w]) * 111 / 1024 + 
                //~ (alias_aux[x-2 + (y-3) * w] + alias_aux[x+2 + (y-3) * w] + alias_aux[x-3 + (y-2) * w] + alias_aux[x+3 + (y-2) * w] + alias_aux[x-3 + (y+2) * w] + alias_aux[x+3 + (y+2) * w] + alias_aux[x-2 + (y+3) * w] + alias_aux[x+2 + (y+3) * w]) * 57 / 1024;
                0;
        }
    }

    free(alias_aux);
#endif


    /* fullres mixing curve */
    static double fullres_curve[65536];
    
    static double fullres_start = 5;
    static double fullres_transition = 2;
    
    for (i = 0; i < 65536; i++)
    {
        double ev2 = log2(MAX(i/4.0 - black/4.0, 1));
        double c2 = -cos(COERCE(ev2 - fullres_start, 0, fullres_transition)*M_PI/fullres_transition);
        double f = (c2+1) / 2;
        fullres_curve[i] = f;
    }

#if 0
    FILE* f = fopen("mix-curve.m", "w");
    fprintf(f, "x = 0:65535; \n");

    fprintf(f, "ev = [");
    for (i = 0; i < 65536; i++)
        fprintf(f, "%f ", log2(MAX(i/4.0 - black/4.0, 1)));
    fprintf(f, "];\n");
    
    fprintf(f, "k = [");
    for (i = 0; i < 65536; i++)
        fprintf(f, "%f ", mix_curve[i]);
    fprintf(f, "];\n");

    fprintf(f, "f = [");
    for (i = 0; i < 65536; i++)
        fprintf(f, "%f ", fullres_curve[i]);
    fprintf(f, "];\n");
    
    fprintf(f, "plot(ev, k, ev, f, 'r');\n");
    fclose(f);
    
    system("octave --persist mix-curve.m");
#endif
    
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            /* high-iso image (for measuring signal level) */
            int b = bright[x + y*w];

            /* half-res image (interpolated and chroma filtered, best for low-contrast shadows) */
            int hr = halfres_smooth[x + y*w];
            
            /* full-res image (non-interpolated, except where one ISO is blown out) */
            int fr = fullres[x + y*w];

            #ifdef CHROMA_SMOOTH
            /* full res with some smoothing applied to hide aliasing artifacts */
            int frs = fullres_smooth[x + y*w];
            #else
            int frs = fr;
            #endif

            /* go from linear to EV space */
            int hrev = raw2ev[hr];
            int frev = raw2ev[fr];
            int frsev = raw2ev[frs];

#ifdef FULLRES_ONLY 
            int output = frev;
#else
            /* blending factor */
            int f = fullres_curve[b & 65535];
            
            #ifdef ALIAS_BLEND
            int co = alias_map[x + y*w];
            double c = COERCE(co / (double) ALIAS_MAP_MAX, 0, 1);
            double ovf = COERCE(overexposed[x + y*w] / 200.0, 0, 1);
            c = MAX(c, ovf);
            //~ k = MIN(MAX(k, ovf*5), 1);
            #else
            double c = 1;
            #endif

            double noisy_or_overexposed = MAX(ovf, 1-f);

            /* use data from both ISOs in high-detail areas, even if it's noisier (less aliasing) */
            f = MAX(f, c);
            
            /* use smoothing in noisy near-overexposed areas to hide color artifacts */
            double fev = noisy_or_overexposed * frsev + (1-noisy_or_overexposed) * frev;
            
            /* blend "half-res" and "full-res" images smoothly to avoid banding*/
            int output = hrev * (1-f) + fev * f;

            /* show full-res map (for debugging) */
            //~ output = f * 14*EV_RESOLUTION;
            
            /* show alias map (for debugging) */
            //~ output = c * 14*EV_RESOLUTION;
#endif
            /* safeguard */
            output = COERCE(output, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1);
            
            /* back to linear space and commit */
            raw_set_pixel16(x, y, ev2raw[output]);
        }
    }

    /* let's see how much dynamic range we actually got */
    compute_black_noise(8, raw_info.active_area.x1 - 8, 20, raw_info.active_area.y2 - 20, 1, 1, &noise_avg, &noise_std[0]);
    printf("Noise level    : %.02f (16-bit)\n", noise_std[0]);
    printf("Dynamic range  : %.02f EV (cooked)\n", log2(white - black) - log2(noise_std[0]));

    if (!rggb) /* back to GBRG */
    {
        raw_info.buffer -= raw_info.pitch;
        raw_info.active_area.y1--;
        raw_info.active_area.y2++;
        raw_info.jpeg.y--;
        raw_info.jpeg.height += 3;
        raw_info.height++;
        h++;
    }

    goto cleanup;

err:
    ret = 0;

cleanup:
    free(dark);
    free(bright);
    free(fullres);
    free(halfres);
    #ifdef ALIAS_BLEND
    free(alias_map);
    free(overexposed);
    #endif
    #ifdef CHROMA_SMOOTH
    if (fullres_smooth) free(fullres_smooth);
    if (halfres_smooth) free(halfres_smooth);
    #endif
    return ret;
}
