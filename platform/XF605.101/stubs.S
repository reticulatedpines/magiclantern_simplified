/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the XF605
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
NSTUB(  ROMBASEADDR,  firmware_entry)           // also known as 2nd stage loader, boot2 or DryOS loader
THUMB_FN(0xe01003f0,  dcache_clean)             // call from firmware_entry(); in assembly: loop with MCR p15 c7,c10,1; DSB
THUMB_FN(0xe01004c4,  icache_invalidate)        // call from firmware_entry(); in assembly: loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB
// This is the "real" cstart addr, in that DryOS runs code from there.
// But, unique to XF605 so far, it's copied there from ROM, as part of firmware_entry().
// We'll use the ROM version to read from for our patching.
//THUMB_FN(0x1e400000,  cstart)                   // call from firmware_entry(); calls bzero32 and create_init_task(..., init_task, ...)
THUMB_FN(0xe0236408,  cstart)                   // call from firmware_entry(); calls bzero32 and create_init_task(..., init_task, ...)
THUMB_FN(0x1ea376b8,  bzero32)                  // call from cstart(); zeros out a data structure
THUMB_FN(0x1e6fc108,  create_init_task)         // call from cstart() only on cpu0; low-level DryOS initialization
THUMB_FN(0x1e400132,  init_task)                // 2nd param to create_init_task(); xrefs strings like USER_MEM size checking, dmSetup, termDriverInit...

/** Startup - Digic X specials **/
THUMB_FN(0xe0005c50,  reinit_autoexec_memory)   // called from 1st stage loader, just before decision where to jump next.

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)        // DryosDebugMsg() prints "TASK Name:Interrupt" and "Vector:0x%l", argument to 2nd print.
DATA_PTR(    0x1028,  current_task)             // from task_create(); pointer to the current task structure
DATA_PTR(    0x4104,  task_max)
THUMB_FN(0x1ea13566,  msleep)                   // argument is always multiple of 10
THUMB_FN(0x1ea12fba,  task_create)              // used to start TaskMain, GuiMainTask etc

/** File I/O **/
// kitor: ordered by ROM address, as FIO block usually holds the same in-ROM order.
THUMB_FN(0x1ea0d28e, _FIO_OpenFile)         // Storage::Storage.c Open
THUMB_FN(0x1ea0d446, _FIO_CreateFile)       // Storage::Storage.c Create
THUMB_FN(0x1ea0d794, _FIO_RemoveFile)       // Storage::Storage.c Remove
THUMB_FN(0x1ea0d846, _FIO_ReadFile)         // Storage::Storage.c Read
THUMB_FN(0x1ea0dc42,  FIO_SeekSkipFile)     // Storage::Storage.c Search
THUMB_FN(0x1ea0dd08, _FIO_WriteFile)        // Storage::Storage.c Write
THUMB_FN(0x1ea0e6c4,  FIO_CloseFile)        // Storage::Storage.c Close
THUMB_FN(0x1ea0e908, _FIO_GetFileSize64)    // Storage::Storage.c AcqSize
THUMB_FN(0x1ea0f3a4, _FIO_RenameFile)       // Storage::Storage.c Rename
THUMB_FN(0x1ea0f82a, _FIO_CreateDirectory)  // Storage::Storage.c CreateDir
THUMB_FN(0x1ea0fb80,  FIO_Flush)            // Storage::Storage.c Flush
THUMB_FN(0x1ea0fe84, _FIO_FindFirstEx)      // Storage::Storage.c FirstEnt
THUMB_FN(0x1ea0ff94,  FIO_FindNextEx)       // Storage::Storage.c NextEnt
THUMB_FN(0x1ea10054,  FIO_FindClose)        // Storage::Storage.c CloseEnt

/** Bitmap **/
// Not needed with FEATURE_VRAM_RGBA (and doesn't exist?)
//DATA_PTR(    0xFDFC,  bmp_vram_info)        // first YUV VRAM buffer, used right after DISP_SetUpdateOSDVram

/** Debug messages **/
THUMB_FN(0x1ea3685a,  DryosDebugMsg)        // Easy to catch in init task: "InitializeIntercom", "CreateSequencer"

/** Eventprocs (call by name) **/
THUMB_FN(0x1ea1c5b6, call);

/** GUI timers **/
THUMB_FN(0x1ebae7a8,  SetHPTimerAfterNow)    // System::HPTimer.c
THUMB_FN(0x1ebae83a,  SetHPTimerNextTick)    // System::HPTimer.c; next function; same "worker" as SetHPTimerAfterNow()
THUMB_FN(0x1ea1de40,  SetTimerAfter)         // SystemIF::Timer1m.c
// CancelTimer now returns int, not void:
THUMB_FN(0x1ea1defa, _CancelTimer)           // CancelRefreshTimer, CancelPlayTimer

/** Memory allocation **/
// There are two _AllocateMemory functions on XF605, one aligns up the size first, to 0x10.
// Possibly to reduce fragmentation?  It gets called more, so I'm picking that one.
// The other is at 0x1ea14fb2
THUMB_FN(0x1ea14e0a, _AllocateMemory)        // NG AllocateMemory
THUMB_FN(0x1ea151d6, _FreeMemory)            // uses the same heap
THUMB_FN(0x1ea14b90,  GetMemoryInformation)  // easy string, called from ShowMemoryInformation
THUMB_FN(0x1ea14b64,  GetSizeOfMaxRegion)    // easy string
THUMB_FN(0x1ea0b4ec, _alloc_dma_memory)      // allocates 64 bytes more, aligns at 32 and sets the uncacheable bit
THUMB_FN(0x1ea0b4e0, _free_dma_memory)       // paired with the above; clears the uncacheable bit and frees the raw pointer
THUMB_FN(0x1e9e0608, _malloc)                // Just before conditional uart_printf("can't malloc")
THUMB_FN(0x1e9e0640, _free)                  // After malloc() in ROM

/** Memory info **/
THUMB_FN(0x1e5360c4,  res_memshow)   // DIGIX X replacement of smemShowFix()
THUMB_FN(0x1e7d38e8,  memmap_info)   // Exception vector, DRYOS system memory etc
THUMB_FN(0x1e803890,  malloc_info)   // Malloc Information */
THUMB_FN(0x1e803928,  sysmem_info)   // System Memory Information */

/** Misc **/
THUMB_FN(0xe0100564,  uart_printf)   // in init task, prints "%s READY\n"
THUMB_FN(0xe024947c,  vsnprintf)     // called by LogFile_printf(..., "*Data Lost*\n", ...);
                                     // the one called by DebugMsg only knows %s
                                     // ROM copy in case we need to call it early.

/** Startup **/
// 16d0c + 0xd80 -> is this static location, or we have to rewrite for dynamic access?
//DATA_PTR(    0x,  additional_version)                     // via GUI_GetFirmVersion(); Points at '\n' char in char* with FW version

/** Tasks **/
DATA_PTR(    0x4140,  task_dispatch_hook)
THUMB_FN(0x1e68307c,  task_trampoline)
// task_create calls internal function (1e75e89c) which set up basic
// task structure, and part of that is call to task_trampoline_caller (1e68309c)
// (it is followed by check of caller return value).
// task_trampoline_caller will pass task_trampoline+1 as parameter to another
// function.
THUMB_FN(0x1e683390, _get_task_info_by_id)      // via extask

/** Recursive locks **/
THUMB_FN(0x1ea1e87c,  CreateRecursiveLock)      // SystemIF::KerMisc.c
THUMB_FN(0x1ea1d618,  AcquireRecursiveLock)     // SystemIF::KerRLock.c
THUMB_FN(0x1ea1d702,  ReleaseRecursiveLock)     // SystemIF::KerRLock.c

/** Semaphores **/
THUMB_FN(0x1ea22a78,  create_named_semaphore)   // SystemIF::KerSem.c, called from init task with "PU0Wait", "PU1Wait", "CCInit"... as 1st arg
THUMB_FN(0x1ea22c26,  give_semaphore)           // SystemIF::KerSem.c, follow xrefs to result of any of create_named_semaphore()
THUMB_FN(0x1ea22b9c,  take_semaphore)           // SystemIF::KerSem.c, follow xrefs like in give_semaphore()

/** GUI **/
DATA_PTR(    0x5280,  gui_main_struct)  // visible in asm near task_create() for GuiMainTask
DATA_PTR(   0x166d4,  gui_task_list)    // via WINSYS_CreateController()
THUMB_FN(0x1e5fc620,  gui_main_task)    // via task_create("GuiMainTask"...
THUMB_FN(0x1e5fc59c,  gui_enqueue_message)      // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
//THUMB_FN(0x,  gui_massive_event_loop)   // unsure
THUMB_FN(0x1e5f9d34,  SetGUIRequestMode)    // unsure - the GUI level code is different from EOS.
                                            // Compare use of "GuiGateUtil_Request_Mode" strings to R5
                                            // "GUIGate" strings.  Also see CURRENT_GUI_MODE.  XF605 has getter and setter
                                            // for that.  And, CURRENT_GUI_MODE + 0x8 on both R5 and XF605 seems to be
                                            // GUI_REQUEST_MODE.  XF605 has getter and setter for that, which is used
                                            // in this function.  R5 sets that global directly inside SetGUIRequestMode(),
                                            // XF605 calls the setter in a similar manner.

// gui_init_end and GUI_Control DNE. Implemented via function_overrides.

/** Dialog API **/
THUMB_FN(0x1ea261ae,  dialog_redraw)            // Often used at the end of functions that ref CreateDialogBox string
//THUMB_FN(0x,  dialog_set_property_str)  //

/** GUI timers **/
//THUMB_FN(0x,  StartDateTimer)           //
//THUMB_FN(0x,  CancelDateTimer)          //

/** Queue **/
THUMB_FN(0x1ea0aecc,  msg_queue_create)         // SystemIF::KerQueue.c; used in UiOpe_Initialize() (function that creates gui_main_task)
THUMB_FN(0x1ea0afac,  msg_queue_receive)        // SystemIF::KerQueue.c; used in gui_main_task()
THUMB_FN(0x1ea0b13a,  msg_queue_post)           // SystemIF::KerQueue.c; used in gui_enqueue_message()
THUMB_FN(0x1ea0b178,  msg_queue_count)          // via string "warning! QueueLength"

/** Memory **/
THUMB_FN(0x1e825040,  AllocateMemoryResource)               // Via System::RscWrap.c
THUMB_FN(0x1e825072,  AllocateContinuousMemoryResource)     //
THUMB_FN(0x1e8250a4, FreeMemoryResource)                    //
//THUMB_FN(0xe07f4dbc,  ConcatMemorySuite)                  // Via debug msg with fn name (after call). Not used by ML, but utilizes functions below:
THUMB_FN(0x1ea04b00,  GetFirstChunkFromSuite)               // In ConcatMemorySuite(), First function called.
THUMB_FN(0x1ea053e6,  GetNextMemoryChunk)                   // In ConcatMemorySuite(), function called in first loop with param2 passed as 1st arg
THUMB_FN(0x1ea04876,  GetMemoryAddressOfMemoryChunk)        // In ConcatMemorySuite(), uses param2 from GetNextMemoryChunk() as own param1
THUMB_FN(0x1ea044d0,  CreateMemoryChunk)                    // In ConcatMemorySuite(), just after calls to GetMemoryAddressOfMemoryChunk(); also called from CreateMemorySuite()
THUMB_FN(0x1ea04a70,  AddMemoryChunk)                       // In ConcatMemorySuite(), just after calls to CreateMemoryChunk()
THUMB_FN(0x1ea04fa8,  CreateMemorySuite)                    // One of just a few xrefs of CreateMemoryChunk(); String "PackMem.c", calls malloc(0x18)
THUMB_FN(0x1ea04ccc,  DeleteMemorySuite)                    // DebugMsg "DeleteMemorySuite 0x%x"
// kitor: disabled using CONFIG_MEMORY_SRM_NOT_WORKING
// DIGIC X does not use RscMgr anymore, those are dummy functions that just assert!
// SJE: haven't checked but I assume same for XF605
//THUMB_FN(0xe081baa0,  SRM_AllocateMemoryResourceFor1stJob)
//THUMB_FN(0xe081baaa,  SRM_FreeMemoryResourceFor1stJob)

/** Uncategorized **/
// From function that references "SD_DeviceCreate:"; looks like now it is a part of dynamic struct
DATA_PTR(       0x0,  sd_device)              // shouldn't matter unless "keep after format" feature is enabled
THUMB_FN(0x1ec76658,  _LoadCalendarFromRTC)   // Calls a func that does create_named_semaphore("GuiUtilDate"
THUMB_FN(0x0,  ErrForCamera_handler)   // Not yet found, not essential, part of crash logging

/** Properties **/
THUMB_FN(0x1ea35c98, _prop_cleanup)           // called near bindStageChangeAckCBR string
THUMB_FN(0x1ea356a4,  prop_register_slave)    // called near KABOTANDATA string
THUMB_FN(0x1ea3577a, _prop_request_change)    // 

/** General FEATURE_VRAM_RGBA stubs **/
//THUMB_FN(0xe0257452, VMIX_TransferRectangleToVram) // via debug message; not needed - left for references below
//THUMB_FN(0xe022b7fe, RENDERER_InitializeScreen)    // via debug message; not needed - left for references below
DATA_PTR(   0x167f0, winsys_sem);      // Used in VMIX_TransferRectangleToVram() as first arg of take_semaphore()
DATA_PTR(   0x167b4, _rgb_vram_info);  // in RENDERER_InitializeScreen(), written just after "JediWrapperClearSurface vram:%d" message
THUMB_FN(0x1e8195d0, XimrExe);         // In VMIX_TransferRectangleToVram(), just before "XimrExe" debug message

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
THUMB_FN(0x1e6065c4, XCM_GetSourceSurface)    // by debug message
//THUMB_FN(0x1e6062d6, XCM_MakeContext)       // by debug message. Not used in code, left for reference below.
DATA_PTR(  0x139fdc, _pXCM);                  // param1 to XCM_MakeContext() when called from VMIX_TransferRectangleToVram()

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)

/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox(), usually after a debug message.
 */
// SJE I think these don't exist on XF605, this level of the GUI is different.
// There are some StartLiveViewApp strings though.  Maybe leftover FA stuff?
// FIXME I have put in bad guesses for now, usage is mostly comparisons to func addr
// so it shouldn't be dangerous
THUMB_FN(0x1edfd1a0,  PlayMain_handler)          // via "PlayEditAllEraseExec[%d] %d" - probably wrong
THUMB_FN(0x1eeeeeee,  ShootOlcApp_handler)       // FIXME Couldn't find.  There a few Olc strings.
THUMB_FN(0x1ec2cbf0,  LiveViewApp_handler)       // via debug string "DlgLiveViewDV.c DELETE_DIALOG_REQUEST"
//THUMB_FN(0x,  LiveViewShutterApp_handler)      // via debug string "StartLiveViewShutterApp"
THUMB_FN(0x1eeeeeee,  PlayMovieGuideApp_handler) // FIXME couldn't find

/**
 * Code that creates / stops given task from above, also saves dialog pointer
 * somewhere. This seems to be a better way to detect if given app is running.
 *
 * Pointers to result of WINSYS_CreateDialogBox() call in given function.
 */
// SJE using strings over the rom, this pattern seems to remove mostly unwanted dialogs:
// $ grep DELETE_DIALOG strings.txt |grep -v Mn|grep -v DlgNw|grep -v DlgStatus
// I have then guesses the most similar function by name...  these are likely wrong.
DATA_PTR(   0x30058,  PlayMain_dialog)             // PlayEditAllErase_End
//DATA_PTR( 0x,  ShootOlcApp_dialog)
DATA_PTR(   0x1b24c,  LiveViewApp_dialog)          // this is for "LiveViewDV_Start", which might be similar...
//DATA_PTR( 0x,  LiveViewShutterApp_dialog)
//DATA_PTR(      0x,  PlayMovieGuideApp_dialog)

/**
 * UNUSED but referenced elsewhere
 **/
// SJE these do exist on XF605
//THUMB_FN(0x1ea26182, WINSYS_CreateDialogBox)      // Equiv. of `CreateDialogBox()`. Forces layer ID to 0 (GUI)
//THUMB_FN(0x1ea25ef8, WINSYS_CreateDialogBoxEx)    // Actual logic. Takes one more argument than CreateDialogBox() (layer ID)
