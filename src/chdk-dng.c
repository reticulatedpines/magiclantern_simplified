/**
 * DNG saving routines ported from CHDK
 * Code stripped down a bit, since we don't care about GPS and advanced exif stuff (at least for now)
 * 
 * TODO: make it platform-independent and move it to modules.
 */

/*
 * Original code copyright (C) CHDK (GPLv2); ported to Magic Lantern (2013)
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


#undef RAW_DEBUG_BLACK

#ifdef CONFIG_MAGICLANTERN
#include "dryos.h"
#include "property.h"
#include "math.h"
#include "string.h"
#define umalloc fio_malloc
#define ufree fio_free
#define pow powf

static int32_t get_tick_count() { return get_ms_clock(); }

#else // if we compile it for desktop
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "math.h"
#include <sys/types.h>
#define FAST
#define UNCACHEABLE(x) (x)
#define umalloc malloc
#define ufree free

#define FIO_CreateFile(name) fopen(name, "wb")
#define FIO_WriteFile(f, ptr, count) fwrite(ptr, 1, count, f)
#define FIO_CloseFile(f) fclose(f)
#define FIO_RemoveFile(f) unlink(f)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

#define INVALID_PTR             ((void *)0xFFFFFFFF)

#endif

#include "raw.h"
#include "chdk-dng.h"

/* adaptations from CHDK to ML */
#define camera_sensor (*raw_info)
#define get_raw_pixel raw_get_pixel
#define raw_rowpix width
#define raw_rows height
#define raw_size frame_size
#define write FIO_WriteFile

void FAST reverse_bytes_order(char* buf, int32_t count)
{
#ifdef __ARM__
    /* optimized swap from g3gg0 */
    asm volatile ("\
        /* r2 = A B C D */\r\n\
        /* r5 = 0 B 0 D */\r\n\
        /* r2 = 0 A 0 C */\r\n\
        /* r2 = B 0 D 0 | 0 A 0 C */\r\n\
        \
        /* init swap mask */\r\n\
        mov r4, #0xff\r\n\
        orr r4, r4, #0xff0000\r\n\
        \
        _wswap128:\r\n\
        cmp %1, #0x10\r\n\
        blt _wswap64\r\n\
        \
        ldmia %0, {r2, r3, r6, r7}\r\n\
        and r5, r4, r2\r\n\
        and r2, r4, r2, ror #8\r\n\
        orr r2, r2, r5, lsl #8\r\n\
       and r5, r4, r3\r\n\
        and r3, r4, r3, ror #8\r\n\
        orr r3, r3, r5, lsl #8\r\n\
        and r5, r4, r6\r\n\
        and r6, r4, r6, ror #8\r\n\
        orr r6, r6, r5, lsl #8\r\n\
        and r5, r4, r7\r\n\
        and r7, r4, r7, ror #8\r\n\
        orr r7, r7, r5, lsl #8\r\n\
        stmia %0!, {r2, r3, r6, r7}\r\n\
        sub %1, #0x10\r\n\
        b _wswap128\r\n\
        \
        _wswap64:\r\n\
        cmp %1, #0x08\r\n\
        blt _wswap32\r\n\
        \
        ldmia %0, {r2, r3}\r\n\
        and r5, r4, r2\r\n\
        and r2, r4, r2, ror #8\r\n\
        orr r2, r2, r5, lsl #8\r\n\
        and r5, r4, r3\r\n\
        and r3, r4, r3, ror #8\r\n\
        orr r3, r3, r5, lsl #8\r\n\
        stmia %0!, {r2, r3}\r\n\
        sub %1, #0x08\r\n\
        b _wswap64\r\n\
        \
        _wswap32:\r\n\
        cmp %1, #0x04\r\n\
        blt _wswap16\r\n\
        \
        ldmia %0, {r2}\r\n\
        and r5, r4, r2\r\n\
        and r2, r4, r2, ror #8\r\n\
        orr r2, r2, r5, lsl #8\r\n\
        stmia %0!, {r2}\r\n\
        sub %1, #0x04\r\n\
        b _wswap32\r\n\
        \
        _wswap16:\r\n\
        cmp %1, #0x00\r\n\
        beq _wswap_end\r\n\
        ldrh r2, [%0]\r\n\
        mov r3, r2, lsr #8\r\n\
        orr r2, r3, r2, lsl #8\r\n\
        strh r2, [%0]\r\n\
        \
        _wswap_end:\
        " : : "r"(buf), "r"(count) : "r2", "r3", "r4", "r5", "r6", "r7");
#else
    int16_t* buf16 = (int16_t*) buf;
    int32_t i;
    for (i = 0; i < count/2; i++)
    {
        int16_t x = buf16[i];
        buf[2*i+1] = x;
        buf[2*i] = x >> 8;
    }
#endif
}

//thumbnail
static int32_t dng_th_width = 128;
static int32_t dng_th_height = 84;
// higly recommended that dng_th_width*dng_th_height would be divisible by 512

/* warning: not thread safe */
void dng_set_thumbnail_size(int32_t width, int32_t height)
{
    dng_th_width = width;
    dng_th_height = height;
}

struct dir_entry{uint16_t tag; uint16_t type; uint32_t count; unsigned int offset;};

#define T_BYTE      1
#define T_ASCII     2
#define T_SHORT     3
#define T_LONG      4
#define T_RATIONAL  5
#define T_SBYTE     6
#define T_UNDEFINED 7
#define T_SSHORT    8
#define T_SLONG     9
#define T_SRATIONAL 10
#define T_FLOAT     11
#define T_DOUBLE    12
#define T_PTR       0x100   // Stored as long/int16_t etc in DNG header, referenced by pointer in IFD (must be pointer to int32_t variable)
#define T_SKIP      0x200   // Tag value to be skipped (for marking GPS entries if camera does not have GPS)

static const int32_t cam_BaselineNoise[]           = {1,1};
static const int32_t cam_BaselineSharpness[]       = {4,3};
static const int32_t cam_LinearResponseLimit[]     = {1,1};
static const int32_t cam_AnalogBalance[]           = {1,1,1,1,1,1};
static char dng_lens_model[64]              = "";
static char dng_image_desc[64]              = "";
static char cam_name[32]                    = "Canikon";
static char cam_serial[64]                  = "";
static char dng_artist_name[64]             = "";
static char dng_copyright[64]               = "";
static const int16_t cam_PreviewBitsPerSample[]  = {8,8,8};
static const int32_t cam_Resolution[]              = {180,1};
static int32_t cam_AsShotNeutral[6]         = {473635,1000000,1000000,1000000,624000,1000000}; // wbgain default: daylight
static char cam_datetime[20]            = "";                   // DateTimeOriginal
static char cam_subsectime[4]           = "";                   // DateTimeOriginal (milliseconds component)
static int32_t cam_shutter[2]               = { 0, 1000000 };       // Shutter speed
static int32_t cam_aperture[2]              = { 0, 10 };            // Aperture
static int32_t cam_apex_shutter[2]          = { 0, 96 };            // Shutter speed in APEX units
static int32_t cam_apex_aperture[2]         = { 0, 96 };            // Aperture in APEX units
static int32_t cam_exp_bias[2]              = { 0, 96 };
static int32_t cam_max_av[2]                = { 0, 96 };
static int32_t cam_focal_length[2]          = { 0, 1000 };
static char* software_ver = "Magic Lantern";
static int32_t cam_FrameRate[]          = {25000,1000};

struct t_data_for_exif{
    int16_t iso;
    int32_t exp_program;
    int32_t effective_focal_length;
    int16_t orientation;
    int16_t flash_mode;
    int16_t flash_fired;
    int16_t metering_mode;
};


#define BE(v)   ((v&0x000000FF)<<24)|((v&0x0000FF00)<<8)|((v&0x00FF0000)>>8)|((v&0xFF000000)>>24)   // Convert to big_endian

#define BADPIX_CFA_INDEX    6   // Index of CFAPattern value in badpixel_opcodes array

static uint32_t badpixel_opcode[] =
{
    // *** all values must be in big endian order

    BE(1),              // Count = 1

    BE(4),              // FixBadPixelsConstant = 4
    BE(0x01030000),     // DNG version = 1.3.0.0
    BE(1),              // Flags = 1
    BE(8),              // Opcode length = 8 bytes
    BE(0),              // Constant = 0
    BE(0),              // CFAPattern (set in code below)
};


static struct t_data_for_exif exif_data;

#define BE(v)   ((v&0x000000FF)<<24)|((v&0x0000FF00)<<8)|((v&0x00FF0000)>>8)|((v&0xFF000000)>>24)   // Convert to big_endian

// warning: according to TIFF format specification, elements must be sorted by tag value in ascending order!

// Index of specific entries in ifd0 below.
static int32_t find_tag_index(struct dir_entry * ifd, int32_t num, uint16_t tag)
{
    int32_t i;
    for (i = 0; i < num; i++)
        if (ifd[i].tag == tag)
            return i;
    
    /* should be unreachable */
#ifdef CONFIG_MAGICLANTERN
    ASSERT(0);
#else
    exit(1);
#endif
    while(1);
    return -1;
}

// Index of specific entries in ifd0 below.
#define CAMERA_NAME_INDEX           find_tag_index(ifd0, DIR_SIZE(ifd0), 0x110)
#define THUMB_DATA_INDEX            find_tag_index(ifd0, DIR_SIZE(ifd0), 0x111)
#define ORIENTATION_INDEX           find_tag_index(ifd0, DIR_SIZE(ifd0), 0x112)
#define CHDK_VER_INDEX              find_tag_index(ifd0, DIR_SIZE(ifd0), 0x131)
#define ARTIST_NAME_INDEX           find_tag_index(ifd0, DIR_SIZE(ifd0), 0x13B)
#define SUBIFDS_INDEX               find_tag_index(ifd0, DIR_SIZE(ifd0), 0x14A)
#define COPYRIGHT_INDEX             find_tag_index(ifd0, DIR_SIZE(ifd0), 0x8298)
#define EXIF_IFD_INDEX              find_tag_index(ifd0, DIR_SIZE(ifd0), 0x8769)
#define DNG_VERSION_INDEX           find_tag_index(ifd0, DIR_SIZE(ifd0), 0xC612)
#define UNIQUE_CAMERA_MODEL_INDEX   find_tag_index(ifd0, DIR_SIZE(ifd0), 0xC614)

#define CAM_MAKE                    "Canon"


// Index of specific entries in ifd1 below.
#define RAW_DATA_INDEX              find_tag_index(ifd1, DIR_SIZE(ifd1), 0x111)
#define BADPIXEL_OPCODE_INDEX       find_tag_index(ifd1, DIR_SIZE(ifd1), 0xC740)

// Index of specific entries in exif_ifd below.
#define EXPOSURE_PROGRAM_INDEX      find_tag_index(exif_ifd, DIR_SIZE(exif_ifd), 0x8822)
#define METERING_MODE_INDEX         find_tag_index(exif_ifd, DIR_SIZE(exif_ifd), 0x9207)
#define FLASH_MODE_INDEX            find_tag_index(exif_ifd, DIR_SIZE(exif_ifd), 0x9209)
#define SSTIME_INDEX                find_tag_index(exif_ifd, DIR_SIZE(exif_ifd), 0x9290)
#define SSTIME_ORIG_INDEX           find_tag_index(exif_ifd, DIR_SIZE(exif_ifd), 0x9291)

static int32_t get_type_size(int32_t type)
{
    switch(type & 0xFF)
    {
    case T_BYTE:
    case T_SBYTE:
    case T_UNDEFINED:
    case T_ASCII:     return 1; 
    case T_SHORT:
    case T_SSHORT:    return 2;
    case T_LONG:
    case T_SLONG:
    case T_FLOAT:     return 4;
    case T_RATIONAL:
    case T_SRATIONAL:
    case T_DOUBLE:    return 8;
    default:          return 0;
    }
}

#define DIR_SIZE(ifd)   (sizeof(ifd)/sizeof(ifd[0]))

#define TIFF_HDR_SIZE (8)

static char* dng_header_buf;
static int32_t dng_header_buf_size;
static int32_t dng_header_buf_offset;
static char *thumbnail_buf;

static void add_to_buf(void* var, int32_t size)
{
    memcpy(dng_header_buf+dng_header_buf_offset,var,size);
    dng_header_buf_offset += size;
}

static void add_val_to_buf(int32_t val, int32_t size)
{
    add_to_buf(&val,size);
}


void dng_set_camname(char *str)
{
    strncpy(cam_name, str, sizeof(cam_name));
}

void dng_set_camserial(char *str)
{
    strncpy(cam_serial, str, sizeof(cam_serial));
}

void dng_set_description(char *str)
{
    strncpy(dng_image_desc, str, sizeof(dng_image_desc));
}

void dng_set_lensmodel(char *str)
{
    strncpy(dng_lens_model, str, sizeof(dng_lens_model));
}

void dng_set_focal(int32_t nom, int32_t denom)
{
    cam_focal_length[0] = nom;
    cam_focal_length[1] = denom;
}

void dng_set_aperture(int32_t nom, int32_t denom)
{
    cam_aperture[0] = nom;
    cam_aperture[1] = denom;
}

void dng_set_shutter(int32_t nom, int32_t denom)
{
    cam_shutter[0] = nom;
    cam_shutter[1] = denom;
}

void dng_set_framerate(int32_t fpsx1000)
{
    cam_FrameRate[0] = fpsx1000;
    cam_FrameRate[1] = 1000;
}

void dng_set_framerate_rational(int32_t nom, int32_t denom)
{
    cam_FrameRate[0] = nom;
    cam_FrameRate[1] = denom;
}

void dng_set_iso(int32_t value)
{
    exif_data.iso = value;
}

void dng_set_wbgain(int32_t gain_r_n, int32_t gain_r_d, int32_t gain_g_n, int32_t gain_g_d, int32_t gain_b_n, int32_t gain_b_d)
{
    cam_AsShotNeutral[0] = gain_r_n;
    cam_AsShotNeutral[1] = gain_r_d;
    cam_AsShotNeutral[2] = gain_g_n;
    cam_AsShotNeutral[3] = gain_g_d;
    cam_AsShotNeutral[4] = gain_b_n;
    cam_AsShotNeutral[5] = gain_b_d;
}

void dng_set_datetime(char *datetime, char *subsectime)
{
    strncpy(cam_datetime, datetime, sizeof(cam_datetime));
    strncpy(cam_subsectime, subsectime, sizeof(cam_subsectime));
}

static int32_t is_lossless_jpeg(struct raw_info * raw_info)
{
    return *(uint32_t*)(raw_info->buffer) == 0xC4FFD8FF;
}


static void create_dng_header(struct raw_info * raw_info){
    int32_t i,j;
    int32_t extra_offset;
    int32_t raw_offset;

    int32_t dng_compression = 1;

    if (is_lossless_jpeg(raw_info))
    {
        dng_compression = 7; /* JPEG */
    }

    struct dir_entry ifd0[]={
        {0xFE,   T_LONG,       1,  1},                                 // NewSubFileType: Preview Image
        {0x100,  T_LONG,       1,  dng_th_width},                      // ImageWidth
        {0x101,  T_LONG,       1,  dng_th_height},                     // ImageLength
        {0x102,  T_SHORT,      3,  (intptr_t)cam_PreviewBitsPerSample},     // BitsPerSample: 8,8,8
        {0x103,  T_SHORT,      1,  1},                                 // Compression: Uncompressed
        {0x106,  T_SHORT,      1,  2},                                 // PhotometricInterpretation: RGB
        {0x10E,  T_ASCII|T_PTR,      sizeof(dng_image_desc), (intptr_t)dng_image_desc},               // ImageDescription
        {0x10F,  T_ASCII|T_PTR,      sizeof(CAM_MAKE), (intptr_t)CAM_MAKE},       // Make
        {0x110,  T_ASCII|T_PTR,      32, (intptr_t)cam_name},                     // Model: Filled at header generation.
        {0x111,  T_LONG,       1,  0},                                 // StripOffsets: Offset
        {0x112,  T_SHORT,      1,  1},                                 // Orientation: 1 - 0th row is top, 0th column is left
        {0x115,  T_SHORT,      1,  3},                                 // SamplesPerPixel: 3
        {0x116,  T_SHORT,      1,  dng_th_height},                     // RowsPerStrip
        {0x117,  T_LONG,       1,  dng_th_width*dng_th_height*3},      // StripByteCounts = preview size
        {0x11C,  T_SHORT,      1,  1},                                 // PlanarConfiguration: 1
        {0x131,  T_ASCII|T_PTR,32, (intptr_t)0},                                 // Software
        {0x132,  T_ASCII|T_PTR,      20, (intptr_t)cam_datetime},                 // DateTime
        {0x13B,  T_ASCII|T_PTR,64, (intptr_t)dng_artist_name},              // Artist: Filled at header generation.
        {0x14A,  T_LONG,       1,  0},                                 // SubIFDs offset
        {0x8298, T_ASCII|T_PTR,64, (intptr_t)dng_copyright},                // Copyright
        {0x8769, T_LONG,       1,  0},                                 // EXIF_IFD offset
        {0x9216, T_BYTE,       4,  0x00000001},                        // TIFF/EPStandardID: 1.0.0.0
        {0xA431, T_ASCII|T_PTR,      sizeof(cam_serial), (intptr_t)cam_serial},         // Exif.Photo.BodySerialNumber
        {0xA434, T_ASCII|T_PTR,      sizeof(dng_lens_model), (intptr_t)dng_lens_model}, // Exif.Photo.LensModel
        {0xC612, T_BYTE,       4,  0x00000301},                        // DNGVersion: 1.3.0.0
        {0xC613, T_BYTE,       4,  0x00000301},                        // DNGBackwardVersion: 1.1.0.0
        {0xC614, T_ASCII|T_PTR,      32, (intptr_t)cam_name},                     // UniqueCameraModel. Filled at header generation.
        {0xC621, T_SRATIONAL,  9,  (intptr_t)&camera_sensor.color_matrix1},
        {0xC627, T_RATIONAL,   3,  (intptr_t)cam_AnalogBalance},
        {0xC628, T_RATIONAL,   3,  (intptr_t)cam_AsShotNeutral},
        {0xC62A, T_SRATIONAL,  1,  (intptr_t)&camera_sensor.exposure_bias},
        {0xC62B, T_RATIONAL,   1,  (intptr_t)cam_BaselineNoise},
        {0xC62C, T_RATIONAL,   1,  (intptr_t)cam_BaselineSharpness},
        {0xC62E, T_RATIONAL,   1,  (intptr_t)cam_LinearResponseLimit},
        {0xC65A, T_SHORT,      1, 21},                                 // CalibrationIlluminant1 D65
        // {0xC65B, T_SHORT,      1, 21},                                 // CalibrationIlluminant2 D65 (change this if ColorMatrix2 is added); see issue #2343
        {0xC764, T_SRATIONAL,  1,  (intptr_t)cam_FrameRate},
    };

    struct dir_entry ifd1[]={
        {0xFE,   T_LONG,       1,  0},                                 // NewSubFileType: Main Image
        {0x100,  T_LONG|T_PTR, 1,  (intptr_t)&camera_sensor.raw_rowpix},    // ImageWidth
        {0x101,  T_LONG|T_PTR, 1,  (intptr_t)&camera_sensor.raw_rows},      // ImageLength
        {0x102,  T_SHORT|T_PTR,1,  (intptr_t)&camera_sensor.bits_per_pixel},// BitsPerSample
        {0x103,  T_SHORT,      1,  dng_compression},                   // Compression: Uncompressed or JPEG
        {0x106,  T_SHORT,      1,  0x8023},                            // PhotometricInterpretation: CFA
        {0x111,  T_LONG,       1,  0},                                 // StripOffsets: Offset
        {0x115,  T_SHORT,      1,  1},                                 // SamplesPerPixel: 1
        {0x116,  T_SHORT|T_PTR,1,  (intptr_t)&camera_sensor.raw_rows},      // RowsPerStrip
        {0x117,  T_LONG|T_PTR, 1,  (intptr_t)&camera_sensor.raw_size},      // StripByteCounts = CHDK RAW size
        {0x11A,  T_RATIONAL,   1,  (intptr_t)cam_Resolution},               // XResolution
        {0x11B,  T_RATIONAL,   1,  (intptr_t)cam_Resolution},               // YResolution
        {0x11C,  T_SHORT,      1,  1},                                 // PlanarConfiguration: 1
        {0x128,  T_SHORT,      1,  2},                                 // ResolutionUnit: inch
        {0x828D, T_SHORT,      2,  0x00020002},                        // CFARepeatPatternDim: Rows = 2, Cols = 2
        {0x828E, T_BYTE|T_PTR, 4,  (intptr_t)&camera_sensor.cfa_pattern},
        {0xC61A, T_LONG|T_PTR, 1,  (intptr_t)&camera_sensor.black_level},   // BlackLevel
        {0xC61D, T_LONG|T_PTR, 1,  (intptr_t)&camera_sensor.white_level},   // WhiteLevel
        {0xC61F, T_LONG,       2,  (intptr_t)&camera_sensor.crop.origin},
        {0xC620, T_LONG,       2,  (intptr_t)&camera_sensor.crop.size},
        {0xC68D, T_LONG,       4,  (intptr_t)&camera_sensor.dng_active_area},
        {0xC740, T_UNDEFINED|T_PTR, sizeof(badpixel_opcode),  (intptr_t)&badpixel_opcode},
    };

    struct dir_entry exif_ifd[]={
        {0x829A, T_RATIONAL,   1,  (intptr_t)cam_shutter},          // Shutter speed
        {0x829D, T_RATIONAL,   1,  (intptr_t)cam_aperture},         // Aperture
        {0x8822, T_SHORT,      1,  0},                         // ExposureProgram
        {0x8827, T_SHORT|T_PTR,1,  (intptr_t)&exif_data.iso},       // ISOSpeedRatings
        {0x9000, T_UNDEFINED,  4,  0x31323230},                // ExifVersion: 2.21
        {0x9003, T_ASCII|T_PTR,      20, (intptr_t)cam_datetime},         // DateTimeOriginal
        //{0x9201, T_SRATIONAL,  1,  (intptr_t)cam_apex_shutter},     // ShutterSpeedValue (APEX units)
        //{0x9202, T_RATIONAL,   1,  (intptr_t)cam_apex_aperture},    // ApertureValue (APEX units)
        {0x9204, T_SRATIONAL,  1,  (intptr_t)cam_exp_bias},         // ExposureBias
        {0x9205, T_RATIONAL,   1,  (intptr_t)cam_max_av},           // MaxApertureValue
        {0x9207, T_SHORT,      1,  0},                         // Metering mode
        {0x9209, T_SHORT,      1,  0},                         // Flash mode
        {0x920A, T_RATIONAL,   1,  (intptr_t)cam_focal_length},     // FocalLength
        {0x9290, T_ASCII|T_PTR,4,  (intptr_t)cam_subsectime},       // DateTime milliseconds
        {0x9291, T_ASCII|T_PTR,4,  (intptr_t)cam_subsectime},       // DateTimeOriginal milliseconds
        {0xA405, T_SHORT|T_PTR,1,  (intptr_t)&exif_data.effective_focal_length},    // FocalLengthIn35mmFilm
    };

    struct
    {
        struct dir_entry* entry;
        int32_t count;                  // Number of entries to be saved
        int32_t entry_count;            // Total number of entries
    } ifd_list[] = 
    {
        {ifd0,      DIR_SIZE(ifd0),     DIR_SIZE(ifd0)}, 
        {ifd1,      DIR_SIZE(ifd1),     DIR_SIZE(ifd1)}, 
        {exif_ifd,  DIR_SIZE(exif_ifd), DIR_SIZE(exif_ifd)}, 
    };

    ifd0[DNG_VERSION_INDEX].offset = BE(0x01030000);
    
    ifd1[BADPIXEL_OPCODE_INDEX].type &= ~T_SKIP;
        // Set CFAPattern value
        switch (camera_sensor.cfa_pattern)
        {
        case 0x02010100:
            badpixel_opcode[BADPIX_CFA_INDEX] = BE(1);              // BayerPhase = 1 (top left pixel is green in a green/red row)
            break;
        case 0x01020001:
            badpixel_opcode[BADPIX_CFA_INDEX] = BE(0);              // BayerPhase = 0 (top left pixel is red)
            break;
        case 0x01000201:
            badpixel_opcode[BADPIX_CFA_INDEX] = BE(3);              // BayerPhase = 3 (top left pixel is blue)
            break;
        case 0x00010102:
            badpixel_opcode[BADPIX_CFA_INDEX] = BE(2);              // BayerPhase = 2 (top left pixel is green in a green/blue row)
            break;
        }

    // filling EXIF fields
    int32_t ifd_count = DIR_SIZE(ifd_list);

    // Fix the counts and offsets where needed
    ifd0[CAMERA_NAME_INDEX].count = ifd0[UNIQUE_CAMERA_MODEL_INDEX].count = strlen(cam_name) + 1;
    ifd0[CHDK_VER_INDEX].offset = (intptr_t)software_ver;
    ifd0[CHDK_VER_INDEX].count = strlen(software_ver) + 1;
    ifd0[ARTIST_NAME_INDEX].count = strlen(dng_artist_name) + 1;
    ifd0[COPYRIGHT_INDEX].count = strlen(dng_copyright) + 1;
    //~ ifd0[ORIENTATION_INDEX].offset = get_orientation_for_exif(exif_data.orientation);

    //~ exif_ifd[EXPOSURE_PROGRAM_INDEX].offset = get_exp_program_for_exif(exif_data.exp_program);
    //~ exif_ifd[METERING_MODE_INDEX].offset = get_metering_mode_for_exif(exif_data.metering_mode);
    //~ exif_ifd[FLASH_MODE_INDEX].offset = get_flash_mode_for_exif(exif_data.flash_mode, exif_data.flash_fired);
    //~ exif_ifd[SSTIME_INDEX].count = exif_ifd[SSTIME_ORIG_INDEX].count = strlen(cam_subsectime)+1;

    // calculating offset of RAW data and count of entries for each IFD
    raw_offset=TIFF_HDR_SIZE;

    for (j=0;j<ifd_count;j++)
    {
        raw_offset+=6; // IFD header+footer
        for(i=0; i<ifd_list[j].entry_count; i++)
        {
            if ((ifd_list[j].entry[i].type & T_SKIP) == 0)  // Exclude skipped entries (e.g. GPS info if camera doesn't have GPS)
            {
                raw_offset+=12; // IFD directory entry size
                int32_t size_ext=get_type_size(ifd_list[j].entry[i].type)*ifd_list[j].entry[i].count;
                if (size_ext>4) raw_offset+=size_ext+(size_ext&1);
            }
        }
    }

    // creating buffer for writing data
    raw_offset=(raw_offset/512+1)*512; // exlusively for CHDK fast file writing
    dng_header_buf_size=raw_offset;
    dng_header_buf=umalloc(raw_offset);
    dng_header_buf_offset=0;
    if (!dng_header_buf) return;

    // create buffer for thumbnail
    thumbnail_buf = malloc(dng_th_width*dng_th_height*3);
    if (!thumbnail_buf)
    {
        ufree(dng_header_buf);
        dng_header_buf = 0;
        return;
    }

    //  writing offsets for EXIF IFD and RAW data and calculating offset for extra data

    extra_offset=TIFF_HDR_SIZE;

    ifd0[SUBIFDS_INDEX].offset = TIFF_HDR_SIZE + ifd_list[0].count * 12 + 6;                            // SubIFDs offset
    ifd0[EXIF_IFD_INDEX].offset = TIFF_HDR_SIZE + (ifd_list[0].count + ifd_list[1].count) * 12 + 6 + 6; // EXIF IFD offset
    ifd0[THUMB_DATA_INDEX].offset = raw_offset;                                     //StripOffsets for thumbnail
    ifd1[RAW_DATA_INDEX].offset = raw_offset + dng_th_width * dng_th_height * 3;    //StripOffsets for main image

    for (j=0;j<ifd_count;j++)
    {
        extra_offset += 6 + ifd_list[j].count * 12; // IFD header+footer
    }

    // TIFF file header

    add_val_to_buf(0x4949, sizeof(int16_t));      // little endian
    add_val_to_buf(42, sizeof(int16_t));          // An arbitrary but carefully chosen number that further identifies the file as a TIFF file.
    add_val_to_buf(TIFF_HDR_SIZE, sizeof(int32_t)); // offset of first IFD

    // writing IFDs

    for (j=0;j<ifd_count;j++)
    {
        int32_t size_ext;
        add_val_to_buf(ifd_list[j].count, sizeof(int16_t));
        for(i=0; i<ifd_list[j].entry_count; i++)
        {
            if ((ifd_list[j].entry[i].type & T_SKIP) == 0)
            {
                add_val_to_buf(ifd_list[j].entry[i].tag, sizeof(int16_t));
                add_val_to_buf(ifd_list[j].entry[i].type & 0xFF, sizeof(int16_t));
                add_val_to_buf(ifd_list[j].entry[i].count, sizeof(int32_t));
                size_ext=get_type_size(ifd_list[j].entry[i].type)*ifd_list[j].entry[i].count;
                if (size_ext<=4) 
                {
                    if (ifd_list[j].entry[i].type & T_PTR)
                    {
                        add_to_buf((void*)ifd_list[j].entry[i].offset, sizeof(int32_t));
                    }
                    else
                    {
                        add_val_to_buf(ifd_list[j].entry[i].offset, sizeof(int32_t));
                    }
                }
                else
                {
                    add_val_to_buf(extra_offset, sizeof(int32_t));
                    extra_offset += size_ext+(size_ext&1);    
                }
            }
        }
        add_val_to_buf(0, sizeof(int32_t));
    }

    // writing extra data

    for (j=0;j<ifd_count;j++)
    {
        int32_t size_ext;
        for(i=0; i<ifd_list[j].entry_count; i++)
        {
            if ((ifd_list[j].entry[i].type & T_SKIP) == 0)
            {
                size_ext=get_type_size(ifd_list[j].entry[i].type)*ifd_list[j].entry[i].count;
                if (size_ext>4)
                {
                    add_to_buf((void*)ifd_list[j].entry[i].offset, size_ext);
                    if (size_ext&1) add_val_to_buf(0, 1);
                }
            }
        }
    }

    // writing zeros to tail of dng header (just for fun)
    for (i=dng_header_buf_offset; i<dng_header_buf_size; i++) dng_header_buf[i]=0;
}

static void free_dng_header(void)
{
    if (dng_header_buf)
    {
        ufree(dng_header_buf);
        dng_header_buf=NULL;
    }
    if (thumbnail_buf)
    {
        free(thumbnail_buf);
        thumbnail_buf = 0;
    }
}

//-------------------------------------------------------------------
// Functions for creating DNG thumbnail image

static inline int32_t raw_to_8bit(int32_t raw, int32_t wb, struct raw_info * raw_info)
{
    if (raw_info->bits_per_pixel == 16) /* big endian */
    {
        raw = ((raw & 0xFF00) >> 8) | ((raw & 0xFF) << 8);
    }
    int32_t black = raw_info->black_level;
    int32_t white = raw_info->white_level;
    float ev = log2f(MAX(1, raw - black)) + wb - 5;
    float max = log2f(white - black) - 5;
    int32_t out = ev * 255 / max;
    return COERCE(out, 0, 255);
}

static void create_thumbnail(struct raw_info * raw_info)
{
    register int32_t i, j, x, y, yadj, xadj;
    register char *buf = thumbnail_buf;
    
    if (is_lossless_jpeg(raw_info))
    {
        memset(thumbnail_buf, 0, dng_th_width*dng_th_height*3);
        return;
    }

    // The sensor bayer patterns are:
    //  0x02010100  0x01000201  0x01020001
    //      R G         G B         G R
    //      G B         R G         B G
    // for the second pattern yadj shifts the thumbnail row down one line
    // for the third pattern xadj shifts the thumbnail row accross one pixel
    // these make the patterns the same
    yadj = (camera_sensor.cfa_pattern == 0x01000201) ? 1 : 0;
    xadj = (camera_sensor.cfa_pattern == 0x01020001) ? 1 : 0;
    
    for (i=0; i<dng_th_height; i++)
        for (j=0; j<dng_th_width; j++)
        {
            x = camera_sensor.active_area.x1 + ((camera_sensor.jpeg.x + (camera_sensor.jpeg.width  * j) / dng_th_width)  & 0xFFFFFFFE) + xadj;
            y = camera_sensor.active_area.y1 + ((camera_sensor.jpeg.y + (camera_sensor.jpeg.height * i) / dng_th_height) & 0xFFFFFFFE) + yadj;

            *buf++ = raw_to_8bit(get_raw_pixel(x,y), 0, raw_info);        // red pixel
            *buf++ = raw_to_8bit(get_raw_pixel(x+1,y), -1, raw_info);      // green pixel
            *buf++ = raw_to_8bit(get_raw_pixel(x+1,y+1), 0, raw_info);    // blue pixel
        }
}

//-------------------------------------------------------------------
// Write DNG header, thumbnail and data to file

static int32_t write_dng(FILE* fd, struct raw_info * raw_info) 
{
    create_dng_header(raw_info);
    char* rawadr = (void*)raw_info->buffer;

    if (dng_header_buf)
    {
        create_thumbnail(raw_info);
        if (write(fd, dng_header_buf, dng_header_buf_size) != dng_header_buf_size) return 0;
        if (write(fd, thumbnail_buf, dng_th_width*dng_th_height*3) != dng_th_width*dng_th_height*3) return 0;

        if (!is_lossless_jpeg(raw_info))
        {
            reverse_bytes_order(UNCACHEABLE(rawadr), camera_sensor.raw_size);
        }
        if (write(fd, UNCACHEABLE(rawadr), camera_sensor.raw_size) != camera_sensor.raw_size) return 0;

        free_dng_header();
    }
    return 1;
}

#ifdef CONFIG_MAGICLANTERN
PROP_HANDLER(PROP_CAM_MODEL)
{
    snprintf(cam_name, sizeof(cam_name), (const char *)buf);
}
#endif

/* returns 1 on success, 0 on error */
int save_dng(char* filename, struct raw_info * raw_info)
{
    #ifdef RAW_DEBUG_BLACK
    raw_info->active_area.x1 = 0;
    raw_info->active_area.x2 = raw_info->width;
    raw_info->active_area.y1 = 0;
    raw_info->active_area.y2 = raw_info->height;
    raw_info->jpeg.x = 0;
    raw_info->jpeg.y = 0;
    raw_info->jpeg.width = raw_info->width;
    raw_info->jpeg.height = raw_info->height;
    #endif
    
    FILE* f = FIO_CreateFile(filename);
    if (!f) return 0;
    int32_t ok = write_dng(f, raw_info);
    FIO_CloseFile(f);
    if (!ok)
    {
        FIO_RemoveFile(filename);
        return 0;
    }
    return 1;
}
