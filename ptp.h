#ifndef _ptp_h_
#define _ptp_h_

/** \file
 * PTP protocol and interface.
 *
 * The PTP protocol defines how the camera and a host computer communicate
 * over USB.  It defines operations and properties, and is callback driven.
 * Handlers for operations can be registered by calling ptp_register_handler()
 * and will be called when the host initiates that operation.
 */
#define PTP_FM_OBJECTSIZE	0x910a
#define PTP_SET_DEVICE_PROP	0x9110
#define PTP_HD_CAPACITY		0x911a
#define PTP_GUI_OFF		0x911b
#define PTP_LCD_ON		0x911c
#define PTP_911E		0x911e	// unknown
#define PTP_UPDATE_FIRMARE	0x911f
#define PTP_LV_DATA		0x9153
#define PTP_LV_ZOOM_MAYBE	0x9154
#define PTP_LV_ZOOM		0x9158
#define PTP_LV_AFFRAME		0x915a
#define PTP_AF_START		0x9160
#define PTP_FAPI_MESSAGE_TX	0x91fe


#define PTP_RC_OK		0x2001
#define PTP_RC_ERROR		0x2002

struct ptp_handle;

struct ptp_msg
{
	uint32_t		id;
	uint32_t		session;
	uint32_t		transaction;
	uint32_t		param_count;
	uint32_t		param[ 5 ];
} __PACKED__;

SIZE_CHECK_STRUCT( ptp_msg, 0x24 );

struct ptp_context
{
	struct ptp_handle *	handle;		// off_0x00;

	void *		off_0x04;

	// off 0x08
	void 		(*recv)(
		struct ptp_handle *	handle,
		void *			buf,
		size_t			len,
		void			(*callback)(
			void *			cb_priv,
			int			status
		),
		void *			cb_priv
	);

	// Sends a formatted buffer
	// \note format to be determined
	// off_0x0c
	int		(*send)(
		struct ptp_handle *	handle,
		struct ptp_msg *	msg
	);

	// Returns length of message to receive
	// off 0x10
	int		(*len)(
		struct ptp_handle *	handle
	);

	void * off_0x14;
	void * off_0x18; // priv to close handler?
	void * off_0x1c; // close?
};


extern void
ptp_register_handler(
	uint32_t		id,
	int			(*handler)(
		void *			priv,
		struct ptp_context *	context,
		void *			r2, // unknown
		void *			r3 // unknown
	),
	void *			priv
);


/* PTP handlers */
struct ptp_handler
{
	uint32_t		id;
	void *			handler;
	void *			priv;
};

#define PTP_HANDLER( ID, HANDLER, PRIV ) \
struct ptp_handler \
__attribute__((section(".ptp_handlers"))) \
__ptp_handler_##ID = { \
	.id			= ID, \
	.handler		= HANDLER, \
	.priv			= PRIV, \
}

#endif
