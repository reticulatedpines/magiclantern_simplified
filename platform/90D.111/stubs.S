/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the EOS 90D.
 *
 */
/*
 * Copyright (C) 2018 Magic Lantern Team
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

/** Startup **/
    NSTUB(ROMBASEADDR, firmware_entry)
    THUMB_FN(0xe0040104,  cstart)                 /* calls bzero32 and create_init_task(..., init_task, ...) */
    THUMB_FN(0xe05aa59c,  bzero32)                /* zeros out a data structure */
    THUMB_FN(0xe02e04bc,  create_init_task)       /* low-level DryOS initialization */
    THUMB_FN(0xe0040236,  init_task)              /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */
    THUMB_FN(0xe05a91b0,  dcache_clean)           /* loop with MCR p15 c7,c10,1; DSB */
    THUMB_FN(0xe05a9284,  icache_invalidate)      /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */

/** Tasks **/
    THUMB_FN(0xe03812ce,  task_create)            /* used to start TaskMain, GuiMainTask etc */
//THUMB_FN(0xdf008f0b,  task_create_ex)         /* as task_create, but allows selecting CPU for task*/
    THUMB_FN(0xe059a902,  msleep)                 /* argument is always multiple of 10 */
    DATA_PTR(    0x1028,  current_task)           /* from task_create; pointer to the current task structure */
    DATA_PTR(    0x1010,  current_interrupt)      /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */

/** Dumper **/
    THUMB_FN(0xe0383fd6,  dump_file)              /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Memory info **/
    THUMB_FN(0xe02e5a3c,  malloc_info)            /* Malloc Information */
    THUMB_FN(0xe02e5ad4,  sysmem_info)            /* System Memory Information */
    THUMB_FN(0xe02cb580,  memmap_info)            /* Exception vector, DRYOS system memory etc */
    THUMB_FN(0xe0147c1c,  smemShowFix)            /* Common Lower, Common Upper etc */

/** Memory allocation **/
    THUMB_FN(0xe059afa4, _AllocateMemory)
    THUMB_FN(0xe059b15c, _FreeMemory)
    THUMB_FN(0xe059ae2e,  GetMemoryInformation)   /* called from AllocateMemory */
    THUMB_FN(0xe059ae02,  GetSizeOfMaxRegion)
    THUMB_FN(0xe059b764, _alloc_dma_memory)
    THUMB_FN(0xe059b758, _free_dma_memory)
#THUMB_FN(0xE0693B23,  malloc)
#THUMB_FN(0xE0693B5B,  free)

/** Debug messages **/
    THUMB_FN(0xe05a9720,  DryosDebugMsg)          /* lots of debug messages; format string is third argument */

/** DMA **/
    THUMB_FN(0xe05d6172,  dma_memcpy)

/** Eventprocs (call by name) **/
    THUMB_FN(0xe058dbc2,  call)                   /* many functions called by name (lv_start, lv_stop etc) */

/** File I/O **/
    THUMB_FN(0xe057c880, _FIO_OpenFile)
    THUMB_FN(0xe057c932, _FIO_CreateFile)
    THUMB_FN(0xe057cd0a, _FIO_ReadFile)
    THUMB_FN(0xe057cec2, _FIO_WriteFile)
    THUMB_FN(0xe057ce04,  FIO_SeekSkipFile)
    THUMB_FN(0xe057d0dc,  FIO_CloseFile)
    THUMB_FN(0xe057e162, _FIO_CreateDirectory)
//THUMB_FN(0xE04BD061,  FIO_FindFirst)
    THUMB_FN(0xe057e5bc, _FIO_FindFirstEx)
//THUMB_FN(0xE04BD0EF,  FIO_FindNextEx) // SJE this looks like FIO_FindNext to me, not Ex
    THUMB_FN(0xe057e6be,  FIO_FindNextEx)
    THUMB_FN(0xe057e780,  FIO_FindClose)
    THUMB_FN(0xe057d276, _FIO_GetFileSize64)
    THUMB_FN(0xe057cc72, _FIO_RemoveFile)
    THUMB_FN(0xe057dcda, _FIO_RenameFile)
    THUMB_FN(0xe057e346,  FIO_Flush)               // to be called after FIO_CloseFile?

/** GUI **/
//THUMB_FN(0xE00C5415,  GUI_Control) // Looks missing on 850D, possibly same as R
    THUMB_FN(0xe056305e,  SetGUIRequestMode) // Code is quite different from earlier cams, not a big switch?
                                             // Possibly weird decomp?  Param is the same though, maybe it still works
    THUMB_FN(0xe0557a8c,  gui_init_end) // DNE on R, but does on 850D
    THUMB_FN(0xe0061430,  gui_main_task)
    THUMB_FN(0xe0061566,  gui_enqueue_message) // "warning! QueueLength=%d"

/** GUI timers **/
    THUMB_FN(0xe0588faa,  CancelTimer)
    THUMB_FN(0xe03b3c68,  SetHPTimerAfterNow)
    THUMB_FN(0xe03b3cee,  SetHPTimerNextTick)     /* same "worker" function as SetHPTimerAfterNow */
    THUMB_FN(0xe0588ef4,  SetTimerAfter)
    THUMB_FN(0xe070ad32,  CancelDateTimer)

/** Interrupts **/
//DATA_PTR(    0x4030,  pre_isr_hook)
//DATA_PTR(    0x4034,  post_isr_hook)
//DATA_PTR(   0x6CC10,  isr_table_handler)
//DATA_PTR(   0x6CC14,  isr_table_param)
    THUMB_FN(0xe01af0f0,  cli_spin_lock)          /* used in AllocateMemory/FreeMemory and others */

/** MPU communication **/
    THUMB_FN(0xe00c13f8,  mpu_send) // "InterCom::Siodriver.c"
    THUMB_FN(0xe02e1914,  mpu_recv) // "ReceiveCBR : Unknown Command"
    DATA_PTR(    0xafa4,  mpu_recv_cbr) // search for Direct Refs to mpu_recv address, function pointer should be found,
                                        // find the function that uses it, which stores it; address where stored is mpu_recv_cbr
    DATA_PTR(   0x5b808,  mpu_send_ring_buffer)      // ring buffer used in mpu_send
    DATA_PTR(    0xaf98,  mpu_send_ring_buffer_tail) // ring buffer index incremented in mpu_send
    DATA_PTR(   0x5b6c8,  mpu_recv_ring_buffer)      // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
    DATA_PTR(    0xafb0,  mpu_recv_ring_buffer_tail) // ring buffer index incremented in the above subroutine

/** Misc **/
    THUMB_FN(0xe0067a88,  vsnprintf)              /* called near dmstart; references "01234567", "0123456789", "0123456789abcdef"
                                                 and "0123456789ABCDEF"; second arg is size; the one called by DebugMsg only knows %s */

// SJE stuff found during investigation, maybe not needed for ML
// but I wanted to call by name
    THUMB_FN(0xe03b1d54,  debug_assert) // DRYOS_ASSERT_HANDLER related

// SJE trying to get a substitute vsync callback working for screen refresh
//DATA_PTR(    0xfd78,  winsys_sem_count)
//DATA_PTR(    0xfec8,  bmp_vsync_cbr)
//THUMB_FN(0xe049e96c,  BmpVsyncCallBack)

// UNSURE or untested
    THUMB_FN(0xe00bb74e,  get_task_by_id) // called indirectly by task_create and functions using string "TASK Name"
    THUMB_FN(0xe0329738, _get_task_info_by_id)
    NSTUB(    0x50c4,  dm_names) // in DebugMsg, before the 1st loop target
    NSTUB(   0x14514,  sd_device) // in sdReadBlk, probably 0xe02b6fa2.  Lots of refs
                                  // to 0x1fd7c, some to 0x1fd80
    NSTUB(    0x53ec,  gui_main_struct) // See function using "Queue Clear event" string, e00c55b6
                                        // There's a base of struct, and some refs to fields via offsets.
                                        // Find the right one.
//THUMB_FN(0xe00c54ab,  GUI_ChangeMode) // "GUI_ChangeMode:%d" string
    THUMB_FN(0xe05912b2,  DispSensorStart) // 0x86f4 looks to be a bool for whether Sensor is started
    THUMB_FN(0xe0599c34,  DeleteMemorySuite)
    THUMB_FN(0xe0599ec0,  CreateMemorySuite)
    THUMB_FN(0xe0599506,  CreateMemoryChunk) // called from CreateMemorySuite

//NSTUB(0xe04ba7e6,  GetSizeOfMemorySuite) // not needed, but I found it along the way
    THUMB_FN(0xe05a9404,  take_semaphore) // Fairly sure. Same function sig and followed by give_semaphore
    THUMB_FN(0xe05a947e,  give_semaphore)
    THUMB_FN(0xe05a9308,  create_named_semaphore) // also see df00b114, very similar, but df008418 is closer to 50D create_named_semaphore.
                                              // Complicated by there being df00b648, which wraps 8418 with an extra assert possible,
                                              // and e0426a20, which is what most sites call, which calls 8418 - but thunked.
                                              // In theory that means DryOS could replace the underlying create_named_semaphore
                                              // at run-time.  No evidence that they do.

    THUMB_FN(0xe03da5f0,  gui_change_mode) // GUIGate_PostChangeMode
    THUMB_FN(0xe03d84aa,  gui_massive_event_loop) // various "GUICMD_"* strings
//THUMB_FN(0x,  gui_init_event) // "-> handleGuiInit"

    // The various memory functions can't be found by debug_assert strings in 850D
    // because the first field to debug_assert is always 0.
    // Instead, check call sites of the function, find those, find equivalent call
    THUMB_FN(0xe05999e4,  AddMemoryChunk) // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
                                          // maybe 0xe04ba536? Similar, except it returns void, not int.
                                          // Also see 0xe04ba494, v. similar but diff struct offsets
    THUMB_FN(0xe0599a68,  GetFirstChunkFromSuite)
    THUMB_FN(0xe059a288,  GetNextMemoryChunk)
    THUMB_FN(0xe059982a,  GetMemoryAddressOfMemoryChunk)
    THUMB_FN(0xe06ae190,  AllocateMemoryResource)
    THUMB_FN(0xe06ae1be,  AllocateContinuousMemoryResource)
    THUMB_FN(0xe06ae1ec,  FreeMemoryResource)
    THUMB_FN(0xe058097c,  msg_queue_receive) // "SystemIF::KerQueue.c" useful for finding some of these msg_ functions


    THUMB_FN(0xe0580ada,  msg_queue_post) // used in gui_enqueue_message

    THUMB_FN(0xe0580b10,  msg_queue_count) // Fairly hard to find.  Often used near DispLockStop, which is easy to find
                                           // and not used often.
    THUMB_FN(0xe05808b8,  msg_queue_create) // CreateMessageQueue - In 50D, msg_queue_create calls 0xff866424,
                                        // the equiv of which is 0xdf00b114 (and was easier to find)
    THUMB_FN(0xe059c1a0,  prop_deliver) // used near "UNKNOWN PROPERTY ID " in call to DryosDebugMsg
    THUMB_FN(0xe059bcaa,  prop_register_slave)
    DATA_PTR(0xeb80,      LiveViewApp_dialog) // in StartLiveViewApp()
    THUMB_FN(0xe0421974,  LiveViewApp_handler) // DebugMsg: "StartLiveViewApp(%#x)", address passed to CreateDialogBox
    THUMB_FN(0xe087690c,  LiveViewShutterApp_handler) // DebugMsg: "StartLiveViewShutterApp", address passed to CreateDialogBox
    THUMB_FN(0xe08b565c,  PlayMovieGuideApp_handler) // "PlayMovieGuide_Start" 
    THUMB_FN(0xe054bbea,  PlayMain_handler) // DebugMsg: "StartPlayMain", address passed to CreateDialogBox
    THUMB_FN(0xe0625440,  ShootOlcApp_handler) // near "StopShootOlcApp PopPalette(%d)"
    THUMB_FN(0xe0869772,  GetCFnData) // "GetCFnData" Function sig is different but the body looks similar.
                                      // Looks like the inner functions use registers the outer doesn't, which confuses Ghidra
                                      // re how many args outer takes.
    THUMB_FN(0xe0869120,  SetCFnData) // There are some "SetCFnData" related strings
                                      // SJE size of struct has changed, don't know if this is relevant

// Not required on 200D given usage by ML is always behind cam specific guards.
//THUMB_FN(0xe069ce15,  GUI_SetLvMode) // via "GUI_SetBracketReset" - but this takes void now, and overwrites r0,
                                     // whereas ML thinks it takes int.  Either this is wrong, or will
                                     // need wrapping to preserve the reg.

    THUMB_FN(0xe064202c, _malloc) // These two names are used in 50D and CONFIG_HELLO_WORLD won't build without them,
    THUMB_FN(0xe0642064, _free)  // however, the inital stubs for 200D call these simply malloc/free.
    THUMB_FN(0xe051950a,  SRM_AllocateMemoryResourceFor1stJob) // "hJobMemorySuite" strings, function a little earlier
    THUMB_FN(0xe051b930,  SRM_FreeMemoryResourceFor1stJob) // find refs to "pMessage", there are several similar looking functions using it,
                                                           // this is my best guess - this one has pPriorMessage and sets offset 8
                                                           // in the struct to 0, offset 0x10 to param1
    THUMB_FN(0xe059c254, _prop_cleanup) // "./Multicast/PropertyMgr.c" check funcs that ref this string
    THUMB_FN(0xe05a8dd0,  CreateRecursiveLock) // via "KerRLock.c", CRL() calls a function that regs this string, not many do
    THUMB_FN(0xe0589ef4,  AcquireRecursiveLock)
    THUMB_FN(0xe0589fbe,  ReleaseRecursiveLock)
    THUMB_FN(0xe058b7cc,  dialog_redraw) // Found via checking xrefs to "pDialog->pSignature == pcSignature"
//THUMB_FN(0xe04b3739,  CreateResLockEntry) // via xrefs to a pointer to "ResInfo"
//THUMB_FN(0xe04b3b59,  LockEngineResources) //  Down a bit from CreateResLockEntry, start v similar but end is different.
//THUMB_FN(0xe06217f1,  ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

THUMB_FN(0x0,  fsuDecodePartitionTable)

//DATA_PTR(    0x40d0,  first_task) // unsure on naming here.  First item in an array of task structs.
                                  // Seems to always be "idle" task.
                                  // (we don't use this, but instead calculate it during early init,
                                  //  to save backporting this stub for all older cams)
    NSTUB(    0x4108,  task_max)
    NSTUB(    0x115c0,  gui_task_list) // Strings have changed or been removed compared to earlier cams.
                                       // Found via checking all call sites of CreateRecursiveLock()
                                       // and comparing with 200d until similar code found.
                                       // gui_task_list is lock related
    THUMB_FN(0xe07f40f0, _LoadCalendarFromRTC)


// These are required on Digic 678
    THUMB_FN(0xe074623c, RefreshVrmsSurface) // "Call RefreshVrmsSurface" / "XimrExe"
    DATA_PTR(   0x11738,  display_refresh_needed) // if 0, RefreshVrmsSurface() does nothing
    THUMB_FN(0xe00da090, XimrExe) // trigger Ximr unit to execute (HW compositor, partially understood)
    DATA_PTR(   0x11724,  winsys_sem)

// These are not strictly required, although recent dev work has added dependencies;
// these likely should be removed before a "real" release
    THUMB_FN(0xe05952d0, uart_printf)
#NSTUB(0xdf008c7c, uart_printf) // Thunked equivalent of the above.  Note, ARM, not Thumb.
                               // Both seem to work the same.

// These are not required, but have been used for experiments, debugging, etc
//THUMB_FN(0xe049f041, fill_rectangle_to_vram) // "FillRectangleToVram ILLEGAL_PARAMETER"
//THUMB_FN(0xe02b07c3, marv_dcache_clean) // find _rgb_vram_info, this function is called with
                                        // that address as only param, after null check.
                                        // Function inspects the struct and does dcache_clean
                                        // on the extent of the bitmap_data.  Not referenced
                                        // much.  Possibly only used during init?
//THUMB_FN(0xe02ba483, JediDraw) // maybe sends a buffer to 2d accel unit, for output / render?
//THUMB_FN(0xe048f6e7, OsdReverseMode) // "OsdReverseMode" - does this flip the OSD output?
//THUMB_FN(0xe025e88d, XimrSetLayerVisibility)
//THUMB_FN(0xe025e8ad, XimrSetInputLayerMARV)
//THUMB_FN(0xe025ea65, XimrSetLayer_unk2)
//THUMB_FN(0xe025eaed, maybe_XimrSetLayerColorParams)
//THUMB_FN(0xe025e949, XimrContextInputLayerSetDimensions)
//THUMB_FN(0xe021ded9, WinSys_AllocateMemory)
//THUMB_FN(0xe021df7d, WinSys_FreeMemory)


// SJE not yet sure how this one works on 850D,
// looks quite different or I'm reading it wrong
#DATA_PTR(   0xfd84,  bmp_vram_info) // function e04a044c looks relevant, see refs to 0x3c0 and 0x21c; 960x540
                                    // 0xfda0 seems bool for whether it's in 960x540 or 720x480.
                                    // 0xfd70 looks like a struct base, at the end of this func
                                    // you can see fd70 + 0x14, 0x18, 0x1c being used to see which vram buffer
                                    // is active, or ready maybe.

    DATA_PTR(  0x116f0,  _rgb_vram_info) // Used in many places, written to in "InitializeScreen"
                                         // At least for 200D, this is BGRA buffer, earlier in the render
                                         // chain than the YUV ones.  But, changes made to it will not
                                         // render until a refresh occurs (can manually trigger via RefreshVrmsSurface()),
                                         // but direct refresh via XimrExe() is simpler (we think this tells
                                         // the renderer peripheral to take the RGB data and update the screen(s).

    NSTUB(0x0,  LCD_Palette) // Not required / changed in D8
                             // FIXME - presumably we should stop using this stub in source?

    DATA_PTR(    0x4144,  task_dispatch_hook) // find usage in 200D, trace up call hierarchy to task_create,
                                              // trace back down same hierarchy in 850D
    THUMB_FN(0xe059bd78, _prop_request_change)
    THUMB_FN(0xe05a9b1a,  dm_set_store_level) // called by "dmstore"
                                          // Called in init_task(), shortly before ICU Firmware version string
                                          // is logged.
    THUMB_FN(0xe00bb808,  task_trampoline)


#================================================================
# Anything below here is not required, but useful during dev work

THUMB_FN(0xe05aa0c6, dumpf);
