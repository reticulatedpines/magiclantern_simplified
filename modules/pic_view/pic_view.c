/** Simple picture viewer **/
/** Integrates with ML file browser (file_man) */

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include "../file_man/file_man.h"

int bmp_show(char* file)
{
    void* bmp = bmp_load(file, 1);
    if (!bmp) return 0;
    bmp_draw_scaled_ex(bmp, 0, 0, 720, 480, 0);
    bmp_free(bmp);
    return 1;
}

int yuv422_show(char* filename)
{
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 ) return 0;
    uint32_t * buf = shoot_malloc(size);
    if (!buf) return 0;
    struct vram_info * vram = get_yuv422_vram();

    extern int lv;
    if (lv)
    {
        bmp_printf(FONT_MED, 0, 0, "Try again outside LiveView.");
        return 0;
    }

    clrscr();
    bmp_printf(FONT_MED, 600, 460, "%d", size);

    int w,h;
    // auto-generated code from 422-jpg.py

         if (size == 1120 *  746 * 2) { w = 1120; h =  746; } 
    else if (size == 1872 * 1080 * 2) { w = 1872; h = 1080; } 
    else if (size == 1024 *  680 * 2) { w = 1024; h =  680; } 
    else if (size == 1560 *  884 * 2) { w = 1560; h =  884; } 
    else if (size ==  944 *  632 * 2) { w =  944; h =  632; } 
    else if (size ==  928 *  616 * 2) { w =  928; h =  616; } 
    else if (size == 1576 * 1048 * 2) { w = 1576; h = 1048; } 
    else if (size == 1576 *  632 * 2) { w = 1576; h =  632; } 
    else if (size ==  720 *  480 * 2) { w =  720; h =  480; } 
    else if (size == 1056 *  704 * 2) { w = 1056; h =  704; } 
    else if (size == 1720 *  974 * 2) { w = 1720; h =  974; } 
    else if (size == 1280 *  580 * 2) { w = 1280; h =  580; } 
    else if (size ==  640 *  480 * 2) { w =  640; h =  480; } 
    else if (size == 1024 *  680 * 2) { w = 1024; h =  680; } 
    else if (size == 1056 *  756 * 2) { w = 1056; h =  756; } 
    else if (size == 1728 *  972 * 2) { w = 1728; h =  972; } 
    else if (size == 1680 *  945 * 2) { w = 1680; h =  945; } 
    else if (size == 1280 *  560 * 2) { w = 1280; h =  560; } 
    else if (size == 1152 *  768 * 2) { w = 1152; h =  768; } 
    else if (size == 1904 * 1274 * 2) { w = 1904; h = 1274; } 
    else if (size == 1620 * 1080 * 2) { w = 1620; h = 1080; } 
    else if (size == 1280 *  720 * 2) { w = 1280; h =  720; } 
	else if (size == 1808 * 1206 * 2) { w = 1808; h = 1206; } // 6D Movie
	else if (size == 1816 * 1210 * 2) { w = 1816; h = 1210; } // 6D Photo
	else if (size == 1104 *  736 * 2) { w = 1104; h =  736; } // 6D Zoom
	else if (size == 1680 *  952 * 2) { w = 1680; h =  952; } // 600D
	else if (size == 1728 *  972 * 2) { w = 1728; h =  972; } // 600D Crop
    else if (size == 960  *  639 * 2) { w =  960; h =  639; } // 650D LV STDBY
    else if (size == 1729 * 1151 * 2) { w = 1728; h = 1151; } // 650D 1080p/480p recording
    else if (size == 1280 * 689  * 2) { w = 1280; h =  689; } // 650D 720p recording
    else return 0;

    bmp_printf(FONT_MED, 600, 460, " %dx%d ", w, h);

    size_t rc = read_file( filename, buf, size );
    if( rc != size ) return 0;
    yuv_resize(buf, w, h, (uint32_t*)vram->vram, vram->width, vram->height);
    shoot_free(buf);
    return 1;
}

FILETYPE_HANDLER(bmp_filehandler)
{
    switch(cmd)
    {
        case FILEMAN_CMD_VIEW_IN_MENU:
            return bmp_show(filename) ? 1 : -1;
    }
    return 0;
}

FILETYPE_HANDLER(yuv422_filehandler)
{
    switch(cmd)
    {
        case FILEMAN_CMD_VIEW_IN_MENU:
            return yuv422_show(filename) ? 1 : -1;
    }
    return 0;
}

FILETYPE_HANDLER(dng_filehandler)
{
    /* not implemented yet */
    return 0;
}


static unsigned int pic_view_init()
{
    fileman_register_type("BMP", "Bitmap image", bmp_filehandler);
    fileman_register_type("422", "YUV422 image", yuv422_filehandler);
    fileman_register_type("DNG", "DNG image", dng_filehandler);
    return 0;
}

static unsigned int pic_view_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(pic_view_init)
    MODULE_DEINIT(pic_view_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "Image viewer")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "a1ex")
MODULE_STRINGS_END()

//~ MODULE_CBRS_START()
//~ MODULE_CBRS_END()


