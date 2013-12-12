#ifndef __CHDK_DNG_H_
#define __CHDK_DNG_H_

void dng_set_framerate(int fpsx1000);
void dng_set_thumbnail_size(int width, int height);

void dng_set_framerate_rational(int nom, int denom);
void dng_set_shutter(int nom, int denom);
void dng_set_aperture(int nom, int denom);
void dng_set_camname(char *str);
void dng_set_camserial(char *str);
void dng_set_description(char *str);
void dng_set_lensmodel(char *str);
void dng_set_focal(int nom, int denom);
void dng_set_iso(int value);
void dng_set_wbgain(float gain_r_n, float gain_r_d, float gain_g_n, float gain_g_d, float gain_b_n, float gain_b_d);

#endif // __CHDK_DNG_H_
