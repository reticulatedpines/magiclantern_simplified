/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the 1300d / Rebel T6.
 *
 */
/*
 * Copyright (C) 2017 Magic Lantern Team
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

// for task information

/** Startup **/
NSTUB(MAIN_FIRMWARE_ADDR, firmware_entry)
NSTUB(0xFE0C3A24,  cstart)                                  //
NSTUB(   0x29898,  bzero32)                                 // called by cstart() rom
NSTUB(    0x5254,  create_init_task)                        // must be called from ram
NSTUB(0xFE1296C8,  init_task)                               // passed as arg to create_init_task, look for dmSetup
NSTUB(   0x67430,  additional_version)

/** File I/O **/
NSTUB(0xFE2A43FC,  FIO_CloseFile)                           //
NSTUB(0xFE2A53D0,  FIO_FindClose)                           // called with ret_FIO_FindFirstEx after finishing the dir scanning loop
NSTUB(0xFE2A52F0,  FIO_FindNextEx)                          //
NSTUB(0xFE2A41AC, _FIO_ReadFile)                            //
NSTUB(0xFE2A425C,  FIO_SeekSkipFile)                        // same as SeekFile
NSTUB(0xFE2A434C, _FIO_WriteFile)                           //
NSTUB(0xFE2A4C3C, _FIO_CreateDirectory)                     //
NSTUB(0xFE2A4058, _FIO_CreateFile)                          //
NSTUB(0xFE2A51FC, _FIO_FindFirstEx)                         //
NSTUB(0xFE2A4578, _FIO_GetFileSize)                         //
NSTUB(0xFE2A3F9C, _FIO_OpenFile)                            //
NSTUB(0xFE2A4104, _FIO_RemoveFile)                          //
NSTUB(0xFE2A4A74, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x3136C,  gui_main_struct)                         // in gui_main_task
NSTUB(   0x3DA5C,  gui_task_list)
NSTUB(0xFE0CCA48,  GUI_ChangeMode)
NSTUB(0xFE0CC88C,  GUI_Control)
NSTUB(0xFE14EF88,  SetGUIRequestMode)
NSTUB(0xFE0CCEFC,  gui_init_end)                            // EndGuiInit
NSTUB(0xFE0CC4BC,  gui_main_task)                           // message at 0x30, counter at 0x0c
NSTUB(0xFE151800,  gui_massive_event_loop)                  // GUI_Control_Post
NSTUB(0xFE2EB7F8,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
NSTUB(0xFE2EBA2C,  Gui_SetSoundRecord)                   /* present on 6D.113, 5D2.212 */

/** ASIF **/
// TODO:
NSTUB(0xfe11d1cc,  PowerAudioOutput)                       /* not found  old FE108E44*/
NSTUB(0xFE11B938,  SetAudioVolumeOut)
NSTUB(0xFE106A98,  SetNextASIFADCBuffer)
NSTUB(0xFE106C80,  SetNextASIFDACBuffer)
NSTUB(0xFE11C690,  SetSamplingRate)
NSTUB(0xFE105F7C,  StartASIFDMAADC)
NSTUB(0xFE106350,  StartASIFDMADAC)
NSTUB(0xFE1065D0,  StopASIFDMADAC)
NSTUB(0xFE11CD04,  PowerMicAmp)                           /* present on 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFE1066F8,  SetASIFMode)                             /* present on 700D.113 */
//NSTUB(0xFE108E10,  SetAudioVolumeIn) //?                       /* present on 6D.113 */
NSTUB(0xFE108A70,  SoundDevActiveIn) //ok                       /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFE108CC8,  SoundDevShutDownIn)//ok                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFE106228,  StopASIFDMAADC) //ok                          /* present on 7D.203, 6D.113, EOSM.202 */


/** Audio **/
NSTUB(    0x31604,  sounddev)                               // in sounddev_task
// TODO: Not found direct write it looks like its some kind of serial connection now
NSTUB(0xFE2B3A3C, _audio_ic_read) // FE2B3A3C
NSTUB(0xFE2B36D4, _audio_ic_write) // posible sub_FE2B3A18
NSTUB(0xFE8C32CC,  audio_thresholds)                        // find "ALV_Initialize", then pointer to "AudioLevel", thresholds are following array of 41 values
NSTUB(0xFE108A70,  sounddev_active_in)                      // "SoundDevActiveIn"
NSTUB(0xFE108660,  sounddev_task)                           // "[SND] _SoundDeviceTask"
// NSTUB(    ???, _audio_ic_write_bulk)                     /* present on 6D.116 */

/** Bitmap **/
NSTUB(   0x54A40,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x56240,  bmp_vram_info)

/** CFN **/
NSTUB(0xFE71C2A4,  GetCFnData)
NSTUB(0xFE71C4D0,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(0xFE4716F0,  GUI_SetCFnForTab4)                    /* present on 7D.203 */

/** Camera interface? **/
// TODO:
NSTUB(0xC0220000,  camera_engine)

/** MPU communication **/
NSTUB(0xFE2993B4, _mpu_send);                               /* dwSize < TXBD_DATA_SIZE */
NSTUB(0xFE10FA70, _mpu_recv);                               /* registered in InitializeIntercom as CBR */

/** DMA **/
NSTUB(0xFE1244E0,  dma_memcpy)

/** Debug messages **/
NSTUB(   0x31BF4,  dm_names)                                // in DebugMsg, before the 1st loop target
NSTUB(0xFE11F394,  DryosDebugMsg)
NSTUB(0xFE11F6FC,  dm_set_store_level)                      // called by "dmstore"
NSTUB(0xFE11F694,  dm_set_print_level)

/** Dialog API **/
NSTUB(0xFE3F0A44,  dialog_redraw)
NSTUB(0xFE2F8BF8,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFE5AC378,  ErrCardForLVApp_handler)                 // LiveViewErrorApp, as in 60D
NSTUB(0xFE45573C,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFE488590,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xFE48D148,  LiveViewApp_handler)
NSTUB(0xFE48D7B4,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFE48EDF0,  LiveViewApp_handler_end)                 // start of next function
NSTUB(0xFE4A8C24,  PlayMain_handler)
NSTUB(0xFE5E7BDC,  PlayMovieGuideApp_handler)
NSTUB(0xFE49C300,  ShootOlcApp_handler)
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(0xFE49AA14,  LiveViewWbApp_handler)                /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
NSTUB(0xFE40E9E4,  MirrorDisplay)
NSTUB(0xFE40EA44,  NormalDisplay)
NSTUB(0xFE40EA14,  ReverseDisplay)

/** EDMAC **/
NSTUB(0xFE29E8A8,  AbortEDmac)
NSTUB(0xFE29E5B4,  ConnectReadEDmac)
NSTUB(0xFE29E4F0,  ConnectWriteEDmac)
NSTUB(0xFE29E970,  RegisterEDmacAbortCBR)
NSTUB(0xFE29E948,  RegisterEDmacCompleteCBR)
NSTUB(0xFE29EA74,  RegisterEDmacPopCBR)
NSTUB(0xFE29E438,  SetEDmac)
NSTUB(0xFE29E754,  StartEDmac)
NSTUB(0xFE29E9B0,  UnregisterEDmacAbortCBR)
NSTUB(0xFE29E95C,  UnregisterEDmacCompleteCBR)
NSTUB(0xFE29EA88,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFE2EB038,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xfe29f844, _EngDrvOut)
NSTUB(0xFE29FBF0, _engio_write)
//NSTUB(0xFE29FC98,  shamem_read) //0xfe29f838
NSTUB(0xFE29F734,  shamem_read)

/** Eventprocs (call by name) **/
NSTUB(0xFE120288,  call)

/** Events **/
NSTUB(0xFE2C2308,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFE2C2364,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFE2BE730,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFE2BE78C,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFE122C84,  AddMemoryChunk)
NSTUB(0xFE125118,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFE1250CC,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFE122200,  CreateMemoryChunk)
NSTUB(0xFE1229E4,  CreateMemorySuite)                       // FFCCEF34
NSTUB(0xFE122818,  DeleteMemorySuite)                       // FFCCED2C
NSTUB(0xFE125230,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFE122D54,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFE1227A4,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature
NSTUB(0xFE1234E0,  GetNextMemoryChunk)
NSTUB(0xFE0D5554,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFE0D7E54,  SRM_FreeMemoryResourceFor1stJob)

/** GUI for old cameras **/
NSTUB(   0x36CAC,  gui_timer_struct)                        // in GUI_Control_Post
NSTUB(0xFE1536A0,  gui_change_mode)                         // GUI_ChangeMode_Post
NSTUB(0xFE153E80,  gui_local_post)
NSTUB(0xFE1544A8,  gui_other_post)
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

NSTUB(0xFE154890,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
NSTUB(0xFE15511C,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
//NSTUB(0xFE154808,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

/** GUI timers **/
NSTUB(0xFE482FE0,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
NSTUB(0xFE129078,  CancelTimer)
NSTUB(0xFE120CEC,  SetHPTimerAfterNow)
NSTUB(0xFE120DDC,  SetHPTimerNextTick)
NSTUB(0xFE128FA8,  SetTimerAfter)

/** H264 Encoder **/
NSTUB(   0x38AC8,  mvr_config)
NSTUB(0xFE281D2C,  mvrFixQScale)
NSTUB(0xFE2817F0,  mvrSetDefQScale)
NSTUB(0xFE281838,  mvrSetFullHDOptSize)
NSTUB(0xFE281A1C,  mvrSetGopOptSizeFULLHD)

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
NSTUB(   0x310a4,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
NSTUB(   0x310a0,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xFE12BBC8,  DispSensorStart)

/** Making the card bootable **/
NSTUB(   0x499a0,  cf_device)
NSTUB(   0x499C4,  sd_device)                               // in sdReadBlk
NSTUB(0xFE543BD0,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(0xFE1278C8,  GetMemoryInformation)                    // called from AllocateMemory
NSTUB(0xFE127874,  GetSizeOfMaxRegion)
NSTUB(0xFE127B28, _AllocateMemory)                          // "[MEM] NG AllocateMemory"
NSTUB(0xFE127EC4, _FreeMemory)
NSTUB(0xFE2C2528, _alloc_dma_memory)
NSTUB(0xFE2C6C04, _free)
NSTUB(0xFE2C2564, _free_dma_memory)
NSTUB(0xFE2C6BAC, _malloc)                                  // maybe its better to use Allocate/Free?
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(    0x7868,  msg_queue_count)
NSTUB(    0x7480,  msg_queue_create)                        // CreateMessageQueue 7480
NSTUB(    0x7814,  msg_queue_post)
NSTUB(    0x75D0,  msg_queue_receive)

/** PTP **/
NSTUB(0xFE36ED54,  ptp_register_handler)
NSTUB(0xFE1FDBE8,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFE10C5B0, _prop_cleanup)
NSTUB(0xFE10BE14, _prop_request_change)
NSTUB(0xFE10C408,  prop_deliver)
NSTUB(0xFE10C490,  prop_register_slave)                     // called by"GUI_RegisterPropertySlave"
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** Recursive locks **/
NSTUB(    0xBCB8,  AcquireRecursiveLock)
NSTUB(    0xBBF8,  CreateRecursiveLock)
NSTUB(    0xBE28,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xFE2A2668,  CreateResLockEntry)
NSTUB(0xFE2A2BA8,  LockEngineResources)
NSTUB(0xFE2A2CE0,  UnLockEngineResources)

/** Semaphores **/
NSTUB(    0x30C8,  create_named_semaphore)
NSTUB(    0x3358,  give_semaphore)
NSTUB(    0x3270,  take_semaphore)

/** Task info **/
NSTUB(   0x358E8,  task_max)

NSTUB(    0x24C4,  _get_task_info_by_id)
NSTUB(   0x31170,  current_task)
NSTUB(     0x640,  current_interrupt) // in interrupt handler (0x18), where MEM(C0201004) is stored

/* wifi  */
/*
NSTUB(0xFE43005C, nif_setup)
NSTUB(0xFE42fd44, rt_edit)
NSTUB(0xFE443964, wlanconnect)

NSTUB(0x000140CC, _socket_create)
NSTUB(0x000142BC, _socket_bind)
NSTUB(0x000143B0, _socket_connect)
NSTUB(0x00014334, _socket_listen)
NSTUB(0x00014468, _socket_accept)
NSTUB(0x00014670, _socket_recv)
NSTUB(0x00014920, _socket_send)
NSTUB(0x00014DE4, _socket_setsockopt)
*/

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
NSTUB(0xFE11ABF0,  LoadCalendarFromRTC)
NSTUB(0xFE2BC8A4,  vsnprintf)
//NSTUB(0xFE0180A8,  _print_serial)
//NSTUB( 0xff37aee4, ChangeHDMIOutputSizeToFULLHD)
//NSTUB( 0xff37b174, ChangeHDMIOutputSizeToVGA)
NSTUB(0xFE114B20,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  FSUunMountDevice)                        /* present on 5D3.113 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

NSTUB(0xfe0d48b4,  TryReceiveMessageQueue)
