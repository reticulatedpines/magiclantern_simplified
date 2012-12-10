#include "dryos.h"
#include "bmp.h"
#include "gui.h"
#include "menu.h"

// dummy stubs

void movtweak_task_init() {}
void movtweak_step() {}

int fps_get_shutter_speed_shift() { return 0; }

void *AcquireRecursiveLock(void *lock, int n){ return NULL; }
//~ void *CreateRecursiveLock(int n){}
void *ReleaseRecursiveLock(void *lock){ return NULL; }
int audio_meters_are_drawn() { return 0; } 
int override_zoom_buttons = 0;
void lcd_release_step(){};
void force_liveview() {};
int lcd_release_running = 0;
void set_shooting_mode(){};
int get_screen_layout(){ return 0; };
void rec_notify_trigger(){};
void set_display_gain_equiv(){};
int fps_get_current_x1000() { return 0; }
void display_lcd_remote_icon(){}
int get_lcd_sensor_shortcuts(){ return 0; }
void digic_iso_toggle(){}
void digic_iso_print(){}
int lvae_iso_min;
int lvae_iso_max;
int lvae_iso_speed;
int get_current_shutter_reciprocal_x1000(){ return 0; }
int is_mvr_buffer_almost_full(){ return 0; }
void rec_notify_continuous(){};
void movie_indicators_show(){};
void display_shooting_info(){};
void free_space_show_photomode(){};
void dialog_redraw(){}
int digic_iso_gain_photo;
int digic_iso_gain_movie;
void MuteOff_0(){}
void MuteOn_0(){}
void free_space_show(){};
void fps_show();
int time_indic_x = 0;
int time_indic_y = 0;

void bitrate_mvr_log(){};

int bv_auto = 0;
int bv_tv = 0;
int bv_av = 0;
int bv_iso = 0;
void bv_enable(){};
void bv_disable(){};
void bv_toggle(){};

void fps_mvr_log(){};
void hdr_mvr_log(){};

int hdrv_enabled = 0;
void hdr_get_iso_range(){};
void fps_show(){};

void input_toggle(){};
void out_volume_up(){};
void out_volume_down(){};
void volume_up(){};
void volume_down(){};

void _EngDrvOut(int addr, int value) { MEM(addr) = value; }
int shamem_read(int addr) { return 0; }
void _engio_write() {}

int gain_to_ev_scaled() { return 0; }
int gain_to_ev_x8() { return 0; }

void do_movie_mode_remap(){};

void restore_kelvin_wb(){};

void NormalDisplay(){};
void MirrorDisplay(){};

int _dummy_variable = 0;

void bzero32(void* buf, size_t len) { memset(buf, 0, len); }

void HijackFormatDialogBox(){};

int handle_digic_poke(struct event * event) { return 1; }
int handle_movie_rec_key(struct event * event) { return 1; }
int handle_movie_mode_shortcut(struct event * event) { return 1; }
int handle_fps_events(struct event * event) { return 1; }

int get_effective_hdr_iso_for_display(int raw_iso ) { return 1; };
void set_movie_digital_iso_gain(int gain) {};

int bmp_is_on() { return 1; }

void* shoot_malloc(int size) { return AllocateMemory(size); }
void shoot_free(void* ptr) { FreeMemory(ptr); }

void display_gain_toggle(){};
void yuv_resize(){};
