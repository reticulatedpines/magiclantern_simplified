#ifndef __SHOOT_H_
#define __SHOOT_H_
void hdr_shot(int skip0, int wait);
int expo_value_rounding_ok(int raw, int is_aperture);
int round_shutter(int tv, int slowest_shutter);
int round_aperture(int av);
int is_hdr_bracketing_enabled();

int get_interval_count();
int get_interval_time();
void set_interval_time(int seconds);
void intervalometer_stop();
int is_intervalometer_running();

/* take a single picture according to current settings */
/* (regular, bulb, or custom, e.g. silent) */
int take_a_pic(int should_af);

/* take a sequence of regular pictures in continuous (burst) mode */
/* (note: your camera must be already in some continuous mode) */
int take_fast_pictures(int number);

/* take a long exposure image in BULB mode */
/* returns nonzero if canceled by user, zero otherwise */
int bulb_take_pic(int duration);

/* convert bulb timer setting to raw_shutter units */
int get_bulb_shutter_raw_equiv();

/* true if you are in bulb mode (some cameras have a dedicated mode, others have a BULB position for shutter speed in M mode) */
int is_bulb_mode();

/* switch to BULB mode */
void ensure_bulb_mode();

/* start/stop recording */
void movie_start();
void movie_end();
void schedule_movie_start();
void schedule_movie_end();

/* ask user to switch to movie/photo mode and wait until s/he does */
void ensure_movie_mode();
void ensure_photo_mode();

/* to be refactored as callback */
void mvr_rec_start_shoot();

/* set zoom to 1x/5x/10x */
/* todo: move somewhere else (where?) */
void set_lv_zoom(int zoom);

/* todo: move it somewhere else (playback tricks?) */
void next_image_in_play_mode(int direction);
void play_zoom_center_on_selected_af_point();

/* set+maindial actions for playback */
void expfuse_preview_update(int direction);
void playback_compare_images(int direction);
void expo_adjust_playback(int direction);
void playback_compare_images(int direction);

/* private (to be made static) */
void expfuse_preview_update_task(int direction);
void playback_compare_images_task(int direction);

/* movtweaks.c (to be moved in a more logical place) */
void force_liveview();
void close_liveview();

/* to be moved to separate file (backlight.c/h?) */
void set_backlight_level(int level);

/* same here */
int set_display_gain_equiv(int gain);
int get_digic_iso_gain_photo();
int get_digic_iso_gain_movie();
int gain_to_ev_scaled(int gain, int scale);
void set_photo_digital_iso_gain_for_bv(int gain);
int set_movie_digital_iso_gain_for_gradual_expo(int gain);

/* get name of current shooting mode (todo: move to shoot.c or somewhere else?) */
char* get_shootmode_name(int shooting_mode);
char* get_shootmode_name_short(int shooting_mode);

/* be careful calling this one */
void set_shooting_mode(int mode);

/* focus box, to be moved to separate file */
void get_afframe_pos(int W, int H, int* x, int* y);
void get_afframe_sensor_res(int* W, int* H);
void afframe_set_dirty();
void afframe_clr_dirty();
void clear_lv_afframe();    /* in tweaks.c */
void clear_lv_afframe_if_dirty();
void move_lv_afframe(int dx, int dy);
void center_lv_afframe();

/* kelvin WB (to be moved? where?) */
void kelvin_n_gm_auto();

/* true if RGB multipliers are 1/1/1 (or very close) */
int uniwb_is_active();

/* called from lens.c to tell hdr bracketing that user just took a picture */
void hdr_flag_picture_was_taken();

/* private stuff (HDR script shared with focus stacking) */
void hdr_create_script(int f0, int focus_stack);
int hdr_script_get_first_file_number(int skip0);

/* MLU */
int get_mlu();
int set_mlu();
int get_mlu_delay(int raw);
int mlu_lock_mirror_if_needed(); /* implemented in lens.c */

/* flash */
void set_flash_firing(int mode);

/* trap focus */
int get_trap_focus();

/* enqueue a shoot (take picture) command to shoot_task */
void schedule_remote_shot();

/* drive mode */
int set_drive_single();
int is_continuous_drive();

/* expo lock */
void expo_lock_update_value();

/* to be refactored with CBRs (maybe with the lvinfo engine) */
void iso_refresh_display();
void display_trap_focus_info();
void free_space_show_photomode();

const char* format_time_hours_minutes_seconds(int seconds);

/* after IMG_9999, Canon wraps around to IMG_0001 */
#define DCIM_WRAP(x) (MOD((x) - 1, 9999) + 1)

#endif // __SHOOT_H_

