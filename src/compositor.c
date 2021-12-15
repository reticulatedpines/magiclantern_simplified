/** \file
 * VRAM Compositor interface
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

/**
 * Terminology:
 * Zico core    - Xtensa core, running DryOS, used for graphics acceeleration,
 *                GUI rendering and compositing. Connected with GPU (Takumi
 *                GV550 IP core)
 * mzrm         - Marius - Zico remote messaging (?). RPC used to talk between
 *                main ARM core (named Marius on D6) and Zico.
 * Ximr         - Render MIXer (?). Low level compositor on D6 and up.
 * Ximr Context - Data structure representing Ximr configuration, sent via mzrm
 *                using XimrExe() to Zico for render.
 * Input layer  - Layer (described by MARV struct) to be included in mix.
 * XOC          - Ximr Output Chunk. Buffer where final image is rendered.
 * XCM          - Ximr Context Maker. "Userspace" tool to deal with Ximr setup.
 *                Exists on D8 and up, from EOS R (M50 uses pure Ximr)
 */

#include "dryos.h"
#include "bmp.h"
#include "compositor.h"

#ifdef CONFIG_COMPOSITOR_DEDICATED_LAYER
/**
 * refreshVrmsSurface (renamed to VMIX_TransferRectangleToVram on Digic X)
 * is used by Canon code to render Input layers into XOC.
 *
 * On pure XIMR cameras it uses XIMR functions to setup Ximr Context
 */
extern void      refreshVrmsSurface();
extern uint32_t  display_refresh_needed;

// Store our Input Layer ID.
int _rgb_vram_layer = 0;

/**
 * Not sure if sync_caches() call is needed. It was when I was drawing
 * over Canon buffers, but now when we have our own may be unnecessary.
 */
void _compositor_force_redraw()
{
    display_refresh_needed = 1;
    ml_refresh_display_needed = 1;
    sync_caches();
    //refreshVrmsSurface();
}

void compositor_layer_clear()
{
    // abort if we draw over Canon GUI layer
    if(_rgb_vram_layer == 0)
        return;

    bzero32(rgb_vram_info->bitmap_data, BMP_VRAM_SIZE*4);
    _compositor_force_redraw();
 }


struct MARV *_compositor_create_layer()
{
    // buffer for layer data
    // looks like code expect it to be in uncacheable area
    uint8_t* pBitmapData = UNCACHEABLE(malloc(BMP_VRAM_SIZE*4));
    if(pBitmapData == NULL) return NULL;

    // buffer for MARV structure
    struct MARV* pNewLayer = malloc(sizeof(struct MARV));
    if(pNewLayer == NULL) return NULL;

    uint16_t bmp_w = BMP_W_PLUS - BMP_W_MINUS;
    uint16_t bmp_h = BMP_H_PLUS - BMP_H_MINUS;

    // fill MARV data
    pNewLayer->signature    = 0x5652414D;  // MARV
    pNewLayer->bitmap_data  = pBitmapData;
    pNewLayer->opacity_data = 0x0;
    // 0x5040100 is used by D78. Some early D6 use a different value
    pNewLayer->flags        = 0x5040100;   // bitmask (?) for RGBA
    pNewLayer->width        = (uint32_t)bmp_w;
    pNewLayer->height       = (uint32_t)bmp_h;
    // Probably pmem is not needed. No issues observed so far.
    pNewLayer->pmem         = 0x0;

    DryosDebugMsg(0, 15, "MARV 0x%08x, bitmap_data 0x%08x", pNewLayer, pBitmapData);

    return pNewLayer;
}

/* Uncomment if you need to test direct RGBA drawing */
/*
void rgba_fill(uint32_t color, int x, int y, int w, int h)
{
    if(rgb_vram_info == 0x0)
    {
        DryosDebugMsg(0, 15, "ERROR: rgb_vram_info not initialized");
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
}*/

#ifdef CONFIG_COMPOSITOR_XCM
/**
 * Implementation specific to XCM
 */

#ifdef CONFIG_R
/*
 * Structures specific to EOSR
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
 */
extern struct MARV *RENDERER_LayersArr[XCM_MAX_LAYERS];
extern struct MARV *VMIX_LayersArr[XCM_MAX_LAYERS];
extern uint32_t     VMIX_LayersEnableArr[XCM_MAX_LAYERS];
#endif // CONFIG_R

void compositor_set_visibility(int state)
{
    /**
     * We either didn't setup layer, or layer setup failed and we draw to
     * Canon GUI layer. Either way, abort - as this was probably not intended.
     */
    if(_rgb_vram_layer == 0)
        return;

#ifdef CONFIG_R
    /**
     * Very specific to EOS R.
     * This array toggles layers visibility. It is independent from XCM,
     * and somehow XCM still thinks layer is enabled.
     * refreshVrmsSurface() regenerate XimrContext on every redraw, so we can't
     * just use XCM functions to toggle layer off.
     */
    VMIX_LayersEnableArr[_rgb_vram_layer] = state;
#else
    DryosDebugMsg(0, 15, "compositor_set_visibility not implemented yet!");
#endif // CONFIG_R
    //do we want to call redraw, or leave it to the caller?
    _compositor_force_redraw();
}

/*
 * Create a new VRAM (MARV) structure, alloc buffer for VRAM.
 * Call compositor to enable newly created layer.
 */
int compositor_layer_setup()
{
    // So far it seems that default GUI layer is always 0
    int newLayerID = 0;
#ifdef CONFIG_R
    for(int i = 0; i < XCM_MAX_LAYERS; i++)
    {
        if(RENDERER_LayersArr[i] == NULL)
            break;
        newLayerID++;
    }
#else
    DryosDebugMsg(0, 15, "Not implemented yet");
    return 1;
#endif // CONFIG_R

    DryosDebugMsg(0, 15, "Found %d layers", newLayerID);
    if(newLayerID >= XCM_MAX_LAYERS)
    {
        DryosDebugMsg(0, 15, "Too many layers: %d/%d, aborting!",
                newLayerID, XCM_MAX_LAYERS);
        return 1;
    }

    // create layer
    struct MARV *pNewLayer = _compositor_create_layer();
    if(pNewLayer == NULL){
        DryosDebugMsg(0, 15, "Failed to create a new layer. Falling back to _rgb_vram_info");
        return 1;
    }
    /*
     * The code below seems to be R specific. RP/R6 seems to use single struct
     * for that.
     */

#ifdef CONFIG_R
    // EOS R specific

    // add new layer to compositor layers array
    RENDERER_LayersArr[newLayerID] = pNewLayer;
    VMIX_LayersArr[newLayerID] = pNewLayer;

    // enable new layer - just in case (all were enabled by default on R180)
    VMIX_LayersEnableArr[newLayerID] = 1;
#else
    DryosDebugMsg(0, 15, "Not implemented yet");
    return 1;
#endif // CONFIG_R

    // save rgb_vram_info as last step, in case something above fails.
    rgb_vram_info   = pNewLayer;
    _rgb_vram_layer = newLayerID;

    // erase buffer and force redraw
    compositor_layer_clear();
    return 0;
}

/*
 // TODO: Write implementationf for pure Ximr, if we ever want it.
 #elif defined(CONFIG_COMPOSITOR_XIMR)
*/
#endif // CONFIG_COMPOSITOR_XCM

#endif // CONFIG_COMPOSITOR_DEDICATED_LAYER
