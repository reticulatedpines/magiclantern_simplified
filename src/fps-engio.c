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
 *      FPS_REGISTER_B, 0xFFFF, // timer register
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

#define FPS_REGISTER_A 0xC0F06008
#define FPS_REGISTER_B 0xC0F06014

#define FPS_REGISTER_A_VALUE shamem_read(FPS_REGISTER_A)
#define FPS_REGISTER_B_VALUE shamem_read(FPS_REGISTER_B)

static int fps_reg_a_orig = 0;
static int fps_reg_b_orig = 0;

static int fps_timer_a;        // C0F06008
static int fps_timer_a_orig;
static int fps_timer_b;        // C0F06014
static int fps_timer_b_orig; 


static int fps_values_x1000[] = {150, 200, 250, 333, 400, 500, 750, 1000, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 12000, 12500, 15000, 17000, 20000, 24000, 25000, 26000, 27000, 28000, 29000, 30000, 35000, 40000, 48000, 50000, 60000, 65000};

static CONFIG_INT("fps.override", fps_override, 0);
static CONFIG_INT("fps.override.idx", fps_override_index, 10);

// 1000 = zero, more is positive, less is negative
static CONFIG_INT("fps.timer.a.off", desired_fps_timer_a_offset, 1000); // add this to default Canon value
static CONFIG_INT("fps.timer.b.off", desired_fps_timer_b_offset, 1000); // add this to computed value (for fine tuning)

static CONFIG_INT("fps.preset", fps_criteria, 0);

static CONFIG_INT("fps.sound.disable", fps_sound_disable, 1);

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

#define FPS_TIMER_A_MAX 0x2000
#define FPS_TIMER_B_MAX (0x4000-1)
//~ #define FPS_TIMER_B_MIN (fps_timer_b_orig-100)
#define FPS_TIMER_B_MIN fps_timer_b_orig // it might go lower than that, but it causes trouble high shutter speeds

#ifdef CONFIG_5D2
    #define TG_FREQ_BASE 24000000
    #define TG_FREQ_SHUTTER (ntsc ? 39300000 : 40000000)
    #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig, lv_dispsize > 1 ? 0x262 : 0x228) // trial and error (with digic poke)
#else
    #ifdef CONFIG_500D
        #define TG_FREQ_BASE 32000000    // not 100% sure
        #define TG_FREQ_SHUTTER 23188405 // not sure
        #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig, lv_dispsize > 1 ? 1400 : video_mode_resolution == 0 ? 1284 : 1348)
    #else
        // 550D, 600D, 60D, 50D 
        #define TG_FREQ_BASE 28800000
        #ifdef CONFIG_50D
            #define TG_FREQ_SHUTTER 41379310 // not sure
            #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig, lv_dispsize > 1 ? 630 : 688 )
        #else
            #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig, lv_dispsize > 1 ? 734 : video_mode_crop ? 400 : 0x21A)
            #define TG_FREQ_PAL  50000000
            #define TG_FREQ_NTSC_FPS 52747200
            #define TG_FREQ_NTSC_SHUTTER 49440000
            #define TG_FREQ_ZOOM 39230730 // not 100% sure
            #define TG_FREQ_CROP_PAL 64000000

            #define TG_FREQ_CROP_NTSC (crop == 0xc ? 50349600 : 69230700)
            #define TG_FREQ_CROP_NTSC_SHUTTER (crop == 0xc ? 47160000 : 64860000)
            #define TG_FREQ_CROP_PAL_SHUTTER (crop == 0xc ? 50000000 : 64000000)

            #define TG_FREQ_SHUTTER (zoom ? TG_FREQ_ZOOM : (crop ? (ntsc ? TG_FREQ_CROP_NTSC_SHUTTER : TG_FREQ_CROP_PAL_SHUTTER) : (ntsc ? TG_FREQ_NTSC_SHUTTER : TG_FREQ_PAL)))

        #endif
    #endif
#endif

static int calc_tg_freq(int timerA)
{
    int f = (TG_FREQ_BASE / timerA) * 1000 + mod(TG_FREQ_BASE, timerA) * 1000 / timerA;
    return f;
}

static int calc_fps_x1000(int timerA, int timerB)
{
    int f = calc_tg_freq(timerA);
    return f / timerB;
}

static int get_current_tg_freq()
{
    int timerA = (FPS_REGISTER_A_VALUE & 0xFFFF) + 1;
    if (timerA == 1) return 0;
    int f = calc_tg_freq(timerA);
    return f;
}

#define TG_FREQ_FPS get_current_tg_freq()

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

#ifdef CONFIG_1100D
#define VIDEO_PARAMETERS_SRC_3 0x70C0C
#define FRAME_SHUTTER_TIMER (*(uint16_t*)(VIDEO_PARAMETERS_SRC_3+0xC)) // not sure
#endif

#ifdef CONFIG_500D
#define LV_STRUCT_PTR 0x1d78
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x58)
#endif

#ifdef CONFIG_50D
#define LV_STRUCT_PTR 0x1D74
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x5c)
#endif

#ifdef CONFIG_5D2
#define LV_STRUCT_PTR 0x1D78
#define FRAME_SHUTTER_TIMER *(uint16_t*)(MEM(LV_STRUCT_PTR) + 0x60)
#endif

#define FPS_x1000_TO_TIMER(fps_x1000) (((fps_x1000)!=0)?(TG_FREQ_FPS/(fps_x1000)):0)
#define TIMER_TO_FPS_x1000(t) (((t)!=0)?(TG_FREQ_FPS/(t)):0)

#define SHUTTER_x1000_TO_TIMER(s_x1000) (TG_FREQ_SHUTTER/(s_x1000))
#define TIMER_TO_SHUTTER_x1000(t) (TG_FREQ_SHUTTER/(t))

/*static void fps_change_timer_a(int new_value)
{
    int new_timer_a = COERCE(new_value, FPS_TIMER_A_MIN, FPS_TIMER_A_MAX) & 0xFFFE;
    new_timer_a |= (fps_timer_a_orig & 1);
    desired_fps_timer_a_offset = new_timer_a - fps_timer_a_orig + 1000;
}*/

static int get_shutter_reciprocal_x1000(int shutter_r_x1000, int Ta, int Ta0, int Tb)
{
    int shutter_us = 1000000000 / shutter_r_x1000;
    int fps_timer_delta_us = 1000000000 / ((TG_FREQ_BASE / Ta0) * 1000 / Tb) - 1000000 / video_mode_fps;
    int ans_raw = 1000000000 / (shutter_us + fps_timer_delta_us);
    int ans = ans_raw * (Ta0/10) / (Ta/10);
    //~ NotifyBox(2000, "su=%d cfc=%d \nvf=%d td=%d \nd_num=%d d_den=%d\nar=%d ans=%d", shutter_us, ((TG_FREQ_BASE / Ta0) * 1000 / Tb), video_mode_fps, fps_timer_delta_us, Ta, Ta0, ans_raw, ans);
    
    return ans;
}

int get_current_shutter_reciprocal_x1000()
{
#if defined(CONFIG_500D) || defined(CONFIG_50D)// || defined(CONFIG_5D2)
    if (!lens_info.raw_shutter) return 0;
    return (int) roundf(powf(2.0, (lens_info.raw_shutter - 136) / 8.0) * 1000.0 * 1000.0);
#else

    int timer = FRAME_SHUTTER_TIMER;
    int zoom = lv_dispsize > 1 ? 1 : 0;
    int ntsc = is_current_mode_ntsc();
    int crop = video_mode_crop;

    int shutter_r_x1000 = TIMER_TO_SHUTTER_x1000(timer);
    
    // shutter speed can't be slower than 1/fps
    shutter_r_x1000 = MAX(shutter_r_x1000, fps_get_current_x1000());
    
    // FPS override will alter shutter speed (exposure time)
    // FPS "difference" from C0F06014 will be added as a constant term to exposure time
    // FPS factor from C0F06008 will multiply the exposure time (as scalar gain)
    
    // TG = base timer (28.8 MHz on most cams)
    // Ta = current value from C0F06008
    // Tb = current value from C0F06014
    // Ta0, Tb0 = original values
    //
    // FC = current fps = TG / Ta / Tb
    // F0 = factory fps = TG / Ta0 / Tb0
    //
    // E0 = exposure time (shutter speed) as indicated by Canon user interface
    // EA = actual exposure time, after FPS modifications (usually higher)
    //
    // If we only change Tb => Fb = TG / Ta0 / Tb
    //
    // delta_fps = 1/Fb - 1/F0 => this quantity is added to exposure time
    //
    // If we only change Ta => exposure time is multiplied by Ta/Ta0.
    //
    // If we change both, Tb "effect" is applied first, then Ta.
    // 
    // So...
    // EA = (E0 + (1/Fb - 1/F0)) * Ta / Ta0
    //
    // This function returns 1/EA and does all calculations on integer numbers, so actual computations differ slightly.
    
    return get_shutter_reciprocal_x1000(shutter_r_x1000, fps_timer_a, fps_timer_a_orig, fps_timer_b);
#endif
}


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
    if (fps_sound_disable && fps_override && lv) disable_sound_recording();
    else restore_sound_recording();
}

PROP_HANDLER(PROP_LV_ACTION)
{
    restore_sound_recording();
    return prop_cleanup(token, property);
}
//--------------------------------------------------------

static int fps_get_timer(int fps_x1000)
{
    // convert fps into timer ticks (for sensor drive speed)
    int ntsc = is_current_mode_ntsc();
    int fps_timer = FPS_x1000_TO_TIMER(fps_x1000);

    #if defined(CONFIG_500D) || defined(CONFIG_50D) // these cameras use 30.000 fps, not 29.97 => look in system settings to check if PAL or NTSC
    ntsc = !pal;
    #endif

    if (fps_criteria != 1) // if criteria is "exact FPS", don't round
    {
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
    }

    return fps_timer & 0xFFFF;
}

// used to see if Canon firmware changed FPS settings
int written_value_a = 0;
int written_value_b = 0;
int fps_needs_updating = 0;
int fps_was_changed_by_canon() 
{ 
    return 
        written_value_a != FPS_REGISTER_A_VALUE || 
        written_value_b != FPS_REGISTER_B_VALUE;
}

static void fps_setup_timerB(int fps_x1000)
{
    if (!lv) return;
    if (!DISPLAY_IS_ON) return;
    if (!fps_x1000) return;

    // now we can compute timer B
    int timerB_off = ((int)desired_fps_timer_b_offset) - 1000;
    int timerB = 0;
    timerB = fps_get_timer(fps_x1000);
    
    // check hard limits
    timerB = COERCE(timerB, FPS_TIMER_B_MIN, FPS_TIMER_B_MAX);
    
    // apply user fine-tuning offset
    timerB += timerB_off;

    // check hard limits again
    timerB = COERCE(timerB, FPS_TIMER_B_MIN, FPS_TIMER_B_MAX);
    
    // output the value to register
    timerB -= 1;
    written_value_b = timerB;
    EngDrvOut(FPS_REGISTER_B, timerB);

    // apply changes
    EngDrvOut(0xC0F06000, 1);
    
    fps_needs_updating = 0;

    // take care of sound settings to prevent recording from stopping
    update_sound_recording();
}

int fps_get_current_x1000()
{
    int fps_timer = FPS_REGISTER_B_VALUE + 1;
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
    snprintf(msg, sizeof(msg), "%d.%03d", 
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

static void
fps_current_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int current_fps = fps_get_current_x1000();
    
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Actual FPS   : %d.%03d",
        current_fps/1000, current_fps%1000
    );
    
    menu_draw_icon(x, y, MNI_BOOL(fps_override), 0);
}

static void
desired_fps_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int desired_fps = fps_values_x1000[fps_override_index] / 10;

    if (desired_fps % 100)
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Desired FPS  : %d.%02d (from %d)",
            desired_fps/100, desired_fps%100, video_mode_fps
        );
    else
        bmp_printf(
            selected ? MENU_FONT_SEL : MENU_FONT,
            x, y,
            "Desired FPS  : %d (from %d)",
            desired_fps/100, video_mode_fps
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

static void fps_register_reset()
{
    if (fps_reg_a_orig && fps_reg_b_orig)
    {
        EngDrvOut(FPS_REGISTER_A, fps_reg_a_orig);
        EngDrvOut(FPS_REGISTER_B, fps_reg_b_orig);
        EngDrvOut(0xC0F06000, 1);
    }
}

static void fps_reset()
{
    fps_override = 0;
    fps_register_reset();
    fps_needs_updating = 0;

    restore_sound_recording();
}


static void fps_change_value(void* priv, int delta)
{
    fps_override_index = mod(fps_override_index + delta, COUNT(fps_values_x1000));
    desired_fps_timer_a_offset = 1000;
    desired_fps_timer_b_offset = 1000;
    fps_needs_updating = 1;
}

static void fps_enable_disable(void* priv, int delta)
{
    fps_override = !fps_override;
    fps_needs_updating = 1;
}

static int find_fps_index(int fps_x1000)
{
    for (int i = 0; i < COUNT(fps_values_x1000); i++)
    {
        if (fps_values_x1000[i] >= fps_x1000)
            return i;
    }
    return -1;
}


void fps_range_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    int fps_low = calc_fps_x1000(fps_timer_a, FPS_TIMER_B_MAX);
    int fps_high = calc_fps_x1000(fps_timer_a, FPS_TIMER_B_MIN);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "FPS range    : %d.%03d..%d.%03d",
        fps_low  / 1000, fps_low  % 1000, 
        fps_high / 1000, fps_high % 1000
    );
    menu_draw_icon(x, y, MNI_BOOL(fps_override), 0);
}

void shutter_range_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    // EA = (E0 + (1/Fb - 1/F0)) * Ta / Ta0
    // see get_current_shutter_reciprocal_x1000 for details
    
    int shutter_r_0_lo_x1000 = video_mode_fps * 1000;
    int shutter_r_0_hi_x1000 = 4000*1000;
    int tv_low = get_shutter_reciprocal_x1000(shutter_r_0_lo_x1000, fps_timer_a, fps_timer_a_orig, fps_timer_b);
    int tv_high = get_shutter_reciprocal_x1000(shutter_r_0_hi_x1000, fps_timer_a, fps_timer_a_orig, fps_timer_b);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Shutter range: 1/%d..1/%d",
        (tv_low+500)/1000, (tv_high+500)/1000
    );
    menu_draw_icon(x, y, MNI_BOOL(fps_override), 0);
}


static void fps_timer_print(
    void *      priv,
    int         x,
    int         y,
    int         selected
)
{
    int A = (priv == &desired_fps_timer_a_offset);
    int t = A ? fps_timer_a : fps_timer_b;
    int t0 = A ? fps_timer_a_orig : fps_timer_b_orig; 
    if (t0 == 0) t0 = 1;
    int t_min = A ? FPS_TIMER_A_MIN : FPS_TIMER_B_MIN;
    int t_max = A ? FPS_TIMER_A_MAX : FPS_TIMER_B_MAX;
    int finetune_delta = ((int)(A ? desired_fps_timer_a_offset : desired_fps_timer_b_offset)) - 1000;
    int delta = t - t0;
    char dec[10] = "";
    if (!finetune_delta && delta >= 100) 
        snprintf(dec, sizeof(dec), ".%02d", ((t * 100 / t0) % 100));
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "FPS timer %s  : %d (%s%d%s)",
        A ? "A" : "B",
        t, 
        finetune_delta > 0 ? "FT+" : finetune_delta < 0 ? "FT" : delta >= 100 ? "x" : delta >= 0 ? "+" : "", 
        finetune_delta ? finetune_delta : delta >= 100 ? t / t0 : delta, 
        dec
    );
    if (!fps_override)
        menu_draw_icon(x, y, MNI_OFF, 0);
    else if (t_max <= t_min)
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t)"Internal error - please report an issue.");
    else
        menu_draw_icon(x, y, MNI_PERCENT, sqrt(t - t_min) * 100  / sqrt(t_max - t_min));
}

static void tg_freq_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Main clock   : %d.%02d MHz",
        TG_FREQ_BASE / 1000000, (TG_FREQ_BASE % 1000000) / 10000
    );
    menu_draw_icon(x, y, MNI_BOOL(fps_override), 0);
}

/*
static void fps_timer_a_big_change(void* priv, int delta)
{
    int tmin = FPS_TIMER_A_MIN; // map this to -1
    int t0 = fps_timer_a_orig;  // this is the reference point: k=0
    int tmax = FPS_TIMER_A_MAX; // map this to k=20
    int k = ((fps_timer_a - t0) * 20 + (tmax - t0) / 2) / (tmax - t0);
    if (fps_timer_a < t0) k = -1;
    
    k += delta;
    
    fps_change_timer_a(t0 + k * (tmax - t0) / 20);
    fps_needs_updating = 1;
}*/

static void fps_timer_fine_tune_a(void* priv, int delta)
{
    desired_fps_timer_a_offset += delta * 2;
    fps_needs_updating = 1;
}

static void fps_timer_fine_tune_a_big(void* priv, int delta)
{
    desired_fps_timer_a_offset += delta * 20;
    fps_needs_updating = 1;
}

static void fps_timer_fine_tune_b(void* priv, int delta)
{
    desired_fps_timer_b_offset += delta;
    fps_needs_updating = 1;
}


// dumb heuristic :)
// returns value of timer A
static int fps_try_to_get_exact_freq(int fps_x1000)
{
    int min_err = 1000;
    int best_t = 0;
    for (int t = FPS_TIMER_A_MIN; t < FPS_TIMER_A_MAX; t+= 2)
    {
        t = (t & 0xFFFE) | (fps_timer_a_orig & 1); // keep the same parity as factory value
        int tb = calc_tg_freq(t) / fps_x1000;
        if (tb < FPS_TIMER_B_MIN || tb > FPS_TIMER_B_MAX) continue;
        int actual_fps = calc_fps_x1000(t, tb);
        int e = abs(fps_x1000 - actual_fps);
        if (e < min_err)
        {
            min_err = e;
            best_t = t;
        }
    }
    return best_t;
}

int fps_try_to_get_180_360_shutter(int fps_x1000)
{
    // EAtarget = (E0 + (1/Fb - 1/F0)) * Ta / Ta0 => solve for Ta
    // Fb = TG / Ta0 / Tb
    // Tb = TG / Ta / FPS
    // 180 degree => EAtarget = 0.5/FPS
    // and we choose E0 at 1/4000 to get 180 degrees when Canon shutter speed is set to that value
    // => (symbolic math solver)
    // Ta = 2000 * Ta0 * TG / FPS / (4000 * Ta0 * Tb0 - TG)
    // 
    // approx: TG / FPS / 2 / Tb0 if we assume 4000*Ta0*Tb0 >> TG
    // correction factor: (4000 * Ta0 * Tb0) / (4000 * Ta0 * Tb0 - TG)
    // approx correction factor: (Ta0 * Tb0) / (Ta0 * Tb0 - TG/4000)
    
    int Ta0 = fps_timer_a_orig;
    int Tb0 = fps_timer_b_orig;
    int Ta_approx = TG_FREQ_BASE / fps_x1000 * 1000 / 2 / Tb0;
    int Ta_corrected = Ta_approx * (Ta0*Tb0 / 100) / (Ta0*Tb0/100 - TG_FREQ_BASE/4000/100);
    return Ta_corrected;
}

static void fps_setup_timerA(int fps_x1000)
{
    // for NTSC, we probably need FPS * 1000/1001
    int ntsc = is_current_mode_ntsc();
    if (fps_criteria == 1) ntsc = 0; // use PAL-like rounding [hack]
    #if !defined(CONFIG_500D) && !defined(CONFIG_50D) // these cameras use 30.000 fps, not 29.97
    if (ntsc) fps_x1000 = fps_x1000 * 1000/1001;
    #endif

    int timerA = fps_timer_a_orig;
    // {"Low light", "Exact FPS", "180deg shutter", "Jello effect"},
    switch (fps_criteria)
    {
        case 0:
            // if we leave timer A at default value, 
            // or we change it as little as possible (just to bring requested FPS in range),
            // we get best low light capability and lowest amount of jello effect.
            timerA = fps_timer_a_orig;
            break;
        case 1:
            timerA = fps_try_to_get_exact_freq(fps_x1000);
            break;
        case 2:
            timerA = fps_try_to_get_180_360_shutter(fps_x1000);
            break;
        case 3:
            timerA = TG_FREQ_BASE / fps_x1000 * 1000 / fps_timer_b_orig;
            break;
    }

    // we need to make sure the requested FPS is in range (we may need to change timer A)
    int fps_low = calc_fps_x1000(timerA, FPS_TIMER_B_MAX);
    int fps_high = calc_fps_x1000(timerA, FPS_TIMER_B_MIN);
    
    if (fps_x1000 < fps_low)
    {
        timerA = TG_FREQ_BASE / fps_x1000 * 1000 / FPS_TIMER_B_MAX;
    }
    else if (fps_x1000 > fps_high)
    {
        timerA = TG_FREQ_BASE / fps_x1000 * 1000 / FPS_TIMER_B_MIN;
    }

    // check hard limits
    timerA = COERCE(timerA, FPS_TIMER_A_MIN, FPS_TIMER_A_MAX);
    
    // apply user fine tuning
    int timerA_off = ((int)desired_fps_timer_a_offset) - 1000;
    timerA += timerA_off;

    // check hard limits again
    timerA = COERCE(timerA, FPS_TIMER_A_MIN, FPS_TIMER_A_MAX);

    // save setting to DIGIC register
    int val_a = ((timerA-1) & 0x0000FFFE) | (FPS_REGISTER_A_VALUE & 0xFFFF0001);
    written_value_a = val_a;
    EngDrvOut(FPS_REGISTER_A, val_a);
}

static void fps_criteria_change(void* priv, int delta)
{
    desired_fps_timer_a_offset = 1000;
    desired_fps_timer_b_offset = 1000;
    fps_criteria = mod(fps_criteria + delta, 4);
    fps_needs_updating = 1;
}

static void fps_sound_toggle(void* priv, int delta)
{
    fps_sound_disable = !fps_sound_disable;
    fps_needs_updating = 1;
}


static struct menu_entry fps_menu[] = {
    {
        .name = "FPS override", 
        .priv = &fps_override,
        .select = fps_enable_disable,
        .display = fps_print,
        .help = "Changes FPS. Also disables sound and alters shutter speeds.",
        .children =  (struct menu_entry[]) {
/*            {
                .name = "Preset\b",
                .priv       = &fps_criteria,
                .choices = (const char *[]) {"Custom", "24.000fps", "FPS/2 night", "10p jello", "5p jello", "2p jello", "2p 360deg", "1p 360deg", "0.5p 360deg", "0.25p 360deg"},
                .icon_type = IT_BOOL,
                .select = fps_criteria_change,
                .help = "FPS presets - a few useful combinations.",
            },*/
            {
                .priv    = &fps_override_index,
                .display = desired_fps_print,
                .min = 0,
                .max = COUNT(fps_values_x1000) - 1,
                .select = fps_change_value,
                .help = "FPS value for recording. Video will play back at Canon FPS.",
            },
            {
                .name = "Optimize for\b",
                .priv       = &fps_criteria,
                .choices = (const char *[]) {"Low FPS, 360d", "Exact FPS", "LowJello, 180d", "High Jello"},
                .icon_type = IT_DICE,
                .max = 3,
                .select = fps_criteria_change,
                .help = "Optimization criteria - how to setup the two timers.",
            },
            /*{
                .display = fps_range_print,
                //~ .select = fps_timer_a_big_change,
                .help = "FPS range. Changing this will change FPS timer A.",
            },*/
            {
                .display = shutter_range_print,
                //~ .select = fps_timer_a_big_change,
                .select = fps_timer_fine_tune_a_big,
                .help = "Shutter speed range with current settings. Adjusts timer A.",
            },
            {
                //~ .name = "FPS timer A",
                .display = fps_timer_print,
                .priv = &desired_fps_timer_a_offset,
                .select = fps_timer_fine_tune_a,
                .help = "High values = lower FPS, more jello effect, faster shutter.",
            },
            {
                .name = "FPS timer B",
                .display = fps_timer_print,
                .priv = &desired_fps_timer_b_offset,
                .select = fps_timer_fine_tune_b,
                .help = "High values = lower FPS, shutter speed converges to 1/fps.",
            },
            {
                .name = "TG frequency",
                .display = tg_freq_print,
                .help = "Timing generator freq. (read-only). FPS = F/timerA/timerB.",
            },
            {
                .name = "Actual FPS",
                .display = fps_current_print,
                .help = "Exact FPS (computed). For fine tuning, change timer values.",
            },
            {
                .name = "Sound Record\b",
                .priv = &fps_sound_disable,
                .select = fps_sound_toggle,
                .choices = (const char *[]) {"Leave it on", "Auto-Disable"},
                .icon_type = IT_DISABLE_SOME_FEATURE,
                .help = "Sound usually goes out of sync and may stop recording.",
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

static void fps_read_current_timer_values()
{
    int VA = FPS_REGISTER_A_VALUE;
    int VB = FPS_REGISTER_B_VALUE;
    fps_timer_a = (VA & 0xFFFF) + 1;
    fps_timer_b = (VB & 0xFFFF) + 1;

    // this contains original timer value - we won't change it
    // so we can read it here too, no problem
    fps_timer_a_orig = ((VA >> 16) & 0xFFFF) + 1;
}

static void fps_read_default_timer_values()
{
    fps_reg_a_orig = FPS_REGISTER_A_VALUE;
    fps_reg_b_orig = FPS_REGISTER_B_VALUE;
    fps_timer_a_orig = ((fps_reg_a_orig >> 16) & 0xFFFF) + 1;
    fps_timer_b_orig = (fps_reg_b_orig & 0xFFFF) + 1;
}

// do all FPS changes from this task only - to avoid trouble ;)
static void fps_task()
{
    while(1)
    {
        msleep(50);

        if (!lv) continue;
        if (!DISPLAY_IS_ON) continue;
        
        fps_read_current_timer_values();
        
        if (!fps_override) 
        {
            if (fps_needs_updating)
                fps_reset();

            msleep(200);

            fps_read_default_timer_values();
                            
            continue;
        }
        
        if (fps_was_changed_by_canon())
        {
            msleep(50);
            fps_read_default_timer_values();
        }
        
        if (fps_needs_updating || fps_was_changed_by_canon())
        {
            msleep(200);

            int f = fps_values_x1000[fps_override_index];
            
            // Very low FPS: first few frames will be recorded at normal FPS, to bypass Canon's internal checks
            if (f < 5000)
                while (recording && MVR_FRAME_NUMBER < video_mode_fps) 
                    msleep(MIN_MSLEEP);
            
            fps_setup_timerA(f);
            fps_setup_timerB(f);
        }
    }
}

TASK_CREATE("fps_task", fps_task, 0, 0x1c, 0x1000 );


void fps_mvr_log(FILE* mvr_logfile)
{
    int f = fps_get_current_x1000();
    my_fprintf(mvr_logfile, "FPS            : %d.%03d\n", f/1000, f%1000);
}

// on certain events (PLAY, RECORD) we need to disable FPS override temporarily
int handle_fps_events(struct event * event)
{
    if (event->param == BGMT_PLAY || 
    #if defined(CONFIG_50D) || defined(CONFIG_5D2)
        event->param == BGMT_PRESS_SET
    #else
        event->param == BGMT_LV
    #endif
    )
    {
        fps_register_reset();
    }
    
    return 1;
}


