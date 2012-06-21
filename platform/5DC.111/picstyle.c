#include <dryos.h>
#include <lens.h>
#include <property.h>
#include <bmp.h>
#include <config.h>

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
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_PORTRAIT ) {
	memcpy(&picstyle_settings[2], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_LANDSCAPE ) {
	memcpy(&picstyle_settings[3], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_NEUTRAL ) {
	memcpy(&picstyle_settings[4], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_FAITHFUL ) {
	memcpy(&picstyle_settings[5], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_MONOCHROME ) {
	memcpy(&picstyle_settings[6], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_USERDEF1 ) {
	memcpy(&picstyle_settings[7], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_USERDEF2 ) {
	memcpy(&picstyle_settings[8], buf, 24);
}

PROP_HANDLER( PROP_PICSTYLE_SETTINGS_USERDEF3 ) {
	memcpy(&picstyle_settings[9], buf, 24);
}
