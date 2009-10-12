/** \file
 * PTP handlers to extend Magic Lantern to the USB port.
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * a PTP client on the USB port.
 */

#include "dryos.h"
#include "ptp.h"
#include "tasks.h"


static void
ptp_handler_9999(
	void *			priv,
	struct usb_context *	context,
	void *			r2,
	void *			r3
)
{
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
