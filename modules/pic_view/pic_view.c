/** Simple picture viewer **/
/** Integrates with ML file browser (file_man) */

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include "../file_man/file_man.h"
#include "raw.h"

#define T_BYTE      1
#define T_ASCII     2
#define T_SHORT     3
#define T_LONG      4
#define T_RATIONAL  5
#define T_SBYTE     6
#define T_UNDEFINED 7
#define T_SSHORT    8
#define T_SLONG     9
#define T_SRATIONAL 10
#define T_FLOAT     11
#define T_DOUBLE    12

static int tif_parse_ifd(int id, char* buf, int off)
{
    int entries = *(short*)(buf+off); off += 2;
    //~ printf("ifd %d: (%d)\n", id, entries);
    int imagetype = -1;
    for (int i = 0; i < entries; i++)
    {
        unsigned int tag = *(unsigned short*)(buf+off); off += 2;
        unsigned int type = *(unsigned short*)(buf+off); off += 2; (void)type;
        unsigned int count = *(unsigned int*)(buf+off); off += 4; (void)count;
        unsigned int data = *(unsigned int*)(buf+off); off += 4;
        //~ printf("%x %x %x %d\n", tag, type, count, data);
        
        switch (tag)
        {
            case 0xFE: /* NewSubFileType */
                imagetype = data;
                break;
            
            case 0x14A: /* SubIFD */
                //~ printf("subifd: %x\n", data);
                tif_parse_ifd(id+10, buf, data);
                break;
        }
        
        if (imagetype == 0) /* NewSubFileType: Main Image */
        {
            switch (tag)
            {
                case 0x100: /* ImageWidth */
                    //~ printf("width: %d\n", data);
                    raw_info.width = data;
                    break;
                case 0x101: /* ImageLength */
                    //~ printf("height: %d\n", data);
                    raw_info.height = data;
                    break;
                case 0x111: /* StripOffset */
                    //~ printf("buffer offset: %d\n", data);
                    raw_info.buffer = buf + data;
                    break;
                case 0xC61A: /* BlackLevel */
                    //~ printf("black: %d\n", data);
                    raw_info.black_level = data;
                    break;
                case 0xC61D: /* WhiteLevel */
                    //~ printf("white: %d\n", data);
                    raw_info.white_level = data;
                    break;
                case 0xC68D: /* active area */
                {
                    int* area = (void*)buf + data;
                    //~ printf("crop: %d %d %d %d\n", area[0], area[1], area[2], area[3]);
                    memcpy(&raw_info.active_area, area, 4 * 4);
                    break;
                }
            }
        }
    }
    unsigned int next = *(unsigned int*)(buf+off); off += 4;
    return next;
}

static void FAST reverse_bytes_order(char* buf, int count)
{
    short* buf16 = (short*) buf;
    int i;
    for (i = 0; i < count/2; i++)
    {
        short x = buf16[i];
        buf[2*i+1] = x;
        buf[2*i] = x >> 8;
    }
}

static int dng_show(char* filename)
{
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 ) return 0;
    char* buf = shoot_malloc(size);
    if (!buf) return 0;

    size_t rc = read_file( filename, buf, size );
    if( rc != size ) goto err;

    int* buf32 = (int*) buf;
    if (buf32[0] != 0x002A4949 && buf32[1] != 0x00000008)
    {
        bmp_printf(FONT_MED, 0, 0, "Not a CHDK DNG");
        goto err;
    }

    raw_info.width = 0;
    raw_info.height = 0;
    
    int off = 8;
    for (int ifd = 0; off; ifd++)
        off = tif_parse_ifd(ifd, buf, off);
    
    if (!raw_info.width) goto err;
    if (!raw_info.height) goto err;

    raw_set_geometry(raw_info.width, raw_info.height, raw_info.active_area.x1, raw_info.width - raw_info.active_area.x2, raw_info.active_area.y1, raw_info.height - raw_info.active_area.y2);
    raw_force_aspect_ratio_1to1();

    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);

    vram_clear_lv();
    raw_preview_fast();
    shoot_free(buf);
    raw_set_dirty();
    
    bmp_printf(FONT_MED, 600, 460, " %dx%d ", raw_info.jpeg.width, raw_info.jpeg.height);
    return 1;
err:
    shoot_free(buf);
    raw_set_dirty();
    return 0;
}

static int bmp_show(char* file)
{
    void* bmp = bmp_load(file, 1);
    if (!bmp) return 0;
    bmp_draw_scaled_ex(bmp, 0, 0, 720, 480, 0);
    bmp_free(bmp);
    return 1;
}

static int yuv422_show(char* filename)
{
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 ) return 0;
    uint32_t * buf = shoot_malloc(size);
    if (!buf) return 0;
    struct vram_info * vram = get_yuv422_vram();

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
    else goto err;

    bmp_printf(FONT_MED, 600, 460, " %dx%d ", w, h);

    size_t rc = read_file( filename, buf, size );
    if( rc != size ) goto err;
    yuv_resize(buf, w, h, (uint32_t*)vram->vram, vram->width, vram->height);
    shoot_free(buf);
    return 1;

err:
    shoot_free(buf);
    return 0;
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
    extern int gui_state;
    switch(cmd)
    {
        case FILEMAN_CMD_VIEW_IN_MENU:
            if (!menu_request_image_backend()) return 2;
            if (gui_state != GUISTATE_PLAYMENU) return 2;
            return yuv422_show(filename) ? 1 : -1;
    }
    return 0;
}

FILETYPE_HANDLER(dng_filehandler)
{
    extern int gui_state;
    switch(cmd)
    {
        case FILEMAN_CMD_VIEW_IN_MENU:
            if (!menu_request_image_backend()) return 2;
            if (gui_state != GUISTATE_PLAYMENU) return 2;
            return dng_show(filename) ? 1 : -1;
    }
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

//~ MODULE_CBRS_START()
//~ MODULE_CBRS_END()


