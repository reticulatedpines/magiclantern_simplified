/** \file
 * Zebra stripes, contrast edge detection and crop marks.
 *
 */
#include "dryos.h"
#include "bmp.h"
#include "version.h"

/** Draw white thin crop marks
 *  And draw the 16:9 crop marks for full time
 * The screen is 480 vertical lines, but we only want to
 * show 720 * 9 / 16 == 405 of them.  If we use this number,
 * however, things don't line up right.
 *
 * \return 1 if a crop mark was drawn
 * \todo Determine the actual Cinemascope aspect ratio
 */
static int
draw_matte(
	unsigned		y,
	void *			bmp_row
)
{
	if( y != 50 && y != 380 )
		return 0;

	const uint32_t		width	= 720;
	uint32_t *		row	= bmp_row;
	unsigned		x;

	for( x = 0; x<width/4 ; x++ )
		*(row++) = color_word( WHITE_COLOR );

	return 1;
}


static void
draw_zebra( void )
{
	struct vram_info * vram = &vram_info[ vram_get_number(2) ];

	uint32_t x,y;

	const uint8_t zebra_color_0 = 0x6F; // bright read
	const uint8_t zebra_color_1 = 0x5F; // dark red

	// For unused contrast detection algorithm
	//const uint8_t contrast_color = 0x0D; // blue
	//const uint16_t threshold = 0xF000;

	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	for( y=33 ; y < 390; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bmp_vram() + y * bmp_pitch() );

		// Check for crop marks
		if( draw_matte(y, b_row) )
			continue;

		for( x=0 ; x < vram->width ; x+=2 )
		{
			uint32_t pixels = v_row[x/2];
#if 0
			uint16_t pixel0 = (pixels >> 16) & 0xFFFF;
			uint16_t pixel1 = (pixels >>  0) & 0xFFFF;

			// Check for contrast
			// This doesn't work very well, so I have it
			// compiled out for now.
			if( (pixel0 > pixel1 && pixel0 - pixel1 > 0x4000 )
			||  (pixel0 < pixel1 && pixel1 - pixel0 > 0x4000 )
			)
			{
				b_row[x/2] = (contrast_color << 8) | contrast_color;
				continue;
			}
#endif

			// If neither pixel is overexposed, ignore it
			if( (pixels & 0xF000F000) != 0xF000F000 )
			{
				b_row[x/2] = 0;
				continue;
			}

			// Determine if we are a zig or a zag line
			uint32_t zag = ((y >> 3) ^ (x >> 3)) & 1;

			// Build the 16-bit word to write both pixels
			// simultaneously into the BMP VRAM
			uint16_t zebra_color_word = zag
				? (zebra_color_0<<8) | (zebra_color_0<<0)
				: (zebra_color_1<<8) | (zebra_color_1<<0);

			b_row[x/2] = zebra_color_word;
		}
	}
}




int
zebra_task( void )
{
	msleep( 5000 );

	while(1)
	{
		if( !gui_show_menu )
		{
			draw_zebra();
			msleep( 100 );
		} else {
			// Don't display the zebras over the menu.
			// wait a while and then try again
			msleep( 500 );
		}
	}

	return 1;
}


TASK_CREATE( "zebra_task", zebra_task, 0, 0x1f, 0x1000 );
