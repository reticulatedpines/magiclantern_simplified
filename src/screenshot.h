#ifndef _screenshot_h_
#define _screenshot_h_

/**
 * Take a screenshot of the BMP overlay and (optionally) the YUV overlay
 * and save it as PPM (a very simple image format).
 * 
 * filename can be:
 * - 0 -> screenshot will be VRAM0.PPM to VRAM9999.PPM
 * - a plain file name, including the PPM extension
 * - a file pattern containing a %d or similar (e.g. "screen%02d.png")
 *
 * also_yuv: if true, also save the LiveView overlay wherever the BMP is transparent.
 * 
 * returns 1 on success, 0 on failure.
 */
int take_screenshot( char* filename, uint32_t mode );

#define SCREENSHOT_FILENAME_AUTO 0  /* pass it instead of filename => VRAM0.PPM - VRAM9999.PPM in root directory of the ML card */
#define SCREENSHOT_BMP 1            /* mode flag: save BMP overlays */
#define SCREENSHOT_YUV 2            /* mode flag: save YUV422 overlays (specify both flags to get them merged) */

#endif
