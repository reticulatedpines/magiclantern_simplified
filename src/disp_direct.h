#ifndef _DISP_DIRECT_H_
#define _DISP_DIRECT_H_

#define COLOR_EMPTY_DD             0x00
#define COLOR_RED_DD               0x01
#define COLOR_GREEN_DD             0x02
#define COLOR_BLUE_DD              0x03
#define COLOR_CYAN_DD              0x04
#define COLOR_MAGENTA_DD           0x05
#define COLOR_YELLOW_DD            0x06
#define COLOR_ORANGE_DD            0x07
#define COLOR_TRANSPARENT_BLACK_DD 0x08
#define COLOR_BLACK_DD             0x09
#define COLOR_GRAY_DD              0x0A
#define COLOR_WHITE_DD             0x0F

void disp_set_pixel(uint32_t x, uint32_t y, uint32_t color);
void disp_init();

void print_line(uint32_t color, uint32_t scale, char *txt);

#endif
