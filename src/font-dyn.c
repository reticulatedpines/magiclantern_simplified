#include "font.h"
#include "bmp.h"
struct font font_large = {
	.width		= 20,
	.height		= 32,
	.bitmap		= 0,
};

struct font font_med = {
	.width		= 12,
	.height		= 20,
	.bitmap		= 0,
};

struct font font_small = {
	.width		= 8,
	.height		= 12,
	.bitmap		= 0,
};

static void load_fonts(void* unused)
{
	// if something goes wrong, you will see chinese fonts :)
	int size;
	
	//cat SMALL.FNT MEDIUM.FNT LARGE.FNT > FONTS.DAT
	font_small.bitmap = read_entire_file(CARD_DRIVE "FONTS.DAT", &size);
	//~ font_med.bitmap = read_entire_file(CARD_DRIVE "MEDIUM.FNT", &size);
	//~ font_large.bitmap = read_entire_file(CARD_DRIVE "LARGE.FNT", &size);
	font_med.bitmap = font_small.bitmap + 6136/4; // size of SMALL.FNT
	font_large.bitmap = font_med.bitmap + 10232/4; // size of MEDIUM.FNT
}

static void init_fonts()
{
	task_create("load_fonts", 0x1c, 0, load_fonts, 0);
}

INIT_FUNC(__FILE__, init_fonts);
