#ifndef __PLATFORM_MVR_H__
#define __PLATFORM_MVR_H__

// Movie recording.
// tab size: 4

struct mvr_config
{
    uint16_t        debug_flag;             // 0x00,
    uint16_t        qscale_mode;            // 0x02,
    uint16_t        db_filter_a;            // 0x04,
    uint16_t        db_filter_b;            // 0x06,
    int16_t         def_q_scale;            // 0x08,
    int16_t         qscale_limit;           // 0x0a,  // not Qscale
    int32_t         time_const;             // 0x0c,
    uint32_t        fullhd_30fps_opt_size_I;// 0x10, // ok
    uint32_t        fullhd_30fps_opt_size_P;// 0x14, // ok
    uint32_t        vga_30fps_opt_size_I;   // 0x18, // ok
    uint32_t        vga_30fps_opt_size_P;   // 0x1c, // ok
    uint32_t        unkCompressionP1;       // 0x20, // 
    uint32_t        unkCompressionP2;       // 0x24, // 
    uint32_t        unkCompressionP3;       // 0x28, // 
    int16_t         actual_qscale_maybe;    // 0x30, // Here it is!

} __attribute__((aligned,packed));

SIZE_CHECK_STRUCT( mvr_config, 0x30 );

#endif /* __PLATFORM_MVR_H__ */
