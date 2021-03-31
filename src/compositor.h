#ifndef _compositor_h_
#define _compositor_h_

#include "vram.h"
#include "bmp.h"

int surface_setup();
void rgba_fill(uint32_t color, int x, int y, int w, int h);
void surface_redraw();
void surface_clean();

#define XCM_MAX_LAYERS   6             //This is hardcoded on R/RP code
/*
 * Just a pointer to MARV four our own layer, plus layer ID for toggling later.
 */
extern int _rgb_vram_layer;// = 0;

inline uint8_t *compositor_preinit()
{
#ifdef FEATURE_COMPOSITOR_XCM
    struct MARV *MARV = _rgb_vram_info;
    if(MARV)
        rgb_vram_info = _rgb_vram_info;

    return rgb_vram_info ? rgb_vram_info->bitmap_data : NULL;
#else
    //it shouldn't happend.
    return NULL;
#endif
}
#endif
