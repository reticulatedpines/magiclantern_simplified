/**
 * The CMV12000 is affected by pattern noise - a scalar offset applied to
 * each row, and also to each column. The sensor datasheet refers to this
 * noise as "row noise", but it affects the columns as well.
 *
 * Unfortunately, the pattern is not constant, but varies from frame to frame,
 * so we can't call it "fixed-pattern noise" (even if it looks similar),
 * and we can't get rid of it completely with a dark frame (but it helps).
 *
 * Here, we will try to estimate the pattern noise from low-contrast areas
 * in the image (where this kind of noise is obvious).
 */

#include "stdint.h"

void fix_pattern_noise(int16_t * raw, int w, int h, int white, int debug_flags);

/* debug flags */
#define FIXPN_DBG_COLNOISE  0
#define FIXPN_DBG_ROWNOISE  1

#define FIXPN_DBG_DENOISED  2
#define FIXPN_DBG_NOISE     4
#define FIXPN_DBG_MASK      8