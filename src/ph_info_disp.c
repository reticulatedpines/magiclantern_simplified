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
    
    // from flexinfo.c
    info_print_screen();
    
    // the following is stuff not yet ported to flexinfo

    display_lcd_remote_icon(555, 460);
    
    // hack for Rebel cameras to display intermediate ISOs
    iso_refresh_display();
    
    display_trap_focus_info();

#if 0
    extern int header_left_info;
    extern int header_right_info;
    extern int footer_left_info;
    extern int footer_right_info;
    char adate[16];
    char info[72];
        
    if (header_left_info==3 || header_right_info==3 || footer_left_info==3 || footer_right_info==3)
    {
        struct tm now;
        LoadCalendarFromRTC( &now );
        // need to find the date format settings and use it here
        //      snprintf(adate, sizeof(adate), "%2d.%2d.%4d", (now.tm_mon+1),now.tm_mday,(now.tm_year+1900));
        snprintf(adate, sizeof(adate), "%2d.%2d.%4d", now.tm_mday,(now.tm_mon+1),(now.tm_year+1900));
        //		snprintf(adate, sizeof(adate), "%4d.%2d.%2d", (now.tm_year+1900),(now.tm_mon+1),now.tm_mday);
    }
    
    fnt = FONT(FONT_MED, COLOR_FG_NONLV, col_bg);
    if (header_left_info>0)
        bmp_printf(fnt, 28, 2, (
                                header_left_info==1 ? artist_name:
                                header_left_info==2 ? copyright_info:
                                header_left_info==3 ? adate:
                                header_left_info==4 ? lens_info.name:
                                header_left_info==5 ? build_version:
                                "")
                   );
    if (header_right_info>0)
    {
        snprintf(info, sizeof(info), "%s", (
                                            header_right_info==1 ? artist_name:
                                            header_right_info==2 ? copyright_info:
                                            header_right_info==3 ? adate:
                                            header_right_info==4 ? lens_info.name:
                                            header_right_info==5 ? build_version:
                                            ""));
        bmp_printf(fnt, 693-strlen(info) * font_med.width, 2, info);
    }
    
    col_bg = bmp_getpixel(27,459);
    fnt = FONT(FONT_MED, COLOR_FG_NONLV, col_bg);
    if (footer_left_info>0)
        bmp_printf(fnt, 28, 459, (
                                  footer_left_info==1 ? artist_name:
                                  footer_left_info==2 ? copyright_info:
                                  footer_left_info==3 ? adate:
                                  footer_left_info==4 ? lens_info.name:
                                  footer_left_info==5 ? build_version:
                                  "")
                   );
    
    if (footer_right_info>0)
    {
        snprintf(info, sizeof(info), "%s", (
                                            footer_right_info==1 ? artist_name:
                                            footer_right_info==2 ? copyright_info:
                                            footer_right_info==3 ? adate:
                                            footer_right_info==4 ? lens_info.name:
                                            footer_right_info==5 ? build_version:
                                            ""));
        bmp_printf(fnt, 693-strlen(info) * font_med.width, 459, info);
    }
#endif
    
#ifdef STROBO_READY_AND_WE_CAN_USE_IT
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
        bmp_fill(bmp_getpixel(1,1),486,212,212,6);
    }
#endif

}

