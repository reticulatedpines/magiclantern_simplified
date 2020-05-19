// 7D AFMA constants
#define PROP_AFMA 0x80010006

static int8_t afma_buf[0x17];
#define AFMA_MODE       afma_buf[0x05]
#define AFMA_PER_LENS   afma_buf[0x11]
#define AFMA_ALL_LENSES afma_buf[0x13]
