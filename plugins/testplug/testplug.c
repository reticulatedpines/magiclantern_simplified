#define PLUGIN_CLIENT
#include "plugin.h"

#define FONT_LARGE              0x00030000

static const char *const tok[] = { "abcd","efgh","ijkl","mnop","qrst","uvwx","yz","123","456","789","123","a","b","c","d","e","f","g","h","o","q","w","e" };

EXTERN_FUNC(1, int, somefunc, void) {
	bmp_printf(FONT_LARGE, 50, 50, "TEXT!!!!!!");
	int i;
	for (i = 0; i<20; i++) {
		bmp_printf(FONT_LARGE, 50, 50, tok[i]);
	}
	return 0;
}
