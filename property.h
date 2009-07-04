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

/** These are guesses */
#define PROP_MVR_MOVW_START0	0x80000020 // not sure?
#define PROP_MVR_MOVW_START1	0x80000021
#define PROP_MVR_REC		0x80030002
#define PROP_LV_0000		0x80050000
#define PROP_LV_0004		0x80050004
#define PROP_LV_STATE		0x8005000f // output very often
#define PROP_LV_DISPSIZE	0x80050015

/** Gui properties? 0xffc509b0 @ 0xDA */

/** Properties */
extern void
prop_register_slave(
	unsigned *	property_list,
	unsigned	count,
	void		(*prop_handler)(
		unsigned		property,
		void *			priv,
		void *			addr,
		unsigned		len
	),
	void *		priv,
	void		(*token_handler)( void * token )
);


extern void
prop_cleanup(
	void *		token,
	unsigned	property
);
#endif
