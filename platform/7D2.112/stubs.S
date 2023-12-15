/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.2 firmware for the 7D Mark II.
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


// Startup
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xfe0a0af8,  pre_cstart_func) // needed to easily patch for compacting firmware_entry and cstart regions
THUMB_FN(0xfe0a0b18,  cstart)
ARM32_FN(0x8000a3a4,  bzero32)
THUMB_FN(0x80000f78,  create_init_task)
THUMB_FN(0xfe0a1980,  init_task)
THUMB_FN(0xfe0a01fe,  dcache_clean) // not used during 7D2 startup

// Tasks
// 80001358 is get_task_info_by_id(), useful for finding task related addresses
// 80001632 is get_task_by_id()
// 0x2795c is first_task
//DATA_PTR(   0x277b8,  interrupt_level)
DATA_PTR(   0x277bc,  current_interrupt) // See fe0a153c(), this refs both current_interrupt and interrupt_level.
                                         // It's read from d4011000, this was consistent on 3 Digic 6 cams I checked.
DATA_PTR(   0x277dc,  current_task)
THUMB_FN(    0x19b0,  msleep)
THUMB_FN(    0x1a60,  task_create) // unthunked Thumb at 1a60
DATA_PTR(   0x27990,  task_max)

// File I/O
THUMB_FN(0xfe265d0e, _FIO_CreateFile)
THUMB_FN(0xfe265c88, _FIO_OpenFile)
THUMB_FN(0xfe265fc4,  FIO_CloseFile)
THUMB_FN(0xfe265e04, _FIO_ReadFile)
THUMB_FN(0xfe265f40, _FIO_WriteFile)
THUMB_FN(0xfe265e84,  FIO_SeekSkipFile)
THUMB_FN(0xfe266104, _FIO_GetFileSize)
THUMB_FN(0xfe265d88, _FIO_RemoveFile)
THUMB_FN(0xfe2663b2, _FIO_RenameFile) // FIO_RenameFile_Success
THUMB_FN(0xfe26643c, _FIO_CreateDirectory)
THUMB_FN(0xfe266712, _FIO_FindFirstEx)
THUMB_FN(0xfe2667b2,  FIO_FindNextEx)
THUMB_FN(0xfe266848,  FIO_FindClose) // This is FIO_FindCloseEx really, but it seems similar enough
THUMB_FN(0xfe266500,  FIO_Flush)

// Debug messages
THUMB_FN(     0x204,  DryosDebugMsg)

// Eventprocs (call by name)
THUMB_FN(0xfe1009ba,  call)

// GUI timers
THUMB_FN(0xfe103d66,  SetHPTimerNextTick)   // same "worker" function as SetHPTimerAfterNow
THUMB_FN(0xfe103d16,  SetHPTimerAfterNow)   // from error message
THUMB_FN(0xfe10cb00,  SetTimerAfter)        // from error message
THUMB_FN(0xfe10cb72,  CancelTimer)          // CancelRefreshTimer, CancelTimer

    // Memory allocation
THUMB_FN(     0xea0, _AllocateMemory)       // via _alloc_dma_memory
THUMB_FN(    0x1140, _FreeMemory)           // via _free_dma_memory
THUMB_FN(     0xd4e,  GetMemoryInformation) // via _AllocateMemory
THUMB_FN(     0xd22,  GetSizeOfMaxRegion)   // via _AllocateMemory
THUMB_FN(0x800087d0, _alloc_dma_memory)
THUMB_FN(0x800087fc, _free_dma_memory)
THUMB_FN(0xfe10dce4, _malloc)               // called before "can't malloc"
THUMB_FN(0xfe10dd1a, _free)                 // called later on the same buffer

// Memory info
THUMB_FN(0xfe0a1ad0,  malloc_info)          // Malloc Information
THUMB_FN(0xfe27e2cc,  memmap_info)          // Exception vector, DRYOS system memory etc
THUMB_FN(0xfe194a26,  smemShowFix)          // Common Lower, Common Upper etc
THUMB_FN(0xfe0a1b68,  sysmem_info)          // System Memory Information

// Misc
THUMB_FN(0xfe0a1458,  uart_printf)
THUMB_FN(0x80006934,  vsnprintf)            // from dryosdebugmsg, found via ""!!! ERROR !!!"'" print on uart

// New stubs

// More tasks
DATA_PTR(    0x279cc,  task_dispatch_hook)
THUMB_FN(0x80001722,  task_trampoline)
THUMB_FN(0x80001358, _get_task_info_by_id)

// Recursive locks
THUMB_FN(    0x1d36,  CreateRecursiveLock)  // "KernelDry/KerMisc.c"
THUMB_FN(    0x2c30,  AcquireRecursiveLock) // "KernelDry/KerRLock.c", used in WINSYS_CreateController()
THUMB_FN(    0x2cfe,  ReleaseRecursiveLock) // "KernelDry/KerRLock.c"

// Semaphores
THUMB_FN(    0x15c4,  create_named_semaphore)
THUMB_FN(    0x16ba,  take_semaphore)         // GetEndGuiInitState Take/Give
THUMB_FN(    0x1732,  give_semaphore)

// GUI
DATA_PTR(   0x2efac,  gui_task_list)          // via WINSYS_CreateController()
DATA_PTR(   0x27e64,  gui_main_struct)        // structure used by gui_main_task
THUMB_FN(0xfe157bd6,  SetGUIRequestMode)      // by debug message
THUMB_FN(0xfe0b5c98,  gui_main_task)          // task_create("GuiMainTask"...
THUMB_FN(0xfe0b5fbc,  gui_massive_event_loop) // various "GUICMD_"* strings
THUMB_FN(0xfe0b5c30,  gui_enqueue_message)    // via string "warning! QueueLength="
THUMB_FN(0xfe0b6478,  gui_init_end)           // EndGuiInit
THUMB_FN(0xfe0b5e3e,  GUI_Control)            // string GUI_Control

// Dialog API
THUMB_FN(0xfe35d674,  dialog_redraw)   // DebugMsg "StartPlayErrorApp". Called just before assigning 0 to one of variables.
THUMB_FN(0xfe2cbfc4,  dialog_set_property_str) // DebugMsg "***** UpdateStroboZoomGuidance", called with char* buffer as 3rd param.

// GUI timers
THUMB_FN(0xfe3f5e10,  CancelDateTimer) // by debug message

// Queue
THUMB_FN(    0x28ec,  msg_queue_create)  // easy to spot in create_gui_main_task aka GUI_initialize
THUMB_FN(    0x29b4,  msg_queue_receive) // used on gui queue in gui_main_task
THUMB_FN(    0x2b08,  msg_queue_post)    // used in gui_enqueue_message
THUMB_FN(    0x2b3e,  msg_queue_count)   // educated guess based on R/200D. Also xref from fn with "[TASK ERROR] GetNumOfEvent"

// Memory
// SRM general
THUMB_FN(0xfe101c26,  AllocateMemoryResource)               // Assert m_pfAllocMemoryCBR
THUMB_FN(0xfe101c54,  AllocateContinuousMemoryResource)     // Assert m_pfContAllocMemoryCBR
THUMB_FN(0xfe101c82,  FreeMemoryResource)                   // Assert m_pfFreeMemoryCBR
// SRM specific
THUMB_FN(0xfe0bcb64,  SRM_AllocateMemoryResourceFor1stJob)  // via debug message with function name (after call). Function uses RscMgr strings
THUMB_FN(0xfe0be728,  SRM_FreeMemoryResourceFor1stJob)      // used in functions related to deleting Skelton jobs
// MemorySuite, MemoryChunk
THUMB_FN(0xfe102d30,  ConcatMemorySuite)                    // Via debug msg with fn name (after call). Not used by ML, but utilizes functions below:
THUMB_FN(0xfe10273a,  GetFirstChunkFromSuite)               // In ConcatMemorySuite(), First function called.
THUMB_FN(0xfe102cea,  GetNextMemoryChunk)                   // In ConcatMemorySuite(), function called in first loop with param2 passed as 1st arg
THUMB_FN(0xfe102418,  GetMemoryAddressOfMemoryChunk)        // In ConcatMemorySuite(), uses param2 from GetNextMemoryChunk() as own param1
THUMB_FN(0xfe1021f6,  CreateMemoryChunk)                    // In ConcatMemorySuite(), just after calls to GetMemoryAddressOfMemoryChunk(); also called from CreateMemorySuite()
THUMB_FN(0xfe1026ae,  AddMemoryChunk)                       // In ConcatMemorySuite(), just after calls to CreateMemoryChunk()
THUMB_FN(0xfe102bbe,  CreateMemorySuite)                    // One of just a few xrefs of CreateMemoryChunk(); String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xfe1028ca,  DeleteMemorySuite)                    // DebugMsg "DeleteMemorySuite 0x%x"

// Uncategorized
DATA_PTR(   0x331bc,  sd_device)         // From function that references "SD_DeviceCreate:" TODO: check if struct changed like in R
THUMB_FN(0xfe3efd2c,  _LoadCalendarFromRTC) // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xfe3eac0a,  ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c)

// Properties
THUMB_FN(0xfe0f81ce,  prop_register_slave)  // Assert "pRegisterSlave"
THUMB_FN(0xfe0f829e, _prop_request_change)  // Assert "pRequestChange"; two refs, one sets a variable to 1 near the end, the other 0.
                                            // We want 1.
THUMB_FN(0xfe0f86cc, _prop_cleanup)         // DebugMsg "ChangeCBR(...". Last call in those functions.

// CFn
// Doesn't seem to have these, 5D style menus where Custom Functions are a normal menu
//THUMB_FN(0x, GetCFnData)            // "GetCFnData"
//THUMB_FN(0x, SetCFnData)            // "SetCFnData"

//
// RGBA / Compositor functions and structures
//

// General FEATURE_VRAM_RGBA stubs
THUMB_FN(0xfe3b84f2, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message

/*
 * Pointer to first Canon RGBA buffer (GUI).
 * structure starts at 2f100 - easy to find from InitializeScreen
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
DATA_PTR(    0x2f108, _rgb_vram_info);
DATA_PTR(    0x2f10c,  bmp_vram_info)

// Wrong on purpose
DATA_PTR(       0x0,  LCD_Palette)                          // D6 has no indexed RGB buffers.

// DMA
// dma_memcpy() doesn't work on this cam, it hits the "Nothing DMA ch now!" case,
// perhaps this system isn't initialised?
//THUMB_FN(0xfe3b921c,  dma_memcpy) // BLTDMA
//
// These all exist...  but on Omar:
//THUMB_FN(0x8000626a EngDrvOut);
//THUMB_FN(0x800062a4 EngDrvBits);
//THUMB_FN(0x800062ba EngDrvOuts);
//THUMB_FN(0x80006298 shamem_read);
//THUMB_FN(0x800063c2 StartEDmac);
//THUMB_FN(0x80006498 edmac_set_addr);
//THUMB_FN(0x800064c2 edmac_set_size);
//THUMB_FN(0x80006bfe ConnectWriteEDmac);
//THUMB_FN(0x80006c10 ConnectReadEDmac);
//THUMB_FN(0x80006c42 RegisterEDmacCompleteCBR);

// App handlers
//
// Those can usually be found by looking at function address passed to
// CreateDialogBox_wrapper / CreateDialogBox, just after some
// debug message
THUMB_FN(0xfe2dba32,  PlayMain_handler)                     // msg: "StartPlayMain"
THUMB_FN(0xfe4fd15a,  ShootOlcApp_handler)                  // msg: "StartShootOlcApp PushPalette(%d)"
THUMB_FN(0xfe40a6be,  LiveViewApp_handler)                  // msg: "StartLiveViewApp(%#x)"
//THUMB_FN(0x,  LiveViewShutterApp_handler)         // msg: "LiveViewShutterApp"
THUMB_FN(0xfe422052,  PlayMovieGuideApp_handler)            // msg: "StartPlayMovieGuideApp"

// See R180 stubs.S for details.
//DATA_PTR( 0x2BD60,  PlayMain_dialog)                      // in StartPlayMain(), 0x2bd54 + 0xc
//DATA_PTR( 0x35298,  ShootOlcApp_dialog)                   // in StartShootOlcApp()
DATA_PTR(   0x31154,  LiveViewApp_dialog)                   // in StartLiveViewApp()
//DATA_PTR( 0x35a58,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR( 0x3042c,  PlayMovieGuideApp_dialog)             // in StartPlayMovieGuideApp()
