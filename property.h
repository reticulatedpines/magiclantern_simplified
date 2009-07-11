/** \file
 * Properties and events.
 *
 */
#ifndef _property_h_
#define _property_h_

/** These are known */
#define PROP_BAT_INFO		0x8003001d
#define PROP_LENS_NAME		0x80030021
#define PROP_LENS_SOMETHING	0x80030022
#define PROP_HDMI_CHANGE	0x8003002c
#define PROP_HDMI_0002e		0x8003002e
#define PROP_MVR_REC_START	0x80030033 // 0 = no, 1 = stating, 2 = recording

#define PROP_REC_TIME		0x02010009 // output at 1 Hz while recording

/** These are good guesses */
#define PROP_LIVE_VIEW_FACE_AF	0x0205000A
#define PROP_LV_LOCK		0x80050021

/** These are guesses */
#define PROP_MVR_MOVW_START0	0x80000020 // not sure?
#define PROP_MVR_MOVW_START1	0x80000021
#define PROP_MVR_REC		0x80030002
#define PROP_LV_LENS		0x80050000
#define PROP_LV_0004		0x80050004
#define PROP_LV_STATE		0x8005000f // output very often
#define PROP_LV_DISPSIZE	0x80050015

#define PROP_APERTURE		0x80000006
#define PROP_APERTURE2		0x8000002d
#define PROP_APERTURE3		0x80000036

#define PROP_SHUTTER		0x80000005
#define PROP_ISO		0x80000007

/** These need to be found */
#define PROP_LCD_STATE		error_must_be_found
#define PROP_SHOOTING_TYPE	error_must_be_found

/** These are used by DL: 8002000C, 80030014, 100000E, 80020013 */
#define PROP_ACTIVE_SWEEP_STATUS 0x8002000C
#define PROP_DL_ACTION		0x80020013
#define PROP_EFIC_TEMP		0x80030014

/** Gui properties? 0xffc509b0 @ 0xDA */

/** Properties */
extern void
prop_register_slave(
	unsigned *	property_list,
	unsigned	count,
	void *		(*prop_handler)(
		unsigned		property,
		void *			priv,
		void *			addr,
		unsigned		len
	),
	void *		priv,
	void		(*token_handler)( void * token )
);


extern void *
prop_cleanup(
	void *		token,
	unsigned	property
);


extern void
prop_request_change(
	unsigned	property,
	void *		addr,
	size_t		len
);

#endif
