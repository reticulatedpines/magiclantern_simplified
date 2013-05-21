#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_


#ifdef CONFIG_EDMAC_MEMCPY
void* edmac_memcpy(void* dst, void* src, size_t length);
void* edmac_memset(void* dst, int value, size_t length);

/* crop a rectangle from an image buffer; all sizes in bytes */
void* edmac_copy_rectangle(void* dst, void* src, int src_width, int x, int y, int w, int h);
void* edmac_copy_rectangle_adv(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* non-blocking versions */
void* edmac_memcpy_start(void* dst, void* src, size_t length);
void* edmac_copy_rectangle_start(void* dst, void* src, int src_width, int x, int y, int w, int h);
void* edmac_copy_rectangle_adv_start(void* dst, void* src, int src_width, int src_x, int src_y, int dst_width, int dst_x, int dst_y, int w, int h);

/* these are blocking tho */
void edmac_memcpy_finish();
void edmac_copy_rectangle_finish();
void edmac_copy_rectangle_finish();

#endif

#endif // _edmac_memcpy_h_
