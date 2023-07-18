/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.2 firmware for the PowerShot SX740.
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
NSTUB(  ROMBASEADDR,  firmware_entry)
THUMB_FN(0xE00400FC,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE0550EB4,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE010BEA0,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE0320810,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE03208E4,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040224,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE053647A,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xE052E402,  task_create)                          /* used to start TaskMain, GuiMainTask etc */
NSTUB(       0x40CC,  task_max)

/** File I/O **/
THUMB_FN(0xe0520b66, _FIO_CreateFile)
THUMB_FN(0xe0520ab4, _FIO_OpenFile)
THUMB_FN(0xe05212ee,  FIO_CloseFile)
THUMB_FN(0xe0520f3e, _FIO_ReadFile)
THUMB_FN(0xe05210da, _FIO_WriteFile)
THUMB_FN(0xe052101a,  FIO_SeekSkipFile)                     // guess: Search?!
THUMB_FN(0xe0520ea4, _FIO_RemoveFile)
THUMB_FN(0xe0521ec0, _FIO_RenameFile)
THUMB_FN(0xe0522348, _FIO_CreateDirectory)
THUMB_FN(0xe05227aa, _FIO_FindFirstEx)                      // FirstEnt
THUMB_FN(0xe05228a2,  FIO_FindNextEx)                       // NextEnt
THUMB_FN(0xe0522964,  FIO_FindClose)                        // CloseEnt
THUMB_FN(0xe0522532,  FIO_Flush)                            // to be called after FIO_CloseFile?
THUMB_FN(0xe052148a, _FIO_GetFileSize64)                    // AcqSize; different in D8 cameras - returns 64 bit size in struct of lower/upper 32bit part.

/** Bitmap **/
DATA_PTR(    0xf3e0,  bmp_vram_info)                        /* first YUV VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE0550060,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xe00baa36,  dump_file)                            // shouldn't be used, gone from newer models

/** Eventprocs (call by name) **/
THUMB_FN(0xe053c28a,  call)                                 /* used in ElectricCurrentFirm */

/** GUI timers **/
THUMB_FN(0xe0554aaa,  SetHPTimerNextTick)                   /* next function; same "worker" as SetHPTimerAfterNow */
THUMB_FN(0xe0554a4a,  SetHPTimerAfterNow)                   /* System::HPTimer.c */
THUMB_FN(0xe05373bc,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xe0537472,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xe053a704, _AllocateMemory)                       /* NG AllocateMemory */
THUMB_FN(0xe053a8bc, _FreeMemory)                           /* uses the same heap */
THUMB_FN(0xe053a58e,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xe053a562,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xe053a164, _alloc_dma_memory)                     /* allocates 64 bytes more, aligns at 32 and sets the uncacheable bit */
THUMB_FN(0xe053a198, _free_dma_memory)                      /* paired with the above; clears the uncacheable bit and frees the raw pointer */
THUMB_FN(0xE0552D0E, _malloc)                               /* see "can't malloc" */
THUMB_FN(0xE0552D46, _free)                                 /* next function; paired with the above */

/** Memory info **/
THUMB_FN(0xe008a34c,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xe0138f96,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xe00c9c68,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xe00c9d00,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE053A070,  uart_printf)
THUMB_FN(0xE0065A90,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */

/** New stubs **/

/** Startup **/
DATA_PTR(    0xD7B1,  additional_version)
/* kitor: Please note that this points at '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(    0x4108,  task_dispatch_hook)
THUMB_FN(0xe00c3628,  task_trampoline)
/*
 * kitor: task_create calls internal function (e01c8ac0) which set up basic
 * task structure, and part of that is call to task_trampoline_caller (e00c3648)
 * (it is followed by check of caller return value).
 * task_trampoline_caller will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0xe01f39d0, _get_task_info_by_id)
/*
 * kitor: via extask function. While on 5D3 it was called directly, now helper
 * is used. Find extask via string formats. Trace variables holding task id,
 * you will quickly find where it is verified (this is the mentioned helper).
 * In the helper code you will find call to our stub.
 */

/** Recursive locks **/
THUMB_FN(0xe0541750,  CreateRecursiveLock)                  /* SystemIF::KerMisc.c  */
THUMB_FN(0xe0516cec,  AcquireRecursiveLock)                 /* SystemIF::KerRLock.c */
THUMB_FN(0xe0516db6,  ReleaseRecursiveLock)                 /* SystemIF::KerRLock.c */

/** Semaphores **/
THUMB_FN(0xe054fbf0,  create_named_semaphore)
THUMB_FN(0xE054FD66,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xE054FCEC,  take_semaphore)                       /* SystemIF::KerSem.c   */

/** GUI **/
DATA_PTR(    0xF29C,  gui_task_list)                        // based on 200d
THUMB_FN(0xe0515238,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xe004c17c,  gui_main_task)                        // via task_create("GuiMainTask"...
THUMB_FN(0xe03ac4be,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xe004c292,  gui_enqueue_message)                  // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
/*
* kitor: gui_main_struct is referenced in gui_main_task, but it starts earlier.
* Real start was found via function that calls gui_massive_event_loop.
*/
DATA_PTR(    0x4cc8,  gui_main_struct)

/** Dialog API **/
THUMB_FN(0xe052fdba,  dialog_redraw)                        // via xrefs to "pDialog->pSignature == m_pcSignature"
THUMB_FN(0xe051e2d0,  dialog_set_property_str)

/*
 * kitor: gui_init_end DNE. It's the only diff in gui_main_task vs 200d.
 *
 * I was unable to find GUI_Control either. It should print debug message and
 * call gui_enqueue_message(0, param_1, param_2, param_3 ), which adds it to gui queue.
 */

 /** GUI timers **/
 THUMB_FN(0xe01f038c,  StartDateTimer)                       // by debug message
 THUMB_FN(0xe01f0438,  CancelDateTimer)                      // by debug message

/** Queue **/
THUMB_FN(0xe0510c40,  msg_queue_create)                     // easy to spot in create_gui_main_task
THUMB_FN(0xe0510d04,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(0xe0510e62,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(0xe0510e98,  msg_queue_count)                      // string "warning! QueueLength"

/** Memory **/
THUMB_FN(0xe03215ca,  AllocateMemoryResource)               // m_pfAllocMemoryCBR
THUMB_FN(0xe03215f8,  AllocateContinuousMemoryResource)     // m_pfContAllocMemoryCBR
THUMB_FN(0xe032169e,  FreeMemoryResource)                   // m_pfFreeMemoryCBR
//THUMB_FN(0xe05386ee,  ConcatMemorySuite)                  // Via debug msg with fn name (after call). Not used by ML, but utilizes functions below:
THUMB_FN(0xe0537d90,  GetFirstChunkFromSuite)               // In ConcatMemorySuite(), First function called.
THUMB_FN(0xe05386a8,  GetNextMemoryChunk)                   // In ConcatMemorySuite(), function called in first loop with param2 passed as 1st arg
THUMB_FN(0xe0537baa,  GetMemoryAddressOfMemoryChunk)        // In ConcatMemorySuite(), uses param2 from GetNextMemoryChunk() as own param1
THUMB_FN(0xe0537760,  CreateMemoryChunk)                    // In ConcatMemorySuite(), just after calls to GetMemoryAddressOfMemoryChunk(); also called from CreateMemorySuite()
THUMB_FN(0xe0537d0c,  AddMemoryChunk)                       // In ConcatMemorySuite(), just after calls to CreateMemoryChunk()
THUMB_FN(0xe053827a,  CreateMemorySuite)                    // String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xe0537fe8,  DeleteMemorySuite)                    // String "DeleteMemorySuite 0x%x"
// SRM disabled via CONFIG_MEMORY_SRM_NOT_WORKING

/** Uncategorized **/
DATA_PTR(   0x11790,  sd_device)                            // From function that references "SD_DeviceCreate:"
THUMB_FN(0xe058a9d4,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xe0196c40,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xe053bb38, _prop_cleanup)                         // "PropertyMgr.c", strings changed, found based on 200D
THUMB_FN(0xe053b58a,  prop_register_slave)                  // "PropertyMgr.c", "prop_register_slave"
THUMB_FN(0xe053b658, _prop_request_change)                  // "PropertyMgr.c", "pRequestChange"

/**
 * RGBA / Compositor functions and structures
 */

/**
* Things needed for CONFIG_COMPOSITOR_XCM.
*
* R uses two layers (GUI, focus overlays). WINSYS code swaps pointer in
* WINSYS structure (one we know as _rgb_vram_info) to select which layer to
* draw.
*
* With FEATURE_COMPOSITOR_XCM we ask XCM via XCM_GetSourceSurface to give us
* pointer to layer 0, which is more reliable.
*/
THUMB_FN(0xe00db0ac, XCM_GetSourceSurface)                  // by debug message
//THUMB_FN(0xe00dae76, XCM_MakeContext)                     // by debug message. Not used in code, left for reference below.
DATA_PTR(    0xF448, _pXCM);                                // param1 to XCM_MakeContext() when called from refreshVrmsSurface()

/**
 * Stubs needed for CONFIG_XCM_DEDICATED_LAYER
 */
THUMB_FN(0xe036c084, refreshVrmsSurface);                   // by debug message. Renamed to VMIX_TransferRectangleToVram on Digic X
DATA_PTR(    0xF450, display_refresh_needed)                // Easy to spot in refreshVrmsSurface()

/**
 * Structures needed for CONFIG_XCM_DEDICATED_LAYER. Specific for SX740 only!
 * Differs from R by not having VMIX_LayersEnable.
 * See https://www.magiclantern.fm/forum/index.php?topic=26024
 */
DATA_PTR(   0x9FCAC, RENDERER_Layers);
DATA_PTR(   0x9FFB0, VMIX_Layers);

/**
 * General FEATURE_VRAM_RGBA stubs
 *
 * _rgb_vram_info is a pointer to first Canon RGBA buffer (GUI).
 * On R this is set up by RENDERER_InitializeScreen(), after MARVs are created:
 * 'rgb_vram_info = RENDERER_LayersArr[0];'
 **/
//THUMB_FN(0x, RENDERER_InitializeScreen)           // by debug message. Not used in code, left for reference above.
THUMB_FN(0xe00d6838, XimrExe);                              // In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xF3D4, winsys_sem);                           // Used in RefreshVrmsSurface around XimrExe call
DATA_PTR(    0xF37C, _rgb_vram_info);

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D6+ do use palletes to draw GUI, but it is hw rendered into RGBA

/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper(), usually after a debug message.
 * For some reason those doesn't seem to show up on gui_task_list on D8
 * (LiveViewApp) or there are not on top (PlayMain)
 */
THUMB_FN(0xe04f6ebe,  PlayMain_handler)                     // in StartPlayMain()
THUMB_FN(0xe01ad442,  ShootOlcApp_handler)                  // in StartShootOlcAppPrivate()
THUMB_FN(0xe03dafca,  LiveViewApp_handler)                  // in StartLiveViewApp()
//THUMB_FN(0xe018bcf8,  LiveViewShutterApp_handler)         // in StartLiveViewShutterApp()
THUMB_FN(0xe0385fe6,  PlayMovieGuideApp_handler)            // in StartPlayMovieGuideApp()

/**
 * Code that creates / stops given task from above, also saves dialog pointer
 * somewhere. This seems to be a better way to detect if given app is running.
 *
 * Pointers to result of WINSYS_CreateDialogBox_wrapper() call in given function.
 */
//DATA_PTR(  0xDC64,  PlayMain_dialog)                      // in StartPlayMain()
//DATA_PTR( 0x134e0,  ShootOlcApp_dialog)                   // in StartShootOlcAppPrivate()
DATA_PTR(   0x12ca4,  LiveViewApp_dialog)                   // in StartLiveViewApp()
//DATA_PTR( 0x1a424,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR( 0x13d00,  PlayMovieGuideApp_dialog)             // in StartPlayMovieGuideApp()

/**
 * UNUSED but referenced elsewhere
 **/
//THUMB_FN(0xe05d9f30,  ctrlman_dispatch_event)
//THUMB_FN(0xe052fdaa, WINSYS_CreateDialogBox_wrapper)      // Equiv. of `CreateDialogBox()`. Forces layer ID to 0 (GUI)
//THUMB_FN(0xe052fb84, WINSYS_CreateDialogBox)              // Actual logic. Takes one more argument than CreateDialogBox() (layer ID)
                                                            // On R, Layer 1 is used only by StartLiveViewAFFrameApp()
/* Experiments */
THUMB_FN(0xe04c2b3c, GUI_SetImgComposition)                 // For enabling raw photo capability
