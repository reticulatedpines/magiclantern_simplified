#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_

#include "sys/types.h"
#include "stdint.h"

void* edmac_memcpy(void* dst, void* src, size_t length);
void* edmac_memset(void* dst, int value, size_t length);
uint32_t edmac_find_divider(size_t length, size_t transfer_size);

/* crop a rectangle from an image buffer; all sizes in bytes */
void* edmac_copy_rectangle(void* dst, void* src, int src_width, int x, int y, int w, int h);
void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* non-blocking versions */
void* edmac_memcpy_start(void* dst, void* src, size_t length);
void* edmac_copy_rectangle_start(void* dst, void* src, int src_width, int x, int y, int w, int h);
void* edmac_copy_rectangle_adv_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);
void* edmac_copy_rectangle_cbr_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h, void (*cbr_r)(void*), void (*cbr_w)(void*), void *cbr_ctx);
void edmac_copy_rectangle_adv_cleanup();

/* these are blocking tho */
void edmac_memcpy_finish();
void edmac_copy_rectangle_finish();

/* Lock/unlock engine resources used by edmac_memcpy (only if ported for your camera) */
void edmac_memcpy_res_lock();
void edmac_memcpy_res_unlock();

/* pulls the raw data from EDMAC without Canon's lv_save_raw (for raw backend) */
void edmac_raw_slurp(void* dst, int w, int h);

#endif // _edmac_memcpy_h_
