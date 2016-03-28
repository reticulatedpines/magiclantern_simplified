#ifndef _DISP_DIRECT_H_
#define _DISP_DIRECT_H_

#define COLOR_EMPTY             0x00
#define COLOR_RED               0x01
#define COLOR_GREEN             0x02
#define COLOR_BLUE              0x03
#define COLOR_CYAN              0x04
#define COLOR_MAGENTA           0x05
#define COLOR_YELLOW            0x06
#define COLOR_ORANGE            0x07
#define COLOR_TRANSPARENT_BLACK 0x08
#define COLOR_BLACK             0x09
#define COLOR_GRAY              0x0A
#define COLOR_WHITE             0x0F

void disp_set_pixel(uint32_t x, uint32_t y, uint32_t color);
void disp_init();

void print_line(uint32_t color, uint32_t scale, char *txt);

#endif
