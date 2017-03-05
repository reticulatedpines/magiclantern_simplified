#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"
#include "property.h"
#include "raw.h"
#include "lens.h"
#include "timer.h"
#include "qemu-util.h"

int qprint(const char * msg)
{
    for (const char* c = msg; *c; c++)
    {
        *(volatile uint32_t*)REG_PRINT_CHAR = *c;
    }
    return 0;
}

int qprintf(const char * fmt, ...) // prints in the QEMU console
{
    va_list ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    qprint(buf);
    return 0;
}

void qemu_hello()
{
    bmp_printf(FONT_LARGE, 50, 50, "Hello from QEMU!");

    for (int i = 1; i < 14; i++)
    {
        bfnt_draw_char(-i, i * 50, 100, COLOR_BLUE, COLOR_WHITE);
        bmp_printf(FONT(FONT_MED, COLOR_BLUE, COLOR_WHITE), i * 50, 140, "%d", i);
    }

    qprintf("\nHello at QEMU console!\n\n");
    
    call("dispcheck");
    call("shutdown");
    
    while(1); // that's all, folks!
}

static void toggle_display_type()
{
    // set BMP VRAM
    static uintptr_t bmp_raw = 0;
    
    if (!bmp_raw)
    {
        /* first-time initialization */
        bmp_raw = (uintptr_t) fio_malloc(960*540*2 + 16384);
    }
    else
    {
        /* switch to next mode */
        MEM(REG_DISP_TYPE) = MOD(MEM(REG_DISP_TYPE) + 1, 5);
    }
    
    uintptr_t bmp_aligned = (bmp_raw + 0xFFF) & ~0xFFF;
    uintptr_t bmp_hdmi = bmp_aligned + 0x008;
    uintptr_t bmp_lcd = bmp_hdmi + BMP_HDMI_OFFSET;
    
    /* one of those is for PAL, the other is for NTSC; see BMP_VRAM_START in bmp.c */
    uintptr_t bmp_sd1 = bmp_hdmi + BMP_HDMI_OFFSET + 8;
    uintptr_t bmp_sd2 = bmp_hdmi + BMP_HDMI_OFFSET + 0x3c8;
    //uintptr_t bmp_sd3 = bmp_hdmi + BMP_HDMI_OFFSET + 0x3c0; /* 700D and maybe other newer cameras? */
    
    int display_type = MEM(REG_DISP_TYPE);
    char* display_modes[] = {   "LCD",  "HDMI-1080",    "HDMI-480", "SD-PAL",   "SD-NTSC"   };
    uintptr_t buffers[]   = {   bmp_lcd, bmp_hdmi,       bmp_lcd,    bmp_sd1,    bmp_sd2    };
    int hdmi_codes[]      = {   0,       5,              2,          0,          0          };
    int ext_hdmi_codes[]  = {   0,       1,              1,          0,          0          };
    int ext_rca_codes[]   = {   0,       0,              0,          1,          1          };
    //int pal_codes[]     = {   0,       0,              0,          1,          0          };
    
    MEM(REG_BMP_VRAM) = buffers[display_type];
    bmp_vram_info[1].vram2 = (void*)buffers[display_type];
    hdmi_code = hdmi_codes[display_type];
    ext_monitor_hdmi = ext_hdmi_codes[display_type];
    _ext_monitor_rca = ext_rca_codes[display_type];
    //~ pal = pal_codes[display_type];
    
    qprintf(
        "BMP buffer (%s): raw=%x hdmi=%x lcd=%x real=%x idle=%x\n", 
        display_modes[display_type], bmp_raw, bmp_hdmi, bmp_lcd, bmp_vram_real(), bmp_vram_idle()
    );
    
    /* set image VRAM (LV buffer) */
    //~ uintptr_t lv_buffer = (uintptr_t) fio_malloc(1920*1080*2 + 0x1000);
    //~ uintptr_t lv_buffer = (uintptr_t) fio_malloc(720*480*2 + 0x1000);
    //~ uintptr_t lv_buffer = YUV422_LV_BUFFER_1;
    //~ uintptr_t lv_aligned = (lv_buffer + 0x7FF) & ~0x7FF;
    //~ YUV422_LV_BUFFER_DISPLAY_ADDR = lv_aligned + 0x800;
    //~ qprintf(
        //~ "IMG buffer: %x\n", lv_aligned
    //~ );
    
    vram_params_set_dirty();
}

static void toggle_liveview()
{
    if (!lv)
    {
        printf("LiveView on\n"); 

        /* pretend we go into LiveView */
        lv = 1;
        _expsim = 1;
        lv_dispsize = 1;

        /* this will load an image in the YUV buffers, from eos.c */
        /* fixme: hardcoded buffers, may overwrite something */
        MEM(REG_IMG_VRAM) = YUV422_LV_BUFFER_1;
        MEM(REG_IMG_VRAM) = YUV422_LV_BUFFER_2;
        MEM(REG_IMG_VRAM) = YUV422_LV_BUFFER_3;
        YUV422_LV_BUFFER_DISPLAY_ADDR = YUV422_LV_BUFFER_3;
        
#if 0
        /* load LV RAW buffer */
        /* fixme: hardcoded, may overwrite something */
        void* raw_buffer = 0x3F000000;
        MEM(REG_RAW_BUFF) = raw_buffer;
        MEM(0x2600C + 0x2c) = raw_buffer;   /* fixme: 5D3 113 only */
        
        /* simulate ISO, which gives white level and dynamic range in photo mode */
        EngDrvOut(0xc0f08030, 0x10e7);
        lens_info.raw_iso = ISO_3200;
        lens_info.iso = 3200;
        
        /* preview the raw buffer, to make sure it was loaded correctly */
        raw_lv_request();
        raw_update_params();
        raw_preview_fast();
        raw_lv_release();
#endif

        /* fill the gui_task_list structure with some dummy values, 
         * so ML code believes it's running on top of Canon's LV dialog 
         * and show the overlays
         */
        struct gui_task * current = gui_task_list.current;
        if (!current)
        {
            printf("Fake LV dialog handler\n"); 
            extern thunk LiveViewApp_handler;
            gui_task_list.current = current = malloc(sizeof(struct gui_task));
            current->priv = malloc(sizeof(struct dialog));
            struct dialog * dialog = current->priv;
            dialog->handler = (void*)&LiveViewApp_handler;
        }

    }
    else
    {
        printf("LiveView off\n"); 
        lv = 0;
        MEM(REG_IMG_VRAM) = 0;
    }
    
    clrscr();
    redraw();
    vram_params_set_dirty();
}

void hptimer_cbr(int a, void* b)
{
    qprintf("Hello from HPTimer (%d, %d)\n", a, b);
}

void qemu_hptimer_test()
{
    msleep(5000);

    /* one HPTimer is easy to emulate, but getting them to work in multitasking is hard */
    /* note: configuring 20 timers might cause a few of them to say NOT_ENOUGH_MEMORY */
    /* this test will stress both the HPTimers and the interrupt engine */
    SetHPTimerAfterNow(9000, hptimer_cbr, hptimer_cbr, (void*) 9);
    SetHPTimerAfterNow(6000, hptimer_cbr, hptimer_cbr, (void*) 6);
    SetHPTimerAfterNow(8000, hptimer_cbr, hptimer_cbr, (void*) 8);
    SetHPTimerAfterNow(4000, hptimer_cbr, hptimer_cbr, (void*) 4);
    SetHPTimerAfterNow(7000, hptimer_cbr, hptimer_cbr, (void*) 7);
    SetHPTimerAfterNow(9000, hptimer_cbr, hptimer_cbr, (void*) 10);
    SetHPTimerAfterNow(5000, hptimer_cbr, hptimer_cbr, (void*) 5);
    SetHPTimerAfterNow(2000, hptimer_cbr, hptimer_cbr, (void*) 2);
    SetHPTimerAfterNow(3000, hptimer_cbr, hptimer_cbr, (void*) 3);
    SetHPTimerAfterNow(1000, hptimer_cbr, hptimer_cbr, (void*) 1);

    SetHPTimerAfterNow(19000, hptimer_cbr, hptimer_cbr, (void*) 19);
    SetHPTimerAfterNow(16000, hptimer_cbr, hptimer_cbr, (void*) 16);
    SetHPTimerAfterNow(18000, hptimer_cbr, hptimer_cbr, (void*) 18);
    SetHPTimerAfterNow(14000, hptimer_cbr, hptimer_cbr, (void*) 14);
    SetHPTimerAfterNow(17000, hptimer_cbr, hptimer_cbr, (void*) 17);
    SetHPTimerAfterNow(19000, hptimer_cbr, hptimer_cbr, (void*) 20);
    SetHPTimerAfterNow(15000, hptimer_cbr, hptimer_cbr, (void*) 15);
    SetHPTimerAfterNow(12000, hptimer_cbr, hptimer_cbr, (void*) 12);
    SetHPTimerAfterNow(13000, hptimer_cbr, hptimer_cbr, (void*) 13);
    SetHPTimerAfterNow(11000, hptimer_cbr, hptimer_cbr, (void*) 11);
    msleep(5000);
}
