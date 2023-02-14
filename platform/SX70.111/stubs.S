/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the PowerShot SX70.
 *
 */
/*
 * Copyright (C) 2023 Magic Lantern Team
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
THUMB_FN(0xE0040104,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE05338A8,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE01729DC,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE038804C,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE0388120,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE004022C,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE05197fA,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xE038B12A,  task_create)                          /* used to start TaskMain, GuiMainTask etc */
NSTUB(       0x40D0,  task_max)                             /* 6th field in sysconfig struct */

/** Debug messages **/
THUMB_FN(0xE0532B04,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Eventprocs (call by name) **/
THUMB_FN(0xE050D056,  call)                                 /* used in ElectricCurrentFirm */

THUMB_FN(0xE027D19A, _malloc)                               /* see "can't malloc" */
THUMB_FN(0xE027D1D2, _free)                                 /* next function; paired with the above */

/** Semaphores **/
THUMB_FN(0xE0532614,  create_named_semaphore)
THUMB_FN(0xE053278A,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xE0532710,  take_semaphore)                       /* SystemIF::KerSem.c   */


THUMB_FN(0xE0110BD0, XimrExe);                              // In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xFB88, winsys_sem);                           // Used in RefreshVrmsSurface around XimrExe call
DATA_PTR(    0xFB38, _rgb_vram_info);

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D6+ do use palletes to draw GUI, but it is hw rendered into RGBA

THUMB_FN(0xE0523DAC, uart_printf)

THUMB_FN(0xE01AADB4, smemShowFix)
THUMB_FN(0xE00A47E8, memmap_info)
THUMB_FN(0xE00E7f98, malloc_info)
THUMB_FN(0xE00E8030, sysmem_info)
