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
#undef INTERP_MEAN_2RB_3G
#undef INTERP_MEAN_6
#undef INTERP_MEAN_4_OUT_OF_6
#undef INTERP_MEAN_5_OUT_OF_6
#define INTERP_MEDIAN_6

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "../../src/raw.h"
#include "qsort.h"  /* much faster than standard C qsort */

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!ok) FAIL(fmt, ## __VA_ARGS__); }

#define FIXP_ONE 65536
#define FIXP_RANGE 65536

#define RAW_MUL(p, x) ((((int)(p) - raw_info.black_level) * (int)(x) / FIXP_ONE) + raw_info.black_level)
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
    short* buf16 = (short*) buf;
    int i;
    for (i = 0; i < count/2; i++)
    {
        short x = buf16[i];
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
            int ok = 0;
            ok = black_subtract(left_margin, top_margin);
            if (!ok)
                printf("Black subtract didn't work\n");
            ok = hdr_interpolate();
            CHECK(ok, "hdr_interpolate");

            char out_filename[1000];
            snprintf(out_filename, sizeof(out_filename), "%s", filename);
            int len = strlen(out_filename);
            out_filename[len-3] = 'D';
            out_filename[len-2] = 'N';
            out_filename[len-1] = 'G';

            raw_info.black_level *= 4;
            raw_info.white_level *= 4;
            reverse_bytes_order(raw_info.buffer, raw_info.frame_size);

            printf("Output file    : %s\n", out_filename);
            save_dng(out_filename);

            raw_info.black_level /= 4;
            raw_info.white_level /= 4;

            char exif_cmd[100];
            snprintf(exif_cmd, sizeof(exif_cmd), "exiftool -tagsFromFile \"%s\" \"%s\" -overwrite_original", filename, out_filename);
            int r = system(exif_cmd);
            if (r != 0)
                printf("Exiftool didn't work\n");
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

static int white_detect()
{
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
        for (x = 2; x < left_margin - 2; x++)
        {
            avg += raw_get_pixel16(x, y);
        }
        vblack[y] = avg / (left_margin - 4);
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
            for (y = y0; y < top_margin-2; y += 4)
            {
                offset += blackframe[y*w];
                num++;
            }
            offset /= num;
        }

        for (x = 0; x < w; x++)
        {
            int num = 0;
            int avg = 0;
            for (y = y0; y < top_margin-2; y += 4)
            {
                avg += raw_get_pixel16(x, y) - offset;
                num++;
            }
            hblack[x] = avg / num;
        }
        
        /* perform some stronger filtering (averaging), since this data is a lot noisier */
        /* if we don't do that, we will add some strong FPN to the image */
        for (x = 0; x < w; x++)
        {
            int x2;
            int avg = 0;
            int num = 0;
            for (x2 = x - 1000; x2 < x + 1000; x2 ++)
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
            for (x = 0; x < w; x++)
                blackframe[x + y*w] += hblack[x];
    }

#if 0 /* for debugging only */
    void* old_buffer = raw_info.buffer;
    raw_info.buffer = blackframe;
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng("black.dng");
    raw_info.buffer = old_buffer;
#endif

    /* "subtract" the dark frame, keeping the exif black level and preserving the white level */
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            int p = raw_get_pixel16(x, y);
            int black_delta = raw_info.black_level - blackframe[x + y*w];
            p += black_delta;
            p = p * raw_info.white_level / (raw_info.white_level + black_delta);
            raw_set_pixel16(x, y, p);
        }
    }

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
            int p = raw_get_pixel16(x, y);
            int p2 = raw_get_pixel16(x, y+2);
            if (p < white && p2 < white)
            {
                avg_ev[y%4] += raw2ev[p & 16383];
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
static int mean6(int a, int b, int c, int d, int e, int f)
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

#ifdef INTERP_MEAN_2RB_3G
#define INTERP_METHOD_NAME "mean2rb3g"
#define EV_MEAN2(a,b) ev2raw[(raw2ev[a & 16383] + raw2ev[b & 16383]) / 2]
#define EV_MEAN3(a,b,c) ev2raw[(raw2ev[a & 16383] + raw2ev[b & 16383] + raw2ev[c & 16383]) / 3]
#endif

#define EV_RESOLUTION 2000

static int hdr_interpolate()
{
    int black = raw_info.black_level;
    int white = raw_info.white_level;

    int w = raw_info.width;
    int h = raw_info.height;

    int x, y;

    /* for fast EV - raw conversion */
    static int raw2ev[16384];   /* EV x EV_RESOLUTION */
    static int ev2raw[14*EV_RESOLUTION];
    static int ev2raw16[14*EV_RESOLUTION]; /* from 14-bit to 16-bit */
    
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
        raw_info.buffer += raw_info.pitch;

    int i;
    for (i = 0; i < 16384; i++)
        raw2ev[i] = (int)round(log2(MAX(1, i - black)) * EV_RESOLUTION);
    for (i = 0; i < 14*EV_RESOLUTION; i++)
        ev2raw[i] = COERCE(black + pow(2, ((double)i/EV_RESOLUTION)), black, white+10),
        ev2raw16[i] = COERCE(black*4 + pow(2, ((double)i/EV_RESOLUTION))*4.0, black*4, white*4+10);

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

    /* dark and bright exposures, interpolated */
    short* dark   = malloc(w * h * sizeof(short));
    CHECK(dark, "malloc");
    short* bright = malloc(w * h * sizeof(short));
    CHECK(bright, "malloc");

    memset(dark, 0, w * h * sizeof(short));
    memset(bright, 0, w * h * sizeof(short));

    #define BRIGHT_ROW (is_bright[y % 4])
    
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
                int ra = raw_get_pixel16(x, y-2);
                int rb = raw_get_pixel16(x, y+2);
                int ri = (ra < white && rb < white) ? EV_MEAN2(ra, rb) : white;
                
                int ga = raw_get_pixel16(x+1+1, y+s);
                int gb = raw_get_pixel16(x+1-1, y+s);
                int gc = raw_get_pixel16(x+1, y-2*s);
                int gi = (ga < white && gb < white && gc < white) ? EV_MEAN3(ga, gb, gc) : white;

                interp[x   + y * w] = ri;
                interp[x+1 + y * w] = gi;
            }
            else
            {
                int ba = raw_get_pixel16(x+1  , y-2);
                int bb = raw_get_pixel16(x+1  , y+2);
                int bi = (ba < white && bb < white) ? EV_MEAN2(ba, bb) : white;

                int ga = raw_get_pixel16(x+1, y+s);
                int gb = raw_get_pixel16(x-1, y+s);
                int gc = raw_get_pixel16(x, y-2*s);
                int gi = (ga < white && gb < white && gc < white) ? EV_MEAN3(ga, gb, gc) : white;

                interp[x   + y * w] = gi;
                interp[x+1 + y * w] = bi;
            }

            native[x   + y * w] = raw_get_pixel16(x, y);
            native[x+1 + y * w] = raw_get_pixel16(x+1, y);
        }
    }
#else
    for (y = 2; y < h-2; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 2; x < w-2; x ++)
        {
            int ra = raw_get_pixel16(x, y-2);
            int rb = raw_get_pixel16(x, y+2);
            int ral = raw_get_pixel16(x-2, y-2);
            int rbl = raw_get_pixel16(x-2, y+2);
            int rar = raw_get_pixel16(x+2, y-2);
            int rbr = raw_get_pixel16(x+2, y+2);

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
            native[x   + y * w] = raw_get_pixel16(x, y);
        }
    }
#endif
    /* border interpolation */
    for (y = 0; y < 3; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel16(x, y+2);
            native[x + y * w] = raw_get_pixel16(x, y);
        }
    }

    for (y = h-2; y < h; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel16(x, y-2);
            native[x + y * w] = raw_get_pixel16(x, y);
        }
    }

    for (y = 2; y < h; y ++)
    {
        short* native = BRIGHT_ROW ? bright : dark;
        short* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < 2; x ++)
        {
            interp[x + y * w] = raw_get_pixel16(x, y-2);
            native[x + y * w] = raw_get_pixel16(x, y);
        }

        for (x = w-3; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel16(x-2, y-2);
            native[x + y * w] = raw_get_pixel16(x-2, y);
        }
    }

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
#endif

    /* estimate ISO and black difference between bright and dark exposures */
    float corr_ev = 0;
    int black_delta = 0;
    int ok = estimate_iso(dark, bright, &corr_ev, &black_delta);
    if (!ok) goto err;
    int corr = (int)roundf(corr_ev * EV_RESOLUTION);


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

    fprintf(f, "f = [");
    for (i = 0; i < 16384; i++)
        fprintf(f, "%d ", fullres_curve[i]);
    fprintf(f, "];\n");
    
    fprintf(f, "plot(ev, k, ev, f, 'r');\n");
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
            raw_set_pixel16(x, y, ev2raw16[output] + black_delta/8);
            
            /* fixme: why black_delta/8? it looks good for the Batman shot, but why? */
        }
    }
    
    if (hot_pixels)
        printf("Hot pixels     : %d\n", hot_pixels);

    if (!rggb) /* back to GBRG */
        raw_info.buffer -= raw_info.pitch;

    free(dark);
    free(bright);
    return 1;

err:
    free(dark);
    free(bright);
    return 0;
}
