#ifndef _bmp_h_
#define _bmp_h_

/** \file
 * Drawing routines for the bitmap VRAM.
 *
 * These are Magic Lantern routines to draw shapes and text into
 * the LVRAM for display on the LCD or HDMI output.
 */

/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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

#include "dryos.h"
#include "font.h"

uint8_t* read_entire_file(const char * filename, int* buf_size);

extern int bmp_enabled;

/** Returns a pointer to the real BMP vram (or to idle BMP vram) */
uint8_t * bmp_vram(void);

/** Returns a pointer to idle BMP vram */
uint8_t* bmp_vram_idle();

/** Returns a pointer to real BMP vram */
uint8_t* bmp_vram_real();

#define BMPPITCH 960
#define BMP_HEIGHT (hdmi_code == 5 ? 540 : 480)
#define BMP_WIDTH (hdmi_code == 5 ? 960 : 720)
#define BMP_END (bmp_vram() + BMP_HEIGHT * BMPPITCH)

/** Font specifiers include the font, the fg color and bg color */
#define FONT_MASK               0x000F0000
//~ #define FONT_HUGE           0x00080000
#define FONT_LARGE              0x00030000
#define FONT_MED                0x00020000
#define FONT_SMALL              0x00010000

#define SHADOW_MASK             0x00100000
#define SHADOW_FONT(fnt) ((fnt) | SHADOW_MASK)

#define FONT(font,fg,bg)        ( 0 \
        | ((font) & (FONT_MASK | SHADOW_MASK)) \
        | ((bg) & 0xFF) << 8 \
        | ((fg) & 0xFF) << 0 \
)

#define FONT_BG(font) (((font) & 0xFF00) >> 8)
#define FONT_FG(font) (((font) & 0x00FF) >> 0)

static inline struct font *
fontspec_font(
        unsigned                fontspec
)
{
        switch( fontspec & FONT_MASK )
        {
        default:
        case FONT_SMALL:        return &font_small;
        case FONT_MED:          return &font_med;
        case FONT_LARGE:        return &font_large;
        //~ case FONT_HUGE:             return &font_huge;
        }
}


static inline unsigned
fontspec_fg(
        unsigned                fontspec
)
{
        return (fontspec >> 0) & 0xFF;
}

static inline unsigned
fontspec_bg(
        unsigned                fontspec
)
{
        return (fontspec >> 8) & 0xFF;
}



static inline unsigned
fontspec_height(
        unsigned                fontspec
)
{
        return fontspec_font(fontspec)->height;
}

OS_FUNCTION( 0x0500001,	void,	bmp_printf, unsigned fontspec, unsigned x, unsigned y, const char* fmt, ... );
OS_FUNCTION( 0x0500002, size_t,	read_file, const char * filename, void * buf, size_t size);

extern void
con_printf(
        unsigned                fontspec,
        const char *            fmt,
        ...
) __attribute__((format(printf,2,3)));

extern void
bmp_hexdump(
        unsigned                fontspec,
        unsigned                x,
        unsigned                y,
        const void *            buf,
        int                     len
);


extern void
bmp_puts(
        unsigned                fontspec,
        unsigned *              x,
        unsigned *              y,
        const char *            s
);

/** Fill the screen with a bitmap palette */
extern void
bmp_draw_palette( void );


/** Fill a section of bitmap memory with solid color
 * Only has a four-pixel resolution in X.
 */
extern void
bmp_fill(
        uint8_t                 color,
        uint32_t                x,
        uint32_t                y,
        uint32_t                w,
        uint32_t                h
);


/** Some selected colors */
#define COLOR_EMPTY             0x00 // total transparent
#if defined(CONFIG_5D2) || defined(CONFIG_50D)
#define COLOR_BG                0x03 // transparent black
#else
#define COLOR_BG                0x14 // transparent gray
#endif
#define COLOR_BG_DARK           0x03 // transparent black
#define COLOR_WHITE             0x01 // Normal white
#define COLOR_BLUE              0x0B // normal blue
#define COLOR_LIGHTBLUE 9
#define COLOR_RED               0x08 // normal red
#define COLOR_YELLOW            0x0F // normal yellow
#define COLOR_BLACK 2
#define COLOR_ALMOST_BLACK 38
#define COLOR_CYAN 5
#define COLOR_GREEN1 6
#define COLOR_GREEN2 7
#define COLOR_ORANGE 19

static inline uint32_t
color_word(
        uint8_t                 color
)
{
        return 0
                | ( color << 24 )
                | ( color << 16 )
                | ( color <<  8 )
                | ( color <<  0 )
                ;
}


/** BMP file format.
 * Offsets and meaning from:
 *      http://www.fastgraph.com/help/bmp_header_format.html
 */
struct bmp_file_t
{
        uint16_t                signature;      // off 0
        uint32_t                size;           // off 2, in bytes
        uint16_t                res_0;          // off 6, must be 0
        uint16_t                res_1;          // off 8. must be 0
        uint8_t *               image;          // off 10, offset in bytes
        uint32_t                hdr_size;       // off 14, must be 40
        uint32_t                width;          // off 18, in pixels
        uint32_t                height;         // off 22, in pixels
        uint16_t                planes;         // off 26, must be 1
        uint16_t                bits_per_pixel; // off 28, 1, 4, 8 or 24
        uint32_t                compression;    // off 30, 0=none, 1=RLE8, 2=RLE4
        uint32_t                image_size;     // off 34, in bytes + padding
        uint32_t                hpix_per_meter; // off 38, unreliable
        uint32_t                vpix_per_meter; // off 42, unreliable
        uint32_t                num_colors;     // off 46
        uint32_t                num_imp_colors; // off 50
} PACKED;

SIZE_CHECK_STRUCT( bmp_file_t, 54 );

extern struct bmp_file_t *
bmp_load(
        const char *            name,
        uint32_t                compression // what compression to load the file into. 0: none, 1: RLE8
);

// this has the position of the 3:2 image (onto which we draw cropmarks)
struct bmp_ov_loc_size
{
        int x0; //live view x offset within OSD
        int y0; //live view y offset within OSD
        int x_ex; //live view x extend (x0 + x_ex = xmax)
        int y_ex; //live view y extend
        int x_max; // x0 + x_ex
        int y_max; // y0 + y_ex
        int off_169; // width of one 16:9 bar
        int off_1610; // width of one 16:10 bar
};

void clrscr();
void bmp_draw(struct bmp_file_t * bmp, int x0, int y0, uint8_t* const mirror, int clear);
void bmp_draw_scaled(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax);
void bmp_draw_scaled_ex(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax, uint8_t* const mirror);
uint8_t bmp_getpixel(int x, int y);

#define TOPBAR_BGCOLOR (bmp_getpixel(os.x0,os.y0))
#define BOTTOMBAR_BGCOLOR (bmp_getpixel(os.x0,os.y_max-1))

//~ struct semaphore * bmp_sem;
//~ struct semaphore * gmt_sem;
//~ #define BMP_SEM(x) { bmp_printf(FONT_LARGE, 50, 100, "B1 %s:%d", __func__, __LINE__); take_semaphore(bmp_sem, 0); x; give_semaphore(bmp_sem); bmp_printf(FONT_LARGE, 50, 100, "B0                                 ");}
//~ #define GMT_SEM(x) { bmp_printf(FONT_LARGE, 50, 50, "G1 %s:%d", __func__, __LINE__); card_led_on(); take_semaphore(gmt_sem, 0); x; give_semaphore(gmt_sem);  card_led_off(); bmp_printf(FONT_LARGE, 50, 50, "G0                                 "); }

extern void* bmp_lock;
extern void* gmt_lock;
//~ extern int bmp_ctr;

//~ #define BMP_LOCK(x) { if(bmp_lock) AcquireRecursiveLock(bmp_lock, 0); x; if(bmp_lock) ReleaseRecursiveLock(bmp_lock, 0);}
//~ #define GMT_LOCxK(x) { if(gmt_lock) AcquireRecursiveLock(gmt_lock, 0); x; if(gmt_lock) ReleaseRecursiveLock(gmt_lock, 0);}

extern void *AcquireRecursiveLock(void *lock, int n);
extern void *CreateRecursiveLock(int n);
extern void *ReleaseRecursiveLock(void *lock);

//~ #define BMP_LOCK(x) { AcquireRecursiveLock(bmp_lock, 0); x; ReleaseRecursiveLock(bmp_lock);}
#define GMT_LOCK(x) { error }

#define BMP_LOCK(x) { CheckBmpAcquireRecursiveLock(bmp_lock, __LINE__); x; CheckBmpReleaseRecursiveLock(bmp_lock);}

//~ #define BMP_LOCK(x) { x; }
//~ #define BMP_LOCK(x) { bmp_ctr++; bmp_printf(FONT_SMALL, 50, 150, "BMP_LOCK try %s:%d  ", __func__, __LINE__); AcquireRecursiveLock(bmp_lock, 500); bmp_printf(FONT_SMALL, 50, 150, "                          "); bmp_printf(FONT_SMALL, 50, 75, "BMP_LOCK 1 %s:%d  ", __func__, __LINE__); x; bmp_printf(FONT_SMALL, 50, 75, "BMP_LOCK 0 releasing...                    "); ReleaseRecursiveLock(bmp_lock); bmp_printf(FONT_SMALL, 50, 75, "BMP_LOCK 0 %s:%d ", __func__, __LINE__); bmp_ctr--;}
//~ #define GMT_LOCK(x) { bmp_ctr++; bmp_printf(FONT_SMALL, 50, 200, "GMT_LOCK try %s:%d  ", __func__, __LINE__); AcquireRecursiveLock(gmt_lock, 500); bmp_printf(FONT_SMALL, 50, 200, "                          "); bmp_printf(FONT_SMALL, 50, 100, "GMT_LOCK 1 %s:%d  ", __func__, __LINE__); x; bmp_printf(FONT_SMALL, 50, 100, "GMT_LOCK 0 releasing...                    "); ReleaseRecursiveLock(gmt_lock); bmp_printf(FONT_SMALL, 50, 100, "GMT_LOCK 0 %s:%d ", __func__, __LINE__); bmp_ctr--;}

#endif





// Canon built-in icons (CanonGothic font)
#define ICON_TAB 0xa496ee
#define ICON_PRINT 0xac96ee
#define ICON_CONTRAST 0xae96ee
#define ICON_SATURATION 0xaf96ee
#define ICON_COLOR_BW_FILTER 0xb096ee
#define ICON_COLOR_BW_TONING_PAINTBRUSH 0xb196ee
#define ICON_SHARPNESS 0xb296ee
#define ICON_COLOR_TONE 0xb396ee
#define ICON_i 0xb496ee
#define ICON_FILM 0xb596ee
#define ICON_HEAD_WITH_RAYS 0xb696ee
#define ICON_CUSTOMWB 0xb996ee
#define ICON_PHOTOCAM 0xbb96ee
#define ICON_LANDSCAPE 0xbd96ee
#define ICON_MONITOR 0xbe96ee
#define ICON_BW 0x8297ee
#define ICON_KEY 0x819aee
#define ICON_MIC 0x829aee
#define ICON_SUBDIAL 0x839aee
#define ICON_MAINDIAL 0x849aee
#define ICON_CF1 0x869aee
#define ICON_SD2 0x879aee
#define ICON_CYL 0x889aee
#define ICON_KEY_SQUARE 0x899aee
#define ICON_L_SQUARE 0x8a9aee
#define ICON_N_SQUARE 0x8b9aee
#define ICON_P_SQUARE 0x8c9aee
#define ICON_RECTANGLE_VERT 0x8d9aee
#define ICON_CF 0x8e9aee
#define ICON_4NEIGHBOURS 0x909aee
#define ICON_AE 0x919aee
#define ICON_ISO 0x929aee
#define ICON_8ARROWS 0x939aee
#define ICON_SMILE 0x949aee
#define ICON_GRID1 0x959aee
#define ICON_GRID2 0x969aee
#define ICON_STAR 0x979aee
#define ICON_LV 0x989aee
#define ICON_RECTANGLE_ROUNDED 0x999aee
#define ICON_VOICE 0x9a9aee
#define ICON_VIDEOCAM 0x9b9aee
#define ICON_FLASH_A 0xa29aee
#define ICON_FLASH_B 0xa59aee

#define ICON_ML_PLAY -1
#define ICON_ML_SUBMENU -100
