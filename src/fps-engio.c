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

static int fps_get_current_x1000();

#ifdef CONFIG_500D
    #define TG_FREQ_PAL  24660000
    #define TG_FREQ_NTSC_FPS 23160000
    #define FPS_TIMER_OFFSET 0
#else
    #define FPS_TIMER_OFFSET (-1)
    #ifdef CONFIG_5D2
        #define TG_FREQ_PAL  40000000
        #define TG_FREQ_NTSC_FPS 41958000
        #define TG_FREQ_NTSC_SHUTTER 39300000
    #else
        #ifdef CONFIG_50D
            #define TG_FREQ_PAL  41400000
            #define TG_FREQ_NTSC_FPS 41400000
        #else
            #define TG_FREQ_PAL  50000000            // these values are OK on 550D
            #define TG_FREQ_NTSC_FPS 52747200
            #define TG_FREQ_NTSC_SHUTTER 50000000
        #endif
    #endif
#endif

#ifdef CONFIG_550D
#define LV_STRUCT_PTR 0x1d14
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x64)
#endif

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

#define FPS_x1000_TO_TIMER_PAL(fps_x1000) (TG_FREQ_PAL/(fps_x1000))
#define FPS_x1000_TO_TIMER_NTSC(fps_x1000) (TG_FREQ_NTSC_FPS/(fps_x1000))
#define TIMER_TO_FPS_x1000_PAL(t) (TG_FREQ_PAL/(t))
#define TIMER_TO_FPS_x1000_NTSC(t) (TG_FREQ_NTSC_FPS/(t))
#define TIMER_TO_SHUTTER_x1000_NTSC(t) (TG_FREQ_NTSC_SHUTTER/(t))

int get_current_shutter_reciprocal_x1000()
{
#if defined(CONFIG_500D) || defined(CONFIG_50D)
    if (!lens_info.raw_shutter) return 0;
    return (int) roundf(powf(2.0, (136 - lens_info.raw_shutter) / 8.0) * 1000);
#else

    int timer = FRAME_SHUTTER_TIMER;

    int mode = 
        video_mode_fps == 60 ? 0 : 
        video_mode_fps == 50 ? 1 : 
        video_mode_fps == 30 ? 2 : 
        video_mode_fps == 25 ? 3 : 
        video_mode_fps == 24 ? 4 : 0;
    int ntsc = (mode % 2 == 0);

    int shutter_x1000 = ntsc ? TIMER_TO_SHUTTER_x1000_NTSC(timer) : TIMER_TO_FPS_x1000_PAL(timer);
    return MAX(shutter_x1000, fps_get_current_x1000());
#endif
}


static int fps_override = 0;
CONFIG_INT("fps.override", fps_override_value, 10);

static unsigned long frame_rate[] = {
    0xC0F06014, 0xFFFF, /* timer register */
    0xC0F06000, 0b1, /* coherent update */
    0xFFFFFFFF /* end of commands */ };

static int fps_get_timer(int fps)
{
    int ntsc = is_current_mode_ntsc();

    int fps_x1000 = fps * 1000;

    // convert fps into timer ticks (for sensor drive speed)
    int fps_timer = ntsc ? FPS_x1000_TO_TIMER_NTSC(fps_x1000
        #if !defined(CONFIG_500D) && !defined(CONFIG_500D) // these cameras use 30.000 fps, not 29.97
        *1000/1001
        #endif
    ) : FPS_x1000_TO_TIMER_PAL(fps_x1000);

    // NTSC is 29.97, not 30
    // also try to round it in order to avoid flicker
    #if defined(CONFIG_500D) || defined(CONFIG_500D) // these cameras use 30.000 fps, not 29.97 => look in system settings to check if PAL or NTSC
    if (!pal)
    #else
    if (ntsc)
    #endif
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
#if defined(CONFIG_550D) || defined(CONFIG_5D2) || defined(CONFIG_50D)
    safe_limit = fps_get_timer(video_mode_fps); // no overcranking possible
    if (video_mode_crop)
    {
        frame_rate[1] = safe_limit;
        return; // freeze
    }
#endif
#ifdef CONFIG_500D
    safe_limit = fps_get_timer(lv_dispsize > 1 ? 24 : video_mode_resolution == 0 ? 21 : 32);
#endif

    // no more than 30fps in photo mode
    if (!is_movie_mode()) safe_limit = MAX(safe_limit, fps_get_timer(30));
    
    frame_rate[1] = MAX(frame_rate[1], safe_limit);

    frame_rate[1] += FPS_TIMER_OFFSET; // we may need to write computed timer value minus 1
    engio_write(frame_rate);
}

static int fps_get_current_x1000()
{
    if (!fps_override) return video_mode_fps * 1000;
    int fps_timer = frame_rate[1] - FPS_TIMER_OFFSET;
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
        "FPS override  : %s",
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

int shutter_override_enabled = 0;

static void fps_reset()
{
    fps_override = 0;
    shutter_override_enabled = 0;
    //~ fps_override_value = video_mode_fps;
    
    if (!recording) flip_zoom(); // this will force reconfiguring fps with Canon settings
    else fps_setup(video_mode_fps);
}


static void fps_change_value(void* priv, int delta)
{
    #ifdef CONFIG_500D
    fps_override_value = COERCE(fps_override_value + delta, 2, 70);
    #else
    fps_override_value = COERCE(fps_override_value + delta, 4, 70);
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
        .help = "Changes FPS and forces shutter at 1/fps. Turn off sound!",
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
        if (fps_override && lv && !gui_menu_shown())
        {
            #ifdef CONFIG_500D
            msleep(30);
            #else
            msleep(500);
            #endif
            if (lv) fps_setup(fps_override_value);
        }
        else
        {
            msleep(500);
        }
        shutter_override_enabled = fps_override;
    }
}

TASK_CREATE("fps_task", fps_task, 0, 0x1d, 0x1000 );


void fps_mvr_log(FILE* mvr_logfile)
{
    int f = fps_get_current_x1000();
    if (fps_override)
        my_fprintf(mvr_logfile, "FPS+Tv override: %d (%d.%03d)\n", (f+500)/1000, f/1000, f%1000);
}

// FPS has a side effect: to force shutter speed at 1/fps. Let the bottom bar show this.
int is_hard_shutter_override_active() { return fps_override; }
int get_shutter_override_degrees_x10() { return fps_override ? 3600 : 0; }


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


