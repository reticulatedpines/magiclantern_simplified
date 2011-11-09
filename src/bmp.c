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

static int bmp_mirror_flag = 0;
void bmp_draw_to_mirror(int value) { bmp_mirror_flag = value; }

// 0 = copy BMP to mirror
// 1 = copy mirror to BMP
void bmp_mirror_copy(int direction)
{
	uint8_t* mirror = get_bvram_mirror();
	if (!mirror) { beep(); return; }
	if (direction)
		memcpy(bmp_vram_info[1].vram2, mirror, BMP_HEIGHT * BMPPITCH);
	else
		memcpy(mirror, bmp_vram_info[1].vram2, BMP_HEIGHT * BMPPITCH);
}

/** Returns a pointer to the real BMP vram */
uint8_t* bmp_vram_real()
{
	return bmp_vram_info[1].vram2;
}

/** Returns a pointer to currently selected BMP vram (real or mirror) */
uint8_t * bmp_vram(void)
{
	return bmp_mirror_flag ? get_bvram_mirror() : bmp_vram_info[1].vram2;
}


#define USE_LUT

static void
_draw_char(
	unsigned	fontspec,
	uint8_t *	bmp_vram_row,
	char		c
)
{
	//~ if (!bmp_enabled) return;
	unsigned i,j;
	const struct font * const font = fontspec_font( fontspec );

	uint32_t	fg_color	= fontspec_fg( fontspec ) << 24;
	uint32_t	bg_color	= fontspec_bg( fontspec ) << 24;

	// Special case -- fg=bg=0 => white on black
	if( fg_color == 0 && bg_color == 0 )
	{
		fg_color = COLOR_WHITE << 24;
		bg_color = COLOR_BLACK << 24;
	}

	const uint32_t	pitch		= BMPPITCH / 4;
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
	const uint32_t		pitch = BMPPITCH;
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
	int *		x,
	int *		y,
	int max_chars_per_line,
	const char *		s
)
{
	const uint32_t		pitch = BMPPITCH;
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


static char bmp_printf_buf[1024];

void
bmp_printf(
	unsigned		fontspec,
	unsigned		x,
	unsigned		y,
	const char *		fmt,
	...
)
{
BMP_LOCK(
	va_list			ap;

	va_start( ap, fmt );
	vsnprintf( bmp_printf_buf, sizeof(bmp_printf_buf), fmt, ap );
	va_end( ap );

	bmp_puts( fontspec, &x, &y, bmp_printf_buf );
)
}

#if 0
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
	vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	const uint32_t		pitch = BMPPITCH;
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
			"%08x: %08x %08x %08x %08x %08x %08x %08x %08x ",
			(unsigned) d,
			len >  0 ? (unsigned) d[ 0/4] : 0,
			len >  4 ? (unsigned) d[ 4/4] : 0,
			len >  8 ? (unsigned) d[ 8/4] : 0,
			len > 12 ? (unsigned) d[12/4] : 0,
			len > 16 ? (unsigned) d[16/4] : 0,
			len > 20 ? (unsigned) d[20/4] : 0,
			len > 24 ? (unsigned) d[24/4] : 0,
			len > 28 ? (unsigned) d[28/4] : 0
		);

		y += fontspec_height( fontspec );
		d += 8;
		len -= 32;
	} while(len);
}
#endif

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
	//~ if (!bmp_enabled) return;

	const uint32_t start = x;
	const uint32_t width = 960;
	const uint32_t pitch = BMPPITCH;
	const uint32_t height = BMP_HEIGHT;

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
			//~ asm( "nop" );
			//~ asm( "nop" );
			//~ asm( "nop" );
			//~ asm( "nop" );
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
		dispcheck();
	written = 1;
	msleep(2000);
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
	const char *		filename,
	uint32_t 		compression // what compression to load the file into. 0: none, 1: RLE8
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
	size_t rc = read_file( filename, UNCACHEABLE(buf), size );
	if( rc != size )
		goto read_fail;

	*buf_size = size;

	return CACHEABLE(buf);

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
	BMP_LOCK( bmp_fill( 0x0, 0, 0, 960, BMP_HEIGHT ); )
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
	
	x0 = COERCE(x0, 0, 960 - (int)bmp->width);
	y0 = COERCE(y0, 0, BMP_HEIGHT - (int)bmp->height);
	if (x0 < 0) return;
	if (x0 + bmp->width > 960) return;
	if (y0 < 0) return;
	if (y0 + bmp->height > 960) return;
	
	int bmppitch = BMPPITCH;
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
#endif
/*
void bmp_draw_scaled(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax)
{
	if (!bmp) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;

	int bmppitch = BMPPITCH;
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
}*/

// this is slow, but is good for a small number of pixels :)
uint8_t bmp_getpixel(int x, int y)
{
	uint8_t * const bvram = bmp_vram();
	return bvram[x + y * BMPPITCH];
}
void bmp_putpixel(int x, int y, uint8_t color)
{
	//~ if (!bmp_enabled) return;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	int bmppitch = BMPPITCH;
	x = COERCE(x, 0, 960);
	y = COERCE(y, 0, BMP_HEIGHT);
	uint8_t * const b_row = bvram + y * bmppitch;
	b_row[x] = color;
}
void bmp_draw_rect(uint8_t color, int x0, int y0, int w, int h)
{
	//~ if (!bmp_enabled) return;
	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;
	
	int x, y;
	#define P(X,Y) bvram[COERCE(X, 0, 960) + COERCE(Y, 0, BMP_HEIGHT) * BMPPITCH]
	for (x = x0; x <= x0 + w; x++)
		P(x, y0) = P(x, y0+h) = color;
	for (y = y0; y <= y0 + h; y++)
		P(x0, y) = P(x0+w, y) = color;
	#undef P
}


void bmp_draw_scaled_ex(struct bmp_file_t * bmp, int x0, int y0, int xmax, int ymax, uint8_t* const mirror, int clear)
{
	if (!bmp) return;
	//~ if (!bmp_enabled) return;

	uint8_t * const bvram = bmp_vram();
	if (!bvram) return;

	int bmppitch = BMPPITCH;
	int x,y; // those sweep the original bmp
	int xs,ys; // those sweep the BMP VRAM (and are scaled)
	
	if (bmp->compression == 0) {
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
					uint8_t pix = bmp->image[ x + bmp->width * (bmp->height - y - 1) ];
					if (mirror)
					{
						uint8_t p = b_row[ xs ];
						uint8_t m = m_row[ xs ];
						if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue;
						if ((p == 0x14 || p == 0x3) && pix == 0) continue;
					}
					b_row[ xs ] = pix;
				}
			}
		}
	} else if (bmp->compression == 1) {
		uint8_t * bmp_line = bmp->image; // store the start of the line
		int bmp_y_pos = bmp->height-1; // store the line number
		for( ys = y0 + ymax - 1 ; ys >= y0; ys-- )
		{
			y = (ys-y0)*bmp->height/ymax;
			uint8_t * const b_row = bvram + COERCE(ys, 0, BMP_HEIGHT) * bmppitch;
			uint8_t * const m_row = (uint8_t*)( mirror + COERCE(y + y0, 0, BMP_HEIGHT) * bmppitch );
			while (y != bmp_y_pos) {
				// search for the next line
				if (bmp_line[0]!=0) { bmp_line += 2; } else
				if (bmp_line[1]==0) { bmp_line += 2; bmp_y_pos--; } else
				if (bmp_line[1]==1) return;
				if (y<0) return;
				if (bmp_line>(uint8_t*)(bmp+bmp->image_size)) return;
			}
			uint8_t* bmp_col = bmp_line; // store the actual position inside the bitmap
			int bmp_x_pos_start = 0; // store the start of the line
			int bmp_x_pos_end = bmp_col[0]; // store the end of the line
			uint8_t bmp_color = bmp_col[1]; // store the actual color to use
			for (xs = x0; xs < (x0 + xmax); xs++)
			{
				x = COERCE((xs-x0)*bmp->width/xmax, 0, 960);
				while (x>=bmp_x_pos_end) {
					// skip to this position
					if (bmp_col>(uint8_t*)(bmp+bmp->image_size)) break;
					if (bmp_col[0]==0) break;
					bmp_col+=2;
					if (bmp_col>(uint8_t*)(bmp+bmp->image_size)) break;
					if (bmp_col[0]==0) break;
					bmp_x_pos_start = bmp_x_pos_end;
					bmp_x_pos_end = bmp_x_pos_start + bmp_col[0];
					bmp_color = bmp_col[1];
				}
				if (clear)
				{
					uint8_t p = b_row[ xs ];
					if (bmp_color && p == bmp_color) b_row[xs] = 0;
				}
				else
				{
					if (mirror)
					{
						uint8_t p = b_row[ xs ];
						uint8_t m = m_row[ xs ];
						if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue;
						if ((p == 0x14 || p == 0x3) && bmp_color == 0) continue;
					}
					b_row[ xs ] = bmp_color;
				}
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
	
	uint16_t* chardata = (uint16_t*) bfnt_find_char(c);
	if (!chardata) return 0;
	uint8_t* buff = (uint8_t*)(chardata + 5);
	int ptr = 0;
	
	int cw  = chardata[0]; // the stored bitmap width
	int ch  = chardata[1]; // the stored bitmap height
	int crw = chardata[2]; // the displayed character width
	int xo  = chardata[3]; // X offset for displaying the bitmap
	int yo  = chardata[4]; // Y offset for displaying the bitmap
	int bb	= cw / 8 + (cw % 8 == 0 ? 0 : 1); // calculate the byte number per line
	
	//~ bmp_printf(FONT_SMALL, 0, 0, "%x %d %d %d %d %d %d", chardata, cw, ch, crw, xo, yo, bb);
	
	if (crw+xo > 100) return 0;
	if (ch+yo > 50) return 0;
	
	bmp_fill(bg, px, py, crw+xo+3, 40);
	
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
						bmp_putpixel(px+j*8+k+xo, py+i+yo, fg);
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
	unsigned int bb	= cw / 8 + (cw % 8 == 0 ? 0 : 1); // calculate the byte number per line
	
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

void bfnt_puts_utf8(int* s, int x, int y, int fg, int bg)
{
	while (*s)
	{
		x += bfnt_draw_char(*s, x, y, fg, bg);
		s++;
	}
}

#if CONFIG_DEBUGMSG
void
bfnt_test()
{
	while(1)
	{
		beep();
		kill_flicker();
		int* codes = (int*) BFNT_CHAR_CODES;
		static int c = 0;
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				bfnt_draw_char(codes[c], j*70, i*40, COLOR_WHITE, COLOR_BLACK);
				bmp_printf(FONT_SMALL, j*70, i*40, "%x", codes[c]);
				c++;
			}
		}
		while (!get_set_pressed()) msleep(100);
	}
}
#endif

void * bmp_lock = 0;

void bmp_init()
{
	bmp_lock = CreateRecursiveLock(0);
	bvram_mirror_init();
	update_vram_params();
}


INIT_FUNC(__FILE__, bmp_init);
