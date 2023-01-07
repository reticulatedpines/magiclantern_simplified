/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.5.0 firmware for the EOS R6
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

/* All functions are Thumb. */


/** Startup **/
NSTUB(  ROMBASEADDR,  firmware_entry)
THUMB_FN(0xE01000ec,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE07d9244,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE0276578,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE07D2FC4,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE07D3098,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE010021e,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
// DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure. also in  */
THUMB_FN(0xe07bffce,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0XE07BC6AE,  task_create)                          /* used to start TaskMain, GuiMainTask etc */
// NSTUB( 0x40CC, task_max)

/** Debug messages **/
THUMB_FN(0xE07D842C,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE02A3E62,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

THUMB_FN(0xE0921984, _malloc)                               /* see [cpuprof], "can't malloc" */
//THUMB_FN(0xE021C834, _free)                                 /* next function; paired with the above */

THUMB_FN(0xE0123258,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */



THUMB_FN(0xE07D8106,  give_semaphore)                       /* SystemIF::KerSem.c, values 0x133 and 0x136, just after take_semaphore   */
THUMB_FN(0xE07D808C,  take_semaphore)                       /* SystemIF::KerSem.c, values 0x162 and 0x167   */


/** General FEATURE_VRAM_RGBA stubs **/
THUMB_FN(0xe01a1c04, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0x0x18394+4, winsys_sem);                           //Used in RefreshVrmsSurface around XimrExe call

/*
 * Pointer to first Canon RGBA buffer (GUI). On R this is set up by
 * RENDERER_InitializeScreen function, just after MARVs are created.
 */
DATA_PTR(    0x18354+0x10, _rgb_vram_info);


