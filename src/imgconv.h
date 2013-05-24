#ifndef _imgconv_h_
#define _imgconv_h_

#define UYVY_GET_AVG_Y(uyvy) (((((uyvy) >> 24) & 0xFF) + (((uyvy) >> 8) & 0xFF)) >> 1)
#define UYVY_GET_U(uyvy) (((uyvy)       ) & 0xFF)
#define UYVY_GET_V(uyvy) (((uyvy) >>  16) & 0xFF)
#define COMPUTE_UYVY2YRGB(uyvy, Y, R, G, B) \
{ \
    Y = UYVY_GET_AVG_Y(uyvy); \
    const int gv = UYVY_GET_V(uyvy); \
    const int gu = UYVY_GET_U(uyvy); \
    int v = Y + yuv2rgb_RV[gv]; \
    R = COERCE(v, 0, 255); \
    v = Y + yuv2rgb_GU[gu] + yuv2rgb_GV[gv]; \
    G = COERCE(v, 0, 255); \
    v = Y + yuv2rgb_BU[gu]; \
    B = COERCE(v, 0, 255); \
} \

#define UYVY_PACK(u,y1,v,y2) ((u) & 0xFF) | (((y1) & 0xFF) << 8) | (((v) & 0xFF) << 16) | (((y2) & 0xFF) << 24);

extern int yuv2rgb_RV[256];
extern int yuv2rgb_GU[256];
extern int yuv2rgb_GV[256];
extern int yuv2rgb_BU[256];

/** http://www.martinreddy.net/gfx/faqs/colorconv.faq
 * BT 601:
 * R'= Y' + 0.000*U' + 1.403*V'
 * G'= Y' - 0.344*U' - 0.714*V'
 * B'= Y' + 1.773*U' + 0.000*V'
 * 
 * BT 709:
 * R'= Y' + 0.0000*Cb + 1.5701*Cr
 * G'= Y' - 0.1870*Cb - 0.4664*Cr
 * B'= Y' - 1.8556*Cb + 0.0000*Cr
 */

void precompute_yuv2rgb();

void yuv2rgb(int Y, int U, int V, int* R, int* G, int* B);

void uyvy_split(uint32_t uyvy, int* Y, int* U, int* V);

void little_cleanup(void* BP, void* MP);

void yuv_resize(uint32_t* src, int src_w, int src_h, uint32_t* dst, int dst_w, int dst_h);

void yuv_halfcopy(uint32_t* dst, uint32_t* src, int w, int h, int top_half);

void bmp_zoom(uint8_t* dst, uint8_t* src, int x0, int y0, int denx, int deny);

void yuvcpy_main(uint32_t* dst, uint32_t* src, int num_pix, int X, int lut);

int yuv411_to_422(uint32_t addr);

void yuv411_to_rgb(uint32_t addr, int* Y, int* R, int* G, int* B);

#endif /* _imgconv_h_ */