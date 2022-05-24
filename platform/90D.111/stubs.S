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
    THUMB_FN(0xe0040104,  cstart)                 /* calls bzero32 and create_init_task(..., init_task, ...)  MFD: PROBABLY SAME */
    THUMB_FN(0xe05cd190,  bzero32)                /* zeros out a data structure  MFD:UPDATED WITH NEW VALUE*/
    THUMB_FN(0xe02e04bc,  create_init_task)       /* low-level DryOS initialization MFD: PROBABLY SAME*/
    THUMB_FN(0xe0040236,  init_task)              /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc  MFD: PROBABLY SAME*/
    THUMB_FN(0xe05cbe00,  dcache_clean)           /* loop with MCR p15 c7,c10,1; DSB   MFD: UPDATED WITH NEW VALUE */
    THUMB_FN(0xe05cbe7c,  icache_invalidate)      /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB   MFD:UPDATED WITH NEW VALUE*/

/** Tasks **/
    THUMB_FN(0xe0642626,  task_create)            /* used to start TaskMain, GuiMainTask etc MFD: UPDATED WITH POSSIBLE ENTRY */
//THUMB_FN(0xdf008f0b,  task_create_ex)         /* as task_create, but allows selecting CPU for task*/
    THUMB_FN(0xe05bc40e,  msleep)                 /* argument is always multiple of 10  MFD: UPDATED WITH POSSIBLE ENTRY */
    DATA_PTR(    0x1028,  current_task)           /* from task_create; pointer to the current task structure   MFD: possibly same */
    DATA_PTR(    0x1010,  current_interrupt)      /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */

/** Dumper **/
    THUMB_FN(0xe0222804,  dump_file)              /* MFD:UPDATED WITH NEW VALUE tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush     MFD: UPDATED WITH NEW VALUE**/

/** Memory info **/
    THUMB_FN(0xe018c13c,  malloc_info)            /* Malloc Information MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe018c1d4,  sysmem_info)            /* System Memory Information MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe017f194,  memmap_info)            /* Exception vector, DRYOS system memory etc   MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe011c2e8,  smemShowFix)            /* Common Lower, Common Upper etc FD: UPDATED WITH POSSIBLE ENTRY  VERY LOW CONFIDENCE */

/** Memory allocation **/
    THUMB_FN(0xe05bcd74, _AllocateMemory)      /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05bcf2c, _FreeMemory)		/*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05bcbfe,  GetMemoryInformation)   /* called from AllocateMemory MFD: UPDATED WITH POSSIBLE ENTRY  */
    THUMB_FN(0xe05bcbd2,  GetSizeOfMaxRegion)    /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05bc818, _alloc_dma_memory)			/*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05bc80c, _free_dma_memory)			/*MFD: UPDATED WITH POSSIBLE ENTRY */
#THUMB_FN(0xE0693B23,  malloc)
#THUMB_FN(0xE0693B5B,  free)

/** Debug messages **/
    THUMB_FN(0xe05cc3e4,  DryosDebugMsg)          /* /*MFD: UPDATED WITH POSSIBLE ENTRY */ //lots of debug messages; format string is third argument */

/** DMA **/
    THUMB_FN(0xe05f73a2,  dma_memcpy)  /*MFD: UPDATED WITH POSSIBLE ENTRY */

/** Eventprocs (call by name) **/
    THUMB_FN(0xe05b2f0e,  call)                   /* many functions called by name (lv_start, lv_stop etc) */   /*MFD: UPDATED WITH POSSIBLE ENTRY */

/** File I/O **/
    THUMB_FN(0xe05aeb54, _FIO_OpenFile)			/*MFD: UPDATED WITH POSSIBLE ENTRY  --low confidence*/
    THUMB_FN(0xe05aec06, _FIO_CreateFile) /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05aefde, _FIO_ReadFile)		/*MFD: UPDATED WITH POSSIBLE ENTRY  --low confidence*/
    THUMB_FN(0xe05af190, _FIO_WriteFile)     /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05af0d2,  FIO_SeekSkipFile) /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05af3ac,  FIO_CloseFile)     /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05b0432, _FIO_CreateDirectory)   /*MFD: UPDATED WITH POSSIBLE ENTRY */
//THUMB_FN(0xE04BD061,  FIO_FindFirst)
    THUMB_FN(0xe05b088c, _FIO_FindFirstEx)   /*MFD: UPDATED WITH POSSIBLE ENTRY */
//THUMB_FN(0xE04BD0EF,  FIO_FindNextEx) // SJE this looks like FIO_FindNext to me, not Ex
    THUMB_FN(0xe05b0988,  FIO_FindNextEx)    /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05b0a4a,  FIO_FindClose)		 /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05af546, _FIO_GetFileSize64)  /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05aef46, _FIO_RemoveFile)    /*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05affaa, _FIO_RenameFile)		/*MFD: UPDATED WITH POSSIBLE ENTRY */
    THUMB_FN(0xe05b0616,  FIO_Flush)               // to be called after FIO_CloseFile? /*MFD: UPDATED WITH POSSIBLE ENTRY */
//THUMB_FN(0xe05afa10,  FIO_FInfo)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b086e,  FIO_SetErr)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b0798,  FIO_DelayFlush)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b0582,  FIO_RemDir)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b019e,  FIO_Divide)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b00b6,  FIO_Concat)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05afeb0,  FIO_ChopT)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05afdc2,  FIO_ChopH)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05afc30,  FIO_ChgCreateTime)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05afb6e,  FIO_FreeEntCnt)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05af95c,  FIO_SetFTime)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05af87a,  FIO_GetFTime)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05af73e,  FIO_SetAttr)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05af46c,  FIO_CloseSC)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b0cd2,  FIO_Mount)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b0d64,  FIO_UnMount)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05af262,  FIO_ChainW)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/
//THUMB_FN(0xe05b04d8,  FIO_CreateDirEx)    /*MFD: UPDATED WITH POSSIBLE ENTRY added just in case*/









/** GUI **/
//THUMB_FN(0xe04044ce,  GUI_Control) // Looks missing on 850D, possibly same as R 
    THUMB_FN(0xee059377e,  SetGUIRequestMode) // Code is quite different from earlier cams, not a big switch? MFD: UPDATED WITH POSSIBLE ENTRY. CODE IS A LITTLE DIFFERENT
                                             // Possibly weird decomp?  Param is the same though, maybe it still works
    THUMB_FN(0xee057c5d0,  gui_init_end) // DNE on R, but does on 850D MFD: UPDATED WITH POSSIBLE ENTRY
    THUMB_FN(0xe020b240,  gui_main_task)    //MFD: UPDATED WITH POSSIBLE ENTRY
    THUMB_FN(0xe020b376,  gui_enqueue_message) // "warning! QueueLength=%d" MFD: UPDATED WITH POSSIBLE ENTRY LOW CONFIDENCE

/** GUI timers **/
    THUMB_FN(0xe05a92ce,  CancelTimer)					//MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT
    THUMB_FN(0xe030a2fe,  SetHPTimerAfterNow)		//MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT
    THUMB_FN(0xe030a36e,  SetHPTimerNextTick)     /* same "worker" function as SetHPTimerAfterNow  MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT*/
    THUMB_FN(0xe05a9218,  SetTimerAfter)		//MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT
    THUMB_FN(0xe06c26a6,  CancelDateTimer)    //MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT

/** Interrupts **/
//DATA_PTR(    0x4030,  pre_isr_hook)
//DATA_PTR(    0x4034,  post_isr_hook)
//DATA_PTR(   0x6CC10,  isr_table_handler)
//DATA_PTR(   0x6CC14,  isr_table_param)
    THUMB_FN(0xe01ab1dc,  cli_spin_lock)          /* used in AllocateMemory/FreeMemory and others */  //MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT

/** MPU communication **/
    THUMB_FN(0xe008ba68,  mpu_send) // "InterCom::Siodriver.c" //MFD: UPDATED WITH POSSIBLE ENTRY CODE IS A LITTLE DIFFERENT
    THUMB_FN(0xe0641e4c,  mpu_recv) // "ReceiveCBR : Unknown Command" //MFD: ReceiveCBR : Unknown Command IS THERE, HOWEVER THE CODE IS QUITE DIFFERENT
    DATA_PTR(    0xafa4,  mpu_recv_cbr) // search for Direct Refs to mpu_recv address, function pointer should be found,
                                        // find the function that uses it, which stores it; address where stored is mpu_recv_cbr
    DATA_PTR(   0x5b808,  mpu_send_ring_buffer)      // ring buffer used in mpu_send
    DATA_PTR(    0xaf98,  mpu_send_ring_buffer_tail) // ring buffer index incremented in mpu_send
    DATA_PTR(   0x5b6c8,  mpu_recv_ring_buffer)      // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
    DATA_PTR(    0xafb0,  mpu_recv_ring_buffer_tail) // ring buffer index incremented in the above subroutine

/** Misc **/
    THUMB_FN(0xe0069bf8,  vsnprintf)              /* called near dmstart; references "01234567", "0123456789", "0123456789abcdef" //MFD: UPDATED WITH POSSIBLE ENTRY
                                                 and "0123456789ABCDEF"; second arg is size; the one called by DebugMsg only knows %s */

// SJE stuff found during investigation, maybe not needed for ML
// but I wanted to call by name
    THUMB_FN(0xe03dcc78,  debug_assert) // DRYOS_ASSERT_HANDLER related MFD: updated with possible entry

// SJE trying to get a substitute vsync callback working for screen refresh
//DATA_PTR(    0xfd78,  winsys_sem_count)
//DATA_PTR(    0xfec8,  bmp_vsync_cbr)
//THUMB_FN(0xe049e96c,  BmpVsyncCallBack)

// UNSURE or untested
    THUMB_FN(0xe00b0822,  get_task_by_id) // called indirectly by task_create and functions using string "TASK Name" //MFD: updated with possible entry
    THUMB_FN(0xe0397950, _get_task_info_by_id)  //MFD: updated with possible entry
    NSTUB(    0x50c4,  dm_names) // in DebugMsg, before the 1st loop target
    NSTUB(   0x14514,  sd_device) // in sdReadBlk, probably 0xe02b6fa2.  Lots of refs
                                  // to 0x1fd7c, some to 0x1fd80
    NSTUB(    0x53ec,  gui_main_struct) // See function using "Queue Clear event" string, e00c55b6
                                        // There's a base of struct, and some refs to fields via offsets.
                                        // Find the right one.
//THUMB_FN(0xe00c54ab,  GUI_ChangeMode) // "GUI_ChangeMode:%d" string
    THUMB_FN(0xe0592018,  DispSensorStart) // 0x86f4 looks to be a bool for whether Sensor is started  //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05bb740,  DeleteMemorySuite)   //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe0599ec0,  CreateMemorySuite)    //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05bb012,  CreateMemoryChunk) // called from CreateMemorySuite  //MFD: REPLACED WITH POSSIBLE Entry

//NSTUB(0xe05bb6c2,  GetSizeOfMemorySuite) // not needed, but I found it along the way //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05cbff0,  take_semaphore) // Fairly sure. Same function sig and followed by give_semaphore //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05cc06a,  give_semaphore)  //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05cbef4,  create_named_semaphore) // also see df00b114, very similar, but df008418 is closer to 50D create_named_semaphore.
                                              // Complicated by there being df00b648, which wraps 8418 with an extra assert possible,
                                              // and e0426a20, which is what most sites call, which calls 8418 - but thunked.
                                              // In theory that means DryOS could replace the underlying create_named_semaphore
                                              // at run-time.  No evidence that they do.
											  //MFD: UPDATED WITH POSSIBLE VALUE

    THUMB_FN(0xe04065f4,  gui_change_mode) // GUIGate_PostChangeMode  //MFD: UPDATED WITH POSSIBLE VALUE
    THUMB_FN(0xe04044ce,  gui_massive_event_loop) // various "GUICMD_"* strings  //MFD: UPDATED WITH POSSIBLE VALUE
//THUMB_FN(0x,  gui_init_event) // "-> handleGuiInit"

    // The various memory functions can't be found by debug_assert strings in 850D
    // because the first field to debug_assert is always 0.
    // Instead, check call sites of the function, find those, find equivalent call
    THUMB_FN(0xe05bb4f0,  AddMemoryChunk) // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
                                          // maybe 0xe04ba536? Similar, except it returns void, not int.
                                          // Also see 0xe04ba494, v. similar but diff struct offsets 
										   //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05bb574,  GetFirstChunkFromSuite)   //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05bbd94,  GetNextMemoryChunk)    //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05bb336,  GetMemoryAddressOfMemoryChunk)   //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe0765370,  AllocateMemoryResource)   //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe076539e,  AllocateContinuousMemoryResource) //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe07653cc,  FreeMemoryResource) //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05b2c24,  msg_queue_receive) // "SystemIF::KerQueue.c" useful for finding some of these msg_ functions  MFD: REPLACED WITH POSSIBLE Entry


    THUMB_FN(0xe05b2d82,  msg_queue_post) // used in gui_enqueue_message  MFD: REPLACED WITH POSSIBLE Entry

    THUMB_FN(0xe05b2db8,  msg_queue_count) // Fairly hard to find.  Often used near DispLockStop, which is easy to find
                                           // and not used often. MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05b2b60,  msg_queue_create) // CreateMessageQueue - In 50D, msg_queue_create calls 0xff866424,
                                        // the equiv of which is 0xdf00b114 (and was easier to find) MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe05bdea8,  prop_deliver) // used near "UNKNOWN PROPERTY ID " in call to DryosDebugMsg   MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe05bd9b2,  prop_register_slave)  // MFD: REPLACED WITH POSSIBLE Entry
    DATA_PTR(0xeb80,      LiveViewApp_dialog) // in StartLiveViewApp()
    THUMB_FN(0xe044ffae,  LiveViewApp_handler) // DebugMsg: "StartLiveViewApp(%#x)", address passed to CreateDialogBox MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe07a3bd0,  LiveViewShutterApp_handler) // DebugMsg: "StartLiveViewShutterApp", address passed to CreateDialogBox  MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe032c6c0,  PlayMovieGuideApp_handler) // "PlayMovieGuide_Start"    MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe0561afa,  PlayMain_handler) // DebugMsg: "StartPlayMain", address passed to CreateDialogBox  MFD: REPLACED WITH POSSIBLE Entry low confidence. code is quite different
    THUMB_FN(0xe0704a2e,  ShootOlcApp_handler) // near "StopShootOlcApp PopPalette(%d)" MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0x0e08dd04e,  GetCFnData) // "GetCFnData" Function sig is different but the body looks similar.  MFD: REPLACED WITH POSSIBLE Entry
                                      // Looks like the inner functions use registers the outer doesn't, which confuses Ghidra
                                      // re how many args outer takes.
    THUMB_FN(0xe08dc6d0,  SetCFnData) // There are some "SetCFnData" related strings MFD: REPLACED WITH POSSIBLE Entry
                                      // SJE size of struct has changed, don't know if this is relevant

// Not required on 200D given usage by ML is always behind cam specific guards.
//THUMB_FN(0xe069ce15,  GUI_SetLvMode) // via "GUI_SetBracketReset" - but this takes void now, and overwrites r0,
                                     // whereas ML thinks it takes int.  Either this is wrong, or will
                                     // need wrapping to preserve the reg.

    THUMB_FN(0xe0757df4, _malloc) // These two names are used in 50D and CONFIG_HELLO_WORLD won't build without them,  MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe0757e2c, _free)  // however, the inital stubs for 200D call these simply malloc/free.  MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe054f5c6,  SRM_AllocateMemoryResourceFor1stJob) // "hJobMemorySuite" strings, function a little earlier  //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05519ec,  SRM_FreeMemoryResourceFor1stJob) // find refs to "pMessage", there are several similar looking functions using it,
                                                           // this is my best guess - this one has pPriorMessage and sets offset 8
                                                           // in the struct to 0, offset 0x10 to param1
														   //MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe05bdf5c, _prop_cleanup) // "./Multicast/PropertyMgr.c" check funcs that ref this string  MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe05cb5a4,  CreateRecursiveLock) // via "KerRLock.c", CRL() calls a function that regs this string, not many do  MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe05a9478,  AcquireRecursiveLock)  //MDF: REPLACED WITH POSSIBLE ENTRY
    THUMB_FN(0xe05a9542,  ReleaseRecursiveLock)  //MDF: REPLACED WITH POSSIBLE ENTRY
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
    THUMB_FN(0xe083c674, _LoadCalendarFromRTC)  //MDF: REPLACED WITH POSSIBLE ENTRY
 

// These are required on Digic 678
    THUMB_FN(0xe02c08ec, RefreshVrmsSurface) // "Call RefreshVrmsSurface" / "XimrExe"  MDF: REPLACED WITH POSSIBLE ENTRY
    DATA_PTR(   0x11738,  display_refresh_needed) // if 0, RefreshVrmsSurface() does nothing
    THUMB_FN(0xe0194248, XimrExe) // trigger Ximr unit to execute (HW compositor, partially understood) MDF: REPLACED WITH POSSIBLE ENTRY
    DATA_PTR(   0x11724,  winsys_sem)

// These are not strictly required, although recent dev work has added dependencies;
// these likely should be removed before a "real" release
    THUMB_FN(0xe05b6f74, uart_printf)  //MDF: REPLACED WITH POSSIBLE ENTRY
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
//THUMB_FN(0xe01fa5e8, WinSys_FreeMemory)   MFD: REPLACED WITH POSSIBLE Entry


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
    THUMB_FN(0xe05bda80, _prop_request_change)  //MFD: REPLACED WITH POSSIBLE Entry
    THUMB_FN(0xe05cc7de,  dm_set_store_level) // called by "dmstore"
                                          // Called in init_task(), shortly before ICU Firmware version string
										  //  MFD: REPLACED WITH POSSIBLE Entry  under "[DM] Priority Escalation!!!!"
										  //  MFD: REPLACED WITH POSSIBLE Entry  under "[DM] Priority Escalation!!!!"
                                          // is logged.
    THUMB_FN(0xe00b08dc,  task_trampoline)   //  MFD: REPLACED WITH POSSIBLE Entry


#================================================================
# Anything below here is not required, but useful during dev work

THUMB_FN(0xe05aa0c6, dumpf);  //  MFD: REPLACED WITH POSSIBLE Entry
