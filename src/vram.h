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




/** VRAM accessors.
 * \returns -1 if the vram is not enabled.
 */
extern int
vram_get_number(
	uint32_t		number
);

/** Write the VRAM to a BMP file named "A:/test.bmp" */
extern void
dispcheck( void );

extern const char * vram_instance_str_ptr;


/** Retrieve the vram info? */
extern void
vram_image_pos_and_size(
	uint32_t *		x,
	uint32_t *		y,
	uint32_t *		w,
	uint32_t *		h
);


/** VRAM info structure (maybe?) */
struct vram_object
{
	const char *		name; // "Vram Instance" 0xFFCA79E5
	uint32_t		off_0x04;
	uint32_t		initialized; // off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	struct semaphore *	sem; // off 0x14;
};

extern struct vram_object * 
vram_instance( void );

extern int
vram_get_lock(
	struct vram_object *	vram
);

struct bmp_vram_info
{
	uint8_t *		vram0;
	uint32_t		off_0x04;
	uint8_t *		vram2;
	//uint32_t		off_0x0c;
};

extern struct bmp_vram_info bmp_vram_info[];



/** VRAM info in the BSS.
 *
 * Pixels are in an YUV 422 format.
 * This points to the image VRAM, not the bitmap vram
 */
struct vram_info
{
	uint8_t *		vram;		// off 0x00
	int		width;		// maybe off 0x04
	int		pitch;		// maybe off 0x08
	int		height;		// off 0x0c
	int		vram_number;	// off 0x10
};
SIZE_CHECK_STRUCT( vram_info, 0x14 );

extern struct vram_info vram_info[2];


extern void
vram_schedule_callback(
	struct vram_info *	vram,
	int			arg1,
	int			arg2,
	int			width,
	int			height,
	void			(*handler)( void * ),
	void *			arg
);


/** HDMI config.
 * This structure is largely unknown.
 */
struct hdmi_config
{
	uint32_t		off_0x00;
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;

	// 0 == 720x480, 1 = 704x480, 2== 704x576, 3==1920x1080?
	// according to ImgDDev_select_parameter up to 0xA?
	uint32_t		disp_type; // off_0x28;
	uint32_t		off_0x2c;

	// 0 == 720x480, 1 = 704x480, 2== 704x576, 3==1920x1080
	uint32_t		hdmi_mode; // off_0x30;
	uint32_t		off_0x34;
	thunk			img_request_notify_blank; // off_0x38;
	thunk			bmp_request_notify_blank; // off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;

	// 0xc0f14070 == video enabled?
	uint32_t		image_vbuf_playback_enabled; // off_0x4c;

	uint32_t		off_0x50;
	uint32_t		off_0x54;
	uint32_t		off_0x58;
	uint32_t		off_0x5c;
	uint32_t		off_0x60;
	uint32_t		off_0x64;
	uint32_t		off_0x68;
	uint32_t		off_0x6c;
	uint32_t		off_0x70;
	uint32_t		off_0x74;
	uint32_t		off_0x78;
	uint32_t		off_0x7c;
	uint32_t		off_0x80;
	uint32_t		off_0x84;
	uint32_t		off_0x88;
	uint32_t		off_0x8c;
	uint32_t		off_0x90;
	uint32_t		off_0x94;
	uint32_t		off_0x98;
	uint32_t		off_0x9c;
	uint32_t		off_0xa0;
	uint32_t		off_0xa4;
	uint32_t		off_0xa8;
	uint32_t		off_0xac;
	uint32_t		off_0xb0;
	struct semaphore *	sem;	// off_0xb4;
	struct semaphore *	bmpddev_sem; // off_0xb8;
	struct semaphore *	imb_cbr_semaphore; // off_0xbc;
	uint32_t		off_0xc0;
	uint32_t		off_0xc4;
	uint32_t		off_0xc8;
	uint32_t		off_0xcc;
	uint32_t		off_0xd0;
	uint32_t		off_0xd4;
	uint32_t		off_0xd8;
	uint32_t		off_0xdc;
	uint32_t		off_0xe0;
	uint32_t		off_0xe4;
	uint32_t		off_0xe8;
	uint32_t		off_0xec;
	uint32_t		off_0xf0;
	uint32_t		off_0xf4;
	uint32_t		off_0xf8;
	uint32_t		off_0xfc;
	uint32_t		off_0x100;
	uint32_t		off_0x104;
	uint32_t		off_0x108;
	uint32_t		off_0x10c;
	uint32_t		off_0x110;
	uint32_t		off_0x114;
	uint32_t		off_0x118;
	uint32_t		off_0x11c;
	uint32_t		off_0x120;
	uint32_t		off_0x124;
	struct bmp_vram_info *	bmp_info;	// off_0x128;
	uint32_t		off_0x12c;
	uint32_t		off_0x130;
	uint32_t		off_0x134;
	uint32_t		off_0x138;
	uint32_t		off_0x13c;
	uint32_t		off_0x140;
	uint32_t		off_0x144;
	void *			off_0x148;
	uint32_t		off_0x14c;
	void *			off_0x150;
	uint32_t		off_0x154;
	uint32_t		off_0x158;
	uint32_t		off_0x15c;
	uint32_t		off_0x160;
	uint32_t		off_0x164;
	uint32_t		off_0x168;
	uint32_t		off_0x16c;
	uint32_t		off_0x170;
	uint32_t		off_0x174;
	uint32_t		off_0x178;
	uint32_t		off_0x17c;
};

extern struct hdmi_config hdmi_config;

#define CACHEABLE(x)   ((void*)(((uint32_t)(x)) |  0x40000000))
#define UNCACHEABLE(x) ((void*)(((uint32_t)(x)) & ~0x40000000))

void redraw();

struct vram_info * get_yuv422_vram();
struct vram_info * get_yuv422_hd_vram();


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
extern struct vram_info vram_hd;
extern struct vram_info vram_lv;
extern struct vram_info vram_bm;


 // offsets on one axis, in pixels
#define BM2LV_X(x) ((x) * bm2lv.sx / 1024 + bm2lv.tx)
#define BM2LV_Y(y) ((y) * bm2lv.sy / 1024 + bm2lv.ty)

#define LV2BM_X(x) ((x) * 1024 / bm2lv.sx - bm2lv.tx * 1024 / bm2lv.sx)
#define LV2BM_Y(y) ((y) * 1024 / bm2lv.sy - bm2lv.ty * 1024 / bm2lv.sy)

#define LV2HD_X(x) ((x) * lv2hd.sx / 1024 + lv2hd.tx)
#define LV2HD_Y(y) ((y) * lv2hd.sy / 1024 + lv2hd.ty)

#define HD2LV_X(x) ((x) * 1024 / lv2hd.sx - lv2hd.tx * 1024 / lv2hd.sx)
#define HD2LV_Y(y) ((y) * 1024 / lv2hd.sy - lv2hd.ty * 1024 / lv2hd.sy)

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

#define HD2BM_DX(x) (HD2BM_Y(x) - HD2BM_Y(0))
#define HD2BM_DY(y) (HD2BM_Y(x) - HD2BM_Y(0))

// offsets in image matrix, in bytes
#define BM2LV(x,y) (BM2LV_Y(y) * vram_lv.pitch + BM2LV_X(x))
#define LV2BM(x,y) (LV2BM_Y(y) * vram_bm.pitch + LV2BM_X(x))

#define LV2HD(x,y) (LV2HD_Y(y) * vram_hd.pitch + LV2HD_X(x))
#define HD2LV(x,y) (HD2LV_Y(y) * vram_lv.pitch + HD2LV_X(x))

#define BM2HD(x,y) (BM2HD_Y(y) * vram_hd.pitch + BM2HD_X(x))
#define HD2BM(x,y) (HD2BM_Y(y) * vram_bm.pitch + HD2BM_X(x))

#define BM(x,y) (x + y * vram_bm.pitch)
#define LV(x,y) (x + y * vram_lv.pitch)
#define HD(x,y) (x + y * vram_lv.pitch)


#endif
