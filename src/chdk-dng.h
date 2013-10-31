#ifndef __CHDK_DNG_H_
#define __CHDK_DNG_H_

void dng_set_framerate(int fpsx1000);
void dng_set_thumbnail_size(int width, int height);

void dng_set_framerate_rational(int nom, int denom);
void dng_set_shutter(int nom, int denom);
void dng_set_aperture(int nom, int denom);
void dng_set_camname(char *name);
void dng_set_focal(int nom, int denom);
void dng_set_iso(int value);

#endif // __CHDK_DNG_H_
