// Temporary, until the one from Tobias will arrive
#ifndef _zebra_h_
#define _zebra_h_
#include "dryos.h"
int liveview_display_idle();
int get_global_draw();
void NotifyBox( int timeout, char* fmt, ...);
/**
 * @brief Set custom movie cropmark in LiveView
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 */
void set_movie_cropmarks(int x, int y, int w, int h);
/**
 * @brief Reset LiveView movie cropmark
 */
void reset_movie_cropmarks();
#endif //_zebra_h_
