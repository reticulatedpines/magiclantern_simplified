/** \file
 * Common property values
 */

#include "dryos.h"

#define _propvalues_h_
#include "property.h"

volatile PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
volatile PROP_INT(PROP_LIVE_VIEW_VIEWTYPE, expsim);
volatile PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
volatile PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
volatile PROP_INT(PROP_EFIC_TEMP, efic_temp);
volatile PROP_INT(PROP_GUI_STATE, gui_state);
volatile PROP_INT(PROP_MAX_AUTO_ISO, max_auto_iso);
volatile PROP_INT(PROP_PIC_QUALITY, pic_quality);
volatile PROP_INT(PROP_AVAIL_SHOT, avail_shot);
volatile PROP_INT(PROP_MVR_REC_START, recording);
volatile PROP_INT(PROP_AF_MODE, af_mode);
volatile PROP_INT(PROP_DOF_PREVIEW_MAYBE, dofpreview);
volatile PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);
volatile PROP_INT(PROP_HDMI_CHANGE, ext_monitor_hdmi);
volatile PROP_INT(PROP_USBRCA_MONITOR, ext_monitor_rca)
volatile PROP_INT(PROP_ALO, alo);
volatile PROP_INT(PROP_FILE_NUMBER, file_number);
volatile PROP_INT(PROP_FOLDER_NUMBER, folder_number);
volatile PROP_INT(PROP_FILE_NUMBER_ALSO, file_number_also);
volatile PROP_INT(PROP_TFT_STATUS, tft_status);
volatile PROP_INT(PROP_DRIVE, drive_mode);
volatile PROP_INT(PROP_STROBO_FIRING, strobo_firing);
volatile PROP_INT(PROP_LVAF_MODE, lvaf_mode);
volatile PROP_INT(PROP_IMAGE_REVIEW_TIME, image_review_time);
volatile PROP_INT(PROP_MIRROR_DOWN, mirror_down);
volatile PROP_INT(PROP_HDMI_CHANGE_CODE, hdmi_code)
volatile PROP_INT(PROP_BACKLIGHT_LEVEL, backlight_level);

volatile int shutter_count = 0;
volatile int liveview_actuations = 0;
PROP_HANDLER(PROP_SHUTTER_COUNTER)
{
	shutter_count = buf[0];
	liveview_actuations = buf[1];
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
PROP_HANDLER(PROP_VIDEO_MODE)
{
	video_mode_crop = buf[0];
	video_mode_fps = buf[2];
	video_mode_resolution = buf[1];
	return prop_cleanup( token, property );
}

volatile int lv;
PROP_HANDLER( PROP_LV_ACTION )
{
	lv = !buf[0];
	return prop_cleanup( token, property );
}
