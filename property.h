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
#define PROP_TFT_STATUS		0x80030015
#define PROP_LENS_NAME		0x80030021
#define PROP_LENS_SOMETHING	0x80030022
#define PROP_HDMI_CHANGE	0x8003002c
#define PROP_HDMI_CHANGE_CODE	0x8003002e // edidc?
#define PROP_MVR_REC_START	0x80030033 // 0 = no, 1 = stating, 2 = recording


/** 0x02010000 - 0x02010014 == card state? */
#define PROP_REC_TIME		0x80030005 // time remaining.  output at 1 Hz while recording

/** These are good guesses */
#define PROP_GUI_STATE		0x80020000 // 0 == IDLE, 1 == PLAYMENU?, 2 = menu->disp, 3 == START_QR_MODE, 9 = Q menu
#define PROP_LIVE_VIEW_FACE_AF	0x0205000A
#define PROP_LV_LOCK		0x80050021
#define PROP_LV_ACTION		0x80050022 // 0 == LV_START, 1 == LV_STOP

/** These are guesses */
#define PROP_USBDEVICE_CONNECT	0x8004000a
#define PROP_MVR_MOVW_START0	0x80000020 // not sure?
#define PROP_MVR_MOVW_START1	0x80000021
#define PROP_AF_MODE		0x80000004 // 0 = one shot, 3 == manual focus, 202 = ai (dumb) focus, 101 = ai servo (slightly better)
#define PROP_MVR_REC		0x80030002
#define PROP_LV_LENS		0x80050000
#define PROP_LV_0004		0x80050004
#define PROP_LV_MANIPULATION	0x80050006
#define PROP_LV_AFFRAME		0x80050007 // called by ptp handler 915a
#define PROP_LV_FOCUS		0x80050001 // only works in liveview mode
#define PROP_LV_FOCUS_DONE	0x80050002 // output when focus motor is done?
#define PROP_LVAF_0003		0x80050003
#define PROP_LVAF_001D		0x8005001d
#define PROP_LV_STATE		0x8005000f // output very often
#define PROP_LV_DISPSIZE	0x80050015 // used to control LV zoom (1 / 5 / A)
#define PROP_LVCAF_STATE	0x8005001B // unknown meaning
#define PROP_HALF_SHUTTER	0x8005000a // two bytes, 1==held; only updated in LV mode
#define PROP_ORIENTATION	0x8005000d // 0 == 0 deg, 1 == +90 deg, 2 == -90 deg

#define PROP_APERTURE2		0x8000002d
#define PROP_APERTURE3		0x80000036
#define PROP_LIVE_VIEW_VIEWTYPE	0x80000034

#define PROP_MODE		0x80000001 // maybe; set in FA_DISP_COM

#define PROP_DRIVE		0x80000003
#define DRIVE_SINGLE 0
#define DRIVE_CONTINUOUS 1
#define DRIVE_SELFTIMER_REMOTE 0x10
#define DRIVE_SELFTIMER_2SEC 0x11
#define DRIVE_SELFTIMER_CONTINUOUS 7

#define PROP_SHUTTER		0x80000005
#define PROP_APERTURE		0x80000006
#define PROP_ISO		0x80000007
#define PROP_AE			0x80000008 // signed 8-bit value
#define PROP_UILOCK		0x8000000b // maybe?
#define PROP_ISO_AUTO		0x8000002E // computed by AUTO ISO if PROP_ISO is 0; otherwise, equal to PROP_ISO

#define PROP_SHUTTER_RELEASE	0x8003000A
#define PROP_AVAIL_SHOT		0x8004000C // also 0x80030005

#define PROP_MIC_INSERTED	0x8003002b

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
#define PROP_HOUTPUT_TYPE	0x80030030      // 0 = no info displayed in LV, 1 = info displayed (this is toggled with DISP)
#define PROP_MIRROR_DOWN	0x8005001C
#define PROP_LV_EXPSIM		0x80050023
#define PROP_MYMENU_LISTING	0x80040009
#define PROP_LV_MODE		0x8004001C	// 0=DisableMovie, 1=? or 2=EnableMovie
#define PROP_SHOOTING_TYPE	0x80040004	// 0 == NORMAL, 3 == LV, 4 == MOVIE
#define PROP_ERR_BATTERY	0x80030020
#define PROP_CUSTOM_SETTING	0x80020007
#define PROP_DEFAULT_CUSTOM	0x80020008
#define PROP_DEFAULT_BRACKET	0x8002000A
#define PROP_PARTIAL_SETTING	0x8002000B
#define PROP_EMPOWER_OFF	0x80030007	// 1 == prohibit, 2 == permit
#define PROP_LVAF_MODE      0x8004001d // 0 = shutter killer, 1 = live mode, 2 = face detect

#define PROP_ACTIVE_SWEEP_STATUS 0x8002000C	// 1 == cleaning sensor?
#define PROP_DL_ACTION		0x80020013 // 0 == end?
#define PROP_EFIC_TEMP		0x80030014

#define PROP_ARTIST_STRING	0x0E070000
#define PROP_COPYRIGHT_STRING	0x0E070001

#define PROP_LANGUAGE		0x02040002
#define PROP_VIDEO_SYSTEM	0x02040003

#define PROP_ICU_UILOCK		0x8300017F	// maybe?

#define PROP_SHOOTING_MODE  0x80000000 // During mode switch, it takes other values.
#define SHOOTMODE_P 0
#define SHOOTMODE_TV 1
#define SHOOTMODE_AV 2
#define SHOOTMODE_M 3
#define SHOOTMODE_ADEP 5
#define SHOOTMODE_CA 13
#define SHOOTMODE_AUTO 9
#define SHOOTMODE_NOFLASH 0xF
#define SHOOTMODE_PORTRAIT 0xC
#define SHOOTMODE_LANDSCAPE 0xD
#define SHOOTMODE_MACRO 0xE
#define SHOOTMODE_SPORTS 0xB
#define SHOOTMODE_NIGHT 0xA
#define SHOOTMODE_MOVIE 0x14

// WB in LiveView (and movie) mode
#define PROP_WB_MODE_LV        0x80050018  // 0=AWB, 1=sun, 8=shade, 2=clouds, 3=tungsten, 4=fluorescent, 5=flash, 6=custom, 9 = kelvin
#define PROP_WB_KELVIN_LV      0x80050019

// WB in photo mode
#define PROP_WB_MODE_PH 0x8000000D
#define PROP_WB_KELVIN_PH 0x8000000E

// values of PROP_WB_MODE_*:
#define WB_AUTO 0
#define WB_SUNNY 1 
#define WB_SHADE 8
#define WB_CLOUDY 2
#define WB_TUNGSTEN 3
#define WB_FLUORESCENT 4
#define WB_FLASH 5
#define WB_CUSTOM 6
#define WB_KELVIN 9

#define PROP_WBS_GM 0x80000010 // signed 8-bit, len=4
#define PROP_WBS_BA 0x80000011 // idem


#define PROP_SHUTTER_COUNT     0x02050001 // maybe?
#define PROP_FILE_NUMBER       0x02040008   // if last saved file is IMG_1234, then this property is 1234. Works both in photo and video mode.
#define PROP_FILE_NUMBER_ALSO  0x02010004 // seems to mirror the previous one, but it's increased earlier
#define PROP_FOLDER_NUMBER     0x02010001 // 100, 101...

#define PROP_PICTURE_STYLE 0x80000028 // 0x81 = std, 82 = portrait, 83 = landscape, 84 = neutral, 85 = faithful, 86 = monochrome, 21 = user 1, 22 = user 2, 23 = user 3
#define PROP_PICSTYLE_SETTINGS_1 0x02060001 // 02060001 for std, 02060002 for portrait... 02060007 for user 1 ... 02060009 for user 3
#define PROP_PICSTYLE_SETTINGS_2 0x02060002
#define PROP_PICSTYLE_SETTINGS_3 0x02060003
#define PROP_PICSTYLE_SETTINGS_4 0x02060004
#define PROP_PICSTYLE_SETTINGS_5 0x02060005
#define PROP_PICSTYLE_SETTINGS_6 0x02060006
#define PROP_PICSTYLE_SETTINGS_7 0x02060007
#define PROP_PICSTYLE_SETTINGS_8 0x02060008
#define PROP_PICSTYLE_SETTINGS_9 0x02060009

#define PROP_ALO 0x8000003D
#define ALO_STD 0
#define ALO_LOW 1
#define ALO_HIGH 2
#define ALO_OFF 3

#define PROP_CFN 0x80010004 // 13 bytes

/** Job progress
 * 0xB == capture end?
 * 0xA == start face catch pass?
 * 0x8 == "guiSetDarkBusy" -- noise reduction?
 * 0x0 == Job Done
 */
#define PROP_LAST_JOB_STATE   0x80030012  // 8 == writing to card, 0 = idle, B = busy.
#define PROP_STROBO_FIRING    0x80040013  // 0 = enable, 1 = disable, 2 = auto?
#define PROP_STROBO_ETTLMETER 0x80040014  // 0 = evaluative, 1 = average
#define PROP_STROBO_CURTAIN   0x80040015  // 0 = first, 1 = second
#define PROP_STROBO_AECOMP    0x80000009  // in 1/8 EV steps, 8-bit signed
#define PROP_STROBO_SETTING   0x80030038  // len=22
#define PROP_STROBO_REDEYE    0x80000025  // 1 = enable, 0 = disable
//strobo_setting[0] & 0xFF000000 = strobo_aecomp << 24
//strobo_setting[1] & 2 = 2 if strobo_curtain else 0
//strobo_setting[1] & 8 = 8 if e-ttl meter = average else 0

#define PROP_LCD_BRIGHTNESS 0x2040000     // 1 .. 7

/** Gui properties? 0xffc509b0 @ 0xDA */

#define PROP_STROBO_FIRING 0x80040013 // 0 = enable, 1 = disable?

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
prop_deliver(
	uint32_t	prop,
	void *		buf,
	size_t		len,
	uint32_t	mzb
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



struct prop_handler
{
	unsigned	property;

	void *		(*handler)(
		unsigned		property,
		void *			priv,
		void *			addr,
		unsigned		len
	);

	void *		token; // must be before token_handler
	uint32_t	token_handler[2]; // function goes here!
};

/** Register a property handler with automated token function */
#define REGISTER_PROP_HANDLER( id, func ) \
__attribute__((section(".prop_handlers"))) \
__attribute__((used)) \
static struct prop_handler _prop_handler_##id##_block = { \
	.handler	= func, \
	.property	= id, \
}

#define PROP_HANDLER(id) \
static void * _prop_handler_##id(); \
REGISTER_PROP_HANDLER( id, _prop_handler_##id ); \
void * _prop_handler_##id( \
	unsigned		property, \
	void *			token, \
	uint32_t *		buf, \
	unsigned		len \
) \


#define PROP_INT(id,name) \
uint32_t name; \
PROP_HANDLER(id) { \
	name = buf[0]; \
	return prop_cleanup( token, property ); \
}


#endif
