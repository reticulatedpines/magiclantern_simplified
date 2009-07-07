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
	bmp_printf( FONT_MED, 620,  0, "%5d mm", lens_info.focal_len );
	if( lens_info.focus_dist == 0xFFFF )
		bmp_printf( FONT_MED, 620, 12, "Infinity" );
	else
		bmp_printf( FONT_MED, 620, 12, "%5d cm", lens_info.focus_dist );

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
