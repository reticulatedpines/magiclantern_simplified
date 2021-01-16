/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.8.0 firmware for the EOS R.
 *
 */
/*
 * Copyright (C) 2020 Magic Lantern Team
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
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xE0040104,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE056313c,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE011E120,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE02C6D40,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE02C6E14,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE004022C,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE0549952,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0XE05172FA,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** File I/O **/
THUMB_FN(0xE053BAC2, _FIO_CreateFile)
THUMB_FN(0xE053BA10, _FIO_OpenFile)
THUMB_FN(0xe053c24a,  FIO_CloseFile)
THUMB_FN(0xe053be9a, _FIO_ReadFile)
THUMB_FN(0xe053c036, _FIO_WriteFile)
THUMB_FN(0xe053bf76,  FIO_SeekSkipFile)                     // guess: Search?!
THUMB_FN(0xe053c3e6, _FIO_GetFileSize)                      // AcqSize
THUMB_FN(0xe053be00, _FIO_RemoveFile)
THUMB_FN(0xe053ce1c, _FIO_RenameFile)
THUMB_FN(0xe053d2a4, _FIO_CreateDirectory)
THUMB_FN(0xe053d706, _FIO_FindFirstEx)                      // FirstEnt
THUMB_FN(0xe053d7fe,  FIO_FindNextEx)                       // NextEnt
THUMB_FN(0xe053d8c0,  FIO_FindClose)                        // CloseEnt
THUMB_FN(0xe053d48e,  FIO_Flush)                            // to be called after FIO_CloseFile?

/** Bitmap **/
DATA_PTR(    0xFB2C,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE0563314,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE13192D8,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE0553072,  call)                                 /* used in ElectricCurrentFirm */

/** GUI timers **/
THUMB_FN(0xe039d062,  SetHPTimerNextTick)                   /* next function; same "worker" as SetHPTimerAfterNow */
THUMB_FN(0xe039d002,  SetHPTimerAfterNow)                   /* System::HPTimer.c */
THUMB_FN(0xe053ae8c,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xe053af42,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xe0552814, _AllocateMemory)                       /* NG AllocateMemory */
THUMB_FN(0xe05529cc, _FreeMemory)                           /* uses the same heap */
THUMB_FN(0xe055269e,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xe0552672,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xe0552274, _alloc_dma_memory)                     /* allocates 64 bytes more, aligns at 32 and sets the uncacheable bit */
THUMB_FN(0xe05522a8, _free_dma_memory)                      /* paired with the above; clears the uncacheable bit and frees the raw pointer */
THUMB_FN(0xe021c7fc, _malloc)                               /* see "can't malloc" */
THUMB_FN(0xe021c834, _free)                                 /* next function; paired with the above */

/** Memory info **/
THUMB_FN(0xe00cde5c,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xe008c974,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xe015a14a,  smemShowFix)                          /* Common Lower, Common Upper etc */ //?
THUMB_FN(0xe00cdef4,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xe054d31c,  uart_printf)
THUMB_FN(0xe006463c,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */
