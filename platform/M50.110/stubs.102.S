/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.2 firmware for the EOS M50.
 *
 */
/*
 * Copyright (C) 2019 Magic Lantern Team
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
THUMB_FN(0xE00400FC,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE0578CD0,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE0143D00,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE057773C,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE0577810,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040224,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE0559866,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xE0545E4E,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** File I/O **/
THUMB_FN(0xE0546552, _FIO_CreateFile)
THUMB_FN(0xE05464A0, _FIO_OpenFile)
THUMB_FN(0xE0546CC2,  FIO_CloseFile)
THUMB_FN(0xE0546922, _FIO_ReadFile)
THUMB_FN(0xE0546AAE, _FIO_WriteFile)
THUMB_FN(0xE05469EE,  FIO_SeekSkipFile)                     // guess: Search?!
THUMB_FN(0xE0546E5E, _FIO_GetFileSize)                      // AcqSize
THUMB_FN(0xE0546888, _FIO_RemoveFile)
THUMB_FN(0xE0547892, _FIO_RenameFile)
THUMB_FN(0xE0547D1C, _FIO_CreateDirectory)
THUMB_FN(0xE054817E, _FIO_FindFirstEx)                      // FirstEnt
THUMB_FN(0xE0548284,  FIO_FindNextEx)                       // NextEnt
THUMB_FN(0xE0548346,  FIO_FindClose)                        // CloseEnt
THUMB_FN(0xE0547F06,  FIO_Flush)                            // to be called after FIO_CloseFile?

/** Bitmap **/
DATA_PTR(    0xFC20,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE0577F3C,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE0085F70,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE0572232,  call)                                 /* many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xE0732B1E,  SetHPTimerNextTick)                   /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xE0732ABE,  SetHPTimerAfterNow)                   /* from error message */
THUMB_FN(0xE054CE80,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xE054CF36,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xE055B000, _AllocateMemory)
THUMB_FN(0xE055B1B8, _FreeMemory)
THUMB_FN(0xE055AE8A,  GetMemoryInformation)
THUMB_FN(0xE055AE5E,  GetSizeOfMaxRegion)
THUMB_FN(0xE055AA60, _alloc_dma_memory)
THUMB_FN(0xE055AA94, _free_dma_memory)
THUMB_FN(0xE03C2386, _malloc)
THUMB_FN(0xE03C23BE, _free)

/** Memory info **/
THUMB_FN(0xE00F1C1C,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xE009DEB8,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE0146FDA,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xE00F1CB4,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE055A820,  uart_printf)
THUMB_FN(0xE0096060,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */
