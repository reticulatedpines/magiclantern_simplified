/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.2.0 firmware for the 1300d / Rebel T6.
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

.text

// ROM1.BIN starts at 0xFE000000

// ??? (old)
// ROMBASE = 0xFE0C0000
// RESTARTSTART = 0x82000
// ROM->RAM START = 0xF80110CC -> 0x100000

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )                        // 0xFE0C0000
NSTUB(0xfe0c3a24,  cstart)                                  //
NSTUB(   0x29898,  bzero32)                                 // called by cstart() rom
NSTUB(    0x5250,  create_init_task)                        // must be called from ram
NSTUB(0xfe129684,  init_task)                               // passed as arg to create_init_task, look for dmSetup
NSTUB(   0x67430,  additional_version)

/** File I/O **/
NSTUB(0xfe2a4478,  FIO_CloseFile)                           //
NSTUB(0xFE2A544C,  FIO_FindClose)                           // called with ret_FIO_FindFirstEx after finishing the dir scanning loop
NSTUB(0xFE2A536C,  FIO_FindNextEx)                          //
NSTUB(0xFE2A4228, _FIO_ReadFile)                            //
NSTUB(0xFE2A42D8,  FIO_SeekSkipFile)                        // same as SeekFile
NSTUB(0xFE2A43C8, _FIO_WriteFile)                           //
NSTUB(0xFE2A4CB8, _FIO_CreateDirectory)                     //
NSTUB(0xFE2A40D4, _FIO_CreateFile)                          //
NSTUB(0xFE2A5278, _FIO_FindFirstEx)                         //
NSTUB(0xFE2A45F4, _FIO_GetFileSize)                         //
NSTUB(0xFE2A4018, _FIO_OpenFile)                            //
NSTUB(0xFE2A4180, _FIO_RemoveFile)                          //
NSTUB(0xFE2A4AF0, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x3136C,  gui_main_struct)                         // in gui_main_task
NSTUB(   0x3DA5C,  gui_task_list)
NSTUB(0xFE0CCA48,  GUI_ChangeMode)
NSTUB(0xFE0CC88C,  GUI_Control)
NSTUB(0xfe14ef44,  SetGUIRequestMode)
NSTUB(0xFE0CCEFC,  gui_init_end)                            // EndGuiInit
NSTUB(0xFE0CC4BC,  gui_main_task)                           // message at 0x30, counter at 0x0c
NSTUB(0xfe1517bc,  gui_massive_event_loop)                  // GUI_Control_Post
NSTUB(0xfe2eb9d4,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */

/** ASIF **/
// TODO:
// NSTUB(0xfe108e44,  PowerAudioOutput)                       /* not found  old FE108E44 (??) */
NSTUB(0xfe11b8f4,  SetAudioVolumeOut)  //FE11B938
NSTUB(0xfe106a98,  SetNextASIFADCBuffer) //ok
NSTUB(0xfe106c80,  SetNextASIFDACBuffer) //ok
NSTUB(0xfe11c64c,  SetSamplingRate) //FE11C690
NSTUB(0xfe105f7c,  StartASIFDMAADC) //ok
NSTUB(0xfe106350,  StartASIFDMADAC) //ok
NSTUB(0xfe1065d0,  StopASIFDMADAC) //FE1065D0
NSTUB(0xfe11ccc0,  PowerMicAmp) //FE11CD04                            /* present on 650D.104, EOSM.202, 700D.113 */

/** Audio **/
NSTUB(    0x31604,  sounddev)                               // in sounddev_task
// TODO: Not found direct write it looks like its some kind of serial connection now
NSTUB(0xfe2b3c18, _audio_ic_read) // FE2B3A3C
NSTUB(0xfe2b38b0, _audio_ic_write) // posible sub_FE2B3A18
NSTUB(0xfe8c323c,  audio_thresholds)                        // find "ALV_Initialize", then pointer to "AudioLevel", thresholds are following array of 41 values
NSTUB(0xfe108a70,  sounddev_active_in)                      // "SoundDevActiveIn" (CHECK)
NSTUB(0xfe108660,  sounddev_task)                           // "[SND] _SoundDeviceTask"
// NSTUB(    ???, _audio_ic_write_bulk)                     /* present on 6D.116 */

/** Bitmap **/
NSTUB(   0x54A40,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x56240,  bmp_vram_info)

/** CFN  ok**/ 
NSTUB(0xfe71c480,  GetCFnData)
NSTUB(0xfe71c6ac,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(0xFE4716F0,  GUI_SetCFnForTab4)                    /* present on 7D.203 */

/** Camera interface? **/
// UNPORTED
 NSTUB(0xC0220000,  camera_engine)

/** MPU communication **/
NSTUB(0xfe299430, _mpu_send);                               /* dwSize < TXBD_DATA_SIZE */
NSTUB(0xfe10fa70, _mpu_recv);                               /* registered in InitializeIntercom as CBR */

/** DMA **/
NSTUB(0xfe12449c,  dma_memcpy)

/** Debug messages **/
NSTUB(   0x31BF4,  dm_names)                                // in DebugMsg, before the 1st loop target
NSTUB(0xfe11f350,  DryosDebugMsg)
NSTUB(0xfe11f6b8,  dm_set_store_level)                      // called by "dmstore"

/** Dialog API **/
NSTUB(0xfe3f0c20,  dialog_redraw)
NSTUB(0xfe2f8dd4,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xfe5acec8,  ErrCardForLVApp_handler)                 // LiveViewErrorApp, as in 60D
NSTUB(0xfe455918,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xfe48876c,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xfe48d324,  LiveViewApp_handler)
NSTUB(0xfe48d990,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xfe48efcc,  LiveViewApp_handler_end)                 // start of next function
NSTUB(0xfe4a8e00,  PlayMain_handler)
NSTUB(0xfe5e7db8,  PlayMovieGuideApp_handler)
NSTUB(0xfe49c4dc,  ShootOlcApp_handler)

// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(0xFE49AA14,  LiveViewWbApp_handler)                /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
NSTUB(0xfe40ebc0,  MirrorDisplay)
NSTUB(0xfe40ec20,  NormalDisplay)
NSTUB(0xfe40ebf0,  ReverseDisplay)

/** EDMAC **/
NSTUB(0xfe29e924,  AbortEDmac)
NSTUB(0xfe29e630,  ConnectReadEDmac)
NSTUB(0xfe29e56c,  ConnectWriteEDmac)
NSTUB(0xfe29e9ec,  RegisterEDmacAbortCBR)
NSTUB(0xfe29e9c4,  RegisterEDmacCompleteCBR)
NSTUB(0xfe29eaf0,  RegisterEDmacPopCBR)
NSTUB(0xfe29e4b4,  SetEDmac)
NSTUB(0xfe29e7d0,  StartEDmac)
NSTUB(0xfe29ea2c,  UnregisterEDmacAbortCBR)
NSTUB(0xfe29e9d8,  UnregisterEDmacCompleteCBR)
NSTUB(0xfe29eb04,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xfe2eb218,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xfe29f8c0, _EngDrvOut)
NSTUB(0xfe29fbec, _engio_write)
NSTUB(0xfe29f7b0,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xfe120244,  call)

/** Events **/
NSTUB(0xfe2c24e4,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xfe2c2540,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xfe2be90c,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xfe2be968,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xfe122b34,  AddMemoryChunk) // ????
NSTUB(0xfe1250d4,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xfe125088,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xfe1221bc,  CreateMemoryChunk)
NSTUB(0xfe1229a0,  CreateMemorySuite)                       // FFCCEF34
NSTUB(0xfe1227d4,  DeleteMemorySuite)                       // FFCCED2C
NSTUB(0xfe1251ec,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xfe122d10,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xfe122760,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature
NSTUB(0xfe12349c,  GetNextMemoryChunk)
NSTUB(0xfe0d5554,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xfe0d7e54,  SRM_FreeMemoryResourceFor1stJob)

/** GUI for old cameras **/
NSTUB(   0x36CAC,  gui_timer_struct)                        // in GUI_Control_Post
NSTUB(0xfe15365c,  gui_change_mode)                         // GUI_ChangeMode_Post
NSTUB(0xfe153e3c,  gui_local_post)
NSTUB(0xfe154464,  gui_other_post)
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

NSTUB(0xfe15484c,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
NSTUB(0xfe1550d8,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
NSTUB(0xfe1547c4,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

/** GUI timers **/
NSTUB(0xfe4831bc,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
NSTUB(0xfe129034,  CancelTimer)
NSTUB(0xfe120ca8,  SetHPTimerAfterNow)
NSTUB(0xfe120d98,  SetHPTimerNextTick)
NSTUB(0xfe128f64,  SetTimerAfter)

// UNPORTED
/** H264 Encoder **/
NSTUB(   0x38AC8,  mvr_config)
NSTUB(0xfe281da8,  mvrFixQScale)
NSTUB(0xfe28186c,  mvrSetDefQScale)
NSTUB(0xfe2818b4,  mvrSetFullHDOptSize)
NSTUB(0xfe281a98,  mvrSetGopOptSizeFULLHD)

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
NSTUB(   0x310a4,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
NSTUB(   0x310a0,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xfe12bb84,  DispSensorStart)
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x499a0,  cf_device)
NSTUB(   0x499C4,  sd_device)                               // in sdReadBlk
NSTUB(0xfe543dac,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(0xfe127884,  GetMemoryInformation)                    // called from AllocateMemory
NSTUB(0xfe127830,  GetSizeOfMaxRegion)
NSTUB(0xfe127ae4, _AllocateMemory)                          // "[MEM] NG AllocateMemory"
NSTUB(0xfe127e80, _FreeMemory)
NSTUB(0xfe2c2704, _alloc_dma_memory)
NSTUB(0xfe2c6de0, _free)
NSTUB(0xfe2c2740, _free_dma_memory)
NSTUB(0xfe2c6d88, _malloc)                                  // maybe its better to use Allocate/Free?
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(    0x7868,  msg_queue_count)
NSTUB(    0x7480,  msg_queue_create)                        // CreateMessageQueue
NSTUB(    0x7814,  msg_queue_post)
NSTUB(    0x75D0,  msg_queue_receive)

/** PTP **/
NSTUB(0xfe36ef30,  ptp_register_handler)
NSTUB(0xfe1fdb6c,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xfe10c5b0, _prop_cleanup)
NSTUB(0xfe10be14, _prop_request_change)
NSTUB(0xfe10c408,  prop_deliver)
NSTUB(0xfe10c490,  prop_register_slave)                     // called by"GUI_RegisterPropertySlave"
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(0xFE2B8E18,  BulkInIPCTransfer)                    /* present on 7D.203 */
// NSTUB(0xFE2B8D54,  BulkOutIPCTransfer)                   /* present on 7D.203 */
// NSTUB(0xFE2B8F8C,  QuickOutIPCTransfer)                  /* present on 7D.203 */
// NSTUB(0xFE2B950C,  RegisterRPCHandler)                   /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(0xFE2B95BC,  RequestRPC)                           /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(    0xBCB8,  AcquireRecursiveLock)
NSTUB(    0xBBF8,  CreateRecursiveLock) 
NSTUB(    0xBE28,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xfe2a26e4,  CreateResLockEntry)
NSTUB(0xfe2a2c24,  LockEngineResources)
NSTUB(0xfe2a2d5c,  UnLockEngineResources)

/** Semaphores **/
NSTUB(    0x30C8,  create_named_semaphore)
NSTUB(    0x3358,  give_semaphore)
NSTUB(    0x3270,  take_semaphore)

/** Task info **/
NSTUB(   0x358E8,  task_max)
NSTUB(0xfe2c30ac,  is_taskid_valid)                         // AJ_task_trampoline_related_p10
NSTUB(   0x31170,  current_task)
NSTUB(     0x640,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

NSTUB(0x24c4,  _get_task_info_by_id) 

/** Task sleep **/
NSTUB(    0x3780,  msleep)                                  // called from EV_GETDONE_CACHE param 0x64

/** Tasks **/
NSTUB(   0x35924,  task_dispatch_hook)  // task_trampoline -> last call -> last non-empty BL -> one indirect call here
NSTUB(    0x38FC,  task_create)
NSTUB(    0x41DC,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(   0x6D418,  lv_path_struct)                          // first arg of SelectPathDriveMode
NSTUB(0xfe11abac,  LoadCalendarFromRTC)
NSTUB(0xfe2bca80,  vsnprintf)
//NSTUB(0xFE0180A8,  _print_serial)
//NSTUB( 0xff37aee4, ChangeHDMIOutputSizeToFULLHD)
//NSTUB( 0xff37b174, ChangeHDMIOutputSizeToVGA)
NSTUB(0xfe114b20,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  FSUunMountDevice)                        /* present on 5D3.113 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

NSTUB(0xfe0d48b4,  TryReceiveMessageQueue)
