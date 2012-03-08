#define PLUGIN_CLIENT
#include "plugin.h"

static const char *const tok[] = { "abcd","efgh","ijkl","mnop","qrst","uvwx","yz","123","456","789","123","a","b","c","d","e","f","g","h","o","q","w","e" };

EXTERN_FUNC(MODULE_FUNC_INIT, struct plugin_descriptor *, somefunc) {
	bmp_printf(FONT_LARGE, 50, 50, "TEXT!!!!!!");
	msleep(100);
	int i;
	for (i = 0; i<20; i++) {
		bmp_printf(FONT_LARGE, i*20, i*20, tok[i]);
		msleep(100);
	}
	return 0;
}
