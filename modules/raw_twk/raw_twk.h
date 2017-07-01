
#ifndef __raw_twk_h__
#define __raw_twk_h__

/* only for external code, provide the prototype as weak function */
#ifdef __raw_twk_c__
    #define EXT_WEAK_FUNC(f) 
#else
    #define EXT_WEAK_FUNC(f) WEAK_FUNC(f)
#endif



uint32_t EXT_WEAK_FUNC(ret_1) raw_twk_set_zoom(uint32_t zoom, uint32_t x_pct, uint32_t y_pct);

/* render a raw frame into lv buf. directly accesses lv buffer via get_lcd_422_buf(), nothing else */
uint32_t EXT_WEAK_FUNC(ret_1) raw_twk_render_ex(void *raw_buffer, uint32_t xRes, uint32_t yRes, uint32_t bpp, uint32_t quality, uint32_t blackLevel);
/* the older one directly accesses raw_info. kept for compatibility reasons */
uint32_t EXT_WEAK_FUNC(ret_1) raw_twk_render(void *raw_buffer, uint32_t xRes, uint32_t yRes, uint32_t bpp, uint32_t quality);

/* check if the module is available */
uint32_t EXT_WEAK_FUNC(ret_0) raw_twk_available();


#endif
