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

#define EV_RESOLUTION 65536

static int is_bright[4];
#define BRIGHT_ROW (is_bright[y % 4])

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "../../src/raw.h"
#include "../../src/chdk-dng.h"
#include "qsort.h"  /* much faster than standard C qsort */

#include "wirth.h"  /* fast median, generic implementation (also kth_smallest) */
#include "optmed.h" /* fast median for small common array sizes (3, 7, 9...) */

#include "dcraw-bridge.h"
#include "exiftool-bridge.h"
#include "adobedng-bridge.h"

#include "../../src/module.h"
#undef MODULE_STRINGS_SECTION
#define MODULE_STRINGS_SECTION
#include "module_strings.h"


/** Command-line interface */

int interp_method = 0;          /* 0:amaze-edge, 1:mean23 */
int chroma_smooth_method = 2;
int fix_pink_dots = 0;
int fix_bad_pixels = 1;
int use_fullres = 1;
int use_alias_map = 1;
int use_stripe_fix = 1;
int compress_highlights = 0;

int debug_black = 0;
int debug_blend = 0;
int debug_amaze = 0;
int debug_edge = 0;
int debug_alias = 0;
int debug_bad_pixels = 0;
int plot_iso_curve = 0;
int plot_mix_curve = 0;
int plot_fullres_curve = 0;

int compress = 0;

int shortcut_fast = 0;

void check_shortcuts()
{
    if (shortcut_fast)
    {
        interp_method = 1;
        chroma_smooth_method = 0;
        use_alias_map = 0;
        use_fullres = 0;
        use_stripe_fix = 0;
        shortcut_fast = 0;
        fix_bad_pixels = 0;
    }
}

struct cmd_option
{
    int* variable;
    int value_to_assign;
    char* option;
    char* help;
};
#define OPTION_EOL { 0, 0, 0, 0 }

struct cmd_group
{
    char* name;
    struct cmd_option * options;
};
#define OPTION_GROUP_EOL { 0, 0 }

struct cmd_group options[] = {
    {
        "Shortcuts", (struct cmd_option []) {
            { &shortcut_fast, 1, "--fast",  "disable most postprocessing steps (fast, but low quality)\n"
                            "                  (--mean23, --no-cs, --no-fullres, --no-alias-map, --no-stripe-fix, --no-bad-pix)" },
            OPTION_EOL,
        },
    },
    {
        "Interpolation methods", (struct cmd_option[]) {
            { &interp_method, 0, "--amaze-edge",  "use a temporary demosaic step (AMaZE) followed by edge-directed interpolation (default)" },
            { &interp_method, 1, "--mean23",      "average the nearest 2 or 3 pixels of the same color from the Bayer grid (faster)" },
            OPTION_EOL
        },
    },
    {
        "Chroma smoothing", (struct cmd_option[]) {
            { &chroma_smooth_method, 2, "--cs2x2",       "apply 2x2 chroma smoothing in noisy and aliased areas (default)" },
            { &chroma_smooth_method, 3, "--cs3x3",       "apply 3x3 chroma smoothing in noisy and aliased areas" },
            { &chroma_smooth_method, 5, "--cs5x5",       "apply 5x5 chroma smoothing in noisy and aliased areas" },
            { &chroma_smooth_method, 0, "--no-cs",       "disable chroma smoothing" },
            OPTION_EOL
        },
    },
    {
        "Bad pixel handling", (struct cmd_option[]) {
          //{ &fix_pink_dots,  1, "--pink-dots",        "fix pink dots with a early chroma smoothing step" },
            { &fix_bad_pixels, 1, "--bad-pix",          NULL },
            { &fix_bad_pixels, 2, "--really-bad-pix",   "aggressive bad pixel fix, at the expense of detail and aliasing" },
            { &fix_bad_pixels, 0, "--no-bad-pix",       "disable bad pixel fixing (try it if you shoot stars)" },
            { &debug_bad_pixels,1,"--black-bad-pix",    "mark all bad pixels as black (for troubleshooting)" },
            OPTION_EOL
        },
    },
    {
        "Other postprocessing steps", (struct cmd_option[]) {
            { &use_fullres,     0, "--no-fullres",       "disable full-resolution blending" },
            { &use_fullres,     1, "--fullres",          NULL},
            { &use_alias_map,   0, "--no-alias-map",     "disable alias map, used to fix aliasing in deep shadows" },
            { &use_alias_map,   1, "--alias-map",        NULL},
            { &use_stripe_fix,  0, "--no-stripe-fix",    "disable horizontal stripe fix" },
            { &use_stripe_fix,  1, "--stripe-fix",       NULL},
            OPTION_EOL
        },
    },
    {
        "Shadow/highlight handling", (struct cmd_option[]) {
            { &compress_highlights,     4, "--soft-film",       "bake a soft-film curve to compress highlights.\n"
                                                                "                  (default 4 stops if specified; change wth --soft-film=1 to 8)" },
            { &compress_highlights,     1, "--soft-film=1",     NULL},
            { &compress_highlights,     2, "--soft-film=2",     NULL},
            { &compress_highlights,     3, "--soft-film=3",     NULL},
            { &compress_highlights,     4, "--soft-film=4",     NULL},
            { &compress_highlights,     5, "--soft-film=5",     NULL},
            { &compress_highlights,     6, "--soft-film=6",     NULL},
            { &compress_highlights,     7, "--soft-film=7",     NULL},
            { &compress_highlights,     8, "--soft-film=8",     NULL},
            OPTION_EOL
        },
    },
    {
        "DNG compression (requires Adobe DNG Converter)", (struct cmd_option[]) {
            { &compress,     1, "--compress",       "Lossless DNG compression" },
            { &compress,     2, "--compress-lossy", "Lossy DNG compression (be careful, may destroy shadow detail)" },
            OPTION_EOL
        },
    },
    {
        "Troubleshooting options", (struct cmd_option[]) {
            { &debug_blend,    1, "--debug-blend",      "save intermediate images used for blending:\n"
                                                        "    dark.dng        the low-ISO exposure, interpolated\n"
                                                        "    bright.dng      the high-ISO exposure, interpolated and darkened\n"
                                                        "    halfres.dng     half-resolution blending (low noise, high aliasing)\n"
                                                        "    fullres.dng     full-resolution blending (minimal aliasing, high noise)\n"
                                                        "    *_smooth.dng    images after chroma smoothing"
                                                        },
            { &debug_black,    1, "--debug-black",      "save intermediate images used for black level subtraction" },
            { &debug_amaze,    1, "--debug-amaze",      "save AMaZE input and output" },
            { &debug_edge,     1, "--debug-edge",       "save debug info from edge-directed interpolation" },
            { &debug_alias,    1, "--debug-alias",      "save debug info about the alias map" },
            { &plot_iso_curve, 1, "--iso-curve",        "plot the curve fitting results for ISO and black offset (requires octave)" },
            { &plot_mix_curve, 1, "--mix-curve",        "plot the curve used for half-res blending (requires octave)" },
            { &plot_fullres_curve, 1, "--fullres-curve","plot the curve used for full-res blending (requires octave)" },
            OPTION_EOL
        },
    },
    OPTION_GROUP_EOL
};

static void parse_commandline_option(char* option)
{
    struct cmd_group * g;
    for (g = options; g->name; g++)
    {
        struct cmd_option * o;
        for (o = g->options; o->option; o++)
        {
            if (!strcmp(option, o->option))
            {
                *(o->variable) = o->value_to_assign;
                check_shortcuts();
                return;
            }
        }
    }
    printf("Unknown option: %s\n", option);
}

static void show_commandline_help(char* progname)
{
    printf("Command-line usage: %s [OPTIONS] [FILES]\n\n", progname);
    struct cmd_group * g;
    for (g = options; g->name; g++)
    {
        printf("%s:\n", g->name);
        struct cmd_option * o;
        for (o = g->options; o->option; o++)
        {
            if (o->help)
            {
                printf("%-16s: %s\n", o->option, o->help);
            }
        }
        printf("\n");
    }
}

static void solve_commandline_deps()
{
    if (!use_fullres)
        use_alias_map = 0;
}

static void show_active_options()
{
    printf("Active options:\n");
    struct cmd_group * g;
    for (g = options; g->name; g++)
    {
        struct cmd_option * o;
        for (o = g->options; o->option; o++)
        {
            if (o->help && (*o->variable) == o->value_to_assign)
            {
                printf("%-16s: %s\n", o->option, o->help);
            }
        }
    }
}

/* here we only have a global raw_info */
#define save_dng(filename) save_dng(filename, &raw_info)

#define FAIL(fmt,...) { fprintf(stderr, "Error: "); fprintf(stderr, fmt, ## __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }
#define CHECK(ok, fmt,...) { if (!(ok)) FAIL(fmt, ## __VA_ARGS__); }

static void* malloc_or_die(size_t size)
{
    void* p = malloc(size);
    CHECK(p, "malloc");
    return p;
}

/* replace all malloc calls with malloc_or_die (if any call fails, abort right away) */
#define malloc(size) malloc_or_die(size)

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

#define SGN(a) \
   ((a) > 0 ? 1 : -1 )

struct raw_info raw_info = {
    .api_version = 1,
    .bits_per_pixel = 16,
    .black_level = 2048,
    .white_level = 15000,
    .cfa_pattern = 0x02010100,          // Red  Green  Green  Blue
    .calibration_illuminant1 = 1,       // Daylight
};

static int hdr_check();
static int hdr_interpolate();
static int black_subtract(int left_margin, int top_margin);
static int black_subtract_simple(int left_margin, int top_margin);
static void white_detect(int* white_dark, int* white_bright);

static inline int raw_get_pixel16(int x, int y)
{
    uint16_t * buf = raw_info.buffer;
    int value = buf[x + y * raw_info.width];
    return value;
}

static inline void raw_set_pixel16(int x, int y, int value)
{
    uint16_t * buf = raw_info.buffer;
    buf[x + y * raw_info.width] = value;
}

static inline int raw_get_pixel32(int x, int y)
{
    uint32_t * buf = raw_info.buffer;
    int value = buf[x + y * raw_info.width];
    return value;
}

static inline void raw_set_pixel32(int x, int y, int value)
{
    uint32_t * buf = raw_info.buffer;
    buf[x + y * raw_info.width] = value;
}

static inline int raw_get_pixel20(int x, int y)
{
    uint32_t * buf = raw_info.buffer;
    int value = buf[x + y * raw_info.width];
    return value & 0xFFFFF;
}

static inline void raw_set_pixel20(int x, int y, int value)
{
    uint32_t * buf = raw_info.buffer;
    buf[x + y * raw_info.width] = COERCE(value, 0, 0xFFFFF);
}

int raw_get_pixel(int x, int y) {
    return raw_get_pixel16(x,y);
}

/* from 14 bit to 16 bit */
int raw_get_pixel_14to16(int x, int y) {
    return (raw_get_pixel16(x,y) << 2) & 0xFFFF;
}

/* from 14 bit to 20 bit */
int raw_get_pixel_14to20(int x, int y) {
    return (raw_get_pixel16(x,y) << 6) & 0xFFFFF;
}

/* from 20 bit to 16 bit */
int raw_get_pixel_20to16(int x, int y) {
    return (raw_get_pixel32(x,y) >> 4) & 0xFFFF;
}

void raw_set_pixel_20to16(int x, int y, int value) {
    raw_set_pixel16(x, y, value >> 4);
}

static int startswith(char* str, char* prefix)
{
    char* s = str;
    char* p = prefix;
    for (; *p; s++,p++)
        if (*s != *p) return 0;
    return 1;
}

static void reverse_bytes_order(void* buf, int count)
{
    char* buf8 = (char*) buf;
    uint16_t* buf16 = (uint16_t*) buf;
    int i;
    for (i = 0; i < count/2; i++)
    {
        uint16_t x = buf16[i];
        buf8[2*i+1] = x;
        buf8[2*i] = x >> 8;
    }
}

static const char* module_get_string(const char* name)
{
    module_strpair_t *strings = &__module_strings_MODULE_NAME[0];
    
    if (strings)
    {
        for ( ; strings->name != NULL; strings++)
        {
            if (!strcmp(strings->name, name))
            {
                return strings->value;
            }
        }
    }
    return 0;
}

static void save_debug_dng(char* filename)
{
    int black20 = raw_info.black_level;
    int white20 = raw_info.white_level;
    raw_info.black_level = black20/16;
    raw_info.white_level = white20/16;
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    save_dng(filename);
    raw_info.black_level = black20;
    raw_info.white_level = white20;
}

int main(int argc, char** argv)
{
    printf("cr2hdr: a post processing tool for Dual ISO images\n\n");
    printf("Last update: %s\n", module_get_string("Last update"));
    
    if (argc == 1)
    {
        printf("No input files.\n\n");
        printf("GUI usage: drag some CR2 or DNG files over cr2hdr.exe.\n\n");
        show_commandline_help(argv[0]);
        return 0;
    }
    
    int k;
    int r;

    /* parse all command-line options */
    for (k = 1; k < argc; k++)
        if (argv[k][0] == '-')
            parse_commandline_option(argv[k]);
    
    solve_commandline_deps();
    show_active_options();
    
    /* all other arguments are input files */
    for (k = 1; k < argc; k++)
    {
        if (argv[k][0] == '-')
            continue;
        
        char* filename = argv[k];

        printf("\nInput file      : %s\n", filename);

        char dcraw_cmd[1000];
        snprintf(dcraw_cmd, sizeof(dcraw_cmd), "dcraw -v -i -t 0 \"%s\"", filename);
        FILE* t = popen(dcraw_cmd, "r");
        CHECK(t, "%s", filename);
        
        unsigned int model = get_model_id(filename);
        int exit_code = get_raw_info(model, &raw_info);

        CHECK(exit_code == 0, "RAW INFO INJECTION FAILED");

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
        pclose(t);

        printf("Full size       : %d x %d\n", raw_width, raw_height);
        printf("Active area     : %d x %d\n", out_width, out_height);
        
        int left_margin = raw_width - out_width;
        int top_margin = raw_height - out_height;

        snprintf(dcraw_cmd, sizeof(dcraw_cmd), "dcraw -4 -E -c -t 0 \"%s\"", filename);
        FILE* fp = popen(dcraw_cmd, "r");
        CHECK(fp, "%s", filename);
        #ifdef _O_BINARY
        _setmode(_fileno(fp), _O_BINARY);
        #endif

        /* PGM read code from dcraw */
          int dim[3]={0,0,0}, comment=0, number=0, error=0, nd=0, c;

          if (fgetc(fp) != 'P' || fgetc(fp) != '5') error = 1;
          while (!error && nd < 3 && (c = fgetc(fp)) != EOF) {
            if (c == '#')  comment = 1;
            if (c == '\n') comment = 0;
            if (comment) continue;
            if (isdigit(c)) number = 1;
            if (number) {
              if (isdigit(c)) dim[nd] = dim[nd]*10 + c -'0';
              else if (isspace(c)) {
            number = 0;  nd++;
              } else error = 1;
            }
          }

        CHECK(!(error || nd < 3), "dcraw output is not a valid PGM file\n");

        int width = dim[0];
        int height = dim[1];
        CHECK(width == raw_width, "pgm width");
        CHECK(height == raw_height, "pgm height");

        void* buf = malloc(width * (height+1) * 2); /* 1 extra line for handling GBRG easier */
        int size = fread(buf, 1, width * height * 2, fp);
        CHECK(size == width * height * 2, "fread");
        pclose(fp);

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
        
        dng_set_thumbnail_size(384, 252);

        if (hdr_check())
        {
            if (!black_subtract(left_margin, top_margin))
                printf("Black subtract didn't work\n");

            if (hdr_interpolate())
            {
                char out_filename[1000];
                snprintf(out_filename, sizeof(out_filename), "%s", filename);
                int len = strlen(out_filename);
                out_filename[len-3] = 'D';
                out_filename[len-2] = 'N';
                out_filename[len-1] = 'G';

                reverse_bytes_order(raw_info.buffer, raw_info.frame_size);

                printf("Output file     : %s\n", out_filename);
                save_dng(out_filename);

                copy_tags_from_source(filename, out_filename);
                
                if (compress)
                {
                    dng_compress(out_filename, compress-1);
                }
            }
            else
            {
                printf("ISO blending didn't work\n");
            }
        }
        else
        {
            printf("Doesn't look like interlaced ISO\n");
        }
        
        free(buf);
    }
    
    return 0;
}

static void white_detect(int* white_dark, int* white_bright)
{
    /* sometimes the white level is much lower than 15000; this would cause pink highlights */
    /* workaround: consider the white level as a little under the maximum pixel value from the raw file */
    /* caveat: bright and dark exposure may have different white levels, so we'll take the minimum value */
    /* side effect: if the image is not overexposed, it may get brightened a little; shouldn't hurt */
    
    /* ignore hot pixels when finding white level (at least 50 pixels should confirm it) */
    
    int white = 10000;
    int whites[2] = {0, 0};
    int maxies[2] = {white, white};
    int counts[2] = {0, 0};
    int safety_margins[2] = {100, 2000}; /* use a higher safety margin for the higher ISO */

    int x,y;
    for (y = raw_info.active_area.y1; y < raw_info.active_area.y2; y ++)
    {
        for (x = raw_info.active_area.x1; x < raw_info.active_area.x2; x ++)
        {
            int pix = raw_get_pixel16(x, y);
            #define BIN_IDX is_bright[y%4]
            if (pix > maxies[BIN_IDX])
            {
                maxies[BIN_IDX] = pix;
                counts[BIN_IDX] = 1;
            }
            else if (pix == maxies[BIN_IDX])
            {
                counts[BIN_IDX]++;
                if (counts[BIN_IDX] > 50)
                {
                    whites[BIN_IDX] = maxies[BIN_IDX] - safety_margins[BIN_IDX];
                }
            }
            #undef BIN_IDX
        }
    }

    /* no confirmed max? use unconfirmed ones */
    if (whites[0] == 0) whites[0] = maxies[0] - safety_margins[0];
    if (whites[1] == 0) whites[1] = maxies[1] - safety_margins[1];

    //~ printf("%8d %8d\n", whites[0], whites[1]);
    //~ printf("%8d %8d\n", counts[0], counts[1]);
    
    *white_dark = whites[0];
    *white_bright = whites[1];
    
    printf("White levels    : %d %d\n", *white_dark, *white_bright);
}

static int black_subtract(int left_margin, int top_margin)
{
    if (debug_black)
    {
        save_debug_dng("untouched.dng");
    }

    if (left_margin < 10 || top_margin < 10)
    {
        printf("Black borders   : N/A\n");
        return 1;
    }

    printf("Black borders   : %d left, %d top\n", left_margin, top_margin);

    int w = raw_info.width;
    int h = raw_info.height;
    
    int* vblack = malloc(h * sizeof(int));
    int* hblack = malloc(w * sizeof(int));
    int* aux = malloc(MAX(w,h) * sizeof(int));
    uint16_t * blackframe = malloc(w * h * sizeof(uint16_t));

    /* data above this may be gibberish */
    int ymin = (top_margin-8-2) & ~3;
    int ymax = ymin + 8;

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
            for (y = y0; y < ymax; y += 4)
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
                for (y = y0; y < ymax; y += 4)
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
    
    if (debug_black)
    {
        /* change black and white levels to see the black frame when developing the DNG */
        int black_black = INT_MAX;
        int black_white = 0;
        for (y = raw_info.active_area.y1; y < raw_info.active_area.y2; y ++)
        {
            for (x = raw_info.active_area.x1; x < raw_info.active_area.x2; x++)
            {
                black_black = MIN(black_black, blackframe[x + y*w]);
                black_white = MAX(black_white, blackframe[x + y*w]);
            }
        }
        void* old_buffer = raw_info.buffer;
        raw_info.buffer = (void*)blackframe;
        int orig_black = raw_info.black_level;
        int orig_white = raw_info.white_level;
        raw_info.black_level = black_black;
        raw_info.white_level = black_white;
        reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
        save_dng("black.dng");
        raw_info.buffer = old_buffer;
        raw_info.black_level = orig_black;
        raw_info.white_level = orig_white;
    }

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
    printf("Black level     : %d\n", raw_info.black_level);

    if (debug_black)
    {
        save_debug_dng("subtracted.dng");
    }

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
    
    int h = raw_info.height;

    /* average left bar */
    int x,y;
    long long avg = 0;
    int num = 0;
    for (y = 0; y < h; y++)
    {
        for (x = 2; x < left_margin - 8; x++)
        {
            int p = raw_get_pixel20(x, y);
            if (p > 0)
            {
                avg += p;
                num++;
            }
        }
    }
    
    int new_black = avg / num;
        
    int black_delta = raw_info.black_level - new_black;
    
    printf("Black adjust    : %d\n", (int)black_delta);
    raw_info.black_level -= black_delta;
    raw_info.white_level -= black_delta;
    
    return 1;
}

static void compute_black_noise(int x1, int x2, int y1, int y2, int dx, int dy, double* out_mean, double* out_stdev, int (*raw_get_pixel)(int x, int y))
{
    long long black = 0;
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

    double mean = (double) black / num;

    /* compute standard deviation */
    double stdev = 0;
    for (y = y1; y < y2; y += dy)
    {
        for (x = x1; x < x2; x += dx)
        {
            double dif = raw_get_pixel(x, y) - mean;
            stdev += dif * dif;
        }
    }
    stdev /= (num-1);
    stdev = sqrt(stdev);
    
    if (num == 0)
    {
        mean = raw_info.black_level;
        stdev = 8; /* default to 11 stops of DR */
    }

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
            if ((p > black+32 || p2 > black+32) && p < white && p2 < white)
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

static int identify_bright_and_dark_fields(int rggb)
{
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

    int white = raw_info.white_level/4; /* fixme: we work on 14 bits, but white level is the one for 16 bits here */
    if (white > 16383) return 0;        /* should be unreachable */
    
    int w = raw_info.width;
    int h = raw_info.height;

    int x,y;
    int i;
    
    /* build 4 little histograms */
    int hist_size = 16384 * sizeof(int);
    int* hist[4];
    for (i = 0; i < 4; i++)
    {
        hist[i] = malloc(hist_size);
        memset(hist[i], 0, hist_size);
    }
    
    int y0 = (raw_info.active_area.y1 + 3) & ~3;
    
    /* to simplify things, analyze an identical number of bright and dark lines */
    for (y = y0; y < h/4*4; y++)
    {
        for (x = 0; x < w; x++)
            hist[y%4][raw_get_pixel16(x,y) & 16383]++;
    }
    
    int hist_total = 0;
    for (i = 0; i < 16384; i++)
        hist_total += hist[0][i];

    /* choose the highest percentile that is not overexposed */
    int acc[4] = {0};
    int raw[4] = {0};
    int ref;
    for (ref = 0; ref < hist_total - 10; ref++)
    {
        for (i = 0; i < 4; i++)
        {
            while (acc[i] < ref)
            {
                acc[i] += hist[i][raw[i]];
                raw[i]++;
            }
        }

        if (raw[0] >= white) break;
        if (raw[1] >= white) break;
        if (raw[2] >= white) break;
        if (raw[3] >= white) break;
    }

    for (i = 0; i < 4; i++)
    {
        free(hist[i]); hist[i] = 0;
    }

    /* very crude way to compute median */
    int sorted_bright[4];
    memcpy(sorted_bright, raw, sizeof(sorted_bright));
    {
        int i,j;
        for (i = 0; i < 4; i++)
        {
            for (j = i+1; j < 4; j++)
            {
                if (sorted_bright[i] > sorted_bright[j])
                {
                    double aux = sorted_bright[i];
                    sorted_bright[i] = sorted_bright[j];
                    sorted_bright[j] = aux;
                }
            }
        }
    }
    double median_bright = (sorted_bright[1] + sorted_bright[2]) / 2;

    for (i = 0; i < 4; i++)
        is_bright[i] = raw[i] > median_bright;

    printf("ISO pattern     : %c%c%c%c %s\n", is_bright[0] ? 'B' : 'd', is_bright[1] ? 'B' : 'd', is_bright[2] ? 'B' : 'd', is_bright[3] ? 'B' : 'd', rggb ? "RGGB" : "GBRG");
    
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
    return 1;
}

static int match_histograms(double* corr_ev, int* white_darkened)
{
    /* guess ISO - find the factor and the offset for matching the bright and dark images */
    int black20 = raw_info.black_level;
    int white20 = MIN(raw_info.white_level, *white_darkened);
    int black = black20/16;
    int white = white20/16;

    int w = raw_info.width;
    int h = raw_info.height;

    int x,y;
    int i;
    
    /* build two histograms */
    int hist_size = 65536 * sizeof(int);
    int* hist_lo = malloc(hist_size);
    int* hist_hi = malloc(hist_size);
    memset(hist_lo, 0, hist_size);
    memset(hist_hi, 0, hist_size);
    
    int y0 = MAX(0, raw_info.active_area.y1 - 8) & ~3; /* gibberish above */
    
    /* to simplify things, analyze an identical number of bright and dark lines */
    for (y = y0; y < h/4*4; y++)
    {
        if (BRIGHT_ROW)
        {
            for (x = 0; x < w; x++)
                hist_hi[raw_get_pixel_20to16(x,y)]++;
        }
        else
        {
            for (x = 0; x < w; x++)
                hist_lo[raw_get_pixel_20to16(x,y)]++;
        }
    }
    
    /* compare the two histograms and plot the curve between the two exposures (dark as a function of bright) */
    const int min_pix = 100;                                /* extract a data point every N image pixels */
    int data_size = (w * h / min_pix + 1);                  /* max number of data points */
    int* data_x = malloc(data_size * sizeof(data_x[0]));
    int* data_y = malloc(data_size * sizeof(data_y[0]));
    double* data_w = malloc(data_size * sizeof(data_w[0]));
    int data_num = 0;
    
    int acc_lo = 0;
    int acc_hi = 0;
    int raw_lo = 0;
    int raw_hi = 0;
    int prev_acc_hi = 0;
    
    int hist_total = 0;
    for (i = 0; i < 65536; i++)
        hist_total += hist_hi[i];
    
    for (raw_hi = 0; raw_hi < white; raw_hi++)
    {
        acc_hi += hist_hi[raw_hi];

        while (acc_lo < acc_hi)
        {
            acc_lo += hist_lo[raw_lo];
            raw_lo++;
        }
        
        if (raw_lo >= white)
            break;
        
        if (acc_hi - prev_acc_hi > min_pix)
        {
            if (acc_hi > hist_total * 1 / 100 && acc_hi < hist_total * 99.99 / 100)    /* throw away outliers */
            {
                data_x[data_num] = raw_hi - black;
                data_y[data_num] = raw_lo - black;
                data_w[data_num] = (MAX(0, raw_hi - black + 100));    /* points from higher brightness are cleaner */
                data_num++;
                prev_acc_hi = acc_hi;
            }
        }
    }

    /**
     * plain least squares
     * y = ax + b
     * a = (mean(xy) - mean(x)mean(y)) / (mean(x^2) - mean(x)^2)
     * b = mean(y) - a mean(x)
     */
    
    double mx = 0, my = 0, mxy = 0, mx2 = 0;
    double weight = 0;
    for (i = 0; i < data_num; i++)
    {
        mx += data_x[i] * data_w[i];
        my += data_y[i] * data_w[i];
        mxy += (double)data_x[i] * data_y[i] * data_w[i];
        mx2 += (double)data_x[i] * data_x[i] * data_w[i];
        weight += data_w[i];
    }
    mx /= weight;
    my /= weight;
    mxy /= weight;
    mx2 /= weight;
    double a = (mxy - mx*my) / (mx2 - mx*mx);
    double b = my - a * mx;

    #define BLACK_DELTA_THR 1000

    if (ABS(b) > BLACK_DELTA_THR)
    {
        /* sum ting wong */
        b = 0;
        a = (double) my / mx;
        printf("Black delta looks bad, skipping correction\n");
        goto after_black_correction;
    }

    /* apply the correction */
    double b20 = b * 16;
    for (y = 0; y < h-1; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int p = raw_get_pixel32(x, y);
            if (p == 0) continue;

            if (BRIGHT_ROW)
            {
                /* bright exposure: darken and apply the black offset (fixme: why not half?) */
                p = (p - black20) * a + black20 + b20*a;
            }
            else
            {
                p = p - b20 + b20*a;
            }
            
            /* out of range? mark as bad pixel and interpolate it later */
            if (p < 0 || p > 0xFFFFF) p = 0;
            
            raw_set_pixel20(x, y, p);
        }
    }
    *white_darkened = (white20 - black20 + b20) * a + black20;

after_black_correction:
    
    if (plot_iso_curve)
    {
        printf("Least squares   : y = %f*x + %f\n", a, b);
        FILE* f = fopen("iso-curve.m", "w");

        fprintf(f, "x = [");
        for (i = 0; i < data_num; i++)
            fprintf(f, "%d ", data_x[i]);
        fprintf(f, "];\n");
        
        fprintf(f, "y = [");
        for (i = 0; i < data_num; i++)
            fprintf(f, "%d ",data_y[i]);
        fprintf(f, "];\n");

        fprintf(f, "hl = [");
        for (i = 0; i < 65536; i++)
            fprintf(f, "%d ", hist_lo[i]);
        fprintf(f, "];\n");
        
        fprintf(f, "hh = [");
        for (i = 0; i < 65536; i++)
            fprintf(f, "%d ",hist_hi[i]);
        fprintf(f, "];\n");

        fprintf(f, "a = %f;\n", a);
        fprintf(f, "b = %f;\n", b);

        fprintf(f, "plot(x, y); hold on;\n");
        fprintf(f, "plot(x, y - b, 'g');\n");
        fprintf(f, "plot(x, a * x, 'r');\n");
        fprintf(f, "print -dpng iso-curve.png\n");
        fclose(f);
        
        if(system("octave --persist iso-curve.m"));
    }

    free(hist_lo);
    free(hist_hi);
    free(data_x);
    free(data_y);
    free(data_w);

    double factor = 1/a;
    if (factor < 1.2 || !isfinite(factor))
    {
        printf("Doesn't look like interlaced ISO\n");
        factor = 1;
    }
    
    *corr_ev = log2(factor);

    printf("ISO difference  : %.2f EV (%d)\n", log2(factor), (int)round(factor*100));
    printf("Black delta     : %.2f\n", b/4); /* we want to display black delta for the 14-bit original data, but we have computed it from 16-bit data */
    return 1;
}

static int mean2(int a, int b, int white, int* err)
{
    if (a >= white || b >= white)
    {
        if (err) *err = 10000000;
        return white;
    }
    
    int m = (a + b) / 2;

    if (err)
        *err = ABS(a - b);

    return m;
}

static int mean3(int a, int b, int c, int white, int* err)
{
    int m = (a + b + c) / 3;

    if (err)
        *err = MAX(MAX(ABS(a - m), ABS(b - m)), ABS(c - m));

    if (a >= white || b >= white || c >= white)
        return MAX(m, white);

    return m;
}

/* various chroma smooth filters */
/* (trick to avoid duplicate code) */

#define CHROMA_SMOOTH_2X2
#include "chroma_smooth.c"
#undef CHROMA_SMOOTH_2X2

#define CHROMA_SMOOTH_3X3
#include "chroma_smooth.c"
#undef CHROMA_SMOOTH_3X3

#define CHROMA_SMOOTH_5X5
#include "chroma_smooth.c"
#undef CHROMA_SMOOTH_5X5

static void chroma_smooth(uint32_t * inp, uint32_t * out, int* raw2ev, int* ev2raw)
{
    switch (chroma_smooth_method)
    {
        case 2:
            chroma_smooth_2x2(inp, out, raw2ev, ev2raw);
            break;
        case 3:
            chroma_smooth_3x3(inp, out, raw2ev, ev2raw);
            break;
        case 5:
            chroma_smooth_5x5(inp, out, raw2ev, ev2raw);
            break;
    }
}

static inline int FC(int row, int col)
{
    if ((row%2) == 0 && (col%2) == 0)
        return 0;  /* red */
    else if ((row%2) == 1 && (col%2) == 1)
        return 2;  /* blue */
    else
        return 1;  /* green */
}

static void find_and_fix_bad_pixels(int dark_noise, int bright_noise, int* raw2ev, int* ev2raw)
{
    int w = raw_info.width;
    int h = raw_info.height;
    
    int black = raw_info.black_level;
    
    printf("Looking for hot/cold pixels...\n");

    /* hot pixel map */
    uint32_t* hotpixel = malloc(w * h * sizeof(uint32_t));
    memset(hotpixel, 0, w * h * sizeof(uint32_t));

    int hot_pixels = 0;
    int cold_pixels = 0;
    int x,y;
    for (y = 6; y < h-6; y ++)
    {
        for (x = 6; x < w-6; x ++)
        {
            int p = raw_get_pixel20(x, y);
            
            int is_hot = 0;
            int is_cold = 0;

            /* really dark pixels (way below the black level) are probably noise */
            is_cold = (p < black - dark_noise*8);

            /* we don't have no hot pixels on the bright exposure */
            /* but we may have cold pixels */
            if (!BRIGHT_ROW || is_cold)
            {
                /* let's look at the neighbours: is this pixel clearly brigher? (isolated) */
                int neighbours[100];
                int k = 0;
                int i,j;
                int fc0 = FC(x, y);
                int b0 = is_bright[y%4];
                for (i = -4; i <= 4; i++)
                {
                    for (j = -4; j <= 4; j++)
                    {
                        if (i == 0 && j == 0)
                            continue;

                        /* only look at pixels of the same brightness */
                        if (is_bright[(y+i)%4] != b0)
                            continue;
                        
                        /* only look at pixels of the same color */
                        if (FC(x+j, y+i) != fc0)
                            continue;
                        
                        int p = raw_get_pixel20(x+j, y+i);
                        neighbours[k++] = -p;
                    }
                }
                
                int max = -kth_smallest_int(neighbours, k, 1);
                is_hot = (raw2ev[p] - raw2ev[max] > EV_RESOLUTION) && (max > black + 8*dark_noise);
                
                if (fix_bad_pixels == 2)    /* aggressive */
                {
                    int second_max = -kth_smallest_int(neighbours, k, 2);
                    is_hot = ((raw2ev[p] - raw2ev[max] > EV_RESOLUTION/4) && (max > black + 8*dark_noise))
                          || (raw2ev[p] - raw2ev[second_max] > EV_RESOLUTION/2);
                }

                if (is_hot)
                {
                    hot_pixels++;
                    hotpixel[x + y*w] = -kth_smallest_int(neighbours, k, 2);
                }
                
                if (is_cold)
                {
                    cold_pixels++;
                    hotpixel[x + y*w] = -median_int_wirth(neighbours, k);
                }
            }
        }
    }

    /* apply the correction */
    for (y = 0; y < h; y ++)
        for (x = 0; x < w; x ++)
            if (hotpixel[x + y*w])
                raw_set_pixel20(x, y, debug_bad_pixels ? black : hotpixel[x + y*w]);

    if (hot_pixels)
        printf("Hot pixels      : %d\n", hot_pixels);

    if (cold_pixels)
        printf("Cold pixels     : %d\n", cold_pixels);
    
    free(hotpixel);
}

static int hdr_interpolate()
{
    int x, y;
    int w = raw_info.width;
    int h = raw_info.height;

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

    if (!identify_bright_and_dark_fields(rggb))
        return 0;

    int ret = 1;

    /* will use 20-bit processing and 16-bit output, instead of 14 */
    raw_info.black_level *= 64;
    raw_info.white_level *= 64;
    
    int black = raw_info.black_level;
    int white = raw_info.white_level;

    int white_bright = white;
    white_detect(&white, &white_bright);
    white *= 64;
    white_bright *= 64;
    raw_info.white_level = white;

    /* for fast EV - raw conversion */
    static int raw2ev[1<<20];   /* EV x EV_RESOLUTION */
    static int ev2raw_0[24*EV_RESOLUTION];
    
    /* handle sub-black values (negative EV) */
    int* ev2raw = ev2raw_0 + 10*EV_RESOLUTION;

    int i;
    for (i = 0; i < 1<<20; i++)
    {
        double signal = MAX(i/64.0 - black/64.0, -1023);
        if (signal > 0)
            raw2ev[i] = (int)round(log2(1+signal) * EV_RESOLUTION);
        else
            raw2ev[i] = -(int)round(log2(1-signal) * EV_RESOLUTION);
    }

    for (i = -10*EV_RESOLUTION; i < 0; i++)
    {
        ev2raw[i] = COERCE(black+64 - round(64*pow(2, ((double)-i/EV_RESOLUTION))), 0, black);
    }

    for (i = 0; i < 14*EV_RESOLUTION; i++)
    {
        ev2raw[i] = COERCE(black-64 + round(64*pow(2, ((double)i/EV_RESOLUTION))), black, (1<<20)-1);
        
        if (i >= raw2ev[white])
        {
            ev2raw[i] = MAX(ev2raw[i], white);
        }
    }
    
    /* keep "bad" pixels, if any */
    ev2raw[raw2ev[0]] = 0;
    ev2raw[raw2ev[0]] = 0;
    
    /* check raw <--> ev conversion */
    //~ printf("%d %d %d %d %d %d %d *%d* %d %d %d %d %d\n", raw2ev[0],         raw2ev[16000],         raw2ev[32000],         raw2ev[131068],         raw2ev[131069],         raw2ev[131070],         raw2ev[131071],         raw2ev[131072],         raw2ev[131073],         raw2ev[131074],         raw2ev[131075],         raw2ev[131076],         raw2ev[132000]);
    //~ printf("%d %d %d %d %d %d %d *%d* %d %d %d %d %d\n", ev2raw[raw2ev[0]], ev2raw[raw2ev[16000]], ev2raw[raw2ev[32000]], ev2raw[raw2ev[131068]], ev2raw[raw2ev[131069]], ev2raw[raw2ev[131070]], ev2raw[raw2ev[131071]], ev2raw[raw2ev[131072]], ev2raw[raw2ev[131073]], ev2raw[raw2ev[131074]], ev2raw[raw2ev[131075]], ev2raw[raw2ev[131076]], ev2raw[raw2ev[132000]]);

    double noise_std[4];
    double noise_avg;
    for (y = 0; y < 4; y++)
        compute_black_noise(8, raw_info.active_area.x1 - 8, raw_info.active_area.y1/4*4 + 20 + y, raw_info.active_area.y2 - 20, 1, 4, &noise_avg, &noise_std[y], raw_get_pixel16);

    printf("Noise levels    : %.02f %.02f %.02f %.02f (14-bit)\n", noise_std[0], noise_std[1], noise_std[2], noise_std[3]);
    double dark_noise = MIN(MIN(noise_std[0], noise_std[1]), MIN(noise_std[2], noise_std[3]));
    double bright_noise = MAX(MAX(noise_std[0], noise_std[1]), MAX(noise_std[2], noise_std[3]));
    double dark_noise_ev = log2(dark_noise);
    double bright_noise_ev = log2(bright_noise);

    if (0)
    {
        /* dump the bright image without interpolation */
        /* (well, use nearest neighbour, which is an interpolation in the same way as black and white are colors) */
        for (y = 0; y < h; y ++)
            for (x = 0; x < w; x ++)
                raw_set_pixel16(x, y, raw_get_pixel_14to16(x, !BRIGHT_ROW ? y : y+2));
        raw_info.black_level /= 16;
        raw_info.white_level /= 16;
        goto end;
    }

    /* promote from 14 to 20 bits (original raw buffer holds 14-bit values stored as uint16_t) */
    void* raw_buffer_16 = raw_info.buffer;
    uint32_t * raw_buffer_32 = malloc(w * h * sizeof(raw_buffer_32[0]));
    
    for (y = 0; y < h; y ++)
        for (x = 0; x < w; x ++)
            raw_buffer_32[x + y*w] = raw_get_pixel_14to20(x, y);

    raw_info.buffer = raw_buffer_32;
    for (y = 0; y < h; y ++)
        for (x = 0; x < w; x ++)
            raw_set_pixel32(x, y, raw_buffer_32[x + y*w]);

    /* we have now switched to 20-bit, update noise numbers */
    dark_noise *= 64;
    bright_noise *= 64;
    dark_noise_ev += 6;
    bright_noise_ev += 6;

    /* dark and bright exposures, interpolated */
    uint32_t* dark   = malloc(w * h * sizeof(uint32_t));
    uint32_t* bright = malloc(w * h * sizeof(uint32_t));
    memset(dark, 0, w * h * sizeof(uint32_t));
    memset(bright, 0, w * h * sizeof(uint32_t));
    
    /* fullres image (minimizes aliasing) */
    uint32_t* fullres = malloc(w * h * sizeof(uint32_t));
    memset(fullres, 0, w * h * sizeof(uint32_t));
    uint32_t* fullres_smooth = fullres;

    /* halfres image (minimizes noise and banding) */
    uint32_t* halfres = malloc(w * h * sizeof(uint32_t));
    memset(halfres, 0, w * h * sizeof(uint32_t));
    uint32_t* halfres_smooth = halfres;
    
    /* overexposure map */
    uint16_t* overexposed = 0;

    uint16_t* alias_map = malloc(w * h * sizeof(uint16_t));
    memset(alias_map, 0, w * h * sizeof(uint16_t));

    /* fullres mixing curve */
    static double fullres_curve[1<<20];
    
    const double fullres_start = 4;
    const double fullres_transition = 3;
    const double fullres_thr = 0.8;
    
    for (i = 0; i < (1<<20); i++)
    {
        double ev2 = log2(MAX(i/64.0 - black/64.0, 1));
        double c2 = -cos(COERCE(ev2 - fullres_start, 0, fullres_transition)*M_PI/fullres_transition);
        double f = (c2+1) / 2;
        fullres_curve[i] = f;
    }
    

    if (plot_fullres_curve)
    {
        FILE* f = fopen("fullres-curve.m", "w");
        fprintf(f, "x = 0:65535; \n");

        fprintf(f, "ev = [");
        for (i = 0; i < 65536; i++)
            fprintf(f, "%f ", log2(MAX(i/4.0 - black/64.0, 1)));
        fprintf(f, "];\n");

        fprintf(f, "f = [");
        for (i = 0; i < 65536; i++)
            fprintf(f, "%f ", fullres_curve[i*16]);
        fprintf(f, "];\n");
        
        fprintf(f, "plot(ev, f);\n");
        fprintf(f, "print -dpng fullres-curve.png\n");
        fclose(f);
        
        if(system("octave --persist fullres-curve.m"));
    }

    //~ printf("Histogram matching...\n");
    /* estimate ISO difference between bright and dark exposures */
    double corr_ev = 0;
    int white_darkened = white_bright;
    int ok = match_histograms(&corr_ev, &white_darkened);
    if (!ok) goto err;

    /* estimate dynamic range */
    double lowiso_dr = log2(white - black) - dark_noise_ev;
    double highiso_dr = log2(white_bright - black) - bright_noise_ev;
    printf("Dynamic range   : %.02f (+) %.02f => %.02f EV (in theory)\n", lowiso_dr, highiso_dr, highiso_dr + corr_ev);

    /* correction factor for the bright exposure, which was just darkened */
    double corr = pow(2, corr_ev);
    
    /* update bright noise measurements, so they can be compared after scaling */
    bright_noise /= corr;
    bright_noise_ev -= corr_ev;
    
    if (fix_bad_pixels)
    {
        /* best done before interpolation */
        find_and_fix_bad_pixels(dark_noise, bright_noise, raw2ev, ev2raw);
    }

    if (interp_method == 0) /* amaze-edge */
    {
        int* squeezed = malloc(h * sizeof(squeezed));
        memset(squeezed, 0, h * sizeof(squeezed));
 
        float** rawData = malloc(h * sizeof(rawData[0]));
        float** red     = malloc(h * sizeof(red[0]));
        float** green   = malloc(h * sizeof(green[0]));
        float** blue    = malloc(h * sizeof(blue[0]));
        
        for (i = 0; i < h; i++)
        {
            rawData[i] =   malloc(w * sizeof(rawData[0][0]));
            memset(rawData[i], 0, w * sizeof(rawData[0][0]));
            red[i]     = malloc(w * sizeof(red[0][0]));
            green[i]   = malloc(w * sizeof(green[0][0]));
            blue[i]    = malloc(w * sizeof(blue[0][0]));
        }
        
        /* squeeze the dark image by deleting fields from the bright exposure */
        int yh = -1;
        for (y = 0; y < h; y ++)
        {
            if (BRIGHT_ROW)
                continue;
            
            if (yh < 0) /* make sure we start at the same parity (RGGB cell) */
                yh = y;
            
            for (x = 0; x < w; x++)
            {
                int p = raw_get_pixel32(x, y);
                
                if (x%2 != y%2) /* divide green channel by 2 to approximate the final WB better */
                    p = (p - black) / 2 + black;
                
                rawData[yh][x] = p;
            }
            
            squeezed[y] = yh;
            
            yh++;
        }

        /* now the same for the bright exposure */
        yh = -1;
        for (y = 0; y < h; y ++)
        {
            if (!BRIGHT_ROW)
                continue;

            if (yh < 0) /* make sure we start with the same parity (RGGB cell) */
                yh = h/4*2 + y;
            
            for (x = 0; x < w; x++)
            {
                int p = raw_get_pixel32(x, y);
                
                if (x%2 != y%2) /* divide green channel by 2 to approximate the final WB better */
                    p = (p - black) / 2 + black;
                
                rawData[yh][x] = p;
            }
            
            squeezed[y] = yh;
            
            yh++;
            if (yh >= h) break; /* just in case */
        }

        if (debug_amaze)
        {
            for (y = 0; y < h; y ++)
                for (x = 0; x < w; x ++)
                    raw_set_pixel_20to16(x, y, rawData[y][x]);
            save_debug_dng("amaze-input.dng");
        }

        void amaze_demosaic_RT(
            float** rawData,    /* holds preprocessed pixel values, rawData[i][j] corresponds to the ith row and jth column */
            float** red,        /* the interpolated red plane */
            float** green,      /* the interpolated green plane */
            float** blue,       /* the interpolated blue plane */
            int winx, int winy, /* crop window for demosaicing */
            int winw, int winh
        );

        amaze_demosaic_RT(rawData, red, green, blue, 0, 0, w, h);

        /* undo green channel scaling */
        for (y = 0; y < h; y ++)
            for (x = 0; x < w; x ++)
                green[y][x] = COERCE((green[y][x] - black) * 2 + black, 0, 0xFFFFF);

        if (debug_amaze)
        {
            for (y = 0; y < h; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, red[y][x]);
            save_debug_dng("amaze-red.dng");

            for (y = 0; y < h; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, green[y][x]);
            save_debug_dng("amaze-green.dng");

            for (y = 0; y < h; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, blue[y][x]);
            save_debug_dng("amaze-blue.dng");
            
            /* the above operations were destructive, so we stop here */
            printf("debug exit\n");
            exit(1);
        }

        printf("Edge-directed interpolation...\n");
        
        //~ printf("Grayscale...\n");
        /* convert to grayscale and de-squeeze for easier processing */
        uint32_t * gray = malloc(w * h * sizeof(gray[0]));
        for (y = 0; y < h; y ++)
            for (x = 0; x < w; x ++)
                gray[x + y*w] = green[squeezed[y]][x]/2 + red[squeezed[y]][x]/4 + blue[squeezed[y]][x]/4;

        #if 0
        for (y = 0; y < h; y ++)
            for (x = 2; x < w-2; x ++)
                raw_set_pixel_20to16(x, y, gray[x + y*w]);
        save_debug_dng("edge-gray.dng");
        exit(1);
        #endif

        /* define edge directions for interpolation */
        struct xy { int x; int y; };
        const struct
        {
            struct xy ack;      /* verification pixel near a */
            struct xy a;        /* interpolation pixel from the nearby line: normally (0,s) but also (1,s) or (-1,s) */
            struct xy b;        /* interpolation pixel from the other line: normally (0,-2s) but also (1,-2s), (-1,-2s), (2,-2s) or (-2,-2s) */
            struct xy bck;      /* verification pixel near b */
        }
        edge_directions[] = {       /* note: all y coords should be multiplied by s */
            //~ { {-6,2}, {-3,1}, { 6,-2}, { 9,-3} },     /* almost horizontal (little or no improvement) */
            { {-4,2}, {-2,1}, { 4,-2}, { 6,-3} },
            { {-3,2}, {-1,1}, { 3,-2}, { 4,-3} },
            { {-2,2}, {-1,1}, { 2,-2}, { 3,-3} },     /* 45-degree diagonal */
            { {-1,2}, {-1,1}, { 1,-2}, { 2,-3} },
            { {-1,2}, { 0,1}, { 1,-2}, { 1,-3} },
            { { 0,2}, { 0,1}, { 0,-2}, { 0,-3} },     /* vertical, preferred; no extra confirmations needed */
            { { 1,2}, { 0,1}, {-1,-2}, {-1,-3} },
            { { 1,2}, { 1,1}, {-1,-2}, {-2,-3} },
            { { 2,2}, { 1,1}, {-2,-2}, {-3,-3} },     /* 45-degree diagonal */
            { { 3,2}, { 1,1}, {-3,-2}, {-4,-3} },
            { { 4,2}, { 2,1}, {-4,-2}, {-6,-3} },
            //~ { { 6,2}, { 3,1}, {-6,-2}, {-9,-3} },     /* almost horizontal */
        };

        uint8_t* edge_direction = malloc(w * h * sizeof(edge_direction[0]));
        int d0 = COUNT(edge_directions)/2;
        memset(edge_direction, d0, w * h * sizeof(edge_direction[0]));

        //~ printf("Cross-correlation...\n");
        int semi_overexposed = 0;
        int not_overexposed = 0;
        int deep_shadow = 0;
        int not_shadow = 0;
        
        for (y = 5; y < h-5; y ++)
        {
            int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;    /* points to the closest row having different exposure */
            for (x = 5; x < w-5; x ++)
            {
                int d;
                int e_best = INT_MAX;
                int d_best = d0;
                int dmin = 0;
                int dmax = COUNT(edge_directions)-1;
                int search_area = 5;

                /* only use high accuracy on the dark exposure where the bright ISO is overexposed */
                if (!BRIGHT_ROW)
                {
                    /* interpolating bright exposure */
                    if (fullres_curve[raw_get_pixel32(x, y)] > fullres_thr && !debug_edge)
                    {
                        /* no high accuracy needed, just interpolate vertically */
                        not_shadow++;
                        dmin = d0;
                        dmax = d0;
                    }
                    else
                    {
                        /* deep shadows, unlikely to use fullres, so we need a good interpolation */
                        deep_shadow++;
                    }
                }
                else if (raw_get_pixel32(x, y) < white_darkened && !debug_edge)
                {
                    /* interpolating dark exposure, but we also have good data from the bright one */
                    not_overexposed++;
                    dmin = d0;
                    dmax = d0;
                }
                else
                {
                    /* interpolating dark exposure, but the bright one is clipped */
                    semi_overexposed++;
                }

                if (dmin == dmax)
                {
                    d_best = dmin;
                }
                else
                {
                    for (d = dmin; d <= dmax; d++)
                    {
                        int e = 0;
                        int j;
                        for (j = -search_area; j <= search_area; j++)
                        {
                            int dx1 = edge_directions[d].ack.x + j;
                            int dy1 = edge_directions[d].ack.y * s;
                            int p1 = raw2ev[gray[x+dx1 + (y+dy1)*w]];
                            int dx2 = edge_directions[d].a.x + j;
                            int dy2 = edge_directions[d].a.y * s;
                            int p2 = raw2ev[gray[x+dx2 + (y+dy2)*w]];
                            int dx3 = edge_directions[d].b.x + j;
                            int dy3 = edge_directions[d].b.y * s;
                            int p3 = raw2ev[gray[x+dx3 + (y+dy3)*w]];
                            int dx4 = edge_directions[d].bck.x + j;
                            int dy4 = edge_directions[d].bck.y * s;
                            int p4 = raw2ev[gray[x+dx4 + (y+dy4)*w]];
                            e += ABS(p1-p2) + ABS(p2-p3) + ABS(p3-p4);
                        }
                        
                        /* add a small penalty for diagonal directions */
                        /* (the improvement should be significant in order to choose one of these) */
                        e += ABS(d - d0) * EV_RESOLUTION/8;
                        
                        if (e < e_best)
                        {
                            e_best = e;
                            d_best = d;
                        }
                    }
                }
                
                edge_direction[x + y*w] = d_best;
            }
        }

        if (!debug_edge)
        {
            printf("Semi-overexposed: %.02f%%\n", semi_overexposed * 100.0 / (semi_overexposed + not_overexposed));
            printf("Deep shadows    : %.02f%%\n", deep_shadow * 100.0 / (deep_shadow + not_shadow));
        }

        /* burn the interpolation directions into a test image */
        if (debug_edge)
        {
            for (y = 4; y < h-4; y += 10)
            {
                /* only show bright rows (interpolated from dark ones) */
                while (!BRIGHT_ROW) y++;
                
                int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;    /* points to the closest row having different exposure */
                for (x = 4; x < w-4; x += 10)
                {
                    gray[x + y*w] = black;

                    int dir = edge_direction[x + y*w];

                    int dx = edge_directions[dir].a.x;
                    int dy = edge_directions[dir].a.y * s;
                    gray[x+dx + (y+dy)*w] = black;

                    dx = edge_directions[dir].b.x;
                    dy = edge_directions[dir].b.y * s;
                    gray[x+dx + (y+dy)*w] = black;

                    dx = edge_directions[dir].ack.x;
                    dy = edge_directions[dir].ack.y * s;
                    gray[x+dx + (y+dy)*w] = black;

                    dx = edge_directions[dir].bck.x;
                    dy = edge_directions[dir].bck.y * s;
                    gray[x+dx + (y+dy)*w] = black;
                }
            }

            for (y = 0; y < h; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, gray[x + y*w]);
            save_debug_dng("edges.dng");
            if(system("dcraw -d -r 1 1 1 1 edges.dng"));
            /* best viewed at 400% with nearest neighbour interpolation (no filtering) */

            for (y = 0; y < h; y ++)
            {
                for (x = 2; x < w-2; x ++)
                {
                    int dir = edge_direction[x + y*w];
                    if (y%2) dir = COUNT(edge_directions)-1-dir;
                    raw_set_pixel16(x, y, ev2raw[dir * EV_RESOLUTION]);
                }
            }
            save_debug_dng("edge-map.dng");
            if(system("dcraw -d -r 1 1 1 1 edge-map.dng"));
            printf("debug exit\n");
            exit(1);
        }
        
        //~ printf("Actual interpolation...\n");

        for (y = 2; y < h-2; y ++)
        {
            uint32_t* native = BRIGHT_ROW ? bright : dark;
            uint32_t* interp = BRIGHT_ROW ? dark : bright;
            int is_rg = (y % 2 == 0); /* RG or GB? */
            int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;    /* points to the closest row having different exposure */

            //~ printf("Interpolating %s line %d from [near] %d (squeezed %d) and [far] %d (squeezed %d)\n", BRIGHT_ROW ? "BRIGHT" : "DARK", y, y+s, yh_near, y-2*s, yh_far);
            
            for (x = 2; x < w-2; x += 2)
            {
                int k;
                for (k = 0; k < 2; k++, x++)
                {
                    float** plane = is_rg ? (x%2 == 0 ? red   : green)
                                          : (x%2 == 0 ? green : blue );

                    int dir = edge_direction[x + y*w];
                    
                    int dxa = edge_directions[dir].a.x;
                    int dya = edge_directions[dir].a.y * s;
                    int pa = COERCE((int)plane[squeezed[y+dya]][x+dxa], 0, 0xFFFFF);
                    int dxb = edge_directions[dir].b.x;
                    int dyb = edge_directions[dir].b.y * s;
                    int pb = COERCE((int)plane[squeezed[y+dyb]][x+dxb], 0, 0xFFFFF);
                    int pi = (raw2ev[pa] + raw2ev[pa] + raw2ev[pb]) / 3;
                    
                    interp[x   + y * w] = ev2raw[pi];
                    native[x   + y * w] = raw_get_pixel32(x, y);
                }
                x -= 2;
            }
        }

        for (i = 0; i < h; i++)
        {
            free(rawData[i]);
            free(red[i]);
            free(green[i]);
            free(blue[i]);
        }
        
        free(squeezed); squeezed = 0;
        free(rawData); rawData = 0;
        free(red); red = 0;
        free(green); green = 0;
        free(blue); blue = 0;
        free(gray); gray = 0;
        free(edge_direction);
    }
    else /* mean23 */
    {
        printf("Interpolation   : mean23\n");
        for (y = 2; y < h-2; y ++)
        {
            uint32_t* native = BRIGHT_ROW ? bright : dark;
            uint32_t* interp = BRIGHT_ROW ? dark : bright;
            int is_rg = (y % 2 == 0); /* RG or GB? */
            int white = !BRIGHT_ROW ? white_darkened : raw_info.white_level;
            
            for (x = 2; x < w-3; x += 2)
            {
            
                /* red/blue: interpolate from (x,y+2) and (x,y-2) */
                /* green: interpolate from (x+1,y+1),(x-1,y+1),(x,y-2) or (x+1,y-1),(x-1,y-1),(x,y+2), whichever has the correct brightness */
                
                int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;
                
                if (is_rg)
                {
                    int ra = raw_get_pixel32(x, y-2);
                    int rb = raw_get_pixel32(x, y+2);
                    int ri = mean2(raw2ev[ra], raw2ev[rb], raw2ev[white], 0);
                    
                    int ga = raw_get_pixel32(x+1+1, y+s);
                    int gb = raw_get_pixel32(x+1-1, y+s);
                    int gc = raw_get_pixel32(x+1, y-2*s);
                    int gi = mean3(raw2ev[ga], raw2ev[gb], raw2ev[gc], raw2ev[white], 0);

                    interp[x   + y * w] = ev2raw[ri];
                    interp[x+1 + y * w] = ev2raw[gi];
                }
                else
                {
                    int ba = raw_get_pixel32(x+1  , y-2);
                    int bb = raw_get_pixel32(x+1  , y+2);
                    int bi = mean2(raw2ev[ba], raw2ev[bb], raw2ev[white], 0);

                    int ga = raw_get_pixel32(x+1, y+s);
                    int gb = raw_get_pixel32(x-1, y+s);
                    int gc = raw_get_pixel32(x, y-2*s);
                    int gi = mean3(raw2ev[ga], raw2ev[gb], raw2ev[gc], raw2ev[white], 0);

                    interp[x   + y * w] = ev2raw[gi];
                    interp[x+1 + y * w] = ev2raw[bi];
                }

                native[x   + y * w] = raw_get_pixel32(x, y);
                native[x+1 + y * w] = raw_get_pixel32(x+1, y);
            }
        }
    }

    /* border interpolation */
    for (y = 0; y < 3; y ++)
    {
        uint32_t* native = BRIGHT_ROW ? bright : dark;
        uint32_t* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x, y+2);
            native[x + y * w] = raw_get_pixel32(x, y);
        }
    }

    for (y = h-2; y < h; y ++)
    {
        uint32_t* native = BRIGHT_ROW ? bright : dark;
        uint32_t* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x, y-2);
            native[x + y * w] = raw_get_pixel32(x, y);
        }
    }

    for (y = 2; y < h; y ++)
    {
        uint32_t* native = BRIGHT_ROW ? bright : dark;
        uint32_t* interp = BRIGHT_ROW ? dark : bright;
        
        for (x = 0; x < 2; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x, y-2);
            native[x + y * w] = raw_get_pixel32(x, y);
        }

        for (x = w-3; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x-2, y-2);
            native[x + y * w] = raw_get_pixel32(x-2, y);
        }
    }
    
    if (use_stripe_fix)
    {
        printf("Horizontal stripe fix...\n");
        int * delta[14];
        int delta_num[14];
        for (i = 0; i < 14; i++)
        {
            delta[i] = malloc(w * sizeof(delta[0]));
        }

        /* adjust dark lines to match the bright ones */
        for (y = 0; y < h; y ++)
        {
            for (i = 0; i < 14; i++)
            {
                delta_num[i] = 0;
            }

            /* apply a constant offset to each stop (except overexposed areas) */
            for (x = 0; x < w; x ++)
            {
                int b = bright[x + y*w];
                int d = dark[x + y*w];
                if (b < white_darkened && d < white)
                {
                    int stop = COERCE(raw2ev[b] / EV_RESOLUTION, 0, 13);
                    delta[stop][delta_num[stop]++] = b - d;
                }
            }

            /* compute median difference for each stop */
            int med_delta[14];
            for (i = 0; i < 14; i++)
            {
                if (delta_num[i] > 0)
                {
                    /* enough data points? */
                    med_delta[i] = median_int_wirth(delta[i], delta_num[i]);
                    
                    /* avoid large corrections (they are probably outliers) */
                    if (ABS(med_delta[i]) > 200*16) med_delta[i] = 0;
                }
                else
                {
                    /* not enough data points; will extrapolate from neighbours */
                    med_delta[i] = delta_num[i] = 0;
                }
            }

            /* extrapolate the measurements to neighbour stops */
            for (i = 0; i < 14; i++)
            {
                if (delta_num[i] == 0)
                {
                    int acc = 0;
                    int num = 0;
                    if (i < 13 && delta_num[i+1]) { acc += med_delta[i+1]; num++; }
                    if (i > 0 && delta_num[i-1]) { acc += med_delta[i-1]; num++; }
                    if (num) med_delta[i] = acc / num;
                }
                
                //~ printf("%d(%d) ", med_delta[i], delta_num[i]);
            }
            //~ printf("\n");
            
            for (x = 0; x < w; x ++)
            {
                int b = bright[x + y*w];
                int d = dark[x + y*w];

                if (b < white_darkened && d < white)
                {
                    /* linear interpolation */
                    int b = bright[x + y*w];
                    double stop = COERCE((double)(raw2ev[b] - EV_RESOLUTION/2) / EV_RESOLUTION, 0.01, 13-0.01);
                    int stop1 = floor(stop);
                    int stop2 = ceil(stop);
                    double k = stop - stop1;
                    //~ int stop = COERCE(raw2ev[b] / EV_RESOLUTION, 0, 13);
                    dark[x + y*w] += med_delta[stop1] * (1-k) + med_delta[stop2] * k;
                    
                    //~ if (y == raw_info.active_area.y1 + 2986)
                        //~ printf("%d %d %d %f\n", b, stop1, stop2, k);
                }
            }
        }
        for (i = 0; i < 14; i++)
        {
            free(delta[i]);
        }
    }

    /* reconstruct a full-resolution image (discard interpolated fields whenever possible) */
    /* this has full detail and lowest possible aliasing, but it has high shadow noise and color artifacts when high-iso starts clipping */
    if (use_fullres)
    {
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
    
    printf("ISO overlap     : %.1f EV (approx)\n", overlap);
    
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
    double max_ev = log2(white/64 - black/64);
    static double mix_curve[1<<20];
    
    for (i = 0; i < 1<<20; i++)
    {
        double ev = log2(MAX(i/64.0 - black/64.0, 1)) + corr_ev;
        double c = -cos(MAX(MIN(ev-(max_ev-overlap),overlap),0)*M_PI/overlap);
        double k = (c+1) / 2;
        mix_curve[i] = k;
    }

    if (plot_mix_curve)
    {
        FILE* f = fopen("mix-curve.m", "w");
        fprintf(f, "x = 0:65535; \n");

        fprintf(f, "ev = [");
        for (i = 0; i < 65536; i++)
            fprintf(f, "%f ", log2(MAX(i/4.0 - black/4.0, 1)));
        fprintf(f, "];\n");
        
        fprintf(f, "k = [");
        for (i = 0; i < 65536; i++)
            fprintf(f, "%f ", mix_curve[i*16]);
        fprintf(f, "];\n");
        
        fprintf(f, "plot(ev, k);\n");
        fprintf(f, "print -dpng mix-curve.png\n");
        fclose(f);
        
        if(system("octave --persist mix-curve.m"));
    }
    
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            /* bright and dark source pixels  */
            /* they may be real or interpolated */
            /* they both have the same brightness (they were adjusted before this loop), so we are ready to mix them */ 
            int b = bright[x + y*w];
            int d = dark[x + y*w];

            /* go from linear to EV space */
            int bev = raw2ev[b];
            int dev = raw2ev[d];

            /* blending factor */
            double k = COERCE(mix_curve[b & 0xFFFFF], 0, 1);
            
            /* mix bright and dark exposures */
            int mixed = bev * (1-k) + dev * k;
            halfres[x + y*w] = ev2raw[mixed];
        }
    }

    if (chroma_smooth_method)
    {
        printf("Chroma smoothing...\n");

        if (use_fullres)
        {
            fullres_smooth = malloc(w * h * sizeof(uint32_t));
            memcpy(fullres_smooth, fullres, w * h * sizeof(uint32_t));
        }

        halfres_smooth = malloc(w * h * sizeof(uint32_t));
        memcpy(halfres_smooth, halfres, w * h * sizeof(uint32_t));

        chroma_smooth(fullres, fullres_smooth, raw2ev, ev2raw);
        chroma_smooth(halfres, halfres_smooth, raw2ev, ev2raw);
    }

    if (debug_blend)
    {
        raw_info.buffer = raw_buffer_16;
        for (y = 3; y < h-2; y ++)
            for (x = 2; x < w-2; x ++)
                raw_set_pixel_20to16(x, y, raw_buffer_32[x + y*w]);
        save_debug_dng("normal.dng");
        raw_info.buffer = raw_buffer_32;

        for (y = 3; y < h-2; y ++)
            for (x = 2; x < w-2; x ++)
                raw_set_pixel_20to16(x, y, bright[x + y*w]);
        save_debug_dng("bright.dng");

        for (y = 3; y < h-2; y ++)
            for (x = 2; x < w-2; x ++)
                raw_set_pixel_20to16(x, y, dark[x + y*w]);
        save_debug_dng("dark.dng");

        if (use_fullres)
        {
            for (y = 3; y < h-2; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, fullres[x + y*w]);
            save_debug_dng("fullres.dng");
        }

        for (y = 3; y < h-2; y ++)
            for (x = 2; x < w-2; x ++)
                raw_set_pixel_20to16(x, y, halfres[x + y*w]);
        save_debug_dng("halfres.dng");

        if (chroma_smooth_method)
        {
            if (use_fullres)
            {
                for (y = 3; y < h-2; y ++)
                    for (x = 2; x < w-2; x ++)
                        raw_set_pixel_20to16(x, y, fullres_smooth[x + y*w]);
                save_debug_dng("fullres_smooth.dng");
            }

            for (y = 3; y < h-2; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, halfres_smooth[x + y*w]);
            save_debug_dng("halfres_smooth.dng");
        }
    }


    /* trial and error - too high = aliasing, too low = noisy */
    int ALIAS_MAP_MAX = 15000;
    
    if (use_alias_map)
    {
        printf("Building alias map...\n");

        uint16_t* alias_aux = malloc(w * h * sizeof(uint16_t));
        
        /* build the aliasing maps (where it's likely to get aliasing) */
        /* do this by comparing fullres and halfres images */
        /* if the difference is small, we'll prefer halfres for less noise, otherwise fullres for less aliasing */
        for (y = 0; y < h; y ++)
        {
            for (x = 0; x < w; x ++)
            {
                /* do not compute alias map where we'll use fullres detail anyway */
                if (fullres_curve[bright[x + y*w]] > fullres_thr)
                    continue;

                int f = fullres_smooth[x + y*w];
                int h = halfres_smooth[x + y*w];
                int fe = raw2ev[f];
                int he = raw2ev[h];
                int e_lin = ABS(f - h); /* error in linear space, for shadows (downweights noise) */
                e_lin = MAX(e_lin - dark_noise*3/2, 0);
                int e_log = ABS(fe - he); /* error in EV space, for highlights (highly sensitive to noise) */
                alias_map[x + y*w] = MIN(MIN(e_lin/2, e_log/16), 65530);
            }
        }

        if (debug_alias)
        {
            for (y = 3; y < h-2; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, ev2raw[COERCE(alias_map[x + y*w] * 1024, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]);
            save_debug_dng("alias.dng");
        }

        memcpy(alias_aux, alias_map, w * h * sizeof(uint16_t));

        printf("Filtering alias map...\n");
        for (y = 6; y < h-6; y ++)
        {
            for (x = 6; x < w-6; x ++)
            {
                /* do not compute alias map where we'll use fullres detail anyway */
                if (fullres_curve[bright[x + y*w]] > fullres_thr)
                    continue;
                
                /* use 5th max (out of 37) to filter isolated pixels */
                
                int neighbours[] = {
                                                                              -alias_map[x-2 + (y-6) * w], -alias_map[x+0 + (y-6) * w], -alias_map[x+2 + (y-6) * w],
                                                 -alias_map[x-4 + (y-4) * w], -alias_map[x-2 + (y-4) * w], -alias_map[x+0 + (y-4) * w], -alias_map[x+2 + (y-4) * w], -alias_map[x+4 + (y-4) * w],
                    -alias_map[x-6 + (y-2) * w], -alias_map[x-4 + (y-2) * w], -alias_map[x-2 + (y-2) * w], -alias_map[x+0 + (y-2) * w], -alias_map[x+2 + (y-2) * w], -alias_map[x+4 + (y-2) * w], -alias_map[x+6 + (y-2) * w], 
                    -alias_map[x-6 + (y+0) * w], -alias_map[x-4 + (y+0) * w], -alias_map[x-2 + (y+0) * w], -alias_map[x+0 + (y+0) * w], -alias_map[x+2 + (y+0) * w], -alias_map[x+4 + (y+0) * w], -alias_map[x+6 + (y+0) * w], 
                    -alias_map[x-6 + (y+2) * w], -alias_map[x-4 + (y+2) * w], -alias_map[x-2 + (y+2) * w], -alias_map[x+0 + (y+2) * w], -alias_map[x+2 + (y+2) * w], -alias_map[x+4 + (y+2) * w], -alias_map[x+6 + (y+2) * w], 
                                                 -alias_map[x-4 + (y+4) * w], -alias_map[x-2 + (y+4) * w], -alias_map[x+0 + (y+4) * w], -alias_map[x+2 + (y+4) * w], -alias_map[x+4 + (y+4) * w],
                                                                              -alias_map[x-2 + (y+6) * w], -alias_map[x+0 + (y+6) * w], -alias_map[x+2 + (y+6) * w],
                };
                
                /* code generation & unoptimized version */
                /*
                int neighbours[50];
                int k = 0;
                int i,j;
                for (i = -3; i <= 3; i++)
                {
                    for (j = -3; j <= 3; j++)
                    {
                        //~ neighbours[k++] = -alias_map[x+j*2 + (y+i*2)*w];
                        printf("-alias_map[x%+d + (y%+d) * w], ", j*2, i*2);
                    }
                    printf("\n");
                }
                exit(1);
                */
                
                alias_aux[x + y * w] = -kth_smallest_int(neighbours, COUNT(neighbours), 5);
            }
        }

        if (debug_alias)
        {
            for (y = 3; y < h-2; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, ev2raw[COERCE(alias_aux[x + y*w] * 1024, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]);
            save_debug_dng("alias-dilated.dng");
        }

        printf("Smoothing alias map...\n");
        /* gaussian blur */
        for (y = 6; y < h-6; y ++)
        {
            for (x = 6; x < w-6; x ++)
            {
                /* do not compute alias map where we'll use fullres detail anyway */
                if (fullres_curve[bright[x + y*w]] > fullres_thr)
                    continue;

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
                    (alias_aux[x-2 + (y-2) * w] + alias_aux[x+2 + (y-2) * w] + alias_aux[x-2 + (y+2) * w] + alias_aux[x+2 + (y+2) * w]) * 173 / 1024 + 
                    (alias_aux[x+0 + (y-6) * w] + alias_aux[x-6 + (y+0) * w] + alias_aux[x+6 + (y+0) * w] + alias_aux[x+0 + (y+6) * w]) * 139 / 1024 + 
                    (alias_aux[x-2 + (y-6) * w] + alias_aux[x+2 + (y-6) * w] + alias_aux[x-6 + (y-2) * w] + alias_aux[x+6 + (y-2) * w] + alias_aux[x-6 + (y+2) * w] + alias_aux[x+6 + (y+2) * w] + alias_aux[x-2 + (y+6) * w] + alias_aux[x+2 + (y+6) * w]) * 111 / 1024 + 
                    (alias_aux[x-2 + (y-6) * w] + alias_aux[x+2 + (y-6) * w] + alias_aux[x-6 + (y-2) * w] + alias_aux[x+6 + (y-2) * w] + alias_aux[x-6 + (y+2) * w] + alias_aux[x+6 + (y+2) * w] + alias_aux[x-2 + (y+6) * w] + alias_aux[x+2 + (y+6) * w]) * 57 / 1024;
                alias_map[x + y * w] = c;
            }
        }

        if (debug_alias)
        {
            for (y = 3; y < h-2; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, ev2raw[COERCE(alias_map[x + y*w] * 128, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]);
            save_debug_dng("alias-smooth.dng");
        }

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

        if (debug_alias)
        {
            for (y = 3; y < h-2; y ++)
                for (x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, ev2raw[(long long)alias_map[x + y*w] * 13*EV_RESOLUTION / ALIAS_MAP_MAX]);
            save_debug_dng("alias-filtered.dng");
        }

        free(alias_aux);
    }

    /* where the image is overexposed? */
    overexposed = malloc(w * h * sizeof(uint16_t));
    memset(overexposed, 0, w * h * sizeof(uint16_t));

    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            overexposed[x + y * w] = bright[x + y * w] >= white_darkened || dark[x + y * w] >= white ? 100 : 0;
        }
    }
    
    /* "blur" the overexposed map */
    uint16_t* over_aux = malloc(w * h * sizeof(uint16_t));
    memcpy(over_aux, overexposed, w * h * sizeof(uint16_t));

    for (y = 3; y < h-3; y ++)
    {
        for (x = 3; x < w-3; x ++)
        {
            overexposed[x + y * w] = 
                (over_aux[x+0 + (y+0) * w])+ 
                (over_aux[x+0 + (y-1) * w] + over_aux[x-1 + (y+0) * w] + over_aux[x+1 + (y+0) * w] + over_aux[x+0 + (y+1) * w]) * 820 / 1024 + 
                (over_aux[x-1 + (y-1) * w] + over_aux[x+1 + (y-1) * w] + over_aux[x-1 + (y+1) * w] + over_aux[x+1 + (y+1) * w]) * 657 / 1024 + 
                //~ (over_aux[x+0 + (y-2) * w] + over_aux[x-2 + (y+0) * w] + over_aux[x+2 + (y+0) * w] + over_aux[x+0 + (y+2) * w]) * 421 / 1024 + 
                //~ (over_aux[x-1 + (y-2) * w] + over_aux[x+1 + (y-2) * w] + over_aux[x-2 + (y-1) * w] + over_aux[x+2 + (y-1) * w] + over_aux[x-2 + (y+1) * w] + over_aux[x+2 + (y+1) * w] + over_aux[x-1 + (y+2) * w] + over_aux[x+1 + (y+2) * w]) * 337 / 1024 + 
                //~ (over_aux[x-2 + (y-2) * w] + over_aux[x+2 + (y-2) * w] + over_aux[x-2 + (y+2) * w] + over_aux[x+2 + (y+2) * w]) * 173 / 1024 + 
                //~ (over_aux[x+0 + (y-3) * w] + over_aux[x-3 + (y+0) * w] + over_aux[x+3 + (y+0) * w] + over_aux[x+0 + (y+3) * w]) * 139 / 1024 + 
                //~ (over_aux[x-1 + (y-3) * w] + over_aux[x+1 + (y-3) * w] + over_aux[x-3 + (y-1) * w] + over_aux[x+3 + (y-1) * w] + over_aux[x-3 + (y+1) * w] + over_aux[x+3 + (y+1) * w] + over_aux[x-1 + (y+3) * w] + over_aux[x+1 + (y+3) * w]) * 111 / 1024 + 
                //~ (over_aux[x-2 + (y-3) * w] + over_aux[x+2 + (y-3) * w] + over_aux[x-3 + (y-2) * w] + over_aux[x+3 + (y-2) * w] + over_aux[x-3 + (y+2) * w] + over_aux[x+3 + (y+2) * w] + over_aux[x-2 + (y+3) * w] + over_aux[x+2 + (y+3) * w]) * 57 / 1024;
                0;
        }
    }
    
    free(over_aux); over_aux = 0;

    /* let's check the ideal noise levels (on the halfres image, which in black areas is identical to the bright one) */
    for (y = 3; y < h-2; y ++)
        for (x = 2; x < w-2; x ++)
            raw_set_pixel32(x, y, bright[x + y*w]);
    compute_black_noise(8, raw_info.active_area.x1 - 8, raw_info.active_area.y1 + 20, raw_info.active_area.y2 - 20, 1, 1, &noise_avg, &noise_std[0], raw_get_pixel32);
    double ideal_noise_std = noise_std[0];

    printf("Final blending...\n");
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

            /* full res with some smoothing applied to hide aliasing artifacts */
            int frs = fullres_smooth[x + y*w];

            /* go from linear to EV space */
            int hrev = raw2ev[hr];
            int frev = raw2ev[fr];
            int frsev = raw2ev[frs];

            int output = hrev;
            
            if (use_fullres)
            {
                /* blending factor */
                double f = fullres_curve[b & 0xFFFFF];
                
                double c = 0;
                if (use_alias_map)
                {
                    int co = alias_map[x + y*w];
                    c = COERCE(co / (double) ALIAS_MAP_MAX, 0, 1);
                }

                double ovf = COERCE(overexposed[x + y*w] / 200.0, 0, 1);
                c = MAX(c, ovf);

                double noisy_or_overexposed = MAX(ovf, 1-f);

                /* use data from both ISOs in high-detail areas, even if it's noisier (less aliasing) */
                f = MAX(f, c);
                
                /* use smoothing in noisy near-overexposed areas to hide color artifacts */
                double fev = noisy_or_overexposed * frsev + (1-noisy_or_overexposed) * frev;
                
                /* limit the use of fullres in dark areas (fixes some black spots, but may increase aliasing) */
                int sig = (dark[x + y*w] + bright[x + y*w]) / 2;
                f = MAX(0, MIN(f, (double)(sig - black) / (4*dark_noise)));
                
                /* blend "half-res" and "full-res" images smoothly to avoid banding*/
                output = hrev * (1-f) + fev * f;

                /* show full-res map (for debugging) */
                //~ output = f * 14*EV_RESOLUTION;
                
                /* show alias map (for debugging) */
                //~ output = c * 14*EV_RESOLUTION;

                //~ output = hotpixel[x+y*w] ? 14*EV_RESOLUTION : 0;
                //~ output = raw2ev[dark[x+y*w]];
                /* safeguard */
                output = COERCE(output, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1);
            }
            
            /* back to linear space and commit */
            raw_set_pixel32(x, y, ev2raw[output]);
        }
    }

    /* let's see how much dynamic range we actually got */
    compute_black_noise(8, raw_info.active_area.x1 - 8, raw_info.active_area.y1 + 20, raw_info.active_area.y2 - 20, 1, 1, &noise_avg, &noise_std[0], raw_get_pixel32);
    printf("Noise level     : %.02f (20-bit), ideally %.02f\n", noise_std[0], ideal_noise_std);
    printf("Dynamic range   : %.02f EV (cooked)\n", log2(white - black) - log2(noise_std[0]));

    /* run a second black subtract pass, to fix whatever our funky processing may do to blacks */
    black_subtract_simple(raw_info.active_area.x1, raw_info.active_area.y1);
    white = raw_info.white_level;
    black = raw_info.black_level;

    /* go back from 20-bit to 16-bit output */
    raw_info.buffer = raw_buffer_16;
    raw_info.black_level /= 16;
    raw_info.white_level /= 16;

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            raw_set_pixel_20to16(x, y, raw_buffer_32[x + y*w]);

    if (compress_highlights)
    {
        /* Soft film curve from ufraw */
        double exposure = 1 << compress_highlights;
        double a = MAX(exposure * 2 - 1, 1e-5);

        double baked_wb[3] = {2, 1, 2};
        double max_wb = MAX(baked_wb[0], baked_wb[2]);
        printf("Soft-film curve : +%.2f EV baked at WB %.2f %.2f %.2f\n", log2(exposure), baked_wb[0], baked_wb[1], baked_wb[2]);

        static double soft_film[1<<20];
        for (i = 0; i < 1<<20; i++)
        {
            double x = COERCE((double)(i - black) / (white - black), 0, 1);
            soft_film[i] = (1 - 1/(1+a*x)) / (1 - 1/(1+a)) * (white/16 - black/16) + black/16;
        }
        
        /* linear extrapolation under black level */
        double delta = soft_film[black+1] - soft_film[black];
        for (i = black-1; i > 0; i--)
        {
            soft_film[i] = soft_film[i+1] - delta;
        }

        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                double wb = baked_wb[FC(x,y)];
                int raw_baked = COERCE(((int)raw_buffer_32[x + y*w] - black) * wb / max_wb + black, 0, (1<<20)-1);
                raw_set_pixel16(x, y, COERCE((soft_film[raw_baked] - black/16) / wb + black/16, 0, 65535));
                
                /* with WB 1/1/1: */
                //~ raw_set_pixel16(x, y, soft_film[raw_buffer_32[x + y*w]]);
            }
        }
    }

end:

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
    free(overexposed);
    free(alias_map);
    free(raw_buffer_32);
    if (fullres_smooth && fullres_smooth != fullres) free(fullres_smooth);
    if (halfres_smooth && halfres_smooth != halfres) free(halfres_smooth);
    return ret;
}
