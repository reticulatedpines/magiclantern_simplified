#ifndef _lens_h_
#define _lens_h_

struct lens_info
{
	char 			name[ 32 ];
	int			focal_len;
	int			focus_dist;
	unsigned		aperture;
	unsigned		shutter;
	unsigned		iso;
	void *			token;
};

extern struct lens_info lens_info;


struct prop_lv_lens
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
	uint16_t		focal_len;	// off_0x2c;
	uint16_t		focus_dist;	// off_0x2e;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint16_t		off_0x38;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 58 );

#endif
