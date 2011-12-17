#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "af_patterns.h"

static type_PATTERN_MAP_ITEM pattern_map[] = {
		{AF_PATTERN_CENTER,         AF_PATTERN_SQUARE, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},
		{AF_PATTERN_SQUARE,         AF_PATTERN_HLINE,  AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_TOP,            AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_TOPLEFT,       AF_PATTERN_TOPRIGHT},
		{AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_TOPDIAMOND,     AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_RIGHTTRIANGLE},
		{AF_PATTERN_TOPDIAMOND,     AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_TOPHALF,        AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_RIGHTDIAMOND},
		{AF_PATTERN_TOPHALF,        AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_HLINE,          AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_BOTTOM,         AF_PATTERN_CENTER, AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_BOTTOM,         AF_PATTERN_BOTTOMLEFT,    AF_PATTERN_BOTTOMRIGHT},
		{AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_CENTER, AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_BOTTOM,         AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_RIGHTTRIANGLE},
		{AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_CENTER, AF_PATTERN_BOTTOMHALF,     AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_RIGHTDIAMOND},
		{AF_PATTERN_BOTTOMHALF,     AF_PATTERN_CENTER, AF_PATTERN_HLINE,          AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_TOPLEFT,        AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_LEFT,          AF_PATTERN_TOPRIGHT},
		{AF_PATTERN_TOPRIGHT,       AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_TOPLEFT,       AF_PATTERN_RIGHT},
		{AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_CENTER, AF_PATTERN_TOPLEFT,        AF_PATTERN_BOTTOM,         AF_PATTERN_LEFT,          AF_PATTERN_BOTTOMRIGHT},
		{AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_CENTER, AF_PATTERN_TOPRIGHT,       AF_PATTERN_BOTTOM,         AF_PATTERN_BOTTOMLEFT,    AF_PATTERN_RIGHT},

		{AF_PATTERN_LEFT,           AF_PATTERN_CENTER, AF_PATTERN_TOPLEFT,        AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_LEFT,          AF_PATTERN_LEFTTRIANGLE},
		{AF_PATTERN_LEFTTRIANGLE,   AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_LEFT,          AF_PATTERN_LEFTDIAMOND},
		{AF_PATTERN_LEFTDIAMOND,    AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_LEFTHALF},
		{AF_PATTERN_LEFTHALF,       AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_VLINE},

		{AF_PATTERN_RIGHT,          AF_PATTERN_CENTER, AF_PATTERN_TOPRIGHT,       AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_RIGHTTRIANGLE, AF_PATTERN_RIGHT},
		{AF_PATTERN_RIGHTTRIANGLE,  AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_RIGHTDIAMOND,  AF_PATTERN_RIGHT},
		{AF_PATTERN_RIGHTDIAMOND,   AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_RIGHTHALF,     AF_PATTERN_RIGHTTRIANGLE},
		{AF_PATTERN_RIGHTHALF,      AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_VLINE,         AF_PATTERN_RIGHTDIAMOND},

		{AF_PATTERN_HLINE,          AF_PATTERN_VLINE,  AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},
		{AF_PATTERN_VLINE,          AF_PATTERN_ALL,    AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_ALL,            AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		END_OF_LIST
};

int afp_transformer (int pattern, type_DIRECTION direction);

int afp[2];
PROP_HANDLER(PROP_AFPOINT)
{
	afp[0] = buf[0];
	afp[1] = buf[1];

	return prop_cleanup(token, property);
}
#define af_point afp[0]

void afp_show_in_viewfinder() // this function may be called from multiple tasks
{
BMP_LOCK( // reuse this for locking
	card_led_on();
	assign_af_button_to_halfshutter(); // this has semaphores
	SW1(1,150);
	SW1(0,50);
	restore_af_button_assignment();
	card_led_off();
)
}

void set_af_point(int afpoint)
{
	if (beep_enabled) Beep();
	afp[0] = afpoint;
	prop_request_change(PROP_AFPOINT, afp, 7);
	task_create("afp_tmp", 0x18, 0, afp_show_in_viewfinder, 0);
}

int afpoint_for_key_guess = 0;

void afp_center () {
	set_af_point(afp_transformer(af_point, DIRECTION_CENTER));
}

void afp_top () {
	set_af_point(afp_transformer(af_point, DIRECTION_UP));
}

void afp_bottom () {
	set_af_point(afp_transformer(af_point, DIRECTION_DOWN));
}

void afp_left () {
	set_af_point(afp_transformer(af_point, DIRECTION_LEFT));
}

void afp_right () {
	set_af_point(afp_transformer(af_point, DIRECTION_RIGHT));
}

int afp_transformer (int pattern, type_DIRECTION direction) {
	type_PATTERN_MAP_ITEM *item;

	// Loop over all items in the pattern map
	for (item = pattern_map; ! IS_EOL(item); item++) {

		// When we find an item matching the current pattern...
		if (item->pattern == pattern) {

			// ... we return the next pattern, according to the direction indicated
			switch (direction) {
			case DIRECTION_CENTER:
				return item->next_center;
			case DIRECTION_UP:
				return item->next_top;
			case DIRECTION_DOWN:
				return item->next_bottom;
			case DIRECTION_LEFT:
				return item->next_left;
			case DIRECTION_RIGHT:
				return item->next_right;
			}
		}
	}

	// Just in case something goes wrong
	return AF_PATTERN_CENTER;
}

int handle_af_patterns(struct event * event)
{
	extern int af_patterns;
	if (af_patterns && !lv && gui_state == GUISTATE_IDLE && tft_status)
	{
		switch (event->param)
		{
		case BGMT_PRESS_LEFT:
			afp_left();
			return 0;
		case BGMT_PRESS_RIGHT:
			afp_right();
			return 0;
		case BGMT_PRESS_UP:
			afp_top();
			return 0;
		case BGMT_PRESS_DOWN:
			afp_bottom();
			return 0;
		case BGMT_PRESS_SET:
			#ifdef CONFIG_60D
			if (get_cfn_function_for_set_button()) return 1; // do that custom function instead
			#endif
			afp_center();
			return 0;
		#ifdef BGMT_PRESS_UP_LEFT
		case BGMT_PRESS_UP_LEFT:
		case BGMT_PRESS_UP_RIGHT:
		case BGMT_PRESS_DOWN_LEFT:
		case BGMT_PRESS_DOWN_RIGHT:
			afp_center();
			return 0;
		#endif
		}
	}
	return 1;
}
