/** \file
 * Common property values
 */

#include "dryos.h"

#define _propvalues_h_
#include "property.h"

PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
PROP_INT(PROP_LIVE_VIEW_VIEWTYPE, expsim);
PROP_INT(PROP_SHOOTING_MODE, shooting_mode);
PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
PROP_INT(PROP_EFIC_TEMP, efic_temp);
PROP_INT(PROP_GUI_STATE, gui_state);
PROP_INT(PROP_MAX_AUTO_ISO, max_auto_iso);
PROP_INT(PROP_PIC_QUALITY, pic_quality);
//~ PROP_INT(PROP_BURST_COUNT, burst_count);
PROP_INT(PROP_AVAIL_SHOT, avail_shot);
PROP_INT(PROP_MVR_REC_START, recording);
PROP_INT(PROP_AF_MODE, af_mode);
PROP_INT(PROP_DOF_PREVIEW_MAYBE, dofpreview);
PROP_INT(PROP_SHUTTER_COUNT, shutter_count);
PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);
PROP_INT(PROP_HDMI_CHANGE, ext_monitor_hdmi);
PROP_INT(PROP_ALO, alo);
PROP_INT(PROP_FILE_NUMBER, file_number);
PROP_INT(PROP_FOLDER_NUMBER, folder_number);
PROP_INT(PROP_FILE_NUMBER_ALSO, file_number_also);
PROP_INT(PROP_TFT_STATUS, tft_status);
PROP_INT(PROP_DRIVE, drive_mode);
PROP_INT(PROP_STROBO_FIRING, strobo_firing);
PROP_INT(PROP_LVAF_MODE, lvaf_mode);
PROP_INT(PROP_IMAGE_REVIEW_TIME, image_review_time);
PROP_INT(PROP_HOUTPUT_TYPE, lv_disp_mode);
PROP_INT(PROP_MIRROR_DOWN, mirror_down);
PROP_INT(PROP_HDMI_CHANGE_CODE, hdmi_code)
PROP_INT(PROP_BACKLIGHT_LEVEL, backlight_level);

int display_sensor = 0;

PROP_HANDLER(PROP_DISPSENSOR_CTRL)
{
	display_sensor = !buf[0];
	return prop_cleanup(token, property);
}
