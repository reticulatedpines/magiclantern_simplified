#define PLUGIN_CLIENT
#include "plugin.h"

#define FONT_LARGE              0x00030000

EXTERN_INT_FUNC(1, somefunc, void) {
	bmp_printf(FONT_LARGE, 50, 50, "TEXT!!!!!!");
	return 0;
}
