#ifndef _compositor_h_
#define _compositor_h_

#include "vram.h"
#include "bmp.h"

int compositor_layer_setup();
void compositor_layer_clear();

/**
 * Really D8, DX -> 6, D7 -> 8, D6 -> either 7 or 8 depending on Zico fw.
 *
 * Limiting to 6 is OK as Canon code uses either 1 or 2 (GUI + Focus overlays),
 * and so far we we alocate no more than one extra- still leaving headroom of 3.
 */
#define XIMR_MAX_LAYERS 6

/**
 * color / flags field contains at least 4 information, starting from MSB:
 *
 * 8 bits: Possibly Ximr type ID for color space.
 * 8 bits: Bits per pixel?
 * 8 bits: Subsampling factor?
 * 1 bit : Flag if structure utilize that "separate opacity layer"
 * 7 bits: Unknown / not referenced in any code I analyzed.
 *
 * "Bits per pixel" and "Subsampling factor" are used as follows in CreateMARV:
 *
 * BMP_VRAM_SIZE = fast_division(width * bits_per_pixel, subsampling_factor);
 * BMP_VRAM_SIZE = BMP_VRAM_SIZE * height;
 *
 * Examples (real values from D678 ROMs):
 *
 * 0x11060200 UYVYAA:
 *     0x11: type, 0x06: bytes per pixel, 0x02: subsampling factor,
 *     no bit flag set for separate alpha layer
 *
 * 0x01040280 UYVY + alpha:
 *     0x01: type, 0x04: bytes per pixel, 0x02: subsampling factor,
 *     0x80 (10000000b) - use separate alpha
 *
 * 0x05040100 RGBA:
 *     0x05: type, 0x04: bytes per pixel, 0x01: subsampling factor,
 *     no bit flag for separate alpha layer
 *
 * On EOSR i was able to conduct a few experiments:
 * 0x03000000 - nothing is drawn on screen.
 * 0x04020100 - YUV without alpha channel.
 * 0x02010100 - Also YUV, with half the data per pixel from above.
 *
 * WARNING: Early D6 cameras use other flag format. From EOS M10 (PowerShoot):
 * 0x04000002, 0x05000004 and 0x07000002. This depends on Zico FW.
 * I don't know any D6 EOS codebase models that do that, but if any is found,
 * it will require an ifdef below.
 *
 * See https://www.magiclantern.fm/forum/index.php?topic=26024 for more details.
 */
#define XIMR_FLAGS_LAYER_RGBA 0x5040100

// This shouldn't change, but...
#ifndef CANON_GUI_LAYER_ID
#define CANON_GUI_LAYER_ID 0
#endif

// ID of our allocated layer. See compositor.c for initialization.
extern int _rgb_vram_layer_id; // = CANON_GUI_LAYER_ID;

#endif
