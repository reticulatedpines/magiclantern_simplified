#ifndef _video_h_
#define _video_h_

#include "arm-mcr.h"


/** \file
 * Interface to the 5D Mark II's video ram (VRAM).
 */


/** VRAM accessors */
extern uint32_t
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
	uint32_t		off_0x08;
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
 * Pixels are in an unknown format.
 * This points to the image VRAM, not the bitmap vram
 */
struct vram_info
{
	uint16_t *		vram;		// off 0x00
	uint32_t		width;		// maybe off 0x04
	uint32_t		pitch;		// maybe off 0x08
	uint32_t		height;		// off 0x0c
	uint32_t		vram_number;	// off 0x10
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
	uint32_t		off_0x28;
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


#endif
