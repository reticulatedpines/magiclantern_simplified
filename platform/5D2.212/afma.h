// 5D2 AFMA constants

#define PROP_AFMA 0x80010006

static int8_t afma_buf[0xF];
#define AFMA_MODE       afma_buf[0x8]
#define AFMA_PER_LENS   afma_buf[0xC]
#define AFMA_ALL_LENSES afma_buf[0xE]
