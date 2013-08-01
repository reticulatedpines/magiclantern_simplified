#ifndef __PLATFORM_MVR_H__
#define __PLATFORM_MVR_H__

// Movie recording.

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;

// tab size: 4

struct mvr_config
{
	uint16_t		debug_flag;				// 0x00,
	uint16_t		qscale_mode;			// 0x02,
	uint16_t		db_filter_a;			// 0x04,
	uint16_t		db_filter_b;			// 0x06,
	int16_t			def_q_scale;			// 0x08,
	int16_t 		actual_qscale_maybe;	// 0x0a,  // random, just to compile
	int16_t 		qscale_related_2;		// 0x0c,
	int16_t 		qscale_related_3;		// 0x0e,
	uint32_t 		fullhd_30fps_opt_size_I;// 0x10, // ok
	uint32_t		fullhd_30fps_opt_size_P;// 0x14, // ok
	uint32_t		vga_30fps_opt_size_I;   // 0x18, // ok
	uint32_t	    vga_30fps_opt_size_P;   // 0x1c, // ok
	uint32_t 		fullhd_30fps_gop_opt_0; // 0x10, // ng,  just to compile
} __attribute__((aligned,packed));

//~ SIZE_CHECK_STRUCT( mvr_config, 0x30 );

extern struct mvr_config mvr_config;

#endif /* __PLATFORM_MVR_H__ */
