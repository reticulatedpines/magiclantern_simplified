#ifndef __CHDK_DNG_H_
#define __CHDK_DNG_H_

void dng_set_framerate(int32_t fpsx1000);
void dng_set_thumbnail_size(int32_t width, int32_t height);

void dng_set_framerate_rational(int32_t nom, int32_t denom);
void dng_set_shutter(int32_t nom, int32_t denom);
void dng_set_aperture(int32_t nom, int32_t denom);
void dng_set_camname(char *str);
void dng_set_camserial(char *str);
void dng_set_description(char *str);
void dng_set_lensmodel(char *str);
void dng_set_focal(int32_t nom, int32_t denom);
void dng_set_iso(int32_t value);
void dng_set_wbgain(int32_t gain_r_n, int32_t gain_r_d, int32_t gain_g_n, int32_t gain_g_d, int32_t gain_b_n, int32_t gain_b_d);
void dng_set_datetime(char *datetime, char *subsectime);

#endif // __CHDK_DNG_H_
