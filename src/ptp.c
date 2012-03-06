#include "ptp.h"

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
}

INIT_FUNC( __FILE__, ptp_init );
