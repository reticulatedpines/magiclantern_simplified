
// Movie recording.

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;

// tab size: 4

struct mvr_config
{
	uint16_t                debug_flag;                     // 0x00, 85f4, 1 = write debugmsg's
	uint16_t                qscale_mode;                    // 0x02, 85f6, 1 = QScale, 0 = CBR
	uint16_t                db_filter_a;                    // 0x04, 85f8, no effect
	uint16_t                db_filter_b;                    // 0x06, 85fa, no effect
	int16_t                 def_q_scale;                    // 0x08, 85fc, works when qscale_mode = 1
	int16_t                 qscale_limit_L;                 // 0x0a, 85fe
	int16_t                 qscale_limit_H;                 // 0x0c, 8600
	uint16_t                time_const;                     // 0x0e, 8602, unknown
	uint32_t                fullhd_20fps_opt_size_I;		// 0x10, 8604, works when qscale_mode = 0
	uint32_t                fullhd_20fps_opt_size_P;		// 0x14, 8608
	uint32_t                fullhd_20fps_D1;                // 0x18, 860c
	uint32_t                fullhd_20fps_D2;                // 0x1c, 8610
	uint32_t                x8610_unknown;                  // 0x20, 8614
	uint32_t                hd_30fps_opt_size_I;			// 0x24, 8618
	uint32_t                hd_30fps_opt_size_P;			// 0x28, 861c
	uint32_t                hd_30fps_D1;                    // 0x2c, 8620
	uint32_t                hd_30fps_D2;                    // 0x30, 8624
	uint32_t                x8624_unknown;                  // 0x34, 8628
	uint32_t                vga_30fps_opt_size_I;			// 0x38, 862c
	uint32_t                vga_30fps_opt_size_P;			// 0x3c, 8630
	uint32_t                vga_30fps_D1;                   // 0x40, 8634
	uint32_t                vga_30fps_D2;                   // 0x44, 8638
	uint32_t                x8638_unknown;                  // 0x48, 863c
	int32_t                 IniQScale;                      // 0x4c, 8640
	int32_t                 actual_qscale_maybe;			// 0x50, 8644
	uint32_t                IOptSize;                       // 0x54, 8648
	uint32_t                POptSize;                       // 0x58, 864c
	uint32_t                IOptSize2;                      // 0x5c, 8650
	uint32_t                POptSize2;						// 0x60, 8654
	uint32_t                GopSize;						// 0x64, 8658
	uint32_t                B_zone_NowIndex;                // 0x68, 865c
	uint32_t                gop_opt_array_ptr;              // 0x6c, 8660
	uint32_t                _D1;                            // 0x70, 8664
	uint32_t                _D2;                            // 0x74, 8668
	uint32_t                x8668_unknown;                  // 0x78, 866c -- 1080p = 0x9, 720p/480p = 0xE
	uint32_t                fullhd_20fps_gop_opt_0;			// 0x7c, 8670
	uint32_t                fullhd_20fps_gop_opt_1;			// 0x80, 8674
	uint32_t                fullhd_20fps_gop_opt_2;			// 0x84, 8678
	uint32_t                hd_30fps_gop_opt_0;             // 0x88, 867c
	uint32_t                hd_30fps_gop_opt_1;             // 0x8c, 8680
	uint32_t                hd_30fps_gop_opt_2;             // 0x90, 8684
	uint32_t                vga_30fps_gop_opt_0;			// 0x94, 8688
	uint32_t                vga_30fps_gop_opt_1;			// 0x98, 868c
	uint32_t                vga_30fps_gop_opt_2;			// 0x9c, 8690
} __attribute__((aligned,packed));

//~ SIZE_CHECK_STRUCT( mvr_config, 0x30 );

extern struct mvr_config mvr_config;


// This is from 5D2, not used here.
/*
 *
 * State information is in this structure.  A pointer to the global
 * object is at 0x1ee0.  It is of size 0x1b4.
 *
 * The state object is in 0x68a4.
 */
/*struct mvr_struct
{
	const char *		type;	 // "MovieRecorder" off 0
	uint32_t		off_0x04;
	uint32_t		task;	// off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	uint32_t		off_0x4c;
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
	uint32_t		off_0xb4;
	uint32_t		off_0xb8;
	uint32_t		off_0xbc;
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
	uint32_t		off_0x128;
	uint32_t		off_0x12c;
	uint32_t		off_0x130;
	uint32_t		off_0x134;
	uint32_t		off_0x138;
	uint32_t		off_0x13c;
	uint32_t		is_vga;	// 0==1920, 1==640 off_0x140;
	uint32_t		off_0x144;
	uint32_t		off_0x148;
	uint32_t		fps;		// 30, off_0x14c;
	uint32_t		width;		// off_0x150;
	uint32_t		height;		// off_0x154;
	uint32_t		audio_rec;	// off_0x158;
	uint32_t		auido_channels;	// 2 or 0, off_0x15c;
	uint32_t		audio_rate;	// 44100 or 0, off_0x160;
	uint32_t		off_0x164;
	uint32_t		off_0x168;
	uint32_t		off_0x16c;
	uint32_t		off_0x170;
	uint32_t		off_0x174;
	uint32_t		off_0x178;
	uint32_t		off_0x17c;
	uint32_t		off_0x180;
	uint32_t		off_0x184;
	uint32_t		off_0x188;
	uint32_t		off_0x18c;
	uint32_t		bit_rate; // off_0x190;
	uint32_t		off_0x194;
	uint32_t		off_0x198;
	uint32_t		off_0x19c;
	uint32_t		off_0x1a0;
	uint32_t		off_0x1a4;
	uint32_t		off_0x1a8;
	uint32_t		off_0x1ac;
	uint32_t		off_0x1b0;
	uint32_t		off_0x1b4;
	uint32_t		off_0x1b8;
	uint32_t		off_0x1bc;
	uint32_t		off_0x1c0;
	uint32_t		off_0x1c4;
	uint32_t		off_0x1c8;
	uint32_t		off_0x1cc;
	uint32_t		off_0x1d0;
	uint32_t		off_0x1d4;
	uint32_t		off_0x1d8;
	uint32_t		off_0x1dc;
	uint32_t		off_0x1e0;
	uint32_t		off_0x1e4;
	uint32_t		off_0x1e8;
	uint32_t		off_0x1ec;
	uint32_t		off_0x1f0;
	uint32_t		off_0x1f4;
	uint32_t		off_0x1f8;
	uint32_t		off_0x1fc;
};

SIZE_CHECK_STRUCT( mvr_struct, 512 );*/
