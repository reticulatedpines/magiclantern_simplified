/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.5.2 firmware for the EOS R5
 *
 */
/*
 * Copyright (C) 2022 Magic Lantern Team
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

/* All functions are Thumb. */


/** Startup **/
NSTUB(  ROMBASEADDR,  firmware_entry)                       // also known as 2nd stage loader, boot2 or DryOS loader
THUMB_FN(0xe08132d8,  dcache_clean)                         // call from firmware_entry(); in assembly: loop with MCR p15 c7,c10,1; DSB
THUMB_FN(0xe08133ac,  icache_invalidate)                    // call from firmware_entry(); in assembly: loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB
THUMB_FN(0xE01000ec,  cstart)                               // call from firmware_entry(); calls bzero32 and create_init_task(..., init_task, ...)
THUMB_FN(0xe08158b4,  bzero32)                              // call from cstart(); zeros out a data structure
THUMB_FN(0xe0272a60,  create_init_task)                     // call from cstart() only on core0; low-level DryOS initialization
THUMB_FN(0xe010021e,  init_task)                            // 2nd param to create_init_task(); xrefs strings like USER_MEM size checking, dmSetup, termDriverInit...

/** Startup - Digic X specials **/
THUMB_FN(0xE0005B79,  reinit_autoexec_memory)               // called from 1st stage loader, just before decision where to jump next.

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    // DryosDebugMsg() prints "TASK Name:Interrupt" and "Vector:0x%l", argument to 2nd print.
THUMB_FN(0xe07fc502,  msleep)                               // argument is always multiple of 10
THUMB_FN(0xe07f25b6,  task_create)                          // used to start TaskMain, GuiMainTask etc

/** Debug messages **/
THUMB_FN(0xe0814a9c,  DryosDebugMsg)                        // Easy to catch in init task: "InitializeIntercom", "CreateSequencer"

/** Memory allocation **/
THUMB_FN(0xe0a7dcb0, _malloc)                               // Just before conditional uart_printf("can't malloc")
THUMB_FN(0xe0a7dce8, _free)                                 // After malloc() in ROM


THUMB_FN(0xe07fd960,  uart_printf)                          // in init task, prints "%s READY\n"
THUMB_FN(0xe0123b2c,  vsnprintf)                            // called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s

/** Semaphores **/
THUMB_FN(0xe0814600,  create_named_semaphore)               // SystemIF::KerSem.c, called from init task with "PU0Wait", "PU1Wait", "CCInit"... as 1st arg
THUMB_FN(0xe0814776,  give_semaphore)                       // SystemIF::KerSem.c, follow xrefs to result of any of create_named_semaphore()
THUMB_FN(0xe08146fc,  take_semaphore)                       // SystemIF::KerSem.c, follow xrefs like in give_semaphore()

/** Misc **/

THUMB_FN(0xe080064a, call);

/** General FEATURE_VRAM_RGBA stubs **/
//THUMB_FN(0xe0257452, VMIX_TransferRectangleToVram)        // via debug messagel not needed - left for references below
//THUMB_FN(0xe022b7fe, RENDERER_InitializeScreen)           // via debug message; not needed - left for references below
DATA_PTR(   0x194f0, winsys_sem);                           // Used in VMIX_TransferRectangleToVram() as first arg of take_semaphore()
DATA_PTR(   0x194bc, _rgb_vram_info);                       // in RENDERER_InitializeScreen(), written just after "JediWrapperClearSurface vram:%d" message
THUMB_FN(0xe01a4b38, XimrExe);                              // In VMIX_TransferRectangleToVram(), just before "XimrExe" debug message
