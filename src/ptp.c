/** \file
 * PTP handlers to extend Magic Lantern to the USB port.
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * a PTP client on the USB port.
 */

#include "dryos.h"
#include "ptp.h"
#include "ptp-chdk.h"
#include "tasks.h"
#include "menu.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"


PTP_HANDLER( PTP_OC_CHDK, 0 )
{
	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
		.param_count	= 4,
		.param		= { 1, 2, 0xdeadbeef, 3 },
	};
	/*
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
	int len = context->get_data_size( context->handle );
	bmp_printf( FONT_LARGE, 0, 50, "Len = %d", len );
	if( !len )
	{
		context->send_resp(
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

    */
    
  bmp_printf(FONT_LARGE, 0, 0, "PTP: %8x %8x %8x", param1, param2, param3);

  // ported from CHDK

  static int temp_data_kind = 0; // 0: nothing, 1: ascii string, 2: lua object
  static int temp_data_extra; // size (ascii string) or type (lua object)

  static union {
    char *str;
  } temp_data;

  // handle command
  switch ( param1 )
  {

    case PTP_CHDK_Version:
      msg.param_count = 2;
      msg.param[0] = PTP_CHDK_VERSION_MAJOR;
      msg.param[1] = PTP_CHDK_VERSION_MINOR;
      gui_unlock();
      break;

    case PTP_CHDK_GetMemory:
      if ( param2 == 0 || param3 < 1 ) // null pointer or invalid size?
      {
        msg.id = PTP_RC_GeneralError;
        break;
      }

      if ( !send_ptp_data(context, (char *) param2, param3) )
      {
        msg.id = PTP_RC_GeneralError;
      }
      break;
      
    case PTP_CHDK_SetMemory:
      /*
      if ( param2 == 0 || param3 < 1 ) // null pointer or invalid size?
      {
        msg.id = PTP_RC_GeneralError;
        break;
      }

      data->get_data_size(data->handle); // XXX required call before receiving
      if ( !recv_ptp_data(data,(char *) param2,param3) )
      {
        msg.id = PTP_RC_GeneralError;
      } */
      
      bmp_printf(FONT_LARGE, 0, 0, "SetMemory: not implemented");
	  msleep(1000);
      break;

    case PTP_CHDK_CallFunction:
      bmp_printf(FONT_LARGE, 0, 0, "CallFunction: not implemented");
	  msleep(1000);
      break;

    case PTP_CHDK_TempData:
      if ( param2 & PTP_CHDK_TD_DOWNLOAD )
      {
        const char *s;
        size_t l;

        if ( temp_data_kind == 0 )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }

        if ( temp_data_kind == 1 )
        {
          s = temp_data.str;
          l = temp_data_extra;
        } else { // temp_data_kind == 2
		  s = 0;
          //~ s = lua_tolstring(get_lua_thread(temp_data.lua_state),1,&l);
        }

        if ( !send_ptp_data(context,s,l) )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }
        
      } else if ( ! (param2 & PTP_CHDK_TD_CLEAR) ) {
        if ( temp_data_kind == 1 )
        {
          FreeMemory(temp_data.str);
        } else if ( temp_data_kind == 2 )
        {
          //~ lua_close(temp_data.lua_state);
        }
        temp_data_kind = 0;

        temp_data_extra = context->get_data_size(context->handle);

        temp_data.str = (char *) AllocateMemory(temp_data_extra);
        if ( temp_data.str == NULL )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }

        if ( !recv_ptp_data(context,temp_data.str,temp_data_extra) )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }
        temp_data_kind = 1;
      }
      if ( param2 & PTP_CHDK_TD_CLEAR )
      {
        if ( temp_data_kind == 1 )
        {
          FreeMemory(temp_data.str);
        } else if ( temp_data_kind == 2 )
        {
          //~ lua_close(temp_data.lua_state);
        }
        temp_data_kind = 0;
      }
      break;

    case PTP_CHDK_UploadFile:
	{
        FILE *f;
        int s,r,fn_len;
        char *buf, *fn;

        s = context->get_data_size(context->handle);

        recv_ptp_data(context,(char *) &fn_len,4);
        s -= 4;

        fn = (char *) AllocateMemory(fn_len+1);
        if ( fn == NULL )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }
        fn[fn_len] = '\0';

        recv_ptp_data(context,fn,fn_len);
        s -= fn_len;
		
		bmp_printf(FONT_LARGE, 0, 0, "UL '%s' %db", fn, s);

		FIO_RemoveFile(fn);
        f = FIO_CreateFile(fn);
        if ( f == NULL )
        {
          msg.id = PTP_RC_GeneralError;
          FreeMemory(fn);
          break;
        }
        FreeMemory(fn);

        buf = (char *) AllocateMemory(BUF_SIZE);
        if ( buf == NULL )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }
        while ( s > 0 )
        {
          if ( s >= BUF_SIZE )
          {
            recv_ptp_data(context,buf,BUF_SIZE);
            FIO_WriteFile(f, UNCACHEABLE(buf), BUF_SIZE);
            s -= BUF_SIZE;
          } else {
            recv_ptp_data(context,buf,s);
            FIO_WriteFile(f, UNCACHEABLE(buf), s);
            s = 0;
          }
        }

        FIO_CloseFile(f);

        FreeMemory(buf);
        break;
	}
      
    case PTP_CHDK_DownloadFile:
	{
        FILE *f;
        int tmp,t,s,r,fn_len;

		bmp_printf(FONT_LARGE, 0, 0, "DL request");

        if ( temp_data_kind != 1 )
        {
          bmp_printf(FONT_LARGE, 0, 0, "DL kind err %d", temp_data_kind);
          msg.id = PTP_RC_GeneralError;
          break;
        }

        char fn[101];
		if (temp_data_extra > 100)
        {
          bmp_printf(FONT_LARGE, 0, 0, "DL extra err %d", temp_data_extra);
          msg.id = PTP_RC_GeneralError;
          break;
        }
		
        memcpy(fn,temp_data.str,temp_data_extra);
        fn[temp_data_extra] = '\0';

        FreeMemory(temp_data.str);
        temp_data_kind = 0;

        if( FIO_GetFileSize( fn, &s ) != 0 )
        {
          bmp_printf(FONT_LARGE, 0, 0, "DL '%s' size err", fn);
          msg.id = PTP_RC_GeneralError;
          break;
        }

		bmp_printf(FONT_LARGE, 0, 0, "DL '%s' %db", fn, s);

        f = FIO_Open(fn, 0);
        if ( f == NULL )
        {
          msg.id = PTP_RC_GeneralError;
          break;
        }

        char buf[BUF_SIZE+32];
        tmp = s;
        t = s;
        while ( (r = FIO_ReadFile(f, UNCACHEABLE(buf), (t<BUF_SIZE) ? t : BUF_SIZE)) > 0 )
        {
          t -= r;
          // cannot use send_ptp_data here
          context->send_data(context->handle,buf,r,tmp,0,0,0);
          tmp = 0;
        }
        FIO_CloseFile(f);
        // XXX check that we actually read/send s bytes! (t == 0)

        msg.param_count = 1;
        msg.param[0] = s;

        break;
	}
	break;

    case PTP_CHDK_ExecuteScript:
      bmp_printf(FONT_LARGE, 0, 0, "ExecuteScript: not implemented");
	  msleep(1000);
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


/** Start recording when we get a PTP operation 0x9997
 * MovieStop doesn't seem to do anything, but MovieStart
 * toggles recording on and off
 */
/*PTP_HANDLER( 0x9997, 0 )
{
	call( "MovieStart" );

	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
		.param_count	= 1,
		.param		= { param1 },
	};

	context->send_resp(
		context->handle,
		&msg
	);

	return 0;
}
*/

/** Dump memory */
/*PTP_HANDLER( 0x9996, 0 )
{
	const uint32_t * const buf = (void*) param1;

	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
		.param_count	= 5,
		.param		= {
			buf[0],
			buf[1],
			buf[2],
			buf[3],
			buf[4],
		},
	};

	context->send_resp(
		context->handle,
		&msg
	);

	return 0;
}*/

/** Write to memory, returning the old value */
/*PTP_HANDLER( 0x9995, 0 )
{
	uint32_t * const buf = (void*) param1;
	const uint32_t val = (void*) param2;

	const uint32_t old = *buf;
	*buf = val;

	struct ptp_msg msg = {
		.id		= PTP_RC_OK,
		.session	= session,
		.transaction	= transaction,
		.param_count	= 2,
		.param		= {
			param1,
			old,
		},
	};

	context->send_resp(
		context->handle,
		&msg
	);

	return 0;
}*/

/*
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
};*/


static void
ptp_init( void )
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
