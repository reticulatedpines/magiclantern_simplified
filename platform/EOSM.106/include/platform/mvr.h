#ifndef __PLATFORM_MVR_H__
#define __PLATFORM_MVR_H__

// Movie recording.

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;

// tab size: 4

struct mvr_config
{
	int16_t         qscale_related_1;		// 0x00, 67bc
	uint16_t        debug_flag;             // 0x02, 67be   1 = write debugmsg's
	uint16_t        qscale_mode;            // 0x04, 67c0   1 = QScale, 0 = CBR
	uint16_t		db_filter_a;			// 0x06, 67c2
	uint16_t		db_filter_b;            // 0x08, 67c4
	int16_t 		def_q_scale;            // 0x0a, 67c6
	int16_t 		qscale_related_2;		// 0x0c, 67c8
	int16_t 		qscale_related_3;		// 0x0e, 67ca
	uint16_t 		x67cc;                  // 0x10, 67cc
	int16_t 		qscale_limit_L;			// 0x12, 67ce
	int16_t         qscale_limit_H;         // 0x14, 67d0
	uint16_t		time_const;				// 0x16, 67d2
	uint32_t		fullhd_30fps_opt_size_I;// 0x18, 67d4
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
	uint32_t		x6838;                  // 0x7c, 6838
	uint32_t		x683c;                  // 0x80, 683c
	uint32_t        x6840;                  // 0x84, 6840
	uint32_t		x6844;                  // 0x88, 6844
	uint32_t		x6848;					// 0x8c, 6848
    int32_t         another_def_q_scale;	// 0x90, 684c
	int32_t         IniQScale;              // 0x94, 6850
	uint32_t		x6854;                  // 0x98, 6854
	uint32_t		x6858;                  // 0x9c, 6858
	uint32_t		x685c;					// 0xa0, 685c
	uint32_t        vga_30fps_opt_size_I;	// 0xa4, 6860
	uint32_t 	    vga_30fps_opt_size_P;	// 0xa8, 6864
	uint32_t 		vga_30fps_D1;           // 0xac, 6868
	uint32_t		vga_30fps_D2;			// 0xb0, 686c
	uint32_t		x6870;                  // 0xb4, 6870
	uint32_t	    vga_25fps_opt_size_I;	// 0xb8, 6874
	uint32_t        vga_25fps_opt_size_P;	// 0xbc, 6878
	uint32_t		vga_25fps_D1;			// 0xc0, 687c
	uint32_t		vga_25fps_D2;           // 0xc4, 6880
	uint32_t		x6884;                  // 0xc8, 6884
	uint32_t		x6888;                  // 0xcc, 6888
	uint32_t		x688c;                  // 0xd0, 688c
	uint32_t		x6890;                  // 0xd4, 6890
	uint32_t		x6894;                  // 0xd8, 6894
	uint32_t		x6898;                  // 0xdc, 6898
	uint32_t		x689c;                  // 0xe0, 689c
	uint32_t		x68a0;                  // 0xe4, 68a0
	uint32_t		x68a4;                  // 0xe8, 68a4
	uint32_t		x68a8;                  // 0xec, 68a8
	uint32_t		x68ac;                  // 0xf0, 68ac
	uint32_t		actual_qscale_maybe3;   // 0xf4, 68b0
	uint32_t		actual_qscale_maybe2;   // 0xf8, 68b4
	uint32_t		actual_qscale_maybe;    // 0xfc, 68b8
	uint32_t		IOptSize;               // 0x100, 68bc
	uint32_t		POptSize;               // 0x104, 68c0
	uint32_t		IOptSize2;              // 0x108, 68c4
	uint32_t		POptSize2;              // 0x10c, 68c8
	uint32_t		GopSize;                // 0x110, 68cc
	uint32_t		x68d0;                  // 0x114, 68d0
	uint32_t		x68d4;                  // 0x118, 68d4
	uint32_t		_D1;                    // 0x11c, 68d8
	uint32_t		_D2;                    // 0x120, 68dc
	uint32_t		x68e0;                  // 0x124, 68e0
	uint32_t		fullhd_30fps_gop_opt_0;	// 0x128, 68e4
	uint32_t		fullhd_30fps_gop_opt_1;	// 0x12c, 68e8
	uint32_t		fullhd_30fps_gop_opt_2;	// 0x130, 68ec
	uint32_t		fullhd_30fps_gop_opt_3;	// 0x134, 68f0
	uint32_t		fullhd_30fps_gop_opt_4;	// 0x138, 68f4
	uint32_t		fullhd_25fps_gop_opt_0;	// 0x13c, 68f8
	uint32_t		fullhd_25fps_gop_opt_1;	// 0x140, 68fc
	uint32_t		fullhd_25fps_gop_opt_2;	// 0x144, 6900
	uint32_t		fullhd_25fps_gop_opt_3;	// 0x148, 6904
	uint32_t		fullhd_25fps_gop_opt_4;	// 0x14c, 6908
	uint32_t		fullhd_24fps_gop_opt_0;	// 0x150, 690c
	uint32_t		fullhd_24fps_gop_opt_1;	// 0x154, 6910
	uint32_t		fullhd_24fps_gop_opt_2;	// 0x158, 6914
	uint32_t		fullhd_24fps_gop_opt_3;	// 0x15c, 6918
	uint32_t		fullhd_24fps_gop_opt_4;	// 0x160, 691c
    uint32_t		hd_60fps_gop_opt_0;     // 0x164, 6920
    uint32_t		hd_60fps_gop_opt_1;     // 0x168, 6924
    uint32_t		hd_60fps_gop_opt_2;     // 0x16c, 6928
    uint32_t		hd_60fps_gop_opt_3;     // 0x170, 692c
    uint32_t		hd_60fps_gop_opt_4;     // 0x174, 6930
    uint32_t		hd_50fps_gop_opt_0;     // 0x178, 6934
    uint32_t		hd_50fps_gop_opt_1;     // 0x17c, 6938
    uint32_t		hd_50fps_gop_opt_2;     // 0x180, 693c
    uint32_t		hd_50fps_gop_opt_3;     // 0x184, 6940
    uint32_t		hd_50fps_gop_opt_4;     // 0x188, 6944
    uint32_t		x6948;                  // 0x18c, 6948
    uint32_t		x694c;                  // 0x190, 694c
    uint32_t		x6950;                  // 0x194, 6950
    uint32_t		x6954;                  // 0x198, 6954
    uint32_t		x6958;                  // 0x19c, 6958
    uint32_t		x695c;                  // 0x1a0, 695c
    uint32_t		x6960;                  // 0x1a4, 6960
    uint32_t		x6964;                  // 0x1a8, 6964
    uint32_t		x6968;                  // 0x1ac, 6968
    uint32_t		x696c;                  // 0x1b0, 696c
    uint32_t		vga_30fps_gop_opt_0;    // 0x1b4, 6970
    uint32_t		vga_30fps_gop_opt_1;    // 0x1b8, 6974
    uint32_t		vga_30fps_gop_opt_2;    // 0x1bc, 6978
    uint32_t		vga_30fps_gop_opt_3;    // 0x1c0, 697c
    uint32_t		vga_30fps_gop_opt_4;    // 0x1c4, 6980
    uint32_t		vga_25fps_gop_opt_0;    // 0x1c8, 6984
    uint32_t		vga_25fps_gop_opt_1;    // 0x1cc, 6988
    uint32_t		vga_25fps_gop_opt_2;    // 0x1d0, 698c
    uint32_t		vga_25fps_gop_opt_3;    // 0x1d4, 6990
    uint32_t		vga_25fps_gop_opt_4;    // 0x1d8, 6994
} __attribute__((aligned,packed));

//~ SIZE_CHECK_STRUCT( mvr_config, 0x30 );

extern struct mvr_config mvr_config;

#endif /* __PLATFORM_MVR_H__ */
