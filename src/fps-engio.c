/** 
 * FPS control with engio calls (talking to DIGIC!)
 * This method is portable: works on all cameras.
 * 
 * https://docs.google.com/spreadsheet/ccc?key=0AgQ2MOkAZTFHdEZrXzBSZmdaSE9WVnpOblJ2ZGtoZXc#gid=0
 * 
 **/
 
/**
 * fps_timer_b_method
 * Notes by g3gg0:
 * 
 * okay i found how to directly change the sensor frame rate without patching and copying memory areas.
 * it doesnt matter which mode is selected.
 * 
 * on 600D v1.0.1 it is calling engio_write() with a buffer that writes the rate.
 * 
 * unsigned long frame_rate[] = {
 *      FPS_REGISTER_B, 0xFFFF, // timer register
 *      FPS_REGISTER_CONFIRM_CHANGES, 0x01,   // coherent update
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
#include "math.h"
#include "raw.h"
#include "fps.h"
#include "shoot.h"


#define FPS_REGISTER_A 0xC0F06008
#define FPS_REGISTER_B 0xC0F06014
#define FPS_REGISTER_CONFIRM_CHANGES 0xC0F06000

#define PACK(lo, hi) ((lo) & 0x0000FFFF) | (((hi) & 0x0000FFFF) << 16)

#define FPS_REGISTER_A_VALUE ((int) shamem_read(FPS_REGISTER_A))
#define FPS_REGISTER_A_DEFAULT_VALUE ((int) shamem_read(FPS_REGISTER_A+4))
#define FPS_REGISTER_B_VALUE ((int) shamem_read(FPS_REGISTER_B))

#ifdef CONFIG_7D
uint32_t *buf = NULL;
uint32_t QuickOutIPCTransfer(int type, uint32_t *buffer, int length, uint32_t master_addr, void (*cb)(uint32_t*, uint32_t, uint32_t), volatile uint32_t* cb_parm);

void fps_bulk_cb(uint32_t *parm, uint32_t address, uint32_t length)
{
    *parm = 0;
}
#endif

void EngDrvOutLV(uint32_t reg, uint32_t val)
{
    if (!lv) return;
    if (!DISPLAY_IS_ON && NOT_RECORDING) return;
    if (lens_info.job_state) return;
    if (ml_shutdown_requested) return;

#if defined(CONFIG_7D)
    if (reg == FPS_REGISTER_A || reg == FPS_REGISTER_B || reg == FPS_REGISTER_CONFIRM_CHANGES)
    {
        volatile uint32_t wait = 1;
        memcpy(buf, &val, sizeof(uint32_t));
        QuickOutIPCTransfer(0, buf, sizeof(uint32_t), reg, &fps_bulk_cb, &wait);
        
        while(wait)
        {
            msleep(10);
        }
    }
#endif
    
    _EngDrvOut(reg, val);
}

static void fps_set_timers_from_evfstate(int timerA, int timerB, int wait);
static void fps_disable_timers_evfstate();

static void EngDrvOutFPS(uint32_t reg, uint32_t val)
{
    #ifdef CONFIG_FPS_UPDATE_FROM_EVF_STATE
    // some cameras seem to prefer changing FPS registers from EVF (LiveView) task
    static int a;
    static int b;
    if (reg == FPS_REGISTER_A)
    {
        EngDrvOutLV(reg, val);
        a = val;
    }
    else if (reg == FPS_REGISTER_B)
    {
        EngDrvOutLV(reg, val);
        b = val;
    }
    else if (reg == FPS_REGISTER_CONFIRM_CHANGES) 
    {
        fps_set_timers_from_evfstate(a, b, 1);
    }
    #else
    EngDrvOutLV(reg, val);
    #endif
}

static int fps_reg_a_orig = 0;
static int fps_reg_b_orig = 0;

static int fps_timer_a;        // C0F06008
static int fps_timer_a_orig;
static int fps_timer_b;        // C0F06014
static int fps_timer_b_orig; 

static int fps_values_x1000[] = {
    150, 200, 250, 333, 400, 500, 750, 1000, 1500, 2000, 2500, 3000, 4000,
    5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000, 12500, 14000, 15000, 16000,
    17000, 18000, 19000, 20000, 21000, 22000, 23000, 23976, 24000, 25000, 26000, 27000,
    28000, 29000, 29970, 30000, 31000, 32000, 33000, 33333, 34000, 35000
    // restrict max fps to 35 for 1100D, 5D2, 50D, 500D (others?)
    #if !defined(CONFIG_1100D) && !defined(CONFIG_5D2) && !defined(CONFIG_50D) && !defined(CONFIG_500D)
    , 37000, 38000, 39000, 40000, 41000, 42000, 43000, 44000, 45000, 48000, 50000, 60000, 65000, 70000
    #endif
};

static CONFIG_INT("fps.override", fps_override, 0);

static inline int get_fps_override()
{
#ifdef FEATURE_FPS_OVERRIDE
    #ifdef CONFIG_7D
    /* on 7D, FPS override can be used only for RAW and in photo mode */
    return fps_override && (!is_movie_mode() || raw_lv_is_enabled());
    #else
    return fps_override;
    #endif
#else
    return 0;
#endif
}

static CONFIG_INT("fps.override.idx", fps_override_index, 10);

// 1000 = zero, more is positive, less is negative
static CONFIG_INT("fps.timerA.off", desired_fps_timer_a_offset, 0); // add this to default Canon value
static CONFIG_INT("fps.timerB.off", desired_fps_timer_b_offset, 0); // add this to computed value (for fine tuning)
static CONFIG_INT("fps.preset", fps_criteria, 0);
static CONFIG_INT("fps.wav.record", fps_wav_record, 0);

static CONFIG_INT("fps.const.expo", fps_const_expo, 0);
static CONFIG_INT("fps.sync.shutter", fps_sync_shutter, 0);

#ifdef FEATURE_FPS_RAMPING
static CONFIG_INT("fps.ramp", fps_ramp, 0);
static CONFIG_INT("fps.ramp.duration", fps_ramp_duration, 3);
static int fps_ramp_timings[] = {1, 2, 5, 15, 30, 60, 120, 300, 600, 1200, 1800};
static int fps_ramp_up = 0;
#else
#define fps_ramp 0
#endif

#define FPS_RAMP (fps_ramp && is_movie_mode())

#ifdef FEATURE_FPS_WAV_RECORD
    #ifndef FEATURE_FPS_OVERRIDE
    #error This requires FEATURE_FPS_OVERRIDE.
    #endif
    #ifndef FEATURE_WAV_RECORDING
    #error This requires FEATURE_WAV_RECORDING.
    #endif
int fps_should_record_wav() { return get_fps_override() && fps_wav_record && is_movie_mode() && FPS_SOUND_DISABLE && was_sound_recording_disabled_by_fps_override(); }
#else
int fps_should_record_wav() { return 0; }
#endif

static int is_current_mode_ntsc()
{
    if (!is_movie_mode()) return 0;

    #if defined(CONFIG_50D)
    return !video_system_pal;
    #endif
    if (video_mode_fps == 30 || video_mode_fps == 60 || video_mode_fps == 24) return 1;
    return 0;
}

static void fps_unpatch_table();
static void fps_patch_timerB(int timer_value);
static void fps_read_default_timer_values();
static void fps_read_current_timer_values();

#ifdef CONFIG_DIGIC_V
#define FPS_TIMER_A_MAX 0xFFFF
#define FPS_TIMER_B_MAX 0xFFFF
#else
#define FPS_TIMER_A_MAX 0x2000
#define FPS_TIMER_B_MAX (0x4000-1)
#endif

//~ #define FPS_TIMER_B_MIN (fps_timer_b_orig-100)
#define FPS_TIMER_B_MIN fps_timer_b_orig // it might go lower than that, but it causes trouble high shutter speeds

#define ZOOM (lv_dispsize > 1)
#define MV1080 (is_movie_mode() && video_mode_resolution == 0)
#define MV720 (is_movie_mode() && video_mode_resolution == 1)
#define MV480 (is_movie_mode() && video_mode_resolution == 2)
#define MV1080CROP (MV1080 && video_mode_crop)
#define MV480CROP (MV480 && video_mode_crop)

#if defined(CONFIG_5D2)
    #define TG_FREQ_BASE 24000000
    #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig - (ZOOM ? 0 : 20), ZOOM ? 0x262 : 0x228) // trial and error (with digic poke)
#elif defined(CONFIG_7D)
    #define TG_FREQ_BASE 24000000
    #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig - (ZOOM ? 0 : 20), ZOOM ? 0x262 : 0x228) // todo
#elif defined(CONFIG_5D3)
    #define TG_FREQ_BASE 24000000
    #define FPS_TIMER_A_MIN (fps_timer_a_orig - (ZOOM ? 4 : MV720 ? 30 : 42)) /* zoom: can do 20, but has a black bar on the right */
    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN (fps_timer_b_orig - (ZOOM ? 44 : MV720 ? 0 : 70)) /* you can push LiveView until 68fps (timer_b_orig - 50), but good luck recording that */
#elif defined(CONFIG_EOSM)
    #define TG_FREQ_BASE 32000000
    #define FPS_TIMER_A_MIN (ZOOM ? 666 : MV1080CROP ? 532 : 520)
    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN ( \
    RECORDING_H264 ? (MV1080CROP ? 1750 : MV720 ? 990 : 1970) \
                   : (ZOOM || MV1080CROP ? 1336 : 1970))
#elif defined(CONFIG_6D)
    #define TG_FREQ_BASE 25600000
    #define FPS_TIMER_A_MIN (fps_timer_a_orig - (ZOOM ? 22 : MV720 ? 10 : 34) ) //, ZOOM ? 708 : 512)
    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN (fps_timer_b_orig - (ZOOM ? 6 : MV720 ? 10 : 10)) 
#elif defined(CONFIG_650D)
    #define TG_FREQ_BASE 32000000
    #define FPS_TIMER_A_MIN (fps_timer_a_orig)
#elif defined(CONFIG_700D)
    #define TG_FREQ_BASE 32000000 //copy from 650D
    #define FPS_TIMER_A_MIN (fps_timer_a_orig)
#elif defined(CONFIG_500D)
    #define TG_FREQ_BASE 32000000    // not 100% sure
    #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig - (ZOOM ? 0 : 10), ZOOM ? 1400 : video_mode_resolution == 0 ? 1284 : 1348)
#elif defined(CONFIG_50D)
    #define TG_FREQ_BASE 28800000
    #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig - (ZOOM ? 0 : 10), ZOOM ? 630 : 688 )
#elif defined(CONFIG_550D) || defined(CONFIG_600D) || defined(CONFIG_60D)
    #define TG_FREQ_BASE 28800000
    #define FPS_TIMER_A_MIN MIN(fps_timer_a_orig - (ZOOM ? 0 : 10), ZOOM ? 734 : video_mode_crop ? (video_mode_resolution == 2 ? 400 : 560) : 0x21A)
#endif

// these can change timer B with another method, more suitable for high FPS
#ifdef CONFIG_600D
    #define NEW_FPS_METHOD 1
    #define SENSOR_TIMING_TABLE MEM(0xCB20)
    #define VIDEO_PARAMETERS_SRC_3 0x70AE8 // notation from g3gg0
    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN MIN(fps_timer_b_orig, 1420)
#elif defined(CONFIG_60D)
    #define NEW_FPS_METHOD 1
    #define SENSOR_TIMING_TABLE MEM(0x2a668)
    #define VIDEO_PARAMETERS_SRC_3 0x4FDA8
    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN MIN(fps_timer_b_orig, 1420)
#elif defined(CONFIG_1100D)
    #define NEW_FPS_METHOD 1
    #undef TG_FREQ_BASE
    #define TG_FREQ_BASE 32000000
    #undef FPS_TIMER_A_MIN
    #define FPS_TIMER_A_MIN (ZOOM ? 940 : 872)
    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN 1050
    #define SENSOR_TIMING_TABLE MEM(0xce98)
    #define VIDEO_PARAMETERS_SRC_3 0x70C0C
#elif defined(CONFIG_5D3)
    #define NEW_FPS_METHOD 1
    #ifdef CONFIG_5D3_123
    #define SENSOR_TIMING_TABLE MEM(0x32530)
    #else
    #define SENSOR_TIMING_TABLE MEM(0x325ac)
    #endif
    //~ #define VIDEO_PARAMETERS_SRC_3 MEM(MEM(0x25FF0))

    #undef FPS_TIMER_A_MIN
    #define FPS_TIMER_A_MIN (ZOOM ? 510 : MV720 ? 410 : 398)

    #undef FPS_TIMER_B_MIN
    #define FPS_TIMER_B_MIN (ZOOM ? 1490 : MV720 ? 873 : raw_lv_is_enabled() ? 1500 : 1580)
#endif

static int fps_timer_b_method = 0;
#ifdef NEW_FPS_METHOD
static uint16_t * sensor_timing_table_original = 0;
static uint16_t sensor_timing_table_patched[175*2];
#endif

static int calc_tg_freq(int timerA)
{
    int f = (TG_FREQ_BASE / timerA) * 1000 + MOD(TG_FREQ_BASE, timerA) * 1000 / timerA;
    return f;
}

static int calc_fps_x1000(int timerA, int timerB)
{
    int f = calc_tg_freq(timerA);
    return f / timerB;
}

int get_current_tg_freq()
{
    int timerA = (FPS_REGISTER_A_VALUE & 0xFFFF) + 1;
    if (timerA == 1) return 0;
    int f = calc_tg_freq(timerA);
    return f;
}




/** For FRAME_SHUTTER_TIMER, hex dump VIDEO_PARAMETERS_SRC_3 and look for a value that gets smaller
 *  when you select a faster shutter speed (in movie mode), and gets bigger when you select a slower
 *  shutter speed.
 */

#define TG_FREQ_FPS get_current_tg_freq()

#define FPS_x1000_TO_TIMER(fps_x1000) (((fps_x1000)!=0)?(TG_FREQ_FPS/(fps_x1000)):0)
#define TIMER_TO_FPS_x1000(t) (((t)!=0)?(TG_FREQ_FPS/(t)):0)

#define TG_FREQ_SHUTTER calc_tg_freq(fps_timer_a_orig)
#define SHUTTER_x1000_TO_TIMER(s_x1000) (TG_FREQ_SHUTTER/(s_x1000))
#define TIMER_TO_SHUTTER_x1000(t) (TG_FREQ_SHUTTER/(t))

#ifndef FRAME_SHUTTER_BLANKING_WRITE

static int get_shutter_reciprocal_x1000(int shutter_r_x1000, int Ta, int Ta0, int Tb, int Tb0)
{
    int default_fps = calc_fps_x1000(Ta0, Tb0);
    shutter_r_x1000 = MAX(shutter_r_x1000, default_fps);

    if (Ta == Ta0 && Tb == Tb0) 
        return shutter_r_x1000; // otherwise there may be small rounding errors
    
    int shutter_us = 1000000000 / shutter_r_x1000;
    //~ int actual_fps = calc_fps_x1000(Ta, Tb);
    int resulting_fps_if_we_only_change_timer_b = calc_fps_x1000(Ta0, Tb);
    int fps_timer_delta_us = MAX(1000000000 / resulting_fps_if_we_only_change_timer_b - 1000000000 / default_fps, 0);
    #ifdef NEW_FPS_METHOD
    if (fps_timer_b_method == 1) fps_timer_delta_us = 0;
    #endif
    int ans_raw = 1000000000 / (shutter_us + fps_timer_delta_us);
    int ans = ans_raw * (Ta0/10) / (Ta/10);
    //~ NotifyBox(2000, "shutter_us=%d\ndef_fps=%d res_fps=%d\ntimer_delta_us=%d\nans_raw=%d ans=%d", shutter_us, default_fps, resulting_fps_if_we_only_change_timer_b, fps_timer_delta_us, ans_raw, ans);
    
    return ans;
}
#endif

int get_max_shutter_timer()
{
    int default_fps = calc_fps_x1000(fps_timer_a_orig, fps_timer_b_orig);
    return SHUTTER_x1000_TO_TIMER(default_fps);
}

/* shutter speed in microseconds, from timer value */
int get_shutter_speed_us_from_timer(int timer)
{
    return timer * 1000000 / (TG_FREQ_SHUTTER/1000);
}

#ifdef FRAME_SHUTTER_BLANKING_READ
static uint32_t nrzi_decode( uint32_t in_val )
{
    uint32_t val = 0;
    if (in_val & 0x8000)
        val |= 0x8000;
    for (int num = 0; num < 31; num++)
    {
        uint32_t old_bit = (val & 1<<(30-num+1)) >> 1;
        val |= old_bit ^ (in_val & 1<<(30-num));
    }
    return val;
}
#endif

#ifdef FRAME_SHUTTER_BLANKING_WRITE
static uint32_t nrzi_encode( uint32_t in_val )
{
    uint32_t out_val = 0;
    uint32_t old_bit = 0;
    for (int num = 0; num < 32; num++)
    {
        uint32_t bit = in_val & 1<<(30-num) ? 1 : 0;
        if (bit != old_bit)
            out_val |= (1 << (30-num));
        old_bit = bit;
    }
    return out_val;
}

/* Low Light mode: scale shutter speed with FPS (keep shutter angle constant) */
/* All other modes: keep shutter speed constant */
void fps_override_shutter_blanking()
{
    if (!get_fps_override())
        return;

    /* already overriden? */
    if (FRAME_SHUTTER_BLANKING_READ != *FRAME_SHUTTER_BLANKING_WRITE)
        return;

    /* sensor duty cycle: range 0 ... timer B */
    int current_blanking = nrzi_decode(FRAME_SHUTTER_BLANKING_READ);
    int fps_timer_b_assumed_by_canon = fps_timer_b_method ? fps_timer_b : fps_timer_b_orig;
    int current_exposure = fps_timer_b_assumed_by_canon - current_blanking;
    
    /* wrong assumptions? */
    if (current_exposure < 0)
        return;

    int default_fps = calc_fps_x1000(fps_timer_a_orig, fps_timer_b_orig);
    int current_fps = fps_get_current_x1000();
    
    float frame_duration_orig = 1000.0 / default_fps;
    float frame_duration_current = 1000.0 / current_fps;
    
    float orig_shutter = frame_duration_orig * current_exposure / fps_timer_b_orig;
    float new_shutter = fps_criteria ? orig_shutter : orig_shutter * default_fps / current_fps;

    int new_exposure = new_shutter * fps_timer_b / frame_duration_current;
    int new_blanking = COERCE(fps_timer_b - new_exposure, 2, fps_timer_b - 10);
    
    *FRAME_SHUTTER_BLANKING_WRITE = nrzi_encode(new_blanking);
}
#endif

int get_current_shutter_reciprocal_x1000()
{
#ifdef FRAME_SHUTTER_BLANKING_READ
    #ifdef FRAME_SHUTTER_BLANKING_WRITE
    int blanking = nrzi_decode(*FRAME_SHUTTER_BLANKING_WRITE);   /* prefer to use the overriden value */
    #else
    int blanking = nrzi_decode(FRAME_SHUTTER_BLANKING_READ);
    #endif
    int max = fps_timer_b;
    float frame_duration = 1000.0 / fps_get_current_x1000();
    float shutter = frame_duration * (max - blanking) / max;
    return (int)(1.0 / shutter * 1000);
    
#elif defined(FRAME_SHUTTER_TIMER)
    int timer = FRAME_SHUTTER_TIMER;

    #ifdef FEATURE_SHUTTER_FINE_TUNING
    extern int shutter_finetune_get_adjusted_timer(); /* lv-img-engio.c, to be cleaned up somehow */
    timer = shutter_finetune_get_adjusted_timer();
    #endif
    
    //~ NotifyBox(1000, "%d ", timer);
    int shutter_r_x1000 = TIMER_TO_SHUTTER_x1000(timer);
    
    // shutter speed can't be slower than 1/fps
    //~ shutter_r_x1000 = MAX(shutter_r_x1000, fps_get_current_x1000());
    
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

    return get_shutter_reciprocal_x1000(shutter_r_x1000, fps_timer_a, fps_timer_a_orig, fps_timer_b, fps_timer_b_orig);
#else
    // fallback to APEX units
    if (!lens_info.raw_shutter) return 0;
    return (int) roundf(powf(2.0f, (lens_info.raw_shutter - 136) / 8.0f) * 1000.0f * 1000.0f);
#endif
}

// low fps => positive value
int fps_get_shutter_speed_shift(int raw_shutter)
{
    if (fps_timer_a == fps_timer_a_orig && fps_timer_b == fps_timer_b_orig)
        return 0;

#ifdef FRAME_SHUTTER_BLANKING_WRITE
    if (fps_criteria == 0)
    {
        int default_fps = calc_fps_x1000(fps_timer_a_orig, fps_timer_b_orig);
        int current_fps = fps_get_current_x1000();
        return (int)roundf(8.0f * log2f((float)default_fps / (float)current_fps));
    }
    else return 0;
#else
    // consider that shutter speed is 1/30, to simplify things (that's true in low light)
    int unaltered = (int)roundf(1000/raw2shutterf(MAX(raw_shutter, 96)));
    int altered_by_fps = get_shutter_reciprocal_x1000(unaltered, fps_timer_a, fps_timer_a_orig, fps_timer_b, fps_timer_b_orig);
    
    return (int)roundf(8.0f * log2f((float)unaltered / (float)altered_by_fps));
#endif
}

//--------------------------------------------------------
// sound recording has to be disabled
// otherwise recording is not stable
//--------------------------------------------------------

static int fps_should_disable_sound()
{
    if (get_fps_override() && lv && is_movie_mode())
    {
        /* same FPS as the one from Canon menu? sound OK */
        int default_fps = is_current_mode_ntsc() ? video_mode_fps * 1000 * 1000 / 1001 : video_mode_fps * 1000;
        int current_fps = fps_get_current_x1000();
        if (current_fps == default_fps)
        {
            return 0;
        }

        /* only disable sound when recording H.264, not raw */
        if (!raw_lv_is_enabled())
        {
            return 1;
        }
    }

    /* no problems, no messing with sound */
    return 0;
}


static int old_sound_recording_mode = -1;
int was_sound_recording_disabled_by_fps_override()
{
    return old_sound_recording_mode != -1;
}

static void set_sound_recording(int x)
{
    #ifdef CONFIG_50D
    return;
    #endif
    
    #ifdef CONFIG_5D2
    Gui_SetSoundRecord(COERCE(x,1,3));
    #else
    prop_request_change(PROP_MOVIE_SOUND_RECORD, &x, 4);
    #endif
}

static void restore_sound_recording()
{
    if (RECORDING) return;
    if (old_sound_recording_mode != -1)
    {
        set_sound_recording(old_sound_recording_mode);
        old_sound_recording_mode = -1;
        if (lv) redraw();
    }
}
static void disable_sound_recording()
{
    if (RECORDING) return;
    if (sound_recording_enabled_canon() && is_movie_mode())
    {
        old_sound_recording_mode = sound_recording_mode;
        set_sound_recording(1);
        if (lv) redraw();
    }
}

static void update_sound_recording()
{
    if (RECORDING) return;
    if (fps_should_disable_sound()) disable_sound_recording();
    else restore_sound_recording();
}

PROP_HANDLER(PROP_LV_ACTION)
{
    restore_sound_recording();
}

PROP_HANDLER(PROP_MVR_REC_START)
{
    if (!buf[0] && !lv)
        restore_sound_recording();

#ifdef FEATURE_FPS_RAMPING
    if (buf[0] == 1)
        fps_ramp_up = !fps_ramp_up;
#endif
}
//--------------------------------------------------------

static int fps_get_timer(int fps_x1000)
{
    // convert fps into timer ticks (for sensor drive speed)
    int ntsc = is_current_mode_ntsc();
    int fps_timer = FPS_x1000_TO_TIMER(fps_x1000);

    #if defined(CONFIG_500D) || defined(CONFIG_50D) // these cameras use 30.000 fps, not 29.97 => look in system settings to check if PAL or NTSC
    ntsc = !video_system_pal;
    #endif

    // in PAL/NTSC, round FPS to match the power supply frequency and avoid flicker
    // if criteria is "exact FPS", or fps ramping is enabled, or we are in photo mode, don't round
    if (fps_criteria != 1 && !fps_ramp && is_movie_mode())
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
    
    return fps_timer;
}

// used to see if Canon firmware changed FPS settings
static int written_value_a = 0;
static int written_value_b = 0;
static int fps_needs_updating = 0;
/*int fps_was_changed_by_canon() 
{ 
    int ans = 
        written_value_a != FPS_REGISTER_A_VALUE || 
        written_value_b != FPS_REGISTER_B_VALUE;
        
    //~ if (ans) NotifyBox(2000, "wa=%8x wb=%8x\nra=%8x rb=%8x", written_value_a, written_value_b, FPS_REGISTER_A_VALUE, FPS_REGISTER_B_VALUE);
    return ans;
}*/

static void fps_setup_timerB(int fps_x1000)
{
    if (!lv) return;
    if (!DISPLAY_IS_ON && NOT_RECORDING) return;
    if (lens_info.job_state) return;
    if (!fps_x1000) return;

    // now we can compute timer B
    int timerB_off = desired_fps_timer_b_offset;
    int timerB = 0;
    timerB = fps_get_timer(fps_x1000);
    
    // check hard limits
    timerB = COERCE(timerB, FPS_TIMER_B_MIN, FPS_TIMER_B_MAX);
    
    // apply user fine-tuning offset
    timerB += timerB_off;

    // check hard limits again
    timerB = COERCE(timerB, FPS_TIMER_B_MIN, FPS_TIMER_B_MAX);
    
    #ifdef NEW_FPS_METHOD
    if (fps_timer_b_method == 0) // digic method
    {
        fps_unpatch_table(1);
    #endif
    
        // did anything change since our calculations?
        int defA_before = fps_reg_a_orig;
        int defB_before = fps_reg_b_orig;
        fps_read_default_timer_values();
        if (defA_before != fps_reg_a_orig || defB_before != fps_reg_b_orig)
        {
            // phuck! CTRL-Z, CTRL-Z!
            //~ beep();
            written_value_a = 0;
            EngDrvOutFPS(FPS_REGISTER_A, fps_reg_a_orig);
            return;
        }

        
        // output the value to register
        timerB -= 1;
        written_value_b = PACK(timerB, fps_reg_b_orig);
        EngDrvOutFPS(FPS_REGISTER_B, written_value_b);
        fps_needs_updating = 0;
    #if defined(NEW_FPS_METHOD)
    }
    else
    {
        fps_read_default_timer_values();
        int defA_before_patching = fps_reg_a_orig;
        int defB_before_patching = fps_reg_b_orig;
        
        fps_patch_timerB(timerB);
        written_value_b = timerB-1;
        if (NOT_RECORDING) msleep(500);
        // timer A was changed by refreshing the screen
        // timer B may not be refreshed when recording
        
        // BUT... are we still in the same video mode? or did the user switch it quickly?
        fps_read_default_timer_values();
        if (defA_before_patching == fps_reg_a_orig && defB_before_patching == fps_reg_b_orig)
        {
            EngDrvOutFPS(FPS_REGISTER_A, written_value_a);
            fps_needs_updating = 0;
        }
        else // something went wrong, will fix at next iteration
        {
            //~ beep();
        }
        //~ EngDrvOutFPS(FPS_REGISTER_B, written_value_b);
        msleep(500);
    }
    #endif

    // apply changes
    EngDrvOutFPS(FPS_REGISTER_CONFIRM_CHANGES, 1);
}

int fps_get_current_x1000()
{
    if (!lv) return 0;
    int fps_timer = (FPS_REGISTER_B_VALUE & 0xFFFF) + 1;
    int fps_x1000 = TIMER_TO_FPS_x1000(fps_timer);
    return fps_x1000;
}

static void calc_rolling_shutter(int * line_ns, int * frame_us, int * frame_percent, int * xres, int * yres);

static MENU_UPDATE_FUNC(fps_print)
{
    static int last_inactive = 0;
    int t = get_ms_clock_value_fast();

    int frame_readout_time_percent;
    calc_rolling_shutter(0, 0, &frame_readout_time_percent, 0, 0);

    if (fps_override)
    {
        int current_fps = fps_get_current_x1000();
        MENU_SET_VALUE("%d.%03d", 
            current_fps/1000, current_fps%1000
        );

        if (frame_readout_time_percent)
        {
            MENU_SET_RINFO("Roll.sh.%d%%", frame_readout_time_percent);
        }

        /* FPS override will disable sound recording automatically, but not right away (only at next update step) */
        /* if it can't be disabled automatically (timeout 1 second), show a warning so the user can disable it himself */
        if (sound_recording_enabled_canon() && is_movie_mode() && fps_should_disable_sound() && t > last_inactive + 1000)
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Sound recording must be disabled from Canon menu.");

#ifndef CONFIG_FRAME_ISO_OVERRIDE
        if (fps_sync_shutter && !is_movie_mode() && !CONTROL_BV)
            MENU_SET_WARNING(MENU_WARN_ADVICE, "Enable exposure override to get proper exposure simulation.");
#endif
    }
    else
    {
        last_inactive = t;
    
        int current_fps = fps_get_current_x1000();
        MENU_SET_RINFO("%d.%03d", 
            current_fps/1000, current_fps%1000
        );

        if (frame_readout_time_percent)
        {
            MENU_APPEND_RINFO(", Rs.%d%%", frame_readout_time_percent);
        }
    }

#ifdef CONFIG_7D
    if (is_movie_mode() && !raw_lv_is_enabled())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "On 7D, FPS override can be used only with RAW, or in photo mode.");
    }
#endif

}

static MENU_UPDATE_FUNC(fps_current_print)
{
    int current_fps = fps_get_current_x1000();
    
    MENU_SET_VALUE(
        "%d.%03d",
        current_fps/1000, current_fps%1000
    );
}

static MENU_UPDATE_FUNC(desired_fps_print)
{
    int desired_fps = fps_values_x1000[fps_override_index];
    int default_fps = lv ? calc_fps_x1000(fps_timer_a_orig, fps_timer_b_orig) : 0;
    if (desired_fps % 1000)
        MENU_SET_VALUE(
            "%d.%03d (from %d)",
            desired_fps/1000, desired_fps%1000, (default_fps+500)/1000
        );
    else
        MENU_SET_VALUE(
            "%d (from %d)",
            desired_fps/1000, (default_fps+500)/1000
        );
    
    if (fps_sync_shutter && !is_movie_mode())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "FPS value is computed from photo shutter speed.");
}

static void calc_rolling_shutter(int * line_ns, int * frame_us, int * frame_percent, int * xres, int * yres)
{
    /* Timer A tells us how fast the rows are read out */
    /* Timer A / main clock = time for reading one line */
    /* Multiply by raw vertical resolution => rolling shutter */

    int main_clock_div_timer_A = get_current_tg_freq();
    float line_readout_time_us = 1.0e9f / main_clock_div_timer_A;
    if (line_ns) *line_ns = (int)roundf(line_readout_time_us * 1000.0f);

    int vertical_res = 0;
    int horizontal_res = 0;
    
    if (raw_lv_is_enabled())
    {
        horizontal_res = raw_info.jpeg.width;
        vertical_res = raw_info.jpeg.height;
    }
    
    if (RECORDING_H264)
    {
        get_yuv422_hd_vram();
        horizontal_res = vram_hd.width;
        vertical_res = vram_hd.height;

        if (video_mode_resolution <= 1)
        {
            /* guess how many pixels are actually used for 16:9 recording */
            /* (the HD buffer might be a little larger than that) */
            int vertical_res_16_9 = (horizontal_res * 9/16) & ~1;
            vertical_res = MIN(vertical_res, vertical_res_16_9);
        }
    }

    if (xres) *xres = horizontal_res;
    if (yres) *yres = vertical_res;
    if (frame_us) *frame_us = 0;
    if (frame_percent) *frame_percent = 0;
    
    if (vertical_res)
    {
        int frame_duration_us = (int)roundf(1e9 / fps_get_current_x1000());

        if (frame_us) *frame_us = line_readout_time_us * vertical_res;
        if (frame_percent) *frame_percent = (int)roundf(line_readout_time_us * vertical_res * 100 / frame_duration_us);
    }
}

static MENU_UPDATE_FUNC(rolling_shutter_print)
{
    int line_readout_time_ns, frame_readout_time_us, frame_readout_time_percent, horizontal_res, vertical_res;
    calc_rolling_shutter(&line_readout_time_ns, &frame_readout_time_us, &frame_readout_time_percent, &horizontal_res, &vertical_res);
   
    int line_readout_time_us_x10 = line_readout_time_ns / 100;

    /* since we don't know exactly the recording resolution, that's the only reliable value that we can display */
    MENU_SET_VALUE("%s%d.%d "SYM_MICRO"s / line", FMT_FIXEDPOINT1(line_readout_time_us_x10));
    
    /* trick to display status messages even with FPS override turned off */
    int old_warn = info->warning_level;
    info->warning_level = MENU_WARN_NONE;
    
    if (vertical_res)
    {
        int rolling_shutter_ms_x10 = frame_readout_time_us / 100;
        
        MENU_SET_WARNING(MAX(MENU_WARN_INFO, old_warn), "Rolling shutter: %s%d.%d ms (%d%%) at %dx%d.",
            FMT_FIXEDPOINT1(rolling_shutter_ms_x10),
            frame_readout_time_percent, 0,
            horizontal_res, vertical_res
        );
    }
    else
    {
        MENU_SET_WARNING(MAX(MENU_WARN_ADVICE, old_warn), "Start recording to find out the vertical resolution.");
    }
}

#if defined(NEW_FPS_METHOD)

static int video_mode[10];
PROP_HANDLER(PROP_VIDEO_MODE)
{
    ASSERT(len <= sizeof(video_mode));
    memcpy(video_mode, buf, len);
}

static void flip_zoom_twostage(int stage)
{
    // flip zoom or video mode back and forth to apply settings instantly
    if (!lv) return;
    if (RECORDING) return;
    #ifndef CONFIG_DIGIC_V // causes corrupted video headers on 5D3
    if (is_movie_mode())
    {
        // in movie mode, flipping the FPS seems nicer
        {
            static int f0;
            if (stage == 1)
            {
                f0 = video_mode[2];
                video_mode[2] = 
                    f0 == 24 ? 25 : 
#ifndef CONFIG_1100D
                    f0 == 25 ? 24 : 
#else
                    f0 == 25 ? 30 :
#endif
                    f0 == 30 ? 25 : 
                    f0 == 50 ? 60 :
                  /*f0 == 60*/ 50;
                prop_request_change(PROP_VIDEO_MODE, video_mode, 0);
                msleep(50);
            }
            else if (stage == 2)
            {
                video_mode[2] = f0;
                prop_request_change(PROP_VIDEO_MODE, video_mode, 0);
                msleep(50);
            }

            return;
        }
    }
    #endif
    
    static int zoom0;
    if (stage == 1)
    {
        zoom0 = lv_dispsize;
        int zoom1 = zoom0 == 5 ? 10 : 5;
        prop_request_change_wait(PROP_LV_DISPSIZE, &zoom1, 4, 100);
    }
    else if (stage == 2)
    {
        prop_request_change_wait(PROP_LV_DISPSIZE, &zoom0, 4, 1000);
    }
}

static void flip_zoom()
{
    flip_zoom_twostage(1);
    flip_zoom_twostage(2);
}

static void fps_unpatch_table(int refresh)
{
    if (SENSOR_TIMING_TABLE == (uintptr_t) sensor_timing_table_original)
        return;
    SENSOR_TIMING_TABLE = (uintptr_t) sensor_timing_table_original;
    
    if (refresh)
    {
        flip_zoom();
        msleep(500);
    }
}
#endif

// don't msleep from here, it may be called from GMT
static void fps_register_reset()
{
    if (!lv) return;
    if (!DISPLAY_IS_ON && NOT_RECORDING) return;
    if (lens_info.job_state) return;

    if (fps_reg_a_orig && fps_reg_b_orig)
    {
        written_value_a = 0;
        written_value_b = 0;
        EngDrvOutFPS(FPS_REGISTER_A, fps_reg_a_orig);
        EngDrvOutFPS(FPS_REGISTER_B, fps_reg_b_orig);
        EngDrvOutFPS(FPS_REGISTER_CONFIRM_CHANGES, 1);
    }
}

static void fps_reset()
{
    //~ get_fps_override() = 0;
    fps_needs_updating = 0;

    #ifdef NEW_FPS_METHOD
    /* may be a little slower, but if we call it after fps_register_reset, 
     * it would be like a short overcranking (with transient image artifacts)
     * this happens because the powersave timers are optimized for the low FPS */
    fps_unpatch_table(1);
    #endif
    
    fps_register_reset();
    
    #ifdef CONFIG_FPS_UPDATE_FROM_EVF_STATE
    fps_disable_timers_evfstate();
    #endif

    restore_sound_recording();
}


static void fps_change_value(void* priv, int delta)
{
    fps_override_index = MOD(fps_override_index + delta, COUNT(fps_values_x1000));
    desired_fps_timer_a_offset = 0;
    desired_fps_timer_b_offset = 0;
    if (get_fps_override()) fps_needs_updating = 1;
}

static void fps_enable_disable(void* priv, int delta)
{
    #ifdef FEATURE_FPS_OVERRIDE
    fps_override = !fps_override;
    #endif
    if (get_fps_override()) fps_needs_updating = 1;
}

#ifndef FRAME_SHUTTER_BLANKING_WRITE
static MENU_UPDATE_FUNC(shutter_range_print)
{
    // EA = (E0 + (1/Fb - 1/F0)) * Ta / Ta0
    // see get_current_shutter_reciprocal_x1000 for details
    
    int shutter_r_0_lo_x1000 = video_mode_fps * 1000;
    int shutter_r_0_hi_x1000 = 4000*1000;
    int tv_low = get_shutter_reciprocal_x1000(shutter_r_0_lo_x1000, fps_timer_a, fps_timer_a_orig, fps_timer_b, fps_timer_b_orig);
    int tv_high = get_shutter_reciprocal_x1000(shutter_r_0_hi_x1000, fps_timer_a, fps_timer_a_orig, fps_timer_b, fps_timer_b_orig);
    int tv_low_r = (tv_low+500)/1000;
    int tv_low_s_x10 = 100000 / tv_low;
    int tv_high_r = (tv_high+500)/1000;
    int tv_high_s_x10 = 100000 / tv_high;

    if (tv_low >= 10000) MENU_SET_VALUE("1/%d", tv_low_r);
    else MENU_SET_VALUE("%d.%02d\"", tv_low_s_x10/100, tv_low_s_x10%100);

    MENU_APPEND_VALUE("..");

    if (tv_high >= 10000) MENU_APPEND_VALUE("1/%d", tv_high_r);
    else MENU_APPEND_VALUE("%d.%02d\"", tv_high_s_x10/100, tv_high_s_x10%100);
}
#endif

static MENU_UPDATE_FUNC(fps_timer_print)
{
    int A = (entry->priv == &desired_fps_timer_a_offset);
    int t = A ? fps_timer_a : fps_timer_b;
    int t0 = A ? fps_timer_a_orig : fps_timer_b_orig; 
    if (t0 == 0) t0 = 1;
    int t_min = A ? FPS_TIMER_A_MIN : FPS_TIMER_B_MIN;
    int t_max = A ? FPS_TIMER_A_MAX : FPS_TIMER_B_MAX;
    int finetune_delta = A ? desired_fps_timer_a_offset : desired_fps_timer_b_offset;
    int delta = t - t0;
    char dec[10] = "";
    if (!finetune_delta && ABS(delta) >= 100) 
        snprintf(dec, sizeof(dec), ".%02d", ((t * 100 / t0) % 100));
    MENU_SET_NAME(
        "FPS timer %s",
        A ? "A" : "B"
    );
    MENU_SET_VALUE(
        "%d (%s%d%s)",
        t, 
        finetune_delta > 0 ? "FT+" : finetune_delta < 0 ? "FT" : ABS(delta) >= 100 ? "x" : delta >= 0 ? "+" : "", 
        finetune_delta ? finetune_delta : ABS(delta) >= 100 ? t / t0 : delta, 
        dec
    );
    
    if (t_max <= t_min)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Internal error - please report to ML devs.");
    else
        MENU_SET_ICON(MNI_PERCENT, sqrtf(t - t_min) * 100  / sqrtf(t_max - t_min));
}

static MENU_UPDATE_FUNC(tg_freq_print)
{
    MENU_SET_VALUE(
        "%d.%02d MHz",
        TG_FREQ_BASE / 1000000, (TG_FREQ_BASE % 1000000) / 10000
    );
}

static void fps_timer_fine_tune_a(void* priv, int delta)
{
    desired_fps_timer_a_offset += delta * 2;
    if (get_fps_override()) fps_needs_updating = 1;
}

static void fps_timer_fine_tune_a_big(void* priv, int delta)
{
    desired_fps_timer_a_offset += delta * 100;
    if (get_fps_override()) fps_needs_updating = 1;
}

static void fps_timer_fine_tune_b(void* priv, int delta)
{
    desired_fps_timer_b_offset += delta;
    if (get_fps_override()) fps_needs_updating = 1;
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
        int e = ABS(fps_x1000 - actual_fps);
        if (e < min_err)
        {
            min_err = e;
            best_t = t;
        }
        if (min_err == 0) break;
    }
    return best_t;
}

static int fps_try_to_get_180_360_shutter(int fps_x1000)
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

/**

Timer side effects:
- timer A: increase jello effect and multiply shutter speed (exposure time)
- timer B: the difference in FPS, computed as time units, is added to shutter speed (exposure time)
           for example: 1/500 in Canon menu, 12fps from 24 by doubling timer B. Resulting shutter speed:
           1/500 + 1/12 - 1/24 = 0.0437 = 1/22.9 
- with NEW_FPS_METHOD, the side effect of timer B can be cancelled

The algorithm for choosing timer values depends on the optimization setting:

- low light (for low frame rates): try to increase only timer B
- exact FPS: try to get as close as possible to the exact value; if there are more solutions, 
  the one with smallest timer A is chosen (that results in lowest jello effect)
- high FPS (NEW_FPS_METHOD): try to decrease timer B first (no side effect on shutter speed)
- low jello, 180d (not NEW_FPS_METHOD): at moderately low FPS, make sure you can get 180-degree 
  shutter speed, and choose the solution with the lowest jello effect. Usually you have to select 
  1/4000 from Canon menu.
- high jello: try to increase timer A first, since this is what causes jello effect

At extremes, in all cases the algorithm should hit the hard limits for both timers (at least in theory).

On new cameras (NEW_FPS_METHOD), timer B can be changed either with or without the side effect of altering 
the shutter speed (you can choose whether you want the side effect, or not). So, low light mode includes 
the side effect, to get slower shutter speeds, but the other modes will cancel the side effect.

Technical: timer B can be altered via engio (with side effect) or via table patching 
(without side effect, but requires a video mode change to take effect).

*/

static void fps_setup_timerA(int fps_x1000)
{
    if (!lv) return;
    if (!DISPLAY_IS_ON && NOT_RECORDING) return;
    if (!fps_x1000) return;
    if (lens_info.job_state) return;

    // for NTSC, we probably need FPS * 1000/1001
    int ntsc = is_current_mode_ntsc();
    ntsc += 0; // bypass warning
    if (fps_criteria == 1) ntsc = 0; // use PAL-like rounding [hack]
    #if !defined(CONFIG_500D) && !defined(CONFIG_50D) // these cameras use 30.000 fps, not 29.97
    if (ntsc) fps_x1000 = fps_x1000 * 1000/1001;
    #endif

    int timerA = fps_timer_a_orig;
    int timerA_max = FPS_TIMER_A_MAX;

    #ifdef CONFIG_DIGIC_V
    /* try to limit vertical noise lines */
    timerA_max = timerA * 3/2;
    #endif

    #ifdef NEW_FPS_METHOD
    int default_fps = calc_fps_x1000(fps_timer_a_orig, fps_timer_b_orig);
    #endif
    
    // {"Low light", "Exact FPS", "180deg shutter", "Jello effect"},
    switch (fps_criteria)
    {
        case 0:
            // if we leave timer A at default value, 
            // or we change it as little as possible (just to bring requested FPS in range),
            // we get best low light capability and lowest amount of jello effect.
            timerA = fps_timer_a_orig;
            #ifdef NEW_FPS_METHOD
            fps_timer_b_method = fps_x1000 < default_fps ? 0 : 1;
            #endif
            break;
        case 1:
            timerA = fps_try_to_get_exact_freq(fps_x1000);
            #ifdef NEW_FPS_METHOD
            fps_timer_b_method = 1;
            #endif
            break;
        case 2:
            #ifdef NEW_FPS_METHOD
            timerA = fps_timer_a_orig;
            fps_timer_b_method = 1;
            #else
            timerA = fps_try_to_get_180_360_shutter(fps_x1000);
            #endif
            break;
        case 3:
            timerA = TG_FREQ_BASE / fps_x1000 * 1000 / fps_timer_b_orig;
            #ifdef NEW_FPS_METHOD
            fps_timer_b_method = 1;
            #endif
            
            #ifdef CONFIG_DIGIC_V
            /* we need to do the magic from timer A... */
            timerA_max = FPS_TIMER_A_MAX;
            #endif
            break;
    }
    
    #ifdef NEW_FPS_METHOD
    // FPS ramping effect requires being able to change FPS on the fly
    if (FPS_RAMP)
    {
        fps_timer_b_method = 0;
    }
    
    #ifdef FRAME_SHUTTER_BLANKING_WRITE
    /* if we can override shutter blanking, table patching will be only needed for overcranking */
    /* otherwise, the classic method is preferred, because it does not require video mode switching */
    if (fps_x1000 < default_fps + 500)
    {
        fps_timer_b_method = 0;
    }
    #endif
    #endif

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
    timerA = COERCE(timerA, FPS_TIMER_A_MIN, timerA_max);
    
    // apply user fine tuning
    int timerA_off = desired_fps_timer_a_offset;
    timerA += timerA_off;

    // check hard limits again
    timerA = COERCE(timerA, FPS_TIMER_A_MIN, timerA_max);
    
    // keep the same parity as original timer A
    timerA = (timerA & 0xFFFE) | (fps_timer_a_orig & 1);

    // save setting to DIGIC register
    int val_a = PACK(timerA-1, fps_timer_a_orig-1);
    written_value_a = val_a;

    EngDrvOutFPS(FPS_REGISTER_A, val_a);
}

static void fps_criteria_change(void* priv, int delta)
{
    desired_fps_timer_a_offset = 0;
    desired_fps_timer_b_offset = 0;
    fps_criteria = MOD(fps_criteria + delta, 4);
    if (get_fps_override()) fps_needs_updating = 1;
}

static MENU_UPDATE_FUNC(fps_wav_record_print)
{
    MENU_SET_ENABLED(1);
    MENU_SET_ICON(CURRENT_VALUE ? MNI_ON : MNI_DISABLE, 0);
}

static MENU_UPDATE_FUNC(fps_ramp_duration_update)
{
    if (!fps_ramp) MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "FPS ramping disabled.");
}
static MENU_UPDATE_FUNC(fps_const_expo_update)
{
    extern int smooth_iso;
    if (smooth_iso) MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "You need to disable gradual exposure.");
}

static struct menu_entry fps_menu[] = {
    #ifdef FEATURE_FPS_OVERRIDE
    {
        .name = "FPS override", 
        .priv = &fps_override,
        .select = fps_enable_disable,
        .update = fps_print,
        .max = 1,
        .help = "Changes FPS. Also disables sound and alters shutter speeds.",
        .help2 = "Tip: in photo mode, it makes LiveView usable in darkness.",
        .depends_on = DEP_LIVEVIEW,
        .submenu_width = 650,
        .children =  (struct menu_entry[]) {
            {
                .name = "Desired FPS", 
                .priv    = &fps_override_index,
                .update = desired_fps_print,
                .min = 0,
                .max = COUNT(fps_values_x1000) - 1,
                .select = fps_change_value,
                .icon_type = IT_PERCENT,
                .help = "FPS value for recording. Video will play back at Canon FPS.",
            },
            {
                .name = "Optimize for",
                .priv       = &fps_criteria,
                .choices = (const char *[]) {
                    "Low light", 
                    "Exact FPS", 
                    #if defined(NEW_FPS_METHOD) || defined(FRAME_SHUTTER_BLANKING_WRITE)
                    "High FPS",
                    "High Jello",
                    #else
                    "Low Jello, 180d", 
                    "HiJello, FastTv",
                    #endif
                },
                .icon_type = IT_DICE,
                .max = 3,
                .select = fps_criteria_change,
                .help = "Changing FPS has side effects - choose what's best for you:",
                .help2 =
                        #ifdef FRAME_SHUTTER_BLANKING_WRITE
                        "Low light: slow shutter speeds. Shutter angle is constant.\n"
                        #else
                        "Low light: at low FPS, use 1/FPS (360 deg) shutter speeds.\n"
                        #endif
                        "Exact FPS: for 24.000 instead of 23.976 and similar.\n"
                        #if defined(NEW_FPS_METHOD) || defined(FRAME_SHUTTER_BLANKING_WRITE)
                        "High FPS: best for slight overcranking (eg 35fps from 30).\n"
                        "High Jello: slit-scan effect (use 2-5 fps and fast shutter).\n"
                        #else
                        "Low Jello, 180d: for 1/2fps shutter speed (1/20 at 10fps).\n" 
                        "HiJello, FastTv: jello effects and fast shutters (2-5 fps).\n"
                        #endif
            },
            #ifndef FRAME_SHUTTER_BLANKING_WRITE
            {
                .name = "Shutter range",
                .update = shutter_range_print,
                .select = fps_timer_fine_tune_a_big,
                .icon_type = IT_ALWAYS_ON,
                .help  = "Shutter speed range, when Canon shows 1/30-1/4000.",
                .help2 = "You can fine-tune this, but don't expect miracles.",
                .advanced = 1,
            },
            #endif
            {
                .name = "FPS timer A",
                .update = fps_timer_print,
                .priv = &desired_fps_timer_a_offset,
                .select = fps_timer_fine_tune_a,
                .icon_type = IT_PERCENT,
                .help = "High values = lower FPS, more jello effect, faster shutter.",
                .advanced = 1,
            },
            {
                .name = "FPS timer B",
                .update = fps_timer_print,
                .priv = &desired_fps_timer_b_offset,
                .select = fps_timer_fine_tune_b,
                .icon_type = IT_PERCENT,
                .help = "High values = lower FPS, shutter speed converges to 1/fps.",
                .advanced = 1,
            },
            {
                .name = "Main Clock",
                .update = tg_freq_print,
                .icon_type = IT_ALWAYS_ON,
                .help = "Timing generator freq. (READ-ONLY). FPS = F/timerA/timerB.",
                .advanced = 1,
            },
            {
                .name = "Actual FPS",
                .update = fps_current_print,
                .icon_type = IT_ALWAYS_ON,
                .help = "Exact FPS (computed). For fine tuning, change timer values.",
            },

            {
                .name = "Rolling shutter",
                .update = rolling_shutter_print,
                .icon_type = IT_ALWAYS_ON,
                .help = "Amount of jello effect. Multiply \""SYM_MICRO"s/line\" by vertical resolution.",
            },

            #ifdef CONFIG_FRAME_ISO_OVERRIDE
            #ifndef FRAME_SHUTTER_BLANKING_WRITE
            {
                .name = "Constant expo",
                .priv = &fps_const_expo,
                .max = 1,
                .update = fps_const_expo_update,
                .help  = "Keep the same exposure (brightness) as with default FPS.",
                .help2 = "This works by lowering ISO => you may get pink highlights.",
                .depends_on = DEP_MANUAL_ISO | DEP_MOVIE_MODE,
            },
            #endif
            #endif

            {
                .name = "Sync w. Shutter",
                .priv = &fps_sync_shutter,
                .max = 1,
                .help  = "Sync FPS with shutter speed, for long exposures in LiveView.",
                .depends_on = DEP_PHOTO_MODE,
                .advanced = 1,
            },

            #ifdef FEATURE_FPS_WAV_RECORD
            {
                .name = "Sound Record",
                .priv = &fps_wav_record,
                .max = 1,
                .update = fps_wav_record_print,
                .choices = (const char *[]) {"Disabled", "Separate WAV"},
                .help = "Sound goes out of sync, so it has to be recorded separately.",
                .advanced = 1,
            },
            #endif


            #ifdef FEATURE_FPS_RAMPING
            {
                .name = "FPS ramping", 
                .priv = &fps_ramp,
                .max = 1,
                .help = "Ramp between overridden FPS and default FPS. Undercrank only.",
                .help2 = "To start ramping, press " INFO_BTN_NAME " or just start recording.",
                .depends_on = DEP_MOVIE_MODE,
                .advanced = 1,
            },

            {
                .name = "Ramp duration",
                .priv = &fps_ramp_duration,
                .max = 10,
                .update = fps_ramp_duration_update,
                .choices = (const char *[]) {"1s", "2s", "5s", "15s", "30s", "1min", "2min", "5min", "10min", "20min", "30min"},
                .icon_type = IT_PERCENT,
                .help = "Duration of FPS ramping (in real-time, not in playback).",
                .depends_on = DEP_MOVIE_MODE,
                .advanced = 1,
            },
            #endif
            MENU_ADVANCED_TOGGLE,
            MENU_EOL
        },
    },
    #endif
};

static void fps_init()
{
    menu_add( "Movie", fps_menu, COUNT(fps_menu) );
}

INIT_FUNC("fps", fps_init);

static void fps_read_current_timer_values()
{
    if (!lv) { fps_timer_a = fps_timer_b = 0; return; }

    int VA = FPS_REGISTER_A_VALUE;
    int VB = FPS_REGISTER_B_VALUE;
    fps_timer_a = (VA & 0xFFFF) + 1;
    fps_timer_b = (VB & 0xFFFF) + 1;
}

/*static int fps_check_if_current_timer_values_changed()
{
    int changed = 0;
    static int prev_a = 0;
    static int prev_b = 0;
    if (prev_a != fps_timer_a || prev_b != fps_timer_b) changed = 1;
    prev_a = fps_timer_a;
    prev_b = fps_timer_b;
    return changed;    
}*/

static int get_fps_video_mode();
static int get_table_pos(unsigned int fps_mode, unsigned int crop_mode, unsigned int type, int dispsize);

static void fps_read_default_timer_values()
{
    if (!lv) { fps_reg_a_orig = fps_reg_b_orig = 0; return; }
    
    if (RECORDING_H264_STARTING) return;
    //~ info_led_blink(1,10,10);
    fps_reg_a_orig = FPS_REGISTER_A_DEFAULT_VALUE;
    #if defined(NEW_FPS_METHOD)
    int mode = get_fps_video_mode();
    unsigned int pos = get_table_pos(mode, video_mode_crop, 0, lv_dispsize);
    fps_reg_b_orig = sensor_timing_table_original[pos] - 1; // nobody will change it from here :)
    //bmp_printf(FONT_LARGE, 50, 50, "%08x %08x %08x", fps_reg_a_orig, bmp_vram_real(), bmp_vram_idle());
    #else
    int val = FPS_REGISTER_B_VALUE;
    if (val & 0xFFFF0000)
        fps_reg_b_orig = val >> 16; // timer value written by ML - contains original value in highest 16 bits
    else {
        fps_reg_b_orig = val; // timer value written by Canon
    }
    #endif
    fps_timer_a_orig = ((fps_reg_a_orig >> 16) & 0xFFFF) + 1;
    fps_timer_b_orig = (fps_reg_b_orig & 0xFFFF) + 1;
}

// maybe FPS settings were changed by someone else? if yes, force a refresh
static void fps_check_refresh()
{
    int fps_ov = get_fps_override();
    static int old_fps_ov = 0;
    if (old_fps_ov != fps_ov) fps_needs_updating = 1;
    old_fps_ov = fps_ov;
}

#ifdef FEATURE_FPS_OVERRIDE

#ifdef CONFIG_FPS_UPDATE_FROM_EVF_STATE
static int fps_video_mode_changed()
{
    if (written_value_a != FPS_REGISTER_A_VALUE)
        return 1;
/* false positives on 6D
    int wb = written_value_b & 0xFFFF;
    int WB = FPS_REGISTER_B_VALUE & 0xFFFF;

    if (ABS(wb - WB) > 2)
        return 1;
*/
    return 0;
}

static volatile int fps_timerA_override = 0;
static volatile int fps_timerB_override = 0;
static volatile int fps_timers_updated = 0;

void fps_update_timers_from_evfstate()
{
    if (fps_timerA_override && fps_timerB_override && !fps_video_mode_changed())
    {
        EngDrvOutLV(FPS_REGISTER_A, fps_timerA_override);
        EngDrvOutLV(FPS_REGISTER_B, fps_timerB_override);
        EngDrvOutLV(FPS_REGISTER_CONFIRM_CHANGES, 1);
    }
    fps_timers_updated = 1;
}

static void fps_set_timers_from_evfstate(int timerA, int timerB, int wait)
{
    fps_timers_updated = 0;
    fps_timerA_override = timerA;
    fps_timerB_override = timerB;
    if (wait)
        while (!fps_timers_updated)
            msleep(20);
}

static void fps_disable_timers_evfstate()
{
    fps_timerA_override = fps_timerB_override = 0;
}

#endif

// do all FPS changes from this task only - to avoid trouble ;)
static void fps_task()
{
    #ifdef CONFIG_7D
    buf = fio_malloc(sizeof(uint32_t));
    #endif
    
    TASK_LOOP
    {
        #ifdef FEATURE_FPS_RAMPING
        if (FPS_RAMP) 
        {
            msleep(20);
        }
        else
        #endif
        {
            #ifdef CONFIG_FPS_AGGRESSIVE_UPDATE
            msleep(get_fps_override() && RECORDING ? 10 : 100);
            #else
            msleep(100);
            #endif
        }
        
        fps_check_refresh();

        //~ bmp_hexdump(FONT_SMALL, 10, 200, SENSOR_TIMING_TABLE, 32*10);
        //~ NotifyBox(1000, "defB: %d ", fps_timer_b_orig); msleep(1000);

        static int fps_warned = 0;
        if (!lv) { fps_warned = 0; continue; }
        if (!DISPLAY_IS_ON && NOT_RECORDING) continue;
        if (lens_info.job_state) continue;
        
        fps_read_current_timer_values();
        fps_read_default_timer_values();
        
        //~ NotifyBox(2000, "d: %d,%d. c: %d,%d ", fps_timer_a_orig, fps_timer_b_orig, fps_timer_a, fps_timer_b);
        
        if (!get_fps_override()) 
        {
            msleep(100);

            if (!get_fps_override() && fps_needs_updating)
            {
                fps_reset();
                fps_read_current_timer_values();
                fps_read_default_timer_values();
                #ifdef FEATURE_EXPO_OVERRIDE
                bv_auto_update();
                #endif
            }
                            
            continue;
        }

        int default_fps = calc_fps_x1000(fps_timer_a_orig, fps_timer_b_orig);
        int f = fps_values_x1000[fps_override_index];
        
        if (fps_sync_shutter && !is_movie_mode())
        {
            f = MIN(1000000 / raw2shutter_ms(lens_info.raw_shutter), default_fps);
        }
        
        if (lv_dispsize == 10 && get_halfshutter_pressed())
        {
            /* x10 zoom - disable FPS override to check focus */
            f = default_fps;
        }
        
        #ifdef FEATURE_FPS_RAMPING
        if (FPS_RAMP) // artistic effect - http://www.magiclantern.fm/forum/index.php?topic=2963.0
        {
            f = MIN(f, default_fps); // no overcranking possible with FPS ramping
            
            int total_duration = fps_ramp_timings[fps_ramp_duration];
            float delta = 1.0 / 50 / total_duration;
            
            static float k = 0;

            if (!(RECORDING && MVR_FRAME_NUMBER < 1))
            {
                if (fps_ramp_up) k += delta; else k -= delta;
            }
            k = COERCE(k, 0, 1);
            
            float ks = k*k;
            float ff = default_fps * ks + f * (1-ks);
            int fr = (int)roundf(ff);
            fps_setup_timerA(fr);
            fps_setup_timerB(fr);
            fps_read_current_timer_values();

            // take care of sound settings to prevent recording from stopping
            update_sound_recording();

            int x0 = os.x0;
            int y0 = os.y_max - 2;
            
            bmp_draw_rect(COLOR_ORANGE, x0, y0, (int)(k * os.x_ex), 1);
            bmp_draw_rect(COLOR_BLACK, (int)(k * os.x_ex), y0, (int)((1-k) * os.x_ex), 1);

            continue;
        }
        #endif
        
        // Very low FPS: first few frames will be recorded at normal FPS, to bypass Canon's internal checks
        if (f < 5000)
            while (RECORDING && MVR_FRAME_NUMBER < video_mode_fps)
                msleep(MIN_MSLEEP);

        // on new cameras, timer B may be changed back and forth, e.g. EOS M: 2527/2528 for 24p
        // these tiny changes should not be considered a video mode change
        static int prev_sig = 0;
        static int prev_timer_b = 0;
        fps_read_current_timer_values();
        fps_read_default_timer_values();
        int sig = fps_timer_a_orig + lv_dispsize*111 + video_mode_resolution*17 + video_mode_fps*123 + video_mode_crop*4567;
        int video_mode_changed = (sig != prev_sig) || ABS(fps_timer_b_orig - prev_timer_b) > 2;
        prev_sig = sig;
        prev_timer_b = fps_timer_b_orig;
        
        //~ bmp_printf(FONT_LARGE, 50, 150, "%dx, setting up from %d,%d   ", lv_dispsize, fps_timer_a_orig, fps_timer_b_orig);

        if (video_mode_changed && NOT_RECORDING) // Video mode changed, wait for it to settle
        {                                     // This won't happen while recording (obvious),
            #ifdef CONFIG_FPS_UPDATE_FROM_EVF_STATE
            fps_disable_timers_evfstate();
            #endif
            msleep(500);                      // BUT sometimes Canon code might choose to revert FPS back - in this case, ML must act quickly
            if (is_movie_mode() && video_mode_crop) msleep(500);
            continue;
        }
        
        //~ info_led_on();
        fps_setup_timerA(f);
        fps_setup_timerB(f);
        //~ info_led_off();
        fps_read_current_timer_values();
        //~ bmp_printf(FONT_LARGE, 50, 100, "%dx, new timers: %d,%d ", lv_dispsize, fps_timer_a, fps_timer_b);

        // take care of sound settings to prevent recording from stopping
        update_sound_recording();

        if (!fps_warned && !gui_menu_shown())
        {
            int current_fps = fps_get_current_x1000();
            char msg[30];
            snprintf(msg, sizeof(msg), "FPS override: %d.%03d", 
                current_fps/1000, current_fps%1000
                );
            NotifyBox(2000, msg);
            fps_warned = 1;
        }

        // 50D-specific warning when the FPS is incorrectly locked to 22, likely due to overheating
        // http://www.magiclantern.fm/forum/index.php?topic=6537.0
        #ifdef CONFIG_50D
        if (fps_warned && ((fps_get_current_x1000()/1000) == 22) && (fps_get_current_x1000() != fps_values_x1000[fps_override_index]) ) 
            NotifyBox(2000, "FPS warning, possible overheating!\n");
        #endif

        #ifdef FEATURE_EXPO_OVERRIDE
        if (CONTROL_BV && !is_movie_mode()) // changes in FPS may affect expsim calculations in photo mode
            bv_auto_update();
        #endif
    }
}

#ifdef CONFIG_500D
TASK_CREATE("fps_task", fps_task, 0, 0x17, 0x1000 );
#else
TASK_CREATE("fps_task", fps_task, 0, 0x1c, 0x1000 );
#endif
#endif

void fps_mvr_log(char* mvr_logfile_buffer)
{
    int f = fps_get_current_x1000();
    MVR_LOG_APPEND (
        "FPS            : %d.%03d\n", f/1000, f%1000
    );
}

// on certain events (PLAY, RECORD) we need to disable FPS override temporarily
int handle_fps_events(struct event * event)
{
    if (!get_fps_override()) return 1;
    
    #ifdef FEATURE_FPS_RAMPING
    if (FPS_RAMP && event->param == BGMT_INFO)
    {
        fps_ramp_up = !fps_ramp_up;
        #ifdef FEATURE_EXPO_PRESET
        handle_expo_preset(event); // will trigger both rampings if they are both enabled
        #endif
        return 0;
    }
    #endif
    
    if (event->param == BGMT_PLAY)
    {
        fps_register_reset();
    }
    
    // Very low FPS: first few frames will be recorded at normal FPS, to bypass Canon's internal checks
    // and to make the user interface responsive without having to wait for 30 frames
    int f = fps_values_x1000[fps_override_index];
    if (f < 5000 && NOT_RECORDING &&
    #if defined(CONFIG_50D) || defined(CONFIG_5D2)
        event->param == BGMT_PRESS_SET
    #else
        event->param == BGMT_LV
    #endif
    
    #if defined(NEW_FPS_METHOD)
    // we won't be able to change/restore FPS on the fly with table patching method :(
    && SENSOR_TIMING_TABLE != (uintptr_t) sensor_timing_table_patched
    #endif
    )
    {
        fps_register_reset();
    }


    return 1;
}

int fps_get_iso_correction_evx8()
{
#ifdef FRAME_SHUTTER_BLANKING_WRITE
    return 0;
#else
    if (!get_fps_override()) return 0;
    if (!fps_const_expo) return 0;
    if (!is_movie_mode()) return 0;
    if (!lens_info.raw_iso) return 0; // no auto iso

    int unaltered = (int)roundf(1000/raw2shutterf(MAX(lens_info.raw_shutter, 96)));
    int altered_by_fps = get_shutter_reciprocal_x1000(unaltered, fps_timer_a, fps_timer_a_orig, fps_timer_b, fps_timer_b_orig);
    float gf = 1.0f * altered_by_fps / unaltered;
    return log2f(gf)*8;
#endif
}

void fps_expo_iso_step()
{
#ifdef CONFIG_FRAME_ISO_OVERRIDE
#ifndef FRAME_SHUTTER_BLANKING_WRITE
    if (!lv) return;
    if (!lens_info.raw_iso) return; // no auto iso
    
    int mv = is_movie_mode();
    
    static int dirty = 0;
    if (mv) /* movie mode: only enable when it's selected from menu */
    {
        if (!(fps_const_expo && get_fps_override()))
        {
            if (dirty) set_movie_digital_iso_gain_for_gradual_expo(1024);
            return;
        }
    }
    else /* photo mode: always on if FPS is enabled and expo override is disabled */
    {
        if (!get_fps_override())
            return;
        
        if (CONTROL_BV) /* expo override will take care of it */
            return;
    }

    int tv = MAX(lens_info.raw_shutter, 96);
    #ifdef FRAME_SHUTTER
    tv = FRAME_SHUTTER & 0xFF;
    #endif
    int unaltered = (int)roundf(1000/raw2shutterf(tv));
    int altered_by_fps = get_shutter_reciprocal_x1000(unaltered, fps_timer_a, fps_timer_a_orig, fps_timer_b, fps_timer_b_orig);

    float gf = 1024.0f * altered_by_fps / unaltered;

    // adjust ISO just like in smooth_iso_step (copied from there)
    int current_iso = FRAME_ISO & 0xFF;
    int altered_iso = current_iso;
    
    int digic_iso_gain_movie = get_digic_iso_gain_movie();
    #define G_ADJ ((int)roundf(digic_iso_gain_movie ? gf * digic_iso_gain_movie / 1024 : gf))
    while (G_ADJ > 861*2 && altered_iso < MAX_ANALOG_ISO) 
    {
        altered_iso += 8;
        gf /= 2;
    }
    while ((G_ADJ < 861 && altered_iso > 80) || (altered_iso > MAX_ANALOG_ISO))
    {
        altered_iso -= 8;
        gf *= 2;
    }

    if (altered_iso != current_iso)
    {
        FRAME_ISO = altered_iso | (altered_iso << 8);
    }

    int g = (int)roundf(COERCE(gf, 1, 1<<20));
    if (g == 1024) g = 1025; // force override 

    if (mv) set_movie_digital_iso_gain_for_gradual_expo(g);
    dirty = 1;
#endif
#endif
}

#ifdef NEW_FPS_METHOD

static int get_fps_video_mode()
{
    int mode =
        lv_dispsize > 1 || get_expsim()!=2 ? 1 :
        video_mode_fps == 60 ? 3 : 
        video_mode_fps == 50 ? 6 : 
        video_mode_fps == 30 ? 1 : 
        video_mode_fps == 25 ? 5 : 
        video_mode_fps == 24 ? 4 : 0;
    return mode;
}

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
    
    if (get_expsim() != 2)
    {
        /* no crop mode in photo LV */
        crop_mode = 0;
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

static void fps_patch_timerB(int timer_value)
{
    int mode = get_fps_video_mode();   
    int pos = get_table_pos(mode, video_mode_crop, 0, lv_dispsize);

    if (sensor_timing_table_patched[pos] == timer_value && SENSOR_TIMING_TABLE == (uintptr_t) sensor_timing_table_patched)
        return;

    // at this point we are in previous FPS mode (maybe with timer A altered)

    fps_unpatch_table(0);
    fps_read_default_timer_values();
    EngDrvOutFPS(FPS_REGISTER_A, fps_reg_a_orig);
    EngDrvOutFPS(FPS_REGISTER_A, fps_reg_a_orig);

    flip_zoom_twostage(1);
    
    // at this point we are in some other video mode, at default fps

    for (int i = 0; i < COUNT(sensor_timing_table_patched); i++)
        sensor_timing_table_patched[i] = (i == pos) ? timer_value : sensor_timing_table_original[i];

    // use the patched sensor table
    SENSOR_TIMING_TABLE = (intptr_t) sensor_timing_table_patched;
    
    // no effect yet...
    
    flip_zoom_twostage(2);
    
    // now we are back to original video mode, at new FPS
}

static void sensor_timing_table_init()
{
    // make a copy of the original sensor timing table (so we can patch it)
    sensor_timing_table_original = (void*)SENSOR_TIMING_TABLE;
    memcpy(sensor_timing_table_patched, sensor_timing_table_original,  sizeof(sensor_timing_table_patched));
}

INIT_FUNC("sensor-timing", sensor_timing_table_init);

#endif // NEW_FPS_METHOD

int get_frame_iso()
{
    #ifdef FRAME_ISO
    return FRAME_ISO & 0xFF;
    #else
    return 0;
    #endif
}

void set_frame_iso(int iso)
{
    #ifdef CONFIG_FRAME_ISO_OVERRIDE
    FRAME_ISO = iso | (iso << 8);
    #endif
}

int can_set_frame_iso()
{
    #ifdef CONFIG_EOSM
    if (!RECORDING_H264) return 0;  /* EOS-M is stubborn, http://www.magiclantern.fm/forum/index.php?topic=5200.msg104816#msg104816 */
    #endif
    
    #ifdef CONFIG_FRAME_ISO_OVERRIDE
    return 1;
    #else
    return 0;
    #endif
}

int get_frame_shutter_timer()
{
    #ifdef FRAME_SHUTTER_TIMER
    return FRAME_SHUTTER_TIMER;
    #else
    return 0;
    #endif
}

void set_frame_shutter_timer(int timer)
{
    #ifdef CONFIG_FRAME_SHUTTER_OVERRIDE
        #ifdef CONFIG_DIGIC_V
        FRAME_SHUTTER_TIMER = MAX(timer, 2);
        #else
        FRAME_SHUTTER_TIMER = MAX(timer, 1);
        #endif
    #endif
}

void set_frame_shutter(int shutter_reciprocal)
{
    set_frame_shutter_timer(TG_FREQ_SHUTTER / shutter_reciprocal / 1000);
}

int can_set_frame_shutter_timer()
{
    #ifdef CONFIG_EOSM
    if (!RECORDING_H264) return 0;  /* EOS-M is stubborn, http://www.magiclantern.fm/forum/index.php?topic=5200.msg104816#msg104816 */
    #endif

    #ifdef CONFIG_FRAME_SHUTTER_OVERRIDE
    return 1;
    #else
    return 0;
    #endif
}

