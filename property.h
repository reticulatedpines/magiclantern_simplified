/** \file
 * Properties and events.
 *
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _property_h_
#define _property_h_

/** These are known */
#define PROP_BURST_COUNT	0x80030006
#define PROP_BAT_INFO		0x8003001d
#define PROP_LENS_NAME		0x80030021
#define PROP_LENS_SOMETHING	0x80030022
#define PROP_HDMI_CHANGE	0x8003002c
#define PROP_HDMI_0002e		0x8003002e
#define PROP_MVR_REC_START	0x80030033 // 0 = no, 1 = stating, 2 = recording

#define PROP_REC_TIME		0x02010009 // output at 1 Hz while recording

/** These are good guesses */
#define PROP_GUI_STATE		0x80020000 // 0 == IDLE, 1 == PLAYMENU?, 3 == START_QR_MODE
#define PROP_LIVE_VIEW_FACE_AF	0x0205000A
#define PROP_LV_LOCK		0x80050021
#define PROP_LV_ACTION		0x80050022 // 0 == LV_START, 1 == LV_STOP

/** These are guesses */
#define PROP_USBDEVICE_CONNECT	0x8004000a
#define PROP_MVR_MOVW_START0	0x80000020 // not sure?
#define PROP_MVR_MOVW_START1	0x80000021
#define PROP_AF_MODE		0x80000004 // 3 == contrast
#define PROP_MVR_REC		0x80030002
#define PROP_LV_LENS		0x80050000
#define PROP_LV_0004		0x80050004
#define PROP_LV_FOCUS		0x80050001 // only works in liveview mode
#define PROP_LV_FOCUS_DONE	0x80050002 // output when focus motor is done?
#define PROP_LVAF_0003		0x80050003
#define PROP_LVAF_001D		0x8005001d
#define PROP_LV_STATE		0x8005000f // output very often
#define PROP_LV_DISPSIZE	0x80050015
#define PROP_LVCAF_STATE	0x8005001B // unknown meaning
#define PROP_HALF_SHUTTER	0x8005000a // two bytes, 1==held

#define PROP_APERTURE2		0x8000002d
#define PROP_APERTURE3		0x80000036

#define PROP_MODE		0x80000001 // maybe; set in FA_DISP_COM
#define PROP_SHUTTER		0x80000005
#define PROP_APERTURE		0x80000006
#define PROP_ISO		0x80000007
#define PROP_AE			0x80000008 // signed 8-bit value

#define PROP_SHUTTER_RELEASE	0x8003000A

/** These need to be found */
#define PROP_LCD_STATE		error_must_be_found

/** These are used by DL: 8002000C, 80030014, 100000E, 80020013 */
// These come form master result cbr
#define PROP_SENSOR		0x80020002
#define PROP_DEFAULT_CAMERA	0x80020003
#define PROP_DEFAULT_CFN	0x80020004
#define PROP_DEFALT_AF_SHIFT	0x80020005
#define PROP_DEFAULT_LV_MANIP	0x80020006
#define PROP_DISPSENSOR_CTRL	0x80020010	// 1 == show results?
#define PROP_LV_OUTPUT_DEVICE	0x80050011	// 1 == LCD?
#define PROP_HOUTPUT_TYPE	0x80030030
#define PROP_MIRROR_DOWN	0x8005001C
#define PROP_LV_EXPSIM		0x80050023
#define PROP_MYMENU_LISTING	0x80040009
#define PROP_SHOOTING_TYPE	0x80040004	// 0 == NORMAL, 3 == LV
#define PROP_ERR_BATTERY	0x80030020
#define PROP_CUSTOM_SETTING	0x80020007
#define PROP_DEFAULT_CUSTOM	0x80020008
#define PROP_DEFAULT_BRACKET	0x8002000A
#define PROP_PARTIAL_SETTING	0x8002000B
#define PROP_EMPOWER_OFF	0x80030007	// 1 == prohibit, 2 == permit


#define PROP_ACTIVE_SWEEP_STATUS 0x8002000C	// 1 == cleaning sensor?
#define PROP_DL_ACTION		0x80020013 // 0 == end?
#define PROP_EFIC_TEMP		0x80030014


/** Job progress
 * 0xB == capture end?
 * 0xA == start face catch pass?
 * 0x8 == "guiSetDarkBusy" -- noise reduction?
 * 0x0 == Job Done
 */
#define PROP_LAST_JOB_STATE	0x80030012	// 8 == done?

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


/** Change a property.
 *
 */
extern void
prop_request_change(
	unsigned	property,
	void *		addr,
	size_t		len
);


/** Get the current value of a property.
 *
 * \todo Does initial value of len matter?
 */
extern void
prop_get_value(
	unsigned	property,
	void *		addr,
	size_t *	len
);

#endif
