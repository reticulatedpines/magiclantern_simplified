#include "dryos.h"
#include "raw.h"
#include "property.h"
#include "math.h"
#include "bmp.h"
#include "lens.h"

#undef RAW_DEBUG        /* define it to help with porting */
#undef RAW_DEBUG_DUMP   /* if you want to save the raw image buffer and the DNG from here */
#undef RAW_DEBUG_BLACK  /* for checking black level calibration */
/* see also RAW_ZEBRA_TEST and RAW_SPOTMETER_TEST in zebra.c */

#ifdef RAW_DEBUG
#define dbg_printf(fmt,...) { console_printf(fmt, ## __VA_ARGS__); }
#else
#define dbg_printf(fmt,...) {}
#endif

static int shave_right = 0;

/*********************** Camera-specific constants ****************************/

/**
 * LiveView raw buffer address
 * To find it, call("lv_save_raw") and look for an EDMAC channel that becomes active (Debug menu)
 **/

#if defined(CONFIG_5D2) || defined(CONFIG_50D)
#define RAW_LV_EDMAC 0xC0F04508
#endif

#if defined(CONFIG_500D) || defined(CONFIG_550D)
#define RAW_LV_EDMAC 0xC0F26008
#endif

#if defined(CONFIG_5D3) || defined(CONFIG_6D) || defined(CONFIG_650D) || defined(CONFIG_600D) || defined(CONFIG_60D) || defined(CONFIG_EOSM)
/* probably all new cameras use this address */
#define RAW_LV_EDMAC 0xC0F26208
#endif

/**
 * Photo-mode raw buffer address
 * On old cameras, it can be intercepted from SDSf3 state object, right after sdsMem1toRAWcompress.
 * On new cameras, use the SSS state, sssCompleteMem1ToRaw.
 * 
 * See state-object.c for intercepting code,
 * and http://a1ex.bitbucket.org/ML/states/ for state diagrams.
 */

#if defined(CONFIG_5D2) || defined(CONFIG_500D) || defined(CONFIG_600D) || defined(CONFIG_650D) || defined(CONFIG_EOSM) || defined(CONFIG_50D)
#define RAW_PHOTO_EDMAC 0xc0f04A08
#endif

#if defined(CONFIG_5D3) || defined(CONFIG_6D)
#define RAW_PHOTO_EDMAC 0xc0f04808
#endif

#if defined(CONFIG_60D) || defined (CONFIG_550D)
#define RAW_PHOTO_EDMAC 0xc0f04208
#endif

static uint32_t raw_buffer_photo = 0;

/* called from state-object.c, SDSf3 or SSS state */
void raw_buffer_intercept_from_stateobj()
{
    /**
     * will grab the RAW image buffer address and hope it doesn't change
     * 
     * with dm-spy log:
     * 5D2: [TTJ] START RD1:0x4000048 RD2:0x64d1864
     * 5D3: [TTL] START RD1:0x8602914 RD2:0xad24490
     * 
     * don't use the value from debug logs, since it will change after a few pics;
     * look it up on the EDMAC registers and use that one instead.
     */
    raw_buffer_photo = shamem_read(RAW_PHOTO_EDMAC);


}

/** 
 * Raw type (optional)
 * decompile lv_af_raw
 * => (5D3) lv_set_raw_type(arg0 ? 4 : 7)
 * => MEM(0x2D168) = a bunch of values, default 34, 18 with lv_af on, 14 with lv_af off.
 * see also http://www.magiclantern.fm/forum/index.php?topic=5614.msg39696#msg39696
 */

#ifdef CONFIG_5D3
/**
 * Renato [http://www.magiclantern.fm/forum/index.php?topic=5614.msg41070#msg41070]:
 * "Best images in: 17, 35, 37, 39, 81, 83, 99"
 * note: values are off by 1
 */
#define PREFERRED_RAW_TYPE 16
#define RAW_TYPE_ADDRESS 0x2D168
#endif

#ifdef CONFIG_5D2
/* a.d.: without lv_af_raw, 5D2 has magenta cast in zoom mode */
/* af raw is actually edge detection for focusing (nanomad) */
// #define USE_LV_AF_RAW
#endif

/** 
 * White level
 * one size fits all: should work on most cameras and can't be wrong by more than 0.1 EV
 */
#define WHITE_LEVEL 15000

/** there may be exceptions */
#ifdef CONFIG_6D
#undef WHITE_LEVEL
#define WHITE_LEVEL 13000
#endif

/**
 * Color matrix should be copied from DCRAW.
 * It will also work with the values from some other camera, but colors may be a little off.
 **/

#ifdef CONFIG_5D2
    //~ { "Canon EOS 5D Mark II", 0, 0x3cf0,
    //~ { 4716,603,-830,-7798,15474,2480,-1496,1937,6651 } },
    #define CAM_COLORMATRIX1                       \
     4716, 10000,      603, 10000,    -830, 10000, \
    -7798, 10000,    15474, 10000,    2480, 10000, \
    -1496, 10000,     1937, 10000,    6651, 10000
#endif

#ifdef CONFIG_5D3
    //~ { "Canon EOS 5D Mark III", 0, 0x3c80,
    //~ { 6722,-635,-963,-4287,12460,2028,-908,2162,5668 } },
    #define CAM_COLORMATRIX1                       \
     6722, 10000,     -635, 10000,    -963, 10000, \
    -4287, 10000,    12460, 10000,    2028, 10000, \
     -908, 10000,     2162, 10000,    5668, 10000
#endif

#ifdef CONFIG_550D
   //~ { "Canon EOS 550D", 0, 0x3dd7,
   //~	{  6941,-1164,-857,-3825,11597,2534,-416,1540,6039 } },
    #define CAM_COLORMATRIX1                        \
      6461, 10000,     -1164, 10000,    -857, 10000,\
     -3825, 10000,     11597, 10000,    2534, 10000,\
      -416, 10000,      1540, 10000,    6039, 10000
#endif

#ifdef CONFIG_6D
    //~ { "Canon EOS 6D", 0, 0,
    //~ { 7034,-804,-1014,-4420,12564,2058,-851,1994,5758 } },
    #define CAM_COLORMATRIX1                       \
     7034, 10000,     -804, 10000,    -1014, 10000,\
    -4420, 10000,    12564, 10000,    2058, 10000, \
     -851, 10000,     1994, 10000,    5758, 10000
#endif

#ifdef CONFIG_500D
    //~ { "Canon EOS 500D", 0, 0x3479,
    //~ { 4763,712,-646,-6821,14399,2640,-1921,3276,6561 } },
    #define CAM_COLORMATRIX1                       \
     4763, 10000,      712, 10000,    -646, 10000, \
    -6821, 10000,    14399, 10000,    2640, 10000, \
    -1921, 10000,     3276, 10000,    6561, 10000
#endif

#ifdef CONFIG_600D
	//~ { "Canon EOS 600D", 0, 0x3510,
	//~ { 6461,-907,-882,-4300,12184,2378,-819,1944,5931 } },
    #define CAM_COLORMATRIX1                       \
      6461, 10000,     -907, 10000,    -882, 10000,\
    -4300, 10000,    12184, 10000,    2378, 10000, \
     -819, 10000,     1944, 10000,    5931, 10000
#endif

#ifdef CONFIG_60D
        //~ { "Canon EOS 60D", 0, 0x2ff7,
        //~ {  6719,-994,-925,-4408,12426,2211,-887,2129,6051 } },
    #define CAM_COLORMATRIX1                       \
      6719, 10000,     -994, 10000,    -925, 10000,\
    -4408, 10000,    12426, 10000,    2211, 10000, \
     -887, 10000,     2129, 10000,    6051, 10000
#endif

#ifdef CONFIG_50D // these values are in ufraw-0.19.2
    //~{ "Canon EOS 50D", 0, 0x3d93,
	//~{ 4920,616,-593,-6493,13964,2784,-1774,3178,7005 } }, 
    #define CAM_COLORMATRIX1                       \
     4920, 10000,      616, 10000,    -593, 10000, \
    -6493, 10000,    12964, 10000,    2784, 10000, \
    -1774, 10000,     3178, 10000,    7005, 10000
#endif
	
#if defined(CONFIG_650D) || defined(CONFIG_EOSM) //Same sensor??
    //~ { "Canon EOS 650D", 0, 0x354d,
    //~ { "Canon EOS M", 0, 0,
    //~ { 6602,-841,-939,-4472,12458,2247,-975,2039,6148 } },
	#define CAM_COLORMATRIX1                     \
     6602, 10000,     -841, 10000,    -939, 10000,\
    -4472, 10000,    12458, 10000,    2247, 10000, \
     -975, 10000,     2039, 10000,    6148, 10000
#endif

struct raw_info raw_info = {
    .api_version = 1,
    .bits_per_pixel = 14,
    .black_level = 1024,
    .white_level = 13000,
    .cfa_pattern = 0x02010100,          // Red  Green  Green  Blue
    .calibration_illuminant1 = 1,       // Daylight
    .color_matrix1 = {CAM_COLORMATRIX1},// camera-specific, from dcraw.c
    .dynamic_range = 1100,              // not correct; use numbers from DxO instead
};

static int autodetect_black_level();

int raw_update_params()
{
    #ifdef RAW_DEBUG
    console_show();
    #endif
    
    int width = 0;
    int height = 0;
    int skip_left = 0;
    int skip_right = 0;
    int skip_top = 0;
    int skip_bottom = 0;
    
    /* params useful for hardcoding buffer sizes, according to video mode */
    int mv = is_movie_mode();
    int mv720 = mv && video_mode_resolution == 1;
    int mv1080 = mv && video_mode_resolution == 0;
    int mv640 = mv && video_mode_resolution == 2;
    int mv1080crop = mv && video_mode_resolution == 0 && video_mode_crop;
    int mv640crop = mv && video_mode_resolution == 2 && video_mode_crop;
    int zoom = lv_dispsize > 1;
    
    /* silence warnings; not all cameras have all these modes */
    (void)mv640; (void)mv720; (void)mv1080; (void)mv640; (void)mv1080crop; (void)mv640crop;

    if (lv)
    {
        /* grab the image buffer from EDMAC; first pixel should be red */
        raw_info.buffer = (void*) shamem_read(RAW_LV_EDMAC);
        if (!raw_info.buffer)
        {
            dbg_printf("LV raw buffer null\n");
            return 0;
        }

        /* autodetect raw size from EDMAC */
        uint32_t lv_raw_height = shamem_read(RAW_LV_EDMAC+4);
        uint32_t lv_raw_size = shamem_read(RAW_LV_EDMAC+8);
        if (!lv_raw_size)
        {
            dbg_printf("LV RAW size null\n");
            return 0;
        }
        int pitch = lv_raw_size & 0xFFFF;
        width = pitch * 8 / 14;
        
        /* 5D2 uses lv_raw_size >> 16, 5D3 uses lv_raw_height, so this hopefully covers both cases */
        height = MAX((lv_raw_height & 0xFFFF) + 1, ((lv_raw_size >> 16) & 0xFFFF) + 1);

        /** 
         * The RAW file has unused areas, usually black; we need to skip them.
         *
         * To find the skip values, start with 0,
         * load the RAW in your favorite photo editor (e.g. ufraw+gimp),
         * then find the usable area, read the coords and plug the skip values here.
         * 
         * Try to use even offsets only, otherwise the colors will be screwed up.
         */
        #ifdef CONFIG_5D2
        skip_top        = zoom ?   50 :   18;
        skip_left       = 160;
        #endif
        
        #ifdef CONFIG_5D3
        skip_top        = zoom ?   60 : mv720 ?  20 :   30;
        skip_left       = 146;
        skip_right      = 6;
        #endif

        #ifdef CONFIG_600D
        //  raw_info.height = mv1080crop ? 1042 : zoom ? 1100 : mv720 ? 714 : 1176;
        skip_top        =  26;
        skip_left       = zoom ?   0 : 152;
        skip_right      = zoom ?   0 : 2;
        #endif        

        #ifdef CONFIG_6D
        //~ raw_info.height = zoom ? 980 : mv720 ? 656 : 1244;
        skip_top        = zoom ? 30 : mv720 ? 28 : 28; //28
        skip_left       = zoom ? 84 : mv720 ? 86: 86; //86
        skip_right      = zoom ? 0  : mv720 ? 12 : 10;
        //~ skip_bottom = 1;
        #endif

        #ifdef CONFIG_500D
        skip_top    = 24;
        skip_left   = zoom ? 64 : 74;
        #endif

        #if defined(CONFIG_550D) || defined(CONFIG_600D)
        skip_top    = 26;
        skip_left   = zoom ? 0 : 152;
        skip_right  = zoom ? 0 : 2;
        #endif

        #ifdef CONFIG_60D
        skip_top    = 26;
        skip_left   = zoom ? 0 : 152;
        skip_right  = zoom ? 0 : 2;
        #endif

        #ifdef CONFIG_50D
		skip_top    = zoom ? 0 : 26;
		skip_left   = 74;
        skip_right  = 0;
        skip_bottom = 0;
        #endif
		
        #if defined(CONFIG_650D) || defined(CONFIG_EOSM)
        skip_top    = 28;
        skip_left   = 74;
        skip_right  = 0;
        skip_bottom = 4;
        #endif
        
        if (shave_right)
        {
            width -= shave_right;
            skip_right = MAX(0, skip_right - shave_right);
        }
        
        dbg_printf("LV raw buffer: %x (%dx%d)\n", raw_info.buffer, width, height);
        dbg_printf("Skip left:%d right:%d top:%d bottom:%d\n", skip_left, skip_right, skip_top, skip_bottom);
    }
    else if (QR_MODE) // image review after taking pics
    {
        raw_info.buffer = (void*) raw_buffer_photo;
        
        #ifdef CONFIG_60D
        raw_info.buffer = (void*) shamem_read(RAW_PHOTO_EDMAC);
        #endif
        
        if (!raw_info.buffer)
        {
            dbg_printf("Photo raw buffer null\n");
            return 0;
        }
        
        /**
         * Raw buffer size for photos
         * Usually it's slightly larger than what raw converters will tell you.
         * 
         * Width value is critical (if incorrect, the image will be heavily distorted).
         * Height is not critical.
         *
         * I've guessed the image width by dumping the raw buffer, and then using FFT to guess the period of the image stream.
         * 
         * 1) define RAW_DEBUG_DUMP
         * 
         * 2) load raw.buf into img.py and run guesspitch, details here: http://magiclantern.wikia.com/wiki/VRAM/550D 
         * 
         *          In [4]: s = readseg("raw.buf", 0, 30000000)
         * 
         *          In [5]: guesspitch(s)
         *          3079
         *          9743.42318935
         * 
         *          In [6]: 9743*8/14
         *          Out[6]: 5567
         * 
         *          Then, trial and error => 5568.
         *
         * Also, the RAW file has unused areas, usually black; we need to skip them.
         * 
         * Start with 0, then load the RAW in your favorite photo editor (e.g. ufraw+gimp),
         * then find the usable area, read the coords and plug the skip values here.
         * 
         * Try to use even offsets only, otherwise the colors will be screwed up.
         */
        
        #ifdef CONFIG_5D2
        /* from debug log: [TTJ][150,27089,0] RAW(5792,3804,0,14) */
        width = 5792;
        height = 3804;
        skip_left = 160;
        skip_top = 54;
        /* first pixel should be red, but here it isn't, so we'll skip one line */
        /* also we have a 16-pixel border on the left that contains image data */
        raw_info.buffer += width * 14/8 + 16*14/8;
        #endif

        #ifdef CONFIG_5D3
        /* it's a bit larger than what the debug log says: [TTL][167,9410,0] RAW(5920,3950,0,14) */
        width = 5936;
        height = 3950;
        skip_left = 126;
        skip_right = 20;
        skip_top = 80;
        #endif

        #ifdef CONFIG_550D
        width = 5344;
        height = 3516;
        skip_left = 142;
        skip_right = 18;
        skip_top = 58;
        skip_bottom = 10;
        #endif

        #ifdef CONFIG_600D
        width = 5344; //From Guess Py
        height = 3465;
        skip_left = 152;
        skip_right = 10;
        skip_top = 56;
        #endif

        #ifdef CONFIG_6D  //Needs check from Raw dump but looks aligned.
        width = 5568;
        height = 3708;
        skip_left = 84; //Meta Data
        skip_right = 14;
        skip_top = 50; // Meta Data
        #endif

        #if defined(CONFIG_60D)
        width = 5344;
        height = 3516;
        skip_left = 142;
        skip_right = 0;
        skip_top = 50;
        #endif

/*        #if defined(CONFIG_50D) NEED Raw dump to get correct values
        width = 5344;
        height = 3516;
        skip_left = 142;
        skip_right = 0;
        skip_top = 50;
        #endif 
*/

        #if defined(CONFIG_650D) || defined(CONFIG_EOSM)
        width = 5280;
        height = 3528;
        skip_left = 72;
        skip_top = 52;
        #endif

        dbg_printf("Photo raw buffer: %x (%dx%d)\n", raw_info.buffer, width, height);
        dbg_printf("Skip left:%d right:%d top:%d bottom:%d\n", skip_left, skip_right, skip_top, skip_bottom);
    }
    else
    {
        dbg_printf("Neither LV nor QR\n");
        return 0;
    }

/*********************** Portable code ****************************************/

    raw_set_geometry(width, height, skip_left, skip_right, skip_top, skip_bottom);

    raw_info.white_level = WHITE_LEVEL;

    if (!lv)
    {
        /* at ISO 160, 320 etc, the white level is decreased by -1/3 EV */
        /* in LiveView, it doesn't change */
        int iso = 0;
        if (!iso) iso = lens_info.raw_iso;
        if (!iso) iso = lens_info.raw_iso_auto;
        int iso_rounded = COERCE((iso + 3) / 8 * 8, 72, 200);
        float iso_digital = (iso - iso_rounded) / 8.0f;
        if (iso_digital <= 0)
            raw_info.white_level *= powf(2, iso_digital);
    }
    else if (!is_movie_mode())
    {
        /* in photo mode, LV iso is not equal to photo ISO because of ExpSim *
         * the digital ISO will not change the raw histogram
         * but we want the histogram to mimic the CR2 one as close as possible
         * so we do this by compensating the white level manually
         * warning: this may exceed 16383!
         */
        int shad_gain = shamem_read(0xc0f08030);
        
        /* LV histogram seems to be underexposed by 0.15 EV compared to photo one,
         * so we compensate for that too (4096 -> 3691) */
        raw_info.white_level = raw_info.white_level * 3691 / shad_gain;
    }

    raw_info.black_level = autodetect_black_level();
    
    dbg_printf("black=%d white=%d\n", raw_info.black_level, raw_info.white_level);

    #ifdef RAW_DEBUG_DUMP
    dbg_printf("saving raw buffer...\n");
    dump_seg(raw_info.buffer, MAX(raw_info.frame_size, 1000000), CARD_DRIVE"raw.buf");
    dbg_printf("saving DNG...\n");
    save_dng(CARD_DRIVE"raw.dng");
    dbg_printf("done\n");
    #endif
    
    return 1;
}

static int preview_rect_x;
static int preview_rect_y;
static int preview_rect_w;
static int preview_rect_h;

void raw_set_preview_rect(int x, int y, int w, int h)
{
    preview_rect_x = x;
    preview_rect_y = y;
    preview_rect_w = w;
    preview_rect_h = h;

    get_yuv422_vram(); // update vram parameters
    lv2raw.sx = 1024 * w / BM2LV_DX(os.x_ex);
    lv2raw.sy = 1024 * h / BM2LV_DY(os.y_ex);
    lv2raw.tx = x - LV2RAW_DX(os.x0);
    lv2raw.ty = y - LV2RAW_DY(os.y0);
}

void raw_set_geometry(int width, int height, int skip_left, int skip_right, int skip_top, int skip_bottom)
{
    raw_info.width = width;
    raw_info.height = height;
    raw_info.pitch = raw_info.width * 14 / 8;
    raw_info.frame_size = raw_info.height * raw_info.pitch;
    raw_info.active_area.x1 = skip_left;
    raw_info.active_area.y1 = skip_top;
    raw_info.active_area.x2 = raw_info.width - skip_right;
    raw_info.active_area.y2 = raw_info.height - skip_bottom;
    raw_info.jpeg.x = 0;
    raw_info.jpeg.y = 0;
    raw_info.jpeg.width = raw_info.width - skip_left - skip_right;
    raw_info.jpeg.height = raw_info.height - skip_top - skip_bottom;

    dbg_printf("active area: x=%d..%d, y=%d..%d\n", raw_info.active_area.x1, raw_info.active_area.x2, raw_info.active_area.y1, raw_info.active_area.y2);
    
    raw_set_preview_rect(skip_left, skip_top, raw_info.jpeg.width + shave_right, raw_info.jpeg.height);

    dbg_printf("lv2raw sx:%d sy:%d tx:%d ty:%d\n", lv2raw.sx, lv2raw.sy, lv2raw.tx, lv2raw.ty);
    dbg_printf("raw2lv test: (%d,%d) - (%d,%d)\n", RAW2LV_X(raw_info.active_area.x1), RAW2LV_Y(raw_info.active_area.y1), RAW2LV_X(raw_info.active_area.x2), RAW2LV_Y(raw_info.active_area.y2));
    dbg_printf("  should be: (%d,%d) - (%d,%d)\n", 0, 0, vram_lv.width, vram_lv.height);
    dbg_printf("raw2bm test: (%d,%d) - (%d,%d)\n", RAW2BM_X(raw_info.active_area.x1), RAW2BM_Y(raw_info.active_area.y1), RAW2BM_X(raw_info.active_area.x2), RAW2BM_Y(raw_info.active_area.y2));
    dbg_printf("  should be: (%d,%d) - (%d,%d)\n", os.x0, os.y0, os.x_max, os.y_max);
    dbg_printf("bm2raw test: (%d,%d) - (%d,%d)\n", BM2RAW_X(os.x0), BM2RAW_Y(os.y0), BM2RAW_X(os.x_max), BM2RAW_Y(os.y_max));
    dbg_printf("  should be: (%d,%d) - (%d,%d)\n", raw_info.active_area.x1, raw_info.active_area.y1, raw_info.active_area.x2, raw_info.active_area.y2);
}

int FAST raw_red_pixel(int x, int y)
{
    struct raw_pixblock * buf = raw_info.buffer;
    y = (y/2) * 2;
    int i = ((y * raw_info.width + x) / 8);
    return buf[i].a;
}

int FAST raw_green_pixel(int x, int y)
{
    struct raw_pixblock * buf = raw_info.buffer;
    y = (y/2) * 2;
    int i = ((y * raw_info.width + x) / 8);
    return buf[i].h;
}

int FAST raw_blue_pixel(int x, int y)
{
    struct raw_pixblock * buf = raw_info.buffer;
    y = (y/2) * 2 + 1;
    int i = ((y * raw_info.width + x) / 8);
    return buf[i].h;
}

int FAST raw_get_pixel(int x, int y) {
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: return p->a;
        case 1: return p->b_lo | (p->b_hi << 12);
        case 2: return p->c_lo | (p->c_hi << 10);
        case 3: return p->d_lo | (p->d_hi << 8);
        case 4: return p->e_lo | (p->e_hi << 6);
        case 5: return p->f_lo | (p->f_hi << 4);
        case 6: return p->g_lo | (p->g_hi << 2);
        case 7: return p->h;
    }
    return p->a;
}

int FAST raw_get_pixel_ex(void* raw_buffer, int x, int y) {
    struct raw_pixblock * p = (void*)raw_buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: return p->a;
        case 1: return p->b_lo | (p->b_hi << 12);
        case 2: return p->c_lo | (p->c_hi << 10);
        case 3: return p->d_lo | (p->d_hi << 8);
        case 4: return p->e_lo | (p->e_hi << 6);
        case 5: return p->f_lo | (p->f_hi << 4);
        case 6: return p->g_lo | (p->g_hi << 2);
        case 7: return p->h;
    }
    return p->a;
}

int FAST raw_set_pixel(int x, int y, int value)
{
    struct raw_pixblock * p = (void*)raw_info.buffer + y * raw_info.pitch + (x/8)*14;
    switch (x%8) {
        case 0: p->a = value; break;
        case 1: p->b_lo = value; p->b_hi = value >> 12; break;
        case 2: p->c_lo = value; p->c_hi = value >> 10; break;
        case 3: p->d_lo = value; p->d_hi = value >> 8; break;
        case 4: p->e_lo = value; p->e_hi = value >> 6; break;
        case 5: p->f_lo = value; p->f_hi = value >> 4; break;
        case 6: p->g_lo = value; p->g_hi = value >> 2; break;
        case 7: p->h = value; break;
    }
    return p->a;
}

int FAST raw_get_gray_pixel(int x, int y, int gray_projection)
{
    switch (gray_projection)
    {
        case GRAY_PROJECTION_RED:
            return raw_red_pixel(x, y);
        case GRAY_PROJECTION_GREEN:
            return raw_green_pixel(x, y);
        case GRAY_PROJECTION_BLUE:
            return raw_blue_pixel(x, y);
        case GRAY_PROJECTION_AVERAGE_RGB:
            return (raw_red_pixel(x, y) + raw_green_pixel(x, y) + raw_blue_pixel(x, y)) / 3;
        case GRAY_PROJECTION_MAX_RGB:
            return MAX(MAX(raw_red_pixel(x, y), raw_green_pixel(x, y)), raw_blue_pixel(x, y));
        case GRAY_PROJECTION_MAX_RB:
            return MAX(raw_red_pixel(x, y), raw_blue_pixel(x, y));
        case GRAY_PROJECTION_MEDIAN_RGB:
        {
            int r = raw_red_pixel(x, y);
            int g = raw_green_pixel(x, y);
            int b = raw_blue_pixel(x, y);
            int M = MAX(MAX(r,g),b);
            int m = MIN(MIN(r,g),b);
            if (r >= m && r <= M) return r;
            if (g >= m && g <= M) return g;
            return b;
        }
        default:
            return -1;
    }
}

/* input: 0 - 16384 (valid range: from black level to white level) */
/* output: -14 ... 0 */
float FAST raw_to_ev(int raw)
{
    int raw_max = raw_info.white_level - raw_info.black_level;
    
    if (unlikely(raw_info.white_level > 16383) && unlikely(raw > 10000))
    {
        /**
         * Hack for photo mode LV raw overlays (histogram & friends)
         * to show correct overexposure warnings when ExpSim is done with -1/3 EV digital ISO.
         * 
         * Canon implements ExpSim by varying iso/shutter/aperture in full stops, and digital ISO for 1/3 stops.
         * Digital ISO does not affect the raw histogram, so they add -1/3, 0 or +1/3 EV when developing the raw for LV display
         * We did the same adjustment by adjusting the white level in raw_update_params.
         * But when the correction is -1/3 EV, the white level is greater than 16383,
         * so the overexposure indicators will read a negative EV instead of 0 (they will no longer indicate overexposure).
         * 
         * With this hack, we are pushing raw values greater than 10000 towards 0 EV (overexposed) level,
         * thus keeping the correct horizontal position of the histogram at midtones (raw - 1/3 EV)
         * and getting correct overexposure indicators for highlights (0 EV).
         * 
         * Math:
         *      at raw=10000 we keep the original white level,
         *      at raw=15000 or more, white level becomes 15000,
         *      with linear interpolation, thus stretching the histogram in the brightest half-stop.
         * 
         * Feel free to optimize it with fixed point.
         * 
         * This hack has no effect in movie mode or outside LV, because white level is normally under 16383.
         */
        float k = COERCE((raw - 10000) / 5000.0, 0.0, 1.0);
        int adjusted_white = raw_info.white_level * (1-k) + 15000 * k;
        raw_max = adjusted_white - raw_info.black_level;
    }
    
    float raw_ev = -log2f(raw_max) + log2f(COERCE(raw - raw_info.black_level, 1, raw_max));
    return raw_ev;
}

int FAST ev_to_raw(float ev)
{
    int raw_max = MIN(raw_info.white_level, 16383) - raw_info.black_level;
    return raw_info.black_level + powf(2, ev) * raw_max;
}

static void autodetect_black_level_calc(int x1, int x2, int y1, int y2, int dx, int dy, float* out_mean, float* out_stdev)
{
    int black = 0;
    int num = 0;
    /* compute average level */
    for (int y = y1; y < y2; y += dy)
    {
        for (int x = x1; x < x2; x += dx)
        {
            black += raw_get_pixel(x, y);
            num++;
        }
    }

    float mean = black / num;

    /* compute standard deviation */
    float stdev = 0;
    for (int y = y1; y < y2; y += dy)
    {
        for (int x = x1; x < x2; x += dx)
        {
            int dif = raw_get_pixel(x, y) - mean;
            stdev += dif * dif;
            
            #ifdef RAW_DEBUG_BLACK
            /* to check if we are reading the black level from the proper spot, enable RAW_DEBUG_BLACK here and in save_dng. */
            raw_set_pixel(x, y, rand());
            #endif
        }
    }
    stdev /= num;
    stdev = sqrtf(stdev);
    
    *out_mean = mean;
    *out_stdev = stdev;
}

int autodetect_black_level()
{
    float mean = 0;
    float stdev = 0;
    
    if (raw_info.active_area.x1 > 10) /* use the left black bar for black calibration */
    {
        autodetect_black_level_calc(
            4, raw_info.active_area.x1 - 4,
            raw_info.active_area.y1 + 20, raw_info.active_area.y2 - 20, 
            3, 5,
            &mean, &stdev
        );
    }
    else /* use the top black bar for black calibration */
    {
        autodetect_black_level_calc(
            raw_info.active_area.x1 + 20, raw_info.active_area.x2 - 20, 
            4, raw_info.active_area.y1 - 4,
            5, 3,
            &mean, &stdev
        );
    }
    
    /**
     * A = full well capacity / read-out noise 
     * DR in dB = 20 log10(A)
     * DR in stops = dB / 6 = log2(A)
     * I guess noise level is the RMS value, which is identical to stdev
     * 
     * This is quite close to DxO measurements (within +/- 0.5 EV), 
     * except at very high ISOs where there seems to be noise reduction applied to raw data
     */
     
    int black_level = mean + stdev/2;
    raw_info.dynamic_range = (int)roundf((log2f(raw_info.white_level - black_level) - log2f(stdev)) * 100);

    // bmp_printf(FONT_MED, 50, 350, "black: mean=%d stdev=%d dr=%d \n", (int)mean, (int)stdev, raw_info.dynamic_range);

    /* slight correction for the magenta cast in shadows */
    return mean + stdev/8;
}

void raw_lv_redirect_edmac(void* ptr)
{
    MEM(RAW_LV_EDMAC) = (intptr_t) CACHEABLE(ptr);
}

void raw_lv_shave_right(int offset)
{
    shave_right = MAX(offset/8*8, 0);
}

void raw_lv_vsync_cbr()
{
    if (shave_right)
    {
        int edmac_pitch = shamem_read(RAW_LV_EDMAC+8) & 0xFFFF;
        int pitch_offset = shave_right/8*14;
        if (pitch_offset >= edmac_pitch) return;
        MEM(RAW_LV_EDMAC-8+0x1C) = -pitch_offset;
    }
}

int raw_lv_settings_still_valid()
{
    /* should be fast enough for vsync calls */
    int edmac_pitch = shamem_read(RAW_LV_EDMAC+8) & 0xFFFF;
    if (edmac_pitch != raw_info.pitch + shave_right*14/8) return 0;
    return 1;
}

static void FAST raw_preview_fast_work(void* raw_buffer, void* lv_buffer, int y1, int y2, int ultra_fast)
{
    uint16_t* lv16 = CACHEABLE(lv_buffer);
    uint64_t* lv64 = (uint64_t*) lv16;
    if (!lv16) return;
    
    struct raw_pixblock * raw = CACHEABLE(raw_buffer);
    if (!raw) return;
    
    uint8_t gamma[1024];
    
    for (int i = 0; i < 1024; i++)
    {
        int g = (i > (raw_info.black_level>>4)) ? log2f(i - (raw_info.black_level>>4)) * 255 / 10 : 0;
        gamma[i] = g * g / 255; /* idk, looks better this way */
    }
    
    int x1 = BM2LV_X(os.x0);
    int x2 = BM2LV_X(os.x_max);
    x1 = MAX(x1, RAW2LV_X(MAX(raw_info.active_area.x1, preview_rect_x)));
    x2 = MIN(x2, RAW2LV_X(MIN(raw_info.active_area.x2, preview_rect_x + preview_rect_w)));

    /* cache the LV to RAW transformation for the inner loop to make it faster */
    /* we will always choose a green pixel */
    
    int* lv2rx = SmallAlloc(x2 * 4);
    if (!lv2rx) return;
    for (int x = x1; x < x2; x++)
        lv2rx[x] = LV2RAW_X(x) & ~1;

    for (int y = y1; y < y2; y++)
    {
        int yr = LV2RAW_Y(y) | 1;

        if (yr <= preview_rect_y || yr >= preview_rect_y + preview_rect_h)
        {
            /* out of range, just fill with black */
            memset(&lv64[LV(0,y)/8], 0, BM2LV_DX(x2-x1)*2);
            continue;
        }

        struct raw_pixblock * row = (void*)raw + yr * raw_info.pitch;
        
        if (ultra_fast) /* prefer real-time low-res display */
        {
            if (y%2) continue;
            
            for (int x = x1; x < x2; x += 4)
            {
                int xr = lv2rx[x];
                struct raw_pixblock * p = row + (xr/8);
                int c = p->a;
                uint64_t Y = gamma[c >> 4];
                Y = (Y << 8) | (Y << 24) | (Y << 40) | (Y << 56);
                int idx = LV(x,y)/8;
                lv64[idx] = Y;
                lv64[idx + vram_lv.pitch/8] = Y;
            }
        }
        else /* prefer full-res, don't care if it's a little slower */
        {
            for (int x = x1; x < x2; x++)
            {
                int xr = lv2rx[x];
                int c = raw_get_pixel_ex(raw, xr, yr);
                uint16_t Y = gamma[c >> 4];
                lv16[LV(x,y)/2] = Y << 8;
            }
        }
    }
    SmallFree(lv2rx);
}

void FAST raw_preview_fast_ex(void* raw_buffer, void* lv_buffer, int y1, int y2, int ultra_fast)
{
    if (raw_buffer == (void*)-1)
        raw_buffer = raw_info.buffer;
    
    if (lv_buffer == (void*)-1)
        lv_buffer = (void*)YUV422_LV_BUFFER_DISPLAY_ADDR;
    
    if (y1 == -1)
        y1 = BM2LV_Y(os.y0);
    
    if (y2 == -1)
        y2 = BM2LV_Y(os.y_max);
    
    if (ultra_fast == -1)
        ultra_fast = 0;
    
    raw_preview_fast_work(raw_buffer, lv_buffer, y1, y2, ultra_fast);
}

void FAST raw_preview_fast()
{
    raw_preview_fast_ex((void*)-1, (void*)-1, -1, -1, -1);
}

static int lv_raw_enabled;
#ifdef PREFERRED_RAW_TYPE
static int old_raw_type = -1;
#endif
static void raw_lv_enable()
{
    lv_raw_enabled = 1;
    shave_right = 0;
    call("lv_save_raw", 1);
    
#ifdef PREFERRED_RAW_TYPE
    old_raw_type = MEM(RAW_TYPE_ADDRESS);
    MEM(RAW_TYPE_ADDRESS) = PREFERRED_RAW_TYPE;
#elif defined(USE_LV_AF_RAW)
    call("lv_af_raw", 1);
#endif
}

static void raw_lv_disable()
{
    lv_raw_enabled = 0;
    call("lv_save_raw", 0);
    
#ifdef PREFERRED_RAW_TYPE
    if (old_raw_type != -1)
    {
        MEM(RAW_TYPE_ADDRESS) = old_raw_type;
        old_raw_type = -1;
    }
#elif defined(USE_LV_AF_RAW)
    call("lv_af_raw", 0);
#endif
}

int raw_lv_is_enabled()
{
    return lv_raw_enabled;
}

static int raw_lv_request_count = 0;

static void raw_lv_update()
{
    int new_state = raw_lv_request_count > 0;
    if (new_state && !lv_raw_enabled)
    {
        raw_lv_enable();
        msleep(50);
    }
    else if (!new_state && lv_raw_enabled)
    {
        raw_lv_disable();
        msleep(50);
    }
}

void raw_lv_request()
{
    raw_lv_request_count++;
    raw_lv_update();
}
void raw_lv_release()
{
    raw_lv_request_count--;
    ASSERT(raw_lv_request_count >= 0);
    raw_lv_update();
}

/* may not be correct on 4:3 screens */
void raw_force_aspect_ratio_1to1()
{
    if (lv2raw.sy < lv2raw.sx) /* image too tall */
    {
        lv2raw.sy = lv2raw.sx;
        int height = RAW2LV_DY(preview_rect_h);
        int offset = (vram_lv.height - height) / 2;
        int skip_top = preview_rect_y;
        lv2raw.ty = skip_top - LV2RAW_DY(os.y0) - LV2RAW_DY(offset);
    }
    else if (lv2raw.sx < lv2raw.sy) /* image too wide */
    {
        lv2raw.sx = lv2raw.sy;
        int width = RAW2LV_DX(preview_rect_w);
        int offset = (vram_lv.width - width) / 2;
        int skip_left = preview_rect_x;
        lv2raw.tx = skip_left - LV2RAW_DX(os.x0) - LV2RAW_DX(offset);
    }
}
