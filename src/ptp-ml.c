/** \file
 * PTP handlers to extend Magic Lantern to the USB port.
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * a PTP client on the USB port.
 */

#include "dryos.h"
#include "ptp.h"
#include "ptp-ml.h"
#include "tasks.h"
#include "menu.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"


PTP_HANDLER( PTP_ML_CHDK, 0 )
{
    struct ptp_msg msg = {
        .id        = PTP_RC_OK,
        .session    = session,
        .transaction    = transaction,
        .param_count    = 4,
        .param        = { 1, 2, 0xdeadbeef, 3 },
    };

    bmp_printf(FONT_LARGE, 0, 0, "PTP: %8x %8x %8x", (unsigned int) param1, (unsigned int) param2, (unsigned int) param3);

  // handle command
  switch ( param1 )
  {

    case PTP_ML_USB_Version:
      msg.param_count = 2;
      msg.param[0] = PTP_ML_VERSION_MAJOR;
      msg.param[1] = PTP_ML_VERSION_MINOR;
      break;

    default:
      msg.id = PTP_RC_ParameterNotSupported;
      break;
  }

   context->send_resp(
     context->handle,
     &msg
   );

   return 0;
}

static void
ptp_init( void *unused )
{
    extern struct ptp_handler _ptp_handlers_start[];
    extern struct ptp_handler _ptp_handlers_end[];
    struct ptp_handler * handler = _ptp_handlers_start;

    for( ; handler < _ptp_handlers_end ; handler++ )
    {
        ptp_register_handler(
            handler->id,
            handler->handler,
            handler->priv
        );
    }

    //~ menu_add( "Debug", ptp_menus, COUNT(ptp_menus) );
}

INIT_FUNC( __FILE__, ptp_init );
