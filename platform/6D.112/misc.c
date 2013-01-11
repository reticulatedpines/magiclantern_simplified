// misc functions specific to 60D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

void display_shooting_info() // called from debug task
{
	if (lv) return;
	int bg = bmp_getpixel(1,1);
	uint32_t fnt = FONT(FONT_MED,COLOR_YELLOW,bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 188, 280, "%5d", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		int ba = lens_info.wbs_ba;
		int x = 314;
		if (ba) bmp_printf(fnt, x + 2 * font_med.width, 280, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else    bmp_printf(fnt, x + 2 * font_med.width, 280, "  ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, x, 280, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else    bmp_printf(fnt, x, 280, "  ");
	}
	
	hdr_display_status(fnt);

    RedrawBatteryIcon();
    
	bmp_printf(fnt, 374, 307, get_mlu() ? "MLU" : "   ");

	display_trap_focus_info();
}

int new_LiveViewApp_handler = 0xff123456;

// dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}
