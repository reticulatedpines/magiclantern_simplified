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
		}
	}

	//sei( flags );
}


void
bmp_puts(
	unsigned		fontspec,
	unsigned		x,
	unsigned		y,
	const char *		s
)
{
	const uint32_t		pitch = bmp_pitch();
	uint8_t * vram = bmp_vram();
	if( !vram )
		return;
	uint8_t * first_row = vram + y * pitch + x;
	uint8_t * row = first_row;

	char c;

	const struct font * const font = fontspec_font( fontspec );

	while( (c = *s++) )
	{
		if( c == '\n' )
		{
			row = first_row += pitch * font->height;
			continue;
		}

		_draw_char( fontspec, row, c );
		row += font->width;
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

	bmp_puts( fontspec, x, y, buf );
}


int
fprintf(
	FILE *			file,
	const char *		fmt,
	...
)
{
	va_list			ap;
	char			buf[ 256 ];

	va_start( ap, fmt );
	int len = vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	FIO_WriteFile( file, buf, len );
	return len;
}


int
snprintf(
	char *			buf,
	size_t			max_len,
	const char *		fmt,
	...
)
{
	va_list			ap;

	va_start( ap, fmt );
	int len = vsnprintf( buf, max_len, fmt, ap );
	va_end( ap );
	return len;
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
	unsigned size;
	if( FIO_GetFileSize( filename, &size ) != 0 )
		goto getfilesize_fail;

	DebugMsg( DM_MAGIC, 3, "File '%s' size %d bytes",
		filename,
		size
	);

	uint8_t * buf = malloc( size );
	if( !buf )
	{
		DebugMsg( DM_MAGIC, 3, "%s: malloc failed", filename );
		goto malloc_fail;
	}

retry:
	// Retry a few times if necessary
	DebugMsg( DM_MAGIC, 3, "%s: try %d\n", __func__, retry_count );

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
		msleep( 100 );
		if( retry_count++ < 5 )
			goto retry;

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

	bmp->image = buf + image_offset;

	return bmp;

offsetsize_fail:
signature_fail:
read_fail:
	free( buf );
malloc_fail:
getfilesize_fail:
	return NULL;
}
