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

#define USE_LUT

extern int bmp_enabled = 1; // global enable/disable for Bitmap Overlay

static void
_draw_char(
	unsigned	fontspec,
	uint8_t *	bmp_vram_row,
	char		c
)
{
	unsigned i,j;
	const struct font * const font = fontspec_font( fontspec );

	uint32_t	fg_color	= fontspec_fg( fontspec ) << 24;
	uint32_t	bg_color	= fontspec_bg( fontspec ) << 24;

	// Special case -- fg=bg=0 => white on transparent
	if( fg_color == 0 && bg_color == 0 )
	{
		fg_color = COLOR_WHITE << 24;
		bg_color = COLOR_BG << 24;
	}

	const uint32_t	pitch		= bmp_pitch() / 4;
	uint32_t *	front_row	= (uint32_t *) bmp_vram_row;

	//uint32_t flags = cli();
	for( i=0 ; i<font->height ; i++ )
	{
		// Start this scanline
		uint32_t * row = front_row;

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
			if( j == 28/4 )
				pixels = font->bitmap[ c + ((i+128) << 7) ];
		}
	}

	//sei( flags );
}


void
bmp_puts(
	unsigned		fontspec,
	unsigned *		x,
	unsigned *		y,
	const char *		s
)
{
	if (!bmp_enabled) return;
	
	const uint32_t		pitch = bmp_pitch();
	uint8_t * vram = bmp_vram();
	if( !vram || ((uintptr_t)vram & 1) == 1 )
		return;
	const unsigned initial_x = *x;
	uint8_t * first_row = vram + (*y) * pitch + (*x);
	uint8_t * row = first_row;

	char c;

	const struct font * const font = fontspec_font( fontspec );

	while( (c = *s++) )
	{
		if( c == '\n' )
		{
			row = first_row += pitch * font->height;
			(*y) += font->height;
			(*x) = initial_x;
			continue;
		}

		_draw_char( fontspec, row, c );
		row += font->width;
		(*x) += font->width;
	}

}

void
bmp_puts_w(
	unsigned		fontspec,
	unsigned *		x,
	unsigned *		y,
	unsigned max_chars_per_line,
	const char *		s
)
{
	const uint32_t		pitch = bmp_pitch();
	uint8_t * vram = bmp_vram();
	if( !vram || ((uintptr_t)vram & 1) == 1 )
		return;
	const unsigned initial_x = *x;
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
			if (lv_drawn()) msleep(1);
			if (c == '\n') continue;
		}

		_draw_char( fontspec, row, c );
		row += font->width;
		(*x) += font->width;
		i++;
	}

}


void
bmp_printf(
	unsigned		fontspec,
	unsigned		x,
	unsigned		y,
	const char *		fmt,
	...
)
{
	va_list			ap;
	char			buf[ 256 ];

	va_start( ap, fmt );
	vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	bmp_puts( fontspec, &x, &y, buf );
}

void
con_printf(
	unsigned		fontspec,
	const char *		fmt,
	...
)
{
	va_list			ap;
	char			buf[ 256 ];
	static int		x = 0;
	static int		y = 32;

	va_start( ap, fmt );
	int len = vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	const uint32_t		pitch = bmp_pitch();
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


void
bmp_hexdump(
	unsigned		fontspec,
	unsigned		x,
	unsigned		y,
	const void *		buf,
	size_t			len
)
{
	if( len == 0 )
		return;

	// Round up
	len = (len + 15) & ~15;

	const uint32_t *	d = (uint32_t*) buf;

	do {
		bmp_printf(
			fontspec,
			x,
			y,
			"%08x: %08x %08x %08x %08x",
			(unsigned) d,
			len >  0 ? (unsigned) d[ 0/4] : 0,
			len >  4 ? (unsigned) d[ 4/4] : 0,
			len >  8 ? (unsigned) d[ 8/4] : 0,
			len > 12 ? (unsigned) d[12/4] : 0
		);

		y += fontspec_height( fontspec );
		d += 4;
		len -= 16;
	} while(len);
}


/** Fill a section of bitmap memory with solid color
 * Only has a four-pixel resolution in X.
 */
void
bmp_fill(
	uint8_t			color,
	uint32_t		x,
	uint32_t		y,
	uint32_t		w,
	uint32_t		h
)
{
	if (!bmp_enabled) return;
	
	const uint32_t start = x;
	const uint32_t width = bmp_width();
	const uint32_t pitch = bmp_pitch();
	const uint32_t height = bmp_height();

	// Convert to words and limit to the width of the LCD
	if( start + w > width )
		w = width - start;
	
	const uint32_t word = 0
		| (color << 24)
		| (color << 16)
		| (color <<  8)
		| (color <<  0);

	if( y > height )
		y = height;

	uint16_t y_end = y + h;
	if( y_end > height )
		y_end = height;

	if( w == 0 || h == 0 )
		return;

	uint8_t * const vram = bmp_vram();
	uint32_t * row = (void*)( vram + y * pitch + start );

	if( !vram || ( 1 & (uintptr_t) vram ) )
	{
		//sei( flags );
		return;
	}


	for( ; y<y_end ; y++, row += pitch/4 )
	{
		uint32_t x;

		for( x=0 ; x<w/4 ; x++ )
		{
			row[ x ] = word;
			asm( "nop" );
			asm( "nop" );
			asm( "nop" );
			asm( "nop" );
		}
	}
}

/** Draw a picture of the BMP color palette. */
void
bmp_draw_palette( void )
{
	gui_stop_menu();
	uint32_t x, y, msb, lsb;
	const uint32_t height = 30;
	const uint32_t width = 45;

	for( msb=0 ; msb<16; msb++ )
	{
		for( y=0 ; y<height; y++ )
		{
			uint8_t * const row = bmp_vram() + (y + height*msb) * bmp_pitch();

			for( lsb=0 ; lsb<16 ; lsb++ )
			{
				for( x=0 ; x<width ; x++ )
					row[x+width*lsb] = (msb << 4) | lsb;
			}
		}
	}

	static int written;
	if( !written )
		dispcheck();
	written = 1;
}

int retry_count = 0;


size_t
read_file(
	const char *		filename,
	void *			buf,
	size_t			size
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
	const char *		filename
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
	uint8_t * fast_buf = AllocateMemory( size + 32);
	if( !fast_buf )
		goto fail_buf_copy;
	memcpy(fast_buf, buf, size);
	bmp = (struct bmp_file_t *) fast_buf;
	bmp->image = fast_buf + image_offset;
	free_dma_memory( buf );
	return bmp;

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

void clrscr()
{
	bmp_fill( 0x0, 0, 0, 960, 540 );
}

// mirror can be NULL
void bmp_draw(struct bmp_file_t * bmp, int x0, int y0, uint8_t* const mirror, int clear)
{
	if (!bmp) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	
	x0 = COERCE(x0, 0, 960 - bmp->width);
	y0 = COERCE(y0, 0, 540 - bmp->height);
	if (x0 < 0) return;
	if (x0 + bmp->width > 960) return;
	if (y0 < 0) return;
	if (y0 + bmp->height > 960) return;
	
	int bmppitch = bmp_pitch();
	uint32_t x,y;
	for( y=0 ; y < bmp->height; y++ )
	{
		uint8_t * const b_row = (uint8_t*)( bvram + (y + y0) * bmppitch );
		uint8_t * const m_row = (uint8_t*)( mirror+ (y + y0) * bmppitch );
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

void bmp_draw_scaled(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax)
{
	if (!bmp) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;

	int bmppitch = bmp_pitch();
	int x,y; // those sweep the original bmp
	int xs,ys; // those sweep the BMP VRAM (and are scaled)
	
	#ifdef USE_LUT 
	// we better don't use AllocateMemory for LUT (Err 70)
	static int lut[960];
	for (xs = x0; xs < (x0 + xmax); xs++)
	{
		lut[xs] = (xs-x0) * bmp->width/xmax;
	}
	#endif

	for( ys = y0 ; ys < (y0 + ymax); ys++ )
	{
		y = (ys-y0)*bmp->height/ymax;
		uint8_t * const b_row = bvram + ys * bmppitch;
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
}

// this is slow, but is good for a small number of pixels :)
uint8_t bmp_getpixel(int x, int y)
{
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return 0;
	int bmppitch = bmp_pitch();

	uint8_t * const b_row = bvram + y * bmppitch;
	return b_row[x];
}


void bmp_draw_scaled_ex(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax, uint8_t* const mirror, int clear)
{
	if (!bmp) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;

	int bmppitch = bmp_pitch();
	int x,y; // those sweep the original bmp
	int xs,ys; // those sweep the BMP VRAM (and are scaled)
	
	#ifdef USE_LUT 
	// we better don't use AllocateMemory for LUT (Err 70)
	static int lut[960];
	for (xs = x0; xs < (x0 + xmax); xs++)
	{
		lut[xs] = (xs-x0) * bmp->width/xmax;
	}
	#endif

	for( ys = y0 ; ys < (y0 + ymax); ys++ )
	{
		y = (ys-y0)*bmp->height/ymax;
		uint8_t * const b_row = bvram + ys * bmppitch;
		uint8_t * const m_row = (uint8_t*)( mirror+ (y + y0) * bmppitch );
		for (xs = x0; xs < (x0 + xmax); xs++)
		{
#ifdef USE_LUT
			x = lut[xs];
#else
			x = (xs-x0)*bmp->width/xmax;
#endif

			if (clear)
			{
				uint8_t p = b_row[ xs ];
				uint8_t pix = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
				if (pix && p == pix)
					b_row[xs] = 0;
			}
			else
			{
				if (mirror)
				{
					uint8_t p = b_row[ xs ];
					uint8_t m = m_row[ xs ];
					if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue;
				}
				uint8_t pix = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
				b_row[ xs ] = pix;
			}
		}
	}
}
