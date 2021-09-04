/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.5 firmware for the EOS 6D Mark II.
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

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
THUMB_FN(0xe00400fd,  cstart)                 /* calls bzero32 and create_init_task(..., init_task, ...) */
THUMB_FN(0xdf00d299,  bzero32)                /* zeros out a data structure */
THUMB_FN(0xdf006515,  create_init_task)       /* low-level DryOS initialization */
THUMB_FN(0xe0040225,  init_task)              /* USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup etc */
THUMB_FN(0xe06b0368,  dcache_clean)           /* loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xe06b043c,  icache_invalidate)      /* loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */

/** Tasks **/
THUMB_FN(0xdf008ce7,  task_create)            /* used to start TaskMain, GuiMainTask etc */
THUMB_FN(0xdf008f0b,  task_create_ex)         /* as task_create, but allows selecting CPU for task*/
THUMB_FN(0xdf00880f,  msleep)                 /* argument is always multiple of 10 */
DATA_PTR(    0x1028,  current_task)           /* from task_create; pointer to the current task structure */
DATA_PTR(    0x1010,  current_interrupt)      /* from interrupt handler (VBAR + 0x18); where the interrupt ID is stored */

/** Dumper **/
THUMB_FN(0xe008099e,  dump_file)              /* tries to save a file to either "A:/%s" or "B:/%s"; calls FIO_RemoveFile/CreateFile/WriteFile/CloseFile/Flush */

/** Memory info **/
THUMB_FN(0xe02898d8,  malloc_info)            /* Malloc Information */
THUMB_FN(0xe0289970,  sysmem_info)            /* System Memory Information */
THUMB_FN(0xe0202728,  memmap_info)            /* Exception vector, DRYOS system memory etc */
THUMB_FN(0xe0175b46,  smemShowFix)            /* Common Lower, Common Upper etc */

/** Memory allocation **/
THUMB_FN(0xdf007b58, _AllocateMemory)
THUMB_FN(0xdf007d10, _FreeMemory)
THUMB_FN(0xdf0079e2,  GetMemoryInformation)   /* called from AllocateMemory */
THUMB_FN(0xdf007946,  GetSizeOfMaxRegion)
THUMB_FN(0xdf00ac54, _alloc_dma_memory)
THUMB_FN(0xdf00ac80, _free_dma_memory)
THUMB_FN(0xe070a8ea,  malloc)
THUMB_FN(0xe070a922,  free)

/** Debug messages **/
THUMB_FN(0xdf006e6c,  DryosDebugMsg)          /* lots of debug messages; format string is third argument */

/** DMA **/
THUMB_FN(0xe01e565a,  dma_memcpy)

/** Eventprocs (call by name) **/
THUMB_FN(0xe04e7356,  call)                   /* many functions called by name (lv_start, lv_stop etc) */

/** File I/O **/
THUMB_FN(0xe04e59b4, _FIO_OpenFile)
THUMB_FN(0xe04e5a1e, _FIO_CreateFile)
THUMB_FN(0xe04e5ad8, _FIO_ReadFile)
THUMB_FN(0xe04e5b7e, _FIO_WriteFile)
THUMB_FN(0xe04e70fc,  FIO_SeekSkipFile)
THUMB_FN(0xe04e5bf0,  FIO_CloseFile)
THUMB_FN(0xe04e6518, _FIO_CreateDirectory)
//THUMB_FN(0x,  FIO_FindFirst)
THUMB_FN(0xe04e6942, _FIO_FindFirstEx)
THUMB_FN(0xe04e69da,  FIO_FindNextEx)
THUMB_FN(0xe04e6922,  FIO_FindClose)
THUMB_FN(0xe04e5d0e, _FIO_GetFileSize)
THUMB_FN(0xe04e5a8c, _FIO_RemoveFile)
THUMB_FN(0xe04e6392, _FIO_RenameFile)
THUMB_FN(0xe04e663c,  FIO_Flush)               // to be called after FIO_CloseFile?

/** GUI **/
THUMB_FN(0xe00dc454,  GUI_Control)
THUMB_FN(0xe04cbed0,  SetGUIRequestMode)
THUMB_FN(0xe0093574,  gui_init_end)
THUMB_FN(0xe00dc2aa,  gui_main_task)

/** GUI timers **/
THUMB_FN(0xe04e2ee2,  CancelTimer)            /* from error message */
THUMB_FN(0xe0625d1e,  SetHPTimerAfterNow)     /* from error message */
THUMB_FN(0xe0625d72,  SetHPTimerNextTick)     /* same "worker" function as SetHPTimerAfterNow */
THUMB_FN(0xe04e2e2c,  SetTimerAfter)          /* from error message */
THUMB_FN(0xe06e3cb8,  CancelDateTimer)

/** Interrupts **/
DATA_PTR(    0x4030,  pre_isr_hook)
DATA_PTR(    0x4034,  post_isr_hook)
DATA_PTR(   0x65a08,  isr_table_handler)
DATA_PTR(   0x65a0c,  isr_table_param)
THUMB_FN(0xe015fc98,  cli_spin_lock)          /* used in AllocateMemory/FreeMemory and others */

/** MPU communication **/
THUMB_FN(0xe01f67aa,  mpu_send)                  // "dwSize < TXBD_DATA_SIZE"
THUMB_FN(0xe064488a,  mpu_recv)                  // passed as last argument by InitializeIntercom and eventually stored into mpu_recv_cbr
DATA_PTR(    0x7c24,  mpu_recv_cbr)              // mpu_recv is called indirectly through this function pointer
DATA_PTR(   0x80ec4,  mpu_send_ring_buffer)      // ring buffer used in mpu_send
DATA_PTR(    0x7c08,  mpu_send_ring_buffer_tail) // ring buffer index incremented in mpu_send
DATA_PTR(   0x80d84,  mpu_recv_ring_buffer)      // ring buffer used in SIO3_ISR, subroutine that processes two chars at a time
DATA_PTR(    0x7c00,  mpu_recv_ring_buffer_tail) // ring buffer index incremented in the above subroutine

/** Misc **/
THUMB_FN(0xdf009338,  vsnprintf)              /* called near dmstart; references "01234567", "0123456789", "0123456789abcdef"
                                                 and "0123456789ABCDEF"; second arg is size; the one called by DebugMsg only knows %s */

DATA_PTR(    0x100b4,  winsys_sem) // used in RefreshVrmsSurface

THUMB_FN(0xdf0031a3,  get_task_by_id) // called indirectly by task_create and functions using string "TASK Name"
THUMB_FN(0xdf004041,  _get_task_info_by_id)
NSTUB(    0x4f40,  dm_names) // in DebugMsg, before the 1st loop target

NSTUB(   0x20054,  sd_device) // written to in SD_DeviceCreate
NSTUB(    0x4800,  gui_main_struct) // See function using "Queue Clear event" string
                                    // There's a base of struct, and some refs to fields via offsets.
                                    // Find the right one.
THUMB_FN(0xe00dc4ea,  GUI_ChangeMode) // "GUI_ChangeMode:%d" string
THUMB_FN(0xe04e165c,  DispSensorStart) // 0xe7ec looks to be a bool for whether Sensor is started
THUMB_FN(0xe04e3fe4,  DeleteMemorySuite)
THUMB_FN(0xe04e4224,  CreateMemorySuite)
THUMB_FN(0xe04e3750,  CreateMemoryChunk) // called from CreateMemorySuite
//NSTUB(0x,  GetSizeOfMemorySuite) // not needed, but I found it along the way
THUMB_FN(0xdf008515,  take_semaphore) // Fairly sure. Same function sig and followed by give_semaphore
THUMB_FN(0xdf00858f,  give_semaphore)
THUMB_FN(0xdf008419,  create_named_semaphore) // also see df00b114, very similar, but df008418 is closer to 50D create_named_semaphore.
                                              // Complicated by there being df00b648, which wraps 8418 with an extra assert possible,
                                              // and e0426a20, which is what most sites call, which calls 8418 - but thunked.
                                              // In theory that means DryOS could replace the underlying create_named_semaphore
                                              // at run-time.  No evidence that they do.

THUMB_FN(0xe032c222,  gui_change_mode) // GUI_ChangeMode_Post
THUMB_FN(0xe032a9c8,  gui_massive_event_loop) // various "GUICMD_"* strings
THUMB_FN(0xe009a070,  gui_init_event) // "-> handleGuiInit"
THUMB_FN(0xe04e3cfe,  AddMemoryChunk) // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
THUMB_FN(0xe04e3d82,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
THUMB_FN(0xe04e4652,  GetNextMemoryChunk)                      // AJ_PackMemory_PackMem.c_IsChunkSignature_p2
THUMB_FN(0xe04e3b9c,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature - 200D uses "MemChunk" directly
THUMB_FN(0xe066ab6e,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
THUMB_FN(0xe066ab9c,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
THUMB_FN(0xe066ac42,  FreeMemoryResource)                      // m_pfFreeMemoryCBR

THUMB_FN(0xdf00b1d9,  msg_queue_receive) // "SystemIF::KerQueue.c" useful for finding some of these msg_ functions
THUMB_FN(0xdf00b337,  msg_queue_post)
THUMB_FN(0xdf00b36d,  msg_queue_count) // Fairly hard to find.  Often used near DispLockStop, which is easy to find
                                       // and not used often.
                                       // This is the thunked address, which is called from e042680c
THUMB_FN(0xdf00b615,  msg_queue_create) // CreateMessageQueue - In 50D, msg_queue_create calls 0xff866424,
                                        // the equiv of which is 0xdf00b114 (and was easier to find)

THUMB_FN(0xe04e520c,  prop_deliver)
THUMB_FN(0xe04e4ca6,  prop_register_slave) // "pRegisterSlave"
THUMB_FN(0xe03a1c00,  LiveViewApp_handler) // not sure - function is quite different 50D -> 200D, but that
                                        // doesn't seem unreasonable since large GUI changes.  String use and surrounding
                                        // functions look similar / related.
THUMB_FN(0xe0486f60,  PlayMain_handler) // "DlgPlayMain.c PRESS_ERASE_BUTTON" - code above the string, wasn't found automatically
THUMB_FN(0xe03d5b1e,  GetCFnData) // Differs significantly from 200D
THUMB_FN(0xe03d4c80,  SetCFnData) // There are some "SetCFnData" related strings

THUMB_FN(0xe070a8ea, _malloc) // These two names are used in 50D and CONFIG_HELLO_WORLD won't build without them,
THUMB_FN(0xe070a922, _free)   // however, the inital stubs for 200D call these simply malloc/free.
                           // I'm leaving them defined with both names, although that seems wrong.
THUMB_FN(0xe04f939a,  SRM_AllocateMemoryResourceFor1stJob) // "hJobMemorySuite" strings, function a little earlier
THUMB_FN(0xe04fc23e,  SRM_FreeMemoryResourceFor1stJob) // find refs to "pMessage", there are several similar looking functions using it,
                                                       // this is my best guess - this one has pPriorMessage and sets offset 8
                                                       // in the struct to 0, offset 0x10 to param1
THUMB_FN(0xe04e529e, _prop_cleanup) // "./Multicast/PropertyMgr.c" check funcs that ref this string
THUMB_FN(0xdf00b67d,  CreateRecursiveLock) // via "KerRLock.c", CRL() calls a function that regs this string, not many do
THUMB_FN(0xdf00b451,  AcquireRecursiveLock)
THUMB_FN(0xdf00b51b,  ReleaseRecursiveLock)
THUMB_FN(0xe04db03c,  dialog_redraw) // Found via checking xrefs to "pDialog->pSignature == pcSignature"
THUMB_FN(0xe04aa630,  CreateResLockEntry) // via xrefs to a pointer to "ResInfo"
THUMB_FN(0xe04aaa50,  LockEngineResources) //  Down a bit from CreateResLockEntry, start v similar but end is different.
THUMB_FN(0xe076b928,  ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

THUMB_FN(0xdf00d365,  fsuDecodePartitionTable) // SJE FIXME wrong - this function doesn't exist on 6D2.  See longer notes in 200D stubs.S.
                                               // I think bootflags.c code should be changed so we do this manually for these cams.

THUMB_FN(0xe05f67d1,  ShootOlcApp_handler) // near "StopShootOlcApp PopPalette(%d)"
//DATA_PTR(    0x40d0,  first_task) // unsure on naming here.  First item in an array of task structs.
                                  // Seems to always be "idle" task.
                                  // (we don't use this, but instead calculate it during early init,
                                  //  to save backporting this stub for all older cams)
NSTUB(    0x40f0,  task_max)
NSTUB(    0xffc0,  gui_task_list) // Found in function 0xe06f0684.
THUMB_FN(0xe06b4128, _LoadCalendarFromRTC)

// These are required on Digic 678
THUMB_FN(0xe04c76f4, RefreshVrmsSurface) // "Call RefreshVrmsSurface"
DATA_PTR(   0x100cc, display_refresh_needed) // if 0, RefreshVrmsSurface() does nothing
THUMB_FN(0xe022d5cc, XimrExe) // trigger Ximr unit to execute (HW compositor, partially understood)

DATA_PTR(   0x100bc, bmp_vram_info) // Used in function with string "B:/YCC_A_%02d.bin".

DATA_PTR(   0x100b8, _rgb_vram_info) // Used in many places, written to in "InitializeScreen"
                                     // At least for 200D, this is BGRA buffer, earlier in the render
                                     // chain than the YUV ones.  But, changes made to it will not
                                     // render until a refresh occurs (can manually trigger via RefreshVrmsSurface()),
                                     // but direct refresh via XimrExe() is simpler (we think this tells
                                     // the renderer peripheral to take the RGB data and update the screen(s).

DATA_PTR(   0x100d8,  display_output_mode) // on 200D, 1: 960x540, anything else, 720x480

NSTUB(0x56500000,  LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800
                                // SJE FIXME fake address in safe writable region, may not exist
                                // on 200D (doesn't seem to use indexed colour anywhere)

DATA_PTR(    0x412c,  task_dispatch_hook)
NSTUB(0x56500000,  PlayMovieGuideApp_handler) // needed for CONFIG_QEMU, unknown
                                              // SJE FIXME
DATA_PTR(    0xe789,  additional_version) // this gets written to in early code.  Pick a safe area!
THUMB_FN(0xe04e4d74, _prop_request_change)
THUMB_FN(0xdf0070c5,  dm_set_store_level) // called by "dmstore"
                                          // Called in init_task(), shortly before ICU Firmware version string
                                          // is logged.
THUMB_FN(0xdf00707d,  dm_set_print_level)
THUMB_FN(0xdf00325d,  task_trampoline)


//////
// SJE stuff that's not required but can be useful
THUMB_FN(0xe06171f0,  debug_assert) // DRYOS_ASSERT_HANDLER related
THUMB_FN(0xe04eb6c4, uart_printf) // trigger Ximr unit to execute (HW compositor, partially understood)
