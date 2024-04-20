/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the 2000D
 *
 */
/*
 * Copyright (C) 2021 Magic Lantern Team
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

#include <stub.h>

// kitor: functions in early ram regions seem to match 1:1 1300D.110

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )                        // 0xFE0C0000
NSTUB(0xfe0c3a38,  cstart)                                  //
NSTUB(   0x29898,  bzero32)                                 // called by cstart() rom
NSTUB(    0x5254,  create_init_task)                        // must be called from ram
NSTUB(0xFE129718,  init_task)                               // passed as arg to create_init_task, look for dmSetup

NSTUB(    0x3780,  msleep)
NSTUB(   0x55820,  bmp_vram_info)
NSTUB(    0x38FC,  task_create)

NSTUB(0xfe11f3c8,  DryosDebugMsg)
NSTUB(0xfe2bbbc8,  vsnprintf)                               // from DryosDebugMsg
