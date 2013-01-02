// Ported from CHDK
// (C) The CHDK team


#include "dryos.h"
#include "bmp.h"

#define coord int
#define color uint8_t

//-------------------------------------------------------------------
#define swap(v1, v2)   {v1^=v2; v2^=v1; v1^=v2;}
//-------------------------------------------------------------------
void draw_line(coord x1, coord y1, coord x2, coord y2, color cl) {
     uint8_t* bvram = bmp_vram();
    
     unsigned char steep = abs(y2 - y1) > abs(x2 - x1);
     if (steep) {
         swap(x1, y1);
         swap(x2, y2);
     }
     if (x1 > x2) {
         swap(x1, x2);
         swap(y1, y2);
     }
     int deltax = x2 - x1;
     int deltay = abs(y2 - y1);
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
void draw_circle(coord x, coord y, const unsigned int r, color cl) {
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

#ifdef CONFIG_ELECTRONIC_LEVEL // only used for level indicator

#include "cordic-16bit.h"

void cordic_ex(int theta, int* s, int* c, int n)
{
    theta = mod(theta + 2*half_pi, 4*half_pi) - 2*half_pi; // range: -pi...pi
    if (theta < -half_pi || theta > half_pi)
    {
        if (theta < 0)
            cordic(theta + 2*half_pi, s, c, n);
        else
            cordic(theta - 2*half_pi, s, c, n);
        *s = -(*s);
        *c = -(*c);
    }
    else
    {
        cordic(theta, s, c, n);
    }
}

// slow like a snail, but at least it works :)
/*void draw_pie(int x, int y, int r, int ang_start, int ang_end, color cl)
{
    float a;
    for (a = ang_start*10; a < ang_end*10; a++)
    {
        int s, c;
        cordic_ex(a * MUL / 573, &s, &c, 16);
        draw_line(x, y, x + r * c / MUL, y + r * s / MUL, cl);
    }
}*/

void draw_angled_line(int x, int y, int r, int ang, color cl)
{
   int s, c;
   cordic_ex(ang * MUL / 573, &s, &c, 16);
   draw_line(x, y, x + r * c / MUL, y + r * s / MUL, cl);
}
#endif
