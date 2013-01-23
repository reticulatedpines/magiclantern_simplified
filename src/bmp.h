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

/** Returns a pointer to the real BMP vram, as reported by Canon firmware.
 *  Not to be used directly - it may be somewhere in the middle of VRAM! */
inline uint8_t* bmp_vram_raw() { return bmp_vram_info[1].vram2; } 

/**
 * The total BMP area starts at 0x***80008 or 0x***00008 and has 960x540 pixels.
 * 
 * Normally, only the center part (720x480) is used. So, Canon BMP functions 
 * will return a pointer with an offset equal to 30*960 + 120 => VRAM address will end in 0x7100.
 * 
 * End of BMP VRAM is at 0x***80008 + 0x7E900 (960x540). It's not safe to write past this address.
 * 
 * The problem is that HDMI properties are not reliable for telling HDMI size 
 * (race condition while changing display modes).
 * 
 * Workaround: ML will always use a pointer to the CROPPED (720x480) BMP VRAM.
 * 
 * Advantages:
 * 
 * - Zero chances to write past the end of the VRAM due to race condition when changing display modes
 * - Everything you draw on the screen will be visible and centered well on HDMI
 * - Keeps most of the existing code (designed for LCD) unchanged
 * 
 * Disadvantage:
 * - On HDMI, you may have to draw BEHIND the VRAM pointer (you can go at most 30 lines and 120 columns back).
 *   Could be a bit ugly to code.
 * 
 */

/** These are the hard limits - never ever write outside them! */
#ifdef CONFIG_VXWORKS

#define BMP_W_PLUS 720
#define BMP_W_MINUS 0
#define BMP_H_PLUS 480
#define BMP_H_MINUS 0

#define BMPPITCH 360
#define BMP_VRAM_SIZE (360*240)
#define BMP_HDMI_OFFSET 0

/** Returns a pointer to the real BMP vram */
#ifdef CONFIG_5DC
inline uint8_t* bmp_vram_real() { return (uint8_t*) MEM(0x29328); }
#elif defined(CONFIG_40D)
inline uint8_t* bmp_vram_real() { return (uint8_t*) MEM(0x1E330); }
#else
error
#endif

extern int bmp_vram_idle_ptr;

/** Returns a pointer to idle BMP vram */
inline uint8_t* bmp_vram_idle()
{
    return (uint8_t *)((uintptr_t)bmp_vram_idle_ptr);
}

inline uint8_t* BMP_VRAM_START(uint8_t* bmp_buf) { return bmp_buf; }
#define BMP_VRAM_END(bmp_buf) (BMP_VRAM_START((uint8_t*)(bmp_buf)) + BMP_VRAM_SIZE)

#define SET_4BIT_PIXEL(p, x, color) *(char*)(p) = ((x) % 2) ? ((*(char*)(p) & 0x0F) | (D2V(color) << 4)) : ((*(char*)(p) & 0xF0) | (D2V(color) & 0x0F))    

#else // dryos

#define BMP_W_PLUS 840
#define BMP_W_MINUS -120
#define BMP_H_PLUS 510
#define BMP_H_MINUS -30

#define BMPPITCH 960
#define BMP_VRAM_SIZE (960*540)

#define BMP_HDMI_OFFSET ((-BMP_H_MINUS)*BMPPITCH + (-BMP_W_MINUS))

// BMP_VRAM_START and BMP_VRAM_START are not generic - they only work on BMP buffer addresses returned by Canon firmware

inline uint8_t* BMP_VRAM_START(uint8_t* bmp_buf)
{
    // 5D3: LCD: 00dc3100 / HDMI: 00d3c008
    // 500D: LCD: 003638100 / HDMI: 003631008
    // 550D/60D/5D2: LCD: ***87100 / HDMI: ***80008
    
    // 5D2 SD: 7108 / 74c8
    
    if (((uintptr_t)bmp_buf & 0xFFF) == 0x100) // 720x480 crop - alter it to point to full 960x540 buffer
        return (uint8_t*)((uintptr_t)bmp_buf - BMP_HDMI_OFFSET);

    if (((uintptr_t)bmp_buf & 0xFFF) == 0x008) // HDMI 960x540 => return it as is
        return bmp_buf;

    if (((uintptr_t)bmp_buf & 0xFFF) == 0x108) // SD mode 1
        return (uint8_t*)((uintptr_t)bmp_buf - BMP_HDMI_OFFSET - 8);

    if (((uintptr_t)bmp_buf & 0xFFF) == 0x4c8) // SD mode 1
        return (uint8_t*)((uintptr_t)bmp_buf - BMP_HDMI_OFFSET - 0x3c8);
        
    // something else - new camera? return it unchanged (failsafe)
    ASSERT(0);
    return bmp_buf;
}

#define BMP_VRAM_END(bmp_buf) (BMP_VRAM_START((uint8_t*)(bmp_buf)) + BMP_VRAM_SIZE)

/** Returns a pointer to the real BMP vram */
inline uint8_t* bmp_vram_real()
{
    return (uint8_t *)((uintptr_t)BMP_VRAM_START(bmp_vram_raw()) + BMP_HDMI_OFFSET);
}

/** Returns a pointer to idle BMP vram */
inline uint8_t* bmp_vram_idle()
{
#ifdef CONFIG_1100D
	return (uint8_t *)((((uintptr_t)bmp_vram_real() + 0x80000) ^ 0x80000) - 0x80000);
#else
    return (uint8_t *)((uintptr_t)bmp_vram_real() ^ 0x80000);
#endif
}
#endif


#define BMP_TOTAL_WIDTH (BMP_W_PLUS - BMP_W_MINUS)
#define BMP_TOTAL_HEIGHT (BMP_H_PLUS - BMP_H_MINUS)


inline void bmp_putpixel_fast(uint8_t * const bvram, int x, int y, uint8_t color)
{
    #ifdef CONFIG_VXWORKS
    char* p = (char*)&bvram[(x)/2 + (y)/2 * BMPPITCH]; 
    SET_4BIT_PIXEL(p, x, color);
    #else
    bvram[x + y * BMPPITCH] = color;
    #endif

     #ifdef CONFIG_500D // err70?!
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
     #endif
}


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
    uint32_t fontspec
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


static inline uint32_t
fontspec_fg(uint32_t fontspec)
{
    return (fontspec >> 0) & 0xFF;
}

static inline uint32_t
fontspec_bg(uint32_t fontspec)
{
    return (fontspec >> 8) & 0xFF;
}

static inline uint32_t
fontspec_height(uint32_t fontspec)
{
    return fontspec_font(fontspec)->height;
}

OS_FUNCTION( 0x0500001,	void,	bmp_printf, uint32_t fontspec, int x, int y, const char* fmt, ... );
OS_FUNCTION( 0x0500002, size_t,	read_file, const char * filename, void * buf, size_t size);

extern void
con_printf(
        uint32_t fontspec,
        const char *fmt,
        ...
) __attribute__((format(printf,2,3)));

extern void
bmp_hexdump(
    uint32_t fontspec,
    uint32_t x,
    uint32_t y,
    const void *buf,
    uint32_t len
);


extern void
bmp_puts(
        uint32_t fontspec,
        int *x,
        int *y,
        const char *s
);

/** Fill the screen with a bitmap palette */
extern void
bmp_draw_palette( void );


/** Fill a section of bitmap memory with solid color
 * Only has a four-pixel resolution in X.
 */
extern void
bmp_fill(
        uint8_t color,
        int x,
        int y,
        int w,
        int h
);


/** Some selected colors */

#define COLOR_EMPTY             0x00 // total transparent
#if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_5DC)
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
#define COLOR_DARK_RED 0xC
#define COLOR_GRAY40 40
#define COLOR_GRAY45 45
#define COLOR_GRAY50 50
#define COLOR_GRAY60 60
#define COLOR_GRAY70 70

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

#define BMP_LOCK(x) { AcquireRecursiveLock(bmp_lock, 0); x; ReleaseRecursiveLock(bmp_lock);}
#define GMT_LOCK(x) { error }

//~ #define BMP_LOCK(x) { CheckBmpAcquireRecursiveLock(bmp_lock, __LINE__, __func__); x; CheckBmpReleaseRecursiveLock(bmp_lock);}

//~ #define BMP_LOCK(x) { x; }
//~ #define BMP_LOCK(x) { bmp_ctr++; bmp_printf(FONT_SMALL, 50, 150, "BMP_LOCK try %s:%d  ", __func__, __LINE__); AcquireRecursiveLock(bmp_lock, 500); bmp_printf(FONT_SMALL, 50, 150, "                          "); bmp_printf(FONT_SMALL, 50, 75, "BMP_LOCK 1 %s:%d  ", __func__, __LINE__); x; bmp_printf(FONT_SMALL, 50, 75, "BMP_LOCK 0 releasing...                    "); ReleaseRecursiveLock(bmp_lock); bmp_printf(FONT_SMALL, 50, 75, "BMP_LOCK 0 %s:%d ", __func__, __LINE__); bmp_ctr--;}
//~ #define GMT_LOCK(x) { bmp_ctr++; bmp_printf(FONT_SMALL, 50, 200, "GMT_LOCK try %s:%d  ", __func__, __LINE__); AcquireRecursiveLock(gmt_lock, 500); bmp_printf(FONT_SMALL, 50, 200, "                          "); bmp_printf(FONT_SMALL, 50, 100, "GMT_LOCK 1 %s:%d  ", __func__, __LINE__); x; bmp_printf(FONT_SMALL, 50, 100, "GMT_LOCK 0 releasing...                    "); ReleaseRecursiveLock(gmt_lock); bmp_printf(FONT_SMALL, 50, 100, "GMT_LOCK 0 %s:%d ", __func__, __LINE__); bmp_ctr--;}


void bmp_flip(uint8_t* dst, uint8_t* src, int voffset);
void bmp_flip_ex(uint8_t* dst, uint8_t* src, uint8_t* mirror, int voffset);


/** 5dc bitmap icons (ones that work and what they are) */
/*  
 0x9EBDEF   =   squigly line like a tilde
 0x8DBCEF   =   dash
 0xBA96EE   =   play icon
 0xBB96EE   =   camera icon
 0xBE96EE   =   computer monitor icon
 0xB596EE   =   35mm film square
 0xB496EE   =   info 'i'
 0xB396EE   =   sharpness icon
 0xB296EE   =   brightness icon [focus]
 0xB196EE   =   contrast icon
 0xB096EE   =   picstyle icon
 0xAF96EE   =   play icon
 0xAD96EE   =   direct print icon
 0xA996EE   =   letter L
 0xA896EE   =   letter M
 0xA796EE   =   letter S
 0xA696EE   =   step icon
 0xA596EE   =   "transfer" arrows (arrows pointing left/right) icon
 0xA496EE   =   RAW icon
 */

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
#define ICON_RECTANGLE_VERT 0x8d9aee
#define ICON_4NEIGHBOURS 0x909aee
#define ICON_ISO 0x929aee
#define ICON_8ARROWS 0x939aee
#define ICON_GRID1 0x959aee
#define ICON_GRID2 0x969aee
#define ICON_STAR 0x979aee
#define ICON_RECTANGLE_ROUNDED 0x999aee
#define ICON_VOICE 0x9a9aee
#define ICON_VIDEOCAM 0x9b9aee
#define ICON_FLASH_A 0xa29aee
#define ICON_FLASH_B 0xa59aee

#ifdef CONFIG_500D
#undef ICON_VIDEOCAM
#define ICON_VIDEOCAM ICON_FILM
#endif

#define ICON_ML_AUDIO -1
#define ICON_ML_EXPO -2
#define ICON_ML_OVERLAY -3
#define ICON_ML_MOVIE -4
#define ICON_ML_SHOOT -5
#define ICON_ML_FOCUS -6
#define ICON_ML_DISPLAY -7
#define ICON_ML_PREFS -8
#define ICON_ML_DEBUG -9
#define ICON_ML_INFO -10
#define ICON_ML_MYMENU -11

#define ICON_ML_SUBMENU -100

/** 5dc has to use some different icons than dryos cameras */
#ifdef CONFIG_VXWORKS
#define ICON_CF 0xAC96EE
#define ICON_AE 0xB096EE
#define ICON_P_SQUARE 0xA596EE
#define ICON_SMILE 0xaf96ee
#define ICON_LV 0xbd96EE
#else
#define ICON_CF 0x8e9aee
#define ICON_AE 0x919aee
#define ICON_P_SQUARE 0x8c9aee
#define ICON_SMILE 0x949aee
#define ICON_LV 0x989aee
#endif

#endif //#ifndef _bmp_h_
