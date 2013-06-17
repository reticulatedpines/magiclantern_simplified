/*
 * Copyright (C) 2013 Magic Lantern Team
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "../lv_rec/lv_rec.h"
#include "../../src/raw.h"

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

lv_rec_file_footer_t lv_rec_footer;

    //~ { "Canon EOS 5D Mark III", 0, 0x3c80,
    //~ { 6722,-635,-963,-4287,12460,2028,-908,2162,5668 } },
    #define CAM_COLORMATRIX1                       \
     6722, 10000,     -635, 10000,    -963, 10000, \
    -4287, 10000,    12460, 10000,    2028, 10000, \
     -908, 10000,     2162, 10000,    5668, 10000

struct raw_info raw_info = {
    .api_version = 1,
    .bits_per_pixel = 14,
    .black_level = 1024,
    .white_level = 13000,
    .cfa_pattern = 0x02010100,          // Red  Green  Green  Blue
    .calibration_illuminant1 = 1,       // Daylight
    .color_matrix1 = {CAM_COLORMATRIX1},// camera-specific, from dcraw.c
    .dynamic_range = 1100,              // not correct; use numbers from DxO instead
};

int parse_ifd(int id, char* buf, int off)
{
    int entries = *(short*)(buf+off); off += 2;
    printf("ifd %d: (%d)\n", id, entries);
    int imagetype = -1;
    int i;
    for (i = 0; i < entries; i++)
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
                printf("subifd: %x\n", data);
                parse_ifd(id+10, buf, data);
                break;
        }
        
        if (imagetype == 0) /* NewSubFileType: Main Image */
        {
            switch (tag)
            {
                case 0x100: /* ImageWidth */
                    printf("width: %d\n", data);
                    raw_info.width = data;
                    break;
                case 0x101: /* ImageLength */
                    printf("height: %d\n", data);
                    raw_info.height = data;
                    break;
                case 0x111: /* StripOffset */
                    printf("buffer offset: %d\n", data);
                    raw_info.buffer = buf + data;
                    break;
                case 0xC61A: /* BlackLevel */
                    printf("black: %d\n", data);
                    raw_info.black_level = data;
                    break;
                case 0xC61D: /* WhiteLevel */
                    printf("white: %d\n", data);
                    raw_info.white_level = data;
                    break;
                case 0xC68D: /* active area */
                {
                    int* area = (void*)buf + data;
                    printf("crop: %d %d %d %d\n", area[0], area[1], area[2], area[3]);
                    memcpy(&raw_info.active_area, area, 4 * 4);
                    break;
                }
            }
        }
    }
    unsigned int next = *(unsigned int*)(buf+off); off += 4;
    return next;
}

static void reverse_bytes_order(char* buf, int count)
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

void raw_set_geometry(int width, int height, int skip_left, int skip_right, int skip_top, int skip_bottom)
{
    raw_info.width = width;
    raw_info.height = height;
    raw_info.pitch = raw_info.width * 14 / 8;
    raw_info.frame_size = raw_info.height * raw_info.pitch;
    raw_info.active_area.x1 = skip_left;
    raw_info.active_area.y1 = skip_top;
    raw_info.active_area.x2 = raw_info.width - skip_right;
    raw_info.active_area.y2 = raw_info.height - skip_bottom;
    raw_info.jpeg.x = 0;
    raw_info.jpeg.y = 0;
    raw_info.jpeg.width = raw_info.width - skip_left - skip_right;
    raw_info.jpeg.height = raw_info.height - skip_top - skip_bottom;
}

int main(int argc, char** argv)
{
    FILE* fi = fopen(argv[1], "rb");
    if (!fi) perror(argv[1]);
    fseek(fi, 0, SEEK_END);
    int size = ftell(fi);
    fseek(fi, 0, SEEK_SET);
    char* buf = malloc(size);
    printf("reading %s (size=%d)\n", argv[1], size);
    int br = fread(buf, 1, size, fi);
    printf("read %d\n", br);
    if (!buf) perror("malloc");

    //~ short* buf16 = (short*) buf;
    int* buf32 = (int*) buf;
    
    if (buf32[0] != 0x002A4949 && buf32[1] != 0x00000008)
        perror("not a chdk dng");
    
    int off = 8;
    int ifd;
    for (ifd = 0; off; ifd++)
    {
        off = parse_ifd(ifd, buf, off);
    }
    
    raw_set_geometry(raw_info.width, raw_info.height, raw_info.active_area.x1, raw_info.active_area.y1, raw_info.width - raw_info.active_area.x2, raw_info.height - raw_info.active_area.y2);
    
    raw_info.api_version = 1;
    strncpy((char*)lv_rec_footer.magic, "RAWM", 4);
    lv_rec_footer.raw_info = raw_info;
    lv_rec_footer.xRes = raw_info.width;
    lv_rec_footer.yRes = raw_info.height;
    lv_rec_footer.frameSize = raw_info.frame_size;
    lv_rec_footer.frameCount = 1;

    int len = strlen(argv[1]);
    argv[1][len-3] = 'R';
    argv[1][len-2] = 'A';
    argv[1][len-1] = 'W';
    
    reverse_bytes_order(raw_info.buffer, raw_info.frame_size);
    
    printf("writing %s\n", argv[1]);
    FILE* fo = fopen(argv[1], "wb");
    fwrite(raw_info.buffer, 1, raw_info.frame_size, fo);
    fwrite(&lv_rec_footer, 1, sizeof(lv_rec_footer), fo);
    fclose(fo);
    return 0;
}

