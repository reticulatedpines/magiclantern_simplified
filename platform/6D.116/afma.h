// 6D AFMA constants from 1%
#define PROP_AFMA 0x80010006

static int8_t afma_buf[0x1b];

#define AFMA_MODE          afma_buf[9]
#define AFMA_PER_LENS_WIDE afma_buf[13]
#define AFMA_PER_LENS_TELE afma_buf[14]
#define AFMA_ALL_LENSES    afma_buf[16]

