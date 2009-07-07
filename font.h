/** \file
 * Internal bitmap font representation
 *
 */
#ifndef _font_h_
#define _font_h_

struct font
{
	unsigned	height;
	unsigned	width;
	unsigned 	bitmap[];
};

extern struct font font_small, font_med, font_large;

#endif
