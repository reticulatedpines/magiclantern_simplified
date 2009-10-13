/** \file
 * PTP handlers to extend Magic Lantern to the USB port.
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * a PTP client on the USB port.
 */

#include "dryos.h"
#include "ptp.h"
#include "tasks.h"
#include "bmp.h"

static int
ptp_handler_9999(
	void *			priv,
	struct usb_context *	context,
	void *			r2,
	void *			r3
)
{
		struct ptp_msg msg = {
			.id		= 0x2002,
		};

	void * (*AllocateMemory)( size_t ) = (void*) 0xFF86DFE8;
	void (*FreeMemory)( void * ) = (void*) 0xFF86AF60;

	bmp_printf( FONT_LARGE, 0, 30, "usb %08x %08x", context, context->handle );

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

	context->send(
		context->handle,
		&msg
	);
	return 0;
}
	

static void
ptp_init( void )
{
	ptp_register_handler(
		0x9999,
		ptp_handler_9999,
		0
	);
}

INIT_FUNC( __FILE__, ptp_init );
