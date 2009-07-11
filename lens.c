/** \file
 * Lens focus and zoom related things
 */

#include "dryos.h"
#include "lens.h"
#include "property.h"
#include "bmp.h"

struct lens_info lens_info = {
	.name		= "NO LENS NAME"
};

static unsigned lens_properties[] = {
	PROP_LENS_NAME,
	PROP_LV_LENS,
	PROP_APERTURE,
	PROP_SHUTTER,
	PROP_ISO,
};

static void
lens_handle_token(
	void *			token
)
{
	lens_info.token = token;
}


static inline uint16_t
bswap16(
	uint16_t		val
)
{
	return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}


static void
lens_handle_property(
	unsigned		property,
	void *			priv,
	char *			buf,
	unsigned		len
)
{
	switch( property )
	{
	case PROP_LENS_NAME:
		if( len > sizeof(lens_info.name) )
			len = sizeof(lens_info.name);
		memcpy( lens_info.name, buf, len );
		break;
	case PROP_APERTURE:
		lens_info.aperture = *(unsigned*) buf;
		break;
	case PROP_SHUTTER:
		lens_info.shutter = *(unsigned*) buf;
		break;
	case PROP_ISO:
		lens_info.iso = *(unsigned*) buf;
		break;
	case PROP_LV_LENS:
	{
		const struct prop_lv_lens * const lv_lens = (void*) buf;
		lens_info.focal_len	= bswap16( lv_lens->focal_len );
		lens_info.focus_dist	= bswap16( lv_lens->focus_dist );

		//bmp_hexdump( 300, 88, buf, len );
		break;
	}
	default:
		break;
	}

	// Needs to be 720 - 8 * 12
	unsigned x = 620;
	unsigned y = 0;
	bmp_printf( FONT_MED, x, y, "%5d mm", lens_info.focal_len );
	y += font_med.height;
	if( lens_info.focus_dist == 0xFFFF )
		bmp_printf( FONT_MED, x, y, "Infinity" );
	else
		bmp_printf( FONT_MED, x, y, "%5d cm", lens_info.focus_dist );
	y += font_med.height;
	bmp_printf( FONT_MED, x, y, "f/%x", lens_info.aperture );
	y += font_med.height;
	bmp_printf( FONT_MED, x, y, "%x shut", lens_info.shutter );
	y += font_med.height;
	bmp_printf( FONT_MED, x, y, "%x iso", lens_info.iso );

	prop_cleanup( lens_info.token, property );
}


static void
lens_init( void )
{
	prop_register_slave(
		lens_properties,
		sizeof(lens_properties)/sizeof(lens_properties[0]),
		lens_handle_property,
		0,
		lens_handle_token
	);
}

INIT_FUNC( "lens", lens_init );
