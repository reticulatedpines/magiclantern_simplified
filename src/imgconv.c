#include "dryos.h"
#include "imgconv.h"
#include "bmp.h"

// precompute some parts of YUV to RGB computations
int yuv2rgb_RV[256];
int yuv2rgb_GU[256];
int yuv2rgb_GV[256];
int yuv2rgb_BU[256];

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

void precompute_yuv2rgb()
{
#ifdef CONFIG_REC709
    /*
    *R = *Y + 1608 * V / 1024;
    *G = *Y -  191 * U / 1024 - 478 * V / 1024;
    *B = *Y + 1900 * U / 1024;
    */
    for (int u = 0; u < 256; u++)
    {
        int8_t U = u;
        yuv2rgb_GU[u] = (-191 * U) >> 10;
        yuv2rgb_BU[u] = (1900 * U) >> 10;
    }

    for (int v = 0; v < 256; v++)
    {
        int8_t V = v;
        yuv2rgb_RV[v] = (1608 * V) >> 10;
        yuv2rgb_GV[v] = (-478 * V) >> 10;
    }
#else // REC 601
    /*
    *R = *Y + ((1437 * V) >> 10);
    *G = *Y -  ((352 * U) >> 10) - ((731 * V) >> 10);
    *B = *Y + ((1812 * U) >> 10);
    */
    for (int u = 0; u < 256; u++)
    {
        int8_t U = u;
        yuv2rgb_GU[u] = (-352 * U) >> 10;
        yuv2rgb_BU[u] = (1812 * U) >> 10;
    }

    for (int v = 0; v < 256; v++)
    {
        int8_t V = v;
        yuv2rgb_RV[v] = (1437 * V) >> 10;
        yuv2rgb_GV[v] = (-731 * V) >> 10;
    }
#endif
}

/*inline void uyvy2yrgb(uint32_t uyvy, int* Y, int* R, int* G, int* B)
{
    uint32_t y1 = (uyvy >> 24) & 0xFF;
    uint32_t y2 = (uyvy >>  8) & 0xFF;
    *Y = (y1+y2) / 2;
    uint8_t u = (uyvy >>  0) & 0xFF;
    uint8_t v = (uyvy >> 16) & 0xFF;
    *R = MIN(*Y + yuv2rgb_RV[v], 255);
    *G = MIN(*Y + yuv2rgb_GU[u] + yuv2rgb_GV[v], 255);
    *B = MIN(*Y + yuv2rgb_BU[u], 255);
} */

void yuv2rgb(int Y, int U, int V, int* R, int* G, int* B)
{
    const int v_and_ff = V & 0xFF;
    const int u_and_ff = U & 0xFF;
    int v = Y + yuv2rgb_RV[v_and_ff];
    *R = COERCE(v, 0, 255);
    v = Y + yuv2rgb_GU[u_and_ff] + yuv2rgb_GV[v_and_ff];
    *G = COERCE(v, 0, 255);
    v = Y + yuv2rgb_BU[u_and_ff];
    *B = COERCE(v, 0, 255);
}

/**
 * BT.709:
 * Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
 * Cb=-0.1146*R' - 0.3854*G' + 0.5000*B'
 * Cr= 0.5000*R' - 0.4541*G' - 0.0458*B'
 *
 * BT.601:
 * Y'= 0.2990*R' + 0.5870*G' + 0.1140*B'
 * Cb=-0.2990*R' - 0.5870*G' + 0.8860*B'
 * Cr= 0.7010*R' - 0.5870*G' - 0.1140*B'
 * 
 * see:
 *   http://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-5-200204-I!!PDF-E.pdf
 *   http://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
 */
uint32_t rgb2yuv422_rec709(int R, int G, int B)
{
    int Y = COERCE(((217) * R + (732) * G + (73) * B) / 1024, 0, 255);
    int U = COERCE(((-117) * R + (-394) * G + (512) * B) / 1024, -128, 127);
    int V = COERCE(((512) * R + (-465) * G + (-46) * B) / 1024, -128, 127);
    return UYVY_PACK(U,Y,V,Y);
}

uint32_t rgb2yuv422_rec601(int R, int G, int B)
{
    int Y = COERCE(((306) * R + (601) * G + (116) * B) / 1024, 0, 255);
    int U = COERCE(((-172) * R + (-337) * G + (509) * B) / 1024, -128, 127);
    int V = COERCE(((509) * R + (-427) * G + (-82) * B) / 1024, -128, 127);
    return UYVY_PACK(U,Y,V,Y);
}

uint32_t rgb2yuv422(int R, int G, int B)
{
#if defined(CONFIG_REC709)
    return rgb2yuv422_rec709(R, G, B);
#else
    return rgb2yuv422_rec601(R, G, B);
#endif
}

void uyvy_split(uint32_t uyvy, int* Y, int* U, int* V)
{
    *Y = UYVY_GET_AVG_Y(uyvy);
    *U = (int)(int8_t)UYVY_GET_U(uyvy);
    *V = (int)(int8_t)UYVY_GET_V(uyvy);
}

void yuv_resize(uint32_t* src, int src_w, int src_h, uint32_t* dst, int dst_w, int dst_h)
{
    int i,j;
    const int srcw_half = src_w >> 1;
    const int dstw_half = dst_w >> 1;
    for (i = 0; i < dst_h; i++)
    {
        const int src_off_part = (i*src_h/dst_h) * srcw_half;
        const int dst_off_y = i * dstw_half;
        int mult_srcw = 0;
        for (j = 0; j < dstw_half; j++, mult_srcw += src_w)
        {
            dst[dst_off_y + j] = src[src_off_part + mult_srcw/dst_w];
        }
    }
}

void yuv_halfcopy(uint32_t* dst, uint32_t* src, int w, int h, int top_half)
{
    int i,j;
    const int w_half = w >> 1;
    int pos = 0;
    for (i = 0; i < h; i++,pos += w_half)
    {
        for (j = 0; j < w/2; j++)
        {
            int sign = j - i * w_half/h;
            const int offset = pos + j;
            if ((top_half && sign > 0) || (!top_half && sign <= 0))
            {
                dst[offset] = src[offset];
            }
        }
    }
}

int yuv411_to_422(uint32_t addr)
{
    // 4 6  8 A  0 2 
    // uYvY yYuY vYyY
    addr = ALIGN32(addr);
        
    // multiples of 12, offset 0: vYyY u
    // multiples of 12, offset 4: uYvY
    // multiples of 12, offset 8: yYuY v

    uint8_t* p = (uint8_t*) addr;

    switch ((addr/4) % 3)
    {
        case 0:
        {
            unsigned u = p[4];
            unsigned v = p[0];
            unsigned y1 = p[1];
            unsigned y2 = p[2];
            return UYVY_PACK(u,y1,v,y2);
        }
        case 1:
        {
            return MEM(addr);
        }
        case 2:
        {
            unsigned u = p[2];
            unsigned v = p[4];
            unsigned y1 = p[0];
            unsigned y2 = p[1];
            return UYVY_PACK(u,y1,v,y2);
        }
    }

	return 0; // unreachable, shut the compiler warnings
}

void yuv411_to_rgb(uint32_t addr, int* Y, int* R, int* G, int* B)
{
    // 4 6  8 A  0 2 
    // uYvY yYuY vYyY
    addr = addr & ~3; // multiple of 4
        
    // multiples of 12, offset 0: vYyY u
    // multiples of 12, offset 4: uYvY
    // multiples of 12, offset 8: yYuY v

    uint8_t* p = (uint8_t*) addr;
    int y = 0;
    int U = 0;
    int V = 0;
    
    // trick to compute [ am3 = (addr/4) % 3 ] a little bit faster
    static int am3 = 0;
    
    static unsigned int prev_addr = 0;
    if (likely(addr == prev_addr + 4))
    {
        am3 = am3 + 1;
        if (unlikely(am3 == 3)) am3 = 0;
    }
    else if (likely(addr == prev_addr))
    {
    }
    else
    {
        am3 = (addr/4) % 3;
    }
    prev_addr = addr;

    switch (am3)
    {
        case 0:
            U = p[4];
            V = p[0];
            y = p[1];
            break;
        case 1:
            U = p[0];
            V = p[2];
            y = p[1];
            break;
        case 2:
            U = p[2];
            V = p[4];
            y = p[0];
            break;
    }

    *Y = y;
    *R = COERCE(y + yuv2rgb_RV[V], 0, 255);
    *G = COERCE(y + yuv2rgb_GU[U] + yuv2rgb_GV[V], 0, 255);
    *B = COERCE(y + yuv2rgb_BU[U], 0, 255);
}

static void FAST yuvcpy_x2(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = ALIGN32(dst);
    src = ALIGN32(src);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 2)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        *(dst) = chroma | (luma1 << 8) | (luma1 << 24);
        *(dst+1) = chroma | (luma2 << 8) | (luma2 << 24);
    }
}

static void FAST yuvcpy_x3(uint32_t* dst, uint32_t* src, int num_pix)
{
    dst = ALIGN32(dst);
    src = ALIGN32(src);
    uint32_t* last_s = src + (num_pix>>1);
    for (; src < last_s; src++, dst += 3)
    {
        uint32_t chroma = (*src)  & 0x00FF00FF;
        uint32_t luma1 = (*src >>  8) & 0xFF;
        uint32_t luma2 = (*src >> 24) & 0xFF;
        const int l18 = luma1 << 8;
        const int l28 = luma2 << 8;
        const int l224 = luma2 << 24;
        *(dst)   = chroma | l18 | (luma1 << 24);
        *(dst+1) = chroma | l18 | l224;
        *(dst+2) = chroma | l28 | l224;
    }
}

void yuvcpy_main(uint32_t* dst, uint32_t* src, int num_pix, int X)
{
    dst = ALIGN32(dst);
    src = ALIGN32(src);
    
    if (X==1)
    {
        #ifdef CONFIG_DMA_MEMCPY
        dma_memcpy(dst, src, num_pix << 1);
        #else
        memcpy(dst, src, num_pix << 1);
        #endif
    }
    else if (X==2)
    {
        yuvcpy_x2(dst, src, num_pix >> 1);
    }
    else if (X==3)
    {
        yuvcpy_x3(dst, src, num_pix/3);
    }
}

void little_cleanup(void* BP, void* MP)
{
    uint8_t* bp = BP; uint8_t* mp = MP;
    if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
    bp++; mp++;
    if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
    bp++; mp++;
    if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
    bp++; mp++;
    if (*bp != 0 && *bp == *mp) *mp = *bp = 0;
}

uint32_t yuv422_get_pixel(uint32_t* buf, int pixoff)
{
    uint32_t* src = &buf[pixoff / 2];
    
    uint32_t chroma = (*src)  & 0x00FF00FF;
    uint32_t luma1 = (*src >>  8) & 0xFF;
    uint32_t luma2 = (*src >> 24) & 0xFF;
    uint32_t luma = pixoff % 2 ? luma2 : luma1;
    return (chroma | (luma << 8) | (luma << 24));
}
