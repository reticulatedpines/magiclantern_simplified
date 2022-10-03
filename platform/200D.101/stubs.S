/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the EOS 200D.
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
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xE00400FD,  cstart)                 /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xDF00D299,  bzero32)                /* zeros out a data structure */
THUMB_FN(0xDF006515,  create_init_task)       /* low-level DryOS initialization */
THUMB_FN(0xE0040225,  init_task)              /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */
THUMB_FN(0xE06BE169,  dcache_clean)           /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xe0013dcc,  dcache_clean_multicore) /* loop writing to c11007b0, called after dcache_clean by copy_mmu_tables() */
THUMB_FN(0xE06BE23D,  icache_invalidate)      /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */

/** Tasks **/
THUMB_FN(0xDF008CE7,  task_create)            /* used to start TaskMain, GuiMainTask etc */
THUMB_FN(0xdf008f0b,  task_create_ex)         /* as task_create, but allows selecting CPU for task*/
THUMB_FN(0xDF00880F,  msleep)                 /* argument is always multiple of 10 */
DATA_PTR(    0x1028,  current_task)           /* from task_create; pointer to the current task structure */
DATA_PTR(    0x1010,  current_interrupt)      /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */

/** Dumper **/
THUMB_FN(0xE007EE5B,  dump_file)              /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Memory info **/
THUMB_FN(0xE023D3C9,  malloc_info)            /* Malloc Information */
THUMB_FN(0xE023D461,  sysmem_info)            /* System Memory Information */
THUMB_FN(0xE01CA9C9,  memmap_info)            /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xE0148E3B,  smemShowFix)            /* Common Lower, Common Upper etc */

/** Memory allocation **/
THUMB_FN(0xDF007B59, _AllocateMemory)
THUMB_FN(0xDF007D11, _FreeMemory)
THUMB_FN(0xDF0079E3,  GetMemoryInformation)   /* called from AllocateMemory */
THUMB_FN(0xDF0079B7,  GetSizeOfMaxRegion)
THUMB_FN(0xDF00AC55, _alloc_dma_memory)
THUMB_FN(0xDF00AC81, _free_dma_memory)
//THUMB_FN(0xE0693B23,  malloc)
//THUMB_FN(0xE0693B5B,  free)

/** Debug messages **/
THUMB_FN(0xDF006E6D,  DryosDebugMsg)          /* lots of debug messages; format string is third argument */

/** DMA **/
THUMB_FN(0xE01B3297,  dma_memcpy)

/** Eventprocs (call by name) **/
THUMB_FN(0xE04BDB8F,  call)                   /* many functions called by name (lv_start, lv_stop etc) */

/** File I/O **/
THUMB_FN(0xE04BC1ED, _FIO_OpenFile)
THUMB_FN(0xE04BC257, _FIO_CreateFile)
THUMB_FN(0xE04BC311, _FIO_ReadFile)
THUMB_FN(0xE04BC3B7, _FIO_WriteFile)
THUMB_FN(0xE04BD935,  FIO_SeekSkipFile)
THUMB_FN(0xE04BC429,  FIO_CloseFile)
THUMB_FN(0xE04BCD51, _FIO_CreateDirectory)
//THUMB_FN(0xE04BD061,  FIO_FindFirst)
THUMB_FN(0xE04BD17b, _FIO_FindFirstEx)
//THUMB_FN(0xE04BD0EF,  FIO_FindNextEx) // SJE this looks like FIO_FindNext to me, not Ex
THUMB_FN(0xe04bd212,  FIO_FindNextEx)
THUMB_FN(0xE04BD15B,  FIO_FindClose)
THUMB_FN(0xE04BC547, _FIO_GetFileSize)
THUMB_FN(0xE04BC2C5, _FIO_RemoveFile)
THUMB_FN(0xE04BCBCB, _FIO_RenameFile)
THUMB_FN(0xE04BCE75,  FIO_Flush)               // to be called after FIO_CloseFile?

/** GUI **/
THUMB_FN(0xE00C5415,  GUI_Control)
THUMB_FN(0xE04A7EF5,  SetGUIRequestMode)
THUMB_FN(0xE00921DD,  gui_init_end)
THUMB_FN(0xE00C526B,  gui_main_task)

/** GUI timers **/
THUMB_FN(0xE04B9A3B,  CancelTimer)            /* from error message */
THUMB_FN(0xE05301DF,  SetHPTimerAfterNow)     /* from error message */
THUMB_FN(0xE0530233,  SetHPTimerNextTick)     /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xE04B9985,  SetTimerAfter)          /* from error message */
THUMB_FN(0xE05F9DB5,  CancelDateTimer)

/** Interrupts **/
DATA_PTR(    0x4030,  pre_isr_hook)
DATA_PTR(    0x4034,  post_isr_hook)
DATA_PTR(   0x6CC10,  isr_table_handler)
DATA_PTR(   0x6CC14,  isr_table_param)
THUMB_FN(0xE0137001,  cli_spin_lock)          /* used in AllocateMemory/FreeMemory and others */
THUMB_FN(0xe0137048,  send_software_interrupt) // int 10 is used by DryOS to sleep/wake cpu1
//THUMB_FN(0xe0004d30,  wfi_sinkhole) // used to make cpu wait for int 10

/** MPU communication **/
THUMB_FN(0xE01C7657,  mpu_send)                  // "dwSize < TXBD_DATA_SIZE"
THUMB_FN(0xE056314F,  mpu_recv)                  // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x79C0,  mpu_recv_cbr)              // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x87AD8,  mpu_send_ring_buffer)      // ring buffer used in mpu_send
DATA_PTR(    0x79A4,  mpu_send_ring_buffer_tail) // ring buffer index incremented in mpu_send
DATA_PTR(   0x87998,  mpu_recv_ring_buffer)      // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x799C,  mpu_recv_ring_buffer_tail) // ring buffer index incremented in the above subroutine

/** Misc **/
THUMB_FN(0xE11C7CF9,  vsnprintf)              /* called near dmstart; references "01234567", "0123456789", "0123456789abcdef"
                                                 and "0123456789ABCDEF"; second arg is size; the one called by DebugMsg only knows %s */

// SJE stuff found during investigation, maybe not needed for ML
// but I wanted to call by name
THUMB_FN(0xe049e9bf,  SaveVRAM)
THUMB_FN(0xe05f1c94,  debug_assert) // DRYOS_ASSERT_HANDLER related
THUMB_FN(0xe0152d8b,  set_rec_mode) // possibly sets Movie mode, e.g., IPB, ALL-I
THUMB_FN(0xe0152689,  c2rec) // Cave2 - might start mp4 recording?
THUMB_FN(0xe0152775,  c2stop) // Cave2 - might stop recording?
THUMB_FN(0xe0153159,  c2install) // Cave2 - looks like an init function
THUMB_FN(0xe0150fd7,  c2command) // Cave2 - takes a command parameter, see movie_state_name_mappings,
                                 // 0xe0aa8120 on 200D 1.0.1

// SJE trying to get a substitute vsync callback working for screen refresh
DATA_PTR(    0xfd78,  winsys_sem_count)
DATA_PTR(    0xfd7c,  winsys_sem)
DATA_PTR(    0xfec8,  bmp_vsync_cbr)
THUMB_FN(0xe049e96c,  BmpVsyncCallBack)

// call("TurnOffDisplay") returns -1 on 200D, no longer exists?
THUMB_FN(0xe048fcac, _turn_off_display)
THUMB_FN(0xe048fc98, _turn_on_display)

// SJE unsure stuff: currently majority copied from 50D (I have roms for this),
// or 100D (more modern, presumably sort of similar)
//
// Alex recommended making a dummy function for stubs that can't be located,
// and leaving the stub blank.  50D does this in misc.c (can choose a better name)

// UNSURE or untested
THUMB_FN(0xdf0031a3,  get_task_by_id) // called indirectly by task_create and functions using string "TASK Name"
THUMB_FN(0xdf004041,  _get_task_info_by_id) // e023ccc7 calls this thunked func, the thunked one has right number of params for ML
NSTUB(    0x4ea4,  dm_names) // in DebugMsg, before the 1st loop target
NSTUB(   0x1fd7c,  sd_device) // in sdReadBlk, probably 0xe02b6fa2.  Lots of refs
                              // to 0x1fd7c, some to 0x1fd80
NSTUB(    0x4838,  gui_main_struct) // See function using "Queue Clear event" string, e00c55b6
                                    // There's a base of struct, and some refs to fields via offsets.
                                    // Find the right one.
THUMB_FN(0xe00c54ab,  GUI_ChangeMode) // "GUI_ChangeMode:%d" string
THUMB_FN(0xe04b907d,  DispSensorStart) // 0xe794 looks to be a bool for whether Sensor is started
THUMB_FN(0xe04ba81d,  DeleteMemorySuite)
THUMB_FN(0xe04baa5d,  CreateMemorySuite)
THUMB_FN(0xe04b9f89,  CreateMemoryChunk) // called from CreateMemorySuite
//NSTUB(0xe04ba7e6,  GetSizeOfMemorySuite) // not needed, but I found it along the way
THUMB_FN(0xdf008515,  take_semaphore) // Fairly sure. Same function sig and followed by give_semaphore
THUMB_FN(0xdf00858f,  give_semaphore)
THUMB_FN(0xdf008419,  create_named_semaphore) // also see df00b114, very similar, but df008418 is closer to 50D create_named_semaphore.
                                              // Complicated by there being df00b648, which wraps 8418 with an extra assert possible,
                                              // and e0426a20, which is what most sites call, which calls 8418 - but thunked.
                                              // In theory that means DryOS could replace the underlying create_named_semaphore
                                              // at run-time.  No evidence that they do.

THUMB_FN(0xe02d35ab,  gui_change_mode) // GUI_ChangeMode_Post
THUMB_FN(0xe02d1d51,  gui_massive_event_loop) // various "GUICMD_"* strings
THUMB_FN(0xe00979d9,  gui_init_event) // "-> handleGuiInit"
THUMB_FN(0xe04ba537,  AddMemoryChunk) // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
                                      // maybe 0xe04ba536? Similar, except it returns void, not int.
                                      // Also see 0xe04ba494, v. similar but diff struct offsets
THUMB_FN(0xe04ba5bb,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
THUMB_FN(0xe04bae8b,  GetNextMemoryChunk)                      // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
THUMB_FN(0xe04ba3d5,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature - 200D uses "MemChunk" directly
THUMB_FN(0xe062df0b,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
THUMB_FN(0xe062df39,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
THUMB_FN(0xe062dfdf,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
THUMB_FN(0xdf00b1d9,  msg_queue_receive) // "SystemIF::KerQueue.c" useful for finding some of these msg_ functions

// This was my first guess for msg_queue_post, and it's very similar.
//THUMB_FN(0xdf00b26b,  msg_queue_post)

// Second guess for msg_queue_post, I think this is right, if you look
// at the function it calls, the checks inside that are a closer match.
THUMB_FN(0xdf00b337,  msg_queue_post)

THUMB_FN(0xdf00b36d,  msg_queue_count) // Fairly hard to find.  Often used near DispLockStop, which is easy to find
                                       // and not used often.
                                       // This is the thunked address, which is called from e042680c
//THUMB_FN(0xdf00b17d,  msg_queue_count) // Above "!!!!!!! QUE Overflo" - maybe 0xdf00b17c.  Not sure here,
                                         // params looks weird but I think decompilation may have confused
                                         // stack variables with params.  Code itself similar.
                                         //
                                         // Wrong, kept for history.
THUMB_FN(0xdf00b615,  msg_queue_create) // CreateMessageQueue - In 50D, msg_queue_create calls 0xff866424,
                                        // the equiv of which is 0xdf00b114 (and was easier to find)
THUMB_FN(0xe04bba45,  prop_deliver)
//THUMB_FN(0xe04bbde1,  prop_register_slave) // called by"GUI_RegisterPropertySlave"
THUMB_FN(0xe04bb4df,  prop_register_slave) // "pRegisterSlave"

THUMB_FN(0xe0381ba9,  LiveViewApp_handler) // DebugMsg: "StartLiveViewApp(%#x)", address passed to CreateDialogBox
//THUMB_FN(0xe06cda95,  LiveViewShutterApp_handler) // DebugMsg: "StartLiveViewShutterApp CreateDialogBox(%d)", address passed to CreateDialogBox
THUMB_FN(0xe046fc49,  PlayMain_handler) // DebugMsg: "StartPlayMain", address passed to CreateDialogBox

// See R180 stubs.s for details.
//DATA_PTR(    0xec7c,  PlayMain_dialog)                    // in StartPlayMain()
//DATA_PTR(   0x1af3c,  ShootOlcApp_dialog)                 // in StartShootOlcApp()
DATA_PTR(      0xeb44,  LiveViewApp_dialog)                 // in StartLiveViewApp()
//DATA_PTR(   0x1a970,  LiveViewShutterApp_dialog)          // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR(   0x1be20,  PlayMovieGuideApp_dialog)           // in StartPlayMovieGuideApp()

THUMB_FN(0xe03536c1,  GetCFnData) // "GetCFnData" Function sig is different but the body looks similar.
                                  // Looks like the inner functions use registers the outer doesn't, which confuses Ghidra
                                  // re how many args outer takes.
THUMB_FN(0xe0353761,  SetCFnData) // There are some "SetCFnData" related strings

// Not required on 200D given usage by ML is always behind cam specific guards.
//THUMB_FN(0xe069ce15,  GUI_SetLvMode) // via "GUI_SetBracketReset" - but this takes void now, and overwrites r0,
                                     // whereas ML thinks it takes int.  Either this is wrong, or will
                                     // need wrapping to preserve the reg.

THUMB_FN(0xe0693b23, _malloc) // These two names are used in 50D and CONFIG_HELLO_WORLD won't build without them,
THUMB_FN(0xe0693b5b, _free)   // however, the inital stubs for 200D call these simply malloc/free.
                           // I'm leaving them defined with both names, although that seems wrong.
THUMB_FN(0xe04cff73,  SRM_AllocateMemoryResourceFor1stJob) // "hJobMemorySuite" strings, function a little earlier
THUMB_FN(0xe04d2e17,  SRM_FreeMemoryResourceFor1stJob) // find refs to "pMessage", there are several similar looking functions using it,
                                                       // this is my best guess - this one has pPriorMessage and sets offset 8
                                                       // in the struct to 0, offset 0x10 to param1
THUMB_FN(0xe04bbad7, _prop_cleanup) // "./Multicast/PropertyMgr.c" check funcs that ref this string
THUMB_FN(0xdf00b67d,  CreateRecursiveLock) // via "KerRLock.c", CRL() calls a function that regs this string, not many do
THUMB_FN(0xdf00b451,  AcquireRecursiveLock)
THUMB_FN(0xdf00b51b,  ReleaseRecursiveLock)
THUMB_FN(0xe04b1d75,  dialog_redraw) // Found via checking xrefs to "pDialog->pSignature == pcSignature"
THUMB_FN(0xe04b3739,  CreateResLockEntry) // via xrefs to a pointer to "ResInfo"
THUMB_FN(0xe04b3b59,  LockEngineResources) //  Down a bit from CreateResLockEntry, start v similar but end is different.
THUMB_FN(0xe06217f1,  ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

THUMB_FN(0xdf00d365,  fsuDecodePartitionTable) // SJE FIXME wrong, see notes below.
//THUMB_FN(0xdf00d365,  fsuDecodePartitionTable) // this is called in one place in 50D, 0xffb0a18c, which is 0xe02b6678 on 200D.
                                               // The function called on 200D looks a lot different from fsuDecodePartitionTable, however.
                                               // Constant offsets used are the same in both, looks like fsuDPT has been partially 
                                               // inlined in 200D.  We can't call it directly.
                                               // The decompilation looks weird too, claims it just returns
                                               // the calling param but it clearly twiddles some registers.
                                               //
                                               // ML looks to use this for writing to bootsector of card, either for installer,
                                               // or preserving ML on the card after an in-cam format.

THUMB_FN(0xe05f67d1,  ShootOlcApp_handler) // near "StopShootOlcApp PopPalette(%d)"
//DATA_PTR(    0x40d0,  first_task) // unsure on naming here.  First item in an array of task structs.
                                  // Seems to always be "idle" task.
                                  // (we don't use this, but instead calculate it during early init,
                                  //  to save backporting this stub for all older cams)
NSTUB(    0x40f0,  task_max)
NSTUB(    0xfc88,  gui_task_list) // "pDialog" strings often have refs to 0xfcb4, if so, should be
                                  // around 20 before that? "pDeleteController->psSignature" also distinctive.
                                  // Found in function 0xe0660c90.
//THUMB_FN(0xe05cd1ff, _LoadCalendarFromRTC) // 0xff885058 in 50D is 0xe00742fc in 200D.  In 50D this calls
                                           // LoadCalendarFromRTC, which takes one arg, the first local var;
                                           // but really this is a local struct of six fields, ie, the next 5 locals
                                           // are initialised by the call (Ghidra can't work out func takes pointer to struct?).
                                           // In 200D a function is called which does take the first local var,
                                           // along with initialising the next 5.  But this function takes 4 other args,
                                           // and the code within looks really different.  One of those args is a pointer to function,
                                           // with that function being semaphore related.  Multi-core sync stuff?  Could
                                           // make sense given RTC related.
THUMB_FN(0xe0642801, _LoadCalendarFromRTC)

// These are required on Digic 678
THUMB_FN(0xe04a044d, RefreshVrmsSurface) // "Call RefreshVrmsSurface"
DATA_PTR(   0xfd94,  display_refresh_needed) // if 0, RefreshVrmsSurface() does nothing
THUMB_FN(0xe01ef519, XimrExe) // trigger Ximr unit to execute (HW compositor, partially understood)

// These are not strictly required, although recent dev work has added dependencies;
// these likely should be removed before a "real" release
THUMB_FN(0xe04bdc5d, uart_printf)
#NSTUB(0xdf008c7c, uart_printf) // Thunked equivalent of the above.  Note, ARM, not Thumb.
                               // Both seem to work the same.

// These are not required, but have been used for experiments, debugging, etc
THUMB_FN(0xe049f041, fill_rectangle_to_vram) // "FillRectangleToVram ILLEGAL_PARAMETER"
THUMB_FN(0xe02b07c3, marv_dcache_clean) // find _rgb_vram_info, this function is called with
                                        // that address as only param, after null check.
                                        // Function inspects the struct and does dcache_clean
                                        // on the extent of the bitmap_data.  Not referenced
                                        // much.  Possibly only used during init?
THUMB_FN(0xe02ba483, JediDraw) // maybe sends a buffer to 2d accel unit, for output / render?
THUMB_FN(0xe048f6e7, OsdReverseMode) // "OsdReverseMode" - does this flip the OSD output?
THUMB_FN(0xe025e88d, XimrSetLayerVisibility)
THUMB_FN(0xe025e8ad, XimrSetInputLayerMARV)
THUMB_FN(0xe025ea65, XimrSetLayer_unk2)
THUMB_FN(0xe025eaed, maybe_XimrSetLayerColorParams)
THUMB_FN(0xe025e949, XimrContextInputLayerSetDimensions)
THUMB_FN(0xe021ded9, WinSys_AllocateMemory)
THUMB_FN(0xe021df7d, WinSys_FreeMemory)

// MMU related, required by CONFIG_MMU features
THUMB_FN(0xe0004a5a, change_mmu_tables) // Updates TTBRs and does TLB maintenance
THUMB_FN(0xe0013cc4, copy_mmu_tables) // copies from one location to another,
                                      // fixing up addresses held in table start,
                                      // and doing cache maintenance
// slightly MMU related, used by CONFIG_MMU_EARLY_REMAP
ARM32_FN(0xe001654c, memcpy_dryos) // useful in early contexts when we don't have
                                   // dietlibc or newlib, and can't use stdio memcpy64 because
                                   // that wants ML assert handling initialised.
THUMB_FN(0xe00138c2,  early_printf) // printf compatible with bootloader context


DATA_PTR(   0xfd84,  bmp_vram_info) // function e04a044c looks relevant, see refs to 0x3c0 and 0x21c; 960x540
                                    // 0xfda0 seems bool for whether it's in 960x540 or 720x480.
                                    // 0xfd70 looks like a struct base, at the end of this func
                                    // you can see fd70 + 0x14, 0x18, 0x1c being used to see which vram buffer
                                    // is active, or ready maybe.

DATA_PTR(   0xfd80,  _rgb_vram_info) // Used in many places, written to in "InitializeScreen"
                                     // At least for 200D, this is BGRA buffer, earlier in the render
                                     // chain than the YUV ones.  But, changes made to it will not
                                     // render until a refresh occurs (can manually trigger via RefreshVrmsSurface()),
                                     // but direct refresh via XimrExe() is simpler (we think this tells
                                     // the renderer peripheral to take the RGB data and update the screen(s).

DATA_PTR(   0xfda0,  display_output_mode) // on 200D, 1: 960x540, anything else, 720x480

// WRONG
//NSTUB(   0x1D6FC,  cf_device) // in cfReadBlk - probably this is 0xe0400446, but, in 200D,
                              // the struct offset looks to be passed in as r0, and I don't see
                              // the function called from anywhere so I can't work that out yet.
                              // Probably not needed; 200D doesn't have CF.


NSTUB(0x56500000,  LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800
                                // SJE FIXME fake address in safe writable region, may not exist
                                // on 200D (doesn't seem to use indexed colour anywhere)

//NSTUB(    0x30F8,  task_max)
DATA_PTR(    0x412c,  task_dispatch_hook)
//NSTUB(    0x3960,  gui_timer_struct) // in GUI_Control_Post
//NSTUB(    0x7674,  mvr_config)
//NSTUB(0xC0220000,  camera_engine)
//NSTUB(0xDEADBEEF,  ErrCardForLVApp_handler)
NSTUB(0x56500000,  PlayMovieGuideApp_handler) // needed for CONFIG_QEMU, unknown
                                              // SJE FIXME
DATA_PTR(    0xe731,  additional_version) // this gets written to in early code.  Pick a safe area!
//NSTUB(0xFF8542C4,  PowerAudioOutput)
//NSTUB(0xFF85460C,  SetAudioVolumeOut)
//NSTUB(0xFF855290,  SetSamplingRate)
THUMB_FN(0xe04bb5ad, _prop_request_change)
//NSTUB(0xFF85E90C,  EnableImagePhysicalScreenParameter)
THUMB_FN(0xdf0070c5,  dm_set_store_level) // called by "dmstore"
                                          // Called in init_task(), shortly before ICU Firmware version string
                                          // is logged.
THUMB_FN(0xdf00707d,  dm_set_print_level)
THUMB_FN(0xdf00325d,  task_trampoline)
//NSTUB(0xFF8898CC,  gui_local_post)
//NSTUB(0xFF889CA8,  gui_other_post)
//NSTUB(0xFF889DCC,  gui_post_10000085) // should be called post 100000a5 no ? <-- not sure what this means.
//NSTUB(0xFF889E80,  gui_change_shoot_type_post)
//NSTUB(0xFF889F18,  gui_change_lcd_state_post)
//NSTUB(0xFF98A990,  gui_timer_something)
//NSTUB(0xFF968690,  mvrSetDefQScale)
//NSTUB(0xFF9686C8,  mvrSetFullHDOptSize)
//NSTUB(0xFF968874,  mvrFixQScale)

// SJE maybe 0xe007e056 is important.  This loops doing a "DmacCh" related function, and that gets us
// to 0x36bf0 - there's a few functions there that ref edmac related strings!
// 
//NSTUB(0xFF97C8D4,  SetEDmac)
//NSTUB(0xFF97C8F0,  ConnectWriteEDmac)
//NSTUB(0xFF97C918,  ConnectReadEDmac)
//NSTUB(0xFF97C938,  StartEDmac)
//NSTUB(0xFF97C9E4,  AbortEDmac)
//NSTUB(0xFF97CA84,  RegisterEDmacCompleteCBR)
//NSTUB(0xFF97CA98,  UnregisterEDmacCompleteCBR)
//NSTUB(0xFF97CAAC,  RegisterEDmacAbortCBR)
//NSTUB(0xFF97CAE4,  UnregisterEDmacAbortCBR)
//NSTUB(0xFF97CB14,  RegisterEDmacPopCBR)
//NSTUB(0xFF97CB28,  UnregisterEDmacPopCBR)
//NSTUB(0xFF97D794, _EngDrvOut)
//NSTUB(0xFF97D7F8,  shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
//NSTUB(0xFF97D904, _engio_write)
//NSTUB(0xFFA6CE4C,  UnLockEngineResources)

// SJE couldn't find it, some other cameras don't have it, looks like
// audio-common.c has a static array for that.
//NSTUB(0xFF4C466C,  audio_thresholds) // after ptr to "ALVFactorySemSignature"

// SJE not required to build CONFIG_HELLO_WORLD
//NSTUB(0xFF063AA8, _audio_ic_read)
//NSTUB(0xFF063BF4, _audio_ic_write)
//NSTUB(0xFF2045E0,  GUI_SetRollingPitchingLevelStatus)
//NSTUB(0xFF4B39AC,  lv_path_struct)
//NSTUB(0xFF2CFA54,  MirrorDisplay)
//NSTUB(    0x1ED0,  sounddev) // in sounddev_task
//NSTUB(0xFF0537D4,  sounddev_active_in)
//NSTUB(0xFF05346C,  sounddev_task)
//NSTUB(0xFF2CFAB4,  NormalDisplay)
//NSTUB(0xFF2CFA84,  ReverseDisplay)
//NSTUB(0xFF852494,  StartASIFDMADAC)
//NSTUB(0xFF852A80,  StopASIFDMADAC)
//NSTUB(0xFF051950,  StartASIFDMAADC)
//NSTUB(0xFF052088,  SetNextASIFADCBuffer)
//NSTUB(0xFF0521CC,  SetNextASIFDACBuffer)
//NSTUB(0xFF2FDEF8,  HideUnaviFeedBack_maybe)
//NSTUB(0xFF1AA6C8,  mvrSetGopOptSizeFULLHD)
//NSTUB(0xFF9A474C,  GUI_SetMovieSize_a)
//NSTUB(0xFF9A4848,  GUI_SetMovieSize_b)
//NSTUB(0xFF9ED888,  ptp_register_handler)
//NSTUB(0xFF3010F4,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
//NSTUB(0xFF30204C,  LiveViewApp_handler_end)
//NSTUB(0xFFAAB654,  dialog_set_property_str)
