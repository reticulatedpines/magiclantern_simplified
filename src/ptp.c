#include "ptp.h"
#include "dryos.h"

int recv_ptp_data(struct ptp_context *data, char *buf, int size)
	// repeated calls per transaction are ok
{
	while ( size >= BUF_SIZE )
	{
		data->recv_data(data->handle,buf,BUF_SIZE,0,0);
		// XXX check for success??

		size -= BUF_SIZE;
		buf += BUF_SIZE;
	}
	if ( size != 0 )
	{
		data->recv_data(data->handle,buf,size,0,0);
		// XXX check for success??
	}

	return 1;
}

int send_ptp_data(struct ptp_context *data, const char *buf, int size)
	// repeated calls per transaction are *not* ok
{
	int tmpsize;

	tmpsize = size;
	while ( size >= BUF_SIZE )
	{
		if ( data->send_data(data->handle,(void *)buf,BUF_SIZE,tmpsize,0,0,0) )
		{
			return 0;
		}

		tmpsize = 0;
		size -= BUF_SIZE;
		buf += BUF_SIZE;
	}
	if ( size != 0 )
	{
		if ( data->send_data(data->handle,(void *)buf,size,tmpsize,0,0,0) )
		{
			return 0;
		}
	}

	return 1;
}

uint32_t ptp_register_all_handlers()
{
	uint32_t ret = 0x0;
	
	extern struct ptp_handler _ptp_handlers_start[];
	extern struct ptp_handler _ptp_handlers_end[];

	struct ptp_handler * handler = _ptp_handlers_start;

	for( ; handler < _ptp_handlers_end ; handler++ )
	{
#if defined(POSITION_INDEPENDENT)
        handler->handler = PIC_RESOLVE(handler->handler);
        handler->priv = PIC_RESOLVE(handler->priv);
#endif
		//DebugMsg("[ML] PTP_INIT reg: id=0x%08X h=0x%08X p=0x%08X", handler->id, handler->handler, handler->priv);		
		ret |= ptp_register_handler(
				handler->id,
				handler->handler,
				handler->priv
				);
	}

	return ret;
}

static void ptp_init( void *unused )
{
#ifndef CONFIG_40D 
	ptp_register_all_handlers();
#endif
}

INIT_FUNC( __FILE__, ptp_init );
