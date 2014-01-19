#ifndef _propvalues_h_
#define _propvalues_h_

extern char camera_model_short[8];
extern char camera_model[32];
extern uint32_t camera_model_id;
extern char firmware_version[32];

#define MODEL_EOS_10D    0x80000168
#define MODEL_EOS_300D   0x80000170
#define MODEL_EOS_20D    0x80000175
#define MODEL_EOS_450D   0x80000176
#define MODEL_EOS_350D   0x80000189
#define MODEL_EOS_40D    0x80000190
#define MODEL_EOS_5D     0x80000213
#define MODEL_EOS_5D2    0x80000218
#define MODEL_EOS_30D    0x80000234
#define MODEL_EOS_400D   0x80000236
#define MODEL_EOS_7D     0x80000250
#define MODEL_EOS_500D   0x80000252
#define MODEL_EOS_1000D  0x80000254
#define MODEL_EOS_50D    0x80000261
#define MODEL_EOS_550D   0x80000270
#define MODEL_EOS_5D3    0x80000285
#define MODEL_EOS_600D   0x80000286
#define MODEL_EOS_60D    0x80000287
#define MODEL_EOS_1100D  0x80000288
#define MODEL_EOS_650D   0x80000301
#define MODEL_EOS_6D     0x80000302
#define MODEL_EOS_70D    0x80000325
#define MODEL_EOS_700D   0x80000326
#define MODEL_EOS_M      0x80000331
#define MODEL_EOS_100D   0x80000346

extern int lv; // TRUE when LiveView is active
extern int lv_paused; // only valid if lv is true
#define LV_PAUSED (lv_paused)
#define LV_NON_PAUSED (lv && !lv_paused)

extern int lv_dispsize; // 1 / 5 / A
extern int expsim;
extern int shooting_mode;        /* C3M => M */
extern int shooting_mode_custom; /* C3M => C3 */
extern int shooting_type;
extern int efic_temp;
extern int gui_state;
extern int auto_iso_range;
extern int pic_quality;
//~ extern int burst_count;
extern int avail_shot;
extern int __recording;
extern int __recording_custom;
#define NOT_RECORDING (__recording == 0 && __recording_custom == 0)
#define RECORDING (__recording || __recording_custom)
#define RECORDING_H264 (__recording > 0)
#define RECORDING_H264_STARTING (__recording == 1) // 1 is preparing for recording
#define RECORDING_H264_STARTED (__recording == 2) //2 is actually recording
#define RECORDING_RAW (__recording_custom ==  CUSTOM_RECORDING_RAW)
#define RECORDING_MJPEG (__recording_custom == CUSTOM_RECORDING_MJPEG) // not implemented, except for some proof of concept code
#define RECORDING_CUSTOM (__recording_custom > 0) // anything that is not H.264
#define RECORDING_STATE (__recording | (__recording_custom << 2))

#define CUSTOM_RECORDING_NOT_RECORDING   0
#define CUSTOM_RECORDING_RAW             1
#define CUSTOM_RECORDING_MJPEG           2
void set_recording_custom(int state);

extern int af_mode;
extern int metering_mode;
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

#endif
