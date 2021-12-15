#ifndef _compositor_h_
#define _compositor_h_

#include "vram.h"
#include "bmp.h"

int compositor_layer_setup();
void compositor_layer_clear();

#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII)
#define XCM_MAX_LAYERS 8
#elif defined(CONFIG_DIGIC_VIII) || defined(CONFIG_DIGIC_X)
#define XCM_MAX_LAYERS 6
#endif
/*
 * Just a pointer to MARV four our own layer, plus layer ID for toggling later.
 */
extern int _rgb_vram_layer; // = 0;
extern struct semaphore *compositor_vsync_semaphore;

#endif
