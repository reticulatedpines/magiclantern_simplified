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
	
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 315, 238, "      \n%5dK\n      ", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 402, 238, "%s%d ", ba > 0 ? "A" : "B", ABS(ba));
		//else bmp_printf(fnt, 402, 238, "   "); // not needed, camera redraws the place itself

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 402, 268, "%s%d ", gm > 0 ? "G" : "M", ABS(gm));
		//else bmp_printf(fnt, 402, 268, "   "); // not needed, camera redraws the place itself
	}

	iso_refresh_display();

	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	hdr_display_status(fnt);

	bmp_printf(fnt, 40, 460, get_mlu() ? "MLU" : "   ");

	display_lcd_remote_icon(480, 0);
	display_trap_focus_info();
}

// dummy stub
int new_LiveViewApp_handler = 0xff123456;
