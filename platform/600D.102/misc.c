// misc functions specific to 600D/102

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

#include <../src/photoinfo.h>
    
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


// some dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}

void* AllocateMemory(size_t size) // this won't throw ERR70
{
	return (void*) AllocateMemory_do(*(int*)0x3070, size);
}
