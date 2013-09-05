// Temporary, until the one from Tobias will arrive
#ifndef _zebra_h_
#define _zebra_h_
#include "dryos.h"
int liveview_display_idle();
int get_global_draw();
void NotifyBox( int timeout, char* fmt, ...);
void set_movie_cropmarks(int x, int y, int w, int h);
void reset_movie_cropmarks();
#endif //_zebra_h_
