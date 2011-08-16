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
	font_small.bitmap = read_entire_file("B:/SMALL.FNT", &size);
	font_med.bitmap = read_entire_file("B:/MEDIUM.FNT", &size);
	font_large.bitmap = read_entire_file("B:/LARGE.FNT", &size);
}

static void init_fonts()
{
	task_create("load_fonts", 0x1c, 0, load_fonts, 0);
}

INIT_FUNC(__FILE__, init_fonts);
