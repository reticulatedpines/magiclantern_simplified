/** \file
 * Drawing routines.
 *
 * These are Magic Lantern routines to draw into the BMP LVRAM.
 * They are not derived from DryOS routines.
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
#include "bmp.h"
#include "font.h"
#include <stdarg.h>
#include "propvalues.h"

//~ int bmp_enabled = 1;

#ifdef CONFIG_VXWORKS

    // inline functions in bmp.h

#else // DryOS

    // BMP_VRAM_START and BMP_VRAM_START are not generic - they only work on BMP buffer addresses returned by Canon firmware
    uint8_t* BMP_VRAM_START(uint8_t* bmp_buf)
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
            
        if (((uintptr_t)bmp_buf & 0xFFF) == 0x4c0) // SD 700D
            return (uint8_t*)((uintptr_t)bmp_buf - BMP_HDMI_OFFSET - 0x3c0);
        
        if (((uintptr_t)bmp_buf & 0xFFF) == 0xc28) // 100D
            return (uint8_t*)((uintptr_t)bmp_buf - BMP_HDMI_OFFSET - 0xb28);

        // something else - new camera? return it unchanged (failsafe)
        ASSERT(0);
        return bmp_buf;
    }

    uint8_t* bmp_vram_real()
    {
        return (uint8_t *)((uintptr_t)BMP_VRAM_START(bmp_vram_raw()) + BMP_HDMI_OFFSET);
    }

    /** Returns a pointer to idle BMP vram */
    uint8_t* bmp_vram_idle()
    {
    #if defined(CONFIG_1100D) || defined(CONFIG_100D) // This fixes "dirty" LCD output for 100D
        return (uint8_t *)((((uintptr_t)bmp_vram_real() + 0x80000) ^ 0x80000) - 0x80000);
    #else
        return (uint8_t *)((uintptr_t)bmp_vram_real() ^ 0x80000);
    #endif
    }
#endif

static int bmp_idle_flag = 0;

void bmp_draw_to_idle(int value) { bmp_idle_flag = value; }

/** Returns a pointer to currently selected BMP vram (real or mirror) */
uint8_t * bmp_vram(void)
{
    #ifdef CONFIG_VXWORKS
    set_ml_palette_if_dirty();
    #endif
    uint8_t * bmp_buf = bmp_idle_flag ? bmp_vram_idle() : bmp_vram_real();

    // if (PLAY_MODE) return UNCACHEABLE(bmp_buf);
    return bmp_buf;
}

// 0 = copy BMP to idle
// 1 = copy idle to BMP
void bmp_idle_copy(int direction, int fullsize)
{
    uint8_t* real = bmp_vram_real();
    uint8_t* idle = bmp_vram_idle();
    ASSERT(real)
    ASSERT(idle)

    if (fullsize)
    {
        if (direction)
            memcpy(BMP_VRAM_START(real), BMP_VRAM_START(idle), BMP_VRAM_SIZE);
        else
            memcpy(BMP_VRAM_START(idle), BMP_VRAM_START(real), BMP_VRAM_SIZE);
    }
    else
    {
#ifdef CONFIG_VXWORKS
        if (direction)
        {
            for (int i = 0; i < 240; i ++)
                memcpy(real+i*BMPPITCH, idle+i*BMPPITCH, 360);
        }
        else
        {
            for (int i = 0; i < 240; i ++)
                memcpy(idle+i*BMPPITCH, real+i*BMPPITCH, 360);
        }
#else
        unsigned char * dst_ptr = direction ? real : idle;
        unsigned char * src_ptr = direction ? idle : real;

        for (int i = 0; i < 480; i++, dst_ptr += BMPPITCH, src_ptr += BMPPITCH)
            memcpy(dst_ptr, src_ptr, 720);
#endif
    }
}

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


#ifndef CONFIG_60D
#define USE_LUT
#endif

int
bmp_puts(
        uint32_t fontspec,
        int *x,
        int *y,
        const char *s
)
{
    *x = COERCE(*x, BMP_W_MINUS, BMP_W_PLUS);
    *y = COERCE(*y, BMP_H_MINUS, BMP_H_PLUS);
    
    uint32_t    fg_color    = fontspec_fg( fontspec );
    uint32_t    bg_color    = fontspec_bg( fontspec );
    
    int len = rbf_draw_string((void*)font_dynamic[FONT_ID(fontspec)].bitmap, *x, *y, s, FONT(fontspec, fg_color, bg_color));
    *x += len;
    return len;
}

/*
void
bmp_puts_w(
    unsigned        fontspec,
    int *        x,
    int *        y,
    int max_chars_per_line,
    const char *        s
)
{
    ASSERT(x)
    ASSERT(y)
    ASSERT(s)

    const uint32_t        pitch = BMPPITCH;
    uint8_t * vram = bmp_vram();
    if( !vram || ((uintptr_t)vram & 1) == 1 )
        return;
    const int initial_x = *x;
    uint8_t * first_row = vram + (*y) * pitch + (*x);
    uint8_t * row = first_row;

    char c;

    const struct font * const font = fontspec_font( fontspec );
    int i = 0;
    while( (c = *s++) )
    {
        if( c == '\n' || i >= max_chars_per_line)
        {
            row = first_row += pitch * font->height;
            (*y) += font->height;
            (*x) = initial_x;
            i = 0;
            if (c == '\n') continue;
        }

        _draw_char( fontspec, row, c );
        row += font->width;
        (*x) += font->width;
        i++;
    }

}
*/

// thread safe
int
bmp_printf(
           uint32_t fontspec,
           int x,
           int y,
           const char *fmt,
           ...
           )
{
    va_list            ap;

    char bmp_printf_buf[128];

    va_start( ap, fmt );
    vsnprintf( bmp_printf_buf, sizeof(bmp_printf_buf)-1, fmt, ap );
    va_end( ap );

    return bmp_puts( fontspec, &x, &y, bmp_printf_buf );
}

// for very large strings only
int
big_bmp_printf(
               uint32_t        fontspec,
               int        x,
               int        y,
               const char *        fmt,
               ...
               )
{
    int ans = 0;
    BMP_LOCK(
        va_list            ap;

        static char bmp_printf_buf[1024];

        va_start( ap, fmt );
        vsnprintf( bmp_printf_buf, sizeof(bmp_printf_buf)-1, fmt, ap );
        va_end( ap );

        ans = bmp_puts( fontspec, &x, &y, bmp_printf_buf );
    )
    return ans;
}

int bmp_string_width(int fontspec, const char* str)
{
    return rbf_str_width((void*)font_dynamic[FONT_ID(fontspec)].bitmap, str);
}

int bmp_strlen_clipped(int fontspec, const char* str, int maxwidth)
{
    return rbf_strlen_clipped((void*)font_dynamic[FONT_ID(fontspec)].bitmap, str, maxwidth);
}

#ifdef CONFIG_HEXDUMP

static void bmp_puts_diff(uint32_t font_normal, uint32_t font_highlight, int x, int y, char* msg, char* old_msg)
{
    for (char *c = msg, *o = old_msg; *c; c++, o++)
    {
        int font = *c == *o ? font_normal : font_highlight;
        int chr = *c;
        bmp_puts(font, &x, &y, (char*) &chr);
    }
}

void
bmp_hexdump(
    uint32_t fontspec,
    uint32_t x,
    uint32_t y,
    const void *buf,
    uint32_t len
)
{
    if( len == 0 )
        return;

    // Round up
    len = (len + 15) & ~15;

    uint32_t d = (uint32_t) buf;
    char msg[100];
    static char prev_msg[10][100];
    
    do {
        snprintf(msg, sizeof(msg),
            "%08x: %08x %08x %08x %08x %08x %08x %08x %08x : %04x",
            (uint32_t) d,
            len >  0 ? MEMX(d+0x00) : 0,
            len >  4 ? MEMX(d+0x04) : 0,
            len >  8 ? MEMX(d+0x08) : 0,
            len > 12 ? MEMX(d+0x0C) : 0,
            len > 16 ? MEMX(d+0x10) : 0,
            len > 20 ? MEMX(d+0x14) : 0,
            len > 24 ? MEMX(d+0x18) : 0,
            len > 28 ? MEMX(d+0x1C) : 0,
            (void*)d - (void*)buf
        );

        int row = (d - (uint32_t) buf) / 32;
        row = COERCE(row, 0, 9);
        bmp_puts_diff(FONT_SMALL, FONT(FONT_SMALL, COLOR_RED, COLOR_BLACK), x, y, msg, prev_msg[row]);
        memcpy(prev_msg[row], msg, sizeof(msg));

        y += fontspec_height( fontspec );
        d += 32;
        len -= 32;
    } while(len > 0);
}
#endif

/** Fill a section of bitmap memory with solid color
 */

void
bmp_fill(
    uint8_t            color,
    int        x,
    int        y,
    int        w,
    int        h
)
{
    #ifdef CONFIG_VXWORKS
    color = D2V(color);
    #endif

    x = COERCE(x, BMP_W_MINUS, BMP_W_PLUS-1);
    y = COERCE(y, BMP_H_MINUS, BMP_H_PLUS-1);
    w = COERCE(w, 0, BMP_W_PLUS-x-1);
    h = COERCE(h, 0, BMP_H_PLUS-y-1);

    uint8_t* b = bmp_vram();

    for (int i = y; i < y + h; i++)
    {
        uint8_t* row = b + BM(x,i);
#ifdef CONFIG_VXWORKS
        memset(row, color, w/2);
#else
        memset(row, color, w);
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
}

/** Load a BMP file into memory so that it can be drawn onscreen */

struct bmp_file_t *bmp_load_ram(uint8_t *buf, uint32_t size, uint32_t compression)
{
    struct bmp_file_t * bmp = (struct bmp_file_t *) buf;
    if( bmp->signature != 0x4D42 )
    {
        goto signature_fail;
    }

    // Update the offset pointer to point to the image data
    // if it is within bounds
    const unsigned image_offset = (unsigned) bmp->image;
    if( image_offset > size )
    {
        goto offsetsize_fail;
    }

    // Since the read was into uncacheable memory, it will
    // be very slow to access.  Copy it into a cached buffer
    // and release the uncacheable space.

    if (compression==bmp->compression) {
        uint8_t * fast_buf = malloc( size + 32);
        if( !fast_buf )
            goto fail_buf_copy;
        memcpy(fast_buf, buf, size);
        bmp = (struct bmp_file_t *) fast_buf;
        bmp->image = fast_buf + image_offset;
        return bmp;
    } else if (compression==1 && bmp->compression==0) { // convert the loaded image into RLE8
        uint32_t size_needed = sizeof(struct bmp_file_t);
        uint8_t* fast_buf;
        uint32_t x = 0;
        uint32_t y = 0;
        uint8_t* gpos;
        uint8_t count = 0;
        uint8_t color = 0;
        bmp->image = buf + image_offset;
        for (y = 0; y < bmp->height; y++) {
            uint8_t* pos = bmp->image + y*bmp->width;
            color = *pos; count = 0;
            for (x = 0; x < bmp->width; x++) {
                if (color==(*pos) && count<255) { count++; } else { color = *pos; count = 1; size_needed += 2; }
                pos++;
            }
            if (count!=0) size_needed += 2; // remaining line
            size_needed += 2; //0000 EOL
        }
        size_needed += 2; //0001 EOF
        fast_buf = malloc( size_needed );
        if( !fast_buf ) goto fail_buf_copy;
        memcpy(fast_buf, buf, sizeof(struct bmp_file_t));
        gpos = fast_buf + sizeof(struct bmp_file_t);
        for (y = 0; y < bmp->height; y++) {
            uint8_t* pos = bmp->image + y*bmp->width;
            color = *pos; count = 0;
            for (x = 0; x < bmp->width; x++) {
                if (color==(*pos) && count<255) { count++; } else { gpos[0] = count;gpos[1] = color; color = *pos; count = 1; gpos+=2;} pos++;
            }
            if (count!=0) { gpos[0] = count; gpos[1] = color; gpos+=2;}
            gpos[0] = 0;
            gpos[1] = 0;
            gpos+=2;
        }
        gpos[0] = 0;
        gpos[1] = 1;

        bmp = (struct bmp_file_t *) fast_buf;
        bmp->compression = 1;
        bmp->image = fast_buf + sizeof(struct bmp_file_t);
        bmp->image_size = size_needed;
        //~ bmp_printf(FONT_SMALL,0,440,"Memory needed %d",size_needed);
        return bmp;
    }

fail_buf_copy:
offsetsize_fail:
signature_fail:
    return NULL;
}

struct bmp_file_t *
bmp_load(
    const char *        filename,
    uint32_t         compression // what compression to load the file into. 0: none, 1: RLE8
)
{
    DebugMsg( DM_MAGIC, 3, "bmp_load(%s)", filename);
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

    DebugMsg( DM_MAGIC, 3, "File '%s' size %d bytes",
        filename,
        size
    );

    uint8_t * buf = fio_malloc( size );
    if( !buf )
    {
        DebugMsg( DM_MAGIC, 3, "%s: fio_malloc failed", filename );
        goto malloc_fail;
    }

    size_t i;
    for( i=0 ; i<size; i++ )
        buf[i] = 'A' + i;
    size_t rc = read_file( filename, buf, size );
    if( rc != size )
        goto read_fail;

    struct bmp_file_t *ret = bmp_load_ram(buf, size, compression);
    
    fio_free( buf );
    
    if(ret)
    {
        return ret;
    }

read_fail:
    fio_free( buf );
malloc_fail:
getfilesize_fail:
    DebugMsg( DM_MAGIC, 3, "bmp_load failed");
    return NULL;
}

void clrscr()
{
    BMP_LOCK( bmp_fill( 0x0, BMP_W_MINUS, BMP_H_MINUS, BMP_TOTAL_WIDTH, BMP_TOTAL_HEIGHT ); )
}

// this is slow, but is good for a small number of pixels :)
uint8_t bmp_getpixel(int x, int y)
{
    ASSERT(x >= BMP_W_MINUS && x < BMP_W_PLUS)
    ASSERT(y >= BMP_H_MINUS && y < BMP_H_PLUS)

    uint8_t * const bvram = bmp_vram();
    return bvram[x + y * BMPPITCH];
}

void bmp_putpixel(int x, int y, uint8_t color)
{
    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    x = COERCE(x, BMP_W_MINUS, BMP_W_PLUS-1);
    y = COERCE(y, BMP_H_MINUS, BMP_H_PLUS-1);

    bmp_putpixel_fast(bvram, x, y, color);
}

void bmp_draw_rect(int color, int x0, int y0, int w, int h)
{
    // this should match bmp_fill
    w--;
    h--;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    draw_line(x0,   y0,   x0+w,   y0, color);
    draw_line(x0+w, y0,   x0+w, y0+h, color);
    draw_line(x0+w, y0+h,   x0, y0+h, color);
    draw_line(x0,   y0,     x0, y0+h, color);
}

void bmp_draw_rect_chamfer(int color, int x0, int y0, int w, int h, int a, int thick_corners)
{
    // this should match bmp_fill
    w--;
    h--;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    draw_line(x0+a,   y0,     x0+w-a, y0,     color);
    draw_line(x0+w-a, y0,     x0+w,   y0+a,   color);
    draw_line(x0+w,   y0+a,   x0+w,   y0+h-a, color);
    draw_line(x0+w,   y0+h-a, x0+w-a, y0+h,   color);
    draw_line(x0+w-a, y0+h,   x0+a,   y0+h,   color);
    draw_line(x0+a,   y0+h,   x0,     y0+h-a, color);
    draw_line(x0,     y0+h-a, x0,     y0+a,   color);
    draw_line(x0,     y0+a,   x0+a,   y0,     color);

    if (thick_corners) // double the chamfer lines, useful for thicker rectangles
    {
        draw_line(x0+w-a, y0+1,   x0+w-1, y0+a,   color);
        draw_line(x0+w-1, y0+h-a, x0+w-a, y0+h-1, color);
        draw_line(x0+a,   y0+h-1, x0+1,   y0+h-a, color);
        draw_line(x0+1,   y0+a,   x0+a,   y0+1,   color);
    }
}

#ifdef CONFIG_VXWORKS

/** converting dryos palette to vxworks one */
static char bmp_lut[80] = {
    0x00, 0xff, 0x99, 0x88, 0x77, 0x44, 0x22, 0x22, 0x11, 0x33, 0xDD, 0x33, 0x11, 0x33, 0x55, 0x66,
    0x55, 0x77, 0x22, 0x77, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x99, 0x99, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xBB,
    0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xDD, 0xDD, 0xDD,
    0xDD, 0xDD, 0xDD, 0xDD, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

void set_ml_palette()
{
    if (!DISPLAY_IS_ON) return;

    int palette[16] = {
        0x00870000, // transparent
        0x0000FFFF, // 1 - red
        0x00CC00FF, // 2 - green
        0xFF0000FF, // 3 - blue
        0xFFFF00FF, // 4 - cyan
        0xFF00FFFF, // 5 - magenta
        0x00FFFFFF, // 6 - yellow
        0x0090FFFF, // 7 - orange
        0x00000080, // 8 - transparent black
        0x000000FF, // 9 - black
        0x1C1C1CFF, // A - gray 1
        0x404040FF, // B - gray 2
        0x7F7F7FFF, // C - gray 3
        0xAAAAAAFF, // D - gray 4
        0xD4D4D4FF, // E - gray 5
        0xFFFFFFFF  // F - white
    };

    extern int RGB_Palette[];
    extern int LCD_Palette[];
    extern int PB_Palette[];

    if (0) // convert from RGB to PB with Canon code, write result to a file
    {      // if you change RGB palette, run this first to get the PB equivalent (comment out BmpDDev semaphores first)
        NotifyBox(10000, "%x ", PB_Palette);
        SetRGBPaletteToDisplayDevice(palette); // problem: this is unsafe to call (race condition with Canon code)
        FILE* f = FIO_CreateFile("pb.log");
        if (f)
        {
            for (int i = 0; i < 16; i++)
                my_fprintf(f, "0x%08x, ", PB_Palette[i*3 + 2]);
            FIO_CloseFile(f);
        }
    }
    else // use pre-computed PB palette (just send it to digic)
    {
        int palette_pb[16] = {0x00fc0000, 0x0346de7f, 0x036dcba1, 0x031a66ea, 0x03a42280, 0x03604377, 0x03cf9a16, 0x0393b94b, 0x00000000, 0x03000000, 0x03190000, 0x033a0000, 0x03750000, 0x039c0000, 0x03c30000, 0x03eb0000};

        for (int i = 0; i < 16; i++)
        {
            EngDrvOut(0xC0F14080 + i*4, palette_pb[i]);
        }
        EngDrvOut(0xC0F14078, 1);
    }
}

// fun stuff
void randomize_palette()
{
    if (!DISPLAY_IS_ON) return;
    BmpDDev_take_semaphore();
    for (int i = 0; i < 16; i++)
        EngDrvOut(0xC0F14080 + i*4, rand());
    EngDrvOut(0xC0F14078, 1);
    BmpDDev_give_semaphore();
}

void set_ml_palette_if_dirty()
{
    if (!DISPLAY_IS_ON) return;
    extern int PB_Palette[];
    if (PB_Palette[15*3+2] == 0x03eb0000) return;

    BmpDDev_take_semaphore();
    set_ml_palette();
    PB_Palette[15*3+2] = 0x03eb0000;
    BmpDDev_give_semaphore();
}

void restore_canon_palette()
{
    // unsafe
    //~ SetRGBPaletteToDisplayDevice(orig_palette);
}

/*
void guess_palette()
{
    // let's try something else: match DryOs palette to VxWorks's on the fly

    static int ref_r[80] = {254, 234, 0, 0, 163, 31, 0, 1, 234, 0, 185, 27, 200, 0, 201, 209, 232, 216, 0, 231, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 231, 9, 18, 27, 36, 41, 46, 50, 55, 59, 64, 69, 73, 82, 92, 101, 109, 117, 119, 124, 129, 133, 138, 142, 147, 152, 156, 161, 165, 170, 175, 179, 184, 188, 193, 198, 202, 207, 211, 216, 221, 225, 229};
    static int ref_g[80] = {254, 235, 0, 0, 56, 187, 153, 172, 0, 66, 186, 34, 0, 0, 0, 191, 0, 94, 62, 109, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 18, 27, 36, 41, 46, 50, 55, 59, 64, 69, 73, 82, 92, 101, 110, 117, 119, 124, 129, 133, 138, 142, 147, 152, 156, 161, 165, 170, 175, 178, 184, 188, 192, 198, 202, 206, 210, 216, 221, 225, 230};
    static int ref_b[80] = {255, 235, 0, 0, 0, 216, 0, 1, 1, 211, 139, 126, 0, 168, 154, 0, 231, 76, 75, 0, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 9, 18, 27, 36, 41, 46, 50, 55, 59, 64, 69, 73, 82, 92, 101, 109, 117, 119, 124, 129, 133, 138, 142, 147, 152, 156, 161, 165, 170, 174, 178, 184, 188, 193, 198, 202, 207, 211, 216, 221, 224, 229};

    extern int RGB_Palette[];
    for (int j = 1; j < 80; j++)
    {
        int r = ref_r[j];
        int g = ref_g[j];
        int b = ref_b[j];
        int dr = g-r;
        int db = g-b;

        int em = 10000;
        for (int i = 1; i < 16; i++)
        {
            int p = RGB_Palette[i];
            int R = (p >> 8) & 0xFF;
            int G = (p >> 16) & 0xFF;
            int B = (p >> 24) & 0xFF;
            int DR = G-R;
            int DB = G-B;

            int d = (r-R)*(r-R) + (g-G)*(g-G) + (b-B)*(b-B) + (dr-DR)*(dr-DR) + (db-DB)*(db-DB);
            if (d < em)
            {
                bmp_lut[j] = i | (i<<4);
                em = d;
            }
        }
    }
}
*/

int D2V(unsigned color) { return bmp_lut[MIN(color & 0xFF,79)]; }

#endif

#ifdef CONFIG_VXWORKS

#define BMP_DRAW_PIX \
    if (mirror) \
    { \
        if (bmp_color) m_row[ xs ] = bmp_color | 0x80; \
        uint8_t p = b_row[ xs ]; \
        uint8_t m = m_row[ xs ]; \
        if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue; \
        if ((p == 0x14 || p == 0x3) && bmp_color == 0) continue; \
    } \
    char* p = &b_row[ xs/2 ];  \
    SET_4BIT_PIXEL(p, xs, bmp_color); \

#else

#define BMP_DRAW_PIX \
    if (mirror) \
    { \
        if (bmp_color) m_row[ xs ] = bmp_color | 0x80; \
        uint8_t p = b_row[ xs ]; \
        uint8_t m = m_row[ xs ]; \
        if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue; \
        if ((p == 0x14 || p == 0x3) && bmp_color == 0) continue; \
    } \
    b_row[ xs ] = bmp_color; \

#endif

static int _bmp_draw_should_stop = 0;
void bmp_draw_request_stop() { _bmp_draw_should_stop = 1; }

void bmp_draw_scaled_ex(struct bmp_file_t * bmp, int x0, int y0, int w, int h, uint8_t* const mirror)
{
    if (!bmp) return;

    _bmp_draw_should_stop = 0;
    //~ if (!bmp_enabled) return;

    x0 = COERCE(x0, BMP_W_MINUS, BMP_W_PLUS-1);
    w = COERCE(w, 0, BMP_W_PLUS-x0-1);

    // the bitmap can extend on Y-axis outside the display limits
    // but those lines will not be drawn

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    int x,y; // those sweep the original bmp
    int xs,ys; // those sweep the BMP VRAM (and are scaled)

    if (bmp->compression == 0) {
#ifdef USE_LUT
        static int16_t lut[960];
        for (xs = x0; xs < (x0 + w); xs++)
        {
            lut[xs-BMP_W_MINUS] = (xs-x0) * bmp->width/w;
        }
#endif

        for( ys = y0 ; ys < (y0 + h); ys++ )
        {
            if (ys < BMP_H_MINUS) continue;
            if (ys >= BMP_H_PLUS) continue;

            if (_bmp_draw_should_stop) return;
            y = (ys-y0)*bmp->height/h;
            #ifdef CONFIG_VXWORKS
            uint8_t * const b_row = bvram + ys/2 * BMPPITCH;
            #else
            uint8_t * const b_row = bvram + ys * BMPPITCH;
            #endif
            uint8_t * const m_row = (uint8_t*)( mirror + ys * BMPPITCH );
            for (xs = x0; xs < (x0 + w); xs++)
            {
#ifdef USE_LUT
                x = lut[xs-BMP_W_MINUS];
#else
                x = (xs-x0)*bmp->width/w;
#endif

                uint8_t bmp_color = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
                BMP_DRAW_PIX;
            }
        }
    } else if (bmp->compression == 1) {
        uint8_t * bmp_line = bmp->image; // store the start of the line
        int bmp_y_pos = bmp->height-1; // store the line number
        for( ys = y0 + h - 1 ; ys >= y0; ys-- )
        {
            if (ys < BMP_H_MINUS) continue;
            if (ys >= BMP_H_PLUS) continue;

            if (_bmp_draw_should_stop) return;
            y = (ys-y0)*bmp->height/h;
            int ysc = COERCE(ys, BMP_H_MINUS, BMP_H_PLUS);
            #ifdef CONFIG_VXWORKS
            uint8_t * const b_row =              bvram + ysc/2 * BMPPITCH;
            #else
            uint8_t * const b_row =              bvram + ysc * BMPPITCH;
            #endif
            uint8_t * const m_row = (uint8_t*)( mirror + ysc * BMPPITCH );

            while (y < bmp_y_pos) {
                // search for the next line
                // see http://www.fileformat.info/format/bmp/corion-rle8.htm
                if (bmp_line[0]!=0) { bmp_line += 2; } else
                if (bmp_line[1]==0) { bmp_line += 2; bmp_y_pos--; } else
                if (bmp_line[1]==1) return; else
                if (bmp_line[1]==2) // delta
                {
                    bmp_y_pos -= bmp_line[3];
                    if (bmp_line[3]) // skip on x+y
                    {
                        // fake the bitmap data so that it will draw a transparent line
                        // with the same size as the part skipped horizontally
                        bmp_line += 2;
                    }
                    else // skip on yonly
                    {
                        bmp_line += 4;
                    }
                }
                else bmp_line = bmp_line + ((bmp_line[1] + 1) & ~1) + 2;
                if (y<0) return;
                if (bmp_line>(uint8_t*)(bmp+bmp->image_size)) return;
            }
            if (y != bmp_y_pos) continue;
            if (bmp_line[0]==0 && bmp_line[1]==2 && bmp_line[3]) continue; // this line is just a vertical skip

            uint8_t* bmp_col = bmp_line; // store the actual position inside the bitmap
            int bmp_x_pos_end = bmp_col[0]; // store the end of the line
            uint8_t bmp_color = bmp_col[1]; // store the actual color to use
            if (y > 0 && bmp_col[-1] == 2 && bmp_col[-2] == 0) bmp_color = 0; // if previous line was a x-y skip, use transparent color
            for (xs = x0; xs < (x0 + w); xs++)
            {
                x = COERCE((int)((xs-x0)*bmp->width/w), BMP_W_MINUS, BMP_W_PLUS-1);

                while (x>=bmp_x_pos_end) {
                    // skip to this position
                    if (bmp_col>(uint8_t*)(bmp+bmp->image_size)) break;
                    bmp_col+=2;
                    if (bmp_col>(uint8_t*)(bmp+bmp->image_size)) break;
                    if (bmp_col[0]==0)
                    {
                        if (bmp_col[1] == 0)
                        {
                            bmp_color = 0;
                            bmp_x_pos_end = bmp->width;
                            break; // end of line
                        }
                        else if (bmp_col[1] == 2) // delta
                        {
                            bmp_color = COLOR_RED;
                            bmp_col += 2;
                        }
                        else if (bmp_col[1] > 2) // uncompressed data
                        {
                            bmp_color = bmp_col[2];
                            bmp_x_pos_end += bmp_col[1];
                            bmp_col += ((bmp_col[1] + 1) & ~1);
                        }
                    }
                    else // RLE data
                    {
                        bmp_x_pos_end += bmp_col[0];
                        bmp_color = bmp_col[1];
                    }
                }
                BMP_DRAW_PIX;
            }
        }
    }
}

// built-in fonts found by Pel
// http://groups.google.com/group/ml-devel/browse_thread/thread/aec4c80eef1cdd6a
// http://chdk.setepontos.com/index.php?topic=6204.0

// quick sanity test
static int bfnt_ok()
{
    int* codes = (int*) BFNT_CHAR_CODES;
    int i;

    for (i = 0; i < 20; i++)
        if (codes[i] != 0x20+i) return 0;

    int* off = (int*) BFNT_BITMAP_OFFSET;
    if (off[0] != 0) return 0;

    for (i = 1; i < 20; i++)
        if (off[i] <= off[i-1]) return 0;

    return 1;
}

// are all char codes in ascending order, for binary search?
static uint8_t* bfnt_find_char(int code)
{
    if (code < 0) // that's a ML icon
    {
        code = -code-1;
        if (code > NUM_ML_ICONS-1) return 0;
        extern canon_font_body_t menu_icons_body;
        return (uint8_t*) &menu_icons_body.bitmaps[code];
    }
    int n = (BFNT_BITMAP_OFFSET - BFNT_CHAR_CODES) / 4;
    int* codes = (int*) BFNT_CHAR_CODES;
    int* off = (int*) BFNT_BITMAP_OFFSET;

    if (code >= 0x20 && code <= 'z')
    {
        return (uint8_t*) (BFNT_BITMAP_DATA + off[code - 0x20]);
    }

    int i;
    for (i = 0; i < n; i++)
        if (codes[i] == code)
            return (uint8_t*) (BFNT_BITMAP_DATA + off[i]);
    return 0;
}

// returns width
int bfnt_draw_char(int c, int px, int py, int fg, int bg)
{
    if (!bfnt_ok())
    {
        bmp_printf(FONT_SMALL, 0, 0, "font addr bad");
        return 0;
    }

#ifdef CONFIG_40D
    //isLower((char)c)
    if(c >= 'a' && c <= 'z') { c += 1; }
#endif

    uint8_t * const bvram = bmp_vram();

    uint16_t* chardata = (uint16_t*) bfnt_find_char(c);
    if (!chardata) return 0;
    uint8_t* buff = (uint8_t*)(chardata + 5);
    int ptr = 0;

    int cw  = chardata[0]; // the stored bitmap width
    int ch  = chardata[1]; // the stored bitmap height
    int crw = chardata[2]; // the displayed character width
    int xo  = chardata[3]; // X offset for displaying the bitmap
    int yo  = chardata[4]; // Y offset for displaying the bitmap
    int bb    = cw / 8 + (cw % 8 == 0 ? 0 : 1); // calculate the byte number per line

    //~ bmp_printf(FONT_SMALL, 0, 0, "%x %d %d %d %d %d %d", chardata, cw, ch, crw, xo, yo, bb);

    if (crw+xo > 100) return 0;
    if (ch+yo > 50) return 0;

    if (bg != NO_BG_ERASE)
    {
        bmp_fill(bg, px, py, crw+xo+3, 40);
    }

    int i,j,k;
    for (i = 0; i < ch; i++)
    {
        for (j = 0; j < bb; j++)
        {
            for (k = 0; k < 8; k++)
            {
                if (j*8 + k < cw)
                {
                    if ((buff[ptr+j] & (1 << (7-k))))
                        #ifdef CONFIG_VXWORKS
                        bmp_putpixel_fast(bvram, px+j*8+k+xo, py + (i+yo)*(c < 0 ? 1 : 2), fg);
                        #else
                        bmp_putpixel_fast(bvram, px+j*8+k+xo, py+i+yo, fg);
                        #endif
                }
            }
        }
        ptr += bb;
    }
    return crw;
}

int bfnt_char_get_width(int c)
{
    if (!bfnt_ok())
    {
        bmp_printf(FONT_SMALL, 0, 0, "font addr bad");
        return 0;
    }

#ifdef CONFIG_40D
    //isLower((char)c)
    if(c >= 'a' && c <= 'z') { c += 1; }
#endif

    uint16_t* chardata = (uint16_t*) bfnt_find_char(c);
    if (!chardata) return 0;
    int crw = chardata[2]; // the displayed character width
    return crw;
}

/*
void bfnt_puts_utf8(int* s, int x, int y, int fg, int bg)
{
    while (*s)
    {
        x += bfnt_draw_char(*s, x, y, fg, bg);
        s++;
    }
}*/


#if 0
void
bfnt_test()
{
    while(1)
    {
        canon_gui_disable_front_buffer();

        int n = (BFNT_BITMAP_OFFSET - BFNT_CHAR_CODES) / 4;
        int* codes = (int*) BFNT_CHAR_CODES;

        int c = 0;
        while(c <= n) {
            for (int i = 0; i < 5; i++)
            {
                for (int j = 0; j < 10; j++)
                {
                    bfnt_draw_char(codes[c], j*70, i*80+20, COLOR_WHITE, COLOR_BLACK);
                    bmp_printf(FONT_SMALL, j*70, i*80, "%x", codes[c]);
                    c++;
                }
            }
            msleep(2000);
            clrscr();
        }
    }
}
#endif

void bmp_flip(uint8_t* dst, uint8_t* src, int voffset)
{
    ASSERT(src)
    ASSERT(dst)
    if (!dst) return;
    int i,j;

    int H_LO = hdmi_code >= 5 ? BMP_H_MINUS : 0;
    int H_HI = hdmi_code >= 5 ? BMP_H_PLUS : 480;

    int W_LO = hdmi_code >= 5 ? BMP_W_MINUS : 0;
    int W_HI = hdmi_code >= 5 ? BMP_W_PLUS : 720;

    for (i = H_LO; i < H_HI; i++) // -30 ... 510
    {
        int i_mod = H_HI + H_LO - i + voffset - 1; // 510 ... -30
        while (i_mod < H_LO) i_mod += (H_HI - H_LO);
        while (i_mod >= H_HI) i_mod -= (H_HI - H_LO);

        for (j = W_LO; j < W_HI; j++) // -120 ... 840
        {
            dst[BM(j,i)] = src[BM(W_HI + W_LO - j, i_mod)]; // 840 ... -120
        }
    }
}

void bmp_flip_ex(uint8_t* dst, uint8_t* src, uint8_t* mirror, int voffset)
{
    ASSERT(src)
    ASSERT(dst)
    ASSERT(mirror)
    if (!dst) return;
    int i,j;

    int H_LO = hdmi_code >= 5 ? BMP_H_MINUS : 0;
    int H_HI = hdmi_code >= 5 ? BMP_H_PLUS : 480;

    int W_LO = hdmi_code >= 5 ? BMP_W_MINUS : 0;
    int W_HI = hdmi_code >= 5 ? BMP_W_PLUS : 720;

    for (i = H_LO; i < H_HI; i++) // -30 ... 510
    {
        int i_mod = H_HI + H_LO - i + voffset - 1; // 510 ... -30
        while (i_mod < H_LO) i_mod += (H_HI - H_LO);
        while (i_mod >= H_HI) i_mod -= (H_HI - H_LO);

        for (j = W_LO; j < W_HI; j++) // -120 ... 840
        {
            uint8_t m = mirror[BM(j,i)];
            uint8_t b = dst[BM(j,i)];
            if (m && (m == b) && !(m & 0x80)) { /* zebras here, do nothing */ }
            else dst[BM(j,i)] = src[BM(W_HI + W_LO - j, i_mod)]; // 840 ... -120
        }
    }
}

static void palette_disable(uint32_t disabled)
{
    #ifdef CONFIG_VXWORKS
    return; // see set_ml_palette
    #endif

    if(disabled)
    {
        for (int i = 0; i < 0x100; i++)
        {
            EngDrvOut(LCD_Palette[i*3], 0x00FF0000);
            EngDrvOut(LCD_Palette[i*3+0x300], 0x00FF0000);
        }
    }
    else
    {
        for (int i = 0; i < 0x100; i++)
        {
            EngDrvOut(LCD_Palette[i*3], LCD_Palette[i*3 + 2]);
            EngDrvOut(LCD_Palette[i*3+0x300], LCD_Palette[i*3 + 2]);
        }
    }
}
//~ #endif


static int _bmp_muted = false;
static int _bmp_unmuted = false;
int bmp_is_on() { return !_bmp_muted; }

void bmp_on()
{
    if (!_bmp_unmuted) 
    {
        palette_disable(0);
        _bmp_muted = false; _bmp_unmuted = true;
    }
}

void bmp_off()
{
    if (!_bmp_muted)
    {
        _bmp_muted = true; _bmp_unmuted = false;
        palette_disable(1);
    }
}

void bmp_mute_flag_reset()
{
    _bmp_muted = 0;
    _bmp_unmuted = 0;
}

/* for menu: scale the BMP overlay by 128/denx and 128/deny */
void bmp_zoom(uint8_t* dst, uint8_t* src, int x0, int y0, int denx, int deny)
{
    ASSERT(src);
    ASSERT(dst);
    if (!dst) return;
    int i,j;
    
    // only used for menu => 720x480
    static int16_t js_cache[720];
    
    for (j = 0; j < 720; j++)
        js_cache[j] = (j - x0) * denx / 128 + x0;
    
    for (i = 0; i < 480; i++)
    {
        int is = (i - y0) * deny / 128 + y0;
        uint8_t* dst_r = &dst[BM(0,i)];
        uint8_t* src_r = &src[BM(0,is)];
        
        if (is >= 0 && is < 480)
        {
            for (j = 0; j < 720; j++)
            {
                int js = js_cache[j];
                dst_r[j] = likely(js >= 0 && js < 720) ? src_r[js] : 0;
            }
        }
        else
            bzero32(dst_r, 720);
    }
}

void * bmp_lock = 0;


static void bmp_init(void* unused)
{
    bmp_lock = CreateRecursiveLock(0);
    ASSERT(bmp_lock)
    bvram_mirror_init();
    _update_vram_params();
}

INIT_FUNC(__FILE__, bmp_init);
