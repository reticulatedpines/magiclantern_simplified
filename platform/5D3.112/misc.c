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
	
	uint32_t fnt = SHADOW_FONT(FONT_MED);

	bmp_printf(fnt, 235, 415, "%d%% ", GetBatteryLevel());

	if (lens_info.wb_mode == WB_KELVIN)
	{
		bmp_printf(fnt, 152, 280, "%5dK", lens_info.kelvin);
	}
	if (lens_info.wbs_gm || lens_info.wbs_ba)
	{
		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(fnt, 270 + 2 * font_med.width, 280, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else    bmp_printf(fnt, 270 + 2 * font_med.width, 280, "  ");

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(fnt, 270, 280, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else    bmp_printf(fnt, 270, 280, "  ");
	}
	
	hdr_display_status(fnt);

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
	static int k = 0;
	k++;
	
	if (k % 10 == 0 &&
		lens_info.job_state == 0) // who knows what race conditions are here... I smell one :)
	{
		int x = 31;
		prop_request_change(PROP_BATTERY_REPORT, &x, 1); // see PROP_Request PROP_BATTERY_REPORT
	}
	
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
