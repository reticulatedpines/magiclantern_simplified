/** 
 * FPS control
 * http://magiclantern.wikia.com/wiki/VideoTimer
 * 
 * Found by g3gg0
 **/

#include "math.h"
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
#define AEWB_struct_ptr 0x1dcc
#endif
#ifdef CONFIG_60D
#define SENSOR_TIMING_TABLE MEM(0x2a668)
#define VIDEO_PARAMETERS_SRC_3 0x4FDA8
#define AEWB_struct_ptr 0x1E80
#define CARTIRIDGE_CALL_TABLE 0x26490
#endif
#ifdef CONFIG_1100D
#define SENSOR_TIMING_TABLE MEM(0xce98)
#define VIDEO_PARAMETERS_SRC_3 0x70C0C
#define CARTIRIDGE_CALL_TABLE 0x8B24
#endif

#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC))
#define FRAME_ISO (*(uint8_t*)(VIDEO_PARAMETERS_SRC_3+0x8))

#define HOOK_TABLE_FUNCTION(table,position,new,old) \
    old = (void*)((unsigned int*)table)[position];\
    ((unsigned int*)table)[position] = (unsigned int)new;\

#define UNHOOK_TABLE_FUNCTION(table,position,old) \
    ((unsigned int*)table)[position] = (unsigned int)old;\

#define IS_TABLE_FUNCTION_HOOKED(table,position,new) \
    (((unsigned int*)table)[position] == (unsigned int)new) \

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
#define TG_FREQ_NTSC_FPS 52747200
#define TG_FREQ_NTSC_SHUTTER 49440000
#define TG_FREQ_ZOOM 39230730 // not 100% sure
#define TG_FREQ_CROP_PAL 64000000

#define TG_FREQ_CROP_NTSC (crop == 0xc ? 50349600 : 69230700)
#define TG_FREQ_CROP_NTSC_SHUTTER (crop == 0xc ? 47160000 : 64860000)
#define TG_FREQ_CROP_PAL_SHUTTER (crop == 0xc ? 50000000 : 64000000)

#define TG_FREQ_FPS (zoom ? TG_FREQ_ZOOM : (crop ? (ntsc ? TG_FREQ_CROP_NTSC : TG_FREQ_CROP_PAL) : (ntsc ? TG_FREQ_NTSC_FPS : TG_FREQ_PAL)))
#define TG_FREQ_SHUTTER (zoom ? TG_FREQ_ZOOM : (crop ? (ntsc ? TG_FREQ_CROP_NTSC_SHUTTER : TG_FREQ_CROP_PAL_SHUTTER) : (ntsc ? TG_FREQ_NTSC_SHUTTER : TG_FREQ_PAL)))

#define FPS_x1000_TO_TIMER(fps_x1000) (((fps_x1000)!=0)?(TG_FREQ_FPS/(fps_x1000)):0)
#define TIMER_TO_FPS_x1000(t) (((t)!=0)?(TG_FREQ_FPS/(t)):0)

#define SHUTTER_x1000_TO_TIMER(s_x1000) (TG_FREQ_SHUTTER/(s_x1000))
#define TIMER_TO_SHUTTER_x1000(t) (TG_FREQ_SHUTTER/(t))

static uint16_t * sensor_timing_table_original = 0;
static uint16_t sensor_timing_table_patched[175*2];

static int fps_override = 0;
static CONFIG_INT("fps.override", fps_override_value, 10);
static CONFIG_INT("shutter.override", shutter_override_enabled, 0);
static CONFIG_INT("shutter.override.mode", shutter_override_mode, 0);

int is_shutter_override_enabled_movie()
{
    return shutter_override_enabled && is_movie_mode();
}

//--------------------------------------------------------
// sound recording has to be disabled
// otherwise recording is not stable
//--------------------------------------------------------
static int old_sound_recording_mode = -1;

static void restore_sound_recording()
{
    if (recording) return;
    if (old_sound_recording_mode != -1)
    {
        prop_request_change(PROP_MOVIE_SOUND_RECORD, &old_sound_recording_mode, 4);
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
        sound_recording_mode = 1;
        prop_request_change(PROP_MOVIE_SOUND_RECORD, &sound_recording_mode, 4);
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

static void shutter_set();

static int video_mode[5];
PROP_HANDLER(PROP_VIDEO_MODE)
{
    memcpy(video_mode, buf, 20);
    return prop_cleanup(token, property);
}

static const int mode_offset_map[] = { 3, 6, 1, 5, 4, 0, 2 };

static int get_table_pos(unsigned int fps_mode, unsigned int crop_mode, unsigned int type, int dispsize)
{
    unsigned short ret[2];   
    
    if(fps_mode > 6 || type > 1)
    {
        return 0;
    }

    int table_offset = 0;
    
    switch(dispsize)
    {
        case 10:
            table_offset = 2;
            fps_mode = 1;
            break;
           
        case 5:
            table_offset = 1;
            fps_mode = 1;
            break;
       
        default:
            table_offset = 0;
            break;
    }

    switch(crop_mode)
    {
        case 0:
            ret[0] = ((0 + table_offset) * 7) + fps_mode;
            ret[1] = ((3 + table_offset) * 7) + fps_mode;
            break;
            
        /* crop recording modes */

        case 0xC: // 600D 3x zoom
            ret[0] = (18 * 7) + fps_mode;
            ret[1] = (21 * 7) + fps_mode;
            break;
            
        default:  // 640 crop
            ret[0] = (10 * 7) + fps_mode;
            ret[1] = (13 * 7) + fps_mode;
            break;
    }
    
    return ret[type];
}

static int get_fps_video_mode()
{
    int mode =
        lv_dispsize > 1 || get_expsim()!=2 ? 2 :
        video_mode_fps == 60 ? 0 : 
        video_mode_fps == 50 ? 1 : 
        video_mode_fps == 30 ? 2 : 
        video_mode_fps == 25 ? 3 : 
        video_mode_fps == 24 ? 4 : 0;
    return mode;
}

int fps_get_current_x1000()
{
    int mode = get_fps_video_mode();
    int zoom = lv_dispsize > 1 ? 1 : 0;
    int ntsc = (mode % 2 == 0);
    int crop = video_mode_crop;

    unsigned int pos = get_table_pos(mode_offset_map[mode], video_mode_crop, 0, lv_dispsize);
    int fps_timer = ((uint16_t*)SENSOR_TIMING_TABLE)[pos];

    int fps_x1000 = TIMER_TO_FPS_x1000(fps_timer);
    //~ NotifyBox(5000,"fps=%d pos=%d", fps_x1000, pos);
    return fps_x1000;
}

static int shutter_get_timer(int degrees_x10)
{
    int mode = get_fps_video_mode();
    int zoom = lv_dispsize > 1 ? 1 : 0;
    int ntsc = (mode % 2 == 0);
    int crop = video_mode_crop;

    int fps_x1000 = fps_get_current_x1000();

    int timer = SHUTTER_x1000_TO_TIMER(fps_x1000);
    return MAX(1, timer * degrees_x10 / 3600);
}

static char cartridge_table[0xF4];
static void (*cartridge_AfStopPathReal)(void *this) = NULL;


static void cartridge_AfStopPath(void *this)
{
    /* force the ISO/exposure values */
    shutter_set();
    
    /* change ISO for HDR movie (will only work if HDR is enabled) */
    hdr_step();

    /* call hooked function */
    cartridge_AfStopPathReal(this);
}


// Q: what happes if this is called when Canon firmware flips the resolution?
static void update_hard_expo_override()
{
    extern int hdrv_enabled;
    if (is_shutter_override_enabled_movie() || hdrv_enabled)
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
    return MEM(CARTIRIDGE_CALL_TABLE) == (int)cartridge_table &&
        IS_TABLE_FUNCTION_HOOKED(cartridge_table, 0x39, cartridge_AfStopPath);
}

int is_hard_shutter_override_active()
{
    return is_shutter_override_enabled_movie() && is_hard_exposure_override_active();
}

int get_shutter_override_degrees_x10()
{
    // 0, 360, 270, 180, 90, 60, 30, 15...
    if (!is_shutter_override_enabled_movie()) return 0;
    if (shutter_override_mode < 4) return (4-shutter_override_mode) * 900;
    return 600 >> (shutter_override_mode - 4);
}

static int get_shutter_override_degrees()
{
    return get_shutter_override_degrees_x10() / 10;
}

static int get_shutter_override_reciprocal_x1000()
{
    int timer = shutter_get_timer(get_shutter_override_degrees_x10());

    int mode = get_fps_video_mode();
    int zoom = lv_dispsize > 1 ? 1 : 0;
    int ntsc = (mode % 2 == 0);
    int crop = video_mode_crop;

    int shutter_x1000 = TIMER_TO_SHUTTER_x1000(timer);

    return shutter_x1000;
}

int get_current_shutter_reciprocal_x1000()
{
    int timer = FRAME_SHUTTER_TIMER;

    int mode = get_fps_video_mode();
    int zoom = lv_dispsize > 1 ? 1 : 0;
    int ntsc = (mode % 2 == 0);
    int crop = video_mode_crop;

    //~ NotifyBox(1000, "%x ", timer);

    int shutter_x1000 = TIMER_TO_SHUTTER_x1000(timer);
    return MAX(shutter_x1000, fps_get_current_x1000());
}
// called every frame
static void shutter_set()
{
    int degrees_x10 = get_shutter_override_degrees_x10();
    if (degrees_x10)
    {
        int t = shutter_get_timer(degrees_x10);
        FRAME_SHUTTER_TIMER = t;
    }
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
    
    menu_draw_icon(x, y, MNI_BOOL((uint16_t *) SENSOR_TIMING_TABLE != sensor_timing_table_original), 0);
    //~ bmp_hexdump(FONT_SMALL, 0, 400, SENSOR_TIMING_TABLE, 32);
}

void
shutter_override_print(
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
        "Shutter(MOV): %d.%ddeg 1/%d",
        d/10, d%10,
        (current_shutter+500)/1000
    );

    menu_draw_icon(x, y, MNI_PERCENT, shutter_override_mode * 100 / 12);
}

void shutter_override_toggle(void* priv, int delta)
{
    shutter_override_mode = mod(shutter_override_mode + delta, 13);
}

static void fps_change_mode(int mode, int fps, int crop, int dispsize)
{
    int ntsc = (mode % 2 == 0);
    /** 
     * 60fps = mode 0 - NTSC
     * 50fps = mode 1
     * 30fps = mode 2 - NTSC
     * 25fps = mode 3
     * 24fps = mode 4 - NTSC?
     **/

    int zoom = lv_dispsize > 1 ? 1 : 0;
    int fps_x1000 = fps * 1000;

    // convert fps into timer ticks (for sensor drive speed)
    if (ntsc) fps_x1000 = fps_x1000*1000/1001;
    int fps_timer = FPS_x1000_TO_TIMER(fps_x1000);

    // make sure we set a valid value (don't drive it too fast)
    unsigned int max_pos = get_table_pos(mode_offset_map[mode], crop, 1, dispsize);
    int fps_timer_absolute_minimum = sensor_timing_table_original[max_pos];

    // NTSC is 29.97, not 30
    // also try to round it in order to avoid flicker
    if (ntsc)
    {
        int timer_120hz = FPS_x1000_TO_TIMER(120000*1000/1001);
        if(timer_120hz > 0)
        {
            int fps_timer_rounded = ((fps_timer + timer_120hz/2) / timer_120hz) * timer_120hz;
            if (ABS(TIMER_TO_FPS_x1000(fps_timer_rounded) - fps_x1000 + 1) < 500) fps_timer = fps_timer_rounded;
        }
    }
    else
    {
        int timer_100hz = FPS_x1000_TO_TIMER(100000);
        if(timer_100hz > 0)
        {
            int fps_timer_rounded = ((fps_timer + timer_100hz/2) / timer_100hz) * timer_100hz;
            if (ABS(TIMER_TO_FPS_x1000(fps_timer_rounded) - fps_x1000 + 1) < 500) fps_timer = fps_timer_rounded;
        }
    }

    fps_timer = MAX(fps_timer_absolute_minimum * 105/100, fps_timer);
    fps_timer = MIN(16383, fps_timer);
    
    unsigned int pos = get_table_pos(mode_offset_map[mode], crop, 0, dispsize);
    
    // fps = 0 means "don't override, use default"
    int fps_timer_default = sensor_timing_table_original[pos];
    sensor_timing_table_patched[pos] = fps ? fps_timer : fps_timer_default;

    // use the patched sensor table
    SENSOR_TIMING_TABLE = (intptr_t) sensor_timing_table_patched;
}

static void fps_change_all_modes(int fps)
{
    if (!fps)
    {
        // use the original sensor table (firmware default)
        SENSOR_TIMING_TABLE = (intptr_t) sensor_timing_table_original;
    }
    else
    {
        // patch all video modes
        for (int i = 0; i < 2; i++) // 60p, 50p
        {
            fps_change_mode(i, fps, 0, 1);
            fps_change_mode(i, fps, 0, 5);
            fps_change_mode(i, fps, 0, 10);
            fps_change_mode(i, fps, 8, 1); // 640 crop
        }
        for (int i = 2; i < 5; i++) // 30p, 25p, 24p
        {
            fps_change_mode(i, fps, 0, 1);
            fps_change_mode(i, fps, 0, 5);
            fps_change_mode(i, fps, 0, 10);
            #ifdef CONFIG_600D
            fps_change_mode(i, fps, 0xC, 1); // 3x crop
            #endif
        }
    }

    if (!lv) return;

    // flip video mode back and forth to apply settings instantly
    int f0 = video_mode[2];
    video_mode[2] = 
        f0 == 24 ? 25 : 
        f0 == 25 ? 24 : 
        f0 == 30 ? 25 : 
        f0 == 50 ? 60 :
      /*f0 == 60*/ 50;
    prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
    msleep(50);
    video_mode[2] = f0;
    prop_request_change(PROP_VIDEO_MODE, video_mode, 20);
    msleep(50);
}

static void fps_change_value(void* priv, int delta)
{
    if (recording) return;

    fps_override_value = COERCE(fps_override_value + delta, 3, 70);
    if (fps_override) fps_change_all_modes(fps_override_value);
}

static void fps_enable_disable(void* priv, int delta)
{
    if (recording) return;

    fps_override = !fps_override;
    if (fps_override) fps_change_all_modes(fps_override_value);
    else fps_change_all_modes(0);
}


struct menu_entry fps_menu[] = {
    {
        .name = "FPS override", 
        .priv = &fps_override,
        .select = fps_enable_disable,
        .display = fps_print,
        .help = "Changes frame rate. Also disables sound recording.",
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
    // make a copy of the original sensor timing table (so we can patch it)
    sensor_timing_table_original = (void*)SENSOR_TIMING_TABLE;
    memcpy(sensor_timing_table_patched, sensor_timing_table_original,  sizeof(sensor_timing_table_patched));
    menu_add( "Movie", fps_menu, COUNT(fps_menu) );
}

INIT_FUNC("fps", fps_init);


static void fps_task()
{
    TASK_LOOP
    {
        if (lv)
        {
            update_hard_expo_override();
            update_sound_recording();
        }
        msleep(200);
    }
}

TASK_CREATE("fps_task", fps_task, 0, 0x1d, 0x1000 );


void fps_mvr_log(FILE* mvr_logfile)
{
    int f = fps_get_current_x1000();
    my_fprintf(mvr_logfile, "FPS override   : %d (%d.%03d)\n", (f+500)/1000, f/1000, f%1000);
    if (is_shutter_override_enabled_movie())
    {
        int d = get_shutter_override_degrees_x10();
        my_fprintf(mvr_logfile, "Tv override    : %d.%d deg\n", d/10, d%10);
    }
}
