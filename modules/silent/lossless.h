/* Lossless raw compression */

#ifndef _lossless_h_
#define _lossless_h_

/* to be called before anything else; 1 = success */
int lossless_init();

/* returns 1 on success */
int lossless_compress_raw_start(struct raw_info * raw_info, struct memSuite * output_memsuite);

/* returns output size if successful, negative on error */
int lossless_compress_raw_finish();

/* wrapper for the above two */
int lossless_compress_raw(struct raw_info * raw_info, struct memSuite * output_memsuite);

#endif
