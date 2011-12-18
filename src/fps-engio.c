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
 *     frame_rate[1] = 0xFFFF; // timer value as usual
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

static int is_current_mode_ntsc()
{
    if (video_mode_fps == 30 || video_mode_fps == 60 || video_mode_fps == 24) return 1;
    return 0;
}

#define TG_FREQ_PAL  50000000
#define TG_FREQ_NTSC 52747252

#define FPS_x1000_TO_TIMER_PAL(fps_x1000) (TG_FREQ_PAL/(fps_x1000))
#define FPS_x1000_TO_TIMER_NTSC(fps_x1000) (TG_FREQ_NTSC/(fps_x1000))
#define TIMER_TO_FPS_x1000_PAL(t) (TG_FREQ_PAL/(t))
#define TIMER_TO_FPS_x1000_NTSC(t) (TG_FREQ_NTSC/(t))

static int fps_override = 0;

static unsigned long frame_rate[] = {
    0xC0F06014, 0xFFFF, /* timer register */
    0xC0F06000, 0x01, /* coherent update */
    0xFFFFFFFF /* end of commands */ };

static int fps_get_timer(int fps)
{
    int ntsc = is_current_mode_ntsc();

    int fps_x1000 = fps * 1000;

    // convert fps into timer ticks (for sensor drive speed)
    int fps_timer = ntsc ? FPS_x1000_TO_TIMER_NTSC(fps_x1000*1000/1001) : FPS_x1000_TO_TIMER_PAL(fps_x1000);

    // NTSC is 29.97, not 30
    // also try to round it in order to avoid flicker
    if (ntsc)
    {
        int timer_120hz = FPS_x1000_TO_TIMER_NTSC(120000*1000/1001);
        int fps_timer_rounded = ((fps_timer + timer_120hz/2) / timer_120hz) * timer_120hz;
        if (ABS(TIMER_TO_FPS_x1000_NTSC(fps_timer_rounded) - fps_x1000 + 1) < 500) fps_timer = fps_timer_rounded;
    }
    else
    {
        int timer_100hz = FPS_x1000_TO_TIMER_PAL(100000);
        int fps_timer_rounded = ((fps_timer + timer_100hz/2) / timer_100hz) * timer_100hz;
        if (ABS(TIMER_TO_FPS_x1000_PAL(fps_timer_rounded) - fps_x1000 + 1) < 500) fps_timer = fps_timer_rounded;
    }

    return fps_timer & 0xFFFF;
}

static void fps_setup(int fps)
{
    if (fps) frame_rate[1] = fps_get_timer(fps);

    unsigned safe_limit = fps_get_timer(70);
#ifdef CONFIG_550D
    safe_limit = fps_get_timer(video_mode_fps); // no overcranking possible
    if (video_mode_crop)
    {
        frame_rate[1] = safe_limit;
        return; // freeze
    }
#endif
#ifdef CONFIG_500D
    safe_limit = fps_get_timer(video_mode_resolution == 0 ? 40 : 70);
#endif
#ifdef CONFIG_5D2
    safe_limit = fps_get_timer(video_mode_resolution == 0 ? 35 : 70);
#endif

    // no more than 30fps in photo mode
    if (!is_movie_mode()) safe_limit = MAX(safe_limit, fps_get_timer(30));
    
    frame_rate[1] = MAX(frame_rate[1], safe_limit);

    engio_write(frame_rate);
}

static int fps_get_current_x1000()
{
    if (!fps_override) return video_mode_fps * 1000;
    int fps_timer = frame_rate[1];
    int ntsc = is_current_mode_ntsc();
    int fps_x1000 = ntsc ? TIMER_TO_FPS_x1000_NTSC(fps_timer) : TIMER_TO_FPS_x1000_PAL(fps_timer);
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
        "FPS override : %s",
        fps_override ? msg : "OFF"
    );
    
    extern int sound_recording_mode;
    if (fps_override && sound_recording_mode != 1)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t)"Turn off sound recording from Canon menu!");
    menu_draw_icon(x, y, MNI_BOOL(fps_override), 0);
    //~ bmp_hexdump(FONT_SMALL, 0, 400, SENSOR_TIMING_TABLE, 32);
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

static void reset_fps(void* priv, int delta)
{
    fps_override = 0;
    
    if (!recording) flip_zoom(); // this will force reconfiguring fps with Canon settings
    else fps_setup(video_mode_fps);
}

static void set_fps(void* priv, int delta)
{
    // first click won't change value
    int fps = (fps_get_current_x1000() + 500) / 1000; // rounded value
    if (fps_override) fps = COERCE(fps + delta, 4, 70);
    
    fps_setup(fps);
    
    fps_override = 1;
}


static struct menu_entry fps_menu[] = {
    {
        .name = "FPS override", 
        .priv = &fps_override,
        .select = set_fps,
        .select_auto = reset_fps,
        .display = fps_print,
        .show_liveview = 1,
        .help = "Makes French Fries with the camera sensor. Turn off sound!"
    },
};

static void fps_init()
{
    menu_add( "Debug", fps_menu, COUNT(fps_menu) );
}

INIT_FUNC("fps", fps_init);

static void fps_task()
{
    while(1)
    {
        if (fps_override && lv) fps_setup(0);
        msleep(1000);
    }
}

TASK_CREATE("fps_task", fps_task, 0, 0x1d, 0x1000 );


void fps_mvr_log(FILE* mvr_logfile)
{
    int f = fps_get_current_x1000();
    my_fprintf(mvr_logfile, "FPS: %d (%d.%03d)\n", (f+500)/1000, f/1000, f%1000);
}
