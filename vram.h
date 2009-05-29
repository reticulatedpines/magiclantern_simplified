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
	uint32_t		off_0x0c;
};

extern struct bmp_vram_info bmp_vram_info;



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


#endif
