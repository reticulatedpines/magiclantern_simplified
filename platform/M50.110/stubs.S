/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the EOS M50.
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
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xE00400FC,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE0578D64,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE0143CFC,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE05777D0,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE05778A4,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040224,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE05598FA,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xE0545EE2,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** File I/O **/
THUMB_FN(0xE05465E6, _FIO_CreateFile)
THUMB_FN(0xE0546534, _FIO_OpenFile)
THUMB_FN(0xE0546D56,  FIO_CloseFile)
THUMB_FN(0xE05469B6, _FIO_ReadFile)
THUMB_FN(0xE0546B42, _FIO_WriteFile)
THUMB_FN(0xE0546A82,  FIO_SeekSkipFile)                     // guess: Search?!
THUMB_FN(0xE054691C, _FIO_RemoveFile)
THUMB_FN(0xE0547926, _FIO_RenameFile)
THUMB_FN(0xE0547DB0, _FIO_CreateDirectory)
THUMB_FN(0xE0548212, _FIO_FindFirstEx)                      // FirstEnt
THUMB_FN(0xE0548318,  FIO_FindNextEx)                       // NextEnt
THUMB_FN(0xE05483DA,  FIO_FindClose)                        // CloseEnt
THUMB_FN(0xE0547F9A,  FIO_Flush)                            // to be called after FIO_CloseFile?
THUMB_FN(0xE0546EF2, _FIO_GetFileSize64)                    // AcqSize; different in D8 cameras - returns 64 bit size in struct of lower/upper 32bit part.

/** Bitmap **/
DATA_PTR(    0xFC20,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE0577FD0,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE12FA534,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE05722C6 ,  call)                                 /* many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xE0732BDE,  SetHPTimerNextTick)                   /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xE0732B7E,  SetHPTimerAfterNow)                   /* from error message */
THUMB_FN(0xE054CF14,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xE054CFCA,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xE055B094, _AllocateMemory)
THUMB_FN(0xE055B24C, _FreeMemory)
THUMB_FN(0xE055AF1E,  GetMemoryInformation)
THUMB_FN(0xE055AEF2,  GetSizeOfMaxRegion)
THUMB_FN(0xE055AAF4, _alloc_dma_memory)
THUMB_FN(0xE055AB28, _free_dma_memory)
THUMB_FN(0xE03C242E, _malloc)
THUMB_FN(0xE03C2466, _free)

/** Memory info **/
THUMB_FN(0xE009DEB8,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE0146FD6,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xE00F1C18,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xE00F1CB0,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE055A8B4,  uart_printf)
THUMB_FN(0xE0096060,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */

/** Semaphores **/
THUMB_FN(0xe0577b60,  create_named_semaphore)
THUMB_FN(0xe0577cd6,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xe0577c5c,  take_semaphore)                       /* SystemIF::KerSem.c   */

/**
 * RGBA / Compositor functions and structures
 */

/** General FEATURE_VRAM_RGBA stubs **/
THUMB_FN(0xe00d6404, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xFC10, winsys_sem);                           //Used in RefreshVrmsSurface around XimrExe call

/*
 * Pointer to first Canon RGBA buffer (GUI). On M50 this is set up by 
 * VDrv_InitializeScreen function, 2nd param is saved there.
 */
DATA_PTR(    0xFC1C, _rgb_vram_info);
