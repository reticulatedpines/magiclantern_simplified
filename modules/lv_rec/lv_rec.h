#ifndef _lv_rec_h_
#define _lv_rec_h_

#include "raw.h"

/* file footer data */
typedef struct
{
    unsigned char magic[4];
    unsigned short xRes;
    unsigned short yRes;
    unsigned int frameSize;
    unsigned int frameCount;
    unsigned int frameSkip;
    unsigned int sourceFpsx1000;
    unsigned int reserved3;
    unsigned int reserved4;
    struct raw_info raw_info;
} lv_rec_file_footer_t;

#endif
