/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the PowerShot SX70 HS.
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

/* All functions are Thumb. */


/** Startup **/
NSTUB(  ROMBASEADDR,  firmware_entry)
THUMB_FN(0xE0040104,  cstart)                               /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xE05338A8,  bzero32)                              /* zeros out a data structure */
THUMB_FN(0xE01729DC,  create_init_task)                     /* low-level DryOS initialization */
THUMB_FN(0xE038804C,  dcache_clean)                         /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE0388120,  icache_invalidate)                    /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE004022C,  init_task)                            /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */

/** Tasks **/
DATA_PTR(    0x1010,  current_interrupt)                    /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */
DATA_PTR(    0x1028,  current_task)                         /* from task_create; pointer to the current task structure */
THUMB_FN(0xE05197fA,  msleep)                               /* argument is always multiple of 10 */
THUMB_FN(0xE038B12A,  task_create)                          /* used to start TaskMain, GuiMainTask etc */
NSTUB(       0x40D0,  task_max)                             /* 6th field in sysconfig struct */

/** File I/O **/
THUMB_FN(0xe0510890, _FIO_OpenFile)               // Open
THUMB_FN(0xe0511260, _FIO_GetFileSize64)          // AcqSize
THUMB_FN(0xe0512564, _FIO_FindFirstEx)            // FirstEnt
THUMB_FN(0xe0510df4,  FIO_SeekSkipFile)           // Search
THUMB_FN(0xe0510942, _FIO_CreateFile)             // Create
THUMB_FN(0xe0510d1a, _FIO_ReadFile)               // Read
THUMB_FN(0xe05122ee,  FIO_Flush)                  // Flush
THUMB_FN(0xe051210a, _FIO_CreateDirectory)        // CreateDir
THUMB_FN(0xe0510c82, _FIO_RemoveFile)             // Remove
THUMB_FN(0xe051265a,  FIO_FindNextEx)             // NextEnt
THUMB_FN(0xe05110c6,  FIO_CloseFile)              // Close
THUMB_FN(0xe051271c,  FIO_FindClose)              // CloseEnt
THUMB_FN(0xe0510eb2, _FIO_WriteFile)              // Write
THUMB_FN(0xe0511c86, _FIO_RenameFile)             // Rename

/** Debug messages **/
THUMB_FN(0xE0532B04,  DryosDebugMsg)                        /* lots of debug messages; format string is third argument */

/** Eventprocs (call by name) **/
THUMB_FN(0xE050D056,  call)                                 /* used in ElectricCurrentFirm */

/** GUI timers **/
THUMB_FN(0xe056885a,  SetHPTimerNextTick)                   /* next function; same "worker" as SetHPTimerAfterNow */
THUMB_FN(0xe05687ea,  SetHPTimerAfterNow)                   /* System::HPTimer.c */
THUMB_FN(0xe0514750,  SetTimerAfter)                        /* from error message */
THUMB_FN(0xe0514806,  CancelTimer)                          /* CancelRefreshTimer, CancelPlayTimer */

/** Memory allocation **/
THUMB_FN(0xe0519f80, _AllocateMemory)                       /* NG AllocateMemory */
THUMB_FN(0xe051a138, _FreeMemory)                           /* uses the same heap */
THUMB_FN(0xe0519e0a,  GetMemoryInformation)                 /* called from AllocateMemory */
THUMB_FN(0xe0519dde,  GetSizeOfMaxRegion)                   /* same */
THUMB_FN(0xe05199e0, _alloc_dma_memory)
THUMB_FN(0xe0519a14, _free_dma_memory)
THUMB_FN(0xE027D19A, _malloc)                               /* see "can't malloc" */
THUMB_FN(0xE027D1D2, _free)                                 /* next function; paired with the above */

/** Memory info **/
THUMB_FN(0xE01AADB4, smemShowFix)
THUMB_FN(0xE00A47E8, memmap_info)
THUMB_FN(0xE00E7f98, malloc_info)
THUMB_FN(0xE00E8030, sysmem_info)

/** Misc **/
THUMB_FN(0xE0523DAC, uart_printf)
THUMB_FN(0xe0071be4, vsnprintf)                            /* called by LogFile_printf(..., "*Data Lost*\n", ...); the one called by DebugMsg only knows %s */

/** Startup **/
// dd58 + 0xd3c -> is this static location, or we have to rewrite for dynamic access?
//DATA_PTR(    0x,  additional_version)                     // via GUI_GetFirmVersion(); Points at '\n' char in char* with FW version

/** Tasks **/
DATA_PTR(    0x410c,  task_dispatch_hook)
THUMB_FN(0xe00e050c,  task_trampoline)
THUMB_FN(0xe034ba48, _get_task_info_by_id)

/** Recursive locks **/
THUMB_FN(0xe05324a4,  CreateRecursiveLock)                  /* SystemIF::KerMisc.c  */
THUMB_FN(0xe0508a70,  AcquireRecursiveLock)                 /* SystemIF::KerRLock.c */
THUMB_FN(0xe0508b3a,  ReleaseRecursiveLock)                 /* SystemIF::KerRLock.c */

/** Semaphores **/
THUMB_FN(0xE0532614,  create_named_semaphore)
THUMB_FN(0xE053278A,  give_semaphore)                       /* SystemIF::KerSem.c   */
THUMB_FN(0xE0532710,  take_semaphore)                       /* SystemIF::KerSem.c   */


/** GUI **/
DATA_PTR(    0xfa28,  gui_task_list)                        // based on 200d
THUMB_FN(0xe04f08c4,  SetGUIRequestMode)                    // by debug message
THUMB_FN(0xe004f484,  gui_main_task)                        // via task_create("GuiMainTask"...
THUMB_FN(0xe038da68,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xe004f598,  gui_enqueue_message)                  // via string "warning! QueueLength=" and "GUI::GUI.c" in one function
/*
 * kitor: gui_main_struct is referenced in gui_main_task, but it starts earlier.
 * Real start was found via function that calls gui_massive_event_loop.
 */
DATA_PTR(    0x4d40,  gui_main_struct)

/** Dialog API **/
THUMB_FN(0xe05067ba,  dialog_redraw)                        // via xrefs to "pDialog->pSignature == m_pcSignature"
THUMB_FN(0xe04f8610,  dialog_set_property_str)

/*
 * kitor: gui_init_end DNE on R180. It's the only diff in gui_main_task vs 200d.
 *
 * I was unable to find GUI_Control either. It should print debug message and
 * call gui_enqueue_message(0, param_1, param_2, param_3 ), which adds it to gui queue.
 */

/** GUI timers **/
THUMB_FN(0xe02bc6f0,  StartDateTimer)                       // by debug message
THUMB_FN(0xe02bc7ae,  CancelDateTimer)                      // by debug message

/** Queue **/
THUMB_FN(0xe0509004,  msg_queue_create)                     // easy to spot in create_gui_main_task
THUMB_FN(0xe05090c8,  msg_queue_receive)                    // used on gui queue in gui_main_task
THUMB_FN(0xe0509226,  msg_queue_post)                       // used in gui_enqueue_message
THUMB_FN(0xe050925c,  msg_queue_count)                      // string "warning! QueueLength"

/** Memory **/
THUMB_FN(0xe0567fdc,  AllocateMemoryResource)
THUMB_FN(0xe056800a,  AllocateContinuousMemoryResource)
THUMB_FN(0xe0568038,  FreeMemoryResource)
THUMB_FN(0xe050de26,  AddMemoryChunk)
THUMB_FN(0xe050deaa,  GetFirstChunkFromSuite)
THUMB_FN(0xe050e6cc,  GetNextMemoryChunk)
THUMB_FN(0xe050dc6c,  GetMemoryAddressOfMemoryChunk)
THUMB_FN(0xe050e078,  DeleteMemorySuite)
THUMB_FN(0xe050e304,  CreateMemorySuite)
THUMB_FN(0xe050d952,  CreateMemoryChunk)
//kitor: disabled using CONFIG_MEMORY_SRM_NOT_WORKING
//THUMB_FN(0x,  SRM_AllocateMemoryResourceFor1stJob)
//THUMB_FN(0x,  SRM_FreeMemoryResourceFor1stJob)

/** Uncategorized **/
DATA_PTR(   0x125ac,  sd_device)                            // From function that references "SD_DeviceCreate:"
THUMB_FN(0xe066d708,  _LoadCalendarFromRTC)                 // "Private" as it now has more arguments, needs a wrapper.
THUMB_FN(0xe07645e6,  ErrForCamera_handler)                 // ERR70, ERR80 etc (DlgErrForCamera.c)

/** Properties **/
THUMB_FN(0xe051ad4a, _prop_cleanup)                         // "PropertyMgr.c", strings changed, found based on 200D
THUMB_FN(0xe051a812,  prop_register_slave)                  // "PropertyMgr.c", "prop_register_slave"
THUMB_FN(0xe051a8e0, _prop_request_change)                  // "PropertyMgr.c", "pRequestChange"

/**
 * RGBA / Compositor functions and structures
 */
THUMB_FN(0xE0110BD0, XimrExe);                              // In RefreshVrmsSurface, just before "XimrExe" debug message
DATA_PTR(    0xFB88, winsys_sem);                           // Used in RefreshVrmsSurface around XimrExe call
DATA_PTR(    0xFB38, _rgb_vram_info);

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
THUMB_FN(0xe01171e8, XCM_GetSourceSurface)          // by debug message
//THUMB_FN(0xe0116fb0, XCM_MakeContext)             // by debug message. Not used in code, left for reference below.
DATA_PTR(    0xfba0, _pXCM);                        // param1 to XCM_MakeContext() when called from refreshVrmsSurface()

/**
 * Stubs needed for CONFIG_XCM_DEDICATED_LAYER
 */
THUMB_FN(0xe0234aac, refreshVrmsSurface);           // by debug message.
DATA_PTR(    0xfb9c, display_refresh_needed)        // Easy to spot in refreshVrmsSurface()
DATA_PTR(   0x6aaac, RENDERER_LayersMetadata);      // accessed in for loop in RENDERER_InitializeScreen()
THUMB_FN(0xe01171ae, XCM_SetSourceSurface);
THUMB_FN(0xe0117218, XCM_SetSourceArea);
THUMB_FN(0xe0234e2c, XOC_SetLayerEnable);

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D6+ do use palletes to draw GUI, but it is hw rendered into RGBA

/** App handlers **/
THUMB_FN(0xe04d8bce,  PlayMain_handler)                     // in PlayMain_StartPlayMain()
THUMB_FN(0xe07abb4e,  ShootOlcApp_handler)                  // in StartShootOlcAppPrivate(), unsure - different than EOSR
THUMB_FN(0xe03c4e7a,  LiveViewApp_handler)                  // in StartLiveViewApp()
//THUMB_FN(0xe07cb36a,  LiveViewShutterApp_handler)         // in StartLiveViewShutterApp()
THUMB_FN(0xe0200342,  PlayMovieGuideApp_handler)            // in PlayMovieGuide_Start()

/**
 * Code that creates / stops given task from above, also saves dialog pointer
 * somewhere. This seems to be a better way to detect if given app is running.
 */
//DATA_PTR( 0xe24c,  PlayMain_dialog)                       // in PlayMain_StartPlayMain()
//DATA_PTR( 0xbb6c,  ShootOlcApp_dialog)                    // in StartShootOlcAppPrivate(), unsure - different than EOSR
DATA_PTR( 0x13b84,  LiveViewApp_dialog)                     // in StartLiveViewApp()
//DATA_PTR( 0x1b0cc,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR( 0x14bc8,  PlayMovieGuideApp_dialog)             // in PlayMovieGuide_Start()
