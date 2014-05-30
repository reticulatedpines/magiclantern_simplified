#include "dryos.h"
#include "bmp.h"
#include "cache_hacks.h"

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN 0xCF123004
#define REG_DUMP_VRAM 0xCF123008
#define REG_GET_KEY    0xCF123010

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
        case 0x10: return BGMT_Q;                       /* Q */
        case 0x1C: return BGMT_PRESS_FULLSHUTTER;       /* ENTER */
        case 0x9C: return BGMT_UNPRESS_FULLSHUTTER;
        case 0x36: return BGMT_PRESS_HALFSHUTTER;       /* right shift */
        case 0xB6: return BGMT_UNPRESS_HALFSHUTTER;
        case 0x32: return BGMT_MENU;                    /* M */
        case 0x39: return BGMT_PRESS_SET;               /* space */
        case 0xB9: return BGMT_UNPRESS_SET;
        case 0x2D: return BGMT_JOY_CENTER;              /* X */
        case 0xAD: return BGMT_UNPRESS_UDLR;
        case 0x1A: return BGMT_WHEEL_LEFT;              /* or [ ] according to the spec */
        case 0x1B: return BGMT_WHEEL_RIGHT;
        case 0x19: return BGMT_PLAY;                    /* P */
        case 0x17: return BGMT_INFO;                    /* I */
        case 0x13: return BGMT_RATE;                    /* R */
        case 0x0D: return BGMT_PRESS_ZOOMIN_MAYBE;      /* + */
        //~ case 0x8D: return BGMT_UNPRESS_ZOOMIN_MAYBE;
        //~ case 0x0C: return BGMT_PRESS_ZOOMOUT_MAYBE;      /* - */
        //~ case 0x8C: return BGMT_UNPRESS_ZOOMOUT_MAYBE;
        
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
                case 0xCD: return BGMT_UNPRESS_UDLR;
                case 0x49: return BGMT_WHEEL_UP;        /* page up */
                case 0x51: return BGMT_WHEEL_DOWN;
                case 0x53: return BGMT_TRASH;           /* delete */
            }
        }
    }
    
    return -1;
}

static void qemu_key_poll()
{
    TASK_LOOP
    {
        int keycode = MEM(REG_GET_KEY);
        if (keycode)
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
    }
}
TASK_CREATE( "qemu_key_poll", qemu_key_poll, 0, 0x1a, 0x2000 );
