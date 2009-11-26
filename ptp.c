/** \file
 * PTP handlers to extend Magic Lantern to the USB port.
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * a PTP client on the USB port.
 */

#include "dryos.h"
#include "ptp.h"
#include "tasks.h"
#include "menu.h"
#include "bmp.h"
#include "hotplug.h"
#include "property.h"

static int
ptp_handler_9999(
	void *			priv,
	struct ptp_context *	context,
	uint32_t		opcode,
	uint32_t		session,
	uint32_t		transaction,
	uint32_t		param1,
	uint32_t		param2,
	uint32_t		param3,
	uint32_t		param4,
	uint32_t		param5
)
{
	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
		.param_count	= 4,
		.param		= { 1, 2, 0xdeadbeef, 3 },
	};

	//call( "FA_StartLiveView" );
	bmp_printf( FONT_MED, 0, 30, "usb %08x %08x", context, context->handle );
	bmp_printf( FONT_MED, 0, 50, "%08x %08x %08x %08x %08x",
		(unsigned) param1,
		(unsigned) param2,
		(unsigned) param3,
		(unsigned) param4,
		(unsigned) param5
	);

#if 0
	int len = context->len( context->handle );
	bmp_printf( FONT_LARGE, 0, 50, "Len = %d", len );
	if( !len )
	{
		context->send(
			context->handle,
			&msg
		);

		return 0;
	}

	void * buf = AllocateMemory( len );
	if( !buf )
		return 1;

	bmp_printf( FONT_LARGE, 0, 60, "buf = %08x", buf );
	context->recv(
		context->handle,
		buf,
		len,
		0,
		0
	);


	bmp_hexdump( FONT_LARGE, 0, 50, buf, len );
	FreeMemory( buf );
#endif

	context->send(
		context->handle,
		&msg
	);

	// Try to disable the USB lock
	gui_unlock();

	return 0;
}



static int
ptp_handler_9998(
	void *			priv,
	struct ptp_context *	context,
	uint32_t		opcode,
	uint32_t		session,
	uint32_t		transaction,
	uint32_t		param1,
	uint32_t		param2,
	uint32_t		param3,
	uint32_t		param4,
	uint32_t		param5
)
{
	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
	};

	lens_focus( 0x7, -1000 );

	context->send(
		context->handle,
		&msg
	);

	return 0;
}


static void
ptp_state_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"PTP State:  %x %08x",
		hotplug_struct.usb_state,
		*(uint32_t*)( 0xC0220000 + 0x34 )
	);
}


static void
ptp_state_toggle( void * priv )
{
	hotplug_struct.usb_state = !hotplug_struct.usb_state;
	prop_deliver(
		hotplug_struct.usb_prop,
		&hotplug_usb_buf,
		sizeof(hotplug_usb_buf),
		0
	);
}

static struct menu_entry ptp_menus[] = {
	{
		.display	= ptp_state_display,
		.select		= ptp_state_toggle,
	},
};


static void
ptp_init( void )
{
	ptp_register_handler(
		0x9999,
		ptp_handler_9999,
		0
	);

	ptp_register_handler(
		0x9998,
		ptp_handler_9998,
		0
	);

	menu_add( "PTP", ptp_menus, COUNT(ptp_menus) );
}

INIT_FUNC( __FILE__, ptp_init );
