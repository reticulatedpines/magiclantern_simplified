/** 
 * FPS control with engio calls (talking to DIGIC!)
 * This method is portable: works on all cameras.
 * 
 **/
 
/**
 * 
 * Notes by g3gg0:
 * 
 * okay i found how to directly change the sensor frame rate without patching and copying memory areas.
 * it doesnt matter which mode is selected.
 * 
 * on 600D v1.0.1 it is calling engio_write() with a buffer that writes the rate.
 * 
 * unsigned long frame_rate[] = {
 *      0xC0F06014, 0xFFFF, // timer register
 *      0xC0F06000, 0x01,   // coherent update
 *      0xFFFFFFFF          // end of commands
 * };
 *
 * void run_test()
 * {
 *     void (*engio_write)(unsigned int) = 0xFF1E1D20;
 *     frame_rate[1] = 0xFFFF; // timer value as usual [Alex: timer value minus 1 on certain cameras]
 *     engio_write(frame_rate);
 * }
 * 
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "menu.h"
#include "lens.h"
#include "config.h"

#if defined(CONFIG_50D) || defined(CONFIG_500D)
PROP_INT(PROP_VIDEO_SYSTEM, pal);
#endif

static int is_current_mode_ntsc()
{
    #if defined(CONFIG_50D)
    return !pal;
    #endif
    if (video_mode_fps == 30 || video_mode_fps == 60 || video_mode_fps == 24) return 1;
    return 0;
}

int fps_get_current_x1000();

#ifdef CONFIG_5D2
    #define TG_FREQ_BASE 24000000
    #define FPS_TIMER_OFFSET (-1)
    #define TG_FREQ_NTSC_SHUTTER 39300000
#else
    #ifdef CONFIG_500D
        #define TG_FREQ_BASE 31960800 // not 100% sure
        #define FPS_TIMER_OFFSET 0
    #else
        // 550D, 600D, 60D, 50D 
        #define TG_FREQ_BASE 28800000
        #define FPS_TIMER_OFFSET (-1)
        #define TG_FREQ_NTSC_SHUTTER 49440000
    #endif
#endif

static unsigned get_current_tg_freq()
{
    int timer = (MEMX(0xC0F06008) & 0xFFFF) + 1;
    if (timer == 1) return 0;
    unsigned f = (TG_FREQ_BASE / timer) * 1000 + mod(TG_FREQ_BASE, timer) * 1000 / timer;
    return f;
}

#define TG_FREQ_FPS get_current_tg_freq()
#define TG_FREQ_SHUTTER (ntsc ? TG_FREQ_NTSC_SHUTTER : TG_FREQ_FPS)

#ifdef CONFIG_550D
#define LV_STRUCT_PTR 0x1d14
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x64)
#endif


#ifdef CONFIG_600D
#define VIDEO_PARAMETERS_SRC_3 0x70AE8 // notation from g3gg0
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC))
#endif

#ifdef CONFIG_60D
#define VIDEO_PARAMETERS_SRC_3 0x4FDA8
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC))
#endif


//~ #define MIN_FPS_MENU (TG_FREQ_PAL / 16384 / 1000)
#define MIN_FPS_MENU 1

/*
#ifdef CONFIG_500D
#define LV_STRUCT_PTR 0x1d78
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#endif

#ifdef CONFIG_50D
#define LV_STRUCT_PTR 0x1D74
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5c)
#endif*/

#ifdef CONFIG_5D2
#define LV_STRUCT_PTR 0x1D78
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x60)
#endif

#define FPS_x1000_TO_TIMER(fps_x1000) (((fps_x1000)!=0)?(TG_FREQ_FPS/(fps_x1000)):0)
#define TIMER_TO_FPS_x1000(t) (((t)!=0)?(TG_FREQ_FPS/(t)):0)

#define SHUTTER_x1000_TO_TIMER(s_x1000) (TG_FREQ_SHUTTER/(s_x1000))
#define TIMER_TO_SHUTTER_x1000(t) (TG_FREQ_SHUTTER/(t))

int get_current_shutter_reciprocal_x1000()
{
#if defined(CONFIG_500D) || defined(CONFIG_50D)// || defined(CONFIG_5D2)
    if (!lens_info.raw_shutter) return 0;
    return (int) roundf(powf(2.0, (lens_info.raw_shutter - 136) / 8.0) * 1000.0 * 1000.0);
#else

    int timer = FRAME_SHUTTER_TIMER;
    int ntsc = is_current_mode_ntsc();

    int shutter_r_x1000 = TIMER_TO_SHUTTER_x1000(timer);
    
    // shutter speed can't be slower than 1/fps
    shutter_r_x1000 = MAX(shutter_r_x1000, fps_get_current_x1000());
    
    // FPS override will alter shutter speed
    // FPS difference will be added as a constant term to shutter speed
    int shutter_us = 1000000000 / shutter_r_x1000;
    int fps_timer_delta_us = 1000000000 / fps_get_current_x1000() - 1000000 / video_mode_fps;
    int ans = 1000000000 / (shutter_us + fps_timer_delta_us);
    //~ NotifyBox(2000, "su=%d cf=%d \nvf=%d td=%d \nans=%d", shutter_us, fps_get_current_x1000(), video_mode_fps, fps_timer_delta_us, ans);
    
    return ans;
#endif
}


static int fps_override = 0;
CONFIG_INT("fps.override", fps_override_value, 10);

//--------------------------------------------------------
// sound recording has to be disabled
// otherwise recording is not stable
//--------------------------------------------------------
static int old_sound_recording_mode = -1;

static void set_sound_recording(int x)
{
    #ifdef CONFIG_5D2
    Gui_SetSoundRecord(COERCE(x,1,3));
    #else
    prop_request_change(PROP_MOVIE_SOUND_RECORD, &x, 4);
    #endif
}

static void restore_sound_recording()
{
    if (recording) return;
    if (old_sound_recording_mode != -1)
    {
        set_sound_recording(old_sound_recording_mode);
        old_sound_recording_mode = -1;
        if (lv) redraw();
    }
}
static void disable_sound_recording()
{
    if (recording) return;
    if (sound_recording_mode != 1)
    {
        old_sound_recording_mode = sound_recording_mode;
        set_sound_recording(1);
        if (lv) redraw();
    }
}

static void update_sound_recording()
{
    if (recording) return;
    if (fps_override && lv) disable_sound_recording();
    else restore_sound_recording();
}

PROP_HANDLER(PROP_LV_ACTION)
{
    restore_sound_recording();
    return prop_cleanup(token, property);
}
//--------------------------------------------------------

static unsigned long frame_rate[] = {
    0xC0F06014, 0xFFFF, /* timer register */
    0xC0F06000, 0b1, /* coherent update */
    0xFFFFFFFF /* end of commands */ };

static int fps_get_timer(int fps)
{
    int ntsc = is_current_mode_ntsc();

    int fps_x1000 = fps * 1000;

    #if !defined(CONFIG_500D) && !defined(CONFIG_50D) // these cameras use 30.000 fps, not 29.97
    if (ntsc) fps_x1000 = fps_x1000 * 1000/1001;
    #endif

    // convert fps into timer ticks (for sensor drive speed)
    int fps_timer = FPS_x1000_TO_TIMER(fps_x1000);

    #if defined(CONFIG_500D) || defined(CONFIG_50D) // these cameras use 30.000 fps, not 29.97 => look in system settings to check if PAL or NTSC
    ntsc = !pal;
    #endif

    // NTSC is 29.97, not 30
    // also try to round it in order to avoid flicker
    if (ntsc)
    {
        int timer_120hz = FPS_x1000_TO_TIMER(120000*1000/1001);
        int fps_timer_rounded = ((fps_timer + timer_120hz/2) / timer_120hz) * timer_120hz;
        if (ABS(TIMER_TO_FPS_x1000(fps_timer_rounded) - fps_x1000 + 1) < 500) fps_timer = fps_timer_rounded;
    }
    else
    {
        int timer_100hz = FPS_x1000_TO_TIMER(100000);
        int fps_timer_rounded = ((fps_timer + timer_100hz/2) / timer_100hz) * timer_100hz;
        if (ABS(TIMER_TO_FPS_x1000(fps_timer_rounded) - fps_x1000 + 1) < 500) fps_timer = fps_timer_rounded;
    }

    return fps_timer & 0xFFFF;
}

static void fps_setup(int fps)
{
    if (!lv) return;
    if (!DISPLAY_IS_ON) return;
    
    if (fps) frame_rate[1] = fps_get_timer(fps);

    unsigned safe_limit = fps_get_timer(70);
#if defined(CONFIG_550D) || defined(CONFIG_5D2) || defined(CONFIG_50D)
    safe_limit = fps_get_timer(video_mode_fps); // no overcranking possible
#endif
#ifdef CONFIG_500D
    safe_limit = fps_get_timer(lv_dispsize > 1 ? 24 : video_mode_resolution == 0 ? 21 : 32);
#endif

    // no more than 30fps in photo mode
    if (!is_movie_mode()) safe_limit = MAX(safe_limit, fps_get_timer(30));
    
    frame_rate[1] = MAX(frame_rate[1], safe_limit);
    frame_rate[1] = MIN(frame_rate[1], 16383);

    frame_rate[1] += FPS_TIMER_OFFSET; // we may need to write computed timer value minus 1
    engio_write(frame_rate);

    update_sound_recording();
}

int fps_get_current_x1000()
{
    int fps_timer = MEMX(0xC0F06014) - FPS_TIMER_OFFSET;
    int fps_x1000 = TIMER_TO_FPS_x1000(fps_timer);
    return fps_x1000;
}

static void
fps_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int current_fps = fps_get_current_x1000();
    
    char msg[30];
    snprintf(msg, sizeof(msg), "%d (%d.%03d)", 
        (current_fps+500)/1000, 
        current_fps/1000, current_fps%1000
        );
    
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "FPS override  : %s",
        fps_override ? msg : "OFF"
    );
    
    menu_draw_icon(x, y, MNI_BOOL(fps_override), 0);
}

static void flip_zoom()
{
    if (!lv) return;
    if (is_movie_mode())
    {
        if (recording) return;
        if (video_mode_crop) return;
    }
    
    // flip zoom mode back and forth to apply settings instantly
    int zoom0 = lv_dispsize;
    int zoom1 = zoom0 == 10 ? 5 : zoom0 == 5 ? 1 : 10;
    prop_request_change(PROP_LV_DISPSIZE, &zoom1, 4);
    prop_request_change(PROP_LV_DISPSIZE, &zoom0, 4);
}

int shutter_override_enabled = 0;

static void fps_reset()
{
    fps_override = 0;
    shutter_override_enabled = 0;
    //~ fps_override_value = video_mode_fps;
    
    if (!recording) flip_zoom(); // this will force reconfiguring fps with Canon settings
    else fps_setup(video_mode_fps);

    restore_sound_recording();
}


static void fps_change_value(void* priv, int delta)
{
    #ifdef CONFIG_500D
    fps_override_value = COERCE(fps_override_value + delta, MIN_FPS_MENU, 70);
    #else
    fps_override_value = COERCE(fps_override_value + delta, MIN_FPS_MENU, 70);
    #endif
    if (fps_override) fps_setup(fps_override_value);
}

static void fps_enable_disable(void* priv, int delta)
{
    fps_override = !fps_override;
    if (fps_override) fps_setup(fps_override_value);
    else fps_reset();
}

struct menu_entry fps_menu[] = {
    {
        .name = "FPS override", 
        .priv = &fps_override,
        .select = fps_enable_disable,
        .display = fps_print,
        .help = "Changes FPS. Also sets shutter at 1/fps and disables sound.",
        .children =  (struct menu_entry[]) {
            {
                .name = "New FPS value",
                .priv       = &fps_override_value,
                .min = 0,
                .max = 60,
                .select = fps_change_value,
                .help = "FPS value for recording. Video will play back at Canon FPS.",
            },
            MENU_EOL
        },
    },
};


static void fps_init()
{
    menu_add( "Movie", fps_menu, COUNT(fps_menu) );
}

INIT_FUNC("fps", fps_init);

static void fps_task()
{
    while(1)
    {
        #ifdef CONFIG_500D
        if (fps_override && lv) // on 500D, it needs to be refreshed continuously (don't know why)
        {
            msleep(20);
            if (lv) fps_setup(fps_override_value);
        }
        #else
        if (fps_override && lv) // on other cameras, it's OK to refresh every now and then (just to make sure it's active after you change video mode)
        {
            msleep(500);
            if (lv) fps_setup(fps_override_value);
        }
        #endif
        else
        {
            msleep(500);
        }
        shutter_override_enabled = fps_override;
    }
}

TASK_CREATE("fps_task", fps_task, 0, 0x1c, 0x1000 );


void fps_mvr_log(FILE* mvr_logfile)
{
    int f = fps_get_current_x1000();
    if (fps_override)
        my_fprintf(mvr_logfile, "FPS+Tv override: %d (%d.%03d)\n", (f+500)/1000, f/1000, f%1000);
}

// FPS has a side effect: to force shutter speed at 1/fps. Let the bottom bar show this.
//~ int is_hard_shutter_override_active() { return fps_override; }
//~ int get_shutter_override_degrees_x10() { return fps_override ? 3600 : 0; }


void
shutter_override_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Shutter(MOV): 360deg 1/%d",
        fps_override_value
    );

    menu_draw_icon(x, y, MNI_ON, 0);
}

void shutter_override_toggle(void* priv, int delta) { }

void fps_set_main_timer(int t)
{
    t = t & 0xFFFF;
    //~ NotifyBox(1000, "%d ", t);
    t--;
    int t2 = t | (t << 16);
    EngDrvOut(0xC0F06008, t2);
    EngDrvOut(0xC0F0600c, t2);
    EngDrvOut(0xC0F06010, t);
    EngDrvOut(0xC0F06001, 1);
}

int is_shutter_override_enabled_movie() { return 0; }
