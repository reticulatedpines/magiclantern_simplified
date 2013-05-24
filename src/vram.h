#ifndef _video_h_
#define _video_h_

/** \file
 * Interface to the 5D Mark II's video ram (VRAM).
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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

#include "arm-mcr.h"


/** Write the VRAM to a BMP file named "A:/test.bmp" */
extern void
dispcheck( void );

/** Canon data structure containing BMP VRAM address.
 * 
 * LCD: it points to a 720x480 cropped area, but the image buffer is actually 960x540.
 * HDMI: it points to the full image buffer.
 * 
 * ML alters this pointer to always indicate the 720x480 cropped area 
 * (to avoid the possibility of buffer overflow due to race conditions when changing display modes).
 */
struct bmp_vram_info
{
        uint8_t *               vram0;
        uint32_t                off_0x04;
        uint8_t *               vram2;
};

extern struct bmp_vram_info bmp_vram_info[];

struct display_filter_buffers { void* src_buf; void* dst_buf; };


/** Internal ML structure for describing VRAM buffers.
 * 
 * History: it was reversed from 5D2, but couldn't be found on later cameras.
 *
 * Pixels are in an YUV 422 format.
 * This points to the image VRAM, not the bitmap vram
 */
struct vram_info
{
        uint8_t *       vram;
        int             width;
        int             pitch;
        int             height;
};

#ifdef CONFIG_VXWORKS
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) |  0x10000000))
#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) & ~0x10000000))
#else
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) |  0x40000000))
#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) & ~0x40000000))
#endif

void redraw();

struct vram_info * get_yuv422_vram();
struct vram_info * get_yuv422_hd_vram();
void display_filter_get_buffers(uint32_t** src_buf, uint32_t** dst_buf);


// [ sx   0   x ]
// [  0  sy   y ]
// [  0   0   1 ]

// inverse:
// [ 1/sx     0   -x/sx ]
// [    0  1/sy   -y/sy ]
// [    0     0       1 ]

struct trans2d // 2D homogeneous transformation matrix with translation and scaling components
{
        int tx;
        int ty;
        int sx; // * 1024
        int sy; // * 1024
};

extern struct trans2d bm2lv;
extern struct trans2d lv2hd;
extern struct trans2d lv2raw;
extern struct vram_info vram_hd;
extern struct vram_info vram_lv;
//~ extern struct vram_info vram_bm; 


 // offsets on one axis, in pixels
#define BM2LV_Xu(x) ((((x) * bm2lv.sx) >> 10) + bm2lv.tx)
#define BM2LV_Y(y) ((((y) * bm2lv.sy) >> 10) + bm2lv.ty)

extern int bm2lv_x_cache[];
#define BM2LV_X(x) bm2lv_x_cache[(x) - BMP_W_MINUS]

#define LV2BM_X(x) (((x) << 10) / bm2lv.sx - (bm2lv.tx << 10) / bm2lv.sx)
#define LV2BM_Y(y) (((y) << 10) / bm2lv.sy - (bm2lv.ty << 10) / bm2lv.sy)

#define LV2HD_X(x) ((((x) * lv2hd.sx) >> 10) + lv2hd.tx)
#define LV2HD_Y(y) ((((y) * lv2hd.sy) >> 10) + lv2hd.ty)

#define HD2LV_X(x) (((x) << 10) / lv2hd.sx - (lv2hd.tx << 10) / lv2hd.sx)
#define HD2LV_Y(y) (((y) << 10) / lv2hd.sy - (lv2hd.ty << 10) / lv2hd.sy)

#define BM2HD_X(x) LV2HD_X(BM2LV_X(x))
#define BM2HD_Y(y) LV2HD_Y(BM2LV_Y(y))

#define HD2BM_X(x) LV2BM_X(HD2LV_X(x))
#define HD2BM_Y(y) LV2BM_Y(HD2LV_Y(y))

// scaling a distance between image buffers
#define BM2LV_DX(x) (BM2LV_X(x) - BM2LV_X(0))
#define BM2LV_DY(y) (BM2LV_Y(y) - BM2LV_Y(0))

#define LV2BM_DX(x) (LV2BM_X(x) - LV2BM_X(0))
#define LV2BM_DY(y) (LV2BM_Y(y) - LV2BM_Y(0))

#define LV2HD_DX(x) (LV2HD_X(x) - LV2HD_X(0))
#define LV2HD_DY(y) (LV2HD_Y(y) - LV2HD_Y(0))

#define HD2LV_DX(x) (HD2LV_X(x) - HD2LV_X(0))
#define HD2LV_DY(y) (HD2LV_Y(y) - HD2LV_Y(0))

#define BM2HD_DX(x) (BM2HD_X(x) - BM2HD_X(0))
#define BM2HD_DY(y) (BM2HD_Y(y) - BM2HD_Y(0))

#define HD2BM_DX(x) (HD2BM_X(x) - HD2BM_X(0))
#define HD2BM_DY(y) (HD2BM_Y(y) - HD2BM_Y(0))

// offsets in image matrix, in bytes
#define BM2LV(x,y) (BM2LV_Y(y) * vram_lv.pitch + (BM2LV_X(x) << 1))
#define LV2BM(x,y) (LV2BM_Y(y) * BMPPITCH      + LV2BM_X(x) * 1)

#define LV2HD(x,y) (LV2HD_Y(y) * vram_hd.pitch + (LV2HD_X(x) << 1))
#define HD2LV(x,y) (HD2LV_Y(y) * vram_lv.pitch + (HD2LV_X(x) << 1))

#define BM2HD(x,y) (BM2HD_Y(y) * vram_hd.pitch + (BM2HD_X(x) << 1))
#define HD2BM(x,y) (HD2BM_Y(y) * BMPPITCH      + HD2BM_X(x) * 1)

// offset for a single row, in bytes

#define BM2LV_R(y) (BM2LV_Y(y) * vram_lv.pitch)
#define LV2BM_R(y) (LV2BM_Y(y) * BMPPITCH     )

#define LV2HD_R(y) (LV2HD_Y(y) * vram_hd.pitch)
#define HD2LV_R(y) (HD2LV_Y(y) * vram_lv.pitch)

#define BM2HD_R(y) (BM2HD_Y(y) * vram_hd.pitch)
#define HD2BM_R(y) (HD2BM_Y(y) * BMPPITCH     )

//~ extern int bm2hd_r_cache[];
//~ #define BM2HD_Rc(y) bm2hd_r_cache[y - BMP_H_MINUS]

extern int y_times_BMPPITCH_cache[];

#ifdef CONFIG_VXWORKS
#define BM(x,y) (((x) >> 1) * 1 + ((y) >> 1) * BMPPITCH     )
#else
#define BM(x,y) ((x) * 1 + y_times_BMPPITCH_cache[y - BMP_H_MINUS])
#endif

#define LV(x,y) (((x) << 1) + (y) * vram_lv.pitch)
#define HD(x,y) (((x) << 1) + (y) * vram_hd.pitch)
#define BM_R(y) ((y) * BMPPITCH     )
#define LV_R(y) ((y) * vram_lv.pitch)
#define HD_R(y) ((y) * vram_hd.pitch)

// normalized coordinates (0,0 ... 720,480)
#define BM2N_X(x) (((x) - os.x0) * 720 / os.x_ex)
#define BM2N_Y(y) (((y) - os.y0) * 480 / os.y_ex)
#define LV2N_X(x) BM2N_X(LV2BM_X(x))
#define LV2N_Y(y) BM2N_Y(LV2BM_Y(y))
#define HD2N_X(x) BM2N_X(HD2BM_X(x))
#define HD2N_Y(y) BM2N_Y(HD2BM_Y(y))

//~ extern int bm2n_x_cache[];
//~ #define BM2N_Xc(x) bm2n_x_cache[x - BMP_W_MINUS]

#define N2BM_X(xn) ((xn) * os.x_ex / 720 + os.x0)
#define N2BM_Y(yn) ((yn) * os.y_ex / 480 + os.y0)
#define N2LV_X(xn) BM2LV_X(N2BM_X(xn))
#define N2LV_Y(yn) BM2LV_Y(N2BM_Y(yn))
#define N2HD_X(xn) BM2HD_X(N2BM_X(xn))
#define N2HD_Y(yn) BM2HD_Y(N2BM_Y(yn))

#define BM2N(x,y) (BM2N_Y(y) * 720 + LV2N_X(x))
#define LV2N(x,y) (LV2N_Y(y) * 720 + LV2N_X(x))
#define HD2N(x,y) (HD2N_Y(y) * 720 + HD2N_X(x))
#define N2BM(x,y) (N2BM_Y(y) * BMPPITCH      + N2LV_X(x) * 1)
#define N2LV(x,y) (N2LV_Y(y) * vram_lv.pitch + (N2LV_X(x) << 1))
#define N2HD(x,y) (N2HD_Y(y) * vram_hd.pitch + (N2HD_X(x) << 1))

#define BM2N_DX(x) (BM2N_X(x) - BM2N_X(0))
#define BM2N_DY(y) (BM2N_Y(y) - BM2N_Y(0))

#define N2BM_DX(x) (N2BM_X(x) - N2BM_X(0))
#define N2BM_DY(y) (N2BM_Y(y) - N2BM_Y(0))

// normalized coordinates with high resolution (0,0 ... 720*16,480*16)
#define Nh2BMh_X(xn) ((xn) * os.x_ex / 720 + (os.x0 << 4))
#define Nh2BMh_Y(yn) ((yn) * os.y_ex / 480 + (os.y0 << 4))

#define BMh2LVh_X(x) ((((x) * bm2lv.sx) >> 10) + (bm2lv.tx << 4))
#define BMh2LVh_Y(y) ((((y) * bm2lv.sy) >> 10) + (bm2lv.ty << 4))

#define LVh2HD_X(x) ((((x) * lv2hd.sx) >> 14) + lv2hd.tx)
#define LVh2HD_Y(y) ((((y) * lv2hd.sy) >> 14) + lv2hd.ty)

#define BMh2HD_X(x) LVh2HD_X(BMh2LVh_X(x))
#define BMh2HD_Y(y) LVh2HD_Y(BMh2LVh_Y(y))

#define Nh2HD(x,y) (BMh2HD_Y(Nh2BMh_Y(y)) * vram_hd.pitch + (BMh2HD_X(Nh2BMh_X(x)) << 1))

// RAW coordinates

// unit: pixels
#define LV2RAW_X(x) ((((x) * lv2raw.sx) >> 10) + lv2raw.tx)
#define LV2RAW_Y(y) ((((y) * lv2raw.sy) >> 10) + lv2raw.ty)
#define RAW2LV_X(x) (((x) << 10) / lv2raw.sx - (lv2raw.tx << 10) / lv2raw.sx)
#define RAW2LV_Y(y) (((y) << 10) / lv2raw.sy - (lv2raw.ty << 10) / lv2raw.sy)

#define BM2RAW_X(x) LV2RAW_X(BM2LV_X(x))
#define BM2RAW_Y(y) LV2RAW_Y(BM2LV_Y(y))
#define RAW2BM_X(x) LV2BM_X(RAW2LV_X(x))
#define RAW2BM_Y(y) LV2BM_Y(RAW2LV_Y(y))

#define HD2RAW_X(x) LV2RAW_X(HD2LV_X(x))
#define HD2RAW_Y(y) LV2RAW_Y(HD2LV_Y(y))
#define RAW2HD_X(x) LV2HD_X(RAW2LV_X(x))
#define RAW2HD_Y(y) LV2HD_Y(RAW2LV_Y(y))

#define RAW2BM_DX(x) (RAW2BM_X(x) - RAW2BM_X(0))
#define RAW2BM_DY(y) (RAW2BM_Y(y) - RAW2BM_Y(0))
#define BM2RAW_DX(x) (BM2RAW_X(x) - BM2RAW_X(0))
#define BM2RAW_DY(y) (BM2RAW_Y(y) - BM2RAW_Y(0))

#define RAW2LV_DX(x) (RAW2LV_X(x) - RAW2LV_X(0))
#define RAW2LV_DY(y) (RAW2LV_Y(y) - RAW2LV_Y(0))
#define LV2RAW_DX(x) (LV2RAW_X(x) - LV2RAW_X(0))
#define LV2RAW_DY(y) (LV2RAW_Y(y) - LV2RAW_Y(0))

#define RAW2HD_DX(x) (RAW2HD_X(x) - RAW2HD_X(0))
#define RAW2HD_DY(y) (RAW2HD_Y(y) - RAW2HD_Y(0))
#define HD2RAW_DX(x) (HD2RAW_X(x) - HD2RAW_X(0))
#define HD2RAW_DY(y) (HD2RAW_Y(y) - HD2RAW_Y(0))

// unit: bytes
#define BM2RAW(x,y) (BM2RAW_Y(y) * raw_info.pitch  + ((BM2RAW_X(x) * 14) >> 3))
#define RAW2BM(x,y) (RAW2BM_Y(y) * BMPPITCH        + RAW2BM_X(x) * 1)

#define LV2RAW(x,y) (LV2RAW_Y(y) * raw_info.pitch  + ((LV2RAW_X(x) * 14) >> 3))
#define RAW2LV(x,y) (RAW2LV_Y(y) * vram_lv.pitch   + (RAW2LV_X(x) << 1))

#define RAW2HD(x,y) (RAW2HD_Y(y) * vram_hd.pitch   + (RAW2HD_X(x) << 1))
#define HD2RAW(x,y) (HD2RAW_Y(y) * raw_info.pitch  + ((HD2RAW_X(x) * 14) >> 3))

// offset for a single row, in bytes
#define BM2RAW_R(y) (BM2RAW_Y(y) * raw_info.pitch)
#define RAW2BM_R(y) (RAW2BM_Y(y) * BMPPITCH      )

#define LV2RAW_R(y) (LV2RAW_Y(y) * raw_info.pitch)
#define RAW2LV_R(y) (RAW2LV_Y(y) * vram_lv.pitch )

#define RAW2HD_R(y) (RAW2HD_Y(y) * vram_hd.pitch)
#define HD2RAW_R(y) (HD2RAW_Y(y) * raw_info.pitch)

#ifdef CONFIG_4_3_SCREEN
#define SCREENLAYOUT_3_2 100
#define SCREENLAYOUT_4_3 0
#else
#define SCREENLAYOUT_3_2 0
#define SCREENLAYOUT_4_3 100
#endif

#define SCREENLAYOUT_3_2_or_4_3 0

#define SCREENLAYOUT_16_10 1
#define SCREENLAYOUT_16_9 2
#define SCREENLAYOUT_UNDER_3_2 3 // 500D/50D
#define SCREENLAYOUT_UNDER_16_9 4 // HDMI VGA and SD

#endif
