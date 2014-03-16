#include <internals.h>
#include <fw-signature.h>

#if defined(CONFIG_60D)
#define CAMERA_SIGNATURE (SIG_60D_111)
#elif defined(CONFIG_550D)
#define CAMERA_SIGNATURE (SIG_550D_109)
#elif defined(CONFIG_600D)
#define CAMERA_SIGNATURE (SIG_600D_102)
#elif defined(CONFIG_50D)
#define CAMERA_SIGNATURE (SIG_50D_109)
#elif defined(CONFIG_500D)
#define CAMERA_SIGNATURE (SIG_500D_111)
#elif defined(CONFIG_5D2)
#define CAMERA_SIGNATURE (SIG_5D2_212)
#elif defined(CONFIG_1100D)
#define CAMERA_SIGNATURE (SIG_1100D_105)
#elif defined(CONFIG_6D)
#define CAMERA_SIGNATURE (SIG_6D_113)
#elif defined(CONFIG_5D3)
#define CAMERA_SIGNATURE (SIG_5D3_113)
#elif defined(CONFIG_EOSM)
#define CAMERA_SIGNATURE (SIG_EOSM_202)
#elif defined(CONFIG_7D)
#define CAMERA_SIGNATURE (SIG_7D_203)
#elif defined(CONFIG_650D)
#define CAMERA_SIGNATURE (SIG_650D_104)
#elif defined(CONFIG_700D)
#define CAMERA_SIGNATURE (SIG_700D_113)
#else
#error Unsupported platform, please edit fw-signature.c accordingly.
#endif

int compute_signature(int* start, int len)
{
    int c = 0;
    int* p;
    for (p = start; p < start + len; p++)
    {
        c += *p;
    }
    return c;
}


int check_signature()
{
    int sig = compute_signature((int*)SIG_START, SIG_LEN);
    if(sig == CAMERA_SIGNATURE)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
