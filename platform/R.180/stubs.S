/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.8.0 firmware for the EOS R.
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
NSTUB(  ROMBASEADDR,  firmware_entry)
THUMB_FN(0xE0040104,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE056313c,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE011E120,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE02C6D40,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE02C6E14,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE004022C,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE0549952,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0XE05172FA,  task_create)                          /* used to start TaskMain, GuiMainTask etc */
NSTUB( 0x40CC, task_max)

/** File I/O **/
THUMB_FN(0xE053BAC2, _FIO_CreateFile)
THUMB_FN(0xE053BA10, _FIO_OpenFile)
THUMB_FN(0xE053C24A,  FIO_CloseFile)
THUMB_FN(0xE053BE9A, _FIO_ReadFile)
THUMB_FN(0xE053C036, _FIO_WriteFile)
THUMB_FN(0xE053BF76,  FIO_SeekSkipFile)                     // guess: Search?!
THUMB_FN(0xE053BE00, _FIO_RemoveFile)
THUMB_FN(0xE053CE1C, _FIO_RenameFile)
THUMB_FN(0xE053D2A4, _FIO_CreateDirectory)
THUMB_FN(0xE053D706, _FIO_FindFirstEx)                      // FirstEnt
THUMB_FN(0xE053D7FE,  FIO_FindNextEx)                       // NextEnt
THUMB_FN(0xE053D8C0,  FIO_FindClose)                        // CloseEnt
THUMB_FN(0xE053D48E,  FIO_Flush)                            // to be called after FIO_CloseFile?
THUMB_FN(0xE053C3E6, _FIO_GetFileSize64)                    // AcqSize; different in D8 cameras - returns 64 bit size in struct of lower/upper 32bit part.

/** Bitmap **/
DATA_PTR(    0xFDFC,  bmp_vram_info)                        /* first YUV VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE0563314,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Dumper **/
THUMB_FN(0xE13192D8,  dump_file)                            /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Eventprocs (call by name) **/
THUMB_FN(0xE0553072,  call)                                 /* used in ElectricCurrentFirm */

/** GUI timers **/
THUMB_FN(0xE039D062,  SetHPTimerNextTick)                   /* next function; same "worker" as SetHPTimerAfterNow */
THUMB_FN(0xE039D002,  SetHPTimerAfterNow)                   /* System::HPTimer.c */
THUMB_FN(0xE053AE8C,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xE053AF42,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
//THUMB_FN(0xE0552814, _AllocateMemory)                       /* NG AllocateMemory */
//THUMB_FN(0XE05529CC, _FreeMemory)                           /* uses the same heap */
//THUMB_FN(0xE055269E,  GetMemoryInformation)                 /* called from AllocateMemory */
//THUMB_FN(0xE0552672,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xE0552274, _alloc_dma_memory)                     /* allocates 64 bytes more, aligns at 32 and sets the uncacheable bit */
THUMB_FN(0xE05522A8, _free_dma_memory)                      /* paired with the above; clears the uncacheable bit and frees the raw pointer */
THUMB_FN(0xE021C7FC, _malloc)                               /* see "can't malloc" */
THUMB_FN(0xE021C834, _free)                                 /* next function; paired with the above */

/** Memory allocation - experimental **/
/* Functions used by MemoryManager based allocators. Take MemoryManager object as 1st argument */
THUMB_FN(0xe0552720, _AllocateMemory_impl)                  /* Called from _AllocateMemory */
THUMB_FN(0Xe05528aa, _FreeMemory_impl)                      /* Called from _FreeMemory */
THUMB_FN(0xe055267a, _GetMemoryInformation_impl)            /* Called from GetMemoryInformation */
THUMB_FN(0xe0552602, _GetSizeOfMaxRegion_impl)              /* Called from GetSizeOfMaxRegion */
THUMB_FN(0xe0552308, MMGR_InitializeRegion)                 /* Initializes MemoryManager. Called in InitializePermanentMemory */

DATA_PTR(    0x50BC, MMGR_DEFAULT_POOL)                     /* MemoryManager pointer from _AllocateMemory (passed to _AllocateMemory_impl) */
DATA_PTR(0x9D9A0000, MMGR_REGION_START)                     /* Start of our managed region */
DATA_PTR(0x9DE5FFFF, MMGR_REGION_END)                       /* End of our managed region */

/** Memory info **/
THUMB_FN(0xE008C974,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE015A14A,  smemShowFix)                          /* Common Lower, Common Upper etc */
THUMB_FN(0xE00CDE5C,  malloc_info)                          /* Malloc Information */
THUMB_FN(0xE00CDEF4,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE054D31C,  uart_printf)
THUMB_FN(0xE006463C,  vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */

/** New stubs **/

/** Startup **/
DATA_PTR(    0xE079,  additional_version)
/* kitor: Please note that this points at '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(    0x4108,  task_dispatch_hook)
THUMB_FN(0xE00C6BC8,  task_trampoline)
/*
 * kitor: task_create calls internal function (e02392e0) which set up basic
 * task structure, and part of that is call to task_trampoline_caller (e00c6be8)
 * (it is followed by check of caller return value).
 * task_trampoline_caller will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0xe02ebf38, _get_task_info_by_id)
/*
 * kitor: via extask function. While on 5D3 it was called directly, now helper
 * is used. Find extask via string formats. Trace variables holding task id, 
 * you will quickly find where it is verified (this is the mentioned helper).
 * In the helper code you will find call to our stub.
 */

/** Recursive locks **/
THUMB_FN(0xE0554740,  CreateRecursiveLock)                  /* SystemIF::KerMisc.c  */
THUMB_FN(0xE02504A4,  AcquireRecursiveLock)                 /* SystemIF::KerRLock.c */
THUMB_FN(0xe025056e,  ReleaseRecursiveLock)                 /* SystemIF::KerRLock.c */

/** Semaphores **/
THUMB_FN(0xE0562DD4,  create_named_semaphore)
THUMB_FN(0xE0562f4A,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xE0562ED0,  take_semaphore)                       /* SystemIF::KerSem.c   */

/** GUI **/
DATA_PTR(    0xfcb4,  gui_task_list)                        // based on 200d
THUMB_FN(0xe052f34c,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xE004AD80,  gui_main_task)                        // task_create("GuiMainTask"...
THUMB_FN(0xe03ae6ea,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xE004AE96,  gui_enqueue_message)                  // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
/*
 * kitor: gui_main_struct is referenced in gui_main_task, but it starts earlier.
 * Real start was found via function that calls gui_massive_event_loop.
 */
DATA_PTR(    0x5304,  gui_main_struct)

 /** Dialog API **/
 THUMB_FN(0xe0541db6,  dialog_redraw)                        // via xrefs to "pDialog->pSignature == m_pcSignature"
 THUMB_FN(0xe0524080,  dialog_set_property_str)

/*
 * kitor: gui_init_end DNE on R180. It's the only diff in gui_main_task vs 200d.
 *
 * I was unable to find GUI_Control either. It should print debug message and
 * call e004ae96(0, param_1, param_2, param_3 ), which adds it to gui queue.
 */

/** GUI timers **/
THUMB_FN(0xE07ad2e4,  StartDateTimer)                       // by debug message
THUMB_FN(0xE07AD390,  CancelDateTimer)                      // by debug message

/** MPU communication **/
THUMB_FN(0xE008AE16,  mpu_send)                             // "dwSize < TXBD_DATA_SIZE", see comment below
THUMB_FN(0xE022D3F4,  mpu_recv)                             // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x9E88,  mpu_recv_cbr)                         // mpu_recv is called indirectly through this function pointer in recv helper of SIO3_ISR
DATA_PTR(   0x47B74,  mpu_send_ring_buffer)                 // ring buffer used in mpu_send
DATA_PTR(    0x9E6C,  mpu_send_ring_buffer_tail)            // ring buffer index incremented in mpu_send
DATA_PTR(   0x47A34,  mpu_recv_ring_buffer)                 // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x9E60,  mpu_recv_ring_buffer_tail)            // ring buffer index incremented in the above subroutine

/*
 * kitor: I'm not 100% sure of mpu_send itself. Offsets below are from R180.
 *
 * InitializeIntercom (e022d496) at the end calls helper (e008ad02) which will
 * allocRingBuffer(e008aa50) in sizes:
 * - 0x50 for mpu_recv_ring_buffer - checks for previous models
 * - 0x32 for mpu_send_ring_buffer - checks for previous models
 * - 0x05 for unk_send_ring_buffer - and this is important for what's going on.
 *
 * Any later use of "mpu_send" in function names is based on assumption that
 * 0x32 buffer is indeed mpu_send_ring_buffer.
 *
 * There are two sio_send functions:
 * sio_send_2arg (e008ae16) takes 2 args. Seems to use unk_send_ring_buffer
 * sio_send_4arg (e008ada0) takes 4 args. Seems to use mpu_send_ring_buffer
 * which is opposite to expected (previous models had 2 args for mpu)
 *
 * SIO3_ISR (e008ac6c) calls (not in order, names may be wrong):
 * mpu_recv_isr (e008af72): uses mpu_recv_ring_buffer
 * sio_send_isr (e008ab4c): uses unk_send_ring_buffer
 * mpu_send_isr (e008aa9c): uses mpu_send_ring_buffer
 *
 * To make matters worse, there seems to be two tails for each buffer.
 * mpu_recv_isr  -> mpu_recv_ring_buffer_tail   (9e60)
 *                  mpu_recv_ring_buffer_tail_2 (9e64)
 *
 * sio_send_2arg -> unk_send_ring_buffer_tail   (9e7c)
 * sio_send_isr  -> unk_send_ring_buffer_tail_2 (9e78)
 *
 * mpu_send_isr  -> mpu_send_ring_buffer_tail   (9e6c)
 * sio_send_4arg -> mpu_send_ring_buffer_tail_2 (9e68)
 *
 * To add even more confusion, if you look at assembly of send-related functions
 * you will find out that sio_unknown_send_ring_buffer is counted by memory
 * offset from mpu_send_ring_buffer.
 * (maybe that should be understood as struct of two buffers?)
 */

/** Queue **/
THUMB_FN(0xe053f63c,  msg_queue_create)                     // easy to spot in create_gui_main_task
THUMB_FN(0xe053f700,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(0xe053f85e,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(0xe053f894,  msg_queue_count)                      // string "warning! QueueLength"

/** Memory **/
THUMB_FN(0xe06b8d56,  AllocateMemoryResource)               // m_pfAllocMemoryCBR
THUMB_FN(0xe06b8d84,  AllocateContinuousMemoryResource)     // m_pfContAllocMemoryCBR
THUMB_FN(0xe06b8e2a,  FreeMemoryResource)                   // m_pfFreeMemoryCBR
THUMB_FN(0xe054c47c,  AddMemoryChunk)                       // Matched to 5D3. Similar (but slightly different): e054c3da e054cf5a
THUMB_FN(0xe054c500,  GetFirstChunkFromSuite)               // AJ_PackMemory_PackMem_p3
THUMB_FN(0xe054ce18,  GetNextMemoryChunk)                   // AJ_PackMemory_PackMem.c_IsChunkSignature_p2, identical fn is at e054cf14, but with just 3 xrefs
THUMB_FN(0xe054c31a,  GetMemoryAddressOfMemoryChunk)        // AJ_PackMemory_PackMem.c_IsChunkSignature - 200D uses "MemChunk" directly
THUMB_FN(0xe054c758,  DeleteMemorySuite)                    // String "DeleteMemorySuite 0x%x"
THUMB_FN(0xe054c9ea,  CreateMemorySuite)                    // String "PackMem.c", calls malloc(0x18)
THUMB_FN(0xe054bed0,  CreateMemoryChunk)                    // called from CreateMemorySuite
//kitor: disabled using CONFIG_MEMORY_SRM_NOT_WORKING
//THUMB_FN(0xe04e41be,  SRM_AllocateMemoryResourceFor1stJob)  // "pMessage" and "pPriorMessage" with "RscMgr.c". Unsure - there seems to be multiple identical functions
                                                            // Matched by structure params to 200D stub.
//THUMB_FN(0xe04e7590,  SRM_FreeMemoryResourceFor1stJob)      // Refs to "pPriorMessage". One that stores 0 and all three input
                                                            // params to struct matches 5D3.113 even in offsets

/** Uncategorized **/
DATA_PTR(   0x11dc0,  sd_device)                            // From function that references "SD_DeviceCreate:"
THUMB_FN(0xe079beec,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xe083819e,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xe0553d88, _prop_cleanup)                         // "PropertyMgr.c", strings changed, found based on 200D
THUMB_FN(0xe05537da,  prop_register_slave)                  // "PropertyMgr.c", "prop_register_slave"
THUMB_FN(0xe05538a8, _prop_request_change)                  // "PropertyMgr.c", "pRequestChange"

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
THUMB_FN(0xe00dde38, XCM_GetSourceSurface)                  // by debug message
//THUMB_FN(0xe00ddc02, XCM_MakeContext)                     // by debug message. Not used in code, left for reference below.
DATA_PTR(    0xFE64, _pXCM);                                // param1 to XCM_MakeContext() when called from refreshVrmsSurface()

/**
 * Stubs needed for CONFIG_XCM_DEDICATED_LAYER
 */
THUMB_FN(0xe0701f4a, refreshVrmsSurface);                   // by debug message. Renamed to VMIX_TransferRectangleToVram on Digic X
DATA_PTR(    0xFE6C, display_refresh_needed)                // Easy to spot in RefreshVrmsSurface

/**
 * Structures needed for CONFIG_XCM_DEDICATED_LAYER. Specific for EOS R only!
 * See https://www.magiclantern.fm/forum/index.php?topic=26024
 */
DATA_PTR(   0x6EFA0, RENDERER_LayersArr);
DATA_PTR(   0x6F2A4, VMIX_LayersArr);
DATA_PTR(   0x6F2BC, VMIX_LayersEnableArr);

/**
 * General FEATURE_VRAM_RGBA stubs
 *
 * _rgb_vram_info is a pointer to first Canon RGBA buffer (GUI).
 * On R this is set up by RENDERER_InitializeScreen(), after MARVs are created:
 * 'rgb_vram_info = RENDERER_LayersArr[0];'
 **/
//THUMB_FN(0xe0567322, RENDERER_InitializeScreen)           // by debug message. Not used in code, left for reference above.
THUMB_FN(0xe00da8c0, XimrExe);                              //In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xFDF0, winsys_sem);                           //Used in RefreshVrmsSurface around XimrExe call
DATA_PTR(    0xFD98, _rgb_vram_info);

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D8 has no indexed RGB buffers.

/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper / WINSYS_CreateDialogBox, just after some
 * debug message */
THUMB_FN(0xe04f71fc,  PlayMain_handler)                     // msg: "StartPlayMain"
THUMB_FN(0xe024584e,  ShootOlcApp_handler)                  // msg: "StartShootOlcApp PushPalette(%d)"
THUMB_FN(0xe03ec4cc,  LiveViewApp_handler)                  // msg: "StartLiveViewApp(%#x)"
THUMB_FN(0xe02a9f58,  LiveViewShutterApp_handler)           // msg: "StartLiveViewShutterApp"
THUMB_FN(0xe038f1c6,  PlayMovieGuideApp_handler)            // msg: "StartPlayMovieGuideApp"

THUMB_FN(0xe0842940,  TouchBarFeedBackApp_handler)          // msg: "StartTouchBarFeedBackApp"
THUMB_FN(0xe0786d74,  LiveViewTouchBarApp_handler)          // msg: "StartLiveViewTouchBarApp"

/**
 * UNUSED but referenced elsewhere
 **/

//THUMB_FN(0xe0780cb8,  ctrlman_dispatch_event)

/**
 * Stuff below should not be enabled. Looks that there are many hardcoded
 * addresses in 0xCxxxxxxx range all over ML code. D8 seems to use 0xDxxxxxxx
 * range for Engio.
 */

/** Engio **/
// THUMB_FN(0x, _EngDrvOut)                                      // not found yet, can be wrapped to _engio_write if needed
// THUMB_FN(0xE06B92B4, _engio_write)                            // via "Count < MAX_ENG_REG_NUM","EngineDS::DSEngIO.c" strings
// THUMB_FN(0x,  shamem_read)                                    // not found yet

/** ResLock **/
// THUMB_FN(0xE054761E,  CreateResLockEntry)                     // one that creates 0x24 structure with cls pointer to "LockEntry"
//                                                               // and multiple "ResInfo" structures
// THUMB_FN(0xE0547806,  LockEngineResources)                    // Takes LockEntry as input, creates "EFM_ERSC_STATIC" inside LockEntry
// THUMB_FN(0xE05478A6,  UnLockEngineResources)                  // Takes LockEntry as input, releases semaphore from LockEntry field

/** EDMAC **/
// THUMB_FN(0x,  AbortEDmac)
// THUMB_FN(0x,  ConnectReadEDmac)
// THUMB_FN(0x,  ConnectWriteEDmac)
// THUMB_FN(0x,  RegisterEDmacAbortCBR)
// THUMB_FN(0x,  UnregisterEDmacAbortCBR)
// THUMB_FN(0xE0535A82,  RegisterEDmacCompleteCBR)               // Not sure. Many registered CBRs have "CompCBR" in debug print names
// THUMB_FN(0xE0535E16,  UnregisterEDmacCompleteCBR)             // Uses same data structure as above. Replaces CBR with pointer to "do nothing" function.
// THUMB_FN(0x,  RegisterEDmacPopCBR)
// THUMB_FN(0x,  UnregisterEDmacPopCBR)
// THUMB_FN(0x,  SetEDmac)
// THUMB_FN(0x,  StartEDmac)
