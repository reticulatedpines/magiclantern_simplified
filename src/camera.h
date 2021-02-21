/** \file
 * Camera engine interface.
 *
 * WARNING: This file is perhaps the most dangerous.
 * Nothing in here is supported by Canon and has all been
 * reverse engineered from random dumps of memory addresses
 * and firmware code.
 *
 * IT MIGHT NOT BE RIGHT.
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

#ifndef _camera_h_
#define _camera_h_

#include "compiler.h"

struct camera_engine
{
        uint32_t                off_0x00;
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint32_t                off_0x20;
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        uint32_t                off_0x2c;
        uint32_t                off_0x30;
        uint32_t                off_0x34;
        uint32_t                off_0x38;
        uint32_t                hdmi_connected; // off_0x3c;
        uint32_t                off_0x40;
        uint32_t                off_0x44;
        uint32_t                off_0x48;
        uint32_t                off_0x4c;
        uint32_t                off_0x50;
        uint32_t                off_0x54;
        uint32_t                off_0x58;
        uint32_t                off_0x5c;
        uint32_t                off_0x60;
        uint32_t                off_0x64;
        uint32_t                off_0x68;
        uint32_t                off_0x6c;

        // 0x121 == LCD, 0x120 == composite video
        uint32_t                av_jack; // off_0x70;
        uint32_t                off_0x74;
        uint32_t                off_0x78;
        uint32_t                off_0x7c;
};

SIZE_CHECK_STRUCT( camera_engine, 0x80 );
extern struct camera_engine camera_engine;


/** FA functions can be called to do stuff */
extern void FA_StartLiveView( void );

#endif
