#ifndef _cropmarks_h_
#define _cropmarks_h_
/**
 * @brief Set custom movie cropmarks in LiveView
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @addtogroup cropmarks
 */
void set_movie_cropmarks(int x, int y, int w, int h);
/**
 * @brief Reset LiveView movie cropmarks
 */
void reset_movie_cropmarks();
#endif