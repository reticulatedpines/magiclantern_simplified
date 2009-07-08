/** \file
 * Zebra stripes, contrast edge detection and crop marks.
 *
 */
#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"


unsigned zebra_level = 0xF000;
unsigned zebra_draw = 1;
unsigned crop_draw = 1;
unsigned edge_draw = 0;


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
	if( !crop_draw )
		return 0;

	if( y != 55 && y != 375 )
		return 0;

	const uint32_t		width	= 720;
	uint32_t *		row	= bmp_row;
	unsigned		x;

	for( x = 0; x<width/4 ; x++ )
		*(row++) = color_word( COLOR_WHITE );

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
	uint8_t * const bvram = bmp_vram();
	if( !bvram )
		return;

	struct vram_info * vram = &vram_info[ vram_get_number(2) ];

	uint32_t x,y;

	const uint8_t zebra_color_0 = COLOR_BG; // 0x6F; // bright read
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

		if( !zebra_draw )
			continue;

		for( x=1 ; x < vram->width-1 ; x++ )
		{

			if( edge_draw )
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


void zebra_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = (*ptr + 0x4000) & 0xF000;
}

void zebra_display( void * priv, int x, int y, int selected )
{
	bmp_printf( MENU_FONT, x, y, "%sZebra level: %04x",
		selected ? "->" : "  ",
		*(unsigned*) priv
	);
}

void zebra_draw_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = !*ptr;
}

void zebra_draw_display( void * priv, int x, int y, int selected )
{
	bmp_printf( MENU_FONT, x, y, "%sZebras %s",
		selected ? "->" : "  ",
		*(unsigned*) priv ? " on" : "off"
	);
}

void crop_toggle( void * priv )
{
	unsigned * ptr = priv;
	*ptr = !*ptr;
}

void crop_display( void * priv, int x, int y, int selected )
{
	bmp_printf( MENU_FONT, x, y, "%sCrop %s",
		selected ? "->" : "  ",
		*(unsigned*) priv ? " on" : "off"
	);
}

struct menu_entry zebra_menus[] = {
	{
		.priv		= &zebra_draw,
		.select		= zebra_draw_toggle,
		.display	= zebra_draw_display,
	},
	{
		.priv		= &zebra_level,
		.select		= zebra_toggle,
		.display	= zebra_display,
	},
	{
		.priv		= &crop_draw,
		.select		= crop_toggle,
		.display	= crop_display,
	},
};

int
zebra_task( void )
{
	msleep( 3000 );

	zebra_draw = config_int( global_config, "zebra.draw", 1 );
	zebra_level = config_int( global_config, "zebra.level", 0xF000 );
	crop_draw = config_int( global_config, "crop.draw", 1 );
	edge_draw = config_int( global_config, "edge.draw", 1 );

	menu_add( &main_menu, zebra_menus, COUNT(zebra_menus) );

	DebugMsg( DM_MAGIC, 3, "Zebras %s, threshold %x",
		zebra_draw ? "on" : "off",
		zebra_level
	);

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
