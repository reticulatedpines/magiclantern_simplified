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
volatile PROP_INT(PROP_MAX_AUTO_ISO, max_auto_iso);
volatile PROP_INT(PROP_PIC_QUALITY, pic_quality);
volatile PROP_INT(PROP_AVAIL_SHOT, avail_shot);
volatile PROP_INT(PROP_AF_MODE, af_mode);
volatile PROP_INT(PROP_DOF_PREVIEW_MAYBE, dofpreview);
volatile PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);
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
volatile PROP_INT(PROP_BACKLIGHT_LEVEL, backlight_level);
volatile PROP_INT(PROP_BEEP, beep_enabled);
volatile PROP_INT(PROP_LV_MOVIE_SELECT, lv_movie_select);
volatile PROP_INT(PROP_ACTIVE_SWEEP_STATUS, sensor_cleaning);

volatile int lv;

int is_movie_mode()
{
	#ifdef CONFIG_50D
	return lv && lv_movie_select == LVMS_ENABLE_MOVIE;
	#else
	return shooting_mode == SHOOTMODE_MOVIE;
	#endif
}

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

PROP_HANDLER( PROP_LV_ACTION )
{
	lv = !buf[0];
	return prop_cleanup( token, property );
}


// External monitors

struct bmp_ov_loc_size os;

volatile int ext_monitor_hdmi;
volatile int ext_monitor_rca;
volatile int hdmi_code;

static void calc_ov_loc_size(struct bmp_ov_loc_size * os)
{
	if (hdmi_code == 2 || ext_monitor_rca)
	{
		os->x0 = 40;
		os->y0 = 24;
		os->x_ex = 640;
		os->y_ex = 388;
	}
	else if (hdmi_code == 5)
	{
		os->x0 = (1920-1620) / 4;
		os->y0 = 0;
		os->x_ex = 540 * 3/2;
		os->y_ex = 540;
	}
	else
	{
		os->x0 = 0;
		os->y0 = 0;
		os->x_ex = 720;
		os->y_ex = 480;
	}
	os->x_max = os->x0 + os->x_ex;
	os->y_max = os->y0 + os->y_ex;
}

PROP_HANDLER(PROP_HDMI_CHANGE_CODE)
{
	hdmi_code = buf[0];
	calc_ov_loc_size(&os);
	return prop_cleanup( token, property );
}
PROP_HANDLER(PROP_HDMI_CHANGE)
{
	ext_monitor_hdmi = buf[0];
	calc_ov_loc_size(&os);
	return prop_cleanup( token, property );
}
PROP_HANDLER(PROP_USBRCA_MONITOR)
{
	ext_monitor_rca = buf[0];
	calc_ov_loc_size(&os);
	return prop_cleanup( token, property );
}


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


#ifdef CONFIG_60D
int lv_disp_mode;
PROP_HANDLER(PROP_HOUTPUT_TYPE)
{
	lv_disp_mode = buf[1];
	return prop_cleanup(token, property);
}
#else
PROP_INT(PROP_HOUTPUT_TYPE, lv_disp_mode);
#endif
