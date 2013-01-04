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

/* NOTE: this 2 #define be put in consts.h of each camera supporting that and maybe adjusted */
#if !defined(CONFIG_600D) || !defined(CONFIG_7D)
    // Audio remote shot position info photo mode
    #define AUDIO_REM_SHOT_POS_X 20
    #define AUDIO_REM_SHOT_POS_Y 40
#endif
/* NOTE: this should be put in consts.h of each camera supporting that and maybe adjusted */


static void double_buffering_start(int ytop, int height)
{
    // use double buffering to avoid flicker
    bmp_vram(); // make sure parameters are up to date
    ytop = MIN(ytop, BMP_H_PLUS - height);
    memcpy(bmp_vram_idle() + BM(0,ytop), bmp_vram_real() + BM(0,ytop), height * BMPPITCH);
    bmp_draw_to_idle(1);
}

static void double_buffering_end(int ytop, int height)
{
    // done drawing, copy image to main BMP buffer
    bmp_draw_to_idle(0);
    bmp_vram(); // make sure parameters are up to date
    ytop = MIN(ytop, BMP_H_PLUS - height);
    memcpy(bmp_vram_real() + BM(0,ytop), bmp_vram_idle() + BM(0,ytop), height * BMPPITCH);
    bzero32(bmp_vram_idle() + BM(0,ytop), height * BMPPITCH);
}


void display_shooting_info() // called from debug task
{
	if (lv) return;
    
    uint32_t fnt;
	int bg;
    
	if (lens_info.wb_mode == WB_KELVIN)
	{
        //------------ ICON KELVIN BLACK -------------
        int icon_x = 196; // x position height icon
        int icon_y = 226; // y position width icon
        
        BMP_LOCK (
                  double_buffering_start(icon_y, 48);
                  
                  for (int i = 0; i < 9; i++) // | vertical K line
                  {
                      bg = COLOR_FG_NONLV;
                      
                      bmp_fill(bg,26+icon_x,4+icon_y,9,38); // | vertical K line
                      for (int i = 0; i < 9; i++) // / first diagonal in K
                      {
                          draw_line(39 + icon_x + i, 4 + icon_y, 30 + icon_x + i, 22 + icon_y, bg);
                      }
                      for (int i = 0; i < 9; i++) // \ second diagonal in K
                      {
                          draw_line(30 + icon_x + i, 23 + icon_y, 39 + icon_x + i, 41 + icon_y, bg);
                      }
                      
                      for (int r = 5; r < 9; r++) // o
                      {
                          draw_circle(11 + icon_x, 11 + icon_y, r, bg); // small circle
                          draw_circle(10 + icon_x, 11 + icon_y, r, bg); // small circle
                      }
                      
                  }
                  double_buffering_end(icon_y, 48);
                  )
        bg = bmp_getpixel(icon_x, icon_y + 46);
        fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
        bmp_printf(fnt, icon_x, icon_y + 46, "%5d", lens_info.kelvin);
        //------------ ICON KELVIN ------------------
        
	}
    
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		bg = bmp_getpixel(WBS_BA_POS_X, WBS_BA_POS_Y);
		fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
        
		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, WBS_BA_POS_X, WBS_BA_POS_Y, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else    bmp_printf(fnt, WBS_BA_POS_X, WBS_BA_POS_Y, "  ");
        
		bg = bmp_getpixel(WBS_GM_POS_X, WBS_GM_POS_Y);
		fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
        
		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, WBS_GM_POS_X, WBS_GM_POS_Y, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else    bmp_printf(fnt, WBS_GM_POS_X, WBS_GM_POS_Y, "  ");
	}
    
	int col_bg;
	int col_field;
    
    col_bg = bmp_getpixel(10,1);
    col_field = bmp_getpixel(615,375);
    
#ifdef DISPLAY_HEADER_FOOTER_INFO
    extern int header_left_info;
    extern int header_right_info;
    extern int footer_left_info;
    extern int footer_right_info;
    char adate[11];
    char info[63];
    
    //bmp_fill(col_bg,28,3,694,20);
    //bmp_fill(col_bg,28,459,694,20);
    
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
    if (footer_left_info>0)
        bmp_printf(fnt, 28, 459, (
                                  footer_left_info==1 ? artist_name:
                                  footer_left_info==2 ? copyright_info:
                                  footer_left_info==3 ? adate:
                                  footer_left_info==4 ? lens_info.name:
                                  footer_left_info==5 ? build_version:
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
    //~ bmp_printf(fnt, 400, 450, "Flash:%s",
    //~ strobo_firing == 0 ? " ON" :
    //~ strobo_firing == 1 ? "OFF" : "Auto"
    //~ strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
    //~ );
#endif
    
    if (avail_shot>9999) // we can write 5 digits if necessary
    {
    bmp_fill(col_field,540,384,152,72); // clear the "[9999]"
        char msg[7];
        snprintf(msg, sizeof(msg), "[%5d%]", avail_shot);
        bfnt_puts(msg, 550 , 402, COLOR_FG_NONLV, col_field);
    }
#ifdef AVAIL_SHOT_WORKAROUND // for camera tha shows only 3 digit in remaining pic
	elif (avail_shot>999)// it is a Canon bug (only 3 digits), it can display on the other info screen four digit number
    {
        bmp_fill(col_field,540,384,152,72); // clear the "[999]"
        char msg[6];
        snprintf(msg, sizeof(msg), "[%4d%]", avail_shot);
        bfnt_puts(msg, 560 , 402, COLOR_FG_NONLV, col_field);
    }
#endif
    
	bg = bmp_getpixel(590, 28);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
    
	if (lens_info.raw_iso == 0) // ISO: MAX AUTO
 	{
        int maxiso=(auto_iso_range %  0xFF) - (auto_iso_range >> 8);
        bmp_printf(fnt, 590, 28, "MAX:%d",raw2iso(maxiso) );
	}
	iso_refresh_display();
    
	bg = bmp_getpixel(HDR_STATUS_POS_X, HDR_STATUS_POS_Y);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	hdr_display_status(fnt);
    
	bg = bmp_getpixel(MLU_STATUS_POS_X, MLU_STATUS_POS_Y);
	fnt = FONT(FONT_SMALL, COLOR_FG_NONLV, bg);
	bmp_printf(fnt, MLU_STATUS_POS_X, MLU_STATUS_POS_Y, get_mlu() ? "MLU" : "   ");
    
	//~ display_lcd_remote_info();
	display_trap_focus_info();
}
