/** \file
 * Common property values
 */

#include "dryos.h"
#include "bmp.h"

#define _propvalues_h_
#include "property.h"

volatile PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
volatile PROP_INT(PROP_LIVE_VIEW_VIEWTYPE, expsim);
volatile PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
volatile PROP_INT(PROP_EFIC_TEMP, efic_temp);
volatile PROP_INT(PROP_GUI_STATE, gui_state);
volatile PROP_INT(PROP_AUTO_ISO_RANGE, auto_iso_range);
volatile PROP_INT(PROP_PIC_QUALITY, pic_quality);
volatile PROP_INT(PROP_AVAIL_SHOT, avail_shot);
volatile PROP_INT(PROP_AF_MODE, af_mode);
volatile PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);
volatile PROP_INT(PROP_FILE_NUMBER, file_number);
volatile PROP_INT(PROP_FOLDER_NUMBER, folder_number);
volatile PROP_INT(PROP_FILE_NUMBER_ALSO, file_number_also);
volatile PROP_INT(PROP_TFT_STATUS, tft_status);
volatile PROP_INT(PROP_DRIVE, drive_mode);
volatile PROP_INT(PROP_STROBO_FIRING, strobo_firing);
volatile PROP_INT(PROP_LVAF_MODE, lvaf_mode);
volatile PROP_INT(PROP_IMAGE_REVIEW_TIME, image_review_time);
volatile PROP_INT(PROP_MIRROR_DOWN, mirror_down);
volatile PROP_INT(PROP_BACKLIGHT_LEVEL, backlight_level);
volatile PROP_INT(PROP_BEEP, beep_enabled);
volatile PROP_INT(PROP_LV_MOVIE_SELECT, lv_movie_select);
volatile PROP_INT(PROP_ACTIVE_SWEEP_STATUS, sensor_cleaning);
volatile PROP_INT(PROP_BURST_COUNT, burst_count);
volatile PROP_INT(PROP_BATTERY_POWER, battery_level_bars);
//~ int battery_level_bars = 0;

volatile int dofpreview;
PROP_HANDLER(PROP_DOF_PREVIEW_MAYBE) // len=2
{
	dofpreview = buf[0] & 0xFFFF;
	return prop_cleanup(token, property);
}

volatile int lv;

bool is_movie_mode()
{
	#if defined(CONFIG_50D) || defined(CONFIG_5D2)
	return lv && lv_movie_select == LVMS_ENABLE_MOVIE;
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
	return prop_cleanup(token, property);
}

volatile int display_sensor = 0;

PROP_HANDLER(PROP_DISPSENSOR_CTRL)
{
	display_sensor = !buf[0];
	return prop_cleanup(token, property);
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
	return prop_cleanup( token, property );
}
#endif

#ifdef CONFIG_500D
PROP_HANDLER(PROP_VIDEO_MODE)
{
	video_mode_resolution = buf[0];
	video_mode_fps = buf[1];
	return prop_cleanup( token, property );
}
#endif

PROP_HANDLER( PROP_LV_ACTION )
{
	lv = !buf[0];
	return prop_cleanup( token, property );
}

volatile PROP_INT(PROP_HDMI_CHANGE_CODE, hdmi_code);
volatile PROP_INT(PROP_HDMI_CHANGE, ext_monitor_hdmi);
volatile PROP_INT(PROP_USBRCA_MONITOR, ext_monitor_rca);

#ifdef CONFIG_50D
int recording = 0;
int shooting_type = 0;
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
	shooting_type = buf[0];
	recording = (shooting_type == 4 ? 2 : 0);
	return prop_cleanup( token, property );
}
#else
volatile PROP_INT(PROP_MVR_REC_START, recording);
volatile PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
#endif

int lv_disp_mode;
PROP_HANDLER(PROP_HOUTPUT_TYPE)
{
	#if defined(CONFIG_60D) || defined(CONFIG_600D) || defined(CONFIG_5D2)
	lv_disp_mode = (uint8_t)buf[1];
	#else
	lv_disp_mode = (uint8_t)buf[0];
	#endif
	return prop_cleanup(token, property);
}
