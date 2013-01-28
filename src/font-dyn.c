#include "font.h"
#include "bmp.h"

#ifndef CONFIG_STATIC_FONTS
struct font font_large = {
    .width      = 20,
    .height     = 32,
    .bitmap     = 0,
};

struct font font_med = {
    .width      = 12,
    .height     = 20,
    .bitmap     = 0,
};

struct font font_small = {
    .width      = 8,
    .height     = 12,
    .bitmap     = 0,
};
#endif

struct sfont font_large_shadow = {
    .width      = 20,
    .height     = 32,
    .bitmap     = 0,
};

struct sfont font_med_shadow = {
    .width      = 12,
    .height     = 20,
    .bitmap     = 0,
};

struct sfont font_small_shadow = {
    .width      = 8,
    .height     = 12,
    .bitmap     = 0,
};

static void shadow_char_compute(struct font * src, struct sfont * dst, char c)
{
    #define PIX(i,j) (src->bitmap[ c + ((i) << 7) ] & (1 << (31-(j))))
    #define PIX_SET(i,j) dst->bitmap[ c + ((i) << 7) ] |= (1 << (31-(j)))
    #define PIX_CLR(i,j) dst->bitmap[ c + ((i) << 7) ] &= ~(1 << (31-(j)))

    unsigned i,j;

    // first draw the shadow
    for( i = 1 ; i<src->height-1 ; i++ )
    {
        for( j=1 ; j<src->width-1 ; j++ )
        {
            if PIX(i,j)
            {
                PIX_SET(i-1,j-1); PIX_SET(i-1,j  ); PIX_SET(i-1,j+1);
                PIX_SET(i  ,j-1);                   PIX_SET(i  ,j+1);
                PIX_SET(i+1,j-1); PIX_SET(i+1,j  ); PIX_SET(i+1,j+1);
            }
        }
    }
    // then erase the body
    for( i = 0 ; i<src->height ; i++ )
    {
        for( j=0 ; j<src->width ; j++ )
        {
            if PIX(i,j)
            {
                PIX_CLR(i,j);
            }
        }
    }
}
static void shadow_fonts_compute()
{
    for (char c = ' '; c < '~'; c++)
    {
        shadow_char_compute(&font_large, &font_large_shadow, c);
        shadow_char_compute(&font_med, &font_med_shadow, c);
        shadow_char_compute(&font_small, &font_small_shadow, c);
    }
}

int fonts_done = 0;

void load_fonts(void* unused)
{
    if (fonts_done) return;
    
    // if something goes wrong, you will see chinese fonts :)
    
#ifndef CONFIG_STATIC_FONTS
    int size;
    for (int i = 0; i < 10; i++)
    {
        //cat SMALL.FNT MEDIUM.FNT LARGE.FNT > FONTS.DAT
        font_small.bitmap = (unsigned *) read_entire_file(CARD_DRIVE "ML/DATA/FONTS.DAT", &size);
        font_med.bitmap = font_small.bitmap + 6136/4; // size of SMALL.FNT
        font_large.bitmap = font_med.bitmap + 10232/4; // size of MEDIUM.FNT
        if (font_small.bitmap) break; // OK!

        bfnt_puts( "ML/DATA/FONTS.DAT retry...", 0, 0, COLOR_WHITE, COLOR_BLACK);
        msleep(500);
    }

    if (font_small.bitmap == 0) // fonts not loaded
    {
        clrscr();
        bfnt_puts("ML/DATA/FONTS.DAT not found", 0, 0, COLOR_WHITE, COLOR_BLACK);
        beep();
        msleep(2000);
        clrscr();
        bfnt_puts("Please copy all ML files!", 0, 0, COLOR_WHITE, COLOR_BLACK);
        beep();
        msleep(2000);
        fonts_done = 1;
        return;
    }
    //~ bfnt_puts("FONTS OK", 0, 0, COLOR_WHITE, COLOR_BLACK);
    //#else
#endif

    /*font_small_shadow.bitmap = SmallAlloc(size);
    memcpy(font_small_shadow.bitmap, font_small.bitmap, size);
    font_med_shadow.bitmap = font_small_shadow.bitmap + 6136/4; // size of SMALL.FNT
    font_large_shadow.bitmap = font_med_shadow.bitmap + 10232/4; // size of MEDIUM.FNT
	*/
    font_small_shadow.bitmap = SmallAlloc(font_small.height*4*0x80);
    memcpy(font_small_shadow.bitmap, font_small.bitmap, font_small.height*4*0x80);
    font_med_shadow.bitmap = SmallAlloc(font_med.height*4*0x80);
    memcpy(font_med_shadow.bitmap, font_med.bitmap, font_med.height*4*0x80);
    font_large_shadow.bitmap = SmallAlloc(font_large.height*4*0x80);
    memcpy(font_large_shadow.bitmap, font_large.bitmap, font_large.height*4*0x80);

    shadow_fonts_compute();
    fonts_done = 1;
}

//~ static void init_fonts()
//~ {
    //~ task_create("load_fonts", 0x1c, 0, load_fonts, 0);
    //~ while (!fonts_done) msleep(100);
//~ }

INIT_FUNC(__FILE__, load_fonts);
