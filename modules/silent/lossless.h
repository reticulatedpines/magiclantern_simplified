/* Lossless raw compression */

#ifndef _lossless_h_
#define _lossless_h_

/* to be called before anything else; 1 = success */
int lossless_init();

/* returns output size if successful, negative on error */
int lossless_compress_raw(struct raw_info * raw_info, struct memSuite * output_memsuite);

/* compress a rectangle cropped from a source buffer */
/* similar to edmac_copy_rectangle */
int lossless_compress_raw_rectangle(
    struct memSuite * dst_suite, void * src,
    int src_width, int src_x, int src_y,
    int width, int height
);

/* Rows the input EDMAC reads past the bottom of the rectangle, to push the last
 * real ones through the DIGIC 4 pipeline ahead of the encoder. Zero elsewhere.
 * Callers that own the source buffer must keep that much readable after it. */
int lossless_input_overread_rows(void);

/* Largest width at or below `width` the encoder can render without shear.
 * Identity except on DIGIC 4, which needs width = 4 (mod 8) so the RD1 row
 * length that cancels the SNDPAS advance is a whole even byte count. */
int lossless_encodable_width(int width);

/* Map a LiveView 14-bit level onto the level the encoder emits, for RAWI
 * metadata. Identity except on DIGIC 4, whose input stage subtracts the optical
 * black, applies a per-Bayer gain and re-pedestals, so levels move affinely.
 * `black_in` is the LiveView black the mapping is measured against. */
int lossless_d4_map_level(int level, int black_in);

/* Bit depth the encode should attenuate to, 8..14. Only meaningful on DIGIC 4,
 * where reduced depths are reached through the encoder's OBWB gain stage. */
void lossless_d4_set_target_bpp(int bpp);

int lossless_decompress_raw(
    struct memSuite * src, void * dst,
    int width, int height,
    int output_bpp
);

#endif
