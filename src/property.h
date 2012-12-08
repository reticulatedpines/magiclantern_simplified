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
#define PROP_BURST_COUNT        0x80030006
#define PROP_BAT_INFO           0x8003001d
#define PROP_TFT_STATUS         0x80030015
#define PROP_LENS_NAME          0x80030021
#define PROP_LENS_SOMETHING     0x80030022

//~ 5dc doesn't have a PROP_LENS.
#ifdef CONFIG_5DC
#define PROP_LENS               0x80030010
#else
#define PROP_LENS               0x80030011 // info about lens? flags?
#endif

#define PROP_HDMI_CHANGE        0x8003002c // 1 if HDMI display connected
#define PROP_HDMI_CHANGE_CODE   0x8003002e // edidc?
#define PROP_USBRCA_MONITOR 0x80030018
#define PROP_MVR_REC_START      0x80030033 // 0 = no, 1 = stating, 2 = recording


/** 0x02010000 - 0x02010014 == card state? */
#define PROP_REC_TIME           0x80030005 // time remaining.  output at 1 Hz while recording

/** These are good guesses */
#define PROP_GUI_STATE          0x80020000 // 0 == IDLE, 1 == PLAYMENU?, 2 = menu->disp, 3 == START_QR_MODE, 9 = Q menu
#define PROP_LIVE_VIEW_FACE_AF  0x0205000A
#define PROP_LV_LOCK            0x80050021
#define PROP_LV_ACTION          0x80050022 // 0 == LV_START, 1 == LV_STOP

/** These are guesses */
#define PROP_LCD_POSITION       0x80040020 // 0 = like on non-flippable, 1 = backwards, 2 = flipped outside
#define PROP_USBDEVICE_CONNECT  0x8004000a
#define PROP_MVR_MOVW_START0    0x80000020 // not sure?
#define PROP_MVR_MOVW_START1    0x80000021
#define PROP_AF_MODE            0x80000004 // 0 = one shot, 3 == manual focus, 202 = ai (dumb) focus, 101 = ai servo (slightly better)
#define PROP_MVR_REC            0x80030002
#define PROP_LV_LENS            0x80050000
#define PROP_LV_0004            0x80050004
#define PROP_LV_LENS_STABILIZE  0x80050005 // 0 = off, e0000 = on
#define PROP_LV_MANIPULATION    0x80050006
#define PROP_LV_AFFRAME         0x80050007 // called by ptp handler 915a
#define PROP_LV_FOCUS           0x80050001 // only works in liveview mode; LVCAF_LensDriveStart
#define PROP_LV_FOCUS_DONE      0x80050002 // output when focus motor is done?
#define PROP_LV_FOCUS_STOP      0x80050003 // LVCAF_LensDriveStop
#define PROP_LV_FOCUS_BAD       0x80050029 // true if camera couldn't focus?
#define PROP_LV_FOCUS_STATE     0x80050009 // 1 OK, 101 bad, 201 not done?
#define PROP_LV_FOCUS_CMD       0x80050027 // 3002 = full speed, 4/5 = slow, 6 = fine tune?
#define PROP_LV_FOCUS_DATA      0x80050026 // 8 integers; updates quickly when AF is active
#define PROP_LVAF_0003          0x80050003
#define PROP_LVAF_001D          0x8005001d // LVCAF_LensSearchDriveStart
#define PROP_LV_STATE           0x8005000f // output very often
#define PROP_LV_DISPSIZE        0x80050015 // used to control LV zoom (1 / 5 / A)
#define PROP_LVCAF_STATE        0x8005001B // unknown meaning
#define PROP_HALF_SHUTTER       0x8005000a // two bytes, 1==held; only updated in LV mode
#define PROP_ORIENTATION        0x8005000d // 0 == 0 deg, 1 == +90 deg, 2 == -90 deg
#define PROP_LV_LENS_DRIVE_REMOTE 0x80050013 // what values?!

#define PROP_APERTURE2          0x8000002d
#define PROP_APERTURE3          0x80000036
#define PROP_LIVE_VIEW_VIEWTYPE 0x80000034

#define PROP_MODE               0x80000001 // maybe; set in FA_DISP_COM

#define PROP_DRIVE              0x80000003
#define DRIVE_SINGLE 0
#define DRIVE_SELFTIMER_REMOTE 0x10
#ifdef CONFIG_5DC
#define DRIVE_SELFTIMER_2SEC 0x10
#else
#define DRIVE_SELFTIMER_2SEC 0x11
#endif
#define DRIVE_SELFTIMER_CONTINUOUS 7

#ifdef CONFIG_60D
    #define DRIVE_HISPEED_CONTINUOUS 4
    #define DRIVE_CONTINUOUS 5
#elif defined(CONFIG_5D3)
    #define DRIVE_HISPEED_CONTINUOUS 4
    #define DRIVE_CONTINUOUS 5
    #define DRIVE_SILENT 19
    #define DRIVE_SILENT_CONTINUOUS 20
#else
    #define DRIVE_CONTINUOUS 1
#endif

#define PROP_SHUTTER            0x80000005
#define PROP_SHUTTER_ALSO       0x8000002c
#define PROP_APERTURE           0x80000006
#define PROP_ISO                        0x80000007
#define PROP_AUTO_ISO_RANGE     0x8000003b // len=2, LSB is max iso, MSB is min iso (ignored?)
#define PROP_AE                         0x80000008 // signed 8-bit value
#define PROP_UILOCK                     0x8000000b // maybe?
#define PROP_ISO_AUTO           0x8000002E // computed by AUTO ISO if PROP_ISO is 0; otherwise, equal to PROP_ISO; in movie mode, is 0 unless you half-press shutter

#define PROP_SHUTTER_RELEASE    0x8003000A
#define PROP_AVAIL_SHOT         0x80030005

#define PROP_MIC_INSERTED       0x8003002b

/** These need to be found */
#define PROP_LCD_STATE          error_must_be_found

/** These are used by DL: 8002000C, 80030014, 100000E, 80020013 */
// These come form master result cbr
#define PROP_SENSOR             0x80020002
#define PROP_DEFAULT_CAMERA     0x80020003
#define PROP_DEFAULT_CFN        0x80020004
#define PROP_DEFALT_AF_SHIFT    0x80020005
#define PROP_DEFAULT_LV_MANIP   0x80020006
#define PROP_DISPSENSOR_CTRL    0x80020010      // 1 == show results?
#define PROP_LV_OUTPUT_DEVICE   0x80050011      // 1 == LCD?
#define PROP_HOUTPUT_TYPE       0x80030030      // 0 = no info displayed in LV, 1 = info displayed (this is toggled with DISP)
#define PROP_MIRROR_DOWN        0x8005001C
#define PROP_LV_EXPSIM          0x80050023
#define PROP_MYMENU_LISTING     0x80040009

#define PROP_LV_MOVIE_SELECT    0x8004001C      // 0=DisableMovie, 1=? or 2=EnableMovie
#define LVMS_DISABLE_MOVIE 0
#define LVMS_ENABLE_MOVIE 2

#define PROP_SHOOTING_TYPE      0x80040004      // 0 == NORMAL, 3 == LV, 4 == MOVIE
#define PROP_ERR_BATTERY        0x80030020
#define PROP_CUSTOM_SETTING     0x80020007
#define PROP_DEFAULT_CUSTOM     0x80020008
#define PROP_DEFAULT_BRACKET    0x8002000A
#define PROP_PARTIAL_SETTING    0x8002000B
#define PROP_EMPOWER_OFF        0x80030007      // 1 == prohibit, 2 == permit
#define PROP_LVAF_MODE      0x8004001d // 0 = shutter killer, 1 = live mode, 2 = face detect

#define PROP_ACTIVE_SWEEP_STATUS 0x8002000C     // 1 == cleaning sensor?

#ifndef CONFIG_5DC
#define PROP_DL_ACTION          0x80020013 // 0 == end?
#endif

#ifdef CONFIG_5DC
#define PROP_EFIC_TEMP          0x80030013
#else
#define PROP_EFIC_TEMP          0x80030014
#endif

#define PROP_EFIC_TEMP_MAYBE            0x010100ed
//#define PROP_BATTERY_RAW_LEVEL_MAYBE          0x80030014

#define PROP_ARTIST_STRING      0x0E070000
#define PROP_COPYRIGHT_STRING   0x0E070001

#define PROP_LANGUAGE           0x02040002
#define PROP_VIDEO_SYSTEM       0x02040003

#define PROP_ICU_UILOCK         0x80020009
#define UILOCK_NONE       0x41000000
#define UILOCK_EVERYTHING_EXCEPT_POWEROFF_AND_MODEDIAL 0x4100014f
#define UILOCK_EVERYTHING 0x4100017f
#define UILOCK_SHUTTER    0x41000001
#define UILOCK_ARROWS     0x41000008
#define UILOCK_MODE_DIAL  0x41000010
#define UILOCK_POWER_SW   0x41000020

#define PROP_SHOOTING_MODE  0x80000000 // During mode switch, it takes other values.
#define SHOOTMODE_P 0
#define SHOOTMODE_TV 1
#define SHOOTMODE_AV 2
#define SHOOTMODE_M 3
#define SHOOTMODE_BULB 4
#define SHOOTMODE_ADEP 5
#define SHOOTMODE_C 7
#define SHOOTMODE_C2 0x10
#define SHOOTMODE_C3 0x11
#define SHOOTMODE_CA 0x13
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

#define PROP_CUSTOM_WB 0x2020000 // len=52, contains multipliers at 0x20 [B?], 0x24 [G], 0x26 [R?] as int16, 1024=1

#define PROP_LAST_JOB_ID     0x02050001 // maybe?

#ifdef CONFIG_5DC
#define PROP_PICTURE_STYLE 0x80000020
#else
#define PROP_PICTURE_STYLE 0x80000028   // 0x81 = std, 82 = portrait, 83 = landscape, 84 = neutral, 85 = faithful, 86 = monochrome, 21 = user 1, 22 = user 2, 23 = user 3
#endif

#define PROP_PICSTYLE_SETTINGS_STANDARD   0x02060001 // 02060001 for std, 02060002 for portrait... 02060007 for user 1 ... 02060009 for user 3
#define PROP_PICSTYLE_SETTINGS_PORTRAIT   0x02060002
#define PROP_PICSTYLE_SETTINGS_LANDSCAPE  0x02060003
#define PROP_PICSTYLE_SETTINGS_NEUTRAL    0x02060004
#define PROP_PICSTYLE_SETTINGS_FAITHFUL   0x02060005
#define PROP_PICSTYLE_SETTINGS_MONOCHROME 0x02060006
#define PROP_PICSTYLE_SETTINGS_USERDEF1   0x02060007
#define PROP_PICSTYLE_SETTINGS_USERDEF2   0x02060008
#define PROP_PICSTYLE_SETTINGS_USERDEF3   0x02060009
#define PROP_PICSTYLE_OF_USERDEF1   0x0206000a
#define PROP_PICSTYLE_OF_USERDEF2   0x0206000b
#define PROP_PICSTYLE_OF_USERDEF3   0x0206000c
#define PROP_PICSTYLE_SETTINGS_AUTO       0x0206001e // 600D only

#define PROP_PC_FLAVOR1_PARAM             0x4010001
#define PROP_PC_FLAVOR2_PARAM             0x4010003
#define PROP_PC_FLAVOR3_PARAM             0x4010005

#define PROP_ALO 0x8000003D
#define ALO_STD 0
#define ALO_LOW 1
#define ALO_HIGH 2
#define ALO_OFF 3

#ifdef CONFIG_5D3
#define PROP_HTP 0x8000004a
#define PROP_MULTIPLE_EXPOSURE 0x0202000c
#define PROP_MLU 0x80000047
#endif

/** Job progress
 * 0xB == capture end?
 * 0xA == start face catch pass?
 * 0x8 == "guiSetDarkBusy" -- noise reduction?
 * 0x0 == Job Done
 */
#ifdef CONFIG_5DC
#define PROP_LAST_JOB_STATE   0x80030011
#else
#define PROP_LAST_JOB_STATE   0x80030012  // 8 == writing to card, 0 = idle, B = busy.
#endif

#define PROP_STROBO_FIRING    0x80040013  // 0 = enable, 1 = disable, 2 = auto?
#define PROP_STROBO_ETTLMETER 0x80040014  // 0 = evaluative, 1 = average
#define PROP_STROBO_CURTAIN   0x80040015  // 0 = first, 1 = second
#define PROP_STROBO_AECOMP    0x80000009  // in 1/8 EV steps, 8-bit signed, len=4
#define PROP_STROBO_SETTING   0x80030038  // len=22
#define PROP_STROBO_REDEYE    0x80000025  // 1 = enable, 0 = disable
#define PROP_POPUP_BUILTIN_FLASH 0x80020019 // request with 1
//strobo_setting[0] & 0xFF000000 = strobo_aecomp << 24
//strobo_setting[1] & 2 = 2 if strobo_curtain else 0
//strobo_setting[1] & 8 = 8 if e-ttl meter = average else 0

#define PROP_LCD_BRIGHTNESS 0x2040000     // 1 .. 7
#define PROP_LCD_BRIGHTNESS_MODE 0x204000D // 0=auto, 1=manual (5D)
#define PROP_LCD_BRIGHTNESS_AUTO_LEVEL 0x204000C // dark, normal, bright? (5D)

/** Gui properties? 0xffc509b0 @ 0xDA */

#define PROP_STROBO_FIRING 0x80040013 // 0 = enable, 1 = disable?

#define PROP_MOVIE_SIZE_50D 0x205000C


#ifdef CONFIG_500D
#define PROP_VIDEO_MODE 0x2050010
// buf[0]: 0 if 1080p, 1 if 720p, 2 if 480p
// buf[1]: 14 if 1080p (20fps), 1e if 720p or 480p (30fps)
// buf[2]: a if 1080p (1/2 of 20fps??), f if 720p or 480p (1/2 of 30fps??)
#endif

#ifndef CONFIG_500D
#define PROP_VIDEO_MODE 0x80000039 
// buf[0]: 8 if crop else 0
// buf[1]: 0 if full hd, 1 if 720p, 2 if 680p
// buf[2]: fps
// buf[3]: GoP
#endif

#define PROP_DOF_PREVIEW_MAYBE 0x8005000B

#define PROP_REMOTE_SW1 0x80020015
#define PROP_REMOTE_SW2 0x80020016
#define PROP_PROGRAM_SHIFT             0x80000033
#define PROP_QUICKREVIEW               0x2020006
#define PROP_QUICKREVIEW_MODE          0x2020005
#define PROP_REMOTE_AFSTART_BUTTON     0x80020017
#define PROP_REMOTE_BULB_RELEASE_END   0x8002000F
#define PROP_REMOTE_BULB_RELEASE_START 0x8002000E
#define PROP_REMOTE_RELEASE            0x8003000A
#define PROP_REMOTE_SET_BUTTON         0x80020018

/* some properties found while reverse engineering */
#define PROP_FA_ADJUST_FLAG            0x80040000



#if defined(CONFIG_5DC) || defined(CONFIG_40D) // not sure, it might be like 5D2
    #define PROP_FOLDER_NUMBER     0x2010000
    #define PROP_FILE_NUMBER       0x2010002
    #define PROP_CARD_RECORD       0x8003000B
    #define PROP_CLUSTER_SIZE      0x2010004
    #define PROP_FREE_SPACE        0x2010006
#elif defined(CONFIG_50D) || defined(CONFIG_5D2) || defined(CONFIG_7D)
    #define PROP_CLUSTER_SIZE      0x02010006
    #define PROP_FREE_SPACE        0x02010009
    //#define PROP_FILE_NUMBER       0x02040007 // if last saved file is IMG_1234, then this property is 1234. Works both in photo and video mode.
    #define PROP_FILE_NUMBER  0x02010003 // seems to mirror the previous one, but it's increased earlier
    #define PROP_FOLDER_NUMBER     0x02010000 // 100, 101...
    #define PROP_CARD_RECORD       0x8003000b // set when writing on the card
#elif defined(CONFIG_5D3) // two card slots
    
    #define PROP_CARD_SELECT         0x80040002 //  1=CF, 2=SD

    // CF card
    #define PROP_CLUSTER_SIZE_A      0x02010006
    #define PROP_FREE_SPACE_A        0x02010009
    #define PROP_FILE_NUMBER_A       0x02010003
    #define PROP_FOLDER_NUMBER_A     0x02010000
    #define PROP_CARD_RECORD_A       0x8003000b

    // SD card
    #define PROP_CLUSTER_SIZE_B      0x02010007
    #define PROP_FREE_SPACE_B        0x0201000a
    #define PROP_FILE_NUMBER_B       0x02010004
    #define PROP_FOLDER_NUMBER_B     0x02010001
    #define PROP_CARD_RECORD_B       0x8003000c
#else
    #define PROP_CLUSTER_SIZE      0x02010007
    #define PROP_FREE_SPACE        0x0201000a // in clusters
    //#define PROP_FILE_NUMBER       0x02040008 // if last saved file is IMG_1234, then this property is 1234. Works both in photo and video mode.
    #define PROP_FILE_NUMBER       0x02010004 // seems to mirror the previous one, but it's increased earlier
    #define PROP_FOLDER_NUMBER     0x02010001 // 100, 101...
    #define PROP_CARD_RECORD       0x8003000c // set when writing on the card
#endif


#define PROP_USER_FILE_PREFIX  0x02050004
#define PROP_SELECTED_FILE_PREFIX  0x02050008
#define PROP_CARD_COVER 0x8003002F

#define PROP_TERMINATE_SHUT_REQ 0x80010001

#define PROP_BUTTON_ASSIGNMENT 0x80010007

#define PROP_PIC_QUALITY   0x8000002f
#define PROP_PIC_QUALITY2  0x80000030
#define PROP_PIC_QUALITY3  0x80000031
#define PICQ_RAW                 0x4060000
#define PICQ_MRAW                0x4060001
#define PICQ_SRAW                0x4060002
#define PICQ_RAW_JPG_LARGE_FINE  0x3070000
#define PICQ_MRAW_JPG_LARGE_FINE 0x3070001
#define PICQ_SRAW_JPG_LARGE_FINE 0x3070002
#define PICQ_RAW_JPG_MED_FINE    0x3070100
#define PICQ_MRAW_JPG_MED_FINE   0x3070101
#define PICQ_SRAW_JPG_MED_FINE   0x3070102
#define PICQ_RAW_JPG_SMALL_FINE  0x3070200
#define PICQ_MRAW_JPG_SMALL_FINE 0x3070201
#define PICQ_SRAW_JPG_SMALL_FINE 0x3070202
#define PICQ_LARGE_FINE   0x3010000
#define PICQ_LARGE_COARSE 0x2010000
#define PICQ_MED_FINE     0x3010100
#define PICQ_MED_COARSE   0x2010100
#define PICQ_SMALL_FINE   0x3010200
#define PICQ_SMALL_COARSE 0x2010200

#ifdef CONFIG_5DC
#define PROP_IMAGE_REVIEW_TIME 0x0202000b
#else
#define PROP_IMAGE_REVIEW_TIME 0x02020006 // 0, 2, 4, 8, ff
#endif

#define PROP_BATTERY_REPORT     0x8003001D
#define PROP_BATTERY_HISTORY    0x0204000F
#define PROP_BATTERY_CHECK      0x80030013
#define PROP_BATTERY_POWER 0x80030004

#define PROP_AE_MODE_MOVIE 0x8000003a

#define PROP_WINDCUT_MODE 0x02050016

#define PROP_SCREEN_COLOR 0x0204000b

#define PROP_ROLLING_PITCHING_LEVEL 0x80030039

#define PROP_VRAM_SIZE_MAYBE 0x8005001f

#define PROP_ICU_AUTO_POWEROFF 0x80030024
#define PROP_AUTO_POWEROFF_TIME 0x80000024
#define PROP_REBOOT_MAYBE 0x80010003 // used by firmware update code

#define PROP_DIGITAL_ZOOM_RATIO 0x8005002f

#define PROP_INFO_BUTTON_FUNCTION 0x02070006

#define PROP_CONTINUOUS_AF_MODE 0x80000042
#define PROP_CONTINUOUS_AF_VALID 0x80000043
#define PROP_LOUDNESS_BUILT_IN_SPEAKER 0x205001B
#define PROP_LED_LIGHT 0x80030042
#define PROP_AFSHIFT_LVASSIST_STATUS 0x8003003D
#define PROP_AFSHIFT_LVASSIST_SHIFT_RESULT 0x8003003E 
#define PROP_MULTIPLE_EXPOSURE_CTRL 0x80070001
#define PROP_MIRROR_DOWN_IN_MOVIE_MODE 0x80030034

//~ #define PROP_CARD2_CLUSTER_SIZE 0x2010007
#define PROP_SHUTTER_COUNTER 0x80030029

#define PROP_AFPOINT 0x8000000A // len=7

#define PROP_BEEP 0x80000023

#define PROP_ELECTRIC_SHUTTER 0x80040011

#ifdef CONFIG_5D2
#define PROP_MOVIE_SOUND_RECORD 0x2050015
#else
#define PROP_MOVIE_SOUND_RECORD 0x205000E
#endif

#define PROP_LOGICAL_CONNECT 0x8003000e

#define PROP_BV 0x80050010
#define PROP_LV_BV 0x80050008
#define PROP_STROBO_CHARGE_INFO_MAYBE 0x8005000C

#define PROP_ONESHOT_RAW 0x80000037

#define PROP_DCIM_DIR_SUFFIX 5 // :)
#define PROP_FILE_PREFIX 0x1000007

#define PROP_AEB 0x8000000B 


#ifdef CONFIG_600D
#define PROP_PLAYMODE_VOL_CHANGE_600D	0x205000F //volume change when playing a video by wheel
#define PROP_AUDIO_VOL_CHANGE_600D	0x2050017 //volume change finished from Cannon Audio menu
#define PROP_PLAYMODE_LAUNCH_600D	0x205000D //Playmode and Q(Quick setting menu) launched

#endif
/** Properties */
extern void
prop_register_slave(
        unsigned *      property_list,
        unsigned        count,
        void *          (*prop_handler)(
                unsigned                property,
                void *                  priv,
                void *                  addr,
                unsigned                len
        ),
        void *          priv,
        void            (*token_handler)( void * token )
);


extern void *
prop_cleanup(
        void *          token,
        unsigned        property
);

// prop
OS_FUNCTION( 0x0600001,	void,	prop_request_change, unsigned property, const void* addr, size_t len );
/** Get the current value of a property.
 *
 * int* data = 0;
 * int len = 0;
 * int err = prop_get_value(prop, &data, &len);
 * 
 * Returns 0 on success.
 */
OS_FUNCTION( 0x0600002,	int,	prop_get_value, unsigned property, void** addr, size_t* len );


extern void
prop_deliver(
        uint32_t        prop,
        void *          buf,
        size_t          len,
        uint32_t        mzb
);

struct prop_handler
{
        unsigned        property;
        unsigned        property_length;

        void          (*handler)(
                unsigned                property,
                void *                  priv,
                void *                  addr,
                unsigned                len
        );

        void *          token; // must be before token_handler
        uint32_t        token_handler[2]; // function goes here!
};


/** Configure a property handler; only necessary if not using automated functions */
extern void
prop_handler_init(
        struct prop_handler * handler
);


/** Register a property handler with automated token function */

#define REGISTER_PROP_HANDLER_EX( id, func, length ) \
__attribute__((section(".prop_handlers"))) \
__attribute__((used)) \
static struct prop_handler _prop_handler_##id##_block = { \
        .handler         = func, \
        .property        = id, \
        .property_length = length, \
}

#define REGISTER_PROP_HANDLER( id, func ) REGISTER_PROP_HANDLER_EX( id, func, 0 )

#define PROP_HANDLER(id) \
static void _prop_handler_##id(); \
REGISTER_PROP_HANDLER( id, _prop_handler_##id ); \
void _prop_handler_##id( \
        unsigned                property, \
        void *                  token, \
        uint32_t *              buf, \
        unsigned                len \
) \


#define PROP_INT(id,name) \
volatile uint32_t name; \
PROP_HANDLER(id) { \
        name = buf[0]; \
}

/**for reading simple integer properties */
int get_prop(int prop);

#include "propvalues.h"

#endif
