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
