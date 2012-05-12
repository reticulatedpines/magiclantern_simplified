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
	
	int bg = COLOR_BLACK;
	uint32_t fnt = SHADOW_FONT(FONT_MED);

	bmp_printf(fnt, 215, 385, "%d%% ", GetBatteryLevel());

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 490, 275, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		bg = bmp_getpixel(15, 460);
		fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);

		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 350 + 2 * font_med.width, 460, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else    bmp_printf(fnt, 350 + 2 * font_med.width, 460, "  ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 350, 460, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else    bmp_printf(fnt, 350, 460, "  ");
	}

	//~ iso_refresh_display();

	//~ bg = bmp_getpixel(15, 430);
	//~ fnt = FONT(FONT_MED, COLOR_FG_NONLV, bg);
	
	extern int hdr_steps, hdr_stepsize, hdr_enabled;
	if (hdr_enabled)
		bmp_printf(fnt, 180, 460, "HDR %dx%dEV", hdr_steps, hdr_stepsize/8);
	else
		bmp_printf(fnt, 180, 460, "         ");

	//~ bmp_printf(fnt, 400, 450, "Flash:%s", 
		//~ strobo_firing == 0 ? " ON" : 
		//~ strobo_firing == 1 ? "OFF" : "Auto"
		//~ strobo_firing < 2 && flash_and_no_flash ? "/T" : "  "
		//~ );

	bmp_printf(fnt, 40, 460, get_mlu() ? "MLU" : "   ");

	display_lcd_remote_icon(555, 460);
	display_trap_focus_info();
}


int battery_level = 0;
CONFIG_INT("battery.drain.rate.rev", battery_seconds_same_level_ok, 0);
int battery_seconds_same_level_tmp = 0;
int battery_level_transitions = 0;

PROP_HANDLER(PROP_BATTERY_REPORT)
{
	battery_level = buf[1] & 0xff;
}
int GetBatteryLevel()
{
	return battery_level;
}
int GetBatteryTimeRemaining()
{
	return battery_seconds_same_level_ok * battery_level;
}
int GetBatteryDrainRate() // percents per hour
{
	return 3600 / battery_seconds_same_level_ok;
}

// called every second
void RefreshBatteryLevel_1Hz()
{
	return;
	int x = 31;
	prop_request_change(PROP_BATTERY_REPORT, &x, 1); // see PROP_Request PROP_BATTERY_REPORT
	
	msleep(50);
	
	// check how many seconds battery indicator was at the same percentage
	// this is a rough indication of how fast the battery is draining
	static int old_battery_level = -1;
	if (battery_level == old_battery_level)
	{
		battery_seconds_same_level_tmp++;
	}
	else
	{
		battery_level_transitions++;
		if (battery_level_transitions >= 2)
			battery_seconds_same_level_ok = battery_seconds_same_level_tmp;
		battery_seconds_same_level_tmp = 0;
	}
	old_battery_level = battery_level;
}

// gcc mempcy has odd alignment issues?
void
my_memcpy(
    void *       dest,
    const void *     src,
    size_t          len
)
{
    while( len-- > 0 )
        *(uint8_t*)dest++ = *(const uint8_t*)src++;
}
