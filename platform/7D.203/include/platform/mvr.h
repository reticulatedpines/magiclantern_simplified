#ifndef __PLATFORM_MVR_H__
#define __PLATFORM_MVR_H__

// Movie recording.
// tab size: 4

struct mvr_config
{
	uint16_t		debug_flag;				// 0x00, 67bc, 1 = write debugmsg's
	uint16_t		qscale_mode;			// 0x02, 67be, 1 = QScale, 0 = CBR
	uint16_t		db_filter_a;			// 0x04, 67c0, no effect
	uint16_t		db_filter_b;			// 0x06, 67c2, no effect
	int16_t			def_q_scale;			// 0x08, 67c4, works when qscale_mode = 1
	int16_t 		qscale_related_1;		// 0x0a, 67c6
	int16_t 		qscale_related_2;		// 0x0c, 67c8
	int16_t 		qscale_related_3;		// 0x0e, 67ca
	int16_t 		qscale_limit_L;			// 0x10, 67cc
	int16_t 		qscale_limit_H;			// 0x12, 67ce
	uint16_t		time_const;				// 0x14, 67d0, unknown
	uint16_t		x67d0;					// 0x16, 67d2
	uint32_t		fullhd_30fps_opt_size_I;// 0x18, 67d4, works when qscale_mode = 0
	uint32_t		fullhd_30fps_opt_size_P;// 0x1c, 67d8
	uint32_t		D1_30fps;				// 0x20, 67dc
	uint32_t		D2_30fps;				// 0x24, 67e0
	uint32_t		x67e4;					// 0x28, 67e4
	uint32_t		fullhd_25fps_opt_size_I;// 0x2c, 67e8
	uint32_t		fullhd_25fps_opt_size_P;// 0x30, 67ec
	uint32_t		fullhd_25fps_D1;		// 0x34, 67f0
	uint32_t		fullhd_25fps_D2;		// 0x38, 67f4
	uint32_t		x67f8;					// 0x3c, 67f8
	uint32_t		fullhd_24fps_opt_size_I;// 0x40, 67fc
	uint32_t		fullhd_24fps_opt_size_P;// 0x44, 6800
	uint32_t		fullhd_24fps_D1;		// 0x48, 6804
	uint32_t		fullhd_24fps_D2;		// 0x4c, 6808
	uint32_t		x680c;					// 0x50, 680c
	uint32_t		hd_60fps_opt_size_I;	// 0x54, 6810
	uint32_t		hd_60fps_opt_size_P;	// 0x58, 6814
	uint32_t		hd_60fps_D1;			// 0x5c, 6818
	uint32_t		hd_60fps_D2;			// 0x60, 681c
	uint32_t		x6820;					// 0x64, 6820
	uint32_t		hd_50fps_opt_size_I;	// 0x68, 6824
	uint32_t		hd_50fps_opt_size_P;	// 0x6c, 6828
	uint32_t		hd_50fps_D1;			// 0x70, 682c
	uint32_t		hd_50fps_D2;			// 0x74, 6830
	uint32_t		x6834_kinda_counter;	// 0x78, 6834
	uint32_t		vga_60fps_opt_size_I;	// 0x7c, 6838
	uint32_t		vga_60fps_opt_size_P;	// 0x80, 683c
	uint32_t		vga_60fps_D1;			// 0x84, 6840
	uint32_t		vga_60fps_D2;			// 0x88, 6844
	uint32_t		x6848;					// 0x8c, 6848
	uint32_t		vga_50fps_opt_size_I;	// 0x90, 684c
	uint32_t		vga_50fps_opt_size_P;	// 0x94, 6850
	uint32_t		vga_50fps_D1;			// 0x98, 6854
	uint32_t		vga_50fps_D2;			// 0x9c, 6858
	uint32_t		x685c;					// 0xa0, 685c
	int32_t 		another_def_q_scale;	// 0xa4, 6860
	int32_t 		IniQScale;				// 0xa8, 6864
	int32_t 		actual_qscale_maybe;	// 0xac, 6868
	uint32_t		IOptSize;				// 0xb0, 686c
	uint32_t		POptSize;				// 0xb4, 6870
	uint32_t		IOptSize2;				// 0xb8, 6874
	uint32_t		POptSize2;				// 0xbc, 6878
	uint32_t		GopSize;				// 0xc0, 687c
	uint32_t		B_zone_NowIndex;		// 0xc4, 6880
	uint32_t		gop_opt_array_ptr;		// 0xc8, 6884
	uint32_t		_D1;					// 0xcc, 6888
	uint32_t		_D2;					// 0xd0, 688c
	uint32_t		x6890_counter_maybe;	// 0xd4, 6890
	uint32_t		fullhd_30fps_gop_opt_0;	// 0xd8, 6894
	uint32_t		fullhd_30fps_gop_opt_1;	// 0xdc, 6898
	uint32_t		fullhd_30fps_gop_opt_2;	// 0xe0, 689c
	uint32_t		fullhd_30fps_gop_opt_3;	// 0xe4, 68a0
	uint32_t		fullhd_30fps_gop_opt_4;	// 0xe8, 68a4
	uint32_t		fullhd_25fps_gop_opt_0;	// 0xec, 68a8
	uint32_t		fullhd_25fps_gop_opt_1;	// 0xf0, 68ac
	uint32_t		fullhd_25fps_gop_opt_2;	// 0xf4, 68b0
	uint32_t		fullhd_25fps_gop_opt_3;	// 0xf8, 68b4
	uint32_t		fullhd_25fps_gop_opt_4;	// 0xfc, 68b8
	uint32_t		fullhd_24fps_gop_opt_0;	// 0x100, 68bc
	uint32_t		fullhd_24fps_gop_opt_1;	// 0x104, 68c0
	uint32_t		fullhd_24fps_gop_opt_2;	// 0x108, 68c4
	uint32_t		fullhd_24fps_gop_opt_3;	// 0x10c, 68c8
	uint32_t		fullhd_24fps_gop_opt_4;	// 0x110, 68cc
	uint32_t		hd_60fps_gop_opt_0;		// 0x114, 68d0
	uint32_t		hd_60fps_gop_opt_1;		// 0x118, 68d4
	uint32_t		hd_60fps_gop_opt_2;		// 0x11c, 68d8
	uint32_t		hd_60fps_gop_opt_3;		// 0x120, 68dc
	uint32_t		hd_60fps_gop_opt_4;		// 0x124, 68e0
	uint32_t		hd_50fps_gop_opt_0;		// 0x128, 68e4
	uint32_t		hd_50fps_gop_opt_1;		// 0x12c, 68e8
	uint32_t		hd_50fps_gop_opt_2;		// 0x130, 68ec
	uint32_t		hd_50fps_gop_opt_3;		// 0x134, 68f0
	uint32_t		hd_50fps_gop_opt_4;		// 0x138, 68f4
	uint32_t		vga_60fps_gop_opt_0;	// 0x13c, 68f8
	uint32_t		vga_60fps_gop_opt_1;	// 0x140, 68fc
	uint32_t		vga_60fps_gop_opt_2;	// 0x144, 6900
	uint32_t		vga_60fps_gop_opt_3;	// 0x148, 6904
	uint32_t		vga_60fps_gop_opt_4;	// 0x14c, 6908
	uint32_t		vga_50fps_gop_opt_0;	// 0x150, 690c
	uint32_t		vga_50fps_gop_opt_1;	// 0x154, 6910
	uint32_t		vga_50fps_gop_opt_2;	// 0x158, 6914
	uint32_t		vga_50fps_gop_opt_3;	// 0x15c, 6918
	uint32_t		vga_50fps_gop_opt_4;	// 0x160, 691c
} __attribute__((aligned,packed));

//~ SIZE_CHECK_STRUCT( mvr_config, 0x30 );

#endif /* __PLATFORM_MVR_H__ */
