/* PPM screenshots */

#include "dryos.h"
#include "bmp.h"
#include "vram.h"
#include "imgconv.h"

#ifdef FEATURE_SCREENSHOT

int take_screenshot( char* filename, uint32_t mode )
{
    beep();
    
    /* what to save? */
    int save_bmp = mode & SCREENSHOT_BMP;
    int save_yuv = mode & SCREENSHOT_YUV;
    
    uint32_t* lv_buffer = (uint32_t*) get_yuv422_vram()->vram;
    
    if (!lv_buffer)
    {
        /* can we save the YUV buffer? (it might be uninitialized, e.g. in photo mode before going to LV) */
        save_yuv = 0;
    }

    /* setup buffer */
    /* todo: support HDMI resolutions? */
    unsigned char * rgb = malloc(720 * 480 * 3);
    if (!rgb)
    {
        return 0;
    }
    
    /* fill it with data */
    info_led_on();
    for (int y = 0; y < 480; y++)
    {
        for (int x = 0; x < 720; x++)
        {
            int p = bmp_getpixel(x, y);
            
            uint8_t Y = 0; int8_t U = 0; int8_t V = 0;
            uint32_t pal = 0; uint8_t opacity = 0;
            
            if (save_bmp)
            {
                /* get palette entry (including our DIGIC pokes, if any) */
                pal = shamem_read(LCD_Palette[3*p]);
                if (!pal) pal = LCD_Palette[3*p + 2];
                opacity = (pal >> 24) & 0xFF;
                Y = (pal >> 16) & 0xFF;
                U = (pal >>  8) & 0xFF;
                V = (pal >>  0) & 0xFF;
            }
            else
            {
                /* don't save BMP overlay => just pretend the entire palette is transparent */
                pal = 0x00FF0000;
            }
            
            uint32_t uyvy = 0;

            /* handle transparency (incomplete, needs more reverse engineering) */
            if (pal == 0x00FF0000) /* fully transparent */
            {
                if (save_yuv)
                {
                    uyvy = yuv422_get_pixel(lv_buffer, BM2LV(x,y)/2);
                }
                Y = UYVY_GET_AVG_Y(uyvy);
                U = UYVY_GET_U(uyvy);
                V = UYVY_GET_V(uyvy);
            }
            else if (opacity == 0 || opacity == 1)  /* semi-transparent? */
            {
                if (save_yuv)
                {
                    uyvy = yuv422_get_pixel(lv_buffer, BM2LV(x,y)/2);
                }
                uint8_t Y2 = UYVY_GET_AVG_Y(uyvy);
                int8_t U2 = UYVY_GET_U(uyvy);
                int8_t V2 = UYVY_GET_V(uyvy);
                
                Y = ((int)Y + (int)Y2) / 2;
                U = ((int)U + (int)U2) / 2;
                V = ((int)V + (int)V2) / 2;
            }
            /* other transparency codes? */
            
            /* convert to RGB */
            int R,G,B;
            yuv2rgb(Y, U, V, &R, &G, &B);
            
            /* copy to our buffer */
            rgb[(y*720 + x)*3    ] = R;
            rgb[(y*720 + x)*3 + 1] = G;
            rgb[(y*720 + x)*3 + 2] = B;
        }
    }
    info_led_off();

    /* output filename */
    char path[100];
    
    if (filename == SCREENSHOT_FILENAME_AUTO)
    {
        get_numbered_file_name("VRAM%d.PPM", 9999, path, sizeof(path));
    }
    else
    {
        if (strchr(filename, '%'))
        {
            get_numbered_file_name(filename, 9999, path, sizeof(path));
        }
        else
        {
            snprintf(path, sizeof(path), "%s", filename);
        }
    }

    FILE *f = FIO_CreateFileEx(path);
    if (f == INVALID_PTR)
    {
        free(rgb);
        return 0;
    }
    
    /* 8-bit RGB */
    my_fprintf(f, "P6\n720 480\n255\n");
    
    FIO_WriteFile(f, rgb, 720*480*3);
    
    FIO_CloseFile(f);
    free(rgb);

    return 1;
}
#endif
