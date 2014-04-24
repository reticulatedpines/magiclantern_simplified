/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.2 firmware for the 600d.
 *
 */
/*
 * Copyright (C) 2011 Magic Lantern Team
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

// ROMBASE = 0xff010000
// RESTARTSTART = 0x82000

// Everything below here is not needed for the first boot

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */

// FF05FECC WinSys_FreeMemory

// called by fsuGetPart with arg0 = (446 + ret_alloc_dma_memory)
// => first partition table; see http://www.datarescue.com/laboratory/partition.htm

// FF04E88C mvrAppendCheckSetRecLimit

// 550d 109 values!!

// for task information


/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF011028,  cstart)
NSTUB(0xFF5683E8,  bzero32)                                 // called by cstart()
NSTUB(0xFF017FA4,  create_init_task)
NSTUB(0xFF0197FC,  init_task)                               // passed as arg to create_init_task, look for dmSetup
NSTUB(    0x82E0,  additional_version)

/** File I/O **/
NSTUB(0xFF1E66B4,  FIO_CloseFile)
NSTUB(0xFF1E7684,  FIO_FindClose)                           // called with ret_FIO_FindFirstEx after finishing the dir scanning loop
NSTUB(0xFF1E75A4,  FIO_FindNextEx)
NSTUB(0xFF1E6464,  FIO_ReadFile)
NSTUB(0xFF1E65E4,  FIO_SeekFile)
NSTUB(0xFF1E6604,  FIO_WriteFile)
NSTUB(0xFF1E6EF0, _FIO_CreateDirectory)
NSTUB(0xFF1E6310, _FIO_CreateFile)
NSTUB(0xFF1E74B0, _FIO_FindFirstEx)
NSTUB(0xFF1E6830, _FIO_GetFileSize)
NSTUB(0xFF1E6254, _FIO_OpenFile)
NSTUB(0xFF1E63BC, _FIO_RemoveFile)
NSTUB(0xFF1E6D28, _FIO_RenameFile)

/** GUI **/
NSTUB(    0x1C38,  gui_main_struct)                         // in gui_main_task
NSTUB(    0xAC30,  gui_task_list)
NSTUB(0xFF021A28,  GUI_ChangeMode)
NSTUB(0xFF021928,  GUI_Control)
NSTUB(0xFF0A5E44,  SetGUIRequestMode)
NSTUB(0xFF021F78,  gui_init_end)                            // EndGuiInit
NSTUB(0xFF02156C,  gui_main_task)                           // message at 0x30, counter at 0x0c
NSTUB(0xFF0A7F90,  gui_massive_event_loop)                  // GUI_Control_Post
// NSTUB(    ???,  GUI_SetLvMode)                           /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */
// NSTUB(    ???,  Gui_SetSoundRecord)                      /* present on 6D.113, 5D2.212 */

/** ASIF **/
NSTUB(0xFF06A93C,  PowerAudioOutput)
NSTUB(0xFF06A420,  SetAudioVolumeOut)
NSTUB(0xFF055E90,  SetNextASIFADCBuffer)
NSTUB(0xFF055F48,  SetNextASIFDACBuffer)
NSTUB(0xFF06A9E0,  SetSamplingRate)
NSTUB(0xFF0556C8,  StartASIFDMAADC)
NSTUB(0xFF05587C,  StartASIFDMADAC)
NSTUB(0xFF055950,  StopASIFDMADAC)
// NSTUB(    ???,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SetAudioVolumeIn)                        /* present on 6D.113 */
// NSTUB(    ???,  SoundDevActiveIn)                        /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  SoundDevShutDownIn)                      /* present on 7D.203, 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */

/** Audio **/
NSTUB(    0x1EA8,  sounddev)                                // in sounddev_task
NSTUB(0xFF069FF8, _audio_ic_read)
NSTUB(0xFF06A144, _audio_ic_write)
NSTUB(0xFF5DF9A0,  audio_thresholds)                        // find "ALV_Initialize", then pointer to "AudioLevel", thresholds are following array of 41 values
NSTUB(0xFF057BE0,  sounddev_active_in)                      // "SoundDevActiveIn"
NSTUB(0xFF057774,  sounddev_task)                           // "[SND] _SoundDeviceTask"

/** Bitmap **/
NSTUB(   0x1C36C,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x1DB6C,  bmp_vram_info)

/** CFN **/
NSTUB(0xFF54F9E0,  GetCFnData)
NSTUB(0xFF54FC0C,  SetCFnData)
// NSTUB(    ???,  GUI_GetCFnForTab4)                       /* present on 7D.203 */
// NSTUB(    ???,  GUI_SetCFnForTab4)                       /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(0xFF075934,  dma_memcpy)

/** Debug messages **/
NSTUB(    0x2E0C,  dm_names)                                // in DebugMsg, before the 1st loop target
NSTUB(0xFF06E398,  DryosDebugMsg)
NSTUB(0xFF06E6F8,  dm_set_store_level)                      // called by "dmstore"

/** Dialog API **/
NSTUB(0xFF31E390,  dialog_redraw)
NSTUB(0xFF234DF4,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF476BF4,  ErrCardForLVApp_handler)                 // LiveViewErrorApp, as in 60D
NSTUB(0xFF3629B0,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF378B34,  HideUnaviFeedBack_maybe)                 // look for StartUnaviFeedBackTimer
NSTUB(0xFF37D5C4,  LiveViewApp_handler)
NSTUB(0xFF37DC4C,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF37F29C,  LiveViewApp_handler_end)                 // start of next function
NSTUB(0xFF398ABC,  PlayMain_handler)
NSTUB(0xFF4B1920,  PlayMovieGuideApp_handler)
NSTUB(0xFF38C928,  ShootOlcApp_handler)
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */
// NSTUB(    ???,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */

/** Display **/
NSTUB(0xFF33BF7C,  MirrorDisplay)
NSTUB(0xFF33BFDC,  NormalDisplay)
NSTUB(0xFF33BFAC,  ReverseDisplay)

/** EDMAC **/
NSTUB(0xFF1E06DC,  ConnectReadEDmac)
NSTUB(0xFF1E0618,  ConnectWriteEDmac)
NSTUB(0xFF1E0A78,  RegisterEDmacAbortCBR)
NSTUB(0xFF1E0A50,  RegisterEDmacCompleteCBR)
NSTUB(0xFF1E0B80,  RegisterEDmacPopCBR)
NSTUB(0xFF1E0560,  SetEDmac)
NSTUB(0xFF1E0860,  StartEDmac)
NSTUB(0xFF1E0AB8,  UnregisterEDmacAbortCBR)
NSTUB(0xFF1E0A64,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF1E0B94,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF227F64,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF1E195C, _EngDrvOut)
NSTUB(0xFF1E1D20, _engio_write)
NSTUB(0xFF1E184C,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF06F260,  call)

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFF0765EC,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF0765A0,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF076704,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF073B98,  GetFirstChunkFromSuite)                  // AJ_PackMemory_PackMem_p3
NSTUB(0xFF0735E0,  GetMemoryAddressOfMemoryChunk)           // AJ_PackMemory_PackMem.c_IsChunkSignature
NSTUB(0xFF07432C,  GetNextMemoryChunk)
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */

/** GUI for old cameras **/
NSTUB(    0x4588,  gui_timer_struct)                        // in GUI_Control_Post
NSTUB(0xFF0A9E34,  gui_change_mode)                         // GUI_ChangeMode_Post
NSTUB(0xFF0AA5A4,  gui_local_post)
NSTUB(0xFF0AAB3C,  gui_other_post)
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

/** GUI timers **/
// NSTUB(    ???,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
NSTUB(    0x6290,  mvr_config)
NSTUB(0xFF1C9A74,  mvrFixQScale)
NSTUB(0xFF1C9550,  mvrSetDefQScale)
NSTUB(0xFF1C9588,  mvrSetFullHDOptSize)
NSTUB(0xFF1C9778,  mvrSetGopOptSizeFULLHD)

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xFF08397C,  DispSensorStart)
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x1574C,  sd_device)                               // in sdReadBlk
NSTUB(0xFF4319A4,  fsuDecodePartitionTable)
//NSTUB( 0x15728, cf_device )                // in cfReadBlk

/** Memory allocation **/
NSTUB(0xFF072194,  GetMemoryInformation)                    // called from AllocateMemory
NSTUB(0xFF072140,  GetSizeOfMaxRegion)
NSTUB(0xFF0723F0, _AllocateMemory)                          // "[MEM] NG AllocateMemory"
NSTUB(0xFF072780, _FreeMemory)
NSTUB(0xFF1FD2E4, _alloc_dma_memory)
NSTUB(0xFF018DB0, _free)
NSTUB(0xFF1FD318, _free_dma_memory)
NSTUB(0xFF018D58, _malloc)                                  // maybe it's better to use Allocate/Free?
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFF1F5C80,  msg_queue_count)
NSTUB(0xFF1F5944,  msg_queue_create)                        // CreateMessageQueue
NSTUB(0xFF1F5C40,  msg_queue_post)
NSTUB(0xFF1F5A54,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF2A49A0,  ptp_register_handler)
// NSTUB(    ???,  ptpPropSetUILock)                        /* present on 650D.104, 700D.113 */

/** Properties **/
NSTUB(0xFF05CB28,  PROPAD_GetPropertyData)
NSTUB(0xFF05BE6C, _prop_cleanup)
NSTUB(0xFF05B6E4, _prop_request_change)
NSTUB(0xFF05BCC4,  prop_deliver)
NSTUB(0xFF05BD4C,  prop_register_slave)                     // called by"GUI_RegisterPropertySlave"
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
// NSTUB(    ???,  BulkInIPCTransfer)                       /* present on 7D.203 */
// NSTUB(    ???,  BulkOutIPCTransfer)                      /* present on 7D.203 */
// NSTUB(    ???,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
// NSTUB(    ???,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFF1F5D8C,  AcquireRecursiveLock)
NSTUB(0xFF075500,  CreateRecursiveLock)
NSTUB(0xFF1F5EA0,  ReleaseRecursiveLock)

/** ResLock **/
// NSTUB(    ???,  CreateResLockEntry)                      /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LockEngineResources)                     /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  UnLockEngineResources)                   /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 650D.104, 1100D.105, EOSM.202, 700D.113 */

/** Semaphores **/
NSTUB(0xFF070E90,  create_named_semaphore)
NSTUB(0xFF0710D8,  give_semaphore)
NSTUB(0xFF070FF0,  take_semaphore)

/** Task info **/
NSTUB(    0x3328,  task_max)
NSTUB(0xFF017618,  get_current_task)                        // called from AJ_armlib_setup_related3
NSTUB(0xFF080ECC,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF3557F0,  is_taskid_valid)                         // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF0714C4,  msleep)                                  // called from EV_GETDONE_CACHE param 0x64

/** Tasks **/
NSTUB(    0x193C,  task_dispatch_hook)
NSTUB(0xFF071578,  task_create)
NSTUB(0xFF07E3F4,  task_trampoline)
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(   0x73590,  lv_path_struct)                          // first arg of SelectPathDriveMode
NSTUB(0xFF069AE0,  LoadCalendarFromRTC)
NSTUB(0xFF1F94BC,  vsnprintf)
//NSTUB( 0xff37aee4, ChangeHDMIOutputSizeToFULLHD)
//NSTUB( 0xff37b174, ChangeHDMIOutputSizeToVGA)
// NSTUB(    ???,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB( 0xFF34B724, AJ_guess_LED_OFF )
///NSTUB( 0xFF34B6F4, AJ_guess_LED_ON )   // led for SD/CF read and write ops
///NSTUB(0xFF1E09B0,  AbortEDmac)
///NSTUB(0xFF29E25C,  AfCtrl_SetLensParameterRemote)
///NSTUB(0xFF37FAF4,  CancelBottomInfoDispTimer)
///NSTUB(0xFF234FCC,  ChangeColorPalette)
///NSTUB(0xFF31DA7C,  CreateDialogBox)
///NSTUB(0xFF31D214,  DeleteDialogBox)
///NSTUB(0xFF23081C,  GUI_GetFirmVersion)
///NSTUB(0xFF1115DC,  GetJpegBufForLV)
///NSTUB(0xFF1115E8,  GetJpegSizeForLV)
///NSTUB(0xFF073910, GetNumberOfChunks)
///NSTUB(0xFF073580, GetRemainOfMemoryChunk)
///NSTUB(0xFF0735B0, GetSizeOfMemoryChunk)
///NSTUB(0xFF0738AC, GetSizeOfMemorySuite)
///NSTUB(0xFF378968,  HideBottomInfoDisp_maybe)                // look for StartBottomInfoDispTimer
///NSTUB(   0x3787C,  LV_EX_X)
///NSTUB(   0x37880,  LV_EX_Y)
///NSTUB(0xFF1E963C,  MuteOff_0)
///NSTUB(0xFF1E95F4,  MuteOn_0)
///NSTUB(0xFF039D4C,  PD_GetBatteryPower)
///NSTUB(0xFF0396E4,  PD_RemoteRelease)
///NSTUB(0xFF05BF04,  PROP_GetMulticastProperty)
///NSTUB(0xFF1E09F8,  PopEDmac)
///NSTUB( 0xFF1A16D0, ReadyLiveViewDevice )
///NSTUB( 0xff33a734, RedrawDisplay )
///NSTUB(0xFF14B9E4,  RemoteRelease)
///NSTUB(0xFF062CEC,  SetBitmapVramAddress)                    // where to find bmp_vram_info
///NSTUB( 0xFF1A1CA4, SleepLiveViewDevice )               // not found
///NSTUB(0xFF072280, _AllocateMemory_do)                       // no err70, malloc structure at 0x3070
///NSTUB(0xFF05BF04, _prop_get_value)
///NSTUB(0xFF06DFCC,  bootdisk_disable)
///NSTUB(0xFF06DFC0,  bootdisk_enable)
///NSTUB( 0xFF4F2648, cfReadBlk )
///NSTUB(0xFF010500,  cli_save)
///NSTUB(0xFF31C444,  ctrlman_dispatch_event)
///NSTUB(0xFF070F38,  delete_semaphore)
///NSTUB(0xFF1E8FE0,  dispcheck)
///NSTUB(0xFF06E690,  dm_set_print_level)
///NSTUB(0xFF06E318,  dmstart)
///NSTUB(0xFF06E358,  dmstop)
///NSTUB(0xFF06ED38,  dumpf)
///NSTUB(0xFF14B8F4,  gui_lock)                                // PtpPropSetUILock
///NSTUB(0xFF56DCCC,  gui_main_task_functbl)                   // 7 functions
///NSTUB(0xFF31C594,  gui_task_create)
///NSTUB(0xFF31C6B4,  gui_task_destroy)
///NSTUB(    0x1A7C,  hotplug_struct)
///NSTUB(0xFF01B5D8,  hotplug_task)
///NSTUB(    0x1AC8,  hotplug_usb_buf)                         // after "USB Disconnect"
///NSTUB(0xFF04E9E4,  mvrSetBitRate)
///NSTUB(0xFF1C94E0,  mvrSetDeblockingFilter)
///NSTUB(0xFF1C97FC,  mvrSetGopOptSizeHD)
///NSTUB(0xFF1C9874,  mvrSetGopOptSizeVGA)
///NSTUB(0xFF1C96E8,  mvrSetHDOptSize)
///NSTUB(0xFF1C9A94,  mvrSetPrintMovieLog)
///NSTUB(0xFF1C9730,  mvrSetVGAOptSize)
///NSTUB(0xFF06FCE4,  oneshot_timer)
///NSTUB(0xFF230E48,  prop_request_icu_auto_poweroff)          // "PROP_Request PROP_ICU_AUTO_POWEROFF(%d)"
///NSTUB(0xFF14BAC8,  ptpPropButtonSW1)
///NSTUB(0xFF14BC30,  ptpPropButtonSW2)
///NSTUB(0xFF1FAEEC,  register_func)
///NSTUB(0xFF4F838C,  sdReadBlk)
///NSTUB(0xFF010514,  sei_restore)
///NSTUB(0xFF2E1124,  vram_get_number)                         // after "[ImgPlyer]   Rate"
///NSTUB(0xFF1E7C04,  vram_get_pos_and_size)
///NSTUB(   0x75DDC,  vram_info)
