/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.6.0 firmware for the EOS RP.
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

/* Using Thumb stubs everywhere to keep things simple. */

//*************************************************************************************************************
// Tier 0
//*************************************************************************************************************

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xE0040104,  cstart)                          /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE05A6CDC,  bzero32)                         /* zeros out a data structure */
THUMB_FN(0xE05C04A4,  create_init_task)                /* low-level DryOS initialization */
THUMB_FN(0xE05A9F08,  dcache_clean)                    /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE05A9FDC,  icache_invalidate)               /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE004022C,  init_task)                       /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)               /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                    /* from task_create; pointer to the current task structure */
THUMB_FN(0xE058F77A,  msleep)                          /* argument is always multiple of 10 */
THUMB_FN(0xE05DA6C2,  task_create)                     /* used to start TaskMain, GuiMainTask etc */

/** Misc (Optional) **/
THUMB_FN(0xE059100C,  uart_printf)                     // The function which prints Kxxx READY.
// THUMB_FN(0xe03b714A,  snprintf) // crashes on cam!?
THUMB_FN(0xE006C3B8,  vsnprintf)                       // prints "!!! ERROR !!!\n" on uart.

//*************************************************************************************************************
// Tier 1
//*************************************************************************************************************

THUMB_FN(0xE067E696,  ptp_register_handler)

/** Interrupts **/
// THUMB_FN(0xE01F3584,  cli_spin_lock)                   /* used in AllocateMemory/FreeMemory and others */
// DATA_PTR(   0x287FC,  isr_table_handler)               /* from interrupt handler; address of the ISR handler */
// DATA_PTR(   0x28800,  isr_table_param)                 /* from interrupt handler; argument of the ISR handler */
// DATA_PTR(    0x4034,  post_isr_hook)                   /* from interrupt handler; called after the ISR handler */
// DATA_PTR(    0x4030,  pre_isr_hook)                    /* from interrupt handler; called before the ISR handler */

/** File I/O **/
THUMB_FN(0xE0584BDE, _FIO_CreateFile)
THUMB_FN(0xE0584B2C, _FIO_OpenFile)
THUMB_FN(0xE0585362,  FIO_CloseFile)
THUMB_FN(0xE0584FB6, _FIO_ReadFile)
THUMB_FN(0xE058514E, _FIO_WriteFile)
THUMB_FN(0xE0585090,  FIO_SeekSkipFile)                // renamed to FIO_SeekSkipFile but seems to have same functionality as FIO_SeekSkipFile. TODO: check!
// THUMB_FN(0xE05854FC, _FIO_GetFileSize)                 // renamed to FIO_AcquireFileSize and was probably called FIO_GetFileSize64 on older models.
THUMB_FN(0xE05854FC, _FIO_GetFileSize64)                 // renamed to FIO_AcquireFileSize and was probably called FIO_GetFileSize64 on older models.
THUMB_FN(0xE0584F1E, _FIO_RemoveFile)
THUMB_FN(0xE0585F5E, _FIO_RenameFile)
THUMB_FN(0xE05863D2, _FIO_CreateDirectory)
THUMB_FN(0xE0586840, _FIO_FindFirstEx)
THUMB_FN(0xE0586936,  FIO_FindNextEx)
THUMB_FN(0xE05869F8,  FIO_FindClose)
// THUMB_FN(0xE05865B6,  FIO_Flush)                       // to be called after FIO_CloseFile?

/** GUI **/
DATA_PTR(    0xfe10,  gui_task_list)       // first function after "LvRotate_StartRotateMgr" is used after WINSYS_AllocateMemory() function
// THUMB_FN(0xE004B91C,  GUI_Control)
THUMB_FN(0xE056A908,  SetGUIRequestMode)   // "SetGUIRequestMode(0x%08x)" string on top, "NotifyGUIEvent(%d)" on bottom of function.
// THUMB_FN(0x,  gui_init_end)             // Function does not exist on RP!
THUMB_FN(0xE004B0BC,  gui_main_task)       // Function pointer used in CreateTask with "GuiMainTask"
DATA_PTR(    0x52fc,  gui_main_struct)     // used after "QueueClearFlg Clear" string
THUMB_FN(0xe05825f6,  dialog_redraw)       // often called as last function in dialog functions
THUMB_FN(0xe004b1ee,  gui_enqueue_message) // via string "warning! QueueLength=" and "GUI::GUI.c" in one function

/** Bitmap **/
// TODO: Rendering is done via XIMR. bmp_vram_info stub is only defined for compatibility to existing code.
//       It should not be required for RP at all! 
DATA_PTR(    0x72258,  bmp_vram_info)                  // first VRAM buffer. */
// DATA_PTR(    0x72254,  bmp_vram_info2)              // second VRAM buffer */

THUMB_FN(0xe010efa4, XCM_GetSourceSurface)
DATA_PTR(    0xFF78, _pXCM);  // FF5C (VRAM_INFO) + 1C (pXCM)

/** DMA **/
// THUMB_FN(0xE03DC006,  dma_memcpy)

/** Debug messages **/
THUMB_FN(0xE05A5F38,  DryosDebugMsg)                   // lots of debug messages; format string is third argument */

/** Dumper **/
// THUMB_FN(0xE00DC99A,  dump_file)                       // tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

// ExecuteEventProcedure / can be used to call Event Procedures
THUMB_FN(0xE0595C2E,  call)                            // also known as ExecuteEventProcedure. Many functions called by name (lv_start, lv_stop etc) */

/** GUI timers **/
THUMB_FN(0xE05AC14E,  SetHPTimerNextTick)              // above "EISSyncTimer0CBR err=(%d)" string
THUMB_FN(0xE05AC0DE,  SetHPTimerAfterNow)              // above "comAdapterDelayMicroSecond : SetHPTimerAfterNow" string
THUMB_FN(0xE057EA20,  SetTimerAfter)                   // function after create semaphore "wlansdcon" string
THUMB_FN(0xE038AA1A,  CancelDateTimer)                 // "CancelDateTimer" string inside function
THUMB_FN(0xE057EAD6,  CancelTimer)                     // function call after "CancelTimer FocusProhibit %d"

/** MPU communication **/
// THUMB_FN(0xE05da06c,  mpu_recv)                        // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
// DATA_PTR(    0xA388,  mpu_recv_cbr)                    // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x54058,  mpu_recv_ring_buffer)            // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0xA360,  mpu_recv_ring_buffer_tail)       // ring buffer index incremented in the above subroutine
// THUMB_FN(0xE009B34C,  mpu_send)                        // "dwSize < TXBD_DATA_SIZE" - TODO: Function signature changed from 2 to 4 args.
// DATA_PTR(   0x54198,  mpu_send_ring_buffer)            // ring buffer used in mpu_send
// DATA_PTR(    0xA368,  mpu_send_ring_buffer_tail)       // ring buffer index incremented in mpu_send

/** Memory allocation **/
THUMB_FN(0xE05953DC, _AllocateMemory)                  // "[MEM] Total = %#lx, Free = %#lx, MaxReg = %#lx"
THUMB_FN(0xE0595594, _FreeMemory)                      // "HDRS_Terminate" called multiple times until "HDRS_Terminate : End"
THUMB_FN(0xE0595266,  GetMemoryInformation)            // called from AllocateMemory
THUMB_FN(0xE059523A,  GetSizeOfMaxRegion)              // same
THUMB_FN(0xE0594E3C, _alloc_dma_memory)                // called before "@@@ ERROR NOT_ENOUGH_MEMORY\n" string ...
THUMB_FN(0xE0594E70, _free_dma_memory)                 // ... called as last instruction where "@@@ ERROR NOT_ENOUGH_MEMORY\n" string is located.
THUMB_FN(0xE05AA07C, _malloc)                          // above "can't malloc" string
THUMB_FN(0xE05AA0B4, _free)                            // below "can't malloc string"

/** Memory info **/
THUMB_FN(0xE07404F4,  malloc_info)                     // "Malloc Information (%s type)\n"
THUMB_FN(0xe071175c,  memmap_info)                     // "DRYOS system object"
THUMB_FN(0xE01907F0,  smemShowFix)                     // "--- Common Lower ----", Decomp. looks a bit weird!?
THUMB_FN(0xe074058c,  sysmem_info)                     // "System Memory Information\n"

/** Recursive locks **/
THUMB_FN(0xe05a58d8,  CreateRecursiveLock)                  // Takes "PCOMNET_WINE CBMan" as argument
THUMB_FN(0xe038a7b8,  AcquireRecursiveLock)                 // function call after "CompleteFree:%s" string
THUMB_FN(0xe038a882,  ReleaseRecursiveLock)                 // Last call in after "LensIf_SetLvMovSelect" string

/** Semaphores **/
THUMB_FN(0xe05a5a48,  create_named_semaphore)               // Takes0 "PU0Wait" string as argument
THUMB_FN(0xe05a5bbe,  give_semaphore)                       // called after "fcsCreateTransformCompleteCallback (%#x)" string
THUMB_FN(0xe05a5b44,  take_semaphore)                       // function call above "TakeSemaphore(%ld)" string

/** Queue **/
THUMB_FN(0xe0580bf4,  msg_queue_create)                     // takes "DispOpeQueue" as argument
THUMB_FN(0xe0580cb8,  msg_queue_receive)                    // function call in the loop above "ReadTask" string
THUMB_FN(0xe0580e16,  msg_queue_post)                       // function call before "TerminateDL : TryPostMessageQueue (%#x)" string
THUMB_FN(0xe0580e4c,  msg_queue_count)                      // insude function where "warning! QueueLength=%d" is used

/** Memory **/
THUMB_FN(0xe061d398,  AllocateMemoryResource)               // function call after "[RD] AllocateSize [%#x][%#x]"
THUMB_FN(0xe061d3c6,  AllocateContinuousMemoryResource)     // function call aftger "StorageAllocateContMem" string
THUMB_FN(0xe061d3f4,  FreeMemoryResource)                   // function call after "SETUP_WriteFile: RemoveFile ERROR(%s)"
THUMB_FN(0xe05902d4,  AddMemoryChunk)                       // called twice in the function where "DpCreatePackMemory 0x%x %d" is used

THUMB_FN(0xe0590358,  GetFirstChunkFromSuite)               // First function called in function where "[FA] MemChunk Buff %lx, Size %ld, Offset %ld" string is located
THUMB_FN(0xe0590b78,  GetNextMemoryChunk)                   // Function call before "%s : NextMemChunk is NULL"
THUMB_FN(0xe059011a,  GetMemoryAddressOfMemoryChunk)        // Function call before "Suite:0x%x DarkAddr1:0x%x" string
THUMB_FN(0xe0590524,  DeleteMemorySuite)                    // Function call before "ConcatDummySuite : Delete hDispHeader failed [%#x]"
THUMB_FN(0xe05907b0,  CreateMemorySuite)                    // Function call before "FM_AsyncWriteFile S %d" string
THUMB_FN(0xe058fdf6,  CreateMemoryChunk)                    // Called inside function where "DpCreatePackMemory 0x%x %d" string is used

// THUMB_FN(0xe052d7c6,  SRM_AllocateMemoryResourceFor1stJob)  // function call before take sempahore call before "SRM_AllocateMemoryResourceFor1stJob failed [%#x]" string
// THUMB_FN(0xe05304bc,  SRM_FreeMemoryResourceFor1stJob)      // second function call below "MEM1 is NULL" string in else block

/** Tasks **/
DATA_PTR(    0x410c,  task_dispatch_hook) // hard to explain how to find. Compare to R
THUMB_FN(0xe00e3ab0,  task_trampoline)    // see comment below:
/*
 * kitor / coon: task_create calls internal function (e0288364) which set up basic
 * task structure, and part of that is call to task_trampoline_caller (e00e3ad0)
 * (it is followed by check of caller return value).
 * task_trampoline_caller will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0xe03c9888, _get_task_info_by_id) // function before " CORE : %d\n" string

/**
 * RGBA / Compositor functions and structures
 */

/**
 * Things needed for CONFIG_COMPOSITOR_XCM.
 *
 * RP uses two layers (GUI, focus overlays). WINSYS code swaps pointer in
 * WINSYS structure (one we know as _rgb_vram_info) to select which layer to
 * draw.
 *
 * With FEATURE_COMPOSITOR_XCM we ask XCM via XCM_GetSourceSurface to give us
 * pointer to layer 0, which is more reliable.
 */
THUMB_FN(0xe010efa4, XCM_GetSourceSurface)          // by debug message
//THUMB_FN(0xe010ed6c, XCM_MakeContext)             // by debug message. Not used in code, left for reference below.
DATA_PTR(    0xFF78, _pXCM);                        // param1 to XCM_MakeContext() when called from refreshVrmsSurface()

/**
 * Stubs needed for CONFIG_XCM_DEDICATED_LAYER
 */
THUMB_FN(0xe0254b0c, refreshVrmsSurface);           // by debug message.
DATA_PTR(    0xFF74, display_refresh_needed)        // Easy to spot in refreshVrmsSurface()
DATA_PTR(   0x720f0, RENDERER_LayersMetadata);
THUMB_FN(0xe010ef6a, XCM_SetSourceSurface);
THUMB_FN(0xe010efd4, XCM_SetSourceArea);
THUMB_FN(0xe0254e8c, XOC_SetLayerEnable);

/**
 * General FEATURE_VRAM_RGBA stubs
 *
 * _rgb_vram_info is a pointer to first Canon RGBA buffer (GUI).
 * On RP this is set up by RENDERER_InitializeScreen(), after MARVs are created:
 * 'rgb_vram_info = RENDERER_LayersArr[0];'
 */
//THUMB_FN(0xe0331492, RENDERER_InitializeScreen)   // by debug message. Not used in code, left for reference above.
THUMB_FN(0xe010c180, XimrExe);                      // In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xff60, winsys_sem);                   // Used in RefreshVrmsSurface around XimrExe call
DATA_PTR(    0xFF1C, _rgb_vram_info);

/** Uncategorized **/
DATA_PTR(   0x12928,  sd_device)                    // Used as parameter in DryosDebugMsg with "EVICE_POWER_CYCLE bPowerDown=%d" string
THUMB_FN(0xe06f0358,  _LoadCalendarFromRTC)         // function call after "handle_LOCAL_PLUSMOVIEAUTO_REGIST_HANDLE NewCreate Handle"
                                                    // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xe088400e,  ErrForCamera_handler)         // ERR70, ERR80 etc (DlgErrForCamera.c)

NSTUB(    0x40D0,  task_max)

/** Properties **/
THUMB_FN(0xe0596920, _prop_cleanup)                         // function call after "faChangeCBR(ID=%#lx)" string
THUMB_FN(0xe059638e,  prop_register_slave)                  // function call after "sdsActivate" string
THUMB_FN(0xe059645c, _prop_request_change)                  // function call after "update inner version string. \"%s\"" string

// *** CONTINUE HERE! ***
 
/** App handlers - found after 200D, but not sure if correct function start found **/
THUMB_FN(0xe055373c,  PlayMain_handler)                     // contains "DlgPlayMain.c StartFirmInfoApp" string
THUMB_FN(0xe06f3ef8,  ShootOlcApp_handler)                  // contains "DlgOlcMainCreative.c GUI_DIAL_Event(%#x)" string
THUMB_FN(0xe043519c,  LiveViewApp_handler)                  // contains "DlgLiveView.c During QR Request Event(0x%08X) Consume" string
THUMB_FN(0xe07e066a,  LiveViewShutterApp_handler)           // msg: "StartLiveViewShutterApp"

// See R180 stubs.s for details.
//DATA_PTR(   0xe498,  PlayMain_dialog)                    // in PlayMain_StartPlayMain(), 0xe508 - 0x70
//DATA_PTR(  0x15008,  ShootOlcApp_dialog)                 // in StartShootOlcApp()
DATA_PTR(    0x14844,  LiveViewApp_dialog)                 // in StartLiveViewApp(), 0x14840 + 0x4
//DATA_PTR(  0x1e488,  LiveViewShutterApp_dialog)          // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR(  0x16338,  PlayMovieGuideApp_dialog)           // in PlayMovieGuide_Start()

/**
 * STUFF BELOW IS WRONG!
 **/
/** App handlers **/
THUMB_FN(0x00000000,  PlayMovieGuideApp_handler)            // needed for CONFIG_QEMU

THUMB_FN(0x00000000,  fsuDecodePartitionTable) // this is called in one place in 50D, 0xffb0a18c,
                                               // and that function is easy to find in 200D via "CARD_EMERGENCY_STOP".
                                               // The function called 200D looks a lot different from fsuDecodePartitionTable, however.
                                               // Constant offsets used are the same in both, maybe fsuDPT has been partially 
                                               // inlined in 200D?  The decompilation looks weird too, claims it just returns
                                               // the calling param but it clearly twiddles some registers.
                                               // TODO check how ML uses this stub, since the sig has changed.

/** Optional for custom code **/
// THUMB_FN(0xE0587F74,  LedOn)
// THUMB_FN(0xE0587f94,  LedOff)
// THUMB_FN(0xE057B450,  CreateEvent)                     // TODO: verify!
