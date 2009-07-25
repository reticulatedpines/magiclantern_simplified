/** \file
 * Magic Lantern debugging and reverse engineering code
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"

void enable_full_hd( void * priv )
{
	DebugMsg( DM_MAGIC, 3, "Attempting to set HDMI to full HD" );

	thunk ChangeHDMIOutputSizeToFULLHD = (thunk) 0xFFA96260;
	void (*SetDisplayType)(int) = (void*) 0xFF8620DC;

	SetDisplayType( 3 );
	ChangeHDMIOutputSizeToFULLHD();

	DebugMsg( DM_MAGIC, 3, "Full HD done?" );
}

void call_dispcheck( void * priv )
{
	call( "dispcheck" );
}



static unsigned efic_temp;
static void * efic_temp_token;

static void
efic_temp_token_handler(
	void *			token
)
{
	efic_temp_token = token;
}


static void
efic_temp_property_handler(
	unsigned		property,
	void *			UNUSED( priv ),
	unsigned *		addr,
	unsigned		len
)
{
	efic_temp = *addr;
	prop_cleanup( efic_temp_token, property );
}


static void
efic_temp_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Sensor Temp %d",
		efic_temp
	);
}

static void
mvr_time_const_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	uint8_t * mvr_struct = (void*) 0x1ed4;
	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	struct state_object ** const mvr_state_object = (void*) 0x68a4;

	bmp_printf(
		FONT_MED, // selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"MVR %08x -> %08x %08x",
		(unsigned) *mvr_state_object,
		*(unsigned*)( 0x4c + (uintptr_t) mvr_state_object ),
		*(unsigned*)( 0x14c + (uintptr_t) mvr_state_object )
	);
}

static void
mvr_time_const_select( void * priv )
{
/*
	void (*mvr_set_time_const)(int *) = (void*) 0xff9716cc;
	void (*mvr_setd_fullhd)(int *) = (void*) 0xff9716cc;
	//int args[] = { 640, 480 };
	//DebugMsg( DM_MAGIC, 3, "calling mvr_setd_fullhd %d %d", args[0], args[1] );
	//mvr_setd_fullhd( args );

	uint32_t buf[] = { 8 };
	//prop_request_change( 0x207000c, buf, sizeof(buf) );
	void (*lv_magnify)( int * ) = (void*) 0xff83359c;
	lv_magnify( buf );
	void (*mvrSetBitRate)( int * ) = (void*) 0xff84f990;
	//int rate = 24;
	//mvrSetBitRate( &rate );
	mvr_struct->is_vga	= 0;
	mvr_struct->width	= 1920;
	mvr_struct->height	= 1080;
	mvr_struct->fps		= 24;
*/

	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	*(unsigned *)( mvr_hdr + 0x60 ) = 2400;
	*(unsigned *)( mvr_hdr + 0x64 ) = 100;
	*(unsigned *)( mvr_hdr + 0x68 ) = 24;
}



static int draw_prop = 0;

static void
draw_prop_select( void * priv )
{
	draw_prop = !draw_prop;
}


struct menu_entry debug_menus[] = {
	{
		.display	= efic_temp_display,
	},
	{
		.display	= mvr_time_const_display,
		.select		= mvr_time_const_select,
	},
	{
		.priv		= "Draw palette",
		.select		= bmp_draw_palette,
		.display	= menu_print,
	},
	{
		.priv		= "Toggle draw_prop",
		.select		= draw_prop_select,
		.display	= menu_print,
	},
	{
		.priv		= "Dump dmlog",
		.select		= (void*) dumpf,
		.display	= menu_print,
	},
	{
		.priv		= "Screenshot",
		.select		= call_dispcheck,
		.display	= menu_print,
	},
};


static void * debug_token;

static void
debug_token_handler(
	void *			token,
	void *			arg1,
	void *			arg2,
	void *			arg3
)
{
	debug_token = token;
	DebugMsg( DM_MAGIC, 3, "token %08x arg=%08x %08x %08x",
		(unsigned) token,
		(unsigned) arg1,
		(unsigned) arg2,
		(unsigned) arg3
	);
}

static void
debug_property_handler(
	unsigned		property,
	void *			UNUSED( priv ),
	unsigned *		addr,
	unsigned		len
)
{
	DebugMsg( DM_MAGIC, 3, "Prop %08x: %d: %08x %08x %08x %08x",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0
	);
		
	if( !draw_prop )
		goto ack;

	const unsigned x = 80;
	static unsigned y = 32;

	bmp_printf( FONT_SMALL, x, y,
		"%08x %04x: %08x %08x %08x %08x %08x %08x",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0,
		len > 0x10 ? addr[4] : 0,
		len > 0x14 ? addr[5] : 0
	);
	y += font_small.height;

	bmp_fill( COLOR_RED, x, y, 100, 1 );

	if( y > 400 )
		y = 32;

ack:
	prop_cleanup( debug_token, property );
	return;
}



#define num_properties 8192
unsigned property_list[ num_properties ];


void
debug_init( void )
{
	draw_prop = 0;

#if 1
	unsigned i, j, k;
	unsigned actual_num_properties = 0;
	//for( i=0 ; i<=0x8 ; i++ )
	i = 8;
	{
		for( j=0 ; j<=0x8 ; j++ )
		{
			for( k=0 ; k<0x40 ; k++ )
			{
				unsigned prop = 0
					| (i << 28) 
					| (j << 16)
					| (k <<  0);

				if( prop == 0x80030014
				||  prop == 0x80030015
				||  prop == 0x80050000
				||  prop == 0x80050004
				||  prop == 0x80050005
				||  prop == 0x80050010
				||  prop == 0x8005000f
				)
					continue;

				property_list[ actual_num_properties++ ] = prop;

				if( i !=0 && j != 0 )
				property_list[ actual_num_properties++ ] = 0
					| (i << 28) 
					| (j << 24)
					| (k <<  0);

				if( actual_num_properties > num_properties )
					goto thats_all;
			}
		}
	}

thats_all:
#else
	int actual_num_properties = 0;
	property_list[actual_num_properties++] = 0x44;
#endif

	prop_register_slave(
		property_list,
		actual_num_properties,
		debug_property_handler,
		0xdeadbeef,
		debug_token_handler
	);

	unsigned property = PROP_EFIC_TEMP;
	prop_register_slave(
		&property,
		1,
		efic_temp_property_handler,
		0xdeadbeef,
		efic_temp_token_handler
	);

	menu_add( "Debug", debug_menus, COUNT(debug_menus) );
}


static void
dump_task( void )
{
	// It was too early to turn these down in debug_init().
	// Only record important events for the display and face detect
	dm_set_store_level( DM_DISP, 4 );
	dm_set_store_level( DM_LVFD, 4 );
	dm_set_store_level( DM_LVCFG, 4 );
	dm_set_store_level( DM_LVCDEV, 4 );
	dm_set_store_level( DM_LV, 4 );
	dm_set_store_level( DM_RSC, 4 );
	dm_set_store_level( 0, 4 ); // catch all?

	// It was too early to read the draw_prop config in debug_init()
	draw_prop = config_int( global_config, "debug.draw-prop", 0 );

	int sec = config_int( global_config, "debug.timed-dump", 0 );
	if( sec == 0 )
		return;

	DebugMsg( DM_MAGIC, 3, "%s: Will do debug dump in %d sec",
		__func__,
		sec
	);

	msleep( sec * 1000 );

	DebugMsg( DM_MAGIC, 3, "%s: calling dumpf", __func__ );
	dumpf();
}


TASK_CREATE( "dump_task", dump_task, 0, 0x1f, 0x1000 );
