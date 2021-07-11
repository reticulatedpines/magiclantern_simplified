/** \file
 * FA_CaptureTestImage - minimal code for QEMU
 */

#include "dryos.h"
#include "vram.h"
#include "bmp.h"
#include "lens.h"
#include "font_direct.h"
#include "raw.h"

struct task *first_task = NULL; // needed to build due to usage in function_overrides.c
                                // for D678 cams, not used

extern void _prop_request_change(unsigned property, const void* addr, size_t len);

static void run_test()
{
    /* clear the screen - hopefully nobody will overwrite us */
    clrscr();

    /* make sure we've got some sane exposure settings */
    int iso = ISO_100;
    int shutter = SHUTTER_1_50;
    _prop_request_change(PROP_ISO, &iso, 4);
    _prop_request_change(PROP_SHUTTER, &shutter, 4);

    /* capture a full-res silent picture */
    /* (on real camera, you won't see anything, unless you start in LV PLAY mode */
    void* job = (void*) call("FA_CreateTestImage");
    call("FA_CaptureTestImage", job);
    call("FA_DeleteTestImage", job);

    /* fake a few things, to make the raw backend happy */
    gui_state = GUISTATE_QR;
    pic_quality = PICQ_RAW;
    lens_info.raw_iso = ISO_100;

    if (!raw_update_params())
    {
        font_draw(50,  75, COLOR_RED, 3, "RAW ERROR.");
    }
    else
    {
        raw_preview_fast();
    }
}

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    /* nothing to do */
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    /* wait for display to initialize */
    while (!bmp_vram_info[1].vram2)
    {
        msleep(100);
    }

    msleep(1000);

#ifdef CONFIG_QEMU
    /* for running in QEMU: go to PLAY mode and take the test picture from there
     * ideally, we should run from LiveView, but we don't have it emulated.
     * 
     * The emulation usually starts with the main Canon screen,
     * however, some models have it off by default (e.g. 550D),
     * others have sensor cleaning animations (5D2, 50D, 600D),
     * and generally it's hard to draw over this screen without trickery. */
    SetGUIRequestMode(GUIMODE_PLAY);
    msleep(1000);

    /* some cameras don't initialize the YUV buffer right away - but we need it! */
    if (!YUV422_LV_BUFFER_DISPLAY_ADDR)
    {
        /* let's hope this works... */
        extern void * _AllocateMemory(size_t);
        int size = 720 * 480 * 2;
        void * buf = _AllocateMemory(720 * 480 * 2);
        while (!buf);   /* lock up on error */
        memset(buf, 0, size);
        MEM(0xC0F140E0) = YUV422_LV_BUFFER_DISPLAY_ADDR = (uint32_t) buf;
        qprintf("Allocated YUV buffer: %X\n", YUV422_LV_BUFFER_DISPLAY_ADDR);
    }
#else
    /* for running on real camera: wait for user to enter LiveView,
     * then switch to PLAY mode (otherwise you'll capture a dark frame) */
    for (int i = 0; i < 5; i++)
    {
        uint8_t* bmp = bmp_vram_info[1].vram2;
        memset(bmp + 950*40, COLOR_BLACK, 960*70);

        font_draw(50, 50, COLOR_WHITE, 3, "Please enter LiveView,");
        font_draw(50, 75, COLOR_WHITE, 3, "then switch to PLAY mode.");

        msleep(1000);
    }
#endif

    task_create("run_test", 0x1e, 0x4000, run_test, 0 );
}

/* used by font_draw */
void disp_set_pixel(int x, int y, int c)
{
    uint8_t* bmp = bmp_vram_info[1].vram2;
    bmp[x + y * 960] = c;
}

void clrscr()
{
    uint8_t* bmp = bmp_vram_info[1].vram2;
    memset(bmp, 0, 960*480);
}


/* dummy stubs to include raw.c */

int get_ms_clock()
{
    static int ms = 0;
    ms += 10;
    return ms;
}

int is_pure_play_photo_mode() { return 0; }
int is_pure_play_movie_mode() { return 0; }
int is_play_mode() { return 0; }
int digic_zoom_overlay_enabled() { return 0; }
int display_filter_enabled() { return 0; }
void display_filter_get_buffers(uint32_t** a, uint32_t** b) { }
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ... ) { return 0; }
int printf(const char * fmt, ...) { return 0; }
void * bmp_lock;
void bmp_mute_flag_reset() {}
void redraw() {}
void afframe_set_dirty() {}
void ml_assert_handler(char* msg, char* file, int line, const char * func) {}
struct font font_med;
struct lens_info lens_info;
int focus_box_get_raw_crop_offset(int* dx, int* dy) { return 0; }
void edmac_raw_slurp() {}
int should_run_polling_action(int ms, int* last) { return 1; }
int fps_get_current_x1000() { return 30000; }
int wait_lv_frames(int n) { return 0; } 
void EngDrvOut(uint32_t reg, uint32_t val) { MEM(reg) = val; }
void EngDrvOutLV(uint32_t reg, uint32_t val) { };
int get_expsim() { return 0; }
int module_exec_cbr(unsigned int type) { return 0; }
int display_idle() { return 1; }

extern void* _AllocateMemory(size_t size);
extern void  _FreeMemory(void* ptr);

void * __mem_malloc( size_t len, unsigned int flags, const char *file, unsigned int line)
{
    return _AllocateMemory(len);
}
void __mem_free( void * buf)
{
    return _FreeMemory(buf);
}

int raw2iso(int raw_iso)
{
    int iso = (int) roundf(100.0f * powf(2.0f, (raw_iso - 72.0f)/8.0f));
    return iso;
}

const char * format_memory_size(uint64_t size)
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "%d", size);
    return buf;
}
