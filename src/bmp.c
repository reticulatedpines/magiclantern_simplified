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

static int bmp_idle_flag = 0;
void bmp_draw_to_idle(int value) { bmp_idle_flag = value; }

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
        if (direction)
        {
            for (int i = 0; i < 480; i ++)
                memcpy(real+i*BMPPITCH, idle+i*BMPPITCH, 720);
        }
        else
        {
            for (int i = 0; i < 480; i ++)
                memcpy(idle+i*BMPPITCH, real+i*BMPPITCH, 720);
        }
#endif
    }
}

/*
void bmp_idle_clear()
{
    bzero32(BMP_VRAM_START(bmp_vram_idle()), BMP_VRAM_SIZE);
}*/

/** Returns a pointer to currently selected BMP vram (real or mirror) */
uint8_t * bmp_vram(void)
{
    #ifdef CONFIG_VXWORKS
    set_ml_palette_if_dirty();
    #endif
    uint8_t * bmp_buf = bmp_idle_flag ? bmp_vram_idle() : bmp_vram_real();
    
    if (PLAY_MODE) return UNCACHEABLE(bmp_buf);
    return bmp_buf;
}


#define USE_LUT

static void
_draw_char(
    unsigned    fontspec,
    uint8_t *    bmp_vram_row,
    char        c
)
{
    uint8_t* v = bmp_vram();

    unsigned i,j;
    const struct font * const font = fontspec_font( fontspec );

    if (bmp_vram_row < BMP_VRAM_START(v)) return;
    if (bmp_vram_row >= BMP_VRAM_END(v)) return;

    uint32_t    fg_color    = fontspec_fg( fontspec ) << 24;
    uint32_t    bg_color    = fontspec_bg( fontspec ) << 24;

    // Special case -- fg=bg=0 => white on black
    if( fg_color == 0 && bg_color == 0 )
    {
        fg_color = COLOR_WHITE << 24;
        bg_color = COLOR_BLACK << 24;
    }

    const uint32_t    pitch        = BMPPITCH / 4;
    uint32_t *    front_row    = (uint32_t *) bmp_vram_row;
    
    // boundary checking, don't write past this address
    uint32_t* end = (uint32_t *)(BMP_VRAM_END(v) - font->width);

#ifndef CONFIG_VXWORKS
    //uint32_t flags = cli();
    if ((fontspec & SHADOW_MASK) == 0)
    {
        for( i=0 ; i<font->height ; i++ )
        {
            // Start this scanline
            uint32_t * row = front_row;
            if (row >= end) return;

            // move to the next scanline
            front_row += pitch;

            uint32_t pixels = font->bitmap[ c + (i << 7) ];
            uint8_t pixel;

            for( j=0 ; j<font->width/4 ; j++ )
            {
                uint32_t bmp_pixels = 0;
                for( pixel=0 ; pixel<4 ; pixel++, pixels <<=1 )
                {
                    bmp_pixels >>= 8;
                    bmp_pixels |= (pixels & 0x80000000) ? fg_color : bg_color;
                }

                *(row++) = bmp_pixels;

                // handle characters wider than 32 bits
                //~ if( j == 28/4 )
                    //~ pixels = font->bitmap[ c + ((i+128) << 7) ];
            }
        }
    }
    else // shadowed fonts
    {
        struct sfont * shadow =
        /*#ifdef CONFIG_STATIC_FONTS
            (stshadowruct font *) font;
        #else*/
            font == &font_large ? &font_large_shadow :
            font == &font_med ? &font_med_shadow :
            font == &font_small ? &font_small_shadow : 0;
        //#endif

        fg_color >>= 24;
        bg_color >>= 24;

        for( i=0 ; i<font->height ; i++ )
        {
            // Start this scanline
            uint32_t * row = front_row;
            row = (uint32_t*)((unsigned)row & ~3); // weird artifacts otherwise
            if (row >= end) return;

            // move to the next scanline
            front_row += pitch;

            uint32_t pixels = font->bitmap[ c + (i << 7) ];
            uint32_t pixels_shadow = shadow->bitmap[ c + (i << 7) ];
            uint8_t pixel;

            for( j=0 ; j<font->width/4 ; j++ )
            {
                uint32_t bmp_pixels = *(row);
                
                for( pixel=0 ; pixel<4 ; pixel++, pixels <<=1, pixels_shadow <<=1 )
                {
                    //~ bmp_pixels >>= 8;
                    //~ bmp_pixels |= (*(row) & 0xFF000000);
                    if (pixels & 0x80000000)
                    {
                        bmp_pixels &= ~(0xFF << (pixel*8));
                        bmp_pixels |= (fg_color << (pixel*8));
                    }
                    if (pixels_shadow & 0x80000000)
                    {
                        bmp_pixels &= ~(0xFF << (pixel*8));
                        bmp_pixels |= (bg_color << (pixel*8));
                    }
                }

                *(row++) = bmp_pixels;
            }
        }
    }

#else // 5DC    
    #define FPIX(i,j) (font->bitmap[ c + ((i) << 7) ] & (1 << (31-(j))))
    //- #define BMPIX(i,j) bmp_vram_row[(i) * BMPPITCH + (j)]
    #define BMPIX(i,j,color) char* p = &bmp_vram_row[((i)/2) * BMPPITCH + (j)/2]; SET_4BIT_PIXEL(p, j, color);
    
    if (font == &font_large) // large fonts look better with line skipping
    {
        for( i = 0 ; i<font->height ; i++ )
        {
            for( j=0 ; j<font->width ; j++ )
            {
                if FPIX(i,j)
                {
                    BMPIX(i,j,fg_color>>24);
                }
                else
                {
                    BMPIX(i,j,bg_color>>24);
                }
            }
        }
    }
    else // smaller fonts look better with all foreground pixels displayed (even if they are a bit bolder than normal)
    {
        for( i = 0 ; i<font->height ; i++ )
        {
            for( j=0 ; j<font->width ; j++ )
            {
                if (!FPIX(i,j))
                {
                    BMPIX(i,j,bg_color>>24);
                }
            }
        }

        for( i = 0 ; i<font->height ; i++ )
        {
            for( j=0 ; j<font->width ; j++ )
            {
                if FPIX(i,j)
                {
                    BMPIX(i,j,fg_color>>24);
                }
            }
        }
    }

#endif
}


void
bmp_puts(
    unsigned        fontspec,
    int *        x,
    int *        y,
    const char *        s
)
{
#ifdef CONFIG_1100D
    //fonts look much better if coordinates are odd
    //hack until there is a better medium font for 1100D
    *x = *x | 1;
    *y = *y | 1;
#endif

    *x = COERCE(*x, BMP_W_MINUS, BMP_W_PLUS);
    *y = COERCE(*y, BMP_H_MINUS, BMP_H_PLUS);
    
    const uint32_t        pitch = BMPPITCH;
    uint8_t * vram = bmp_vram();
    if( !vram || ((uintptr_t)vram & 1) == 1 )
        return;
    const int initial_x = *x;
#ifdef CONFIG_VXWORKS
    uint8_t * first_row = vram + ((*y)/2) * pitch + ((*x)/2);
#else
    uint8_t * first_row = vram + (*y) * pitch + (*x);
#endif
    uint8_t * row = first_row;

    char c;

    const struct font * const font = fontspec_font( fontspec );

    while( (c = *s++) )
    {
        if( c == '\n' )
        {
            #ifdef CONFIG_VXWORKS
            row = first_row += pitch * font->height/2;
            #else
            row = first_row += pitch * font->height;
            #endif
            (*y) += font->height;
            (*x) = initial_x;
            continue;
        }

        if( c == '\b' )
        {
            (*x) -= font->width;
            continue;
        }

        _draw_char( fontspec, row, c );
        #ifdef CONFIG_VXWORKS
        row += font->width / 2;
        #else
        row += font->width;
        #endif
        (*x) += font->width;
    }

}

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
            if (lv) msleep(1);
            if (c == '\n') continue;
        }

        _draw_char( fontspec, row, c );
        row += font->width;
        (*x) += font->width;
        i++;
    }

}


// thread safe
void
bmp_printf(
           unsigned        fontspec,
           int        x,
           int        y,
           const char *        fmt,
           ...
           )
{
    va_list            ap;
    
    char bmp_printf_buf[128];
    
    va_start( ap, fmt );
    vsnprintf( bmp_printf_buf, sizeof(bmp_printf_buf), fmt, ap );
    va_end( ap );
    
    bmp_puts( fontspec, &x, &y, bmp_printf_buf );
}

// for very large strings only
void
big_bmp_printf(
               unsigned        fontspec,
               int        x,
               int        y,
               const char *        fmt,
               ...
               )
{
    BMP_LOCK(
             va_list            ap;
             
             static char bmp_printf_buf[1024];
             
             va_start( ap, fmt );
             vsnprintf( bmp_printf_buf, sizeof(bmp_printf_buf), fmt, ap );
             va_end( ap );
             
             bmp_puts( fontspec, &x, &y, bmp_printf_buf );
             )
}

#if 0
void
con_printf(
    unsigned        fontspec,
    const char *        fmt,
    ...
)
{
    va_list            ap;
    char            buf[ 256 ];
    static int        x = 0;
    static int        y = 32;

    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf), fmt, ap );
    va_end( ap );

    const uint32_t        pitch = BMPPITCH;
    uint8_t * vram = bmp_vram();
    if( !vram )
        return;
    uint8_t * first_row = vram + y * pitch + x;
    uint8_t * row = first_row;

    char * s = buf;
    char c;
    const struct font * const font = fontspec_font( fontspec );

    while( (c = *s++) )
    {
        if( c == '\n' )
        {
            row = first_row += pitch * font->height;
            y += font->height;
            x = 0;
            bmp_fill( 0, 0, y, 720, font->height );
        } else {
            _draw_char( fontspec, row, c );
            row += font->width;
            x += font->width;
        }

        if( x > 720 )
        {
            y += font->height;
            x = 0;
            bmp_fill( 0, 0, y, 720, font->height );
        }

        if( y > 480 )
        {
            x = 0;
            y = 32;
            bmp_fill( 0, 0, y, 720, font->height );
        }
    }
}
#endif

#if 1

void
bmp_hexdump(
    unsigned        fontspec,
    unsigned        x,
    unsigned        y,
    const void *        buf,
    int            len
)
{
    if( len == 0 )
        return;

    // Round up
    len = (len + 15) & ~15;

    const uint32_t *    d = (uint32_t*) buf;

    do {
        bmp_printf(
            fontspec,
            x,
            y,
            #ifdef CONFIG_VXWORKS // low-res screen, need to use larger font
            "%08x: %08x %08x %08x %08x : %04x",
            #else
            "%08x: %08x %08x %08x %08x %08x %08x %08x %08x : %04x",
            #endif
            (unsigned) d,
            len >  0 ? MEMX(d+0) : 0,
            len >  4 ? MEMX(d+1) : 0,
            len >  8 ? MEMX(d+2) : 0,
            len > 12 ? MEMX(d+3) : 0,
            #ifndef CONFIG_VXWORKS
            len > 16 ? MEMX(d+4) : 0,
            len > 20 ? MEMX(d+5) : 0,
            len > 24 ? MEMX(d+6) : 0,
            len > 28 ? MEMX(d+7) : 0,
            #endif
            (void*)d - (void*)buf
        );
        y += fontspec_height( fontspec );
        #ifdef CONFIG_VXWORKS
        d += 4;
        len -= 16;
        #else
        d += 8;
        len -= 32;
        #endif
    } while(len > 0);
}
#endif

/** Fill a section of bitmap memory with solid color
 * Only has a four-pixel resolution in X.
 */

/* will allow 1-pixel resolution in X and also enables BMP_FILL_HALFALIGN which is 2-pixel resolution */
//#define BMP_FILL_BYTEALIGN

/** Fill a section of bitmap memory with solid color
 * Only has a four-pixel resolution in X.
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
   
    const uint16_t halfColor = ((uint16_t)color << 8) | color;
    const uint32_t wordColor = ((uint32_t)halfColor << 16) | halfColor;
    const uint64_t dwordColor = ((uint64_t)wordColor << 32) | wordColor;
   
    uint8_t* b = bmp_vram();
   
    /* pre-align the pixels to speed up fill routine below.
       will draw pixels for byte addresses (if enabled) so the code later can use a optimized dword-store operation.
       if using x positions and widths that are 8-pixel aligned, this code will not get called.
      
       __builtin_expect(exp, result) tells the compiler what we think the result is in most of the cases.
       this will improve performance by not-jumping around, but continously executing code.
       will reduce the perfomance impact of the byte-/word- aligning routines when enabled.
     */
#if defined(BMP_FILL_BYTEALIGN)
    if(unlikely(x & 1))
    {
        for (int posY = y; posY < y+h; posY++)
        {
            *((uint8_t *)&(b[BM(x,posY)])) = color;
        }
        x += 1;
        w -= 1;
    }
    if(unlikely(w & 1))
    {
        w -= 1;
        for (int posY = y; posY < y+h; posY++)
        {
            *((uint8_t *)&(b[BM(x+w,posY)])) = color;
        }
    }
#endif
#if defined(BMP_FILL_BYTEALIGN) || defined(BMP_FILL_HALFALIGN)
    if(unlikely(x & 2))
    {
        for (int posY = y; posY < y+h; posY++)
        {
            *((uint16_t *)&(b[BM(x,posY)])) = halfColor;
        }
        x += 2;
        w -= 2;
    }
    if(unlikely(w & 2))
    {
        w -= 2;
        for (int posY = y; posY < y+h; posY++)
        {
            *((uint16_t *)&(b[BM(x+w,posY)])) = halfColor;
        }
    }
#endif
    /* any 32-bit access necessary? not that complex to handle.
       STRD in the loop later allows 32-bit aligned addresses, so no special treatment needed to make the memory addresses 64 bit aligned.
       unfortunately the compiler does not generate STRDs, but STM.
      
       tell the compiler, is is unlikely that this condition is true, as many paints may start at e.g. 0 and end with e.g. 720
    */
    if(unlikely(w & 4))
    {
        /* fill a column with 32 bit writes */
        for (int posY = y; posY < y+h; posY++)
        {
            *((uint32_t *)&(b[BM(x,posY)])) = wordColor;
        }
        x += 4;
        w -= 4;
    }

    /* not needed if BMP_FILL_BYTEALIGN is set, but doesnt hurt. its just to protect from misaligned accesses */
    x &= ~3;
    w &= ~7;
   
    /* finally fill with optimized 64 bit writes.
       planned to let the compiler generate STRD that uses only 2 cpu cycles plus ADD that needs 1.
       but we only get STMIA. the generated STMIA is also fine, but that uses 4 instead of 3 cpu cycles. still not that bad.
     */
    for (int i = y; i < y+h; i++)
    {
        uint32_t buffer = (uint32_t)&(b[BM(x,i)]);
        uint32_t bufferEnd = (uint32_t)&(b[BM(x+w,i)]);
       
        while (buffer < bufferEnd)
        {
            *((uint64_t*)buffer) = dwordColor;
            buffer += 8;
        }
    }
}

/** Draw a picture of the BMP color palette. */
void
bmp_draw_palette( void )
{
    uint32_t x, y, msb, lsb;
    const uint32_t height = 30;
    const uint32_t width = 45;

    for( msb=0 ; msb<16; msb++ )
    {
        for( y=0 ; y<height; y++ )
        {
            uint8_t * const row = bmp_vram() + (y + height*msb) * BMPPITCH;

            for( lsb=0 ; lsb<16 ; lsb++ )
            {
                for( x=0 ; x<width ; x++ )
                    row[x+width*lsb] = (msb << 4) | lsb;
            }
        }
    }

    static int written;
    if( !written )
        call("dispcheck");
    written = 1;
    msleep(2000);
}

int retry_count = 0;


size_t
read_file(
    const char *        filename,
    void *            buf,
    size_t            size
)
{
    FILE * file = FIO_Open( filename, O_RDONLY | O_SYNC );
    if( file == INVALID_PTR )
        return -1;
    unsigned rc = FIO_ReadFile( file, buf, size );
    FIO_CloseFile( file );

    if( rc == size )
        return size;

    DebugMsg( DM_MAGIC, 3, "%s: size=%d rc=%d", filename, size, rc );
    return -1;
}


/** Load a BMP file into memory so that it can be drawn onscreen */
struct bmp_file_t *
bmp_load(
    const char *        filename,
    uint32_t         compression // what compression to load the file into. 0: none, 1: RLE8
)
{
    DebugMsg( DM_MAGIC, 3, "bmp_load(%s)", filename);
    unsigned size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

    DebugMsg( DM_MAGIC, 3, "File '%s' size %d bytes",
        filename,
        size
    );
    
    uint8_t * buf = alloc_dma_memory( size );
    if( !buf )
    {
        DebugMsg( DM_MAGIC, 3, "%s: alloc_dma_memory failed", filename );
        goto malloc_fail;
    }

    size_t i;
    for( i=0 ; i<size; i++ )
        buf[i] = 'A' + i;
    size_t rc = read_file( filename, buf, size );
    if( rc != size )
        goto read_fail;


    struct bmp_file_t * bmp = (struct bmp_file_t *) buf;
    if( bmp->signature != 0x4D42 )
    {
        DebugMsg( DM_MAGIC, 3, "%s: signature %04x", filename, bmp->signature );
        int i;
        for( i=0 ; i<64; i += 16 )
            DebugMsg( DM_MAGIC, 3,
                "%08x: %08x %08x %08x %08x",
                buf + i,
                ((uint32_t*)(buf + i))[0],
                ((uint32_t*)(buf + i))[1],
                ((uint32_t*)(buf + i))[2],
                ((uint32_t*)(buf + i))[3]
            );

        goto signature_fail;
    }

    // Update the offset pointer to point to the image data
    // if it is within bounds
    const unsigned image_offset = (unsigned) bmp->image;
    if( image_offset > size )
    {
        DebugMsg( DM_MAGIC, 3, "%s: size too large: %x > %x", filename, image_offset, size );
        goto offsetsize_fail;
    }

    // Since the read was into uncacheable memory, it will
    // be very slow to access.  Copy it into a cached buffer
    // and release the uncacheable space.

    if (compression==bmp->compression) {
        uint8_t * fast_buf = AllocateMemory( size + 32);
        if( !fast_buf )
            goto fail_buf_copy;
        memcpy(fast_buf, buf, size);
        bmp = (struct bmp_file_t *) fast_buf;
        bmp->image = fast_buf + image_offset;
        free_dma_memory( buf );
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
        fast_buf = AllocateMemory( size_needed );
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
        free_dma_memory( buf );
        //~ bmp_printf(FONT_SMALL,0,440,"Memory needed %d",size_needed);
        return bmp;
    }

fail_buf_copy:
offsetsize_fail:
signature_fail:
read_fail:
    free_dma_memory( buf );
malloc_fail:
getfilesize_fail:
    DebugMsg( DM_MAGIC, 3, "bmp_load failed");
    return NULL;
}


uint8_t* read_entire_file(const char * filename, int* buf_size)
{
    *buf_size = 0;
    unsigned size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

    DEBUG("File '%s' size %d bytes", filename, size);

    uint8_t * buf = alloc_dma_memory( size );
    if( !buf )
    {
        DebugMsg( DM_MAGIC, 3, "%s: alloc_dma_memory failed", filename );
        goto malloc_fail;
    }
    size_t rc = read_file( filename, buf, size );
    if( rc != size )
        goto read_fail;

    *buf_size = size;

    return buf;

//~ fail_buf_copy:
read_fail:
    free_dma_memory( buf );
malloc_fail:
getfilesize_fail:
    DEBUG("failed");
    return NULL;
}


void clrscr()
{
    BMP_LOCK( bmp_fill( 0x0, BMP_W_MINUS, BMP_H_MINUS, BMP_TOTAL_WIDTH, BMP_TOTAL_HEIGHT ); )
}

#if 0
// mirror can be NULL
void bmp_draw(struct bmp_file_t * bmp, int x0, int y0, uint8_t* const mirror, int clear)
{
    if (!bmp) return;
    //~ if (!bmp_enabled) return;
    if (bmp->compression!=0) return; // bmp_draw doesn't support RLE yet

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    
    x0 = COERCE(x0, BMP_W_MINUS, BMP_W_PLUS - (int)bmp->width);
    y0 = COERCE(y0, BMP_H_MINUS, BMP_H_PLUS - (int)bmp->height);
    
    uint32_t x,y;
    for( y=0 ; y < bmp->height; y++ )
    {
        uint8_t * const b_row = (uint8_t*)( bvram + (y + y0) * BMPPITCH );
        uint8_t * const m_row = (uint8_t*)( mirror+ (y + y0) * BMPPITCH );
        for( x=0 ; x < bmp->width ; x++ )
        {
            if (clear)
            {
                uint8_t p = b_row[ x + x0 ];
                uint8_t pix = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
                if (pix && p == pix)
                    b_row[x + x0] = 0;
            }
            else
            {
                if (mirror)
                {
                    uint8_t p = b_row[ x + x0 ];
                    uint8_t m = m_row[ x + x0 ];
                    if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue;
                }
                uint8_t pix = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
                b_row[x + x0] = pix;
            }
        }
    }
}
#endif
/*
void bmp_draw_scaled(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax)
{
    if (!bmp) return;

    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;

    int x,y; // those sweep the original bmp
    int xs,ys; // those sweep the BMP VRAM (and are scaled)
    
    #ifdef USE_LUT 
    // we better don't use AllocateMemory for LUT (Err 70)
    static int16_t lut[960];
    for (xs = x0; xs < (x0 + xmax); xs++)
    {
        lut[xs] = (xs-x0) * bmp->width/xmax;
    }
    #endif

    for( ys = y0 ; ys < (y0 + ymax); ys++ )
    {
        y = (ys-y0)*bmp->height/ymax;
        uint8_t * const b_row = bvram + ys * BMPPITCH;
        for (xs = x0; xs < (x0 + xmax); xs++)
        {
#ifdef USE_LUT
            x = lut[xs];
#else
            x = (xs-x0)*bmp->width/xmax;
#endif
            uint8_t pix = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
            b_row[ xs ] = pix;
        }
    }
}*/

// this is slow, but is good for a small number of pixels :)
uint8_t bmp_getpixel(int x, int y)
{
    ASSERT(x >= BMP_W_MINUS && x < BMP_W_PLUS)
    ASSERT(y >= BMP_H_MINUS && y < BMP_H_PLUS)

    uint8_t * const bvram = bmp_vram();
    return bvram[x + y * BMPPITCH];
}

/*uint8_t bmp_getpixel_real(int x, int y)
{
    ASSERT(x >= BMP_W_MINUS && x < BMP_W_PLUS)
    ASSERT(y >= BMP_H_MINUS && y < BMP_H_PLUS)

    uint8_t * const bvram = bmp_vram_real();
    return bvram[x + y * BMPPITCH];
}*/

void bmp_putpixel(int x, int y, uint8_t color)
{
    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    x = COERCE(x, BMP_W_MINUS, BMP_W_PLUS-1);
    y = COERCE(y, BMP_H_MINUS, BMP_H_PLUS-1);
    
    bmp_putpixel_fast(bvram, x, y, color);
}

void bmp_draw_rect(uint8_t color, int x0, int y0, int w, int h)
{
    uint8_t * const bvram = bmp_vram();
    if (!bvram) return;
    
    draw_line(x0,   y0,   x0+w,   y0, color);
    draw_line(x0+w, y0,   x0+w, y0+h, color);
    draw_line(x0+w, y0+h,   x0, y0+h, color);
    draw_line(x0,   y0,     x0, y0+h, color);
}

int _bmp_draw_should_stop = 0;
void bmp_draw_request_stop() { _bmp_draw_should_stop = 1; }

#ifdef CONFIG_VXWORKS

/** converting dryos palette to vxworks one */
char bmp_lut[80] = { 
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
        FILE* f = FIO_CreateFileEx(CARD_DRIVE"pb.log");
        for (int i = 0; i < 16; i++)
            my_fprintf(f, "0x%08x, ", PB_Palette[i*3 + 2]);
        FIO_CloseFile(f);
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
        // we better don't use AllocateMemory for LUT (Err 70)
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
int bfnt_ok()
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
uint8_t* bfnt_find_char(int code)
{
    int n = (BFNT_BITMAP_OFFSET - BFNT_CHAR_CODES) / 4;
    int* codes = (int*) BFNT_CHAR_CODES;
    int* off = (int*) BFNT_BITMAP_OFFSET;
    
    if (code <= 'z') return (uint8_t*) (BFNT_BITMAP_DATA + off[code - 0x20]);
    
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
    
    //~ bmp_fill(bg, px, py, crw+xo+3, 40);
    
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
                        bmp_putpixel_fast(bvram, px+j*8+k+xo, py + (i+yo)*2, fg);
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

/*
int bfnt_draw_char_half(int c, int px, int py, int fg, int bg, int g1, int g2)
{
    if (!bfnt_ok())
    {
        bmp_printf(FONT_SMALL, 0, 0, "font addr bad");
        return 0;
    }
    
    uint16_t* chardata = bfnt_find_char(c);
    if (!chardata) return 0;
    uint8_t* buff = chardata + 5;
    int ptr = 0;
    
    unsigned int cw  = chardata[0]; // the stored bitmap width
    unsigned int ch  = chardata[1]; // the stored bitmap height
    unsigned int crw = chardata[2]; // the displayed character width
    unsigned int xo  = chardata[3]; // X offset for displaying the bitmap
    unsigned int yo  = chardata[4]; // Y offset for displaying the bitmap
    unsigned int bb    = cw / 8 + (cw % 8 == 0 ? 0 : 1); // calculate the byte number per line
    
    //~ bmp_printf(FONT_SMALL, 0, 0, "%x %d %d %d %d %d %d", chardata, cw, ch, crw, xo, yo, bb);
    
    if (cw > 100) return 0;
    if (ch > 50) return 0;
    
    static uint8_t tmp[50][25];

    int i,j,k;

    for (i = 0; i < 50; i++)
        for (j = 0; j < 25; j++)
            tmp[i][j] = 0;
    
    for (i = 0; i < ch; i++)
    {
        for (j = 0; j < bb; j++)
        {
            for (k = 0; k < 8; k++)
            {
                if (j*8 + k < cw)
                {
                    if ((buff[ptr+j] & (1 << (7-k)))) 
                        tmp[COERCE((j*8+k)>>1, 0, 49)][COERCE(i>>1, 0, 24)] ++;
                }
            }
        }
        ptr += bb;
    }

    bmp_fill(bg, px+3, py, crw/2+xo/2+3, 20);

    for (i = 0; i <= cw/2; i++)
    {
        for (j = 0; j <= ch/2; j++)
        {
            int c = COLOR_RED;
            switch (tmp[i][j])
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    c = bg;
                    break;
                case 4:
                    c = fg;
                    break;
            }
            if (c != bg) bmp_putpixel(px+xo/2+i, py+yo/2+j, c);
        }
    }

    return crw>>1;
}*/

void bfnt_puts(char* s, int x, int y, int fg, int bg)
{
    while (*s)
    {
        x += bfnt_draw_char(*s, x, y, fg, bg);
        s++;
    }
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

#if 1
void
bfnt_test()
{
    while(1)
    {
        beep();
        canon_gui_disable_front_buffer();
        int* codes = (int*) BFNT_CHAR_CODES;
        static int c = 0;
        c = (BFNT_BITMAP_OFFSET - BFNT_CHAR_CODES) / 4 - 50;
        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 10; j++)
            {
                bfnt_draw_char(codes[c], j*70, i*80+20, COLOR_WHITE, COLOR_BLACK);
                bmp_printf(FONT_MED, j*70, i*80, "%x", codes[c]);
                c++;
            }
        }
        msleep(2000);
        clrscr();
    }
}
#endif

void bmp_flip(uint8_t* dst, uint8_t* src, int voffset)
{
    ASSERT(src)
    ASSERT(dst)
    if (!dst) return;
    int i,j;
    
    int H_LO = hdmi_code == 5 ? BMP_H_MINUS : 0;
    int H_HI = hdmi_code == 5 ? BMP_H_PLUS : 480;

    int W_LO = hdmi_code == 5 ? BMP_W_MINUS : 0;
    int W_HI = hdmi_code == 5 ? BMP_W_PLUS : 720;
    
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
    
    int H_LO = hdmi_code == 5 ? BMP_H_MINUS : 0;
    int H_HI = hdmi_code == 5 ? BMP_H_PLUS : 480;

    int W_LO = hdmi_code == 5 ? BMP_W_MINUS : 0;
    int W_HI = hdmi_code == 5 ? BMP_W_PLUS : 720;
    
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

static void bmp_dim_line(void* dest, size_t n, int even)
{
    ASSERT(dest);

    int* dst = (int*) dest;
    int* end = (int*)(dest + n);
    if (even)
    {
        for( ; dst < end; dst++)
#ifdef CONFIG_VXWORKS
            *dst = (*dst & 0x0F0F0F0F) | 0x90909090;
#else
            *dst = (*dst & 0x00FF00FF) | 0x02000200;
#endif
    }
    else
    {
        for( ; dst < end; dst++)
#ifdef CONFIG_VXWORKS
            *dst = (*dst & 0xF0F0F0F0) | 0x09090909;
#else
            *dst = (*dst & 0xFF00FF00) | 0x00020002;
#endif
    }
}

// this is only used in menu, so only cover the 720x480 part
void bmp_dim()
{
    uint32_t* b = (uint32_t *)bmp_vram();
    ASSERT(b);
    if (!b) return;
    int i;
    //int j;
#ifdef CONFIG_VXWORKS
    for (i = 0; i < 480; i+=2)
    {
        bmp_dim_line(&b[BM(0,i)/4], 360, (i/2)%2);
    }
#else
    for (i = 0; i < 480; i ++)
    {
        bmp_dim_line(&b[BM(0,i)/4], 720, i%2);
    }
#endif
}

void bmp_make_semitransparent()
{
    uint8_t* b = (uint8_t *)bmp_vram();
    ASSERT(b);
    if (!b) return;
    int i,j;
    for (i = BMP_H_MINUS; i < BMP_H_PLUS; i ++)
    {
        for (j = BMP_W_MINUS; j < BMP_W_PLUS; j ++)
        {
            if (b[BM(j,i)] == COLOR_BLACK || b[BM(j,i)] == 40)
                b[BM(j,i)] = COLOR_BG;
        }
    }
}

void * bmp_lock = 0;

void bmp_init(void* unused)
{
    bmp_lock = CreateRecursiveLock(0);
    ASSERT(bmp_lock)
    bvram_mirror_init();
    update_vram_params();
}

INIT_FUNC(__FILE__, bmp_init);
