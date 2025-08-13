// 70D AFMA constants
#define PROP_AFMA 0x80010006

static int8_t afma_buf[0x22];

#define AFMA_MODE          afma_buf[0xD]    // nikfreak ok
#define AFMA_PER_LENS_WIDE afma_buf[20]     // nikfreak ok
#define AFMA_PER_LENS_TELE afma_buf[21]     // nikfreak ok
#define AFMA_ALL_LENSES    afma_buf[23]     // nikfreak ok