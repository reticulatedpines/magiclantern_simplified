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

/* Warning: there are some ARM32 stubs, not only thumb! */

/** Startup **/
NSTUB(  ROMBASEADDR,  firmware_entry)
THUMB_FN(0xfe0ccfec,  cstart)                               // calls bzero32 and create_init_task(..., init_task, ...)
ARM32_FN(0x800072c0,  bzero32)                              // zeros out a data structure
THUMB_FN(0x80000f78,  create_init_task)                     // low-level DryOS initialization
THUMB_FN(0xfe0cd214,  init_task)                            // USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc

/** Tasks **/
    DATA_PTR(    0x44D4,  current_interrupt)                    // WRONG!!! no idea how to find
DATA_PTR(    0x44F4,  current_task)                         // from task_create; pointer to the current task structure
THUMB_FN(    0x1a9c,  msleep)                               // argument is always multiple of 10
THUMB_FN(    0x1e44,  task_create)                          // used to start TaskMain, GuiMainTask etc
DATA_PTR(    0x4678,  task_max)

/** File I/O **/
THUMB_FN(0xfe443c9e, _FIO_CreateFile)
THUMB_FN(0xfe443c28, _FIO_OpenFile)
THUMB_FN(0xfe443ef4,  FIO_CloseFile)
THUMB_FN(0xfe443d74, _FIO_ReadFile)
THUMB_FN(0xfe443e86, _FIO_WriteFile)
THUMB_FN(0xfe443de4,  FIO_SeekSkipFile)
THUMB_FN(0xfe443fe0, _FIO_GetFileSize)
THUMB_FN(0xfe443d08, _FIO_RemoveFile)
THUMB_FN(0xfe444364, _FIO_RenameFile)                       // FIO_RenameFile_Success
THUMB_FN(0xfe4444e2, _FIO_CreateDirectory)
THUMB_FN(0xfe4448aa, _FIO_FindFirstEx)
THUMB_FN(0xfe44493e,  FIO_FindNextEx)
THUMB_FN(0xfe4449c0,  FIO_FindClose)
THUMB_FN(0xfe4445d2,  FIO_Flush)

/** Debug messages **/
THUMB_FN(     0x268,  DryosDebugMsg)                        // lots of debug messages; format string is third argument

/** Eventprocs (call by name) **/
THUMB_FN(0xfe445e02,  call)                                 // many functions called by name (lv_start, lv_stop etc)

/** GUI timers **/
THUMB_FN(0xfe31b7d6,  SetHPTimerNextTick)                   // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xfe31b786,  SetHPTimerAfterNow)                   // from error message
THUMB_FN(0xfe44a3bc,  SetTimerAfter)                        // from error message
THUMB_FN(0xfe44a42e,  CancelTimer)                          // CancelRefreshTimer, CancelTimer

/** Memory allocation **/
THUMB_FN(    0x10ea, _AllocateMemory)                       // via _alloc_dma_memory
THUMB_FN(    0x1284, _FreeMemory)                           // via _free_dma_memory
THUMB_FN(     0xf94,  GetMemoryInformation)                 // via _AllocateMemory
THUMB_FN(     0xf68,  GetSizeOfMaxRegion)                   // via _AllocateMemory
THUMB_FN(0x800056ec, _alloc_dma_memory)
THUMB_FN(0x80005718, _free_dma_memory)
THUMB_FN(0xfe50b4c0, _malloc)                               // called before "can't malloc"
THUMB_FN(0xfe50b4f6, _free)                                 // called later on the same buffer

/** Memory info **/
THUMB_FN(0Xfe1f61fc,  malloc_info)                          // Malloc Information
THUMB_FN(0xfe1d7a54,  memmap_info)                          // Exception vector, DRYOS system memory etc
THUMB_FN(0xfe15c96a,  smemShowFix)                          // Common Lower, Common Upper etc
THUMB_FN(0xfe1f6294,  sysmem_info)                          // System Memory Information

/** Misc **/
THUMB_FN(0xfe465374,  uart_printf)
THUMB_FN(0x8000469c,  vsnprintf)                            // from dryosdebugmsg, found via ""!!! ERROR !!!"'" print on uart

/** New stubs **/

/** Startup **/
DATA_PTR(    0x2bbfc,  additional_version)
/* kitor: Please note that this points at '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(    0x46b4,  task_dispatch_hook)
THUMB_FN(0x80001722,  task_trampoline)
/*
 * kitor: task_create calls internal function (80000b88) which set up basic
 * task structure, and part of that is call to task_trampoline_caller (80001746)
 * (it is followed by check of caller return value).
 * task_trampoline_caller will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0x80001358, _get_task_info_by_id)
/*
 * kitor: via extask (fe1d76d0). While on 5D3 it was called directly, now helper
 * is used. Find extask via string formats. Trace variables holding task id,
 * you will quickly find where it is verified (this is the mentioned helper).
 * In the helper code you will find call to our stub.
 */

/** Recursive locks **/
THUMB_FN(    0x3262,  CreateRecursiveLock)                  // "KernelDry/KerMisc.c"
THUMB_FN(    0x3448,  AcquireRecursiveLock)                 // "KernelDry/KerRLock.c", used in WINSYS_CreateController()
THUMB_FN(    0x3516,  ReleaseRecursiveLock)                 // "KernelDry/KerRLock.c"

/** Semaphores **/
THUMB_FN(    0x2fbc,  create_named_semaphore)
THUMB_FN(    0x30b2,  take_semaphore)                       // GetEndGuiInitState Take/Give
THUMB_FN(    0x312a,  give_semaphore)

/** GUI **/
DATA_PTR(   0x2d2a4,  gui_task_list)                        // via WINSYS_CreateController()
DATA_PTR(    0x4b18,  gui_main_struct)                      // structure used by gui_main_task
THUMB_FN(0xfe4421ec,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xfe0f924c,  gui_main_task)                        // task_create("GuiMainTask"...
THUMB_FN(0xfe31ba34,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xfe0f91e4,  gui_enqueue_message)                  // via string "warning! QueueLength="
THUMB_FN(0xfe173f10,  gui_init_end)                         // EndGuiInit
THUMB_FN(0xfe0f93f2,  GUI_Control)                          // string GUI_Control

/** Dialog API **/
THUMB_FN(0xfe452136,  dialog_redraw)                        // DebugMsg "StartPlayErrorApp". Called just before assigning 0 to one of variables.
THUMB_FN(0xfe453a24,  dialog_set_property_str)              // DebugMsg "***** UpdateStroboZoomGuidance", called with char* buffer as 3rd param.

/** GUI timers **/
THUMB_FN(0xfe654534,  CancelDateTimer)                      // by debug message

/** Queue **/
THUMB_FN(    0x2ce8,  msg_queue_create)                     // easy to spot in create_gui_main_task aka GUI_initialize
THUMB_FN(    0x2db0,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(    0x2f04,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(    0x2f3a,  msg_queue_count)                      // educated guess based on R/200D. Also xref from fn with "[TASK ERROR] GetNumOfEvent"

/** Memory **/
// SRM general
THUMB_FN(0xfe5486ae,  AllocateMemoryResource)               // Assert m_pfAllocMemoryCBR
THUMB_FN(0xfe5486dc,  AllocateContinuousMemoryResource)     // Assert m_pfContAllocMemoryCBR
THUMB_FN(0xfe548782,  FreeMemoryResource)                   // Assert m_pfFreeMemoryCBR
// SRM specific
THUMB_FN(0xfe3f73ba,  SRM_AllocateMemoryResourceFor1stJob)  // via debug message with function name (after call). Function uses RscMgr strings
THUMB_FN(0xfe3f970a,  SRM_FreeMemoryResourceFor1stJob)      // used in functions related to deleting Skelton jobs
// MemorySuite, MemoryChunk
THUMB_FN(0xfe44167e,  ConcatMemorySuite)                    // Via debug msg with fn name (after call). Not used by ML, but utilizes functions below:
THUMB_FN(0xfe440dbc,  GetFirstChunkFromSuite)               // In ConcatMemorySuite(), First function called.
THUMB_FN(0xfe441638,  GetNextMemoryChunk)                   // In ConcatMemorySuite(), function called in first loop with param2 passed as 1st arg
THUMB_FN(0xfe440bbc,  GetMemoryAddressOfMemoryChunk)        // In ConcatMemorySuite(), uses param2 from GetNextMemoryChunk() as own param1
THUMB_FN(0xfe44076a,  CreateMemoryChunk)                    // In ConcatMemorySuite(), just after calls to GetMemoryAddressOfMemoryChunk(); also called from CreateMemorySuite()
THUMB_FN(0xfe440d30,  AddMemoryChunk)                       // In ConcatMemorySuite(), just after calls to CreateMemoryChunk()
THUMB_FN(0xfe441232,  CreateMemorySuite)                    // One of just a few xrefs of CreateMemoryChunk(); String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xfe440f4c,  DeleteMemorySuite)                    // DebugMsg "DeleteMemorySuite 0x%x"

/** Uncategorized **/
DATA_PTR(   0x2e06c,  sd_device)                            // From function that references "SD_DeviceCreate:" TODO: check if stryct changed like in R
THUMB_FN(0xfe614f8c,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xfe613866,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xfe445136,  prop_register_slave)                  // Assert "pRegisterSlave"
THUMB_FN(0xfe445206, _prop_request_change)                  // Assert "pRequestChange"; 2nd (almost identical) is at 0xfe44574a but uses different struct address - matched to struct from prop_register_slave()
THUMB_FN(0xfe445718, _prop_cleanup)                         // DebugMsg "ChangeCBR(...". Last call in those functions.

/**
 * RGBA / Compositor functions and structures
 */

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

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D6 has no indexed RGB buffers.

/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper / WINSYS_CreateDialogBox, just after some
 * debug message */
THUMB_FN(0xfe410e0a,  PlayMain_handler)                     // msg: "StartPlayMain"
THUMB_FN(0xfe575c58,  ShootOlcApp_handler)                  // msg: "StartShootOlcApp PushPalette(%d)"
THUMB_FN(0xfe3d5ecc,  LiveViewApp_handler)                  // msg: "StartLiveViewApp(%#x)"
THUMB_FN(0xfe69c5d0,  LiveViewShutterApp_handler)           // msg: "LiveViewShutterApp"
THUMB_FN(0xfe31022e,  PlayMovieGuideApp_handler)            // msg: "StartPlayMovieGuideApp"
