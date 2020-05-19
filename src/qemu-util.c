#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"
#include "property.h"
#include "raw.h"
#include "lens.h"

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN 0xCF123004
#define REG_DUMP_VRAM 0xCF123008
#define REG_GET_KEY    0xCF123010
#define REG_BMP_VRAM   0xCF123014
#define REG_IMG_VRAM   0xCF123018
#define REG_RAW_BUFF   0xCF12301C
#define REG_DISP_TYPE  0xCF123020

int qprintf(const char * fmt, ...) // prints in the QEMU console
{
    va_list ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    
    for (char* c = buf; *c; c++)
        *(volatile uint32_t*)REG_PRINT_CHAR = *c;
    
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

/* http://www.marjorie.de/ps2/scancode-set1.htm */
static int translate_scancode(int scancode)
{
    switch (scancode)
    {
        #ifdef BGMT_Q
        case 0x10: return BGMT_Q;                       /* Q */
        #endif
        case 0x1C: return BGMT_PRESS_FULLSHUTTER;       /* ENTER */
        case 0x9C: return BGMT_UNPRESS_FULLSHUTTER;
        case 0x36: return BGMT_PRESS_HALFSHUTTER;       /* right shift */
        case 0xB6: return BGMT_UNPRESS_HALFSHUTTER;
        case 0x32: return BGMT_MENU;                    /* M */
        case 0x39: return BGMT_PRESS_SET;               /* space */
        case 0xB9: return BGMT_UNPRESS_SET;
        #ifdef BGMT_JOY_CENTER
        case 0x2D: return BGMT_JOY_CENTER;              /* X */
        case 0xAD: return BGMT_UNPRESS_UDLR;
        #endif
        case 0x1A: return BGMT_WHEEL_LEFT;              /* [ and ] */
        case 0x1B: return BGMT_WHEEL_RIGHT;
        case 0x19: return BGMT_PLAY;                    /* P */
        case 0x17: return BGMT_INFO;                    /* I */
        #ifdef BGMT_RATE
        case 0x13: return BGMT_RATE;                    /* R */
        #endif
        case 0x0D: return BGMT_PRESS_ZOOM_IN;      /* + */
        //~ case 0x8D: return BGMT_UNPRESS_ZOOM_IN;
        //~ case 0x0C: return BGMT_PRESS_ZOOM_OUT;      /* - */
        //~ case 0x8C: return BGMT_UNPRESS_ZOOM_OUT;
        
        case 0xE0:
        {
            int second_code = 0;
            while (!second_code)
            {
                second_code = MEM(REG_GET_KEY);
            }
            
            switch (second_code)
            {
                case 0x48: return BGMT_PRESS_UP;        /* arrows */
                case 0x4B: return BGMT_PRESS_LEFT;
                case 0x50: return BGMT_PRESS_DOWN;
                case 0x4D: return BGMT_PRESS_RIGHT;
                case 0xC8:
                case 0xCB:
                case 0xD0:
                #ifdef BGMT_UNPRESS_UDLR
                case 0xCD: return BGMT_UNPRESS_UDLR;
                #else
                case 0xCD: return BGMT_UNPRESS_LEFT;    /* fixme: not correct, but enough for ML menu */
                #endif
                case 0x49: return BGMT_WHEEL_UP;        /* page up */
                case 0x51: return BGMT_WHEEL_DOWN;
                case 0x53: return BGMT_TRASH;           /* delete */
            }
        }
    }
    
    return -1;
}

static void qemu_print_help()
{
    bmp_printf(FONT_LARGE, 50, 30, "Magic Lantern in QEMU");

    big_bmp_printf(FONT_MONO_20 | FONT_ALIGN_FILL, 50, 80,
        "DELETE       open ML menu\n"
        "SPACE        SET\n"
        "SHIFT        half-shutter\n"
        "ENTER        full-shutter\n"
        "Q            you know :)\n"
        "M            MENU\n"
        "P            PLAY\n"
        "I            INFO\n"
        "R            RATE\n"
        "X            joystick center\n"
        "Arrows       guess :)\n"
        "PageUp/Dn    rear scrollwheel\n"
        "[ and ]      top scrollwheel\n"
        "+/-          zoom in/out\n"
        "H            LCD/HDMI/SD monitor\n"
        "L            LiveView\n"
    );

    bmp_printf(FONT_LARGE, 50, 420, "Have fun!");
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
    uintptr_t bmp_sd3 = bmp_hdmi + BMP_HDMI_OFFSET + 0x3c0; /* 700D and maybe other newer cameras? */
    
    int display_type = MEM(REG_DISP_TYPE);
    char* display_modes[] = {   "LCD",  "HDMI-1080",    "HDMI-480", "SD-PAL",   "SD-NTSC"   };
    uintptr_t buffers[]   = {   bmp_lcd, bmp_hdmi,       bmp_lcd,    bmp_sd1,    bmp_sd2    };
    int hdmi_codes[]      = {   0,       5,              2,          0,          0          };
    int ext_hdmi_codes[]  = {   0,       1,              1,          0,          0          };
    int ext_rca_codes[]   = {   0,       0,              0,          1,          1          };
    int pal_codes[]       = {   0,       0,              0,          1,          0          };
    
    bmp_vram_info[1].vram2 = MEM(REG_BMP_VRAM) = buffers[display_type];
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
            dialog->handler = &LiveViewApp_handler;
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

static void qemu_key_poll()
{
    TASK_LOOP
    {
        int keycode = MEM(REG_GET_KEY);
        if (keycode == 0x23) // H
        {
            toggle_display_type();
        }
        else if (keycode == 0x26) // L
        {
            toggle_liveview();
        }
        else if (keycode)
        {
            int event_code = translate_scancode(keycode);
            if (event_code >= 0)
            {
                GUI_Control(event_code, 0, 0, 0);
            }
            else
            {
                qprintf("Key %x\n", keycode);
            }
        }
        else
        {
            msleep(50);
        }
        
        if (!gui_menu_shown() && !lv)
        {
            qemu_print_help();
        }
    }
}

TASK_CREATE( "qemu_key_poll", qemu_key_poll, 0, 0x1a, 0x2000 );

void qemu_cam_init()
{
    toggle_display_type();

    // fake display on
    #if defined(CONFIG_550D)
    extern int display_is_on_550D;
    display_is_on_550D = 1;
    #elif defined(DISPLAY_STATEOBJ)
    DISPLAY_STATEOBJ->current_state = 1;
    #else
    DISPLAY_IS_ON = 1;
    #endif
    
    pic_quality = PICQ_RAW;
}
