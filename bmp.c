/** \file
 * Drawing routines.
 *
 * These are Magic Lantern routines to draw into the BMP LVRAM.
 * They are not derived from DryOS routines.
 */
#include "dryos.h"
#include "bmp.h"
#include <stdarg.h>

// This is a DryOS routine that must be located for bmp_printf to work
extern int vsnprintf( char *, size_t, const char *, va_list );


static void
_draw_char(
	uint8_t *	bmp_vram_row,
	char		c
)
{
	unsigned i;
	const uint32_t	pitch		= bmp_pitch() / 4;
	const uint32_t	fg_color	= WHITE_COLOR << 24;
	const uint32_t	bg_color	= BLUE_COLOR << 24;
	uint32_t *	front_row	= (uint32_t *) bmp_vram_row;

	for( i=0 ; i<font_height ; i++ )
	{
		// Start this scanline
		uint32_t * row = front_row;

		// move to the next scanline
		front_row += pitch;

		uint8_t pixels = font[ c + (i << 7) ];
		uint8_t pixel;

		uint32_t bmp_pixels = 0;
		for( pixel=0 ; pixel<4 ; pixel++, pixels <<=1 )
		{
			bmp_pixels >>= 8;
			bmp_pixels |= (pixels & 0x80) ? fg_color : bg_color;
		}

		*(row++) = bmp_pixels;
		bmp_pixels = 0;

		for( pixel=0 ; pixel<4 ; pixel++, pixels <<= 1 )
		{
			bmp_pixels >>= 8;
			bmp_pixels |= (pixels & 0x80) ? fg_color : bg_color;
		}

		*(row++) = bmp_pixels;
	}
}


void
bmp_puts(
	unsigned		x,
	unsigned		y,
	const char *		s
)
{
	const uint32_t		pitch = bmp_pitch();
	uint8_t * first_row = bmp_vram() + y * pitch + x;
	uint8_t * row = first_row;

	char c;

	while( (c = *s++) )
	{
		if( c == '\n' )
		{
			row = first_row += pitch * font_height;
			continue;
		}

		_draw_char( row, c );
		row += font_width;
	}

}

void
bmp_printf(
	unsigned		x,
	unsigned		y,
	const char *		fmt,
	...
)
{
	va_list			ap;
	static char buf[ 256 ] TEXT;

	va_start( ap, fmt );
	vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	bmp_puts( x, y, buf );
}


void
bmp_hexdump(
	unsigned		x,
	unsigned		y,
	const void *		buf,
	size_t			len
)
{
	const uint32_t *	d = (uint32_t*) buf;

	// Round up
	len = (len + 3) / 4;

	while( len-- )
	{
		bmp_printf( x, y, "%08x: %08x %08x %08x %08x",
			(unsigned) d,
			(unsigned) d[0],
			(unsigned) d[1],
			(unsigned) d[2],
			(unsigned) d[3]
		);

		y += font_height;
		d += 4;
	}
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
	const uint32_t start = x/4;
	const uint32_t width = bmp_width();
	const uint32_t pitch = bmp_pitch();
	const uint32_t height = bmp_height();

	// Convert to words and limit to the width of the LCD
	w /= 4;
	if( start + w > width/4 )
		w = width/4 - start;
	
	const uint32_t word = 0
		| (color << 24)
		| (color << 16)
		| (color <<  8)
		| (color <<  0);

	if( y > height )
		y = height;

	if( y + h > height )
		h = height - y;

	if( w == 0 || h == 0 )
		return;

	uint32_t * row = (uint32_t*)( bmp_vram() + y * pitch + start );

	// Loop tests inverted to avoid exraneous jumps.
	// This has the minimal compiled form
	do {
		uint32_t i = w;

		do {
			row[ --i ] = word;
		} while(i);

		row += pitch / 4;
	} while( --h );
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
