/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the EOS 77D.
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

/* Using Thumb stubs everywhere to keep things simple. */

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xe00400ec,  cstart)
THUMB_FN(0xdf00d284,  bzero32)
THUMB_FN(0xdf006514,  create_init_task)
THUMB_FN(0xe065e38c,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xe065e460,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xe0040214,  init_task)

/** Tasks **/
DATA_PTR(    0x1008,  current_interrupt)
DATA_PTR(    0x1020,  current_task)
DATA_PTR(    0x40E8,  task_max)
THUMB_FN(0xdf0087fe,  msleep)
THUMB_FN(0xdf008cd2,  task_create)

/** File I/O **/
THUMB_FN(0xe04d714c, _FIO_OpenFile)               // FIO_OpenFile
THUMB_FN(0xe04d74a6, _FIO_GetFileSize)            // FIO_GetFileSize
//THUMB_FN(0xe04d75da, _FIO_GetFileSize64)        // FIO_GetFileSize64, not needed as FIO_GetFileSize exists.
THUMB_FN(0xe04d80da, _FIO_FindFirstEx)            // FIO_FindFirstEx
THUMB_FN(0xe04d71b6, _FIO_CreateFile)             // FIO_CreateFile
THUMB_FN(0xe04d7270, _FIO_ReadFile)               // FIO_ReadFile
THUMB_FN(0xe04d7dd4,  FIO_Flush)                  // FIO_Flush
THUMB_FN(0xe04d7cb0, _FIO_CreateDirectory)        // FIO_CreateDirectory
THUMB_FN(0xe04d7224, _FIO_RemoveFile)             // FIO_RemoveFile
THUMB_FN(0xe04d7388,  FIO_CloseFile)              // FIO_CloseFile
THUMB_FN(0xe04d8172,  FIO_FindNextEx)             // FIO_FindNextEx
THUMB_FN(0xe04d80ba,  FIO_FindClose)              // FIO_FindClose
THUMB_FN(0xe04d7316, _FIO_WriteFile)              // FIO_WriteFile
THUMB_FN(0xe04d7b2a, _FIO_RenameFile)             // FIO_RenameFile
THUMB_FN(0xe04d8894,  FIO_SeekSkipFile)           // FIO_SeekSkipFile, via debug prints in xref sources

/** Bitmap **/
DATA_PTR(   0x10128,  bmp_vram_info)                        /* first YUV VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xdf006e6c,  DryosDebugMsg)

/** Eventprocs (call by name) **/
THUMB_FN(0xe04d8aee,  call)

/** GUI timers **/
THUMB_FN(0xe05aadca,  SetHPTimerNextTick)                   /* next function; same "worker" as SetHPTimerAfterNow */
THUMB_FN(0xe05aad76,  SetHPTimerAfterNow)                   /* System::HPTimer.c */
THUMB_FN(0xe04d48e4,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xe04d499a,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xdf007b48, _AllocateMemory)                       /* NG AllocateMemory */
THUMB_FN(0xdf007d00, _FreeMemory)                           /* uses the same heap */
THUMB_FN(0xdf0079d2,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xdf0079a6,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xdf00ac40, _alloc_dma_memory)                     /* allocates 64 bytes more, aligns at 32 and sets the uncacheable bit */
THUMB_FN(0xdf00ac6c, _free_dma_memory)                      /* paired with the above; clears the uncacheable bit and frees the raw pointer */
THUMB_FN(0xe06ce7a6, _malloc)                               // called before "can't malloc"
THUMB_FN(0xe06ce7de, _free)                                 // called later on the same buffer

/** Memory info **/
THUMB_FN(0xe01eaf80,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xe0164ca6,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xe02640b4,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xe026414c,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xe04d8bbc,  uart_printf)
THUMB_FN(0xdf009324,  vsnprintf)

/** Startup **/
DATA_PTR(    0xea5d,  additional_version)                   /* Pointer to '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(    0x4124,  task_dispatch_hook)
THUMB_FN(0xdf00325c,  task_trampoline)
THUMB_FN(0xdf004040, _get_task_info_by_id)

/** Recursive locks **/
THUMB_FN(0xdf00b668,  CreateRecursiveLock)                  /* SystemIF::KerMisc.c  */
THUMB_FN(0xdf00b43c,  AcquireRecursiveLock)                 /* SystemIF::KerRLock.c */
THUMB_FN(0xdf00b506,  ReleaseRecursiveLock)                 /* SystemIF::KerRLock.c */

/** Semaphores **/
THUMB_FN(0xdf008408,  create_named_semaphore)
THUMB_FN(0xdf00857e,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xdf008504,  take_semaphore)                       /* SystemIF::KerSem.c   */

/** GUI **/
DATA_PTR(   0x1002c,  gui_task_list)                        // easy to find in WINSYS_CreateController, struct starts from lock
DATA_PTR(    0x4828,  gui_main_struct)                      // directly referenced in QueueClearFlg()
THUMB_FN(0xe04c29c8,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xe00d28e2,  gui_main_task)                        // via task_create("GuiMainTask"...
THUMB_FN(0xe02f920c,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xe00d287c,  gui_enqueue_message)                  // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
THUMB_FN(0xe0092d34,  gui_init_end)

/** Dialog API **/
THUMB_FN(0xe04cc700,  dialog_redraw)                        // via xrefs to "pDialog->pSignature == m_pcSignature"
THUMB_FN(0xe04c9768,  dialog_set_property_str)

/** GUI timers **/
THUMB_FN(0xe0608a94,  StartDateTimer)                       // by debug message
THUMB_FN(0xe0608a1c,  CancelDateTimer)                      // by debug message

/** Queue **/
THUMB_FN(0xdf00b100,  msg_queue_create)                     // easy to spot in create_gui_main_task
THUMB_FN(0xdf00b1c4,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(0xdf00b322,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(0xdf00b358,  msg_queue_count)                      // GetNumOfEvent

/** Memory **/
THUMB_FN(0xe064e7da,  AllocateMemoryResource)               // m_pfAllocMemoryCBR
THUMB_FN(0xe064e808,  AllocateContinuousMemoryResource)     // m_pfContAllocMemoryCBR
THUMB_FN(0xe064e8ae,  FreeMemoryResource)                   // m_pfFreeMemoryCBR
THUMB_FN(0xe04d5496,  AddMemoryChunk)                       // Matched to 5D3. Similar (but slightly different): e054c3da e054cf5a
THUMB_FN(0xe04d551a,  GetFirstChunkFromSuite)               // AJ_PackMemory_PackMem_p3
THUMB_FN(0xe04d5dea,  GetNextMemoryChunk)                   // AJ_PackMemory_PackMem.c_IsChunkSignature_p2, identical fn is at e054cf14, but with just 3 xrefs
THUMB_FN(0xe04d5334,  GetMemoryAddressOfMemoryChunk)        // AJ_PackMemory_PackMem.c_IsChunkSignature - 200D uses "MemChunk" directly
THUMB_FN(0xe04d577c,  DeleteMemorySuite)                    // String "DeleteMemorySuite 0x%x"
THUMB_FN(0xe04d59bc,  CreateMemorySuite)                    // String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xe04d4ee8,  CreateMemoryChunk)                    // called from CreateMemorySuite
THUMB_FN(0xe04eaed2,  SRM_AllocateMemoryResourceFor1stJob)  // in CreateSkeltonJob()
THUMB_FN(0xe04edd76,  SRM_FreeMemoryResourceFor1stJob)      // in DeleteSkeltonJob()

/** Uncategorized **/
DATA_PTR(   0x1ffb4,  sd_device)                            // From function that references "SD_DeviceCreate:"
THUMB_FN(0xe066339c,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xe0642f64,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xe04d6a36, _prop_cleanup)                         // "PropertyMgr.c", strings changed, found based on 200D
THUMB_FN(0xe04d643e,  prop_register_slave)                  // "PropertyMgr.c", "prop_register_slave"
THUMB_FN(0xe04d650c, _prop_request_change)                  // "PropertyMgr.c", "pRequestChange"

/** C.Fn **/
THUMB_FN(0xe036b2b8,  GetCFnData)                           // via strings
THUMB_FN(0xe036b358,  SetCFnData)

/** Missing evprocs **/
THUMB_FN(0xe04b3690, _turn_off_display)
THUMB_FN(0xe04b36a4, _turn_on_display)

/**
 * RGBA / Compositor functions and structures
 */

/** General FEATURE_VRAM_RGBA stubs **/
THUMB_FN(0xe020fab8, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(   0x10120, winsys_sem);                           // Used in RefreshVrmsSurface around XimrExe call
DATA_PTR(   0x10124, _rgb_vram_info);                       // Canon RGB VRAM pointer (Ximr layer 0)

/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper(), usually after a debug message.
 * For some reason those doesn't seem to show up on gui_task_list on D8
 * (LiveViewApp) or there are not on top (PlayMain)
 */
THUMB_FN(0xe047c5f6,  PlayMain_handler)                     // in StartPlayMain()
THUMB_FN(0xe0645aa2,  ShootOlcApp_handler)                  // in StartShootOlcApp()
THUMB_FN(0xe04213c4,  LiveViewApp_handler)                  // in StartLiveViewApp()
//THUMB_FN(0xe0701ef8,  LiveViewShutterApp_handler)         // in StartLiveViewShutterApp()
THUMB_FN(0xe0500c2e,  PlayMovieGuideApp_handler)            // in StartPlayMovieGuideApp()

/**
 * Code that creates / stops given task from above, also saves dialog pointer
 * somewhere. This seems to be a better way to detect if given app is running.
 *
 * Pointers to result of WINSYS_CreateDialogBox_wrapper() call in given function.
 */
//DATA_PTR(  0xEF38,  PlayMain_dialog)                      // in StartPlayMain()
//DATA_PTR( 0x1b254,  ShootOlcApp_dialog)                   // in StartShootOlcApp()
DATA_PTR(    0xee10,  LiveViewApp_dialog)                   // in StartLiveViewApp()
//DATA_PTR( 0x1ac98,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR( 0x1c140,  PlayMovieGuideApp_dialog)             // in StartPlayMovieGuideApp()
