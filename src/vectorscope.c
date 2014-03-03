/**\file
 * Draws an on-screen vectorscope
 */

#include "config.h"
#include "dryos.h"
#include "math.h"
#include "bmp.h"
#include "vram.h"
#include "menu.h"
#include "propvalues.h"

#ifdef FEATURE_VECTORSCOPE

/* runtime-configurable size */
#define vectorscope_width 256
#define vectorscope_height 256
/* 128 is also a good choice, but 256 is max. U and V are using that resolution */
#define VECTORSCOPE_WIDTH_MAX 256
#define VECTORSCOPE_HEIGHT_MAX 256

static CONFIG_INT( "vectorscope.draw", vectorscope_draw, 0);
static CONFIG_INT( "vectorscope.gain", vectorscope_gain, 0);

static uint8_t *vectorscope = NULL;

/* helper to draw <count> pixels at given position. no wrap checks when <count> is greater 1 */
static void 
vectorscope_putpixels(uint8_t *bmp_buf, int x_pos, int y_pos, uint8_t color, uint8_t count)
{
    int pos = x_pos + y_pos * vectorscope_width;

    while(count--)
    {
        bmp_buf[pos++] = 255 - color;
    }
}

/* another helper that draws a color dot at given position.
   <xc> and <yc> specify the center of our scope graphic.
   <frac_x> and <frac_y> are in 1/2048th units and specify the relative dot position.
 */
static void 
vectorscope_putblock(uint8_t *bmp_buf, int xc, int yc, uint8_t color, int32_t frac_x, int32_t frac_y)
{
    int x_pos = xc + (((int32_t)vectorscope_width * frac_x) >> 12);
    int y_pos = yc + ((-(int32_t)vectorscope_height * frac_y) >> 12);

    vectorscope_putpixels(bmp_buf, x_pos + 0, y_pos - 4, color, 1);
    vectorscope_putpixels(bmp_buf, x_pos + 0, y_pos + 4, color, 1);

    vectorscope_putpixels(bmp_buf, x_pos - 3, y_pos - 3, color, 7);
    vectorscope_putpixels(bmp_buf, x_pos - 3, y_pos - 2, color, 7);
    vectorscope_putpixels(bmp_buf, x_pos - 3, y_pos - 1, color, 7);
    vectorscope_putpixels(bmp_buf, x_pos - 4, y_pos + 0, color, 9);
    vectorscope_putpixels(bmp_buf, x_pos - 3, y_pos + 1, color, 7);
    vectorscope_putpixels(bmp_buf, x_pos - 3, y_pos + 2, color, 7);
    vectorscope_putpixels(bmp_buf, x_pos - 3, y_pos + 3, color, 7);
}

/* draws the overlay: circle with color dots. */
static void vectorscope_paint(uint8_t *bmp_buf, uint32_t x_origin, uint32_t y_origin)
{    
    //int r = vectorscope_height/2 - 1;
    int xc = x_origin + (vectorscope_width >> 1);
    int yc = y_origin + (vectorscope_height >> 1);

    /* red block at U=-14.7% V=61.5% => U=-304/2048th V=1259/2048th */
    vectorscope_putblock(bmp_buf, xc, yc, 8, -302, 1259);
    /* green block */
    vectorscope_putblock(bmp_buf, xc, yc, 7, -593, -1055);
    /* blue block */
    vectorscope_putblock(bmp_buf, xc, yc, 9, 895, -204);
    /* cyan block */
    vectorscope_putblock(bmp_buf, xc, yc, 5, 301, -1259);
    /* magenta block */
    vectorscope_putblock(bmp_buf, xc, yc, 14, 592, 1055);
    /* yellow block */
    vectorscope_putblock(bmp_buf, xc, yc, 15, -893, 204);
}

static void
vectorscope_clear()
{
    if(vectorscope != NULL)
    {
        bzero32(vectorscope, vectorscope_width * vectorscope_height * sizeof(uint8_t));
    }
}

static void
vectorscope_init()
{
    if(vectorscope == NULL)
    {
        vectorscope = malloc(VECTORSCOPE_WIDTH_MAX * VECTORSCOPE_HEIGHT_MAX * sizeof(uint8_t));
        vectorscope_clear();
    }
}

static int vectorscope_coord_uv_to_pos(int U, int V)
{
    /* convert YUV to vectorscope position */
    V *= vectorscope_height;
    V >>= 8;
    V += vectorscope_height >> 1;

    U *= vectorscope_width;
    U >>= 8;
    U += vectorscope_width >> 1;

    int pos = U + V * vectorscope_width;
    
    return pos;
}

static void
vectorscope_addpixel(uint8_t y, int8_t u, int8_t v)
{
    if(vectorscope == NULL)
    {
        return;
    }
    
    int V = -v << vectorscope_gain;
    int U = u << vectorscope_gain;
    
    int r = U*U + V*V;
    const int r_sqrt = (int)sqrtf(r);
    if (r > 124*124)
    {
        /* almost out of circle, mark it with red */
        for (int R = 124; R < 128; R++)
        {
            int c = U * R / r_sqrt;
            int s = V * R / r_sqrt;
            int pos = vectorscope_coord_uv_to_pos(c, s);
            vectorscope[pos] = 255 - COLOR_RED;
        }
    }
    else
    {
        if (vectorscope_gain)
        {
            /* simulate better resolution */
            U += rand()%2;
            V += rand()%2;
        }
        
        int pos = vectorscope_coord_uv_to_pos(U, V);
        
        /* increase luminance at this position. when reaching 4*0x2A, we are at maximum. */
        if(vectorscope[pos] < (0x2A << 2))
        {
            vectorscope[pos]++;
        }
    }
}

/* memcpy the second part of vectorscope buffer. uses only few resources */
static void
vectorscope_draw_image(uint32_t x_origin, uint32_t y_origin)
{    
    if(vectorscope == NULL)
    {
        return;
    }

    uint8_t * const bvram = bmp_vram();
    if (!bvram)
    {
        return;
    }

    vectorscope_paint(vectorscope, 0, 0);

    const uint32_t vsh2 = vectorscope_height >> 1;
    const int r = vsh2 - 1;
    const int r_plus1_square = (r+1)*(r+1);
    const int r_minus1_square = (r-1)*(r-1);

    for(uint32_t y = 0; y < vectorscope_height; y++)
    {
        #ifdef CONFIG_4_3_SCREEN
        uint8_t *bmp_buf = &(bvram[BM(x_origin, y_origin + (EXT_MONITOR_CONNECTED ? y : y*8/9))]);
        #else
        uint8_t *bmp_buf = &(bvram[BM(x_origin, y_origin+y)]);
        #endif

        const int yc = y - vsh2;
        const int yc_square = yc * yc;
        const int yc_663div1024 = (yc * 663) >> 10;

        for(uint32_t x = 0; x < vectorscope_width; x++)
        {
            uint8_t brightness = vectorscope[x + y*vectorscope_width];

            int xc = x - vsh2;
            int xc_square = xc * xc;
            int xc_plus_yc_square = xc_square + yc_square;
            int inside_circle = xc_plus_yc_square < r_minus1_square;
            int on_circle = !inside_circle && xc_plus_yc_square <= r_plus1_square;
            // kdenlive vectorscope:
            // center: 175,180
            // I: 83,38   => dx=-92, dy=142
            // Q: 320,87  => dx=145, dy=93
            // let's say 660/1024 is a good approximation of the slope

            // wikipedia image:
            // center: 318, 294
            // I: 171, 68  => 147,226
            // Q: 545, 147 => 227,147
            // => 663/1024 is a better approximation

            int on_axis = (x==vectorscope_width/2) || (y==vsh2) || (inside_circle && (xc==yc_663div1024 || -xc*663/1024==yc));

            if (on_circle || (on_axis && brightness==0))
            {
                //#ifdef CONFIG_4_3_SCREEN
                bmp_buf[x] = 60;
                //#else
                //bmp_buf[x] = COLOR_BLACK;
                //#endif
            }
            else if (inside_circle)
            {
                /* paint (semi)transparent when no pixels in this color range */
                if (brightness == 0)
                {
                    //#ifdef CONFIG_4_3_SCREEN
                    bmp_buf[x] = COLOR_WHITE; // semitransparent looks bad
                    //#else
                    //bmp_buf[x] = (x+y)%2 ? COLOR_WHITE : 0;
                    //#endif
                }
                else if (brightness > (0x2A << 2))
                {
                    /* some fake fixed color, for overlays */
                    bmp_buf[x] = 255 - brightness;
                }
                else if (brightness <= (0x29 << 2))
                {
                    /* 0x26 is the palette color for black plus max 0x29 until white */
                    bmp_buf[x] = 0x26 + (brightness >> 2);
                }
                else
                {   /* overflow */
                    bmp_buf[x] = COLOR_YELLOW;
                }
            }
        }
    }
}

static MENU_UPDATE_FUNC(vectorscope_update)
{
    if (vectorscope_draw && vectorscope_gain)
        MENU_SET_VALUE("ON, 2x");
}

int vectorscope_should_draw()
{
    return vectorscope_draw;
}

void vectorscope_request_draw(int flag)
{
    vectorscope_draw = flag;
}

void vectorscope_start()
{
    if(vectorscope_draw)
    {
        vectorscope_init();
        vectorscope_clear();
    }
}

void vectorscope_pixel_step(int Y, int U, int V)
{
    if (vectorscope_draw)
    {
        vectorscope_addpixel(Y, U, V);
    }
}

void vectorscope_redraw()
{
    if(vectorscope_draw)
    {
        /* make sure memory address of bvram will be 4 byte aligned */
        BMP_LOCK( vectorscope_draw_image(os.x0 + 32, 64); )
    }

}

static struct menu_entry vectorscope_menus[] = {
    #ifdef FEATURE_VECTORSCOPE
    {
        .name = "Vectorscope",
        .priv       = &vectorscope_draw,
        .max = 1,
        .update = vectorscope_update,
        .help = "Shows color distribution as U-V plot. For grading & WB.",
        .depends_on = DEP_GLOBAL_DRAW | DEP_EXPSIM,
        .children =  (struct menu_entry[]) {
            {
                .name = "UV scaling",
                .priv = &vectorscope_gain, 
                .max = 1,
                .choices = (const char *[]) {"OFF", "2x", "4x"},
                .help = "Scaling for input signal (useful with flat picture styles).",
            },
            MENU_EOL
        },
    },
    #endif
};

static void vectorscope_feature_init()
{
    menu_add( "Overlay", vectorscope_menus, COUNT(vectorscope_menus) );
}

INIT_FUNC(__FILE__, vectorscope_feature_init);

#endif // FEATURE_VECTORSCOPE
