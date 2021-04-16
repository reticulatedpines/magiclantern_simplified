#ifndef _compositor_h_
#define _compositor_h_

#include "vram.h"
#include "bmp.h"

int surface_setup();
void rgba_fill(uint32_t color, int x, int y, int w, int h);
void surface_clean();

#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII)
#define XCM_MAX_LAYERS 8
#elif defined(CONFIG_DIGIC_VIII) || defined(CONFIG_DIGIC_X)
#define XCM_MAX_LAYERS 6
#endif
/*
 * Just a pointer to MARV four our own layer, plus layer ID for toggling later.
 */
extern int _rgb_vram_layer;// = 0;

#endif
