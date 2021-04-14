/** \file
 * VRAM RGBA Compositor interface
 */
/*
 * kitor: So I found a compositor on EOSR
 * Uses up to 6 RGBA input layers, but Canon ever allocates only two.
 *
 * This uses RGBA layers that @coon noticed on RP long time ago.
 *
 * Layers are stored from bottom (0) to top (5). Canon uses 0 for GUI
 * and 1 for overlays in LV mode (focus overlay)
 *
 * I was able to create own layer(s), drawn above two Canon pre-allocated ones.
 * This PoC will allocate one new layer on top of existing two, and use that
 * buffer to draw on screen.
 *
 * Tested (briefly) on LV, menus, during recording, playback, also on HDMI.
 *
 * The only caveat that I was able to catch was me calling redraw while GUI
 * also wanted to redraw screen. This "glitched" by showing partially rendered
 * frame. Shouldn't be an issue while we have a control over GUI events.
 *
 * But since we don't have to constantly redraw until we want to update the
 * screen - there's no need to fight with Canon code.
 *
 * For drawing own LV overlays it should be enough to disable layers 0 (GUI)
 * and maybe 1 (AF points, AF confirmation).
 * LV calls redraw very often, so probably we don't need to call it ourselves
 * in that mode.
 */

#include "dryos.h"
#include "bmp.h"
#include "compositor.h"

extern int uart_printf(const char * fmt, ...);

//our vram struct
#ifdef FEATURE_COMPOSITOR_XCM
struct MARV *rgb_vram_info = 0x0;

int _rgb_vram_layer = 0;

/*
 * Family of functions new to compositor.
 * Terminology:
 * Ximr - X image renderer (?). Compositor itself.
 * XCM  - Ximr Context Maker. "Userspace" tool to deal with Ximr setup.
 *        Exists on R, RP, DNE on m50, 200d.
 * XOC  - Ximr Output Chunk.
 */

/*
 * XCM functions - for cameras that have it.
 * FEATURE_COMPOSITOR_XCM
 */

/*
 * Pure Ximr functions - for cameras without XCM
 * FEATURE_COMPOSITOR_XIMR
 */

//TODO: Add Ximr support from 200D PoC

/*
 * Common functions and structures for both implementations.
 */
/*
 * R has XCM_SetRefreshDisplay() which all it does is set refreshDisplay
 * location to 1. For compatibility reasons with 200d and similar
 * (no compositor), memory write will be used.
 */
extern void      RefreshVrmsSurface();
extern uint32_t  display_refresh_needed;

/*
 * All the important memory structures.
 * Note: those may be valid only for R. RP seems to have simpler setup,
 *       but it wasn't validated yet.
 *
 * RENDERER_LayersArr    holds MARV struct pointers to layers created by
 *                       RENDERER_InitializeScreen(). Those are layers created
 *                       by Canon GUI renderer code.
 *                       SaveVRAM on EvShell uses this array for enumeration.
 * VMIX_LayersArr        Like above, but used by VMIX for XCM rendering setup.
 *                       Entries from array above are copied in (what I called)
 *                       Initialize_Renedrer() after RENDERER_InitializeScreen()
 *                       and VMIX_InitializeScreen() are done.
 * VMIX_LayersEnableArr  Controls if layer with given ID is displayed or not.
 *                       Set up just after XCM_LayersArr is populated.
 * XCM_Inititialized     Variable set by refreshVrmsSurface after it initialized
 *                       XCM (XCM_Reset(), ... ). Init ends with debug message
 *                       containing "initializeXimrContext".
 */
extern struct MARV *RENDERER_LayersArr[XCM_MAX_LAYERS];
extern struct MARV *VMIX_LayersArr[XCM_MAX_LAYERS];
extern uint32_t     VMIX_LayersEnableArr[XCM_MAX_LAYERS];
extern uint32_t     XCM_Inititialized;

/*
 * Not sure if sync_caches() call is needed. It was when I was drawing
 * over Canon buffers, but now when we have our own may be unnecessary.
 */
void surface_redraw()
{
    display_refresh_needed = 1;
    sync_caches();
    RefreshVrmsSurface();
}

/*
 * This array toggles corresponding layers. What is weird, this has no effect
 * on XCMStatus command output, they will still be seen as "enabled":
 *
 * [Input Layer] No:2, Enabled:1
 *  VRMS Addr:0x000e0c08, pImg:0x00ea6d54, pAlp:0x00000000, W:960, H:540
 *  Color:=0x05040100, Range:FULL
 *  srcX:0120, srcY:0030, srcW:0720, srcH:0480, dstX:0000, dstY:0000
 */
void surface_set_visibility(int state)
{
    if((XCM_Inititialized == 0) || (rgb_vram_info == 0x0))
        return;

    VMIX_LayersEnableArr[_rgb_vram_layer] = state;
    //do we want to call redraw, or leave it to the caller?
    surface_redraw();
}

void surface_clean()
{
    if(rgb_vram_info == 0x0)
        return;

    bzero32(rgb_vram_info->bitmap_data, BMP_VRAM_SIZE*4);
}

/*
 * Create a new VRAM (MARV) structure, alloc buffer for VRAM.
 * Call compositor to enable newly created layer.
 */
int surface_setup()
{
    //just in case we raced renderer init code.
    if(XCM_Inititialized == 0)
        return 1;

    //may differ per camera? R and RP have 6.
    int newLayerID = 0;
    for(int i = 0; i < XCM_MAX_LAYERS; i++)
    {
        if(RENDERER_LayersArr[i] == NULL)
            break;

        newLayerID++;
    }

    uart_printf("Found %d layers\n", newLayerID);
    if(newLayerID >= XCM_MAX_LAYERS)
    {
        uart_printf("Too many layers: %d/%d, aborting!\n",
                newLayerID, XCM_MAX_LAYERS);
        return 1;
    }

    struct MARV* pNewLayer = malloc(sizeof(struct MARV));
    uint8_t* pBitmapData = malloc(BMP_VRAM_SIZE*4);

    if((pNewLayer == NULL) || (pBitmapData == NULL))
    {
        uart_printf("New layer preparation failed.\n");
        return 1;
    }

    //clean up new surface
    bzero32(pBitmapData, BMP_VRAM_SIZE*4);

    uint16_t bmp_w = BMP_W_PLUS - BMP_W_MINUS;
    uint16_t bmp_h = BMP_H_PLUS - BMP_H_MINUS;

    //prepare MARV
    pNewLayer->signature    = 0x5652414D;  //MARV
    pNewLayer->bitmap_data  = pBitmapData;
    pNewLayer->opacity_data = 0x0;
    pNewLayer->flags        = 0x5040100;   //bitmask (?) for RGBA
    pNewLayer->width        = (uint32_t)bmp_w;
    pNewLayer->height       = (uint32_t)bmp_h;
    pNewLayer->pmem         = 0x0;

    uart_printf("pNewLayer   at 0x%08x\n", pNewLayer);
    uart_printf("pBitmapData at 0x%08x\n", pBitmapData);

    /*
     * The code below seems to be R specific. RP/R6 seems to use single struct
     * for that.
     */

    //add new layer to compositor layers array
    RENDERER_LayersArr[newLayerID] = pNewLayer;
    VMIX_LayersArr[newLayerID] = pNewLayer;

    //enable new layer - just in case (all were enabled by default on R180)
    VMIX_LayersEnableArr[newLayerID] = 1;

    //save rgb_vram_info as last step, in case something above fails.
    rgb_vram_info   = pNewLayer;
    _rgb_vram_layer = newLayerID;

    //make sure XCM notices a new layer by calling redraw
    surface_redraw();
    return 0;
}

/* test drawing outside bmp.h code */
void rgba_fill(uint32_t color, int x, int y, int w, int h)
{
    if(rgb_vram_info == 0x0)
    {
        uart_printf("ERROR: rgb_vram_info not initialized\n");
        return;
    }

    //Note: buffers are GBRA :)
    uint32_t *b = (uint32_t*)rgb_vram_info->bitmap_data;
    for (int i = y; i < y + h; i++)
    {
        uint32_t *row = b + 960*i + x;
        for(int j = x; j < x + w; j++)
            *row++ = color;
    }
}

#endif
