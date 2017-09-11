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

#if 0
/* first Hello World in QEMU */
/* http://www.magiclantern.fm/forum/index.php?topic=2864.msg26022#msg26022 */
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
#endif
