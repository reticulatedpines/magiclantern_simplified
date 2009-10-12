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

struct usb_handle;

struct usb_context
{
	struct usb_handle *	handle;		// off_0x00;

	void *		off_0x04;

	// off 0x08
	void 		(*recv)(
		struct usb_handle *	handle,
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
		struct usb_handle *	handle,
		void *			buf // id is first 16-bits
	);

	// Returns length of message to receive
	// off 0x10
	int		(*len)(
		struct usb_handle *	handle
	);
};


extern void
ptp_register_handler(
	uint32_t		id,
	void			(*handler)(
		void *			priv,
		struct usb_context *	context,
		void *			r2, // unknown
		void *			r3 // unknown
	),
	void *			priv
);


#endif
