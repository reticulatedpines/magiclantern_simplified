// additional info on photo screen


#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#ifdef STROBO_READY_AND_WE_CAN_USE_IT
#include <strobo.h>
#endif
#include <version.h>

#if  !defined(AUDIO_REM_SHOT_POS_X) && !defined(AUDIO_REM_SHOT_POS_Y)
    #define AUDIO_REM_SHOT_POS_X 20
    #define AUDIO_REM_SHOT_POS_Y 40
#endif

void display_shooting_info() // called from debug task
{
    if (lv) return;

    #ifdef FEATURE_FLEXINFO
    // from flexinfo.c
    info_print_screen();
    #endif

    // the following is stuff not yet ported to flexinfo
    #ifdef FEATURE_LCD_SENSOR_REMOTE
    display_lcd_remote_icon(555, 460);
    #endif
    
    // hack for Rebel cameras to display intermediate ISOs
    iso_refresh_display();
    
    display_trap_focus_info();

#ifdef STROBO_READY_AND_WE_CAN_USE_IT
    int col_field = bmp_getpixel(20,10);
    if (flash_info.mode==STROBO_FLASH_MODE_MANUAL)
    {
        uint32_t fntl = FONT(FONT_LARGE, COLOR_YELLOW, col_field);
        fnt = FONT(FONT_SMALL, COLOR_CYAN, col_field);
        bmp_printf(fnt, 488, 188, "A");
        bmp_printf(fntl, 498, 185, "%3d", 1 << flash_info.group_a_output);
        bmp_printf(fnt, 556, 188, "B");
        bmp_printf(fntl, 564, 185, "%3d", 1 << flash_info.group_b_output );
        bmp_printf(fnt, 624, 188, "C");
        bmp_printf(fntl, 632, 185, "%3d", 1 << flash_info.group_c_output);
        bmp_fill(col_bg,486,212,212,6);
    }
#endif

}

