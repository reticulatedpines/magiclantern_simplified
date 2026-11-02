/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.2 firmware for the 1200D / Rebel T5.
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

.text

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )       //0xff0c0000
NSTUB(0xFF0C1028,  cstart)
NSTUB(0xFF623B60,  bzero32)
NSTUB(0xFF0C7DF0,  create_init_task)
NSTUB(0xFF0C9648,  init_task)
NSTUB(    0x7B98,  additional_version)

/** File I/O **/
NSTUB(0xFF2A2930,  FIO_CloseFile)
NSTUB(0xFF2A3900,  FIO_FindClose)                           // FindClose
NSTUB(0xFF2A3820,  FIO_FindNextEx)
NSTUB(0xFF2A26E0, _FIO_ReadFile)
NSTUB(0xFF2A2790,  FIO_SeekSkipFile)
NSTUB(0xFF2A2880, _FIO_WriteFile)
NSTUB(0xFF2A316C, _FIO_CreateDirectory)
NSTUB(0xFF2A258C, _FIO_CreateFile)
NSTUB(0xFF2A372C, _FIO_FindFirstEx)
NSTUB(0xFF2A2AAC, _FIO_GetFileSize)
NSTUB(0xFF2A24D0, _FIO_OpenFile)
NSTUB(0xFF2A2638, _FIO_RemoveFile)
NSTUB(0xFF2A2FA4, _FIO_RenameFile)

/** GUI **/
NSTUB(    0x1C18,  gui_main_struct)
NSTUB(    0xA528,  gui_task_list)
NSTUB(0xFF0D1590,  GUI_ChangeMode)
NSTUB(0xFF0D1490,  GUI_Control)
NSTUB(0xFF2E94E8,  GUI_SetLvMode)                           /* ??? present on 500D.111, 50D.109, 5D2.212 */
NSTUB(0xFF2E971C,  Gui_SetSoundRecord)                      /* ??? present on 6D.113, 5D2.212 */
NSTUB(0xFF157514,  SetGUIRequestMode)
NSTUB(0xFF0D1AF0,  gui_init_end)
NSTUB(0xFF0D10D4,  gui_main_task)
NSTUB(0xFF159590,  gui_massive_event_loop)
// NSTUB(    ???,  GUI_SetMovieSize_a)                      /* present on 50D.109 */
// NSTUB(    ???,  GUI_SetMovieSize_b)                      /* present on 50D.109 */

/** ASIF **/
NSTUB(0xFF108C54,  PowerAudioOutput)
NSTUB(0xFF11CFE8,  PowerMicAmp)
NSTUB(0xFF10656C,  SetASIFMode)
NSTUB(0xFF108C20,  SetAudioVolumeIn)                        /* present on 6D.113 */
NSTUB(0xFF11BC20,  SetAudioVolumeOut)                       // keep volume
NSTUB(0xFF106888,  SetNextASIFADCBuffer)                    // mk11174
NSTUB(0xFF106AF4,  SetNextASIFDACBuffer)                    // mk11174
NSTUB(0xFF11C948,  SetSamplingRate)
NSTUB(0xFF10888C,  SoundDevActiveIn)
NSTUB(0xFF108AE0,  SoundDevShutDownIn)
NSTUB(0xFF105E10,  StartASIFDMAADC)
NSTUB(0xFF1061D0,  StartASIFDMADAC)
NSTUB(0xFF1060A8,  StopASIFDMAADC)                          /* present on 7D.203, 6D.113, EOSM.202 */
NSTUB(0xFF106444,  StopASIFDMADAC)

/** Audio **/
NSTUB(    0x1E50,  sounddev)
NSTUB(0xFF2AD554, _audio_ic_read)
NSTUB(0xFF2AD25C, _audio_ic_write)
NSTUB(0xFF69B490,  audio_thresholds)
NSTUB(0xFF10888C,  sounddev_active_in)
NSTUB(0xFF108488,  sounddev_task)
// NSTUB(    ???, _audio_ic_write_bulk)                     /* present on 6D.116 */

/** Bitmap **/
NSTUB(   0x1BBE0,  LCD_Palette)
NSTUB(   0x1D3E0,  bmp_vram_info)

/** CFN **/
NSTUB(0xFF5FB8F0,  GetCFnData)
NSTUB(0xFF5FBB1C,  SetCFnData)
// NSTUB(0xFF42AEE0,  GUI_SetCFnForTab4)                    /* present on 7D.203 */
// NSTUB(0xFF42AF38,  GUI_GetCFnForTab4)                    /* present on 7D.203 */

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(0xFF126CFC,  dma_memcpy)

/** Debug messages **/
NSTUB(    0x2440,  dm_names)
NSTUB(0xFF11F5DC,  DryosDebugMsg)
NSTUB(0xFF11F93C,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF3E63A4,  dialog_redraw)
NSTUB(0xFF2F5E1C,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF532008,  ErrCardForLVApp_handler)
NSTUB(0xFF4250E0,  ErrForCamera_handler)
NSTUB(0xFF43C08C,  HideUnaviFeedBack_maybe)
NSTUB(0xFF440B90,  LiveViewApp_handler)
NSTUB(0xFF441218,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF442864,  LiveViewApp_handler_end)                 // mk11174
NSTUB(0xFF44E238,  LiveViewWbApp_handler)                   /* present on 7D.203, 5D3.113, 5D2.212 */
NSTUB(0xFF45BDB4,  PlayMain_handler)
NSTUB(0xFF56C22C,  PlayMovieGuideApp_handler)               // mk11174
NSTUB(0xFF44F9DC,  ShootOlcApp_handler)
NSTUB(0xFF56D8B0,  StartPlayProtectGuideApp)                /* present on 7D.203, 5D2.212 */
NSTUB(0xFF56D4D0,  StopPlayProtectGuideApp)                 /* present on 7D.203, 5D2.212 */
// NSTUB(    ???,  LiveViewLevelApp_handler)                /* present on 5D3.113 */
// NSTUB(    ???,  LiveViewShutterApp_handler)
// NSTUB(    ???,  LiveViewWifiApp_handler)                 /* present on 6D.113 */

/** Display **/
NSTUB(0xFF403F90,  MirrorDisplay)
NSTUB(0xFF403FF0,  NormalDisplay)
NSTUB(0xFF403FC0,  ReverseDisplay)

/** EDMAC **/
NSTUB(0xFF29CC0C,  AbortEDmac)
NSTUB(0xFF29C938,  ConnectReadEDmac)
NSTUB(0xFF29C874,  ConnectWriteEDmac)
NSTUB(0xFF29CCD4,  RegisterEDmacAbortCBR)
NSTUB(0xFF29CCAC,  RegisterEDmacCompleteCBR)
NSTUB(0xFF29CDDC,  RegisterEDmacPopCBR)
NSTUB(0xFF29C7BC,  SetEDmac)
NSTUB(0xFF29CABC,  StartEDmac)
NSTUB(0xFF29CD14,  UnregisterEDmacAbortCBR)
NSTUB(0xFF29CCC0,  UnregisterEDmacCompleteCBR)
NSTUB(0xFF29CDF0,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF2E8D5C,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF29DBB8, _EngDrvOut)
NSTUB(0xFF29DF84, _engio_write)
NSTUB(0xFF29DAA8,  shamem_read)
// NSTUB(    ???,  EngDrvOut)                               /* present on 7D_MASTER.203 */

/** Eventprocs (call by name) **/
NSTUB(0xFF1204A4,  call)

/** Events **/
NSTUB(0xFF2B9CD8,  TryPostEvent)
NSTUB(0xFF2B9D34,  TryPostEvent_end)
NSTUB(0xFF2B9360,  TryPostStageEvent)
NSTUB(0xFF2B93BC,  TryPostStageEvent_end)

/** ExMem **/
NSTUB(0xFF124E8C,  AddMemoryChunk)                          // called before " 32(Src,bytes,Unit)=(%#lx,%#lx,%#lx)" in many places; see also hAddSuite
NSTUB(0xFF1279B4,  AllocateContinuousMemoryResource)
NSTUB(0xFF127968,  AllocateMemoryResource)
NSTUB(0xFF124408,  CreateMemoryChunk)                       // called from CreateMemorySuite
NSTUB(0xFF124BE8,  CreateMemorySuite)
NSTUB(0xFF124A1C,  DeleteMemorySuite)
NSTUB(0xFF127ACC,  FreeMemoryResource)                      // mk11174
NSTUB(0xFF124F60,  GetFirstChunkFromSuite)
NSTUB(0xFF1249A8,  GetMemoryAddressOfMemoryChunk)
NSTUB(0xFF1256F4,  GetNextMemoryChunk)
NSTUB(0xFF0D9B14,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0DC3A8,  SRM_FreeMemoryResourceFor1stJob)

/** GUI for old cameras **/
NSTUB(    0x3C90,  gui_timer_struct)                        /* present on 7D.203, 500D.111, 60D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */   // mk11174
NSTUB(0xFF15B430,  gui_change_mode)                         /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */            // mk11174
NSTUB(0xFF15C470,  gui_change_shoot_type_post)              /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
NSTUB(0xFF15BBA0,  gui_local_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
NSTUB(0xFF15C150,  gui_other_post)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105 */
// NSTUB(    ???,  gui_change_lcd_state_post)               /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_init_event)                          /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_post_10000085)                       /* present on 500D.111, 50D.109, 5D2.212 */
// NSTUB(    ???,  gui_timer_something)                     /* present on 7D.203, 500D.111, 50D.109, 5D2.212 */

/** GUI timers **/
NSTUB(0xFF437180,  CancelDateTimer)                         /* present on 7D.203, 6D.113, 5D3.113 */
NSTUB(0xFF1292BC,  CancelTimer)
NSTUB(0xFF120F28,  SetHPTimerAfterNow)
NSTUB(0xFF12101C,  SetHPTimerNextTick)
NSTUB(0xFF1290BC,  SetTimerAfter)

/** H264 Encoder **/
NSTUB(    0x5A14,  mvr_config)
NSTUB(0xFF27FEF4,  mvrFixQScale)
NSTUB(0xFF27F9D0,  mvrSetDefQScale)
NSTUB(0xFF27FA08,  mvrSetFullHDOptSize)                     /* present on 500D.111, 50D.109, 600D.102, 550D.109, 1100D.105 */
NSTUB(0xFF27FBF8,  mvrSetGopOptSizeFULLHD)                  /* present on 600D.102, 550D.109, 1100D.105 */

/** Interrupts **/
// NSTUB(    ???,  isr_table_handler)                       /* present on 7D_MASTER.203 */
// NSTUB(    ???,  isr_table_param)                         /* present on 7D_MASTER.203 */
// NSTUB(    ???,  post_isr_hook)                           /* present on 7D.203, 6D.113, 5D3.113, EOSM.202 */
// NSTUB(    ???,  pre_isr_hook)                            /* present on 7D.203, 6D.113, 5D3.113, 7D_MASTER.203, EOSM.202 */

/** LCD Sensor **/
NSTUB(0xFF134BB8,  DispSensorStart)
// NSTUB(    ???,  LightMeasure_n_Callback_r0)              /* present on 7D.203, 5D2.212 */

/** Making the card bootable **/
NSTUB(   0x14FEC,  cf_device)                               // mk11174
NSTUB(   0x15010,  sd_device)                               // mk11174 (0x15010)
NSTUB(0xFF4F93EC,  fsuDecodePartitionTable)
// NSTUB(    ???,  cf_device_ptr)                           /* present on 5D3.113 */

/** Memory allocation **/
NSTUB(0xFF123440,  GetMemoryInformation)
NSTUB(0xFF1233EC,  GetSizeOfMaxRegion)
NSTUB(0xFF12369C, _AllocateMemory)
NSTUB(0xFF123A2C, _FreeMemory)
NSTUB(0xFF2BB314, _alloc_dma_memory)
NSTUB(0xFF0C8BFC, _free)                                    // see "can't malloc"
NSTUB(0xFF2BB348, _free_dma_memory)
NSTUB(0xFF0C8BA4, _malloc)
// NSTUB(    ???,  free_dma_memory)                         /* present on 7D_MASTER.203 */

/** Message queues **/
NSTUB(0xFF2B3F80,  msg_queue_count)
NSTUB(0xFF2B3C44,  msg_queue_create)
NSTUB(0xFF2B3F40,  msg_queue_post)
NSTUB(0xFF2B3D54,  msg_queue_receive)

/** PTP **/
NSTUB(0xFF1FFF84,  ptpPropSetUILock)
NSTUB(0xFF369BDC,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF10C850, _prop_cleanup)
NSTUB(0xFF10C0C8, _prop_request_change)
NSTUB(0xFF10C6A8,  prop_deliver)
NSTUB(0xFF10C730,  prop_register_slave)
// NSTUB(    ???,  prop_unregister_slave)                   /* present on 7D.203 */

/** RPC (for 7D) **/
NSTUB(0xFF2B2A78,  BulkInIPCTransfer)                       /* present on 7D.203 */
NSTUB(0xFF2B29A8,  BulkOutIPCTransfer)                      /* present on 7D.203 */
NSTUB(0xFF2B2BF8,  QuickOutIPCTransfer)                     /* present on 7D.203 */
NSTUB(0xFF2B3178,  RegisterRPCHandler)                      /* present on 7D.203, 7D_MASTER.203 */
NSTUB(0xFF2B3228,  RequestRPC)                              /* present on 7D.203, 7D_MASTER.203 */

/** Recursive locks **/
NSTUB(0xFF2B408C,  AcquireRecursiveLock)
NSTUB(0xFF2B3FF4,  CreateRecursiveLock)
NSTUB(0xFF2B41A0,  ReleaseRecursiveLock)

/** ResLock **/
NSTUB(0xFF2A0B8C,  CreateResLockEntry)
NSTUB(0xFF2A10CC,  LockEngineResources)
NSTUB(0xFF2A1208,  UnLockEngineResources)

/** Semaphores **/
NSTUB(0xFF12213C,  create_named_semaphore)
NSTUB(0xFF122384,  give_semaphore)
NSTUB(0xFF12229C,  take_semaphore)

/** Task info **/
NSTUB(    0x295C,  task_max)
NSTUB(0xFF417ED4,  is_taskid_valid)
NSTUB(    0x1A2C,  current_task)
NSTUB(     0x674,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored

/** Task sleep **/
NSTUB(0xFF122770,  msleep)

/** Tasks **/
NSTUB(    0x193C,  task_dispatch_hook)
NSTUB(0xFF122824,  task_create)
NSTUB(0xFF12F67C,  task_trampoline)                         // mk11174
// NSTUB(    ???,  current_task_ctx)                        /* present on 7D.203 */
// NSTUB(    ???,  next_task_ctx)                           /* present on 7D.203 */

/** Misc **/
NSTUB(0xFF114874,  EnableImagePhysicalScreenParameter)      /* present on 7D.203, 50D.109, 5D2.212 */
NSTUB(0xFF11AEE0,  LoadCalendarFromRTC)
NSTUB(   0x753F0,  lv_path_struct)                          /* present on 60D.111, 600D.102, 550D.109, 1100D.105 */   // mk11174
NSTUB(0xFF2B74D8,  vsnprintf)
// NSTUB(    ???,  PtpDps_remote_release_SW1_SW2_worker)    /* present on 7D.203, 5D2.212 */

/** Unused stubs or porting notes **/
//NSTUB(0xFF10D50C,  PROPAD_GetPropertyData)               /* present on 7D.203, 60D.111, 6D.113, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109 */
//NSTUB(0xFF623904,  memcpy)                               /* present on 7D_MASTER.203 */
