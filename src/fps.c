/** 
 * FPS control
 * http://magiclantern.wikia.com/wiki/VideoTimer
 * 
 * Found by g3gg0
 **/

#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "menu.h"
#include "lens.h"
#include "config.h"

#ifdef CONFIG_600D
#define SENSOR_TIMING_TABLE MEM(0xCB20)
#define VIDEO_PARAMETERS_SRC_3 0x70AE8 // notation from g3gg0
#define CARTIRIDGE_CALL_TABLE 0x8AAC
#define LiveViewMgr_struct_ptr 0x1dcc  // aAJ_0x1D78_LiveViewMgr_struct_ptr
#endif
#ifdef CONFIG_60D
#define SENSOR_TIMING_TABLE MEM(0x2a668)
#define VIDEO_PARAMETERS_SRC_3 0x4FDA8
#define LiveViewMgr_struct_ptr 0x1E80
#define CARTIRIDGE_CALL_TABLE 0x26490
#endif
#ifdef CONFIG_1100D
#define SENSOR_TIMING_TABLE MEM(0xce98)
#endif

#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC))
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x8))

#define HOOK_TABLE_FUNCTION(table,position,new,old) \
    old = ((unsigned int*)table)[position];\
    ((unsigned int*)table)[position] = new;\

#define UNHOOK_TABLE_FUNCTION(table,position,old) \
    ((unsigned int*)table)[position] = old;\

#define IS_TABLE_FUNCTION_HOOKED(table,position,new) \
    (((unsigned int*)table)[position] == new) \

#define REDIRECT_BUFFER(address,buffer) \
    memcpy(buffer, (unsigned char*)(*((unsigned int *)(address))), sizeof(buffer));\
    *((unsigned int *)(address)) = (unsigned int)buffer;\

struct lv_path_struct
{
    int SM; // ?! 1 in video mode, 0 in zoom and photo mode
    int fps_sensor_mode; // 24p:4, 25p:3, 30p:2, 50p:1, 60p:0
    int S; // 1920:0, 720:1, vgacrop:4, zoom:8
    int R; // movie size: 1920:0, 720:1, 480:2
    int Z; // (1 / 5 / A) << 16
    int recording;
    int DZ; // bool?
};

extern struct lv_path_struct lv_path_struct;

#define TG_FREQ_PAL  50000000
#define TG_FREQ_NTSC 52747252

#define FPS_x1000_TO_TIMER_PAL(fps_x1000) (TG_FREQ_PAL/(fps_x1000))
#define FPS_x1000_TO_TIMER_NTSC(fps_x1000) (TG_FREQ_NTSC/(fps_x1000))
#define TIMER_TO_FPS_x1000_PAL(t) (TG_FREQ_PAL/(t))
#define TIMER_TO_FPS_x1000_NTSC(t) (TG_FREQ_NTSC/(t))

static uint16_t * sensor_timing_table_original = 0;
static uint16_t sensor_timing_table_patched[128];

static int fps_override = 0;
static int hard_expo_override = 0;
static CONFIG_INT("override.tv.mode", shutter_override_mode, 2); // 180 degrees

static int hdr_enabled = 0;
static CONFIG_INT("hdrmov.ev", hdr_ev, 2);
static CONFIG_INT("hdrmov.mode", hdr_mode, 0);

static void hdr_ev_toggle(void* priv, int delta)
{
    MEM(priv) = mod(MEM(priv) + delta*16, 8*8) & ~3;
}

static int iso_override = 0;
static void iso_override_toggle(void* priv, int delta)
{
    if (delta > 0)
    {
        if (iso_override == 0) iso_override = 72;
        else if (iso_override < 112) iso_override += 8;
        else iso_override = 0;
    }
    else
    {
        if (iso_override == 0) iso_override = 112;
        else if (iso_override > 72) iso_override -= 8;
        else iso_override = 0;
    }
}

static int video_mode[5];
PROP_HANDLER(PROP_VIDEO_MODE)
{
    memcpy(video_mode, buf, 20);
    return prop_cleanup(token, property);
}

static const int mode_offset_map[] = { 3, 6, 1, 5, 4, 0, 2 };

static int fps_get_current_x1000()
{
    int mode = 
        video_mode_fps == 60 ? 0 : 
        video_mode_fps == 50 ? 1 : 
        video_mode_fps == 30 ? 2 : 
        video_mode_fps == 25 ? 3 : 
        video_mode_fps == 24 ? 4 : 0;
    int fps_timer = ((uint16_t*)SENSOR_TIMING_TABLE)[mode_offset_map[mode]];
    int ntsc = (mode % 2 == 0);
    int fps_x1000 = ntsc ? TIMER_TO_FPS_x1000_NTSC(fps_timer) : TIMER_TO_FPS_x1000_PAL(fps_timer);
    return fps_x1000;
}

static int shutter_get_timer(int degrees_x10)
{
    int mode = 
        video_mode_fps == 60 ? 0 : 
        video_mode_fps == 50 ? 1 : 
        video_mode_fps == 30 ? 2 : 
        video_mode_fps == 25 ? 3 : 
        video_mode_fps == 24 ? 4 : 0;
    int ntsc = (mode % 2 == 0);
    int fps_x1000 = fps_get_current_x1000();
    int timer = ntsc ? FPS_x1000_TO_TIMER_NTSC(fps_x1000) 
                     : FPS_x1000_TO_TIMER_PAL (fps_x1000);
    return MAX(1, timer * degrees_x10 / 3600);
}

static char cartridge_table[0xF4];
static void (*cartridge_AfStopPathReal)(void *this) = NULL;


static void cartridge_AfStopPath(void *this)
{
    /* force the ISO/exposure values */
    shutter_and_hdrvideo_set();

    /* call hooked function */
    cartridge_AfStopPathReal(this);
}


// Q: what happes if this is called when Canon firmware flips the resolution?
static void update_hard_expo_override()
{
    if (hard_expo_override)
    {
        // cartridge call table is sometimes overriden by Canon firmware
        // so this function polls the status periodically (and updates it if needed)
        if (!is_hard_exposure_override_active())
        {
            /* first clone the cartridge call table */
            REDIRECT_BUFFER(CARTIRIDGE_CALL_TABLE, cartridge_table);

            /* now hook the function Af_StopPath in cloned cartridge_table */
            HOOK_TABLE_FUNCTION(cartridge_table, 0x39, cartridge_AfStopPath, cartridge_AfStopPathReal);
            //~ beep();
            
            lens_display_set_dirty();
       }
    }
    else
    {
        if (is_hard_exposure_override_active() && cartridge_AfStopPathReal)
        {
            UNHOOK_TABLE_FUNCTION(cartridge_table, 0x39, cartridge_AfStopPathReal);
            cartridge_AfStopPathReal = NULL;
            //~ beep();

            lens_display_set_dirty();
        }
    }
}

int is_hard_exposure_override_active()
{
    return MEM(CARTIRIDGE_CALL_TABLE) == cartridge_table &&
        IS_TABLE_FUNCTION_HOOKED(cartridge_table, 0x39, cartridge_AfStopPath);
}

int get_shutter_override_degrees_x10()
{
    // 0, 360, 270, 180, 90, 60, 30, 15...
    if (shutter_override_mode == 0) return 0;
    if (shutter_override_mode <= 4) return (5-shutter_override_mode) * 900;
    return 600 >> (shutter_override_mode - 5);
}

static int get_shutter_override_degrees()
{
    return get_shutter_override_degrees_x10() / 10;
}

static int get_shutter_override_reciprocal_x1000()
{
    int mode = 
        video_mode_fps == 60 ? 0 : 
        video_mode_fps == 50 ? 1 : 
        video_mode_fps == 30 ? 2 : 
        video_mode_fps == 25 ? 3 : 
        video_mode_fps == 24 ? 4 : 0;
    int timer = shutter_get_timer(get_shutter_override_degrees_x10());
    int ntsc = (mode % 2 == 0);
    int shutter_x1000 = ntsc ? TIMER_TO_FPS_x1000_NTSC(timer) : TIMER_TO_FPS_x1000_PAL(timer);
    return shutter_x1000;
}

// called every frame
void shutter_and_hdrvideo_set()
{
    if (!hard_expo_override) return;
    int degrees_x10 = get_shutter_override_degrees_x10();

    static int odd_frame = 0;
    static int frame;
    frame++;
    
    if (recording)
    {
        odd_frame = frame % 2;
    }
    else
    {
        if (!HALFSHUTTER_PRESSED) odd_frame = (frame / (fps_get_current_x1000()/1000)) % 2;
    }
    
    if (iso_override)
    {
        FRAME_ISO = iso_override;
        lensinfo_set_iso(iso_override);
    }
    
    int t = shutter_get_timer(degrees_x10);

    if (hdr_enabled && is_movie_mode() && lens_info.raw_iso)
    {
        if (hdr_mode == 0) // ISO brack
        {
            int iso_low = COERCE(lens_info.raw_iso - (int)hdr_ev/2, 72, 120);
            int iso_high = COERCE(lens_info.raw_iso + (int)hdr_ev/2, 72, 120);
            FRAME_ISO = odd_frame ? iso_low : iso_high; // ISO 100-1600
        }
        else // Shutter brack
        {
            int ev_x8 = odd_frame ? -(int)hdr_ev/2 : (int)hdr_ev/2;
            t = shutter_get_timer(degrees_x10 * roundf(1000.0*powf(2, ev_x8 / 8.0))/1000);
        }
    }
    FRAME_SHUTTER_TIMER = t;
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
        menu_draw_icon(x, y, MNI_WARNING, "Turn off sound recording from Canon menu!");
    menu_draw_icon(x, y, MNI_BOOL(SENSOR_TIMING_TABLE != sensor_timing_table_original), 0);
    //~ bmp_hexdump(FONT_SMALL, 0, 400, SENSOR_TIMING_TABLE, 32);
}

static void
shutter_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int current_shutter = get_shutter_override_reciprocal_x1000();
    int d = get_shutter_override_degrees_x10();
    
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Tv Override : %d.%ddeg 1/%d",
        d/10, d%10,
        current_shutter/1000
    );
}

static void
iso_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Analog ISO  : %d",
        iso_override ? (100 << (iso_override/8 - 9)) : 0
    );
}

static void
hdr_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "HDR video     : %s",
        hdr_enabled ? "ON" : "OFF"
    );
    if (hdr_enabled && !lens_info.raw_iso)
        menu_draw_icon(x, y, MNI_WARNING, "HDR video won't work with Auto ISO.");
    else if (hdr_enabled && !hard_expo_override)
        menu_draw_icon(x, y, MNI_WARNING, "Enable Exposure Hack first!");
}

static void fps_change_mode(int mode, int fps)
{
    int ntsc = (mode % 2 == 0);
    /** 
     * 60fps = mode 0 - NTSC
     * 50fps = mode 1
     * 30fps = mode 2 - NTSC
     * 25fps = mode 3
     * 24fps = mode 4 - NTSC?
     **/

    int fps_x1000 = fps * 1000;

    // convert fps into timer ticks (for sensor drive speed)
    int fps_timer = ntsc ? FPS_x1000_TO_TIMER_NTSC(fps_x1000*1000/1001) : FPS_x1000_TO_TIMER_PAL(fps_x1000);

    // make sure we set a valid value (don't drive it too fast)
    int fps_timer_absolute_minimum = sensor_timing_table_original[21 + mode_offset_map[mode]];
    fps_timer = MAX(fps_timer_absolute_minimum * 120/100, fps_timer);

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
    
    // fps = 0 means "don't override, use default"
    int fps_timer_default = sensor_timing_table_original[mode_offset_map[mode]];
    sensor_timing_table_patched[mode_offset_map[mode]] = fps ? fps_timer : fps_timer_default;

    // use the patched sensor table
    SENSOR_TIMING_TABLE = sensor_timing_table_patched;
}

static void fps_change_all_modes(int fps)
{
    if (!fps)
    {
        // use the original sensor table (firmware default)
        SENSOR_TIMING_TABLE = sensor_timing_table_original;
    }
    else
    {
        // patch all video modes
        for (int i = 0; i < 2; i++)
            fps_change_mode(i, fps);
        for (int i = 2; i < 5; i++)
            fps_change_mode(i, MIN(fps, 35));
    }

    if (!lv) return;

    // flip video mode back and forth to apply settings instantly
    int f0 = video_mode[2];
    video_mode[2] = 
        f0 == 24 ? 30 : 
        f0 == 25 ? 50 : 
        f0 == 30 ? 24 : 
        f0 == 50 ? 25 :
      /*f0 == 60*/ 30;
    prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
    msleep(50);
    video_mode[2] = f0;
    prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
    msleep(50);
}

static void reset_fps(void* priv, int delta)
{
    if (recording) return;

    fps_override = 0;
    fps_change_all_modes(0);
}

static void set_fps(void* priv, int delta)
{
    if (recording) return;

    // first click won't change value
    int fps = (fps_get_current_x1000() + 500) / 1000; // rounded value
    if (fps_override) fps = COERCE(fps + delta, 4, 60);
    fps_override = 1;
    
    fps_change_all_modes(fps);
}


struct menu_entry fps_menu[] = {
    {
        .name = "FPS override", 
        .priv = &fps_override,
        .select = set_fps,
        .select_auto = reset_fps,
        .display = fps_print,
        .show_liveview = 1,
        .help = "Makes French Fries with the camera sensor. Turn off sound!"
    },
    {
        .name = "Exposure Hack", 
        .priv = &hard_expo_override,
        .max = 1,
        //~ .show_liveview = 1,
        .help = "Overrides shutter speed, ISO, allows HDR movie...",
        .children =  (struct menu_entry[]) {
            {
                .priv = &shutter_override_mode,
                .min = 1,
                .max = 13,
                .display = shutter_print,
                //~ .show_liveview = 1,
                .help = "Override shutter speed, in degrees. 1/fps ... 1/50000.",
            },
            {
                .priv = &iso_override,
                .select = iso_override_toggle,
                .display = iso_print,
                //~ .show_liveview = 1,
                .help = "Overrides the analog ISO component (100/200/400...3200).",
            },
            MENU_EOL
        },
    },
    {
        .name = "HDR video",
        .priv       = &hdr_enabled,
        .min = 0,
        .max = 1,
        .display = hdr_print,
        .help = "Alternates exposure between frames.",
        .children =  (struct menu_entry[]) {
            {
                .name = "HDR mode",
                .priv       = &hdr_mode,
                .min = 0,
                .max = 1,
                .choices = (const char *[]) {"ISO", "Shutter"},
                .help = "What setting to change for bracketing (ISO or shutter).",
            },
            {
                .name = "EV spacing",
                .priv       = &hdr_ev,
                .min = 0,
                .max = 6,
                .select = hdr_ev_toggle,
                .unit = UNIT_1_8_EV,
                .help = "Example: ISO 400 with 4 EV spacing => ISO 100/1600.",
                //~ .show_liveview = 1,
            },
            MENU_EOL
        },
    }
};

static void fps_init()
{
    // make a copy of the original sensor timing table (so we can patch it)
    sensor_timing_table_original = (void*)SENSOR_TIMING_TABLE;
    memcpy(sensor_timing_table_patched, sensor_timing_table_original,  sizeof(sensor_timing_table_patched));
    menu_add( "Debug", fps_menu, COUNT(fps_menu) );
}

INIT_FUNC("fps", fps_init);


static void fps_task()
{
    while(1)
    {
        if (lv) update_hard_expo_override();
        msleep(200);
    }
}

TASK_CREATE("fps_task", fps_task, 0, 0x1d, 0x1000 );


void fps_mvr_log(FILE* mvr_logfile)
{
    int f = fps_get_current_x1000();
    my_fprintf(mvr_logfile, "FPS: %d (%d.%03d)\n", (f+500)/1000, f/1000, f%1000);
    if (hard_expo_override)
    {
        int d = get_shutter_override_degrees_x10();
        my_fprintf(mvr_logfile, "Hard shutter override: %d.%d deg\n", d/10, d%10);
        if (iso_override)
            my_fprintf(mvr_logfile, "Hard ISO override: %d\n", 100 << (iso_override/8 - 9));
    }
    if (hdr_enabled)
        my_fprintf(mvr_logfile, "HDR: %s, %d EV\n", hdr_mode ? "Shutter" : "ISO", hdr_ev/8);
}
