// misc functions specific to 6D

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

int new_LiveViewApp_handler = 0xff123456;

// dummy stubs
/* called from misc_shooting_info() in shoot.c */

void display_clock()
{
#ifdef CONFIG_PHOTO_MODE_INFO_DISPLAY
   int bg = bmp_getpixel(15, 430);
    struct tm now;
    LoadCalendarFromRTC( &now );
    if (!lv)
    {
        bg = bmp_getpixel( DISPLAY_CLOCK_POS_X, DISPLAY_CLOCK_POS_Y );
        uint32_t fnt = FONT(SHADOW_FONT(FONT_LARGE), COLOR_FG_NONLV, bg);
        bmp_printf(fnt, DISPLAY_CLOCK_POS_X, DISPLAY_CLOCK_POS_Y, "%02d:%02d", now.tm_hour, now.tm_min);
    }
    static int prev_min = 0;
    if (prev_min != now.tm_min) redraw();
    prev_min = now.tm_min;
#endif
}


