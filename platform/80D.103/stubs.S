/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.3 firmware for the EOS 80D.
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

/* Warning: there are some ARM32 stubs, not only thumb! */

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xfe0d314c,  pre_cstart_func) // needed to easily patch for compacting firmware_entry and cstart regions
THUMB_FN(0xfe0d316c,  cstart)
ARM32_FN(0xfe3fc21c,  bzero32)
ARM32_FN(0xfe3fbc78,  create_init_task)
THUMB_FN(0xfe0d3384,  init_task)

/** Tasks **/
//DATA_PTR(  0x44D0,  interrupt_level)
DATA_PTR(    0x44D4,  current_interrupt) // See fe0cd12c() which uses both current_interrupt and interrupt_level.
                                         // It's read from d4011000, this was consistent on 3 Digic 6 cams I checked.
DATA_PTR(    0x44D4,  current_interrupt)
DATA_PTR(    0x44F4,  current_task)
THUMB_FN(     0xED0,  msleep)
THUMB_FN(     0xFF0,  task_create)
DATA_PTR(    0x46a0,  task_max)

/** File I/O **/
THUMB_FN(0xfe4827ba, _FIO_CreateFile)
THUMB_FN(0xfe482744, _FIO_OpenFile)
THUMB_FN(0xfe482a10,  FIO_CloseFile)
THUMB_FN(0xfe482890, _FIO_ReadFile)
THUMB_FN(0xfe4829a2, _FIO_WriteFile)
THUMB_FN(0xfe482900,  FIO_SeekSkipFile)
THUMB_FN(0xfe482afc, _FIO_GetFileSize)
THUMB_FN(0xfe482824, _FIO_RemoveFile)
THUMB_FN(0xfe482e80, _FIO_RenameFile)                       // FIO_RenameFile_Success
THUMB_FN(0xfe482ffe, _FIO_CreateDirectory)
THUMB_FN(0xfe4833c6, _FIO_FindFirstEx)
THUMB_FN(0xfe48345a,  FIO_FindNextEx)
THUMB_FN(0xfe4834dc,  FIO_FindClose)
THUMB_FN(0xfe4830ee,  FIO_Flush)

/** Debug messages **/
THUMB_FN(0x800035E0,  DryosDebugMsg)

/** Eventprocs (call by name) **/
THUMB_FN(0xfe48422e,  call)

/** GUI timers **/
THUMB_FN(0xfe4b3ac2,  SetHPTimerNextTick)                   // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xfe4b3a72,  SetHPTimerAfterNow)                   // from error message
THUMB_FN(0xfe483fe4,  SetTimerAfter)                        // from error message
THUMB_FN(0xfe48405a,  CancelTimer)                          // from error message

/** Memory allocation **/
THUMB_FN(     0x4D6, _AllocateMemory)
THUMB_FN(     0x670, _FreeMemory)
THUMB_FN(     0x380,  GetMemoryInformation)
THUMB_FN(     0x354,  GetSizeOfMaxRegion)
THUMB_FN(0x800062b8, _alloc_dma_memory)
THUMB_FN(0x800062e4, _free_dma_memory)
THUMB_FN(0xfe60d130, _malloc)                               // called before "can't malloc"
THUMB_FN(0xfe60d166, _free)                                 // called later on the same buffer

/** Memory info **/
THUMB_FN(0xFE2DB8AC,  malloc_info)
THUMB_FN(0xfe2ab650,  memmap_info)                          // Exception vector, DRYOS system memory etc
THUMB_FN(0xFE15F746,  smemShowFix)
THUMB_FN(0xFE2DB944,  sysmem_info)

/** Misc **/
THUMB_FN(0xfe483d00,  uart_printf)
THUMB_FN(0x800054AC,  vsnprintf)

/** New stubs **/

/** Startup **/
DATA_PTR(    0x76bd,  additional_version)
/* kitor: Please note that this points at '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(    0x46dc,  task_dispatch_hook)
THUMB_FN(0x8000172c,  task_trampoline)
THUMB_FN(0x8000135c, _get_task_info_by_id)

/** Recursive locks **/
THUMB_FN(    0x267a,  CreateRecursiveLock)                  // "KernelDry/KerMisc.c"
THUMB_FN(    0x2900,  AcquireRecursiveLock)                 // "KernelDry/KerRLock.c", used in WINSYS_CreateController()
THUMB_FN(    0x29c8,  ReleaseRecursiveLock)                 // "KernelDry/KerRLock.c"

/** Semaphores **/
THUMB_FN(    0x23e0,  create_named_semaphore)
THUMB_FN(    0x24d6,  take_semaphore)                       // GetEndGuiInitState Take/Give
THUMB_FN(    0x254e,  give_semaphore)

/** GUI **/
DATA_PTR(    0xc190,  gui_task_list)                        // via WINSYS_CreateController()
DATA_PTR(    0x4b24,  gui_main_struct)                      // structure used by gui_main_task
THUMB_FN(0xfe466592,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xfe18a2cc,  gui_main_task)                        // task_create("GuiMainTask"...
THUMB_FN(0xfe30cf00,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xfe18a264,  gui_enqueue_message)                  // via string "warning! QueueLength="
THUMB_FN(0xfe47a734,  gui_init_end)                         // EndGuiInit
THUMB_FN(0xfe18a472,  GUI_Control)                          // string GUI_Control

/** Dialog API **/
THUMB_FN(0xfe490fdc,  dialog_redraw)                        // DebugMsg "StartPlayErrorApp". Called just before assigning 0 to one of variables.
THUMB_FN(0xfe4847d0,  dialog_set_property_str)              // DebugMsg "***** UpdateStroboZoomGuidance", called with char* buffer as 3rd param.

/** GUI timers **/
THUMB_FN(0xfe651f44,  CancelDateTimer)                      // by debug message

/** Queue **/
THUMB_FN(    0x2120,  msg_queue_create)                     // easy to spot in create_gui_main_task aka GUI_initialize
THUMB_FN(    0x21e0,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(    0x2334,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(    0x236a,  msg_queue_count)                      // educated guess based on R/200D. Also xref from fn with "[TASK ERROR] GetNumOfEvent"

/** Memory **/
// SRM general
THUMB_FN(0xfe583e66,  AllocateMemoryResource)               // Assert m_pfAllocMemoryCBR
THUMB_FN(0xfe583e94,  AllocateContinuousMemoryResource)     // Assert m_pfContAllocMemoryCBR
THUMB_FN(0xfe583f3a,  FreeMemoryResource)                   // Assert m_pfFreeMemoryCBR
// SRM specific
THUMB_FN(0xfe45fef2,  SRM_AllocateMemoryResourceFor1stJob)  // via debug message with function name (after call). Function uses RscMgr strings
THUMB_FN(0xfe46233a,  SRM_FreeMemoryResourceFor1stJob)      // used in functions related to deleting Skelton jobs
// MemorySuite, MemoryChunk
THUMB_FN(0xfe477e3a,  ConcatMemorySuite)                    // Via debug msg with fn name (after call). Not used by ML, but utilizes functions below:
THUMB_FN(0xfe477578,  GetFirstChunkFromSuite)               // In ConcatMemorySuite(), First function called.
THUMB_FN(0xfe477df4,  GetNextMemoryChunk)                   // In ConcatMemorySuite(), function called in first loop with param2 passed as 1st arg
THUMB_FN(0xfe477378,  GetMemoryAddressOfMemoryChunk)        // In ConcatMemorySuite(), uses param2 from GetNextMemoryChunk() as own param1
THUMB_FN(0xfe476f26,  CreateMemoryChunk)                    // In ConcatMemorySuite(), just after calls to GetMemoryAddressOfMemoryChunk(); also called from CreateMemorySuite()
THUMB_FN(0xfe4774ec,  AddMemoryChunk)                       // In ConcatMemorySuite(), just after calls to CreateMemoryChunk()
THUMB_FN(0xfe4779ee,  CreateMemorySuite)                    // One of just a few xrefs of CreateMemoryChunk(); String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xfe477708,  DeleteMemorySuite)                    // DebugMsg "DeleteMemorySuite 0x%x"

/** Uncategorized **/
DATA_PTR(    0xcca0,  sd_device)                            // From function that references "SD_DeviceCreate:" TODO: check if stryct changed like in R
THUMB_FN(0xfe620c5e,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xfe6a1412,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xfe484c0a,  prop_register_slave)                  // Assert "pRegisterSlave"
THUMB_FN(0xfe484cda, _prop_request_change)                  // Assert "pRequestChange"; 2nd (almost identical) is at 0xfe44574a but uses different struct address - matched to struct from prop_register_slave()
THUMB_FN(0xfe4851ec, _prop_cleanup)                         // DebugMsg "ChangeCBR(...". Last call in those functions.

/** CFn **/
THUMB_FN(0xfe37115a, GetCFnData)                            // "GetCFnData", like 200D
THUMB_FN(0xfe37128a, SetCFnData)                            // "SetCFnData"

/**
 * RGBA / Compositor functions and structures
 */

/** General FEATURE_VRAM_RGBA stubs **/
THUMB_FN(0xfe22b2fc, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xc28c, _rgb_vram_info);
DATA_PTR(    0xc288, winsys_sem);                           // Used in RefreshVrmsSurface around XimrExe call

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D6 has no indexed RGB buffers.


/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper / WINSYS_CreateDialogBox, just after some
 * debug message */
THUMB_FN(0xfe41b9ca,  PlayMain_handler)                     // msg: "StartPlayMain"
THUMB_FN(0xfe65071a,  ShootOlcApp_handler)                  // msg: "StartShootOlcApp PushPalette(%d)"
THUMB_FN(0xfe391998,  LiveViewApp_handler)                  // msg: "StartLiveViewApp(%#x)"
//THUMB_FN(0xfe72190d,  LiveViewShutterApp_handler)         // msg: "LiveViewShutterApp"
THUMB_FN(0xfe32f154,  PlayMovieGuideApp_handler)            // msg: "StartPlayMovieGuideApp"

// See R180 stubs.s for details.
//DATA_PTR(  0xb7c0,  PlayMain_dialog)                      // in StartPlayMain(), 0x2bd54 + 0xc
//DATA_PTR(  0xe75c,  ShootOlcApp_dialog)                   // in StartShootOlcApp()
DATA_PTR(    0xe4a0,  LiveViewApp_dialog)                   // in StartLiveViewApp()
//DATA_PTR( 0x11b88,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR(  0xf1e4,  PlayMovieGuideApp_dialog)             // in StartPlayMovieGuideApp()
