/**
 
 */

/*
 * Copyright (C) 2013 Magic Lantern Team
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#define __raw_twk_c__

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <beep.h>
#include <menu.h>
#include <config.h>
#include <edmac.h>
#include <raw.h>
#include <zebra.h>
#include <util.h>
#include <imgconv.h>
#include <vram.h>
#include <math.h>
#include <string.h>

#include "raw_twk.h"

#define CONFIG_RAW_LIVEVIEW
#define dbg_printf(fmt,...) {}

typedef struct 
{
    void *frameBuffer;
    uint16_t xRes;
    uint16_t yRes;
    uint16_t quality;
    uint16_t bpp;
    uint16_t blackLevel;
    uint8_t  freeFrameBuffer;
} frame_buf_t;

struct s_DefcData
{
    void *pSrcAddress;
    void *pDestAddr;
    uint32_t xRes;
    uint32_t yRes;
    uint32_t rawDepth14;
    uint32_t dstModeRaw;
};

void *get_lcd_422_buf();


static struct msg_queue *mlv_play_queue_raw_to_bpp16;
static struct msg_queue *mlv_play_queue_bpp16_to_yuv;


static int raw_twk_task_abort = 0;
static int raw_twk_zoom = 1;
static int raw_twk_zoom_x_pct = 0;
static int raw_twk_zoom_y_pct = 0;

static struct semaphore * edmac_write_done_sem = 0;
static struct LockEntry * resLock = 0;

static uint32_t edmac_read_chan = 10;
static uint32_t edmac_write_chan = 1;
static uint32_t edmac_read_connection = 1;
static uint32_t edmac_write_connection = 16;

#define ZOOMED_LV2RAW_X(x) ((((x) * (lv2raw.sx / (raw_twk_zoom?raw_twk_zoom:1)) + lv2raw.sx * raw_twk_zoom_x) >> 10))
#define ZOOMED_LV2RAW_Y(y) ((((y) * (lv2raw.sy / (raw_twk_zoom?raw_twk_zoom:1)) + lv2raw.sy * raw_twk_zoom_y) >> 10))

#define CBITS_709   (8)
#define CRANGE_709  (1 << CBITS_709)
#define CYR_709 ((int32_t) ( 0.2126f * CRANGE_709))
#define CYG_709 ((int32_t) ( 0.7152f * CRANGE_709))
#define CYB_709 ((int32_t) ( 0.0722f * CRANGE_709))
#define CUR_709 ((int32_t) (-0.1146f * CRANGE_709))
#define CUG_709 ((int32_t) (-0.3854f * CRANGE_709))
#define CUB_709 ((int32_t) ( 0.5000f * CRANGE_709))
#define CVR_709 ((int32_t) ( 0.5000f * CRANGE_709))
#define CVG_709 ((int32_t) (-0.4541f * CRANGE_709))
#define CVB_709 ((int32_t) (-0.0458f * CRANGE_709))

#define PACK_HALVES(u,l) (((uint32_t)(u & 0xFFFF) << 16) | (uint32_t)(l & 0xFFFF))

const uint32_t yuv_709_table[] = { PACK_HALVES(CYR_709,CUR_709), PACK_HALVES(CYG_709,CUG_709), PACK_HALVES(CYB_709,CUB_709), CVR_709, CVG_709, CVB_709 };

static inline uint32_t rgb2yuv422_rec709_opt(int R, int G, int B)
{
/* basically it is this code, just optimzied as much as i can:

    int Y = (CYR_709 * R + CYG_709 * G + CYB_709 * B) / CRANGE_709;
    int U = (CUR_709 * R + CUG_709 * G + CUB_709 * B) / CRANGE_709;
    int V = (CVR_709 * R + CVG_709 * G + CVB_709 * B) / CRANGE_709;
    return UYVY_PACK(U,Y,V,Y);
*/
    int ret = 0;
    
    /* warning: reordered instructions to (hopefully) prevent pipelining effects */
    asm volatile (
        /* load parameters */
        "LDR R3, =yuv_709_table\r\n"
        "LDMIA R3!, {R4-R9}\r\n"
        
        /* multiply RGB into 16 bit fields in 32 bit registers */
        "MOV R12, #0xFF\r\n"
        /* YU */ "MUL R10, R4, %[R]\r\n"
        /* YU */ "MLA R10, R5, %[G], R10\r\n"
        /* YU */ "MLA R10, R6, %[B], R10\r\n"
        "AND R11, R10, R12, LSL#24\r\n"
        
        /* channel V is multiplied separately */
        /* V */ "MUL R4, R7, %[R]\r\n"
        "ORR R11, R11, LSR#16\r\n" /* R11: YY00YY00 */
        /* V */ "MLA R4, R8, %[G], R4\r\n"
        "AND R10, R10, R12, LSL#8\r\n" /* YYYYUUUU -> 0000UU00 */
        /* V */ "MLA R4, R9, %[B], R4\r\n"
        "ORR R11, R10, LSR#8\r\n" /* R11: YY00YYUU */        
        "AND R4, R4, R12, LSL#8\r\n" /* 0000VVVV -> 0000VV00 */
        "ORR %[ret], R11, R4, LSL#8\r\n" /* R11: YYVVYYUU */
        : 
       [ret]"=r"(ret) : 
       [R]"r"(R),  [G]"r"(G),  [B]"r"(B)  : 
       "r3", /* table */
       "r4", "r5", "r6", "r7", "r8", "r9", /* multipliers from table */
       "r10", "r11", "r12" /* temporary results */
    );
    
    return ret;
}

uint32_t raw_twk_set_zoom(uint32_t zoom, uint32_t x_pct, uint32_t y_pct)
{
    raw_twk_zoom_x_pct = MIN(100, MAX(0, x_pct));
    raw_twk_zoom_y_pct = MIN(100, MAX(0, y_pct));
    
    uint32_t zooom_table[] = { 1, 2, 4, 8 };
    
    raw_twk_zoom = zooom_table[MAX(0, MIN(COUNT(zooom_table), zoom))];
    
    return 0;
}

static void bpp16_to_yuv_task()
{
    TASK_LOOP
    {
        frame_buf_t *in_buf;
        
        /* signal to stop rendering */
        if(raw_twk_task_abort)
        {
            break;
        }
        
        /* is there something to render? */
        if(msg_queue_receive(mlv_play_queue_bpp16_to_yuv, &in_buf, 50))
        {
            continue;
        }

        if(!in_buf->frameBuffer)
        {
            bmp_printf(FONT_MED, 30, 400, "buffer empty");
            beep();
            msleep(1000);
            free(in_buf);
            break;
        }
        
        int xRes = in_buf->xRes;
        int yRes = in_buf->yRes;
        int y1 = BM2LV_X(os.y0);
        int y2 = BM2LV_X(os.y_max);
        int x1 = MAX(BM2LV_X(os.x0), RAW2LV_X(0));
        int x2 = MIN(BM2LV_X(os.x_max), RAW2LV_X(xRes));
        
        uint16_t* lv16 = CACHEABLE(get_lcd_422_buf());
        uint32_t* lv32 = (uint32_t*) lv16;
        uint64_t* lv64 = (uint64_t*) lv16;
    
        if (!lv16) return;
        if (x2 < x1) return;
        
    
        /* cache the LV to RAW transformation for the inner loop to make it faster */
        /* white balance 2,1,2 => use two gamma curves to simplify code */
        static uint8_t gamma_rb[1024];
        static uint8_t gamma_g[1024];
        static uint8_t gamma[1024];
        static int* lv2rx = NULL;
        static int* lv2ry = NULL;
        static int last_x1 = 0;
        static int last_x2 = 0;
        static int last_black_level = 0;
        static int last_zoom = 0;
        static int last_zoom_x = 0;
        static int last_zoom_y = 0;

        if(last_zoom != raw_twk_zoom || last_zoom_x != raw_twk_zoom_x_pct || last_zoom_y != raw_twk_zoom_y_pct || last_black_level != in_buf->blackLevel || last_x1 != x1 || last_x2 != x2)
        {
            if(lv2rx)
            {
                free(lv2rx);
            }
            if(lv2ry)
            {
                free(lv2ry);
            }
            lv2rx = NULL;
            lv2ry = NULL;
        }
        
        if(lv2rx == NULL)
        {
            last_x1 = x1;
            last_x2 = x2;
            last_black_level = in_buf->blackLevel;
            last_zoom = raw_twk_zoom;
            last_zoom_x = raw_twk_zoom_x_pct;
            last_zoom_y = raw_twk_zoom_y_pct;
            
            /* as we only work with 10 upper bits and input data is 16bpp, throw away 6 bits */
            int black = (in_buf->blackLevel >> 6);
            
            for (int i = 0; i < 1024; i++)
            {
                int g = (i > black) ? log2f(i - black) * 255 / 10 : 0;
                gamma[i] = g * g / 255; /* idk, looks better this way */
            }
            for (int i = 0; i < 1024; i++)
            {
                /* only show 10 bits */
                int g_rb = (i > black) ? (log2f(i - black) + 1) * 255 / 10 : 0;
                int g_g  = (i > black) ? (log2f(i - black)) * 255 / 10 : 0;
                gamma_rb[i] = COERCE(g_rb * g_rb / 255, 0, 255); /* idk, looks better this way */
                gamma_g[i]  = COERCE(g_g  * g_g  / 255, 0, 255); /* (it's like a nonlinear curve applied on top of log) */
            }
            
            lv2rx = malloc(x2 * sizeof(int));
            if (!lv2rx) return;
            for (int x = x1; x < x2; x++)
            {
                /* from the percent position of the zoom window, determine the number of pixels */
                int zoom_offset = (raw_twk_zoom_x_pct * (vram_lv.width - vram_lv.width / raw_twk_zoom)) / 100;
                /* determine the LV2RAW steps depending on zoom level */
                int step = lv2raw.sx / (raw_twk_zoom?raw_twk_zoom:1);

                /* this is based on LV2RAW */
                lv2rx[x] = (((x * step + lv2raw.sx * zoom_offset) >> 10) + lv2raw.tx) & ~1;
            }
            
            lv2ry = malloc(y2 * sizeof(int));
            if (!lv2ry) return;
            for (int y = y1; y < y2; y++)
            {
                /* from the percent position of the zoom window, determine the number of pixels */
                int zoom_offset = (raw_twk_zoom_y_pct * (vram_lv.height - vram_lv.height / raw_twk_zoom)) / 100;
                /* determine the LV2RAW steps depending on zoom level */
                int step = lv2raw.sy / (raw_twk_zoom?raw_twk_zoom:1);

                /* this is based on LV2RAW */
                lv2ry[y] = (((y * step + lv2raw.sy * zoom_offset) >> 10) + lv2raw.ty) & ~1;
                
                /* on HDMI screens, BM2LV_DX() may get negative */
                if((lv2ry[y] <= 0 || lv2ry[y] >= yRes) && BM2LV_DX(x2-x1) > 0)
                {
                    /* out of range, just fill with black */
                    lv2ry[y] = -1;
                }
            }
        }
        
        
        if(in_buf->quality == RAW_PREVIEW_GRAY_ULTRA_FAST)
        {
            /* full-res vertically */
            for (int y = y1; y < y2; y+=1)
            {
                /* precalculated above */
                if(lv2ry[y] < 0)
                {
                    /* out of range, just fill with black */
                    memset(&lv32[LV(0,y)/4], 0, BM2LV_DX(x2-x1)*2);
                    continue;
                }
                
                uint16_t * row = ((uint16_t *)in_buf->frameBuffer) + (lv2ry[y] * xRes);

                /* half-res horizontally, to simplify YUV422 math */
                for (int x = x1; x < x2; x += 4)
                {
                    int xr = lv2rx[x];
                    
                    uint32_t c = row[xr];
                    uint64_t Y = gamma[c>>6];
                    
                    Y = (Y << 8) | (Y << 24) | (Y << 40) | (Y << 56);
                    int idx = LV(x,y)/8;
                    lv64[idx] = Y;
                    lv64[idx + vram_lv.pitch/8] = Y;
                }
            }
        }
        else
        {
            /* full-res vertically */
            for (int y = y1; y < y2; y+=1)
            {
                /* precalculated above */
                if(lv2ry[y] < 0)
                {
                    /* out of range, just fill with black */
                    memset(&lv32[LV(0,y)/4], 0, BM2LV_DX(x2-x1)*2);
                    continue;
                }
                
                uint16_t * row = ((uint16_t *)in_buf->frameBuffer) + (lv2ry[y] * xRes);
                
                /* half-res horizontally, to simplify YUV422 math */
                for (int x = x1; x < x2; x += 2)
                {
                    int xr = lv2rx[x];
                    
                    uint32_t rg = *((uint32_t*) &row[xr]) >> 6;
                    uint16_t r = rg & 0x3FF;
                    uint16_t g = rg >> 16;
                    uint16_t b = row[xRes + xr + 1] >> 6;

                    r = gamma_rb[r];
                    g = gamma_g [g];
                    b = gamma_rb[b];
                    
                    lv32[LV(x,y)/4] = rgb2yuv422_rec709_opt(r, g, b);
                }
            }
        }
        
        if(in_buf->freeFrameBuffer)
        {
            free(in_buf->frameBuffer);
        }
        free(in_buf);
    }
}

static void edmac_read_complete_cbr(void *ctx)
{
}

static void edmac_write_complete_cbr(void * ctx)
{
    give_semaphore(edmac_write_done_sem);
}

static uint32_t upconvert_bpp(frame_buf_t *frame, void* buf_out)
{
    uint32_t ret = 0;
    
    /* configure image processing modules */
    const uint32_t DS_SEL            = 0xC0F08104;
    const uint32_t PACK16_ISEL       = 0xC0F082D0;
    const uint32_t PACK16_ISEL2      = 0xC0F0839C;
    const uint32_t WDMAC16_ISEL      = 0xC0F082D8;
    const uint32_t DSUNPACK_ENB      = 0xC0F08060;
    const uint32_t DSUNPACK_MODE     = 0xC0F08064;
    const uint32_t DSUNPACK_DM_EN    = 0xC0F08274;
    const uint32_t DEF_ENB           = 0xC0F080A0;
    const uint32_t DEF_80A4          = 0xC0F080A4;
    const uint32_t DEF_MODE          = 0xC0F080A8;
    const uint32_t DEF_CTRL          = 0xC0F080AC;
    const uint32_t DEF_YB_XB         = 0xC0F080B0;
    const uint32_t DEF_YN_XN         = 0xC0F080B4;
    const uint32_t DEF_YA_XA         = 0xC0F080BC;
    const uint32_t DEF_INTR_EN       = 0xC0F080D0;
    const uint32_t DEF_HOSEI         = 0xC0F080D4;
    const uint32_t DEFC_X2MODE       = 0xC0F08270;
    const uint32_t DEFC_DET_MODE     = 0xC0F082B4;
    const uint32_t PACK16_ENB        = 0xC0F08120;
    const uint32_t PACK16_MODE       = 0xC0F08124;
    const uint32_t PACK16_DEFM_ON    = 0xC0F082B8;
    const uint32_t PACK16_ILIM       = 0xC0F085B4;
    const uint32_t PACK16_CCD2_DM_EN = 0xC0F0827C;

    /* for PACK16_MODE, DSUNPACK_MODE, ADUNPACK_MODE (mask 0x131) */
    const uint32_t MODE_10BIT        = 0x000;
    const uint32_t MODE_12BIT        = 0x010;
    const uint32_t MODE_14BIT        = 0x020;
    const uint32_t MODE_16BIT        = 0x120;

    uint32_t pixel_mode = 0;
    
    switch(frame->bpp)
    {
        case 10:
            pixel_mode = MODE_10BIT;
            break;
        case 12:
            pixel_mode = MODE_12BIT;
            break;
        case 14:
            pixel_mode = MODE_14BIT;
            break;
        case 16:
            pixel_mode = MODE_16BIT;
            break;
    }
    
    if (!resLock)
    {
        int edmac_read_ch_index = edmac_channel_to_index(edmac_read_chan);
        int edmac_write_ch_index = edmac_channel_to_index(edmac_write_chan);
        uint32_t resIds[] = {
            0x00000000 | edmac_write_ch_index,      /* write edmac channel */
            0x00010000 | edmac_read_ch_index,       /* read edmac channel */
            0x00020000 | edmac_write_connection,    /* write connection */
            0x00030000 | edmac_read_connection,     /* read connection */
            0x00050002,                             /* DSUNPACK? */
            0x00050005,                             /* DEFC? */
            0x0005001d,                             /* PACK16/WDMAC16 */
            0x0005001f,                             /* PACK16/WDMAC16 */
        };
        resLock = CreateResLockEntry(resIds, COUNT(resIds));
    }
   
    LockEngineResources(resLock);
    
    engio_write((uint32_t[]) {
        /* input selection for the processing modules? */
        DS_SEL,         0,
        PACK16_ISEL,    4,
        PACK16_ISEL2,   0,
        WDMAC16_ISEL,   0,
        
        /* DSUNPACK module (input image data) */
        DSUNPACK_ENB,   0x80000000,
        DSUNPACK_MODE,  pixel_mode,
        DSUNPACK_DM_EN, 0,
        
        /* DEF(C) module (is it needed?) */
        DEF_ENB,        0x80000000,
        DEF_80A4,       0,
        DEF_CTRL,       0,
        DEF_MODE,       0x104,
        DEF_YN_XN,      0,
        DEF_YB_XB,      ((frame->yRes-1) << 16) | (frame->xRes-1),
        DEF_YA_XA,      0,
        DEF_INTR_EN,    0,
        DEF_HOSEI,      0x11,
        DEFC_X2MODE,    0,
        DEFC_DET_MODE,  1,

        /* PACK16 module (output image data) */
        PACK16_ENB,     0x80000000,
        PACK16_MODE,    MODE_16BIT,
        PACK16_DEFM_ON, 1,
        PACK16_ILIM,    0x3FFF, /* white level? */
        PACK16_CCD2_DM_EN, 0,
        
        /* whew! */
        0xFFFFFFFF,     0xFFFFFFFF
    });
 
    /* EDMAC setup */
    RegisterEDmacCompleteCBR(edmac_read_chan, edmac_read_complete_cbr, 0);
    RegisterEDmacCompleteCBR(edmac_write_chan, edmac_write_complete_cbr, 0);

    ConnectWriteEDmac(edmac_write_chan, edmac_write_connection);
    ConnectReadEDmac(edmac_read_chan, edmac_read_connection);

    struct edmac_info src_edmac_info = {
        .xb = frame->xRes * frame->bpp / 8,
        .yb = frame->yRes - 1,
    };
    
    struct edmac_info dst_edmac_info = {
        .xb = frame->xRes * 2,
        .yb = frame->yRes - 1,
    };
   
    SetEDmac(edmac_read_chan, frame->frameBuffer, &src_edmac_info, 0x20000);
    SetEDmac(edmac_write_chan, buf_out, &dst_edmac_info, 1);

    /* start processing */
    StartEDmac(edmac_write_chan, 0);

    /* start operation */
    engio_write((uint32_t[]) {
        PACK16_ENB,     1,
        DEF_ENB,        1,
        DSUNPACK_ENB,   1,
        0xFFFFFFFF,     0xFFFFFFFF
    });
   
    StartEDmac(edmac_read_chan, 2);
   
    /* wait for everything to finish */
    if(take_semaphore(edmac_write_done_sem, 100))
    {
        ret = 1;
    }

    /* reset processing modules */
    engio_write((uint32_t[]) {
        PACK16_ENB,     0x80000000,
        DEF_ENB,        0x80000000,
        DSUNPACK_ENB,   0x80000000,
        0xFFFFFFFF,     0xFFFFFFFF
    });

    /* aborting EDMACs is required so they don't stay waiting, which would block some other routines using them */
    AbortEDmac(edmac_read_chan);
    AbortEDmac(edmac_write_chan);
    UnregisterEDmacCompleteCBR(edmac_read_chan);
    UnregisterEDmacCompleteCBR(edmac_write_chan);
    UnLockEngineResources(resLock);
    
    return ret;
}

static void raw_to_bpp16_task()
{
    TASK_LOOP
    {
        frame_buf_t *in_buf;
        
        /* signal to stop rendering */
        if(raw_twk_task_abort)
        {
            break;
        }
        
        /* is there something to render? */
        if(msg_queue_receive(mlv_play_queue_raw_to_bpp16, &in_buf, 50))
        {
            continue;
        }

        if(!in_buf->frameBuffer)
        {
            bmp_printf(FONT_MED, 30, 400, "in_buf empty");
            beep();
            msleep(1000);
            break;
        }
        
        void *out_frameBuffer = malloc(in_buf->xRes * in_buf->yRes * 16 / 8);
        
        /* if conversion failed, silently fail for now */
        if(upconvert_bpp(in_buf, out_frameBuffer))
        {
            bmp_printf(FONT_MED, 30, 400, "upconvert_bpp failed");
            beep();
            msleep(1000);
            
            free(out_frameBuffer);
            free(in_buf);
            break;
        }
        
        /* reuse the message buffer */
        in_buf->blackLevel = in_buf->blackLevel << (16 - in_buf->bpp);
        in_buf->frameBuffer = out_frameBuffer;
        in_buf->freeFrameBuffer = 1;
        
        /* and post it to display task, meanwhile this engine can process another frame */
        msg_queue_post(mlv_play_queue_bpp16_to_yuv, (uint32_t)in_buf);
    }
}

uint32_t raw_twk_render_ex(void *raw_buffer, uint32_t xRes, uint32_t yRes, uint32_t bpp, uint32_t quality, uint32_t blackLevel)
{
    frame_buf_t *msg = malloc(sizeof(frame_buf_t));
    
    msg->frameBuffer = raw_buffer;
    msg->xRes = xRes;
    msg->yRes = yRes;
    msg->bpp = bpp;
    msg->quality = quality;
    msg->freeFrameBuffer = 0;
    msg->blackLevel = blackLevel;

    /* caller might be so kind and give us an already 16 bit aligned buffer. we wouldnt have to do much */
    if(msg->bpp != 16)
    {
        return msg_queue_post(mlv_play_queue_raw_to_bpp16, (uint32_t)msg);
    }
    else
    {
        return msg_queue_post(mlv_play_queue_bpp16_to_yuv, (uint32_t)msg);
    }
}

uint32_t raw_twk_render(void *raw_buffer, uint32_t xRes, uint32_t yRes, uint32_t bpp, uint32_t quality)
{
    return raw_twk_render_ex(raw_buffer, xRes, yRes, bpp, quality, raw_info.black_level);
}

static unsigned int raw_twk_init()
{
    edmac_write_done_sem = create_named_semaphore("edmac_write_done_sem", 0);
    mlv_play_queue_raw_to_bpp16 = (struct msg_queue *) msg_queue_create("mlv_play_queue_raw_to_bpp16", 3);
    mlv_play_queue_bpp16_to_yuv = (struct msg_queue *) msg_queue_create("mlv_play_queue_bpp16_to_yuv", 3);
    task_create("bpp16_to_yuv_task", 0x15, 0x4000, bpp16_to_yuv_task, 0);
    task_create("raw_to_bpp16_task", 0x15, 0x4000, raw_to_bpp16_task, 0);
    return 0;
}

static unsigned int raw_twk_deinit()
{
    raw_twk_task_abort = 1;
    return 0;
}

uint32_t raw_twk_available()
{
    return 1;
}

MODULE_INFO_START()
    MODULE_INIT(raw_twk_init)
    MODULE_DEINIT(raw_twk_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()

MODULE_CONFIGS_START()
MODULE_CONFIGS_END()
