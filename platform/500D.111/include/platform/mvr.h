#ifndef __PLATFORM_MVR_H__
#define __PLATFORM_MVR_H__

// Movie recording.
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

#endif /* __PLATFORM_MVR_H__ */
