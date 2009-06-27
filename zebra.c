/** \file
 * Zebra stripes, contrast edge detection and crop marks.
 *
 */
#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"

extern unsigned zebra_level;
extern unsigned zebra_draw;

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

static int32_t convolve_x[3][3] = {
	{ -1, 0, +1, },
	{ -2, 0, +2, },
	{ -1, 0, +1, },
};

static int32_t convolve_y[3][3] = {
	{ +1, +2, +1, },
	{  0,  0,  0, },
	{ -1, -2, -1, },
};


static int32_t
convolve(
	int32_t c[3][3],
	uint16_t * buf,
	uint32_t pitch
)
{
	int32_t value = 0;
	value += c[0][0] * buf[ -pitch - 1 ];
	value += c[0][1] * buf[ -pitch ];
	value += c[0][2] * buf[ -pitch + 1 ];

	value += c[1][0] * buf[ -1 ];
	value += c[1][1] * buf[ 0 ];
	value += c[1][2] * buf[ +1 ];

	value += c[2][0] * buf[ +pitch - 1 ];
	value += c[2][1] * buf[ +pitch ];
	value += c[2][2] * buf[ +pitch + 1 ];

	return value;
}


int32_t
edge_detect(
	uint16_t *		buf,
	uint32_t		pitch
)
{
	int32_t gx = convolve( convolve_x, buf, pitch );
	int32_t gy = convolve( convolve_y, buf, pitch );

	return gx*gx + gy*gy;
}



static void
draw_zebra( void )
{
	const int draw_edge_detect = 0;

	uint8_t * const bvram = bmp_vram();
	if( !bvram )
		return;

	struct vram_info * vram = &vram_info[ vram_get_number(2) ];

	uint32_t x,y;

	const uint8_t zebra_color_0 = BG_COLOR; // 0x6F; // bright read
	const uint8_t zebra_color_1 = 0x5F; // dark red

	// For unused contrast detection algorithm
	//const uint8_t contrast_color = 0x0D; // blue
	const uint8_t contrast_color = 0x0E; // pink
	//const uint16_t threshold = 0xF000;

	const int32_t edge_level = zebra_level * zebra_level;

	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	for( y=33 ; y < 390; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bvram + y * bmp_pitch() );

		// Check for crop marks
		if( draw_matte(y, b_row) )
			continue;

		for( x=1 ; x < vram->width-1 ; x++ )
		{

			if( draw_edge_detect )
			{
				// Check for contrast
				int32_t grad = edge_detect(
					(uint16_t*) &v_row[x/2],
					vram->pitch
				);
			
				if( grad < edge_level )
				{
					b_row[x/2] = 0
						| (contrast_color << 8)
						| contrast_color;

					continue;
				}
			}

			uint32_t pixel = v_row[x/2];
			int32_t p0 = (pixel >> 16) & 0xFFFF;
			int32_t p1 = (pixel >>  0) & 0xFFFF;

			// If neither pixel is overexposed, ignore it
			if( p0 < zebra_level && p1 < zebra_level )
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

	const char * zebra_draw_str = config_value( global_config, "zebra.draw" );
	if( zebra_draw_str )
		zebra_draw = atoi( zebra_draw_str );
	const char * zebra_level_str = config_value( global_config, "zebra.level" );
	if( zebra_level_str )
		zebra_level = atoi( zebra_level_str );

	DebugMsg( DM_MAGIC, 3, "Zebras %s, threshold %x",
		zebra_draw ? "on" : "off",
		zebra_level
	);

	while(1)
	{
		if( !gui_show_menu && zebra_draw )
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
