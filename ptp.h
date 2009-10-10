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

struct usb_context
{
	void *		priv;		// off_0x00;

	void *		off_0x04;

	void 		(*recv)(
		void *			priv,
		void *			buf,
		size_t			len,
		void			(*callback)(
			void *			cb_priv,
			int			status
		),
		void *			cb_priv
	);

	int		(*send)(
		void *			priv,
		void *			buf // id is first 16-bits
	);

	// Returns length of message to receive
	int		(*len)( void * priv );	// off_0x10
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

