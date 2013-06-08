// misc functions specific to 50D/109

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>

// some dummy stubs
int lcd_release_running = 0;
void lcd_release_step() {};
int get_lcd_sensor_shortcuts() { return 0; }
void display_lcd_remote_icon(int x0, int y0) {}
int new_LiveViewApp_handler = 0xff123456;

void display_shooting_info() // called from debug task
{
	if (lv) return;
	
	int bg = bmp_getpixel(314, 260);
	uint32_t fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 320, 260, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		fnt = FONT(FONT_LARGE, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 435, 240, "%s%d ", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(fnt, 435, 240, "   ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 435, 270, "%s%d ", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(fnt, 435, 270, "   ");
	}

	iso_refresh_display();

	bg = bmp_getpixel(15, 430);
	fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	extern int hdr_steps, hdr_stepsize, hdr_enabled;
	if (hdr_enabled)
		bmp_printf(fnt, 380, 450, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 380, 450, "           ");

	//~ bmp_printf(fnt, 200, 450, "Flash:%s", 
		//~ strobo_firing == 0 ? " ON" : 
		//~ strobo_firing == 1 ? "OFF" : "Auto"
		//~ strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
		//~ );

	bmp_printf(fnt, 40, 460, get_mlu() ? "MLU" : "   ");

	display_lcd_remote_icon(480, 0);
	display_trap_focus_info();
}


int audio_meters_are_drawn() { return 0; }
void volume_up(){};
void volume_down(){};
void out_volume_up(){};
void out_volume_down(){};
