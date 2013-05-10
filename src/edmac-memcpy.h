#ifndef _edmac_memcpy_h_
#define _edmac_memcpy_h_


#ifdef CONFIG_EDMAC_MEMCPY
void* edmac_memcpy(void* dst, void* src, size_t length);
#endif

#endif // _edmac_memcpy_h_
