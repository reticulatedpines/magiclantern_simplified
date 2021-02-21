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
#include "dither.h"
#include "timing.h"
#include "kelvin.h"

#define MODULE_STRINGS_PREFIX dual_iso_strings
#include "../module_strings_wrapper.h"
#include "module_strings.h"
MODULE_STRINGS()

/** Command-line interface */

int interp_method = 0;          /* 0:amaze-edge, 1:mean23 */
int chroma_smooth_method = 2;
int fix_pink_dots = 0;
int fix_bad_pixels = 1;
int use_fullres = 1;
int use_alias_map = 1;
int use_stripe_fix = 1;
float soft_film_ev = 0;

int exif_wb = 0;
float custom_wb[3] = {0, 0, 0};
int debug_wb = 0;

#define WB_GRAY_MED 1
#define WB_GRAY_MAX 2
int gray_wb = WB_GRAY_MAX;

int debug_black = 0;
int debug_blend = 0;
int debug_amaze = 0;
int debug_edge = 0;
int debug_alias = 0;
int debug_bad_pixels = 0;
int debug_rggb = 0;
int debug_bddb = 0;
int plot_iso_curve = 0;
int plot_mix_curve = 0;
int plot_fullres_curve = 0;

int compress = 0;
int same_levels = 0;
int skip_existing = 0;
int embed_original = 0;

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
    int* variable;              /* can be float */
    int value_to_assign;        /* if the option field contains %d or %f, set this to number of %'s */
    char* option;               /* can contain %d or %f for options with values */
    char* help;
    int force_show;
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
        "Highlight/shadow handling", (struct cmd_option[]) {
            { (int*)&soft_film_ev,    1, "--soft-film=%f",  "bake a soft-film curve to compress highlights and raise shadows by X EV\n"
                                          "                  (if you use this option, you should also specify the white balance)"},
            OPTION_EOL
        },
    },
    {
        "White balance", (struct cmd_option[]) {
            { &gray_wb,     WB_GRAY_MAX, "--wb=graymax",    "set AsShotNeutral by maximizing the number of gray pixels (default)" },
            { &gray_wb,     WB_GRAY_MED, "--wb=graymed",    "set AsShotNeutral from the median of R-G and B-G" },
            { &exif_wb,               1, "--wb=exif",       "set AsShotNeutral from EXIF WB (not exactly working)" },
            { (int*)&custom_wb[0],    3, "--wb=%f,%f,%f",   "use custom RGB multipliers" },
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
        "Flicker handling", (struct cmd_option[]) {
            { (int*)&same_levels,    1, "--same-levels",       "Adjust output white levels to keep the same overall exposure\n"
                                            "                  for all frames passed in a single command line\n"
                                            "                  (useful to avoid flicker - for video or panoramas)" },
            /* todo: deflicker, percentiles... */
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
        "Misc settings", (struct cmd_option[]) {
            { &skip_existing,  1, "--skip-existing",  "Skip the conversion if the output file already exists" },

            { &embed_original, 1, "--embed-original", "Embed (move) the original CR2 file in the output DNG. The original will be deleted.\n"
                                    "                  You will be able to re-process the DNG with a different version or different conversion settings.\n"
                                    "                  To recover the original: exiftool IMG_1234.DNG -OriginalRawFileData -b > IMG_1234.CR2" },
            { &embed_original, 2, "--embed-original-copy",  "\n"
                                    "                  Similar to --embed-original, but without deleting the original.\n" },
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
            { &debug_rggb,     1, "--debug-rggb",       "plot debug info for RGGB/BGGR autodetection (requires octave)" },
            { &debug_bddb,     1, "--debug-bddb",       "plot debug info for bright/dark autodetection (requires octave)" },
            { &debug_wb,       1, "--debug-wb",         "show the vectorscope used for white balance (requires octave)" },
            { &plot_iso_curve, 1, "--iso-curve",        "plot the curve fitting results for ISO and black offset (requires octave)" },
            { &plot_mix_curve, 1, "--mix-curve",        "plot the curve used for half-res blending (requires octave)" },
            { &plot_fullres_curve, 1, "--fullres-curve","plot the curve used for full-res blending (requires octave)" },
            OPTION_EOL
        },
    },
    OPTION_GROUP_EOL
};


static int startswith(char* str, char* prefix)
{
    char* s = str;
    char* p = prefix;
    for (; *p; s++,p++)
        if (*s != *p) return 0;
    return 1;
}

static void parse_sscanf(char* user_input, char* format, void* ptr, int num_vars)
{
    void* pointers[5] = {0, 0, 0, 0, 0};
    if (num_vars > 5) goto err;
    int i;
    char* p = strchr(format, '%');
    for (i = 0; p != NULL && i < num_vars; i++, p = strchr(p+1, '%'))
    {
        //~ printf("%s: %p %p\n", format, ptr, &soft_film_ev);
        pointers[i] = ptr;
        int size = 
            *(p+1) == 'd' ? sizeof(int) :
            *(p+1) == 'f' ? sizeof(float) :
            0;
        if (size == 0) goto err;
        ptr += size;
    }
    if (i != num_vars) goto err;

    int num = sscanf(user_input, format, pointers[0], pointers[1], pointers[2], pointers[3], pointers[4]);
    if (num != num_vars)
    {
        printf("Error parsing %s: expected %d param%s, got %d\n", format, num_vars, num_vars == 1 ? "" : "s", num);
        exit(1);
    }

    return;

err:
    printf("invalid option: %s (internal error)\n", format);
    exit(1);
}

static void print_sscanf_option(char* format, void* ptr, int num_vars, char* help)
{
    int i = 0;
    int len = 0;
    for (char* p = format; *p && i < num_vars; p++)
    {
        if (*p != '%')
        {
            len += printf("%c", *p);
        }
        else
        {
            if (*(p+1) == 'd')
            {
                len += printf("%d", *(int*)ptr);
                ptr += sizeof(float);
            }
            else if (*(p+1) == 'f')
            {
                len += printf("%g", *(float*)ptr);
                ptr += sizeof(int);
            }
            p++; i++;
        }
    }
    while (len < 16)
    {
        len += printf(" ");
    }
    printf(": %s\n", help);
}

static void parse_commandline_option(char* option)
{
    
    for (struct cmd_group * g = options; g->name; g++)
    {
        for (struct cmd_option * o = g->options; o->option; o++)
        {
            if (strchr(o->option, '%'))
            {
                char base[100];
                snprintf(base, sizeof(base), "%s", o->option);
                char* percent = strchr(base, '%');
                if (percent)
                {
                    *percent = 0;   /* trim here */
                    if (startswith(option, base))
                    {
                        /* note that o->variable is the array where %d's or %f's are stored */
                        /* and o->value_to_assign is the number of items in that array */
                        parse_sscanf(option, o->option, o->variable, o->value_to_assign);
                        o->force_show = 1;
                        return;
                    }
                }
            }
            else if (!strcmp(option, o->option))
            {
                *(o->variable) = o->value_to_assign;
                check_shortcuts();
                return;
            }
        }
    }
    printf("Unknown option: %s\n", option);
    exit(1);
}

static void show_commandline_help(char* progname)
{
    printf("Command-line usage: %s [OPTIONS] [FILES]\n\n", progname);
    for (struct cmd_group * g = options; g->name; g++)
    {
        printf("%s:\n", g->name);
        for (struct cmd_option * o = g->options; o->option; o++)
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

    for (struct cmd_group * g = options; g->name; g++)
    {
        for (struct cmd_option * o = g->options; o->option; o++)
        {
            if (strchr(o->option, '%'))
            {
                if (o->force_show)
                {
                    /* note that o->variable is the array where %d's or %f's are stored */
                    /* and o->value_to_assign is the number of items in that array */
                    print_sscanf_option(o->option, o->variable, o->value_to_assign, o->help);
                }
            }
            else
            {
                if (o->help && (*o->variable) == o->value_to_assign)
                {
                    printf("%-16s: %s\n", o->option, o->help);
                }
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

/* conversion from linear to EV space and back, with range checking */
/* when no range checking is needed, just access the array directly */
#define EV2RAW(x) ev2raw[COERCE(x, -10*EV_RESOLUTION, 14*EV_RESOLUTION-1)]
#define RAW2EV(x) raw2ev[COERCE(x, 0, 0xFFFFF)]

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
static void white_balance_gray(float* red_balance, float* blue_balance, int method);

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

void raw_set_pixel_20to16_rand(int x, int y, int value) {
    /* To avoid posterization, it's a good idea to add some noise before rounding */
    /* The sweet spot seems to be with Gaussian noise of stdev=0.5, http://www.magiclantern.fm/forum/index.php?topic=10895.msg107972#msg107972 */
    raw_set_pixel16(x, y, COERCE((int)(value / 16.0 + fast_randn05() + 0.5), 0, 0xFFFF));
}

static void reverse_bytes_order(void* buf, int count)
{
    char* buf8 = (char*) buf;
    uint16_t* buf16 = (uint16_t*) buf;
    for (int i = 0; i < count/2; i++)
    {
        uint16_t x = buf16[i];
        buf8[2*i+1] = x;
        buf8[2*i] = x >> 8;
    }
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

static int is_file(const char* filename)
{
    FILE* f = fopen(filename, "r");
    if (f)
    {
        fclose(f);
        return 1;
    }
    else
    {
        return 0;
    }
}

int main(int argc, char** argv)
{
    printf("cr2hdr: a post processing tool for Dual ISO images\n\n");
    printf("Last update: %s\n", module_get_string(dual_iso_strings, "Last update"));

    fast_randn_init();

    if (argc == 1)
    {
        printf("No input files.\n\n");
        printf("GUI usage: drag some CR2 or DNG files over cr2hdr.exe.\n\n");
        show_commandline_help(argv[0]);
        return 0;
    }
    
    int r;

    /* parse all command-line options */
    for (int k = 1; k < argc; k++)
        if (argv[k][0] == '-')
            parse_commandline_option(argv[k]);
    
    solve_commandline_deps();
    show_active_options();
    
    /* keep track of black and white levels (useful for deflicker) */
    /* (we will not have more than "argc" files) */
    int* file_indices = malloc(argc * sizeof(file_indices[0]));
    int* blacks = malloc(argc * sizeof(blacks[0]));
    int* whites = malloc(argc * sizeof(whites[0]));
    int num_files = 0;
    
    /* all other arguments are input files */
    for (int k = 1; k < argc; k++)
    {
        if (argv[k][0] == '-')
            continue;
        
        char* filename = argv[k];

        printf("\nInput file      : %s\n", filename);
        int len = strlen(filename);

        char orig_filename[1000]; orig_filename[0] = 0;
        char out_filename[1000];

        if (strcmp(filename+len-4, ".DNG") == 0)
        {
            /* this DNG might have embedded CR2 data inside */
            /* note: we only save uppercase .DNGs, so a case-sensitive extension check should be fine */

            if (dng_has_original_raw(filename))
            {
                snprintf(orig_filename, sizeof(orig_filename), "%s", filename);
                orig_filename[len-3] = 'C';
                orig_filename[len-2] = 'R';
                orig_filename[len-1] = '2';
                
                if (is_file(orig_filename))
                {
                    printf("Already exists  : %s (error)\n", orig_filename);
                    continue;
                }

                if (extract_original_raw(filename, orig_filename))
                {
                    /* use the extracted CR2 as input */
                    filename = orig_filename;
                }
                else
                {
                    /* error message was already printed, now just skip this file */
                    continue;
                }
            }
        }

        snprintf(out_filename, sizeof(out_filename), "%s", filename);
        out_filename[len-3] = 'D';
        out_filename[len-2] = 'N';
        out_filename[len-1] = 'G';
        
        /* note: skip_existing will be ignored if we are working on a DNG file with embedded RAW */
        if (skip_existing && is_file(out_filename) && !orig_filename[0])
        {
            printf("Already exists  : %s (skipping)\n", out_filename);
            continue;
        }

        char dcraw_cmd[1000];
        snprintf(dcraw_cmd, sizeof(dcraw_cmd), "dcraw -v -i -t 0 \"%s\"", filename);
        FILE* t = popen(dcraw_cmd, "r");
        CHECK(t, "%s", filename);
        
        const char * model = get_camera_model(filename);
        get_raw_info(model, &raw_info);

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
        
        if (raw_width == 0)
        {
            printf("dcraw could not open this file\n");
            continue;
        }

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

        if (error || nd < 3)
        {
            pclose(fp);
            printf("dcraw output is not a valid PGM file\n");
            continue;
        }

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
        //~ for (int i = 0; i < 10; i++)
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
                reverse_bytes_order(raw_info.buffer, raw_info.frame_size);

                /* This option doesn't really work, since Canon WB is broken with Dual ISO. */
                if (exif_wb)
                {
                    float red_balance = -1, blue_balance = -1;
                    read_white_balance(filename, &red_balance, &blue_balance);
                    if ((red_balance > 0) && (blue_balance > 0))
                    {
                        dng_set_wbgain(1000000, red_balance*1000000, 1, 1, 1000000, blue_balance*1000000);
                        printf("AsShotNeutral   : %.2f 1 %.2f\n", 1/red_balance, 1/blue_balance);
                    }
                    else
                    {
                        printf("AsShotNeutral   : (using default values)\n");
                    }
                }
                
                char renamed_filename[1000];
                char* old_filename = 0;
                if (strcasecmp(filename, out_filename) == 0)
                {
                    /* if the filesystem is not case-sensitive, we will overwrite the input file */
                    /* I don't know how to detect this in a portable way, so I'll rename the input file just in case */
                    /* if no overwriting takes place, the renaming will be undone */
                    //~ printf("Might overwrite input file.\n");
                    snprintf(renamed_filename, sizeof(renamed_filename), "%s", filename);
                    int len = strlen(renamed_filename);
                    renamed_filename[len-1] = '6';
                    rename(filename, renamed_filename);
                    old_filename = filename;
                    filename = renamed_filename;
                }

                if (orig_filename[0])
                {
                    dng_backup_metadata(out_filename);
                }

                printf("Output file     : %s %s\n", out_filename, is_file(out_filename) ? "(already exists, overwriting)" : "");
                save_dng(out_filename);

                copy_tags_from_source(filename, out_filename);

                if (orig_filename[0])
                {
                    dng_restore_metadata(out_filename);
                }
                
                if (compress)
                {
                    dng_compress(out_filename, compress-1);
                }
                
                if (embed_original || orig_filename[0])
                {
                    /* this will move the input file into the DNG (and maybe delete the original) */
                    int delete_original = (embed_original != 2);
                    embed_original_raw(out_filename, filename, delete_original);
                }

                if (old_filename && is_file(renamed_filename))
                {
                    if (!is_file(old_filename))
                    {
                        /* input file not overwritten, undo renaming */
                        rename(renamed_filename, old_filename);
                    }
                    else
                    {
                        /* output file would overwrite the input file */
                        unlink(renamed_filename);
                    }
                }

                /* record black and white levels */
                file_indices[num_files] = k;
                blacks[num_files] = raw_info.black_level;
                whites[num_files] = raw_info.white_level;
                num_files++;
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
    
    if (same_levels && num_files > 1)
    {
        /* Equalize white-black for all shots.
         * 
         * Assuming all the pictures were shot at the same exposure settings,
         * this step will make sure they are all rendered identically (without flicker).
         * 
         * However, for this to work, all the files must be passed in the same command line.
         * 
         * We will use something close to maximum range among all files (with outlier filter).
         * 
         * This should work even if the black level is not the same in all shots.
         */
        
        printf("\nEqualizing levels...\n");
        
        int* ranges = malloc(num_files * sizeof(ranges[0]));
        for (int i = 0; i < num_files; i++)
        {
            ranges[i] = whites[i] - blacks[i];
        }
        int new_range = kth_smallest_int(ranges, num_files, num_files * 8 / 9 - 1);

        for (int i = 0; i < num_files; i++)
        {
            char* input_file = argv[file_indices[i]];

            /* fixme: duplicate code */
            char out_filename[1000];
            snprintf(out_filename, sizeof(out_filename), "%s", input_file);
            int len = strlen(out_filename);
            out_filename[len-3] = 'D';
            out_filename[len-2] = 'N';
            out_filename[len-1] = 'G';

            int new_white = blacks[i] + new_range;
            printf("%-16s: %d ... %d\n", out_filename, blacks[i], new_white);
            set_white_level(out_filename, new_white);
        }
        
        free(ranges);
    }
    
    free(whites);
    free(blacks);
    free(file_indices);
    
    return 0;
}

static void white_detect(int* white_dark, int* white_bright)
{
    /* sometimes the white level is much lower than 15000; this would cause pink highlights */
    /* workaround: consider the white level as a little under the maximum pixel value from the raw file */
    /* caveat: bright and dark exposure may have different white levels, so we'll take the minimum value */
    /* side effect: if the image is not overexposed, it may get brightened a little; shouldn't hurt */
    
    int whites[2]         = {  0,    0};
    int discard_pixels[2] = { 10,   50}; /* discard the brightest N pixels */
    int safety_margins[2] = {100, 1500}; /* use a higher safety margin for the higher ISO */
    /* note: with the high-ISO WL underestimated by 1500, you would lose around 0.15 EV of non-aliased detail */
    
    int* pixels[2];
    int max_pix = raw_info.width * raw_info.height / 2 / 9;
    pixels[0] = malloc(max_pix * sizeof(pixels[0][0]));
    pixels[1] = malloc(max_pix * sizeof(pixels[0][0]));
    int counts[2] = {0, 0};

    /* collect all the pixels and find the k-th max, thus ignoring hot pixels */
    /* change the sign in order to use kth_smallest_int */
    for (int y = raw_info.active_area.y1; y < raw_info.active_area.y2; y += 3)
    {
        for (int x = raw_info.active_area.x1; x < raw_info.active_area.x2; x += 3)
        {
            int pix = raw_get_pixel16(x, y);
            
            #define BIN_IDX is_bright[y%4]
            counts[BIN_IDX] = MIN(counts[BIN_IDX], max_pix-1);
            pixels[BIN_IDX][counts[BIN_IDX]] = -pix;
            counts[BIN_IDX]++;
            #undef BIN_IDX
        }
    }
    
    whites[0] = -kth_smallest_int(pixels[0], counts[0], discard_pixels[0]) - safety_margins[0];
    whites[1] = -kth_smallest_int(pixels[1], counts[1], discard_pixels[1]) - safety_margins[1];

    //~ printf("%8d %8d\n", whites[0], whites[1]);
    //~ printf("%8d %8d\n", counts[0], counts[1]);
    
    /* we assume 14-bit input data; out-of-range white levels may cause crash */
    *white_dark = COERCE(whites[0], 10000, 16383);
    *white_bright = COERCE(whites[1], 5000, 16383);
    
    printf("White levels    : %d %d\n", *white_dark, *white_bright);

    free(pixels[0]);
    free(pixels[1]);
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
    
    int* vblack = malloc(h * sizeof(int) * 2);
    int* hblack = malloc(w * sizeof(int));
    int* aux = malloc(MAX(w, h * 2) * sizeof(int));
    uint16_t * blackframe = malloc(w * h * sizeof(uint16_t));

    /* data above this may be gibberish */
    /* tight fit, any similar trouble would require changing the algorithm */
    int ymin = (top_margin-8-3) & ~3;
    int ymax = ymin + 8;

    /* estimate vertical correction for each line */
    for (int k = 0; k < 2; k++)
    {
       for (int y = ymin; y < h; y++)
       {
            /* use median instead of averaging, to ignore hot pixels */
           int samples[256];
           int num = 0;
           for (int x = 2 + k; x < left_margin - 8 && num < COUNT(samples); x+=2, num++)
           {
               samples[num] = raw_get_pixel16(x, y);
           }
           vblack[y * 2 + k] = median_int_wirth(samples, num);
       }
    }
    /* perform some slight filtering (averaging) so we don't add noise to the image */
    for (int y = ymin; y < h; y++)
    {
        int avg0 = 0;
        int avg1 = 0;
        int num = 0;
        int y2 = MAX(y - 10*4, ymin + y % 4 - ymin % 4);
        if (y2 < ymin) y2 += 4; // for the case when (y % 4) < (ymin % 4)
        for (; y2 < y + 10*4; y2 += 4)
        {
            if (y2 >= h) break;
            avg0 += vblack[y2 * 2];
            avg1 += vblack[y2 * 2 + 1];
            num++;
        }
        if (num > 0)
        {
            avg0 /= num;
            avg1 /= num;
            aux[y * 2] = avg0;
            aux[y * 2 + 1] = avg1;
        }
        else
        {
            aux[y * 2] = vblack[y * 2];
            aux[y * 2 + 1] = vblack[y * 2 + 1];
        }
    }
    
    memcpy(vblack, aux, h * sizeof(vblack[0]) * 2);
    
    double avg_black = 0;

    /* update the dark frame and compute the average black level */
    for (int y = ymin; y < h; y++)
    {
        for (int k = 0; k < 2; k++)
        {
           for (int x = k; x < w; x+=2)
           {
              blackframe[x + y*w] = vblack[y * 2 + k];
           }
           avg_black += vblack[y * 2 + k];
        }
    }
    avg_black /= 2 * (h - ymin);
    
    
    /* estimate horizontal drift for each channel */
    for (int k = 0; k < 4; k++)
    {
        int y0 = ymin + k;
        int offset = 0;
        {
            /* use median instead of averaging, to ignore hot pixels */
            int samples[256];
            int num = 0;
            for (int y = y0; y < ymax && num < COUNT(samples); y += 4, num++)
            {
                samples[num] = blackframe[y*w];
            }
            offset = median_int_wirth(samples, num);
        }
        
        /* try to fix banding that repeats every 8 pixels */
        for (int xg = 0; xg < 8; xg++)
        {
            for (int x = xg; x < w; x += 8)
            {
                int num = 0;
                int avg = 0;
                for (int y = y0; y < ymax; y += 4)
                {
                    avg += raw_get_pixel16(x, y) - offset;
                    num++;
                }
                hblack[x] = avg / num;
            }
            
            /* perform some stronger filtering (averaging), since this data is a lot noisier */
            /* if we don't do that, we will add some strong FPN to the image */
            for (int x = xg; x < w; x += 8)
            {
                int avg = 0;
                int num = 0;
                /* there is no need to start the next loop at the lowest value */
                /* let us simply skip all them at once using MAX */
                int x2 = MAX(x - 1024, x % 8);
                for (; x2 < x + 1024; x2 += 8)
                {
                    if (x2 >= w) break;
                    avg += hblack[x2];
                    num++;
                }
                avg /= num;
                aux[x] = avg;
            }
            memcpy(hblack, aux, w * sizeof(hblack[0]));

            /* update the dark frame */
            for (int y = y0; y < h; y += 4)
                for (int x = xg; x < w; x += 8)
                    blackframe[x + y*w] += hblack[x];
        }
    }
    
    if (debug_black)
    {
        /* change black and white levels to see the black frame when developing the DNG */
        int black_black = INT_MAX;
        int black_white = 0;
        for (int y = raw_info.active_area.y1; y < raw_info.active_area.y2; y ++)
        {
            for (int x = raw_info.active_area.x1; x < raw_info.active_area.x2; x++)
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
    for (int y = ymin; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int p = raw_get_pixel16(x, y);
            int black_delta = avg_black - blackframe[x + y*w];
            p += black_delta;
            p = COERCE(p, 0, 16383);
            raw_set_pixel16(x, y, p);
        }
    }

    raw_info.black_level = (int) avg_black;
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

    /* median value from left OB bar */
    int* samples = malloc(left_margin * h * sizeof(samples[0]));
    
    int num = 0;
    for (int y = top_margin + 20; y < h - 20; y++)
    {
        for (int x = 16; x < left_margin - 16; x++)
        {
            int p = raw_get_pixel20(x, y);
            samples[num++] = p;
        }
    }
    
    int new_black = median_int_wirth(samples, num);
    
    free(samples); samples = 0;
        
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
    for (int y = y1; y < y2; y += dy)
    {
        for (int x = x1; x < x2; x += dx)
        {
            black += raw_get_pixel(x, y);
            num++;
        }
    }

    double mean = (double) black / num;

    /* compute standard deviation */
    double stdev = 0;
    for (int y = y1; y < y2; y += dy)
    {
        for (int x = x1; x < x2; x += dx)
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
    
    /* ignore the last half-stop (we don't know the white level accurately at this point) */
    white = (white - black) * 0.707 + black;

    int w = raw_info.width;
    int h = raw_info.height;

    static double raw2ev[16384];
    
    for (int i = 0; i < 16384; i++)
        raw2ev[i] = log2(MAX(1, i - black));

    double avg_ev = 0;
    int num = 0;
    for (int y = 2; y < h-2; y ++)
    {
        for (int x = 2; x < w-2; x ++)
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
static int identify_rggb_or_gbrg()
{
    int w = raw_info.width;
    int h = raw_info.height;
    
    /* build 4 little histograms: one for red, one for blue and two for green */
    /* we don't know yet which channels are which, but that's what we are trying to find out */
    /* the ones with the smallest difference are likely the green channels */
    int hist_size = 16384 * sizeof(int);
    int* hist[4];
    for (int i = 0; i < 4; i++)
    {
        hist[i] = malloc(hist_size);
        memset(hist[i], 0, hist_size);
    }
    
    int y0 = (raw_info.active_area.y1 + 3) & ~3;
    
    /* to simplify things, analyze an identical number of bright and dark lines */
    for (int y = y0; y < h/4*4; y++)
    {
        for (int x = 0; x < w; x++)
            hist[(y%2)*2 + (x%2)][raw_get_pixel16(x,y) & 16383]++;
    }
    
    /* compute cdf */
    for (int k = 0; k < 4; k++)
    {
        int acc = 0;
        for (int i = 0; i < 16384; i++)
        {
            acc += hist[k][i];
            hist[k][i] = acc;
        }
    }

    /* dump the histograms */
    if (debug_rggb)
    {
        FILE* f = fopen("rggb.m", "w");
        fprintf(f, "hists = [\n");
        for (int i = 0; i < 16384; i++)
        {
            fprintf(f, "%d %d %d %d\n", hist[0][i], hist[1][i], hist[2][i], hist[3][i]);
        }
        fprintf(f, "];\n");
        fprintf(f, "hold on; for i = 1:4, for j = i+1:4, plot(hists(:,i), hists(:,j), 'color', [0.5 0.5 0.5]); end; end;\n");
        fprintf(f, "plot(hists(:,2), hists(:,3), 'r', hists(:,1), hists(:,4), 'g');\n");
        fclose(f);
        if(system("octave --persist rggb.m"));
    }

    /* compare cdf's */
    /* for rggb, greens are at y%2 != x%2, that is, 1 and 2 */
    /* for gbrg, greens are at y%2 == x%2, that is, 0 and 3 */
    double diffs_rggb = 0;
    double diffs_gbrg = 0;
    for (int i = 0; i < 16384; i++)
    {
        diffs_rggb += ABS(hist[1][i] - hist[2][i]);
        diffs_gbrg += ABS(hist[0][i] - hist[3][i]);
    }

    for (int i = 0; i < 4; i++)
    {
        free(hist[i]); hist[i] = 0;
    }
    
    /* which one is most likely? */
    return diffs_rggb < diffs_gbrg;
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

    /* white level is not yet known, just use a rough guess */
    int white = 10000;
    int black = raw_info.black_level;
    
    int w = raw_info.width;
    int h = raw_info.height;
    
    /* build 4 little histograms */
    int hist_size = 16384 * sizeof(int);
    int* hist[4];
    for (int i = 0; i < 4; i++)
    {
        hist[i] = malloc(hist_size);
        memset(hist[i], 0, hist_size);
    }
    
    int y0 = (raw_info.active_area.y1 + 3) & ~3;
    
    /* to simplify things, analyze an identical number of bright and dark lines */
    for (int y = y0; y < h/4*4; y++)
    {
        for (int x = 0; x < w; x++)
        {
            if ((x%2) != (y%2))
            {
                /* only check the green pixels */
                hist[y%4][raw_get_pixel16(x,y) & 16383]++;
            }
        }
    }
    
    int hist_total = 0;
    for (int i = 0; i < 16384; i++)
        hist_total += hist[0][i];

    FILE* f = 0;
    if (debug_bddb)
    {
        f = fopen("bddb.m", "w");
        fprintf(f, "levels = [\n");
    }

    /* choose the highest percentile that is not overexposed */
    /* but not higher than 99.8, to keep a tiny bit of robustness (specular highlights may play dirty tricks) */
    int acc[4] = {0};
    int raw[4] = {0};
    int off[4] = {0};
    int ref;
    int ref_max = hist_total * 0.998;
    int ref_off = hist_total * 0.05;
    for (ref = 0; ref < ref_max; ref++)
    {
        int changed = 0;
        for (int i = 0; i < 4; i++)
        {
            while (acc[i] < ref)
            {
                acc[i] += hist[i][raw[i]];
                raw[i]++;
                changed = 1;
            }
        }
        
        if (debug_bddb && changed)
        {
            fprintf(f, "%d %d %d %d %d\n", raw[0], raw[1], raw[2], raw[3], ref);
        }
        
        if (ref < ref_off)
        {
            if (MAX(MAX(raw[0], raw[1]), MAX(raw[2], raw[3])) < black + (white-black) / 4)
            {
                /* try to remove the black offset by estimating it from relatively dark pixels */
                off[0] = raw[0];
                off[1] = raw[1];
                off[2] = raw[2];
                off[3] = raw[3];
            }
        }

        if ((raw[0] >= white) + (raw[1] >= white) + (raw[2] >= white) + (raw[3] >= white) >= 2)
        {
            /* stop when at least two accumulators reach clipping */
            break;
        }
    }

    if (debug_bddb)
    {
        fprintf(f, "];\n");
        fprintf(f, "off = [%d %d %d %d]\n", off[0], off[1], off[2], off[3]);
        fprintf(f, "ref = levels(:,end);\n");
        fprintf(f, "plot(ref, levels(:,1) - off(1), ref, levels(:,2) - off(2), ref, levels(:,3) - off(3), ref, levels(:,4) - off(4));\n");
        fprintf(f, "legend('0', '1', '2', '3');\n");
        fclose(f);
        if(system("octave --persist bddb.m"));
    }
    
    for (int i = 0; i < 4; i++)
    {
        free(hist[i]); hist[i] = 0;
    }

    /* remove black offsets */
    raw[0] -= off[0];
    raw[1] -= off[1];
    raw[2] -= off[2];
    raw[3] -= off[3];

    /* very crude way to compute median */
    int sorted_bright[4];
    memcpy(sorted_bright, raw, sizeof(sorted_bright));
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = i+1; j < 4; j++)
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

    for (int i = 0; i < 4; i++)
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

#if 0
typedef int (*CritFunc)(int);
// crit returns negative if the tested value is too high, positive if too low, 0 if perfect

static int bin_search(int lo, int hi, CritFunc crit)
{
    if (lo >= hi-1) return lo;
    int m = (lo+hi)/2;
    int c = crit(m);
    if (c == 0) return m;
    if (c > 0) return bin_search(m, hi, crit);
    return bin_search(lo, m, crit);
}
#endif

static int mean2(int a, int b, int white, int* err);

static int match_exposures(double* corr_ev, int* white_darkened)
{
    /* guess ISO - find the factor and the offset for matching the bright and dark images */
    int black20 = raw_info.black_level;
    int white20 = MIN(raw_info.white_level, *white_darkened);
    int black = black20/16;
    int white = white20/16;
    int clip0 = white - black;
    int clip  = clip0 * 0.95;    /* there may be nonlinear response in very bright areas */

    int w = raw_info.width;
    int h = raw_info.height;
    int y0 = raw_info.active_area.y1 + 2;

    /* quick interpolation for matching */
    int* dark   = malloc(w * h * sizeof(dark[0]));
    int* bright = malloc(w * h * sizeof(bright[0]));
    memset(dark, 0, w * h * sizeof(dark[0]));
    memset(bright, 0, w * h * sizeof(bright[0]));
    
    for (int y = y0; y < h-2; y += 3)
    {
        int* native = BRIGHT_ROW ? bright : dark;
        int* interp = BRIGHT_ROW ? dark : bright;

        for (int x = 0; x < w; x += 3)
        {
            int pa = raw_get_pixel_20to16(x, y-2) - black;
            int pb = raw_get_pixel_20to16(x, y+2) - black;
            int pn = raw_get_pixel_20to16(x, y) - black;
            int pi = (pa + pb + 1) / 2;
            if (pa >= clip || pb >= clip) pi = clip0;               /* pixel too bright? discard */
            if (pi >= clip) pn = clip0;                             /* interpolated pixel not good? discard the other one too */
            interp[x + y * w] = pi;
            native[x + y * w] = pn;
        }
    }
    
    /* 
     * Robust line fit (match unclipped data):
     * - use (median_bright, median_dark) as origin
     * - select highlights between 98 and 99.9th percentile to find the slope (ISO)
     * - choose the slope that explains the largest number of highlight points (inspired from RANSAC)
     * 
     * Rationale:
     * - exposure matching is important to be correct in bright_highlights (which are combined with dark_midtones)
     * - low percentiles are likely affected by noise (this process is essentially a histogram matching)
     * - as ad-hoc as it looks, it's the only method that passed all the test samples so far.
     */
    int nmax = (w+2) * (h+2) / 9;   /* downsample by 3x3 for speed */
    int * tmp = malloc(nmax * sizeof(tmp[0]));
    
    /* median_bright */
    int n = 0;
    for (int y = y0; y < h-2; y += 3)
    {
        for (int x = 0; x < w; x += 3)
        {
             int b = bright[x + y*w];
             if (b >= clip) continue;
             tmp[n++] = b;
        }
    }
    int bmed = median_int_wirth(tmp, n);

    int * bps = 0;
    if (plot_iso_curve)
    {
        /* bright percentiles, in 0.5% increments */
        bps = malloc(200 * sizeof(bps[0]));
        for (int i = 0; i < 200; i++)
        {
            bps[i] = kth_smallest_int(tmp, n, (long long) n*i/200);
        }
    }

    /* also compute the range for bright pixels (used to find the slope) */
    int b_lo = kth_smallest_int(tmp, n, n*98/100);
    int b_hi = kth_smallest_int(tmp, n, n*99.9/100);

    /* median_dark */
    n = 0;
    for (int y = y0; y < h-2; y += 3)
    {
        for (int x = 0; x < w; x += 3)
        {
             int d = dark[x + y*w];
             int b = bright[x + y*w];
             if (b >= clip) continue;
             tmp[n++] = d;
        }
    }
    int dmed = median_int_wirth(tmp, n);

    int * dps = 0;
    if (plot_iso_curve)
    {
        /* dark percentiles, in 0.5% increments */
        dps = malloc(200 * sizeof(bps[0]));
        for (int i = 0; i < 200; i++)
        {
            dps[i] = kth_smallest_int(tmp, n, (long long) n*i/200);
        }
    }

    /* select highlights used to find the slope (ISO) */
    /* (98th percentile => up to 2% highlights) */
    int hi_nmax = nmax/50;
    int hi_n = 0;
    int* hi_dark = malloc(hi_nmax * sizeof(hi_dark[0]));
    int* hi_bright = malloc(hi_nmax * sizeof(hi_bright[0]));

    for (int y = y0; y < h-2; y += 3)
    {
        for (int x = 0; x < w; x += 3)
        {
             int d = dark[x + y*w];
             int b = bright[x + y*w];
             if (b >= b_hi) continue;
             if (b <= b_lo) continue;
             hi_dark[hi_n] = d;
             hi_bright[hi_n] = b;
             hi_n++;
             if (hi_n >= hi_nmax) break;
        }
    }

    //~ printf("Selected %d highlight points (max %d)\n", hi_n, hi_nmax);
    
    double a = 0;
    double b = 0;

    int best_score = 0;
    for (double ev = 0; ev < 6; ev += 0.002)
    {
        double test_a = pow(2, -ev);
        double test_b = dmed - bmed * test_a;

        int score = 0;
        for (int i = 0; i < hi_n; i++)
        {
            int d = hi_dark[i];
            int b = hi_bright[i];
            int e = d - (b*test_a + test_b);
            if (ABS(e) < 50) score++;
        }
        if (score > best_score)
        {
            best_score = score;
            a = test_a;
            b = test_b;
            //~ printf("%f: %d\n", a, score);
        }
    }
    free(hi_dark); hi_dark = 0;
    free(hi_bright); hi_bright = 0;
    free(tmp); tmp = 0;

    if (plot_iso_curve)
    {
        printf("Linear fit      : y = %f*x + %f\n", a, b);
        FILE* f = fopen("iso-curve.m", "w");
        fprintf(f, "a = %g\n", a);
        fprintf(f, "b = %g\n", b);
        fprintf(f, "clip = %d\n", clip);
        fprintf(f, "data = [\n");
        int y0 = raw_info.active_area.y1 + 5;
        for (int i = 0; i < 50000; i++)
        {
            int x = (rand() % w)/3*3;
            int y = (rand() % (h-y0-5))/3*3 + y0;
            int d = dark[x + y*w];
            int b = bright[x + y*w];
            if (b >= clip0)
            {
                /* retry (discard this pixel) */
                /* this graph includes a few more highlights, not just those actually used for fitting */
                i--;
                continue;
            }
            /* randomize data, to get a better idea of pixel density */
            fprintf(f, "    %f %f;\n", b + fast_randn05(), d + fast_randn05());
        }
        fprintf(f, "];\n");

        fprintf(f, "bps = [ ");
        for (int i = 0; i < 200; i++)
            fprintf(f, "%d ", bps[i]);
        fprintf(f, "];\n");

        fprintf(f, "dps = [ ");
        for (int i = 0; i < 200; i++)
            fprintf(f, "%d ", dps[i]);
        fprintf(f, "];\n");

        fprintf(f, "bright = data(:,1);\n");
        fprintf(f, "brightd = data(:,1)*a+b;\n");
        fprintf(f, "dark = data(:,2);\n");
        fprintf(f, "hi = bright > %d & bright < %d;\n", b_lo, b_hi);
        //~ fprintf(f, "median(dark(hi) - bright(hi)*a - b)\n");
        fprintf(f, "plot(brightd, dark, 'o', 'markersize', 0.1, brightd(hi), dark(hi), 'og', 'markersize', 0.1, brightd, brightd, 'or', 'markersize', 1); hold on;\n");
        //~ fprintf(f, "axis([-1000 clip*1.1 -1000 1.5*a*clip+b]);\n");
        fprintf(f, "axis auto; set(gca,'xscale','log'); set(gca,'yscale','log'); axis tight;\n");
        fprintf(f, "plot(bps*a+b, dps, 'm', 'linewidth', 2, bps(round(1:9.99:end))*a+b, dps(round(1:9.99:end)), 'om', 'markersize', 3, 'linewidth', 6);\n");
        fprintf(f, "plot([%d %d]*a+b, [%d %d], 'or', 'markersize', 3, 'linewidth', 8);\n", bmed, 0, dmed, 0);
        fprintf(f, "print -dpng iso-curve.png\n");
        fclose(f);
        if(system("octave --persist iso-curve.m"));
    }
    free(dark);
    free(bright);
    if (dps) free(dps);
    if (bps) free(bps);

    /* apply the correction */
    double b20 = b * 16;
    for (int y = 0; y < h; y ++)
    {
        for (int x = 0; x < w; x ++)
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
            
            /* out of range? */
            /* note: this breaks M24-1127 */
            p = COERCE(p, 0, 0xFFFFF);
            
            raw_set_pixel20(x, y, p);
        }
    }
    *white_darkened = (white20 - black20 + b20) * a + black20;

    double factor = 1/a;
    if (factor < 1.2 || !isfinite(factor))
    {
        printf("Doesn't look like interlaced ISO\n");
        factor = 1;
        return 0;
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

    /* really dark pixels (way below the black level) are probably noise */
    /* there might be dark pixels not that much below the black level, but they need further checking */
    int cold_thr = MAX(0, black - dark_noise*8);
    int maybe_cold_thr = black + dark_noise*2;

    for (int y = 6; y < h-6; y ++)
    {
        for (int x = 6; x < w-6; x ++)
        {
            int p = raw_get_pixel20(x, y);
            
            int is_hot = 0;
            int is_cold = (p < cold_thr);
            int maybe_cold = (p < maybe_cold_thr);

            /* we don't have no hot pixels on the bright exposure */
            /* but we may have cold pixels */
            if (!BRIGHT_ROW || maybe_cold)
            {
                /* let's look at the neighbours: is this pixel clearly brigher? (isolated) */
                int neighbours[100];
                int k = 0;
                int fc0 = FC(x, y);
                int b0 = is_bright[y%4];
                int max = 0;
                for (int i = -4; i <= 4; i++)
                {
                    /* only look at pixels of the same brightness */
                    if (is_bright[(y+i)%4] != b0)
                        continue;

                    for (int j = -4; j <= 4; j++)
                    {
                        if (i == 0 && j == 0)
                            continue;
                        
                        /* only look at pixels of the same color */
                        if (FC(x+j, y+i) != fc0)
                            continue;
                        
                        int p = raw_get_pixel20(x+j, y+i);
                        neighbours[k++] = -p;
                        max = MAX(max, p);
                    }
                    
                    /* this difference will only get lower, so if it's already too low (see below), stop scanning */
                    /* (don't stop scanning if the pixel is cold, since we'll need this info to interpolate it) */
                    if (raw2ev[p] - raw2ev[max] <= EV_RESOLUTION && !maybe_cold)
                        break;  
                }
                
                is_hot = (raw2ev[p] - raw2ev[max] > EV_RESOLUTION) && (max > black + 8*dark_noise);
                
                if (maybe_cold)
                {
                    /* there may be cold pixels very close to black level */
                    /* heuristic: if it's much darker than the brightest neighbour, it's a cold pixel */
                    is_cold |= (raw2ev[max] - raw2ev[p] > EV_RESOLUTION * 10);
                }
                
                if (fix_bad_pixels == 2)    /* aggressive */
                {
                    int third_max = -kth_smallest_int(neighbours, k, 2);
                    is_hot = ((raw2ev[p] - raw2ev[max] > EV_RESOLUTION/4) && (max > black + 8*dark_noise))
                          || (raw2ev[p] - raw2ev[third_max] > EV_RESOLUTION/2);
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
    for (int y = 0; y < h; y ++)
        for (int x = 0; x < w; x ++)
            if (hotpixel[x + y*w])
                raw_set_pixel20(x, y, debug_bad_pixels ? black : hotpixel[x + y*w]);

    if (hot_pixels)
        printf("Hot pixels      : %d\n", hot_pixels);

    if (cold_pixels)
        printf("Cold pixels     : %d\n", cold_pixels);
    
    free(hotpixel);
}

/* soft-film curve from ufraw-mod */
static double soft_film(double raw, double exposure, int in_black, int in_white, int out_black, int out_white)
{
    double a = MAX(exposure - 1, 1e-5);
    if (raw > in_black)
    {
        /* at low values, force the derivative equal to exposure (in linear units) */
        /* at high values, map in_white to out_white (which normally happens at exposure=1) */
        double x = (raw - in_black) / (in_white - in_black);
        return (1.0 - 1.0/(1.0 + a*x)) / (1.0 - 1.0/(1.0 + a)) * (out_white - out_black) + out_black;
    }
    else
    {
        /* linear extrapolation below black */
        return COERCE((raw - in_black) * exposure / (in_white - in_black) * (out_white - out_black) + out_black, 0, out_white);
    }
}

static int soft_film_bakedwb(double raw, double exposure, int in_black, int in_white, int out_black, int out_white, double wb, double max_wb)
{
    double raw_baked = (raw - in_black) * wb / max_wb + in_black;
    double raw_soft = soft_film(raw_baked, exposure * max_wb, in_black, in_white, out_black, out_white);
    double raw_adjusted = (raw_soft - out_black) / wb + out_black;
    return round(raw_adjusted + fast_randn05());
}

static int hdr_interpolate()
{
    int w = raw_info.width;
    int h = raw_info.height;

    /* RGGB or GBRG? */
    int rggb = identify_rggb_or_gbrg();
    
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
    {
        return 0;
    }

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

    for (int i = 0; i < 1<<20; i++)
    {
        double signal = MAX(i/64.0 - black/64.0, -1023);
        if (signal > 0)
            raw2ev[i] = (int)round(log2(1+signal) * EV_RESOLUTION);
        else
            raw2ev[i] = -(int)round(log2(1-signal) * EV_RESOLUTION);
    }

    for (int i = -10*EV_RESOLUTION; i < 0; i++)
    {
        ev2raw[i] = COERCE(black+64 - round(64*pow(2, ((double)-i/EV_RESOLUTION))), 0, black);
    }

    for (int i = 0; i < 14*EV_RESOLUTION; i++)
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
    for (int y = 0; y < 4; y++)
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
        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                raw_set_pixel16(x, y, raw_get_pixel_14to16(x, !BRIGHT_ROW ? y : y+2));
        raw_info.black_level /= 16;
        raw_info.white_level /= 16;
        goto end;
    }

    /* promote from 14 to 20 bits (original raw buffer holds 14-bit values stored as uint16_t) */
    void* raw_buffer_16 = raw_info.buffer;
    uint32_t * raw_buffer_32 = malloc(w * h * sizeof(raw_buffer_32[0]));
    
    for (int y = 0; y < h; y ++)
        for (int x = 0; x < w; x ++)
            raw_buffer_32[x + y*w] = raw_get_pixel_14to20(x, y);

    raw_info.buffer = raw_buffer_32;
    for (int y = 0; y < h; y ++)
        for (int x = 0; x < w; x ++)
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
    const double fullres_transition = 4;
    const double fullres_thr = 0.8;
    
    for (int i = 0; i < (1<<20); i++)
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
        for (int i = 0; i < 65536; i++)
            fprintf(f, "%f ", log2(MAX(i/4.0 - black/64.0, 1)));
        fprintf(f, "];\n");

        fprintf(f, "f = [");
        for (int i = 0; i < 65536; i++)
            fprintf(f, "%f ", fullres_curve[i*16]);
        fprintf(f, "];\n");
        
        fprintf(f, "plot(ev, f);\n");
        fprintf(f, "print -dpng fullres-curve.png\n");
        fclose(f);
        
        if(system("octave --persist fullres-curve.m"));
    }

    //~ printf("Exposure matching...\n");
    /* estimate ISO difference between bright and dark exposures */
    double corr_ev = 0;
    int white_darkened = white_bright;
    int ok = match_exposures(&corr_ev, &white_darkened);
    if (!ok) goto err;

    /* run a second black subtract pass, to fix whatever our funky processing may do to blacks */
    black_subtract_simple(raw_info.active_area.x1, raw_info.active_area.y1);

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
        
        for (int i = 0; i < h; i++)
        {
            int wx = w + 16;
            rawData[i] =   malloc(wx * sizeof(rawData[0][0]));
            memset(rawData[i], 0, wx * sizeof(rawData[0][0]));
            red[i]     = malloc(wx * sizeof(red[0][0]));
            green[i]   = malloc(wx * sizeof(green[0][0]));
            blue[i]    = malloc(wx * sizeof(blue[0][0]));
        }
        
        /* squeeze the dark image by deleting fields from the bright exposure */
        int yh = -1;
        for (int y = 0; y < h; y ++)
        {
            if (BRIGHT_ROW)
                continue;
            
            if (yh < 0) /* make sure we start at the same parity (RGGB cell) */
                yh = y;
            
            for (int x = 0; x < w; x++)
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
        for (int y = 0; y < h; y ++)
        {
            if (!BRIGHT_ROW)
                continue;

            if (yh < 0) /* make sure we start with the same parity (RGGB cell) */
                yh = h/4*2 + y;
            
            for (int x = 0; x < w; x++)
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
            for (int y = 0; y < h; y ++)
                for (int x = 0; x < w; x ++)
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

        /* undo green channel scaling and clamp the other channels */
        for (int y = 0; y < h; y ++)
        {
            for (int x = 0; x < w; x ++)
            {
                green[y][x] = COERCE((green[y][x] - black) * 2 + black, 0, 0xFFFFF);
                red[y][x] = COERCE(red[y][x], 0, 0xFFFFF);
                blue[y][x] = COERCE(blue[y][x], 0, 0xFFFFF);
            }
        }

        if (debug_amaze)
        {
            for (int y = 0; y < h; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, red[y][x]);
            save_debug_dng("amaze-red.dng");

            for (int y = 0; y < h; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, green[y][x]);
            save_debug_dng("amaze-green.dng");

            for (int y = 0; y < h; y ++)
                for (int x = 2; x < w-2; x ++)
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
        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                gray[x + y*w] = green[squeezed[y]][x]/2 + red[squeezed[y]][x]/4 + blue[squeezed[y]][x]/4;

        #if 0
        for (int y = 0; y < h; y ++)
            for (int x = 2; x < w-2; x ++)
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
        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                edge_direction[x + y*w] = d0;

        //~ printf("Cross-correlation...\n");
        int semi_overexposed = 0;
        int not_overexposed = 0;
        int deep_shadow = 0;
        int not_shadow = 0;
        
        for (int y = 5; y < h-5; y ++)
        {
            int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;    /* points to the closest row having different exposure */
            for (int x = 5; x < w-5; x ++)
            {
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
                    for (int d = dmin; d <= dmax; d++)
                    {
                        int e = 0;
                        for (int j = -search_area; j <= search_area; j++)
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
            for (int y = 4; y < h-4; y += 10)
            {
                /* only show bright rows (interpolated from dark ones) */
                while (!BRIGHT_ROW) y++;
                
                int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;    /* points to the closest row having different exposure */
                for (int x = 4; x < w-4; x += 10)
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

            for (int y = 0; y < h; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, gray[x + y*w]);
            save_debug_dng("edges.dng");
            if(system("dcraw -d -r 1 1 1 1 edges.dng"));
            /* best viewed at 400% with nearest neighbour interpolation (no filtering) */

            for (int y = 0; y < h; y ++)
            {
                for (int x = 2; x < w-2; x ++)
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

        for (int y = 2; y < h-2; y ++)
        {
            uint32_t* native = BRIGHT_ROW ? bright : dark;
            uint32_t* interp = BRIGHT_ROW ? dark : bright;
            int is_rg = (y % 2 == 0); /* RG or GB? */
            int s = (is_bright[y%4] == is_bright[(y+1)%4]) ? -1 : 1;    /* points to the closest row having different exposure */

            //~ printf("Interpolating %s line %d from [near] %d (squeezed %d) and [far] %d (squeezed %d)\n", BRIGHT_ROW ? "BRIGHT" : "DARK", y, y+s, yh_near, y-2*s, yh_far);
            
            for (int x = 2; x < w-2; x += 2)
            {
                for (int k = 0; k < 2; k++, x++)
                {
                    float** plane = is_rg ? (x%2 == 0 ? red   : green)
                                          : (x%2 == 0 ? green : blue );

                    int dir = edge_direction[x + y*w];
                    
                    int edge_interp(int dir)
                    {
                        
                        int dxa = edge_directions[dir].a.x;
                        int dya = edge_directions[dir].a.y * s;
                        int pa = COERCE((int)plane[squeezed[y+dya]][x+dxa], 0, 0xFFFFF);
                        int dxb = edge_directions[dir].b.x;
                        int dyb = edge_directions[dir].b.y * s;
                        int pb = COERCE((int)plane[squeezed[y+dyb]][x+dxb], 0, 0xFFFFF);
                        int pi = (raw2ev[pa] * 2 + raw2ev[pb]) / 3;
                        
                        return pi;
                    }
                    
                    /* vary the interpolation direction and average the result (reduces aliasing) */
                    int pi0 = edge_interp(dir);
                    int pip = edge_interp(MIN(dir+1, COUNT(edge_directions)-1));
                    int pim = edge_interp(MAX(dir-1,0));
                    
                    interp[x   + y * w] = ev2raw[(2*pi0+pip+pim)/4];
                    native[x   + y * w] = raw_get_pixel32(x, y);
                }
                x -= 2;
            }
        }

        for (int i = 0; i < h; i++)
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
        for (int y = 2; y < h-2; y ++)
        {
            uint32_t* native = BRIGHT_ROW ? bright : dark;
            uint32_t* interp = BRIGHT_ROW ? dark : bright;
            int is_rg = (y % 2 == 0); /* RG or GB? */
            int white = !BRIGHT_ROW ? white_darkened : raw_info.white_level;
            
            for (int x = 2; x < w-3; x += 2)
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
    for (int y = 0; y < 3; y ++)
    {
        uint32_t* native = BRIGHT_ROW ? bright : dark;
        uint32_t* interp = BRIGHT_ROW ? dark : bright;
        
        for (int x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x, y+2);
            native[x + y * w] = raw_get_pixel32(x, y);
        }
    }

    for (int y = h-4; y < h; y ++)
    {
        uint32_t* native = BRIGHT_ROW ? bright : dark;
        uint32_t* interp = BRIGHT_ROW ? dark : bright;
        
        for (int x = 0; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x, y-2);
            native[x + y * w] = raw_get_pixel32(x, y);
        }
    }

    for (int y = 2; y < h; y ++)
    {
        uint32_t* native = BRIGHT_ROW ? bright : dark;
        uint32_t* interp = BRIGHT_ROW ? dark : bright;
        
        for (int x = 0; x < 2; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x, y-2);
            native[x + y * w] = raw_get_pixel32(x, y);
        }

        for (int x = w-3; x < w; x ++)
        {
            interp[x + y * w] = raw_get_pixel32(x-2, y-2);
            native[x + y * w] = raw_get_pixel32(x-2, y);
        }
    }
    
    if (use_stripe_fix)
    {
        printf("Horizontal stripe fix...\n");
        int* delta = malloc(w * sizeof(delta[0]));

        /* adjust dark lines to match the bright ones */
        for (int y = raw_info.active_area.y1; y < raw_info.active_area.y2; y ++)
        {
            /* apply a constant offset (estimated from unclipped areas) */
            int delta_num = 0;
            for (int x = raw_info.active_area.x1; x < raw_info.active_area.x2; x ++)
            {
                int b = bright[x + y*w];
                int d = dark[x + y*w];
                if (MAX(b,d) < white_darkened)
                {
                    delta[delta_num++] = b - d;
                }
            }

            if (delta_num < 200)
            {
                //~ printf("%d: too few points (%d)\n", y, delta_num);
                continue;
            }

            /* compute median difference */
            int med_delta = median_int_wirth(delta, delta_num);

            if (ABS(med_delta) > 200*16)
            {
                printf("%d: offset too large (%d)\n", y, med_delta);
                continue;
            }

            /* shift the dark lines */
            for (int x = 0; x < w; x ++)
            {
                dark[x + y*w] = COERCE(dark[x + y*w] + med_delta, 0, 0xFFFFF);
            }
        }
        free(delta);
    }

    /* reconstruct a full-resolution image (discard interpolated fields whenever possible) */
    /* this has full detail and lowest possible aliasing, but it has high shadow noise and color artifacts when high-iso starts clipping */
    if (use_fullres)
    {
        printf("Full-res reconstruction...\n");
        for (int y = 0; y < h; y ++)
        {
            for (int x = 0; x < w; x ++)
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
    
    for (int i = 0; i < 1<<20; i++)
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
        for (int i = 0; i < 65536; i++)
            fprintf(f, "%f ", log2(MAX(i/4.0 - black/4.0, 1)));
        fprintf(f, "];\n");
        
        fprintf(f, "k = [");
        for (int i = 0; i < 65536; i++)
            fprintf(f, "%f ", mix_curve[i*16]);
        fprintf(f, "];\n");
        
        fprintf(f, "plot(ev, k);\n");
        fprintf(f, "print -dpng mix-curve.png\n");
        fclose(f);
        
        if(system("octave --persist mix-curve.m"));
    }
    
    for (int y = 0; y < h; y ++)
    {
        for (int x = 0; x < w; x ++)
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
        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                raw_set_pixel_20to16(x, y, raw_buffer_32[x + y*w]);
        save_debug_dng("normal.dng");
        raw_info.buffer = raw_buffer_32;

        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                raw_set_pixel_20to16(x, y, bright[x + y*w]);
        save_debug_dng("bright.dng");

        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                raw_set_pixel_20to16(x, y, dark[x + y*w]);
        save_debug_dng("dark.dng");

        if (use_fullres)
        {
            for (int y = 0; y < h; y ++)
                for (int x = 0; x < w; x ++)
                    raw_set_pixel_20to16(x, y, fullres[x + y*w]);
            save_debug_dng("fullres.dng");
        }

        for (int y = 0; y < h; y ++)
            for (int x = 0; x < w; x ++)
                raw_set_pixel_20to16(x, y, halfres[x + y*w]);
        save_debug_dng("halfres.dng");

        if (chroma_smooth_method)
        {
            if (use_fullres)
            {
                for (int y = 0; y < h; y ++)
                    for (int x = 0; x < w; x ++)
                        raw_set_pixel_20to16(x, y, fullres_smooth[x + y*w]);
                save_debug_dng("fullres_smooth.dng");
            }

            for (int y = 0; y < h; y ++)
                for (int x = 0; x < w; x ++)
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
        for (int y = 0; y < h; y ++)
        {
            for (int x = 0; x < w; x ++)
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
            for (int y = 3; y < h-2; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, EV2RAW(alias_map[x + y*w] * 1024));
            save_debug_dng("alias.dng");
        }

        memcpy(alias_aux, alias_map, w * h * sizeof(uint16_t));

        printf("Filtering alias map...\n");
        for (int y = 6; y < h-6; y ++)
        {
            for (int x = 6; x < w-6; x ++)
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
                for (int i = -3; i <= 3; i++)
                {
                    for (int j = -3; j <= 3; j++)
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
            for (int y = 3; y < h-2; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, EV2RAW(alias_aux[x + y*w] * 1024));
            save_debug_dng("alias-dilated.dng");
        }

        printf("Smoothing alias map...\n");
        /* gaussian blur */
        for (int y = 6; y < h-6; y ++)
        {
            for (int x = 6; x < w-6; x ++)
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

                for (int k = 0; k < COUNT(blur_unique); k++)
                {
                    int c = 0;
                    printf("(");
                    for (int dy = -3; dy <= 3; dy++)
                    {
                        for (int dx = -3; dx <= 3; dx++)
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
            for (int y = 3; y < h-2; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, EV2RAW(alias_map[x + y*w] * 128));
            save_debug_dng("alias-smooth.dng");
        }

        /* make it grayscale */
        for (int y = 2; y < h-2; y += 2)
        {
            for (int x = 2; x < w-2; x += 2)
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
            for (int y = 3; y < h-2; y ++)
                for (int x = 2; x < w-2; x ++)
                    raw_set_pixel_20to16(x, y, ev2raw[(long long)alias_map[x + y*w] * 13*EV_RESOLUTION / ALIAS_MAP_MAX]);
            save_debug_dng("alias-filtered.dng");
        }

        free(alias_aux);
    }

    /* where the image is overexposed? */
    overexposed = malloc(w * h * sizeof(uint16_t));
    memset(overexposed, 0, w * h * sizeof(uint16_t));

    for (int y = 0; y < h; y ++)
    {
        for (int x = 0; x < w; x ++)
        {
            overexposed[x + y * w] = bright[x + y * w] >= white_darkened || dark[x + y * w] >= white ? 100 : 0;
        }
    }
    
    /* "blur" the overexposed map */
    uint16_t* over_aux = malloc(w * h * sizeof(uint16_t));
    memcpy(over_aux, overexposed, w * h * sizeof(uint16_t));

    for (int y = 3; y < h-3; y ++)
    {
        for (int x = 3; x < w-3; x ++)
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
    for (int y = 3; y < h-2; y ++)
        for (int x = 2; x < w-2; x ++)
            raw_set_pixel32(x, y, bright[x + y*w]);
    compute_black_noise(8, raw_info.active_area.x1 - 8, raw_info.active_area.y1 + 20, raw_info.active_area.y2 - 20, 1, 1, &noise_avg, &noise_std[0], raw_get_pixel32);
    double ideal_noise_std = noise_std[0];

    printf("Final blending...\n");
    for (int y = 0; y < h; y ++)
    {
        for (int x = 0; x < w; x ++)
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

    /* run a final black subtract pass, to fix whatever our funky processing may do to blacks */
    black_subtract_simple(raw_info.active_area.x1, raw_info.active_area.y1);
    white = raw_info.white_level;
    black = raw_info.black_level;

    /* go back from 20-bit to 16-bit output */
    raw_info.buffer = raw_buffer_16;
    raw_info.black_level /= 16;
    raw_info.white_level /= 16;

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            raw_set_pixel_20to16_rand(x, y, raw_buffer_32[x + y*w]);

    char* AsShotNeutral_method = "default";
    if (exif_wb)
    {
        AsShotNeutral_method = "fixme";
        
        /* fixme: exif WB will not be applied to soft-film curve (will use some dummy values instead) */
        custom_wb[0] = 2;
        custom_wb[1] = 1;
        custom_wb[2] = 2;
    }
    else if (custom_wb[1])
    {
        float red_balance = custom_wb[0]/custom_wb[1];
        float blue_balance = custom_wb[2]/custom_wb[1];
        dng_set_wbgain(1000000, red_balance*1000000, 1, 1, 1000000, blue_balance*1000000);
        AsShotNeutral_method = "custom";
    }
    else /* if (gray_wb) */
    {
        float red_balance = -1, blue_balance = -1;
        white_balance_gray(&red_balance, &blue_balance, gray_wb);
        dng_set_wbgain(1000000, red_balance*1000000, 1, 1, 1000000, blue_balance*1000000);
        custom_wb[0] = red_balance;
        custom_wb[1] = 1;
        custom_wb[2] = blue_balance;
        AsShotNeutral_method = 
            gray_wb == WB_GRAY_MED ? "gray med" : 
            gray_wb == WB_GRAY_MAX ? "gray max" :
             "?"; 
    }

    if (!exif_wb)
    {
        custom_wb[0] /= custom_wb[1];
        custom_wb[2] /= custom_wb[1];
        custom_wb[1] = 1;
        double multipliers[3] = {custom_wb[0], custom_wb[1], custom_wb[2]};
        double temperature, green;
        ufraw_multipliers_to_kelvin_green(multipliers, &temperature, &green);
        printf("AsShotNeutral   : %.2f 1 %.2f, %dK/g=%.2f (%s)\n", 1/custom_wb[0], 1/custom_wb[2], (int)temperature, green, AsShotNeutral_method);
    }

    if (soft_film_ev > 0)
    {
        /* Soft film curve from ufraw */
        double exposure = pow(2, soft_film_ev);

        double baked_wb[3] = {
            custom_wb[0]/custom_wb[1],
            1,
            custom_wb[2]/custom_wb[1],
        };
        
        double max_wb = MAX(baked_wb[0], baked_wb[2]);
        printf("Soft-film curve : +%.2f EV baked at WB %.2f %.2f %.2f\n", log2(exposure), baked_wb[0], baked_wb[1], baked_wb[2]);

        if (0)
        {
            FILE* f = fopen("soft-film.m", "w");
            for (int k = 0; k < 3; k++)
            {
                double wb = baked_wb[k];
                char* rgb = "rgb";
                fprintf(f, "s%c = [", rgb[k]);
                for (int i = 0; i < 1<<20; i++)
                {
                    int raw_compressed = soft_film_bakedwb(i, exposure, black, white, black/16, white/16, wb, max_wb);
                    fprintf(f, "%d ", raw_compressed);
                }
                fprintf(f, "];\n");
            }
            
            fprintf(f, "x = log2(max(1,(1:2^20) - 1 - %d));\n", black);
            fprintf(f, "yr = log2(max(1, sr - %d));\n", black/16);
            fprintf(f, "yg = log2(max(1, sg - %d));\n", black/16);
            fprintf(f, "yb = log2(max(1, sb - %d));\n", black/16);
            fprintf(f, "plot(x, yr, 'r', x, yg, 'g', x, yb, 'b')\n");
            fclose(f);
            if(system("octave --persist soft-film.m"));
        }

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                double wb = baked_wb[FC(x,y)];
                int raw_compressed = soft_film_bakedwb(raw_buffer_32[x + y*w], exposure, black, white, black/16, white/16, wb, max_wb);
                raw_set_pixel16(x, y, COERCE(raw_compressed, 0, 65535));
                
                /* with WB 1/1/1: */
                //~ raw_set_pixel16(x, y, soft_film(raw_buffer_32[x + y*w], exposure, black, white, black/16, white/16));
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

/* filters a monochrome image */
/* kernel: a square with size = 2*radius+1 */
/* complexity: O(w * h * radius) */
static void box_blur(int* img, int* out, int w, int h, int radius)
{
    int area = (2*radius+1) * (2*radius+1);
    
    /* for each row */
    for (int y = radius; y < h-radius; y++)
    {
        int acc = 0;
        int x0 = radius;
        
        /* initial accumulator value for this row */
        for (int dy = -radius; dy <= radius; dy++)
            for (int dx = -radius; dx <= radius; dx++)
                acc += img[x0+dx + (y+dy)*w];
        
        /* scan this row */
        for (int x = radius; x < w-radius-1; x++)
        {
            /* output value for this pixel */
            out[x + y*w] = acc / area;

            /* update accumulator for next pixel, in O(radius) */
            for (int dy = -radius; dy <= radius; dy++)
                acc += img[x + radius + 1 + (y+dy)*w] - img[x - radius + (y+dy)*w];
        }
    }
}

static void white_balance_gray(float* red_balance, float* blue_balance, int method)
{
    int w = raw_info.width;
    int h = raw_info.height;
    int x0 = raw_info.active_area.x1;
    int y0 = raw_info.active_area.y1;
    int black = raw_info.black_level;
    int white = raw_info.white_level;

    /* build a 2D histogram of R-G and B-G, from -5 to +5 EV in 0.02 EV increments */
    #define WB_RANGE 500
    #define WB_EV 50
    #define WB_ORIGIN 250
    
    /* downsample by 16 to get rid of noise and demosaic to make things easier */
    #define WB_DOWN 16
    float* lores[3];
    int wl = w/WB_DOWN;
    int hl = h/WB_DOWN;
    int losize = wl * hl * sizeof(float);
    lores[0] = malloc(losize);
    lores[1] = malloc(losize);
    lores[2] = malloc(losize);
    
    for (int yl = y0/WB_DOWN; yl < hl; yl ++)
    {
        for (int xl = x0/WB_DOWN; xl < wl; xl ++)
        {
            int x = xl * WB_DOWN;
            int y = yl * WB_DOWN;
            int sum[3] = {0, 0, 0};
            int num[3] = {0, 0, 0};
            for (int yy = y; yy < y + WB_DOWN; yy++)
            {
                for (int xx = x; xx < x + WB_DOWN; xx++)
                {
                    int c = FC(yy,xx);
                    sum[c] += raw_get_pixel16(xx, yy) - black;
                    num[c] ++;
                }
            }
            lores[0][xl + yl*wl] = (float) sum[0] / (float) num[0];
            lores[1][xl + yl*wl] = (float) sum[1] / (float) num[1];
            lores[2][xl + yl*wl] = (float) sum[2] / (float) num[2];
        }
    }
    
    int size = WB_RANGE * WB_RANGE * sizeof(int);
    int* hist = malloc(size);
    memset(hist, 0, size);
    
    for (int yl = y0/WB_DOWN; yl < hl; yl ++)
    {
        for (int xl = x0/WB_DOWN; xl < wl; xl ++)
        {
            float r  = lores[0][xl + yl*wl];
            float g  = lores[1][xl + yl*wl];
            float b  = lores[2][xl + yl*wl];

            if (r <= 4 || r >= white-black) continue;
            if (g <= 4 || g >= white-black) continue;
            if (b <= 4 || b >= white-black) continue;
            
            int R = roundf(log2f(r) * WB_EV);
            int G = roundf(log2f(g) * WB_EV);
            int B = roundf(log2f(b) * WB_EV);
            int RG = R - G;
            int BG = B - G;
            
            if (RG < -WB_ORIGIN || RG >= WB_ORIGIN) continue;
            if (BG < -WB_ORIGIN || BG >= WB_ORIGIN) continue;
            
            hist[(RG + WB_ORIGIN) + (BG + WB_ORIGIN) * WB_RANGE] += 1000;
        }
    }

    int rbest = WB_ORIGIN - WB_EV;
    int bbest = WB_ORIGIN - WB_EV;

    int* histblur = malloc(size);
    memcpy(histblur, hist, size);

    if (method == WB_GRAY_MED)
    {
        /* use median values for R-G and B-G */
        int total = 0;
        for (int b = 0; b < WB_RANGE; b++)
        {
            for (int r = 0; r < WB_RANGE; r++)
            {
                total += histblur[r + b*WB_RANGE];
            }
        }

        int acc = 0;
        for (int b = 0; b < WB_RANGE; b++)
        {
            for (int r = 0; r < WB_RANGE; r++)
            {
                acc += histblur[r + b*WB_RANGE];
            }
            if (acc > total/2)
            {
                bbest = b;
                break;
            }
        }
        
        acc = 0;
        for (int r = 0; r < WB_RANGE; r++)
        {
            for (int b = 0; b < WB_RANGE; b++)
            {
                acc += histblur[r + b*WB_RANGE];
            }
            if (acc > total/2)
            {
                rbest = r;
                break;
            }
        }
    }
    else if (method == WB_GRAY_MAX)
    {
        /* scan for the WB value that maximizes the number of gray pixels, within a given color tolerance */
        int tol = 2;

        for (int k = 0; k < 3; k++)
        {
            box_blur(hist, histblur, WB_RANGE, WB_RANGE, tol);
            memcpy(hist, histblur, size);
        }

        if (1)
        {
            /* prefer daylight WB */
            double gains[3];
            ufraw_kelvin_green_to_multipliers(5000, 1, gains);

            for (int b = 0; b < WB_RANGE; b++)
            {
                for (int r = 0; r < WB_RANGE; r++)
                {
                    int dr = r - (WB_ORIGIN - log2(gains[0]) * WB_EV);
                    int db = b - (WB_ORIGIN - log2(gains[2]) * WB_EV);
                    float d = sqrt(dr*dr + db*db) / (WB_EV * WB_EV);
                    float weight = 1000 * exp(-(d*d)*500);
                    histblur[r + b*WB_RANGE] = log2(1 + histblur[r + b*WB_RANGE]) * weight;
                }
            }
        }
        int max = 0;
        for (int b = tol; b < WB_RANGE-tol; b++)
        {
            for (int r = tol; r < WB_RANGE-tol; r++)
            {
                if (histblur[r + b*WB_RANGE] > max)
                {
                    max = histblur[r + b*WB_RANGE];
                    rbest = r;
                    bbest = b;
                }
            }
        }
    }

    if (debug_wb)
    {
        FILE* f = fopen("wb.m", "w");
        for (int k = 0; k < 3; k++)
        {
            fprintf(f, "lores(:,:,%d) = [\n", k+1);
            for (int yl = 0; yl < hl; yl++)
            {
                for (int xl = 0; xl < wl; xl++)
                {
                    fprintf(f, "%f ", lores[k][xl + yl*wl]);
                }
                fprintf(f, "\n");
            }
            fprintf(f, "];\n");
        }

        fprintf(f, "histblur = [\n");
        for (int b = 0; b < WB_RANGE; b++)
        {
            for (int r = 0; r < WB_RANGE; r++)
            {
                fprintf(f, "%d ", histblur[r + b*WB_RANGE]);
            }
            fprintf(f, "\n");
        }
        fprintf(f, "];\n");
        fprintf(f, "kelvin = [");
        for (int k = 1000; k <= 20000; k += 100)
        {
            double gains[3];
            ufraw_kelvin_green_to_multipliers(k, 1, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(k, 2, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(k, 0.5, gains);
            fprintf(f, "%f %f\n", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
        }
        fprintf(f, "];\n");
        fprintf(f, "gm = [");
        for (float g = -2; g <= 2; g += 0.1)
        {
            double glin = powf(2, g);
            double gains[3];
            ufraw_kelvin_green_to_multipliers(1000, glin, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(2000, glin, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(3000, glin, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(5000, glin, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(7000, glin, gains);
            fprintf(f, "%f %f ", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
            ufraw_kelvin_green_to_multipliers(20000, glin, gains);
            fprintf(f, "%f %f\n", WB_ORIGIN - log2(gains[0]) * WB_EV, WB_ORIGIN - log2(gains[2]) * WB_EV);
        }
        fprintf(f, "];\n");
        fprintf(f, "imshow(histblur,[]); colormap jet; hold on;\n");
        fprintf(f, "rbest = %d; bbest = %d;\n", rbest+1, bbest+1);
        fprintf(f, "plot(rbest, bbest, '.w')\n");
        fprintf(f, "plot([-5.01, 5.01]*%d+%d, [bbest, bbest], 'color', 'white')\n", WB_EV, WB_ORIGIN);
        fprintf(f, "plot([rbest, rbest], [-5, 5]*%d+%d,'color', 'white')\n", WB_EV, WB_ORIGIN);

        fprintf(f, "kred = kelvin(:,1); kblue = kelvin(:,2);\n");
        fprintf(f, "kredg = kelvin(:,3); kblueg = kelvin(:,4);\n");
        fprintf(f, "kredm = kelvin(:,5); kbluem = kelvin(:,6);\n");
        fprintf(f, "plot(kred, kblue, 'w', 'linewidth', 4)\n");
        fprintf(f, "plot(kred(1:10:end), kblue(1:10:end), '.w')\n");
        fprintf(f, "plot(kredg, kblueg, 'm', 'linewidth', 2)\n");
        fprintf(f, "plot(kredg(1:10:end), kblueg(1:10:end), '.m')\n");
        fprintf(f, "plot(kredm, kbluem, 'g', 'linewidth', 2)\n");
        fprintf(f, "plot(kredm(1:10:end), kbluem(1:10:end), '.g')\n");
        fprintf(f, "gred1k = gm(:,1); gblue1k = gm(:,2);\n");
        fprintf(f, "gred2k = gm(:,3); gblue2k = gm(:,4);\n");
        fprintf(f, "gred3k = gm(:,5); gblue3k = gm(:,6);\n");
        fprintf(f, "gred5k = gm(:,7); gblue5k = gm(:,8);\n");
        fprintf(f, "gred7k = gm(:,9); gblue7k = gm(:,10);\n");
        fprintf(f, "gred20k = gm(:,11); gblue20k = gm(:,12);\n");
        fprintf(f, "plot(gred1k, gblue1k, 'r')\n");
        fprintf(f, "plot(gred2k, gblue2k, 'r', 'linewidth', 2)\n");
        fprintf(f, "plot(gred3k, gblue3k, 'y', 'linewidth', 2)\n");
        fprintf(f, "plot(gred5k, gblue5k, 'w', 'linewidth', 4)\n");
        fprintf(f, "plot(gred7k, gblue7k, 'b', 'linewidth', 2)\n");
        fprintf(f, "plot(gred20k, gblue20k, 'b')\n");
        fprintf(f, "text(gred1k(end)+1, gblue1k(end)-2, '1000K', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(gred2k(end)+1, gblue2k(end)-2, '2000K', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(gred3k(end)+1, gblue3k(end)-2, '3000K', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(gred5k(end)+1, gblue5k(end)+3, '5000K', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(gred7k(end)+1, gblue7k(end)+2, '7000K', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(gred20k(end)+1, gblue20k(end)+2, '20000K', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(kredg(end)-5, kblueg(end)+5, 'g=2', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(kred(end)-5, kblue(end)+5, 'g=1', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "text(kredm(end)-10, kbluem(end)+5, 'g=0.5', 'color', 'white', 'fontweight', 'bold')\n");
        fprintf(f, "xlabel('red balance (-3..3 EV)'); ylabel('blue balance (3..-3 EV)')\n");

        fprintf(f, "set(gca,'position',[0 0.04 1 0.96])\n");
        fprintf(f, "print -dpng wb.png\n");
        fclose(f);
        if(system("octave wb.m"));
    }
    
    *red_balance = powf(2, (float) -(rbest - WB_ORIGIN) / WB_EV);
    *blue_balance = powf(2, (float) -(bbest - WB_ORIGIN) / WB_EV);
    
    free(lores[0]);
    free(lores[1]);
    free(lores[2]);
    free(histblur);
    free(hist);
}
