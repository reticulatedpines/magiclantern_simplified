#ifndef FW_SIGNATURE_H
#define FW_SIGNATURE_H

#define SIG_LEN 0x10000

#if defined(CONFIG_5D3) || defined(CONFIG_EOSM) || defined(CONFIG_6D)
#define SIG_START 0xFF0C0000
#elif defined(CONFIG_7D) || defined(CONFIG_7D_MASTER)
#define SIG_START 0xF8010000
#else
#define SIG_START 0xFF010000
#endif

#define SIG_60D_111  0xaf91b602 // from FF010000
#define SIG_550D_109 0x851320e6 // from FF010000
#define SIG_600D_102 0x27fc03de // from FF010000
#define SIG_600D_101 0x290106d8 // from FF010000 // firmwares are identical
#define SIG_500D_110 0x4c0e5a7e // from FF010000
#define SIG_50D_109  0x4673ef59 // from FF010000
#define SIG_500D_111 0x44f49aef // from FF010000
#define SIG_5D2_212  0xae78b938 // from FF010000
#define SIG_1100D_105 0x46de7624 // from FF010000
// Not supported cameras
#define SIG_6D_112   0x6D677512
#define SIG_6D_113   0x6B6A9C6F
#define SIG_5D3_113  0x2e2f65f5
#define SIG_EOSM_106 0x6393A881
#define SIG_7D_203   0x50163E93
#define SIG_7D_MASTER_203 0x640BF4D1
#define SIG_650D_101 0x83e04919

#define SHOULD_CHECK_SIG ((!defined(CONFIG_QEMU)) && (defined(CONFIG_5D3) || defined(CONFIG_7D) || defined(CONFIG_7D_MASTER) || defined(CONFIG_EOSM) || defined(CONFIG_650D) || defined(CONFIG_6D)))

static int compute_signature(int* start, int num)
{
    int c = 0;
    int* p;
    for (p = start; p < start + num; p++)
    {
        c += *p;
    }
    return c;
}

#endif //FW_SIGNATURE_H
