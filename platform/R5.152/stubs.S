/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.5.2 firmware for the EOS R5
 *
 */
/*
 * Copyright (C) 2022 Magic Lantern Team
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
NSTUB(  ROMBASEADDR,  firmware_entry)                       // also known as 2nd stage loader, boot2 or DryOS loader
THUMB_FN(0xe08132d8,  dcache_clean)                         // call from firmware_entry(); in assembly: loop with MCR p15 c7,c10,1; DSB
THUMB_FN(0xe08133ac,  icache_invalidate)                    // call from firmware_entry(); in assembly: loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB
THUMB_FN(0xE01000ec,  cstart)                               // call from firmware_entry(); calls bzero32 and create_init_task(..., init_task, ...)
THUMB_FN(0xe08158b4,  bzero32)                              // call from cstart(); zeros out a data structure
THUMB_FN(0xe0272a60,  create_init_task)                     // call from cstart() only on core0; low-level DryOS initialization
THUMB_FN(0xe010021e,  init_task)                            // 2nd param to create_init_task(); xrefs strings like USER_MEM size checking, dmSetup, termDriverInit...

/** Startup - Digic X specials **/
THUMB_FN(0xE0005B79,  fixup_boot_memory)                    // called from 1st stage loader, just before decision where to jump next.

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    // DryosDebugMsg() prints "TASK Name:Interrupt" and "Vector:0x%l", argument to 2nd print.
DATA_PTR(    0x1028,  current_task)                         // from task_create(); pointer to the current task structure
DATA_PTR(    0x4100,  task_max)                             // entry in SysConfig struct
THUMB_FN(0xe07fc502,  msleep)                               // argument is always multiple of 10
THUMB_FN(0xe07f25b6,  task_create)                          // used to start TaskMain, GuiMainTask etc

/** File I/O **/
// kitor: ordered by ROM address, as FIO block usually holds the same in-ROM order.
THUMB_FN(0xe07ece36, _FIO_OpenFile)                         // Storage::Storage.c Open
THUMB_FN(0xe07ecef6, _FIO_CreateFile)                       // Storage::Storage.c Create
THUMB_FN(0xe07ed068, _FIO_RemoveFile)                       // Storage::Storage.c Remove
THUMB_FN(0xe07ed0fc, _FIO_ReadFile)                         // Storage::Storage.c Read
THUMB_FN(0xe07ed51e,  FIO_SeekSkipFile)                     // Storage::Storage.c Search
THUMB_FN(0xe07ed5e4, _FIO_WriteFile)                        // Storage::Storage.c Write
THUMB_FN(0xe07edd5a,  FIO_CloseFile)                        // Storage::Storage.c Close
THUMB_FN(0xe07edf72, _FIO_GetFileSize64)                    // Storage::Storage.c AcqSize
THUMB_FN(0xe07ee9a4, _FIO_RenameFile)                       // Storage::Storage.c Rename
THUMB_FN(0xe07eedc8, _FIO_CreateDirectory)                  // Storage::Storage.c CreateDir
THUMB_FN(0xe07ef0da,  FIO_Flush)                            // Storage::Storage.c Flush
THUMB_FN(0xe07ef2bc, _FIO_FindFirstEx)                      // Storage::Storage.c FirstEnt
THUMB_FN(0xe07ef3cc,  FIO_FindNextEx)                       // Storage::Storage.c NextEnt
THUMB_FN(0xe07ef48c,  FIO_FindClose)                        // Storage::Storage.c CloseEnt

/** Bitmap **/
DATA_PTR(    0xFDFC,  bmp_vram_info)                        // first YUV VRAM buffer, used right after DISP_SetUpdateOSDVram

/** Debug messages **/
THUMB_FN(0xe0814a9c,  DryosDebugMsg)                        // Easy to catch in init task: "InitializeIntercom", "CreateSequencer"

/** Eventprocs (call by name) **/
THUMB_FN(0xe080064a, call);

/** GUI timers **/
THUMB_FN(0xe0546a70,  SetHPTimerAfterNow)                   // System::HPTimer.c
THUMB_FN(0xe0546af6,  SetHPTimerNextTick)                   // System::HPTimer.c; next function; same "worker" as SetHPTimerAfterNow()
THUMB_FN(0xe0802290,  SetTimerAfter)                        // SystemIF::Timer1m.c
THUMB_FN(0xe0802346,  CancelTimer)                          // CancelRefreshTimer, CancelPlayTimer

/** Memory allocation **/
THUMB_FN(0xe0813c7c, _AllocateMemory)                       // NG AllocateMemory
THUMB_FN(0xe0813e34, _FreeMemory)                           // uses the same heap
THUMB_FN(0xe0813b06,  GetMemoryInformation)                 // called from _AllocateMemory()
THUMB_FN(0xe0813ada,  GetSizeOfMaxRegion)                   // called from _AllocateMemory()
THUMB_FN(0xe08024f0, _alloc_dma_memory)                     // allocates 64 bytes more, aligns at 32 and sets the uncacheable bit
THUMB_FN(0xe08024f6, _free_dma_memory)                      // paired with the above; clears the uncacheable bit and frees the raw pointer
THUMB_FN(0xe0a7dcb0, _malloc)                               // Just before conditional uart_printf("can't malloc")
THUMB_FN(0xe0a7dce8, _free)                                 // After malloc() in ROM

/** Memory info **/
THUMB_FN(0xe01442e0,  res_memshow)                          // DIGIX X replacement of smemShowFix()
THUMB_FN(0xe02dfaf4,  memmap_info)                          // Exception vector, DRYOS system memory etc
THUMB_FN(0xe03130ec,  malloc_info)                          // Malloc Information */
THUMB_FN(0xe0313184,  sysmem_info)                          // System Memory Information */

/** Misc **/
THUMB_FN(0xe07fd960,  uart_printf)                          // in init task, prints "%s READY\n"
THUMB_FN(0xe0123b2c,  vsnprintf)                            // called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s

/** Startup **/
// 16d0c + 0xd80 -> is this static location, or we have to rewrite for dynamic access?
//DATA_PTR(    0x,  additional_version)                     // via GUI_GetFirmVersion(); Points at '\n' char in char* with FW version

/** Tasks **/
DATA_PTR(    0x413c,  task_dispatch_hook)
THUMB_FN(0xe011ecb8,  task_trampoline)
/*
 * kitor: task_create calls internal function (e040fa1c) which set up basic
 * task structure, and part of that is call to prepare_task_trampoline (e011ecd8)
 * (it is followed by check of caller return value).
 * prepare_task_trampoline will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0xe0490934, _get_task_info_by_id)                  // via extask

/** Recursive locks **/
THUMB_FN(0xe0814470,  CreateRecursiveLock)                  // SystemIF::KerMisc.c
THUMB_FN(0xe08003e0,  AcquireRecursiveLock)                 // SystemIF::KerRLock.c
THUMB_FN(0xe08004aa,  ReleaseRecursiveLock)                 // SystemIF::KerRLock.c

/** Semaphores **/
THUMB_FN(0xe0814600,  create_named_semaphore)               // SystemIF::KerSem.c, called from init task with "PU0Wait", "PU1Wait", "CCInit"... as 1st arg
THUMB_FN(0xe0814776,  give_semaphore)                       // SystemIF::KerSem.c, follow xrefs to result of any of create_named_semaphore()
THUMB_FN(0xe08146fc,  take_semaphore)                       // SystemIF::KerSem.c, follow xrefs like in give_semaphore()

/** GUI **/
DATA_PTR(    0x546c,  gui_main_struct)                      // via string "QueueClearFlg Clear", accessed by that function
DATA_PTR(   0x19394,  gui_task_list)                        // via WINSYS_CreateController()
THUMB_FN(0xe0457d80,  gui_main_task)                        // via task_create("GuiMainTask"...
THUMB_FN(0xe0457eb6,  gui_enqueue_message)                  // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
THUMB_FN(0xe05bbdb4,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xe07d31fe,  SetGUIRequestMode)                    // by debug message. New style (RP+)
// gui_init_end and GUI_Control DNE. Implemented via function_overrides.

/** Dialog API **/
THUMB_FN(0xe07d6484,  dialog_redraw)                        // via xrefs to "pDialog->pSignature == m_pcSignature"
THUMB_FN(0xe07d0bf4,  dialog_set_property_str)              // call with "RF-TEST" as 3rd arg

/** GUI timers **/
THUMB_FN(0xe0a9a392,  StartDateTimer)                       // by debug message
THUMB_FN(0xe0a9a44e,  CancelDateTimer)                      // by debug message

/** Queue **/
THUMB_FN(0xe07f1ab4,  msg_queue_create)                     // SystemIF::KerQueue.c; used in UiOpe_Initialize() (function that creates gui_main_task)
THUMB_FN(0xe07f1b78,  msg_queue_receive)                    // SystemIF::KerQueue.c; used in gui_main_task()
THUMB_FN(0xe07f1cd6,  msg_queue_post)                       // SystemIF::KerQueue.c; used in gui_enqueue_message()
THUMB_FN(0xe07f1d0c,  msg_queue_count)                      // via string "warning! QueueLength"

/** Memory **/
THUMB_FN(0xe090b968,  AllocateMemoryResource)               // missing debug string on R5, matched via xrefs to R
THUMB_FN(0xe090b996,  AllocateContinuousMemoryResource)     // missing debug string on R5, matched via xrefs to R
THUMB_FN(0xe090b9c4, FreeMemoryResource)                    // missing debug string on R5, matched via xrefs to R
//THUMB_FN(0xe07f4dbc,  ConcatMemorySuite)                  // Via debug msg with fn name (after call). Not used by ML, but utilizes functions below:
THUMB_FN(0xe07f4550,  GetFirstChunkFromSuite)               // In ConcatMemorySuite(), First function called.
THUMB_FN(0xe07f4d78,  GetNextMemoryChunk)                   // In ConcatMemorySuite(), function called in first loop with param2 passed as 1st arg
THUMB_FN(0xe07f4302,  GetMemoryAddressOfMemoryChunk)        // In ConcatMemorySuite(), uses param2 from GetNextMemoryChunk() as own param1
THUMB_FN(0xe07f3fde,  CreateMemoryChunk)                    // In ConcatMemorySuite(), just after calls to GetMemoryAddressOfMemoryChunk(); also called from CreateMemorySuite()
THUMB_FN(0xe07f44cc,  AddMemoryChunk)                       // In ConcatMemorySuite(), just after calls to CreateMemoryChunk()
THUMB_FN(0xe07f49ae,  CreateMemorySuite)                    // One of just a few xrefs of CreateMemoryChunk(); String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xe07f471e,  DeleteMemorySuite)                    // DebugMsg "DeleteMemorySuite 0x%x"
// kitor: disabled using CONFIG_MEMORY_SRM_NOT_WORKING
// DIGIC X does not use RscMgr anymore, those are dummy functions that just assert!
//THUMB_FN(0xe081baa0,  SRM_AllocateMemoryResourceFor1stJob)
//THUMB_FN(0xe081baaa,  SRM_FreeMemoryResourceFor1stJob)

/** Uncategorized **/
// From function that references "SD_DeviceCreate:"; looks like now it is a part of dynamic struct
DATA_PTR(       0x0,  sd_device)                            // shouldn't matter unless "keep after format" feature is enabled
THUMB_FN(0xe0afbd7c,  _LoadCalendarFromRTC)                 // Used in GUI_SetStartMovieRecDate();
THUMB_FN(0xe0c41ae4,  ErrForCamera_handler)                 // via debug msg: "DlgErrForCamera.c TERMINATE_WINSYS"

/** Properties **/
THUMB_FN(0xe07fd378, _prop_cleanup)                         // missing debug string on R5, matched via xrefs to R
THUMB_FN(0xe07fcdce,  prop_register_slave)                  // missing debug string on R5, matched via xrefs to R
THUMB_FN(0xe07fce9c, _prop_request_change)                  // missing debug string on R5, matched via xrefs to R


/** General FEATURE_VRAM_RGBA stubs **/
//THUMB_FN(0xe0257452, VMIX_TransferRectangleToVram)        // via debug message; not needed - left for references below
//THUMB_FN(0xe022b7fe, RENDERER_InitializeScreen)           // via debug message; not needed - left for references below
DATA_PTR(   0x194f0, winsys_sem);                           // Used in VMIX_TransferRectangleToVram() as first arg of take_semaphore()
DATA_PTR(   0x194bc, _rgb_vram_info);                       // in RENDERER_InitializeScreen(), written just after "JediWrapperClearSurface vram:%d" message
THUMB_FN(0xe01a4b38, XimrExe);                              // In VMIX_TransferRectangleToVram(), just before "XimrExe" debug message

/**
 * Things needed for CONFIG_COMPOSITOR_XCM.
 *
 * R5 uses two layers (GUI, focus overlays). WINSYS code swaps pointer in
 * WINSYS structure (one we know as _rgb_vram_info) to select which layer to
 * draw.
 *
 * With FEATURE_COMPOSITOR_XCM we ask XCM via XCM_GetSourceSurface() to give us
 * pointer to layer 0, which is more reliable.
 */
THUMB_FN(0xe01775e4, XCM_GetSourceSurface)                  // by debug message
//THUMB_FN(0xe017712e, XCM_MakeContext)                     // by debug message. Not used in code, left for reference below.
DATA_PTR(  0x14ffb0, _pXCM);                                // param1 to XCM_MakeContext() when called from VMIX_TransferRectangleToVram()

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)

/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox(), usually after a debug message.
 */
THUMB_FN(0xe0778070,  PlayMain_handler)                     // via debug string "PlayMain_StartPlayMain"
THUMB_FN(0xe0b1b66e,  ShootOlcApp_handler)                  // via debug string "StartShootOlcApp PushPalette(%d)"
THUMB_FN(0xe06153b0,  LiveViewApp_handler)                  // via debug string "StartLiveViewApp(%#x)"
//THUMB_FN(0xe0b76700,  LiveViewShutterApp_handler)         // via debug string "StartLiveViewShutterApp"
THUMB_FN(0xe9999999,  PlayMovieGuideApp_handler)            // PlayMovieGuideApp DNE on R5, used for comparison so it is safe-ish.

/**
 * Code that creates / stops given task from above, also saves dialog pointer
 * somewhere. This seems to be a better way to detect if given app is running.
 *
 * Pointers to result of WINSYS_CreateDialogBox() call in given function.
 */
//DATA_PTR( 0x1719c,  PlayMain_dialog)                      // via debug string "PlayMain_StartPlayMain"
//DATA_PTR( 0x22770,  ShootOlcApp_dialog)                   // via debug string "StartShootOlcApp PushPalette(%d)"
DATA_PTR(   0x16610,  LiveViewApp_dialog)                   // via debug string "StartLiveViewApp(%#x)"
//DATA_PTR( 0x21c60,  LiveViewShutterApp_dialog)            // via debug string "StartLiveViewShutterApp"
//DATA_PTR(      0x,  PlayMovieGuideApp_dialog)             // PlayMovieGuideApp DNE on R5

/**
 * UNUSED but referenced elsewhere
 **/
//THUMB_FN(0xe07d6458, WINSYS_CreateDialogBox)              // Equiv. of `CreateDialogBox()`. Forces layer ID to 0 (GUI)
//THUMB_FN(0xe07d6204, WINSYS_CreateDialogBoxEx)            // Actual logic. Takes one more argument than CreateDialogBox() (layer ID)
