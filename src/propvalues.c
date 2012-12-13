/** \file
 * Common property values
 */

#include "dryos.h"
#include "bmp.h"

#define _propvalues_h_
#include "property.h"

volatile PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
volatile PROP_INT(PROP_LIVE_VIEW_VIEWTYPE, expsim);
volatile PROP_INT(PROP_EFIC_TEMP, efic_temp);
volatile PROP_INT(PROP_GUI_STATE, gui_state);
volatile PROP_INT(PROP_PIC_QUALITY, pic_quality);
volatile PROP_INT(PROP_AVAIL_SHOT, avail_shot);
volatile PROP_INT(PROP_AF_MODE, af_mode);
#ifndef CONFIG_5D3
volatile PROP_INT(PROP_FILE_NUMBER, file_number);
volatile PROP_INT(PROP_FOLDER_NUMBER, folder_number);
#endif
//volatile PROP_INT(PROP_FILE_NUMBER_ALSO, file_number_also);
volatile PROP_INT(PROP_DRIVE, drive_mode);
volatile PROP_INT(PROP_STROBO_FIRING, strobo_firing);
volatile PROP_INT(PROP_LVAF_MODE, lvaf_mode);
volatile PROP_INT(PROP_IMAGE_REVIEW_TIME, image_review_time);
volatile PROP_INT(PROP_MIRROR_DOWN, mirror_down);
volatile PROP_INT(PROP_LCD_BRIGHTNESS, backlight_level);
volatile PROP_INT(PROP_LV_MOVIE_SELECT, lv_movie_select);
volatile PROP_INT(PROP_ACTIVE_SWEEP_STATUS, sensor_cleaning);
volatile PROP_INT(PROP_BURST_COUNT, burst_count);
volatile PROP_INT(PROP_BATTERY_POWER, battery_level_bars);
//~ int battery_level_bars = 0;
PROP_INT(PROP_MOVIE_SOUND_RECORD, sound_recording_mode);

#ifndef CONFIG_5D2
volatile PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);
#else
int ae_mode_movie = 1;
#endif

volatile int shooting_mode;
PROP_HANDLER(PROP_SHOOTING_MODE)
{
    shooting_mode = buf[0];

    #ifdef CONFIG_5D2
    ae_mode_movie = shooting_mode == SHOOTMODE_M;
    #endif
}

volatile int dofpreview;
PROP_HANDLER(PROP_DOF_PREVIEW_MAYBE) // len=2
{
    dofpreview = buf[0] & 0xFFFF;
}

volatile int lv = 0;
volatile int lv_paused = 0; // not a property, but related

bool is_movie_mode()
{
    #if defined(CONFIG_50D) || defined(CONFIG_5D2)
    return lv && lv_movie_select == LVMS_ENABLE_MOVIE
            #if !defined(CONFIG_50D)
            && expsim == 2  // movie enabled, but photo display is considered photo mode
            #endif
        ;
    #elif defined(CONFIG_5D3) || defined(CONFIG_7D) || defined(CONFIG_EOSM) || defined(CONFIG_650D)
    return lv_movie_select == LVMS_ENABLE_MOVIE;
    #else
    return shooting_mode == SHOOTMODE_MOVIE;
    #endif
}

volatile int shutter_count = 0;
volatile int shutter_count_plus_lv_actuations = 0;
PROP_HANDLER(PROP_SHUTTER_COUNTER)
{
    shutter_count = buf[0];
    shutter_count_plus_lv_actuations = buf[1];
}

volatile int display_sensor = 0;

PROP_HANDLER(PROP_DISPSENSOR_CTRL)
{
    display_sensor = !buf[0];
}

volatile int video_mode_crop = 0;
volatile int video_mode_fps = 0;
volatile int video_mode_resolution = 0; // 0 if full hd, 1 if 720p, 2 if 480p
#ifndef CONFIG_500D
PROP_HANDLER(PROP_VIDEO_MODE)
{
    video_mode_crop = buf[0];
    video_mode_fps = buf[2];
    video_mode_resolution = buf[1];
}
#endif

#ifdef CONFIG_500D
PROP_HANDLER(PROP_VIDEO_MODE)
{
    video_mode_resolution = buf[0];
    video_mode_fps = buf[1];
}
#endif

PROP_HANDLER( PROP_LV_ACTION )
{
#ifdef CONFIG_5DC
    //~ make sure ML never thinks LV is open, because 5dc doesn't have LV.
    lv = 0;
#else
    lv = !buf[0];
#endif
}

volatile PROP_INT(PROP_HDMI_CHANGE_CODE, hdmi_code);
volatile PROP_INT(PROP_HDMI_CHANGE, ext_monitor_hdmi);
volatile PROP_INT(PROP_USBRCA_MONITOR, _ext_monitor_rca);

#ifdef CONFIG_50D
int recording = 0;
int shooting_type = 0;
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
    shooting_type = buf[0];
    recording = (shooting_type == 4 ? 2 : 0);
}

PROP_HANDLER(PROP_MOVIE_SIZE_50D)
{
    video_mode_resolution = buf[0];
    video_mode_fps = 30;
}
#else
volatile PROP_INT(PROP_MVR_REC_START, recording);
volatile PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
#endif

int lv_disp_mode;

PROP_HANDLER(PROP_HOUTPUT_TYPE)
{
    #if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_5D3) || defined(CONFIG_1100D) || defined(CONFIG_50D) || defined(CONFIG_EOSM) || defined(CONFIG_650D)
    lv_disp_mode = (uint8_t)buf[1];
    #else
    lv_disp_mode = (uint8_t)buf[0];
    #endif

    #ifdef CONFIG_5D2 // PROP_HOUTPUT_TYPE not reported correctly?
    lv_disp_mode = (MEM(0x34894 + 0x48) != 3); // AJ_LDR_0x34894_guess_HDMI_disp_type_related_0x48
    #endif

}

#if defined(CONFIG_5D2) || defined(CONFIG_50D) || defined(CONFIG_500D)
int auto_iso_range = 0x4868; // no auto ISO in Canon menus; considering it fixed 100-1600.
#else
volatile PROP_INT(PROP_AUTO_ISO_RANGE, auto_iso_range);
#endif
