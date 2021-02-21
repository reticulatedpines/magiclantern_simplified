// Ported from CHDK
// (C) The CHDK team


#include "dryos.h"
#include "bmp.h"
#include "math.h"

//-------------------------------------------------------------------
#define swap(v1, v2)   {v1^=v2; v2^=v1; v1^=v2;}
//-------------------------------------------------------------------
void draw_line(int x1, int y1, int x2, int y2, int cl)
{
     uint8_t* bvram = bmp_vram();
    
     unsigned char steep = ABS(y2 - y1) > ABS(x2 - x1);
     if (steep) {
         swap(x1, y1);
         swap(x2, y2);
     }
     if (x1 > x2) {
         swap(x1, x2);
         swap(y1, y2);
     }
     int deltax = x2 - x1;
     int deltay = ABS(y2 - y1);
     int error = 0;
     int y = y1;
     int ystep = (y1 < y2)?1:-1;
     int x;
     for (x=x1; x<=x2; ++x) {
         if (steep) bmp_putpixel_fast(bvram, y, x, cl);
         else bmp_putpixel_fast(bvram, x, y, cl);
         error += deltay;
         if ((error<<1) >= deltax) {
             y += ystep;
             error -= deltax;
         }
     }
}

//-------------------------------------------------------------------
void draw_circle(int x, int y, int r, int cl)
{
    uint8_t* bvram = bmp_vram();

    int dx = 0;
    int dy = r;
    int p=(3-(r<<1));

    do {
        bmp_putpixel_fast(bvram,(x+dx),(y+dy),cl);
        bmp_putpixel_fast(bvram,(x+dy),(y+dx),cl);
        bmp_putpixel_fast(bvram,(x+dy),(y-dx),cl);
        bmp_putpixel_fast(bvram,(x+dx),(y-dy),cl);
        bmp_putpixel_fast(bvram,(x-dx),(y-dy),cl);
        bmp_putpixel_fast(bvram,(x-dy),(y-dx),cl);
        bmp_putpixel_fast(bvram,(x-dy),(y+dx),cl);
        bmp_putpixel_fast(bvram,(x-dx),(y+dy),cl);

        ++dx;

        if (p<0) 
            p += ((dx<<2)+6);
        else {
            --dy;
            p += (((dx-dy)<<2)+10);
        }
    } while (dx<=dy);
}

void fill_circle(int x, int y, int r, int cl)
{
    int dx = 0;
    int dy = r;
    int p=(3-(r<<1));

    do {
        draw_line(x+dx, y+dy, x+dx, y-dy, cl);
        draw_line(x-dx, y+dy, x-dx, y-dy, cl);
        draw_line(x+dy, y+dx, x+dy, y-dx, cl);
        draw_line(x-dy, y+dx, x-dy, y-dx, cl);

        ++dx;

        if (p<0) 
            p += ((dx<<2)+6);
        else {
            --dy;
            p += (((dx-dy)<<2)+10);
        }
    } while (dx<=dy);
}

void draw_angled_line(int x, int y, int r, int ang, int cl)
{
   #define MUL 16384
   #define PI_1800 0.00174532925
   int s = sinf(ang * PI_1800) * MUL;
   int c = cosf(ang * PI_1800) * MUL;
   draw_line(x, y, x + r * c / MUL, y + r * s / MUL, cl);
}
