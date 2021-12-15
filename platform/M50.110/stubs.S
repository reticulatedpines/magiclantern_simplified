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
NSTUB( 0x40CC, task_max)

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

/** Startup **/
DATA_PTR(    0xE484,  additional_version)
/* kitor: Please note that this points at '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(    0x4108,  task_dispatch_hook)                   /* same as R180 */
THUMB_FN(0xe00e968c,  task_trampoline)
/*
 * kitor: task_create calls internal function (e05f5fb4) which set up basic
 * task structure, and part of that is call to task_trampoline_caller (e00e96ac)
 * (it is followed by check of caller return value).
 * task_trampoline_caller will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0xe01c8378, _get_task_info_by_id)
/*
 * kitor: via extask function. While on 5D3 it was called directly, now helper
 * is used. Find extask via string formats. Trace variables holding task id,
 * you will quickly find where it is verified (this is the mentioned helper).
 * In the helper code you will find call to our stub.
 */

/** Recursive locks **/
THUMB_FN(0xe0577598,  CreateRecursiveLock)                  /* SystemIF::KerMisc.c  */
THUMB_FN(0xe06d77dc,  AcquireRecursiveLock)                 /* SystemIF::KerRLock.c */
THUMB_FN(0xe06d78a6,  ReleaseRecursiveLock)                 /* SystemIF::KerRLock.c */

/** Semaphores **/
THUMB_FN(0xe0577b60,  create_named_semaphore)
THUMB_FN(0xe0577cd6,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xe0577c5c,  take_semaphore)                       /* SystemIF::KerSem.c   */

/** GUI **/
DATA_PTR(    0xfb38,  gui_task_list)                        // based on R. Ref to "CtrlMan: Use RecursiveLock"
THUMB_FN(0xe0519d94,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xe004f032,  gui_main_task)                        // task_create("GuiMainTask"...
THUMB_FN(0xe054f3ca,  dialog_redraw)                        // via xrefs to "pDialog->pSignature == pcSignature"
THUMB_FN(0xe037719e,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xe004efcc,  gui_enqueue_message)                  // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
THUMB_FN(0xe065a4f4,  gui_init_end)                         // via "EndGuiInit" string
DATA_PTR(    0x536c,  gui_main_struct)

/*
 * kitor Similar to R180 I was unable to find GUI_Control either. If you compare
 * 200D.101 surrounding functions, it just seems to DNE on R and M50.
 */

/** GUI timers **/
THUMB_FN(0xe064895c,  StartDateTimer)                       // by debug message
THUMB_FN(0xe06489d4,  CancelDateTimer)                      // by debug message

/** MPU communication **/
/*
 * kitor: skipped for now. InitializeIntercom looks identical to R180?!
 * I thought M50 has no MPU, yet at least buffers are initialized.
 */

/** Queue **/
THUMB_FN(0xe0544124,  msg_queue_create)                     // easy to spot in create_gui_main_task
THUMB_FN(0xe05441e8,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(0xe0544346,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(0xe054437c,  msg_queue_count)                      // called before string "warning! QueueLength"

/** Memory **/
THUMB_FN(0xe05c7f0e,  AllocateMemoryResource)               // m_pfAllocMemoryCBR, this function takes 4 params in 200D/EOSR, 3 for 50D
THUMB_FN(0xe05c7f3c,  AllocateContinuousMemoryResource)     // m_pfContAllocMemoryCBR, takes 4 params now, 3 for 50D
THUMB_FN(0xe05c7fe2,  FreeMemoryResource)                   // m_pfFreeMemoryCBR
THUMB_FN(0xe0553c5c,  AddMemoryChunk)                       // Same as in 200D (matched by offsets), may be wrong as multiple variants exists.
                                                            // Other options: e054c3da e054cf5a
THUMB_FN(0xe0553d82,  GetFirstChunkFromSuite)               // "IsSuiteSignature( hSuite )" + PackMem
THUMB_FN(0xe05546a4,  GetNextMemoryChunk)                   // "IsSuiteSignature( hSuite )" + PackMem
                                                            // Similar function is at e05547a0, but with just a few xrefs and different offset used
THUMB_FN(0xe0553b9c,  GetMemoryAddressOfMemoryChunk)        // "IsChunkSignature( hChunk )" + PackMem - 200D uses "MemChunk" directly
THUMB_FN(0xe0553fe4,  DeleteMemorySuite)                    // String "DeleteMemorySuite 0x%x"
THUMB_FN(0xe0554276,  CreateMemorySuite)                    // String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xe055374c,  CreateMemoryChunk)                    // called from CreateMemorySuite
//kitor: disabled using CONFIG_MEMORY_SRM_NOT_WORKING
//THUMB_FN(0xe056a5c6,  SRM_AllocateMemoryResourceFor1stJob)  // "pMessage" and "pPriorMessage" with "RscMgr.c". Unsure - there seems to be multiple identical functions
                                                            // Matched like in R by structure params to 200D stub 
//THUMB_FN(0xe056da22,  SRM_FreeMemoryResourceFor1stJob)      // find refs to "pMessage", there are several similar looking functions using it,
                                                            // Matched to 200D stub, just buffer size differs (0x34 vs 0x30)

/** Uncategorized **/
DATA_PTR(   0x115b8,  sd_device)                            // From function that references "SD_DeviceCreate:"
THUMB_FN(0xe0637210,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xe037f9c0,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xe055a246, _prop_cleanup)                         // "PropertyMgr.c", strings changed, found based on 200D
THUMB_FN(0xe0559c92,  prop_register_slave)                  // "PropertyMgr.c", "pRegisterSlave"
THUMB_FN(0xe0559d60, _prop_request_change)                  // "PropertyMgr.c", "pRequestChange"

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

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D8 has no indexed RGB buffers.


/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper / WINSYS_CreateDialogBox, just after some
 * debug message */
THUMB_FN(0xe04c7e5c,  PlayMain_handler)                     // msg: "StartPlayMain"
THUMB_FN(0xe02d40f2,  ShootOlcApp_handler)                  // msg: "StopShootOlcApp PopPalette(%d)"
THUMB_FN(0xe04dc446,  LiveViewApp_handler)                  // msg: "StartLiveViewApp(%#x)"
//THUMB_FN(0xe0338a10,  LiveViewShutterApp_handler)         // msg: "StartLiveViewShutterApp"
THUMB_FN(0xe058af54,  PlayMovieGuideApp_handler)            // msg: "StartPlayMovieGuideApp"

// See R180 stubs.s for details.
//DATA_PTR(  0xe708,  PlayMain_dialog)                      // in StartPlayMain()
//DATA_PTR( 0x13fa4,  ShootOlcApp_dialog)                   // in StartShootOlcApp()
DATA_PTR(   0x139b8,  LiveViewApp_dialog)                   // in StartLiveViewApp()
//DATA_PTR( 0x1c434,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR( 0x14fa8,  PlayMovieGuideApp_dialog)             // in StartPlayMovieGuideApp()



/**
 * STUFF BELOW IS WRONG!
 **/

THUMB_FN(0xdf00d365,  fsuDecodePartitionTable)             // inlined like in EOS R. See R180 function_overrides.c for WIP alternative
