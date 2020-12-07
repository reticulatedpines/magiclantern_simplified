/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.2.0 firmware for the EOS R.
 *
 */
/*
 * Copyright (C) 2018 Magic Lantern Team
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
THUMB_FN(0xE059A32C,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE011E0D8,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE03149CC,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE0314AA0,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040224,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE0580C1A,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xE0550566,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** File I/O **/
THUMB_FN(0xE0572966, _FIO_CreateFile)
THUMB_FN(0xE05728B4, _FIO_OpenFile)
THUMB_FN(0xE05730EE,  FIO_CloseFile)
THUMB_FN(0xE0572D3E, _FIO_ReadFile)
THUMB_FN(0xE0572EDA, _FIO_WriteFile)
THUMB_FN(0xE0572E1A,  FIO_SeekSkipFile)                     // guess: Search?!
THUMB_FN(0xE057328A, _FIO_GetFileSize)                      // AcqSize
THUMB_FN(0xE0572CA4, _FIO_RemoveFile)
THUMB_FN(0xE0573CC0, _FIO_RenameFile)
THUMB_FN(0xE0574148, _FIO_CreateDirectory)
THUMB_FN(0xE05745AA, _FIO_FindFirstEx)                      // FirstEnt
THUMB_FN(0xE05746A2,  FIO_FindNextEx)                       // NextEnt
THUMB_FN(0xE0574764,  FIO_FindClose)                        // CloseEnt
THUMB_FN(0xE0574332,  FIO_Flush)                            // to be called after FIO_CloseFile?

/** Bitmap **/
DATA_PTR(    0xFB2C,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE059A504,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE00BD692,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE058A286,  call)                                 /* used in ElectricCurrentFirm */

/** GUI timers **/
THUMB_FN(0xE03A9A9E,  SetHPTimerNextTick)                   /* next function; same "worker" as SetHPTimerAfterNow */
THUMB_FN(0xE03A9A3E,  SetHPTimerAfterNow)                   /* System::HPTimer.c */
THUMB_FN(0xE0571D30,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xE0571DE6,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xE0589A28, _AllocateMemory)                       /* NG AllocateMemory */
THUMB_FN(0xE0589BE0, _FreeMemory)                           /* uses the same heap */
THUMB_FN(0xE05898B2,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xE0589886,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xE0589488, _alloc_dma_memory)                     /* allocates 64 bytes more, aligns at 32 and sets the uncacheable bit */
THUMB_FN(0xE05894BC, _free_dma_memory)                      /* paired with the above; clears the uncacheable bit and frees the raw pointer */
THUMB_FN(0xE02C8468, _malloc)                               /* see "can't malloc" */
THUMB_FN(0xE02C84A0, _free)                                 /* next function; paired with the above */

/** Memory info **/
THUMB_FN(0xE00CDE50,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xE008CB28,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE015A03E,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xE00CDEE8,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE0584530,  uart_printf)
THUMB_FN(0xE006459C,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */
