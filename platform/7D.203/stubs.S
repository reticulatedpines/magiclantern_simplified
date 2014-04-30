/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 2.0.3 firmware for the 7D.
 *
 */
/*
 * Copyright (C) 2010 Magic Lantern Team
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
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF010FB0,  cstart)
NSTUB(0xFF50CDEC,  bzero32)
NSTUB(0xFF01559C,  create_init_task)
NSTUB(0xFF018734,  init_task)
NSTUB(   0x16FB8,  additional_version)

/** File I/O **/
NSTUB(0xFF1FC684,  FIO_CloseFile)
NSTUB(0xFF1FD554,  FIO_FindClose)                           // AJ__switch_0x1A50_n_calls_fstOpenDir
NSTUB(0xFF1FD42C,  FIO_FindNextEx)
NSTUB(0xFF1FC434,  FIO_ReadFile)
NSTUB(0xFF1FC5B4,  FIO_SeekFile)
NSTUB(0xFF1FC5D4,  FIO_WriteFile)
NSTUB(0xFF1FCCF4, _FIO_CreateDirectory)                     // not sure. g3gg0: confirmed by caller 'ctgDirCreate'
NSTUB(0xFF1FC2E0, _FIO_CreateFile)
NSTUB(0xFF1FD2B8, _FIO_FindFirstEx)
NSTUB(0xFF1FC800, _FIO_GetFileSize)
NSTUB(0xFF1FC224, _FIO_OpenFile)
NSTUB(0xFF1FC38C, _FIO_RemoveFile)
// NSTUB(0xFF0455B4, _FIO_RenameFile)   // not sure it is ceresFIORename   /* present on 60D.111, 6D.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */

/** GUI **/
NSTUB(    0x1BF4,  gui_main_struct)
NSTUB(   0x1A904,  gui_task_list)
NSTUB(0xFF022004,  GUI_ChangeMode)
NSTUB(0xFF021F04,  GUI_Control)
NSTUB(0xFF237724,  GUI_SetLvMode)
NSTUB(0xFF237858,  Gui_SetSoundRecord)                      // WRONG NAME: it should be Gui_SetSoundRec
NSTUB(0xFF0A8738,  SetGUIRequestMode)
NSTUB(0xFF022680,  gui_init_end)
NSTUB(0xFF021B44,  gui_main_task)
NSTUB(0xFF0A9384,  gui_massive_event_loop)
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */

/** ASIF **/
NSTUB(0xFF07365C,  PowerAudioOutput)
NSTUB(0xFF237A90,  SetAudioVolumeIn)                        // WRONG NAME: it should be SetAudioVolume
NSTUB(0xFF073944,  SetAudioVolumeOut)
NSTUB(0xFF0621C4,  SetNextASIFADCBuffer)                    // called by SetNextUINT8ASIFADCBuffer and SetNextINT16ASIFADCBuffer
NSTUB(0xFF06227C,  SetNextASIFDACBuffer)
NSTUB(0xFF0736F4,  SetSamplingRate)
NSTUB(0xFF0640EC,  SoundDevActiveIn)
NSTUB(0xFF064304,  SoundDevShutDownIn)
NSTUB(0xFF061A88,  StartASIFDMAADC)
NSTUB(0xFF061C44,  StartASIFDMADAC)
NSTUB(0xFF061C08,  StopASIFDMAADC)                          // Called by SoundDevStopIn
NSTUB(0xFF061D20,  StopASIFDMADAC)

/** Audio **/
NSTUB(    0x1F54,  sounddev)
NSTUB(0xFF072F7C, _audio_ic_read)
NSTUB(0xFF0730C8, _audio_ic_write)
NSTUB(0xFF57B014,  audio_thresholds)
NSTUB(0xFF0640EC,  sounddev_active_in)
NSTUB(0xFF063C70,  sounddev_task)

/** Bitmap **/
NSTUB(   0x29A9C,  LCD_Palette)                             // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(   0x29A60,  bmp_vram_info)

/** CFN **/
NSTUB(0xFF3759F0,  GUI_GetCFnForTab4)
NSTUB(0xFF375998,  GUI_SetCFnForTab4)
NSTUB(0xFF4C621C,  GetCFnData)
NSTUB(0xFF4C6424,  SetCFnData)

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(0xFF080224,  dma_memcpy)

/** Debug messages **/
NSTUB(    0x2B18,  dm_names)
NSTUB(0xFF0776AC,  DryosDebugMsg)
NSTUB(0xFF077990,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF300640,  dialog_redraw)
NSTUB(0xFF361BDC,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF430250,  ErrCardForLVApp_handler)
NSTUB(0xFF336D60,  ErrForCamera_handler)                    // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF347918,  LiveViewApp_handler)
NSTUB(0xFF347E6C,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF348D94,  LiveViewApp_handler_end)
NSTUB(0xFF3674A4,  LiveViewWbApp_handler)
NSTUB(0xFF3552DC,  PlayMain_handler)
NSTUB(0xFF42B700,  PlayMovieGuideApp_handler)
NSTUB(0xFF35DF58,  ShootOlcApp_handler)                     // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
NSTUB(0xFF41E17C,  StartPlayProtectGuideApp)
NSTUB(0xFF41DDE0,  StopPlayProtectGuideApp)
// NSTUB(    ???,  HideUnaviFeedBack_maybe)                 /* present on 60D.111, 6D.113, 5D3.113, 600D.102, 650D.104, 550D.109, 1100D.105, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)              /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */

/** Display **/
// NSTUB(    ???,  MirrorDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  NormalDisplay)                           /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */
// NSTUB(    ???,  ReverseDisplay)                          /* present on 60D.111, 600D.102, 650D.104, 550D.109, 1100D.105, 700D.113 */

/** EDMAC **/
NSTUB(0xFF1F54DC,  ConnectReadEDmac)
NSTUB(0xFF1F5418,  ConnectWriteEDmac)
NSTUB(0xFF1F5878,  RegisterEDmacAbortCBR)
NSTUB(0xFF1F5850,  RegisterEDmacCompleteCBR)
NSTUB(0xFF1F5980,  RegisterEDmacPopCBR)
NSTUB(0xFF1F5360,  SetEDmac)
NSTUB(0xFF1F5660,  StartEDmac)
NSTUB(0xFF1F58B8,  UnregisterEDmacAbortCBR)
NSTUB(0xFF1F5864,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF1F5994,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF236E9C,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF1F675C, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF1F6B20, _engio_write)
NSTUB(0xFF1F664C,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF07842C,  call)

/** Events **/
// NSTUB(    ???,  TryPostEvent)                            /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostEvent_end)                        /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent)                       /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  TryPostStageEvent_end)                   /* present on 6D.113, 650D.104, EOSM.202, 700D.113 */

/** ExMem **/
NSTUB(0xFF080EC4,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF080E78,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF080FDC,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF07E4A0,  GetFirstChunkFromSuite)
NSTUB(0xFF07DEE8,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFF07EC38,  GetNextMemoryChunk)
// NSTUB(    ???,  CreateMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */
// NSTUB(    ???,  DeleteMemorySuite)                       /* present on 6D.113, 5D3.113, 650D.104, EOSM.202, 700D.113 */

/** GUI for old cameras **/
NSTUB(    0x3624,  gui_timer_struct)
NSTUB(0xFF0AB97C,  gui_change_lcd_state_post)
NSTUB(0xFF0AABC4,  gui_change_mode)
NSTUB(0xFF0AB8E4,  gui_change_shoot_type_post)
NSTUB(0xFF0AC000,  gui_init_event)
NSTUB(0xFF0AB1C0,  gui_local_post)
NSTUB(0xFF0AB5DC,  gui_other_post)
NSTUB(0xFF0AB858,  gui_post_10000085)
NSTUB(0xFF07B708,  gui_timer_something)

/** GUI timers **/
NSTUB(0xFF34E5B0,  CancelDateTimer)                         // Look for "CancelDateTimer" or "StopDateTimer" (7D)
// NSTUB(    ???,  SetTimerAfter)                           /* present on 650D.104 */
// NSTUB(    ???,  TimerCancel)                             /* present on 650D.104 */

/** H264 Encoder **/
NSTUB(    0xA39C,  mvr_config)
NSTUB(0xFF1E0E04,  mvrFixQScale)
NSTUB(0xFF1E08E0,  mvrSetDefQScale)
NSTUB(0xFF1E0918,  mvrSetFullHDOptSize)
NSTUB(0xFF1E0B08,  mvrSetGopOptSizeFULLHD)

/** Interrupts **/
NSTUB(    0x1940,  post_isr_hook)
NSTUB(    0x193C,  pre_isr_hook)
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */

/** LCD Sensor **/
NSTUB(0xFF08E350,  DispSensorStart)
NSTUB(0xFF202814,  LightMeasure_n_Callback_r0)

/** Making the card bootable **/
NSTUB(   0x22938,  cf_device)
NSTUB(0xFF3D7798,  fsuDecodePartitionTable)                 // AJ_fsuGetPart_related
    //NSTUB(   0x208D8, sd_device)

/** Memory allocation **/
NSTUB(0xFF07CAC8,  GetMemoryInformation)
NSTUB(0xFF07CA74,  GetSizeOfMaxRegion)
NSTUB(0xFF07CD24, _AllocateMemory)
NSTUB(0xFF07D0B4, _FreeMemory)
NSTUB(0xFF20DA50, _alloc_dma_memory)
NSTUB(0xFF01634C, _free)
NSTUB(0xFF20DA84, _free_dma_memory)
NSTUB(0xFF0162F0, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFF205E9C,  msg_queue_count)
NSTUB(0xFF07FC28,  msg_queue_create)
NSTUB(0xFF205E5C,  msg_queue_post)
NSTUB(0xFF205C70,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF15FC84,  ptpPropSetUILock)
NSTUB(0xFF2885CC,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF06934C,  PROPAD_GetPropertyData)
NSTUB(0xFF068734, _prop_cleanup)
NSTUB(0xFF067FAC, _prop_request_change)
NSTUB(0xFF06858C,  prop_deliver)
NSTUB(0xFF068614,  prop_register_slave)
NSTUB(0xFF0686A4,  prop_unregister_slave)

/** RPC (for 7D) **/
NSTUB(0xFF07A7D4,  BulkInIPCTransfer)
NSTUB(0xFF07A704,  BulkOutIPCTransfer)
NSTUB(0xFF07AED4,  RegisterRPCHandler)
NSTUB(0xFF07AF84,  RequestRPC)

/** Recursive locks **/
NSTUB(0xFF205FA8,  AcquireRecursiveLock)                    // AJ_KernelDry_KerRLock.c
NSTUB(0xFF07FE0C,  CreateRecursiveLock)
NSTUB(0xFF2060BC,  ReleaseRecursiveLock)                    // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF1F9694,  CreateResLockEntry)
NSTUB(0xFF1F9BDC,  LockEngineResources)
NSTUB(0xFF1F9D18,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFF07B7C4,  create_named_semaphore)
NSTUB(0xFF07BA0C,  give_semaphore)
NSTUB(0xFF07B924,  take_semaphore)

/** Task info **/
NSTUB(    0x2B24,  task_max)
NSTUB(0xFF014C10,  get_current_task)
NSTUB(0xFF08779C,  get_obj_attr)                            // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFF087940,  is_taskid_valid)                         // AJ_task_trampoline_related_p10

/** Task sleep **/
NSTUB(0xFF07BDF8,  msleep)

/** Tasks **/
NSTUB(     0x680,  current_task_ctx)
NSTUB(     0x67C,  next_task_ctx)
NSTUB(    0x1934,  task_dispatch_hook)
NSTUB(0xFF07BEAC,  task_create)
NSTUB(0xFF084CA4,  task_trampoline)

/** Misc **/
NSTUB(0xFF06E8B0,  EnableImagePhysicalScreenParameter)
NSTUB(0xFF075210,  LoadCalendarFromRTC)
NSTUB(0xFF16004C,  PtpDps_remote_release_SW1_SW2_worker)    // called from: ptpRemoteRelease Occupy
NSTUB(0xFF5517B0,  lv_path_struct)
NSTUB(0xFF209670,  vsnprintf)
// NSTUB(    ???,  memcpy)                                  /* present on 7D_MASTER.203 */

/** Unused stubs or porting notes **/
///NSTUB([idk], AbortEDmac)
///NSTUB(0xFF06EDD0,  AsyncEnableImagePhysicalScreenParameter)
///NSTUB(0xFF2FFD3C,  CreateDialogBox)
///NSTUB(0xFF2FF7C4,  DeleteDialogBox)
///NSTUB(0xFF23F414,  GUI_GetFirmVersion)
///NSTUB(0xFF11A8E0,  GetJpegBufForLV)
///NSTUB(0xFF11A8EC,  GetJpegSizeForLV)
///NSTUB(0xFF07DE88,  GetRemainOfMemoryChunk)
///NSTUB(0xFF242F38,  GuiEdLedBlink)
///NSTUB(0xFF242F14,  GuiEdLedOff)
///NSTUB(0xFF242EF0,  GuiEdLedOn)
///NSTUB(0xFF07AAFC,  MasterExecute)
///NSTUB(0xFF072290,  MuteOff_0)                               // not sure
///NSTUB(0xFF06D6A8,  MuteOn_0)
///NSTUB(0xFF35AC90,  OlcAFFrameApp_handler)
///NSTUB(0xFF03F4A8,  PD_RemoteRelease)
///NSTUB(0xFF0687C8,  PROP_GetMulticastProperty)
///NSTUB(0xFF3D2644,  PackMem_PopEDmacForMemorySuite)
///NSTUB(0xFF3D27CC,  PackMem_RegisterEDmacAbortCBRForMemorySuite)
///NSTUB(0xFF3D26EC,  PackMem_RegisterEDmacCompleteCBRForMemorySuite)
///NSTUB(0xFF3D275C,  PackMem_RegisterEDmacPopCBRForMemorySuite)
///NSTUB(0xFF3D24A0,  PackMem_SetEDmacForMemorySuite)
///NSTUB(0xFF3D2598,  PackMem_StartEDmac)
///NSTUB(0xFF1F5864,  PopEDmac)
///NSTUB(0xFF074410,  PowerMicAmp)
///NSTUB(0xFF07A954,  QuickOutIPCTransfer)
///NSTUB(0xFF317EE8,  RedrawDisplay)
///NSTUB(0xFF15FD74,  RemoteRelease)
///NSTUB(0xFF064D80,  SamplingParamInRate)
///NSTUB(0xFF06F2FC,  SetBitmapVramAddress)
///NSTUB(0xFF073F54,  SetFilterRec)
///NSTUB(0xFF064FF8,  SetVolumeInVolume)
///NSTUB(0xFF0650BC,  SetWindCutMode)
///NSTUB(0xFF064034,  SoundDevLock)
///NSTUB(0xFF06494C,  SoundDevStopIn)
///NSTUB(0xFF064A1C,  SoundDevStopOut)
///NSTUB(0xFF07443C,  UnpowerMicAmp)
///NSTUB(0xFF0687C8, _prop_get_value)
///NSTUB(0xFF44EBB0,  cfReadBlk)                               // might be good (dumps=1, score=6.1)  g3gg0: confirmed by debugmsg 'cfReadBlk'
///NSTUB(0xFF0104BC,  cli_save)
///NSTUB(0xFF2FE9F4,  ctrlman_dispatch_event)
///NSTUB(0xFF07B86C,  delete_semaphore)
///NSTUB(0xFF0AB858,  gui_post_100000a1)
///NSTUB(0xFF2FEB44,  gui_task_create)
///NSTUB(0xFF2FEC64,  gui_task_destroy)
///NSTUB(0xFF1E0870,  mvrSetDeblockingFilter)
///NSTUB(0xFF1E0E24,  mvrSetPrintMovieLog)
///NSTUB(0xFF078E6C,  oneshot_timer)
///NSTUB(0xFF20AE48,  register_func)
///NSTUB(0xFF454540,  sdReadBlk)                               // g3gg0: confirmed by DebugMsg - but makes no sense on 7D
///NSTUB(0xFF0104D0,  sei_restore)
///NSTUB(0xFF0705E4,  set_brightness_registers)
///NSTUB(0xFF2CB1E0,  vram_get_number)
///NSTUB(   0x7E9E8,  vram_info)
