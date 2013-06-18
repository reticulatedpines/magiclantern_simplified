/*
 * Copyright (C) 2013 Magic Lantern Team
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

#ifndef _lv_rec_h_
#define _lv_rec_h_

#include "raw.h"

/* file footer data */
typedef struct
{
    unsigned char magic[4];
    unsigned short xRes;
    unsigned short yRes;
    unsigned int frameSize;
    unsigned int frameCount;
    unsigned int frameSkip;
    unsigned int sourceFpsx1000;
    unsigned int reserved3;
    unsigned int reserved4;
    struct raw_info raw_info;
} lv_rec_file_footer_t;

#endif
