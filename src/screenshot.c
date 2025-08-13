/* BMP screenshots */

#include "dryos.h"
#include "bmp.h"
#include "imgconv.h"
#include "beep.h"
#include "screenshot.h"

#ifdef FEATURE_SCREENSHOT

// Takes a buffer containing 24-bpp RGB data, 3 bytes per pixel,
// saves to BMP format through the provided file handle.
// Width and height are in pixels.
// Caller is responsible for ensuring buffer is large enough!
// Caller is responsible for closing handle.
static int save_bmp_file(FILE *fp, uint8_t *rgb, uint32_t width, uint32_t height)
{
    // working from:
    // https://cdn.hackaday.io/files/274271173436768/Simplified%20Windows%20BMP%20Bitmap%20File%20Format%20Specification.htm

    // For BMP, scanlines must be padded so each line is a multiple of 4 bytes.
    uint32_t pad_len = 4 - (width % 4);
    if (pad_len == 4)
        pad_len = 0;
    uint32_t file_size = 54 // fixed header size
                         + (height * width * 3)
                         + (height * pad_len);

    uint8_t *buf = malloc(file_size);
    if (buf == NULL)
        return -1;
    uint8_t *buf_start = buf;

    // fixed header with BMP magic
/*
    struct bmp_file_header bmp_file_header =
    {
        .type = {'B', 'M'},
        .size = file_size,
        .reserved = 0,
        .off_bits = 54
    };
*/
    // Copy to buffer.  NB un-aligned access to buffers is weird on
    // some ARM archs, and we care about endianness, so, write it all as bytes.
    *buf++ = 'B';
    *buf++ = 'M';
    *buf++ = (uint8_t)(file_size & 0xff);
    *buf++ = (uint8_t)((file_size >> 8) & 0xff);
    *buf++ = (uint8_t)((file_size >> 16) & 0xff);
    *buf++ = (uint8_t)((file_size >> 24) & 0xff);
    *buf++ = 0;; // reserved, always 0
    *buf++ = 0;
    *buf++ = 0;
    *buf++ = 0;
    *buf++ = 54; // image data offset from file start
    *buf++ = 0;
    *buf++ = 0;
    *buf++ = 0;

    // image header
/*
    struct bmp_image_header bmp_image_header =
    {
        .size = 40,
        .width = width,
        .height = height,
        .planes = 1,
        .bit_count = 24,
        .compression = 0,
        .size_image = 0,
        .x_pixels_per_meter = 0,
        .y_pixels_per_meter = 0,
        .colour_used = 0,
        .colour_important = 0
    };
*/
    // BMP data is stored from bottom row to top.
    // Inverting the height allows us to store "normally"
    // and display normally.
    height = -height;

    *buf++ = 40; // uint32_t header size
    *buf++ = 0;
    *buf++ = 0;
    *buf++ = 0;
    *buf++ = (uint8_t)(width & 0xff);
    *buf++ = (uint8_t)((width >> 8) & 0xff);
    *buf++ = (uint8_t)((width >> 16) & 0xff);
    *buf++ = (uint8_t)((width >> 24) & 0xff);
    *buf++ = (uint8_t)(height & 0xff);
    *buf++ = (uint8_t)((height >> 8) & 0xff);
    *buf++ = (uint8_t)((height >> 16) & 0xff);
    *buf++ = (uint8_t)((height >> 24) & 0xff);
    *buf++ = 1; // uint16_t planes
    *buf++ = 0;
    *buf++ = 24; // uint16_t bpp
    *buf++ = 0;
    // 6 dwords, all 0:
    // compression type, image size, x pix per m, y pix per m,
    // colours used, important colours
    for (int n = 0; n < 6; n++)
    {
        *(buf + 0) = 0;
        *(buf + 1) = 0;
        *(buf + 2) = 0;
        *(buf + 3) = 0;
        buf += 4;
    }

    height = -height; // restore, so we can loop using it

    // now, the padded image data
    for (uint32_t r = 0; r < height; r++)
    {
        for (uint32_t c = 0; c < width; c++)
        {
            // saved as RGB, but little-endian
            *buf++ = *(rgb + 2);
            *buf++ = *(rgb + 1);
            *buf++ = *rgb;
            rgb += 3;
        }
        for (uint32_t p = 0; p < pad_len; p++)
        {
            *buf++ = 0;
        }
    }

    FIO_WriteFile(fp, buf_start, file_size);

    free(buf);
    return 0;
}

#ifdef CONFIG_DIGIC_45
int take_screenshot( char* filename, uint32_t mode )
{
    /* image buffers */
    uint8_t *rgb = NULL;
    uint8_t *bmp_copy = NULL;
    uint32_t *yuv_copy = NULL;
    
    beep();
    info_led_on();
    
    /* what to save? */
    int save_bmp = mode & SCREENSHOT_BMP;
    int save_yuv = mode & SCREENSHOT_YUV;
    
    uint8_t *bvram = bmp_vram();
    uint8_t *lvram = NULL;
    struct vram_info *vram_info = get_yuv422_vram();
    if (vram_info != NULL)
        lvram = vram_info->vram;
    
    if (!lvram)
    {
        /* can we save the YUV buffer? (it might be uninitialized, e.g. in photo mode before going to LV) */
        save_yuv = 0;
    }

    /* do a fast temporary copy of the VRAMs to minimize motion artifacts (tearing) */
    if (save_yuv)
    {
        yuv_copy = tmp_malloc(vram_lv.width * vram_lv.pitch);
        if (!yuv_copy)
            goto err;
        memcpy(yuv_copy, lvram, vram_lv.width * vram_lv.pitch);
    }

    bmp_copy = tmp_malloc(720 * 480);
    if (!bmp_copy)
        goto err;
    for (int y = 0; y < 480; y++)
    {
        memcpy(bmp_copy + y*720, &bvram[BM(0,y)], 720);
    }

    /* setup output buffer */
    /* todo: support HDMI resolutions? */
    rgb = malloc(720 * 480 * 3);
    if (!rgb)
        goto err;
    
    /* fill it with data */
    for (int y = 0; y < 480; y++)
    {
        for (int x = 0; x < 720; x++)
        {
            int p = 0;
            uint8_t Y = 0; int8_t U = 0; int8_t V = 0;
            uint32_t pal = 0; uint8_t opacity = 0;
            
            if (save_bmp)
            {
                /* get pixel at (x,y) */
                p = bmp_copy[x + y*720];
                
                /* get palette entry (including our DIGIC pokes, if any) */
                pal = shamem_read(LCD_Palette[3*p]);
                if (!pal)
                    pal = LCD_Palette[3*p + 2];
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
                    uyvy = yuv422_get_pixel(yuv_copy, BM2LV(x,y)/2);
                }
                Y = UYVY_GET_AVG_Y(uyvy);
                U = UYVY_GET_U(uyvy);
                V = UYVY_GET_V(uyvy);
            }
            else if (opacity == 0 || opacity == 1)  /* semi-transparent? */
            {
                if (save_yuv)
                {
                    uyvy = yuv422_get_pixel(yuv_copy, BM2LV(x,y)/2);
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

    /* no longer needed, output image created */
    free(bmp_copy); bmp_copy = 0;
    free(yuv_copy); yuv_copy = 0;

    /* output filename */
    char path[100];
    
    if (filename == SCREENSHOT_FILENAME_AUTO)
    {
        get_numbered_file_name("VRAM%d.BMP", 9999, path, sizeof(path));
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

    FILE *f = FIO_CreateFile(path);
    if (!f)
    {
        goto err;
    }
    
    /* 8-bit RGB BMP */
    save_bmp_file(f, rgb, 720, 480);
    FIO_CloseFile(f);
    free(rgb);
    return 1;

err:
    if (rgb)
        free(rgb);
    if (bmp_copy)
        free(bmp_copy);
    if (yuv_copy)
        free(yuv_copy);
    info_led_off();
    return 0;
}
#elif defined(CONFIG_DIGIC_678X)
// a lot of duplication, could instead ifdef the RGB stuff
// above, based on FEATURE_VRAM_RGBA maybe
int take_screenshot( char* filename, uint32_t mode )
{
    /* image buffers */
    uint8_t *rgb = NULL;
    uint8_t *bmp_copy = NULL;
    uint32_t *yuv_copy = NULL;

    beep();
    info_led_on();

    /* what to save? */
    // "save_bmp" means save ML drawn elements,
    // "save_yuv" means Canon Liveview elements.
    // You can select both.
    int save_bmp = mode & SCREENSHOT_BMP;
    int save_yuv = mode & SCREENSHOT_YUV;

    uint8_t *bvram = bmp_vram();
    uint8_t *lvram = NULL;
    struct vram_info *vram_info = get_yuv422_vram();
    if (vram_info != NULL)
        lvram = vram_info->vram;
#ifdef CONFIG_DIGIC_678X // SJE FIXME confirmed on 7 and 8 only
    if (YUV422_LV_BUFFER_DISPLAY_ADDR == 0x01000000) // indicates uninit buffer
        lvram = NULL;
#endif

    if (!lvram)
    {
        /* can we save the YUV buffer? (it might be uninitialized, e.g. in photo mode before going to LV) */
        save_yuv = 0;
    }

    /* do a fast temporary copy of the VRAMs to minimize motion artifacts (tearing) */
    if (save_yuv)
    {
        yuv_copy = tmp_malloc(vram_lv.width * vram_lv.pitch);
        if (!yuv_copy)
            goto err;
        memcpy(yuv_copy, lvram, vram_lv.width * vram_lv.pitch);
    }

    bmp_copy = tmp_malloc(720 * 480);
    if (!bmp_copy)
        goto err;
    for (int y = 0; y < 480; y++)
    {
        memcpy(bmp_copy + y*720, &bvram[BM(0,y)], 720);
    }

    /* setup output buffer */
    /* todo: support HDMI resolutions? */
    rgb = malloc(720 * 480 * 3);
    if (!rgb)
        goto err;

    /* fill it with data */
    for (int y = 0; y < 480; y++)
    {
        for (int x = 0; x < 720; x++)
        {
            int p = 0;
            uint8_t Y = 0; int8_t U = 0; int8_t V = 0;
            int R, G, B, A;
            uint32_t colour;

            if (save_bmp)
            {
                /* get pixel at (x,y) */
                p = bmp_copy[x + y*720];

                colour = indexed2rgb(p);
                R = colour & 0xff;
                G = (colour & 0xff00) >> 0x8;
                B = (colour & 0xff0000) >> 0x10;
                A = (colour & 0xff000000) >> 0x18;
            }
            else
            {
                // don't save BMP overlay => force pixel transparent
                A = 0x00;
            }

            uint32_t uyvy = 0;
            if (A == 0x00) // ML pixel transparent, consider drawing Canon pixel
            {
                if (save_yuv)
                {
                    uyvy = yuv422_get_pixel(yuv_copy, BM2LV(x,y)/2);
                }
                Y = UYVY_GET_AVG_Y(uyvy);
                U = UYVY_GET_U(uyvy);
                V = UYVY_GET_V(uyvy);
                yuv2rgb(Y, U, V, &B, &G, &R);
            }
            // copy to our buffer
            rgb[(y*720 + x)*3 + 2] = R;
            rgb[(y*720 + x)*3 + 1] = G;
            rgb[(y*720 + x)*3 + 0] = B;
        }
    }
    info_led_off();

    /* no longer needed, output image created */
    free(bmp_copy); bmp_copy = 0;
    free(yuv_copy); yuv_copy = 0;

    /* output filename */
    char path[100];

    if (filename == SCREENSHOT_FILENAME_AUTO)
    {
        get_numbered_file_name("VRAM%d.BMP", 9999, path, sizeof(path));
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

    FILE *f = FIO_CreateFile(path);
    if (!f)
    {
        goto err;
    }

    /* 8-bit RGB BMP */
    save_bmp_file(f, rgb, 720, 480);
    FIO_CloseFile(f);
    free(rgb);
    return 1;

err:
    if (rgb)
        free(rgb);
    if (bmp_copy)
        free(bmp_copy);
    if (yuv_copy)
        free(yuv_copy);
    info_led_off();
    return 0;
}
#else
    #error "Expected Digic 4-X inclusive"
#endif // Digic version checks

#endif // FEATURE_SCREENSHOT
