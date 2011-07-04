#include "../../dryos.h"
#include "../../lens.h"
#include "../../property.h"
#include "../../bmp.h"
#include "../../config.h"

extern int lv_focus_done;

struct lens_control {
	// 0x00-0x1D4: not used
	                   uint32_t off_0x1D4;uint32_t off_0x1D8;uint32_t off_0x1DC; //1D4: amount of rotation, 1D8: step size (?)
	uint32_t off_0x1E0;uint32_t off_0x1E4;uint32_t off_0x1E8;uint32_t off_0x1EC;
	uint32_t off_0x1F0;uint32_t off_0x1F4;uint32_t off_0x1F8;uint32_t off_0x1FC;
	uint32_t off_0x200;uint32_t off_0x204;uint32_t off_0x208;uint32_t off_0x20C;
	uint32_t off_0x210;uint32_t off_0x214;uint32_t off_0x218;uint32_t off_0x21C;
	uint32_t off_0x220;uint32_t off_0x224;uint32_t off_0x228;
}__attribute__((packed));

struct lens_control lctr;

void
lens_focus(
	unsigned		mode,
	int			step
)
{
	if (!lv) return;
	if (is_manual_focus()) return;

	while (lens_info.job_state) msleep(100);
	lens_focus_wait();
	lv_focus_done = 0;

	lctr.off_0x228 = 0x1;
	if (step<0) { lctr.off_0x228 += 0x8000; step = -step; }

	float a = lens_info.lens_rotation/((float)step);
	float b = lens_info.lens_step;
	uint32_t* aconv = &a;
	uint32_t* bconv = &b;

	lctr.off_0x1D4 = SWAP_ENDIAN(*aconv); //0x00400F3C; // single floating point number: 0x008743286
	lctr.off_0x1D8 = SWAP_ENDIAN(*bconv); // single floating point number: 1
	lctr.off_0x1F4 = 0x08080000;
	lctr.off_0x200 = 0x00003200;
	lctr.off_0x20C = 0xFF000000;
	lctr.off_0x210 = 0x000000FF;
	lctr.off_0x214 = 0xFFFFFF00;
	lctr.off_0x218 = 0x0000FFFF;
	
	AfCtrl_SetLensParameterRemote(((char*)&lctr)-0x1D4);

	if (get_zoom_overlay_mode()==2) zoom_overlay_set_countdown(300);
}

int get_prop_picstyle_from_index(int index)
{
	switch(index)
	{
		case 1: return 0x81;
		case 2: return 0x82;
		case 3: return 0x83;
		case 4: return 0x84;
		case 5: return 0x85;
		case 6: return 0x86;
		case 7: return 0x21;
		case 8: return 0x22;
		case 9: return 0x23;
	}
	bmp_printf(FONT_LARGE, 0, 0, "unk picstyle index: %x", index);
	return 0;
}

int get_prop_picstyle_index(int pic_style)
{
	switch(pic_style)
	{
		case 0x81: return 1;
		case 0x82: return 2;
		case 0x83: return 3;
		case 0x84: return 4;
		case 0x85: return 5;
		case 0x86: return 6;
		case 0x21: return 7;
		case 0x22: return 8;
		case 0x23: return 9;
	}
	bmp_printf(FONT_LARGE, 0, 0, "unk picstyle: %x", pic_style);
	return 0;
}

struct prop_picstyle_settings picstyle_settings[10];

// prop_register_slave is much more difficult to use than copy/paste...

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_STANDARD ) {
	memcpy(&picstyle_settings[1], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_PORTRAIT ) {
	memcpy(&picstyle_settings[2], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_LANDSCAPE ) {
	memcpy(&picstyle_settings[3], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_NEUTRAL ) {
	memcpy(&picstyle_settings[4], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_FAITHFUL ) {
	memcpy(&picstyle_settings[5], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_MONOCHROME ) {
	memcpy(&picstyle_settings[6], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_USERDEF1 ) {
	memcpy(&picstyle_settings[7], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_USERDEF2 ) {
	memcpy(&picstyle_settings[8], buf, 24);
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_USERDEF3 ) {
	memcpy(&picstyle_settings[9], buf, 24);
	return prop_cleanup( token, property );
}
