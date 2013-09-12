/** \file
 * Common functions for image buffers
 * http://magiclantern.wikia.com/wiki/VRAM
 */

#include "dryos.h"
#include "property.h"
#include "propvalues.h"
#include "bmp.h"
#include "menu.h"

//~ #define CONFIG_DEBUGMSG 1

/**

BMP total: 960x540
BMP crop : 720x480

LCD: BMP and LV are usually 720x480, 1:1
HDMI: BMP may have half-resolution compared to LV, in one or both directions
___________________________________________________________________________________________________________________________
|HDMI: LV0,0     BMy-30|                                                                           |                      |
|                      |                                                                           |                      |
|BMx-120          BM0,0|                                                                    BMx720 |               BMx840 |
|----------------------+---------------------------------------------------------------------------+----------------------|
|                      |LCD: LV0,0                                                                 |                      |
|    os.x0,y0 _ _ _ _ _|_\ _____________________________________________________________________   |  _ _ _ _             |
|                      | /|___________________________16:9 gray/black bar_______________________|  |     A                |
|                      |  |                                                                     |  |     |                |
|                      |  |                                                                     |  |     |                |
|   here we usually    |  |                                                                     |  |     |                |
|   have pillarboxes   |  |                                                                     |  |     |                |
|   included in LV     |  |       HD (recording) buffer                                         |  |     |                |
|                      |  |       may or may nt include the 16:9 bars (camera-specific)         |  |     |                |
|                      |  |       but never includes pillarboxes                                |  |     |                |
|                      |  |                                                                     |  |     os.y_ex          |
|                      |  |                                                                     |  |     | (in BM space)  |
|                      |  |                                                                     |  |     |                |
|                      |  |                                                                     |  |     |                |
|                      |  |                                                                     |  |     |                |
|                      |  |_____________________________________________________________________|  |     |                |
|                      |  |_____________________________________________________________________|  |  _ _V_ _             |
|                      |  :                                                                     :  |                      |
|               BMy480 |  :<----------------------- os.x_ex (BM space) ------------------------>:  |                      |
|----------------------+---------------------------------------------------------------------------+----------------------|
|                      |                                                                           |                      |
|                      |                                                                           |                      |
|_______________BMy510_|___________________________________________________________________________|______________________|

*/

void update_vram_params();

// cached LUTs for BM2LV-like macros

int bm2lv_x_cache[BMP_W_PLUS - BMP_W_MINUS];
//~ int bm2n_x_cache [BMP_W_PLUS - BMP_W_MINUS];
//~ int bm2hd_r_cache[BMP_H_PLUS - BMP_H_MINUS];
int y_times_BMPPITCH_cache[BMP_H_PLUS - BMP_H_MINUS];

static void vram_update_luts()
{
    for (int x = BMP_W_MINUS; x < BMP_W_PLUS; x++) 
    {
        bm2lv_x_cache[x - BMP_W_MINUS] = BM2LV_Xu(x);
        //~ bm2n_x_cache[x - BMP_W_MINUS] = BM2N_Xu(x);
    }

    for (int y = BMP_H_MINUS; y < BMP_H_PLUS; y++) 
    {
        //~ bm2hd_r_cache[y - BMP_H_MINUS] = BM2HD_Ru(y);
        y_times_BMPPITCH_cache[y - BMP_H_MINUS] = y * BMPPITCH;
    }
}

struct vram_info vram_lv = {
    .pitch = 720 * 2,
    .width = 720,
    .height = 480,
};

struct vram_info vram_hd = {
    .pitch = 1056 * 2,
    .width = 1056,
    .height = 704,
};

/*struct vram_info vram_bm = {
    .pitch = 960,
    .width = 720,
    .height = 480,
};*/


struct trans2d bm2lv = { 
    .tx = 0,
    .ty = 0,
    .sx = 1024,
    .sy = 1024,
};

struct trans2d lv2hd = { 
    .tx = 0,
    .ty = 0,
    .sx = 2048, // dummy
    .sy = 2048, // dummy
};

struct trans2d lv2raw = {
    .tx = 0,
    .ty = 0,
    .sx = 2048,
    .sy = 2048,
};

// area from BMP where the LV image (3:2) is effectively drawn, without black bars
// in this area we'll draw cropmarks, zebras and so on
struct bmp_ov_loc_size os = {
    .x0 = 0,
    .y0 = 0,
    .x_ex = 480,
    .y_ex = 720,
};

//~ int lv_ratio_num = 3;
//~ int lv_ratio_den = 2;
//~ int hd_ratio_num = 3;
//~ int hd_ratio_den = 2;

static int vram_params_dirty = 1;
void vram_params_set_dirty()
{
    vram_params_dirty = 1;
    bmp_mute_flag_reset();
    afframe_set_dirty();
}

static uint32_t hd_size = 0;

static void vram_params_update_if_dirty()
{
    #ifdef REG_EDMAC_WRITE_LV_ADDR
    // EDMAC sizes may update after prop handlers, so check if their values changed
    // if so, recompute all VRAM params
    hd_size = shamem_read(REG_EDMAC_WRITE_HD_ADDR + 8);
    static uint32_t prev_hd_size = 0;
    if (prev_hd_size != hd_size) vram_params_dirty = 1;
    prev_hd_size = hd_size;
    #endif
    
    if (vram_params_dirty)
    {
        BMP_LOCK( 
            if (vram_params_dirty)
            {
                update_vram_params(); 
                vram_params_dirty = 0;
            }
        )
    }
}

#if CONFIG_DEBUGMSG

static int increment = 4;

int* vram_params[] = { 
    &increment,
    //~ &vram_bm.width, &vram_bm.height, 
    &os.x0, &os.y0, &os.x_ex, &os.y_ex, 
    &vram_lv.width, &vram_lv.height, 
    &bm2lv.tx, &bm2lv.ty, &bm2lv.sx, &bm2lv.sy,
    &vram_hd.width, &vram_hd.height, 
    //~ &hd_ratio_num, &hd_ratio_den ,
    &lv2hd.tx, &lv2hd.ty, &lv2hd.sx, &lv2hd.sy,
};
char vram_param_names[][12] = {
    "increment ",
    //~ "bmp.width ", "bmp.height",
    "os.x_left ", "os.y_top  ",
    "os.x_ex   ", "os.y_ex   ",
    "lv.width  ", "lv.height ",
    "bm2lv.tx  ", "bm2lv.ty  ",
    "bm2lv.sx  ", "bm2lv.sy  ",
    "hd.width  ", "hd.height ",
    //~ "ratio_num ", "ratio_den ",
    "lv2hd.tx* ", "lv2hd.ty* ",
    "lv2hd.sx* ", "lv2hd.sy* ",
};

#endif

static int digital_zoom_ratio = 0;
static int logical_connect=0;

PROP_HANDLER(PROP_DIGITAL_ZOOM_RATIO)
{
    digital_zoom_ratio = buf[0];
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_LOGICAL_CONNECT)
{
    logical_connect = buf[0];
    vram_params_set_dirty();
}

static PROP_INT(PROP_VIDEO_SYSTEM, pal);

void update_vram_params()
{
    #if CONFIG_DEBUGMSG
    if (is_menu_active("VRAM")) return;
    #endif
    //~ msleep(100); // just to make sure all prop handlers finished after mode change
    
    if (!ext_monitor_hdmi) hdmi_code = 0; // 5D doesn't revert it, maybe other cameras too

    // force a redraw when you connect the external monitor
    static int prev_hdmi_code = 0;
    static int prev_EXT_MONITOR_RCA = 0;
    if (prev_hdmi_code != hdmi_code || prev_EXT_MONITOR_RCA != EXT_MONITOR_RCA) redraw();
    prev_hdmi_code = hdmi_code;
    prev_EXT_MONITOR_RCA = EXT_MONITOR_RCA;
    
    // LV crop area (black bars)
    os.x0   = hdmi_code == 5 ?  75 - 120 : (hdmi_code == 2 ? 40 : EXT_MONITOR_RCA ? (pal ? 40 : 40) :    0);
    os.y0   = hdmi_code == 5 ?   0 - 30  : (hdmi_code == 2 ? 24 : EXT_MONITOR_RCA ? (pal ? 29 : 25) :    0);
    os.x_ex = hdmi_code == 5 ? 810 : (hdmi_code == 2 || EXT_MONITOR_RCA) ? 640 : 720;
    os.y_ex = hdmi_code == 5 ? 540 : (hdmi_code == 2 || EXT_MONITOR_RCA) ? 388 : 480;
#if defined(CONFIG_4_3_SCREEN)
    if (!EXT_MONITOR_CONNECTED)
    {
        if (PLAY_MODE || QR_MODE)
        {
            os.y0   = 52; // black bar is at the top in play mode, 48 with additional info
            os.y_ex = 428; // 480 - os.y0; // screen height is 480px in total
        }
        else
        {
            os.y_ex = 424; // 480 * 8/9; // BMP is 4:3, image is 3:2;
        }
    }
#else
    if (PLAY_MODE && hdmi_code == 2)
    {
        os.y_ex = 480 - 52;
        os.y0 = 52;
    }
#endif
    
    os.x_max = os.x0 + os.x_ex;
    os.y_max = os.y0 + os.y_ex;
    os.off_43 = (os.x_ex - os.x_ex * 8/9) / 2;
    os.off_169 = (os.y_ex - os.y_ex * 3/2 * 9/16) / 2;
    os.off_1610 = (os.y_ex - os.y_ex * 3/2 * 10/16) / 2;

    // LV buffer (used for display)
    // these buffer sizes include any black bars

#if defined(CONFIG_5DC)
    vram_lv.width = 540;
    vram_lv.height = 426;
    vram_lv.pitch = vram_lv.width * 2;
    os.x0 = 0; os.y0 = 26;
    os.x_ex = 720;
    os.y_ex = 480-52;
    os.x_max = os.x0 + os.x_ex;
    os.y_max = os.y0 + os.y_ex;
    os.off_169 = 0;
    os.off_1610 = 0;
#elif defined(CONFIG_40D)
    //~ vram_lv.width = 720; // we only know the HD buffer for now... let's try to pretend it can be used as LV :)
    //~ vram_lv.height = 480;
    // we only know the HD buffer for now... let's try to pretend it can be used as LV :)
    vram_lv.width = 768; // real width is 1024 in yuv411, but ML code assumes yuv422
    vram_lv.height = 680;
    vram_lv.pitch = vram_lv.width * 2;    
    os.x0 = 0;
    //~ os.y0 = 0;
    os.y0 = (PLAY_MODE || QR_MODE)? 48 : 0; 
    os.x_ex = 720;
    //~ os.y_ex = 480;
    os.y_ex = 480 - os.y0;    
    os.x_max = os.x0 + os.x_ex;
    os.y_max = os.y0 + os.y_ex;
    os.off_169 = 0;
    os.off_1610 = 0;     
    //~ os.off_169 = (os.y_ex - os.y_ex * 4/3 * 9/16) / 2;
    //~ os.off_1610 = (os.y_ex - os.y_ex * 4/3 * 10/16) / 2;
       
#else
    #ifdef CONFIG_1100D
        vram_lv.width  = 720;
        vram_lv.height = 240;
    #else
        vram_lv.width  = hdmi_code == 5 ? (is_movie_mode() && video_mode_resolution > 0 && video_mode_crop ? 960 : 1920) : EXT_MONITOR_RCA ? 540 : 720;
        vram_lv.height = hdmi_code == 5 ? (is_movie_mode() && video_mode_fps > 30                          ? 540 : 1080) : EXT_MONITOR_RCA ? (pal ? 572 : 480) : 480;
    #endif
    vram_lv.pitch = vram_lv.width * 2;
#endif


#ifdef CONFIG_5DC
    bm2lv.sx = 1024 * vram_lv.width / 720;
    bm2lv.sy = 1024 * vram_lv.height / (480-52);
    bm2lv.tx = 0;
    bm2lv.ty = -26;
#elif CONFIG_40D
    bm2lv.sx = 1024 * vram_lv.width / 720;
    bm2lv.sy = 1024 * vram_lv.height / 480;
    //~ bm2lv.sy = 1024 * vram_lv.height / (480-48);    
    bm2lv.tx = 0;
    bm2lv.ty = 0;
    //~ bm2lv.ty = (PLAY_MODE || QR_MODE)? -48 : 0;     
#else
    // bmp to lv transformation
    // LCD: (0,0) -> (0,0)
    // HDMI: (-120,-30) -> (0,0) and scaling factor is 2
    bm2lv.tx = hdmi_code == 5 ?  240 : EXT_MONITOR_RCA ? 4 : 0;
    bm2lv.ty = hdmi_code == 5 ? (video_mode_resolution>0 ? 30 : 60) : 0;
    bm2lv.sx = hdmi_code == 5 ? 2048 : EXT_MONITOR_RCA ? 768 : 1024;
    bm2lv.sy = 1024 * vram_lv.height / (hdmi_code==5 ? 540 : 480); // no black bars at top or bottom
#endif
    
    //~ lv_ratio_num = hdmi_code == 5 ? 16 : 3;
    //~ lv_ratio_den = hdmi_code == 5 ?  9 : 2;

    // HD buffer (used for recording)
    //~ hd_ratio_num = recording ? (video_mode_resolution < 2 ? 16 : 4) : 3;
    //~ hd_ratio_den = recording ? (video_mode_resolution < 2 ?  9 : 3) : 2;

#if defined(CONFIG_40D)
    vram_hd.width = vram_lv.width;
    vram_hd.height = vram_lv.height;
    vram_hd.pitch = vram_lv.pitch;
    //~ vram_hd.width = 1024;
    //~ vram_hd.height = 680;
    //~ vram_hd.pitch = vram_hd.width * 2;
#elif defined(CONFIG_5DC)
    vram_hd.width = 1872;
    vram_hd.height = 1664;
    vram_hd.pitch = vram_lv.pitch;
#else
    vram_hd.pitch = hd_size & 0xFFFF;
    vram_hd.width = vram_hd.pitch / 2;
    vram_hd.height = ((hd_size >> 16) & 0xFFFF)
        #if !defined(CONFIG_DIGIC_V)
        + 1
        #endif
        ;
#endif

    int off_43 = (os.x_ex - os.x_ex * 8/9) / 2;

    // gray bars for 16:9 or 4:3
    #if defined(CONFIG_600D)
    int bar_x = is_movie_mode() && video_mode_resolution >= 2 ? off_43 : 0;
    int bar_y = is_movie_mode() && video_mode_resolution <= 1 ? os.off_169 : 0;
    #elif defined(CONFIG_1100D) || defined(CONFIG_DIGIC_V)
    int bar_x = 0;
    int bar_y = is_movie_mode() && video_mode_resolution == 1 ? os.off_169 : 0;
    off_43+=0; // bypass warning
    #elif defined(CONFIG_500D) || defined(CONFIG_7D) //TODO: 650D/6D/EOSM used to have this one enabled too...which one is correct?
    int bar_x = 0;
    int bar_y = 0;
    off_43+=0; // bypass warning
    #else
    int bar_x = recording && video_mode_resolution >= 2 ? off_43 : 0;
    int bar_y = recording && video_mode_resolution <= 1 ? os.off_169 : 0;
    #endif

    vram_update_luts();

    lv2hd.sx = 1024 * vram_hd.width / BM2LV_DX(os.x_ex - bar_x * 2);
    lv2hd.sy = 1024 * vram_hd.height / BM2LV_DY(os.y_ex - bar_y * 2);
    
    // HD buffer does not contain pillarboxes, LV does
    // and HD may or may not contain bars 
    // the offset needs to be specified in HD units
    lv2hd.tx = -LV2HD_DX(BM2LV_X(os.x0 + bar_x));
    lv2hd.ty = -LV2HD_DY(BM2LV_Y(os.y0 + bar_y));

//~ #ifndef CONFIG_5DC
    if (!lv) // HD buffer not active, use LV instead
    {
        lv2hd.sx = lv2hd.sy = 1024;
        lv2hd.tx = lv2hd.ty = 0;
        vram_hd.pitch = vram_lv.pitch;
        vram_hd.width = vram_lv.width;
        vram_hd.height = vram_lv.height;
    }
//~ #endif

    vram_update_luts();
}


/*void trans_test()
{
    for (int i = 0; i < 1000; i += 10)
    {
        // should say almost "i"
        bmp_printf(FONT_MED, 50, 50, "%d => %d %d %d %d %d %d ", 
            i,
            BM2LV_X(LV2BM_X(i)), 
            BM2LV_Y(LV2BM_Y(i)),
            LV2HD_X(HD2LV_X(i)), 
            LV2HD_Y(HD2LV_Y(i)), 
            BM2HD_X(HD2BM_X(i)), 
            BM2HD_Y(HD2BM_Y(i))
        );
        msleep(300);
    }
}*/

/*
int* lut_bm2lv_x = 0;
int* lut_lv2bm_x = 0;

int* lut_lv2hd_x = 0;
int* lut_hd2lv_x = 0;

int* lut_bm2hd_x = 0;
int* lut_hd2bm_x = 0;

int* lut_bm2lv_y = 0;
int* lut_lv2bm_y = 0;

int* lut_lv2hd_y = 0;
int* lut_hd2lv_y = 0;

int* lut_bm2hd_y = 0;
int* lut_hd2bm_y = 0;

void lut_realloc(int** buf, int size)
{
    if (*buf) FreeMemory(*buf);
    *buf = AllocateMemory(size);
}

void lut_init()
{
    lut_realloc(&lut_bm2lv_x, SL.BM.W);
    lut_realloc(&lut_bm2lv_y, SL.BM.H);

    lut_realloc(&lut_lv2bm_x, SL.LV.W);
    lut_realloc(&lut_lv2bm_y, SL.LV.H);

    lut_realloc(&lut_lv2bm);
}*/

#include "bmp.h"


static inline void * get_yuv422buffer(int offset)
{
    #if defined(CONFIG_1100D) || defined(CONFIG_6D)
    return (void*)CACHEABLE(YUV422_LV_BUFFER_DISPLAY_ADDR); // Good enough
    #else
    if (YUV422_LV_BUFFER_DISPLAY_ADDR == YUV422_LV_BUFFER_1)
       offset += 0;
    else if (YUV422_LV_BUFFER_DISPLAY_ADDR == YUV422_LV_BUFFER_2)
       offset += 1;
    else
       offset += 2;

    switch (offset)
    {
        case 0:
        case 3:
        default:
           return (void*)CACHEABLE(YUV422_LV_BUFFER_1);

        case 1:
        case 4:
           return (void*)CACHEABLE(YUV422_LV_BUFFER_2);
        case 2:
        case 5:
           return (void*)CACHEABLE(YUV422_LV_BUFFER_3);
    }
    #endif
}


void* get_lcd_422_buf()
{
    return get_yuv422buffer(0);
}

static int fastrefresh_direction = 0;

static unsigned old_buffer_pos = 0;

void guess_fastrefresh_direction() {
    if (old_buffer_pos == YUV422_LV_BUFFER_DISPLAY_ADDR) return;
    if (old_buffer_pos == YUV422_LV_BUFFER_1 && YUV422_LV_BUFFER_DISPLAY_ADDR == YUV422_LV_BUFFER_2) fastrefresh_direction = 1;
    if (old_buffer_pos == YUV422_LV_BUFFER_1 && YUV422_LV_BUFFER_DISPLAY_ADDR == YUV422_LV_BUFFER_3) fastrefresh_direction = 0;
    old_buffer_pos = YUV422_LV_BUFFER_DISPLAY_ADDR;
}


void* get_fastrefresh_422_buf()
{
    return get_yuv422buffer(fastrefresh_direction ? 1 : 2);
}

// Unfortunately this doesn't work on every 1100D model yet :(
static void* get_fastrefresh_422_other_buf()
{
    return get_yuv422buffer(fastrefresh_direction ? 2 : 1);
}

#ifdef CONFIG_500D
int first_video_clip = 1;
#endif


struct vram_info * get_yuv422_vram()
{
    vram_params_update_if_dirty();
    
    if (digic_zoom_overlay_enabled()) // compute histograms and such on full-screen image
    {
        vram_lv.vram = (void*)CACHEABLE(YUV422_LV_BUFFER_1);
        return &vram_lv;
    }

    #ifdef CONFIG_DISPLAY_FILTERS
    int d = display_filter_enabled();
    if (d)
    {
        uint32_t* src_buf;
        uint32_t* dst_buf;
        display_filter_get_buffers(&src_buf, &dst_buf);
        vram_lv.vram = (void*)(d == 1 ? dst_buf : src_buf);
        return &vram_lv;
    }
    #endif

    #ifdef CONFIG_500D // workaround for issue 1108 - zebras flicker on first clip
    
    if (lv && !is_movie_mode()) first_video_clip = 0; // starting in photo mode is OK
    
    if (first_video_clip)
    {
        vram_lv.vram = CACHEABLE(get_lcd_422_buf());
        return &vram_lv;
    }
    #endif

    extern int lv_paused;
    if (gui_state == GUISTATE_PLAYMENU || lv_paused || QR_MODE)
        vram_lv.vram = CACHEABLE(get_lcd_422_buf());
    else
        vram_lv.vram = CACHEABLE(get_fastrefresh_422_buf());
    return &vram_lv;
}

struct vram_info * get_yuv422_hd_vram()
{
    vram_params_update_if_dirty();

//~ #ifndef CONFIG_5DC
    if (!lv) // play/quickreview, HD buffer not active => use LV instead
        return get_yuv422_vram();
//~ #endif

    vram_hd.vram = CACHEABLE(YUV422_HD_BUFFER_DMA_ADDR);
    return &vram_hd;
}

void vram_clear_lv()
{
    struct vram_info * lv_vram = get_yuv422_vram();
    memset(lv_vram->vram, 0, lv_vram->height * lv_vram->pitch);
}

// those properties may signal a screen layout change

PROP_HANDLER(PROP_HDMI_CHANGE)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_HDMI_CHANGE_CODE)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_USBRCA_MONITOR)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_LV_DISPSIZE)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_MVR_REC_START)
{
    vram_params_set_dirty();
    
    #ifdef CONFIG_500D
    static int prev;
    if (prev && !buf[0]) first_video_clip = 0;
    prev = buf[0];
    #endif
}

PROP_HANDLER(PROP_SHOOTING_TYPE)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_LV_ACTION)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_GUI_STATE)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_VIDEO_MODE)
{
    vram_params_set_dirty();
}

PROP_HANDLER(PROP_LV_MOVIE_SELECT)
{
    vram_params_set_dirty();
}

#if CONFIG_DEBUGMSG

static void
vram_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    unsigned        menu_font = selected ? FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK) : MENU_FONT;

    bmp_printf(
        menu_font,
        x, y,
        "%s = %d",
        vram_param_names[(int)priv], MEM(vram_params[(int)priv])
    );
    menu_draw_icon(x,y,MNI_NONE,0);
}

static void vram_toggle(void* priv, int delta)
{
    MEM(vram_params[(int)priv]) += priv ? delta : SGN(delta);
    crop_set_dirty(1);
    //~ update_vram_params_calc();
}

static void vram_toggle_fwd(void* priv, int unused) { vram_toggle(priv,  increment); }
static void vram_toggle_rev(void* priv, int unused) { vram_toggle(priv, -increment); }
//~ static void vram_toggle_delta(void* priv)  { menu_quinternary_toggle(&increment, 1); }

#define VRAM_MENU_ENTRY(x)  { \
        .priv = (void *) x, \
        .update    = vram_print, \
        .select     = vram_toggle, \
        .select_Q = vram_toggle_rev, \
    }, \

static struct menu_entry vram_menus[] = {
    VRAM_MENU_ENTRY(0)
    VRAM_MENU_ENTRY(1)
    VRAM_MENU_ENTRY(2)
    VRAM_MENU_ENTRY(3)
    VRAM_MENU_ENTRY(4)
    VRAM_MENU_ENTRY(5)
    VRAM_MENU_ENTRY(6)
    VRAM_MENU_ENTRY(7)
    VRAM_MENU_ENTRY(8)
    VRAM_MENU_ENTRY(9)
    VRAM_MENU_ENTRY(10)
    VRAM_MENU_ENTRY(11)
    VRAM_MENU_ENTRY(12)
    VRAM_MENU_ENTRY(13)
    VRAM_MENU_ENTRY(14)
    VRAM_MENU_ENTRY(15)
    VRAM_MENU_ENTRY(16)
    //~ VRAM_MENU_ENTRY(17)
    //~ VRAM_MENU_ENTRY(18)
    //~ VRAM_MENU_ENTRY(19)
    //~ VRAM_MENU_ENTRY(20)
};

void vram_init()
{
    menu_add("VRAM", vram_menus, COUNT(vram_menus));
    old_buffer_pos = YUV422_LV_BUFFER_1;
}

INIT_FUNC(__FILE__, vram_init);
#endif
