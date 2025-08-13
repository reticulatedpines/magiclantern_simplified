#ifndef __MVR_H__
#define __MVR_H__

#include <platform/mvr.h>

#if defined(FEATURE_NITRATE) || defined(CONFIG_7D)
extern struct mvr_config mvr_config;
#endif

// Horrible old code uses various defines, MVR_190_STRUCT, MVR_FRAME_NUMBER etc
// to partially define a fake struct in 99D/consts.h
//
// This is the start of work to convert to a real struct.

// eventually we'll want
//# defined(CONFIG_MVR_STRUCT_VX)
// when we know which cams use what.

// FIXME list:
// - rename below mvr_struct_real to mvr_struct, remove above extern mvr_struct
// - ideally, pick a better name than "mvr_struct"
// - confirm platform/EOSM/mvr.h is needed and/or converted, this cam
//   has both mvr.h and include/platform/mvr.h
//

// Some field names can be found via MVR_AppendCheck()

#ifdef CONFIG_70D
struct mvr_struct_real
{
    uint32_t unk[0x192];
};
SIZE_CHECK_STRUCT(mvr_struct_real, 0x658);
#endif

#ifdef CONFIG_200D
struct mvr_struct_real
{
    char *class_name_h264;
    uint32_t Rec_StateObject;
    uint32_t task_class_MovRecH264;
    uint32_t debugLogType;
    uint32_t        unk_83;
    struct semaphore *MVR_Common_sem;
    struct semaphore *MVR_CommonInfo_sem;
    uint32_t        unk_84;
    uint32_t pbyStreamAddress;
    uint32_t        unk_85;
    uint32_t pbyRecWorkAddress;
    uint32_t        unk_86[0x5];
    uint32_t maxRecSec;
    uint32_t        unk_87[0x6];
    void            *cbr_unk_01;
    void            *cbr_unk_02;
    void            *cbr_unk_03;
    void            *cbr_unk_04;
    void            *cbr_unk_05;
    uint32_t        unk_88[0x1a];
    uint32_t RecRly_StateObject;
    uint32_t task_class_MovieRecorderRLY;
    uint32_t        unk_89[0x86];
    void            *cbr_unk_07;
    uint32_t        unk_90[0x17];
    void            *cbr_unk_06;
    uint32_t        unk_91[0xf];
    uint32_t        p_0x808_block;
    uint32_t        unk_92[0x3];
    uint32_t mvrResult_cbr_out;
    uint32_t        unk_93[0x68];
    uint32_t RecSettings_06;
    uint32_t RecSettings_07;
    uint32_t AppendFilePath_unk_02;
    uint32_t AppendFilePath;
    uint32_t        unk_94[0x3f];
    uint32_t RecSettings_01;
    uint32_t RecSettings_02;
    uint32_t RecSettings_03;
    uint32_t RecSettings_dwFrameType_fps;
    uint32_t        unk_95[0x2];
    uint32_t RecSettings_FiledTimeRatio_dividend;
    uint32_t RecSettings_08;
    uint32_t        unk_96[0x13];
    uint32_t AppendFilePath_unk_01;
    uint32_t        unk_97[0xd];
    uint32_t aac_bitrate;
    uint32_t        unk_98[0x3];
    uint32_t RecSettings_09;
    uint32_t        unk_99[0xa1d];
} __attribute__((aligned,packed));
SIZE_CHECK_STRUCT(mvr_struct_real, 0x2f88); // allocated in MVR_Initialize
#endif


#endif /* __PLATFORM_MVR_H__ */
