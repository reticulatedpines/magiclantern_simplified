// 5D3 AFMA constants

#define PROP_AFMA 0x80040027

static int8_t afma_buf[0x10];
#define AFMA_MODE           afma_buf[0x0]
#define AFMA_PER_LENS_WIDE  afma_buf[0x2]
#define AFMA_PER_LENS_TELE  afma_buf[0x3]
#define AFMA_ALL_LENSES     afma_buf[0x5]
