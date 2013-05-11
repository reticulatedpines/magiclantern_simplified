#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_


#ifdef CONFIG_EDMAC_MEMCPY
void* edmac_memcpy(void* dst, void* src, size_t length);

/* crop a rectangle from an image buffer; all sizes in bytes */
void* edmac_copy_rectangle(void* dst, void* src, int src_width, int x, int y, int w, int h);
#endif

#endif // _edmac_memcpy_h_
