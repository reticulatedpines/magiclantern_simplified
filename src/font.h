/** \file
 * Internal bitmap font representation
 *
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _font_h_
#define _font_h_

struct font
{
        unsigned        height;
        unsigned        width;
#ifdef CONFIG_STATIC_FONTS
        unsigned        bitmap[];
#else
        unsigned*       bitmap;
#endif
};

struct sfont
{
        unsigned        height;
        unsigned        width;
        unsigned*       bitmap;
};


extern struct font font_small;
extern struct font font_med;
extern struct font font_large;

extern struct sfont font_small_shadow;
extern struct sfont font_med_shadow;
extern struct sfont font_large_shadow;

#endif
