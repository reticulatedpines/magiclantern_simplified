// misc functions specific to 550D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

void display_shooting_info() // called from debug task
{
	if (lv) return;
	
	int bg = bmp_getpixel(X0+314, Y0+260);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, X0+320, Y0+260, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, X0+435, Y0+240, "%s%d ", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(fnt, X0+435, Y0+240, "   ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, X0+435, Y0+270, "%s%d ", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(fnt, X0+435, Y0+270, "   ");
	}

	iso_refresh_display();

	bg = bmp_getpixel(X0+15, Y0+430);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	hdr_display_status(fnt);

	bmp_printf(fnt, X0+40, Y0+460, get_mlu() ? "MLU" : "   ");

	display_lcd_remote_icon(X0+480, Y0);
	display_trap_focus_info();
}

// dummy stub
int new_LiveViewApp_handler = 0xff123456;
