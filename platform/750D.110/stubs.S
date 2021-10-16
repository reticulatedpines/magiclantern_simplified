/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the EOS 750D.
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
THUMB_FN(0xfe0ccfec,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
ARM32_FN(0x800072c0,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0x80000f78,  create_init_task)                     /* low-level DryOS initialization, this is thunk */
THUMB_FN(0xfe0cd214,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
    /* DATA_PTR(    0x44B0,  current_interrupt)                 /* no idea at this moment */
DATA_PTR(    0x44F4,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(    0x1a9c,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(    0x1e44,  task_create)                          /* used to start TaskMain, GuiMainTask etc */

/** File I/O **/
THUMB_FN(0xfe443c9e, _FIO_CreateFile)
THUMB_FN(0xfe443c28, _FIO_OpenFile)
THUMB_FN(0xfe443ef4,  FIO_CloseFile)
THUMB_FN(0xfe443d74, _FIO_ReadFile)
THUMB_FN(0xfe443e86, _FIO_WriteFile)
THUMB_FN(0xfe443de4,  FIO_SeekSkipFile)
THUMB_FN(0xfe443fe0, _FIO_GetFileSize)
THUMB_FN(0xfe443d08, _FIO_RemoveFile)
THUMB_FN(0xfe444364, _FIO_RenameFile)                       /* FIO_RenameFile_Success */
THUMB_FN(0xfe4444e2, _FIO_CreateDirectory)
THUMB_FN(0xfe4448aa, _FIO_FindFirstEx)
THUMB_FN(0xfe44493e,  FIO_FindNextEx)
THUMB_FN(0xfe4449c0,  FIO_FindClose)
THUMB_FN(0xfe4445d2,  FIO_Flush)

/** Debug messages **/
THUMB_FN(     0x268,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Eventprocs (call by name) **/
THUMB_FN(0xfe445e02,  call)                                 /* many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xfe31b7d6,  SetHPTimerNextTick)                   /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xfe31b786,  SetHPTimerAfterNow)                   /* from error message */
THUMB_FN(0xfe44a3bc,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xfe44a42e,  CancelTimer)                          /* CancelRefreshTimer, CancelTimer */

/** Memory allocation **/
THUMB_FN(    0x10ea, _AllocateMemory)                       /* via _alloc_dma_memory maybe 0xfe8aa156? */
THUMB_FN(    0x1284, _FreeMemory)                           /* via _free_dma_memory */
THUMB_FN(     0xf94,  GetMemoryInformation)                 /* via _AllocateMemory */
THUMB_FN(     0xf68,  GetSizeOfMaxRegion)                   /* via _AllocateMemory */
THUMB_FN(0x800056ec, _alloc_dma_memory)
THUMB_FN(0x80005718, _free_dma_memory)
THUMB_FN(0xfe50b4c0, _malloc)                               /* called before 'can't malloc' */
THUMB_FN(0xfe50b4f6, _free)                                 /* called later on the same buffer */

/** Memory info **/
THUMB_FN(0Xfe1f61fc,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xfe1d7a54,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xfe15c96a,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xfe1f6294,  sysmem_info)                          /* System Memory Information */

/** Semaphores **/
THUMB_FN(    0x2fbc,  create_named_semaphore)
THUMB_FN(    0x30b2,  take_semaphore)                       /* GetEndGuiInitState Take/Give  */
THUMB_FN(    0x312a,  give_semaphore)


/** Misc **/
THUMB_FN(0xfe465374,  uart_printf)
THUMB_FN(0x8000469c,  vsnprintf)                            /* from dryosdebugmsg, found via '!!! ERROR !!!' print on uart */

THUMB_FN(0xfe173f10,  gui_init_end)                         /* EndGuiInit */
THUMB_FN(0xfe0f924c,  gui_main_task)
/** General FEATURE_VRAM_RGBA stubs **/
THUMB_FN(0xfe2181d4, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message

/*
 * Pointer to first Canon RGBA buffer (GUI).
 * structure starts at 2d38c - easy to find from InitializeScreen
 *
 * 0x0  unknown
 * 0x4  is_initialized
 * 0x8  _rgb_vram_info (rgb buffer)
 * 0xc   bmp_vram_info.yuv1
 * 0x10  bmp_vram_info.yuv2
 * 0x14  bmp_vram_info.back_vram
 *
 * Structure is missing WinsysWait_v (vsync) semaphore as compared to D7+
 */
DATA_PTR(    0x2D394, _rgb_vram_info);
DATA_PTR(    0x2D398,  bmp_vram_info)
