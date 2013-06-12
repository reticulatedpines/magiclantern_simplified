#ifndef _propvalues_h_
#define _propvalues_h_

extern char camera_model_short[8];
extern char camera_model[32];
extern char firmware_version[32];

extern int lv; // TRUE when LiveView is active
extern int lv_paused; // only valid if lv is true
#define LV_PAUSED (lv_paused)
#define LV_NON_PAUSED (lv && !lv_paused)

extern int lv_dispsize; // 1 / 5 / A
extern int expsim;
extern int shooting_mode;
extern int shooting_type;
extern int efic_temp;
extern int gui_state;
extern int auto_iso_range;
extern int pic_quality;
//~ extern int burst_count;
extern int avail_shot;
extern int recording;
extern int af_mode;
extern int dofpreview;
extern int display_sensor;
extern int shutter_count;
extern int shutter_count_plus_lv_actuations;
extern int ae_mode_movie;
extern int ext_monitor_hdmi;
extern int _ext_monitor_rca;
extern int file_number;
extern int folder_number;
extern int file_number_also;
extern int drive_mode;
extern int strobo_firing;
extern int lvaf_mode;
extern int image_review_time;
extern int lv_disp_mode;
extern int mirror_down;
extern int hdmi_code;
extern int backlight_level;
extern int video_mode_crop;
extern int video_mode_fps;
extern int video_mode_resolution; // 0 if full hd, 1 if 720p, 2 if 480p
extern int lv_movie_select;
extern int sensor_cleaning;
extern int burst_count;
extern int battery_level_bars;
extern int sound_recording_mode; // 1 = disable?
extern char artist_name[64];
extern char copyright_info[64];
extern int date_format;
extern int auto_power_off_time;

#define EXT_MONITOR_CONNECTED (ext_monitor_hdmi || _ext_monitor_rca)
#define EXT_MONITOR_RCA (_ext_monitor_rca && !ext_monitor_hdmi)

extern struct bmp_ov_loc_size os;

bool is_movie_mode();

#ifndef _beep_c_
extern int beep_enabled;
#endif

#define EFIC_CELSIUS ((int)efic_temp - 128)

#endif
