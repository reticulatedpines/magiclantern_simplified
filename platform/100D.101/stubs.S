/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the 100D / SL1
 *
 */
/*
 * Copyright (C) 2012 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License , or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful ,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not , write to the
 * Free Software Foundation , Inc. ,
 * 51 Franklin Street , Fifth Floor ,
 * Boston , MA  02110-1301 , USA.
 */

#include <stub.h>

.text
#define RAM_OFFSET 0xFFA9C4B8                               // (0xFFA9DDB8-0x1900)

/** Startup **/
NSTUB( ROMBASEADDR, firmware_entry )
NSTUB(0xFF0C1BD0,  cstart)                                    
NSTUB(   0x5E3A8,  bzero32)                                    
NSTUB(    0x2F28,  create_init_task)                        
NSTUB(0xFF0C56BC,  init_task)                                
// NSTUB(    ???,  additional_version)                      /* present on 7D.203, 500D.111, 60D.111, 5D3.113, 50D.109, 5D2.212, 600D.102, 550D.109, 1100D.105, 700D.113 */

/** File I/O **/
NSTUB(0xFF343230,  FIO_CloseFile)
NSTUB(0xFF344228,  FIO_FindClose)
NSTUB(0xFF344148,  FIO_FindNextEx)
NSTUB(0xFF342FE0, _FIO_ReadFile)
NSTUB(0xFF343090,  FIO_SeekSkipFile)
NSTUB(0xFF343180, _FIO_WriteFile)
NSTUB(0xFF343A8C, _FIO_CreateDirectory)
NSTUB(0xFF342E8C, _FIO_CreateFile)
NSTUB(0xFF344054, _FIO_FindFirstEx)
NSTUB(0xFF3433AC, _FIO_GetFileSize)
NSTUB(0xFF342DD0, _FIO_OpenFile)
NSTUB(0xFF342F38, _FIO_RemoveFile)
NSTUB(0xFF3438C4, _FIO_RenameFile)

/** GUI **/
NSTUB(   0x6557C,  gui_main_struct)
NSTUB(   0x76258,  gui_task_list)
NSTUB(0xFF0D6344,  GUI_Control) 
NSTUB(0xFF196DEC,  SetGUIRequestMode)
NSTUB(0xFF0D69E8,  gui_init_end)
NSTUB(0xFF0D5F70,  gui_main_task)
// NSTUB(0xFF0D6504,  GUI_ChangeMode)
// NSTUB(0xFF1A4058,  Gui_SetSoundRecord)

/** ASIF **/
NSTUB(0xFF13DF78,  PowerAudioOutput)
NSTUB(0xFF13DB5C,  PowerMicAmp)                             /* present on 650D.104, EOSM.202, 700D.113 */
NSTUB(0xFF13BF38,  SetAudioVolumeOut)
NSTUB(0xFF112EF0,  SetNextASIFADCBuffer)
NSTUB(0xFF1130D8,  SetNextASIFDACBuffer)
NSTUB(0xFF13D33C,  SetSamplingRate)                         // AudioIC.c_SetRate_hHPTimer Identical
NSTUB(0xFF1123D4,  StartASIFDMAADC)
NSTUB(0xFF1127A8,  StartASIFDMADAC)
NSTUB(0xFF112A28,  StopASIFDMADAC)
NSTUB(0xFF112680,  StopASIFDMAADC)
NSTUB(0xFF114EB0,  SoundDevActiveIn)
NSTUB(0xFF115108,  SoundDevShutDownIn)

/** Audio **/
NSTUB(   0x65900,  sounddev)
NSTUB(0xFF336074, _audio_ic_read)
NSTUB(0XFF335CB4, _audio_ic_write)
NSTUB(0xFF827898,  audio_thresholds)
NSTUB(0xFF114AA0,  sounddev_task)

/** Bitmap **/
NSTUB(   0x8D63C,  LCD_Palette)
NSTUB(   0x8EE3C,  bmp_vram_info)

/** CFN **/
NSTUB(0xFF6995F8,  GetCFnData)
NSTUB(0xFF699810,  SetCFnData)

/** Camera interface? **/
NSTUB(0xC0220000,  camera_engine)

/** DMA **/
NSTUB(    0xB950,  dma_memcpy)

/** Debug messages **/
NSTUB(    0x4A74,  DryosDebugMsg)
NSTUB(   0x66038,  dm_names)
NSTUB(    0x4DE0,  dm_set_store_level)

/** Dialog API **/
NSTUB(0xFF4C3300,  dialog_redraw)
NSTUB(0xFF538FEC,  dialog_set_property_str)

/** Dialog handlers **/
NSTUB(0xFF6BB02C,  ErrCardForLVApp_handler)
NSTUB(0xFF52B010,  ErrForCamera_handler)
NSTUB(0xFF53FFCC,  HideUnaviFeedBack_maybe)
NSTUB(0xFF545F38,  LiveViewApp_handler)
NSTUB(0xFF5485FC,  LiveViewApp_handler_end)
NSTUB(0xFF546CC4,  LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF7103A4,  LiveViewShutterApp_handler)
NSTUB(0xFF72EC20,  PlayMain_handler)                        // StartPlayTouchPassFilterApp
NSTUB(0xFF56436C,  PlayMovieGuideApp_handler)               // StartPlayMovieGuideApp
NSTUB(0xFF553078,  ShootOlcApp_handler)

/** Display **/
NSTUB(0xFF4E6AD0,  MirrorDisplay)
NSTUB(0xFF4E6B30,  NormalDisplay)
NSTUB(0xFF4E6B00,  ReverseDisplay)

/** EDMAC **/
NSTUB(   0x3506C,  AbortEDmac)
NSTUB(   0x34BC0,  ConnectReadEDmac)
NSTUB(   0x34AFC,  ConnectWriteEDmac)
NSTUB(   0x351A0,  RegisterEDmacAbortCBR)
NSTUB(   0x3510C,  RegisterEDmacCompleteCBR)
NSTUB(   0x35234,  RegisterEDmacPopCBR)
NSTUB(   0x3499C,  SetEDmac)
NSTUB(   0x34D48,  StartEDmac)
NSTUB(   0x351DC,  UnregisterEDmacAbortCBR)
NSTUB(   0x35148,  UnregisterEDmacCompleteCBR)
NSTUB(   0x35270,  UnregisterEDmacPopCBR)

/** Electronic Level **/
NSTUB(0xFF274D88,  GUI_SetRollingPitchingLevelStatus)

/** Engio **/
NSTUB(0xFF2B2148, _EngDrvOut)                               // AJ_EngDrvOut_1xVar_to_ShadowStruct
NSTUB(0xFF2B2460, _engio_write)
NSTUB(0xFF2B21E4,  shamem_read)                             // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr

/** Eventprocs (call by name) **/
NSTUB(0xFF1433EC,  call)

/** Events **/
NSTUB(   0x3A8F0,  TryPostEvent)
// NSTUB(   0x3DD80,  TryPostEvent_end)                  
// NSTUB(   0x3D644,  TryPostStageEvent)
// NSTUB(   0x3D6A0,  TryPostStageEvent_end)

/** ExMem **/
NSTUB(    0xA53C,  CreateMemorySuite)
NSTUB(    0xA18C,  DeleteMemorySuite)
NSTUB(    0x9EA8,  GetFirstChunkFromSuite)
NSTUB(    0x9B24,  GetMemoryAddressOfMemoryChunk)
NSTUB(    0xABAC,  GetNextMemoryChunk)
NSTUB(0xFF146FD8,  AllocateContinuousMemoryResource)        // m_pfContAllocMemoryCBR
NSTUB(0xFF146F8C,  AllocateMemoryResource)                  // m_pfAllocMemoryCBR
NSTUB(0xFF1470F0,  FreeMemoryResource)                      // m_pfFreeMemoryCBR
NSTUB(0xFF0E6490,  SRM_AllocateMemoryResourceFor1stJob)
NSTUB(0xFF0EA02C,  SRM_FreeMemoryResourceFor1stJob)
NSTUB(    0x9D8C,  AddMemoryChunk)
NSTUB(    0x9578,  CreateMemoryChunk)

/** GUI timers **/
NSTUB(    0xCBC8,  CancelTimer)
NSTUB(    0x60F0,  SetHPTimerAfterNow)
NSTUB(    0xC9C8,  SetTimerAfter)
NSTUB(0xFF5364E0,  CancelDateTimer)
NSTUB(    0x61E0,  SetHPTimerNextTick)

/** H264 Encoder **/
NSTUB(   0x6BC24,  mvr_config)
NSTUB(0xFF2B13F8,  mvrFixQScale)                            // FIXME: use call()
NSTUB(0xFF2B0EF0,  mvrSetDefQScale)
// NSTUB(0xFF2BB18C,  mvrSetFullHDOptSize)
// NSTUB(0xFF2BB370,  mvrSetGopOptSizeFULLHD)

/** Interrupts **/
NSTUB(   0x651A4,  post_isr_hook)
NSTUB(   0x651A0,  pre_isr_hook)

/** LCD Sensor **/
NSTUB(0xFF150200,  DispSensorStart)

/** Making the card bootable **/
NSTUB(   0x7FBE8,  sd_device)
NSTUB(0xFF4FAED4,  fsuDecodePartitionTable)

/** Memory allocation **/
NSTUB(    0x8618,  GetMemoryInformation)
NSTUB(    0x85C4,  GetSizeOfMaxRegion)
NSTUB(    0x8878, _AllocateMemory)
NSTUB(    0x8C14, _FreeMemory)
NSTUB(   0x3AB38, _alloc_dma_memory)
NSTUB(   0x3AB6C, _free_dma_memory)
NSTUB(0xFF0C901C, _free)                                    // see "can't malloc"
NSTUB(0xFF0C8FC4, _malloc)

/** Message queues **/
NSTUB(   0x373F0,  msg_queue_count)
NSTUB(   0x370B4,  msg_queue_create)
NSTUB(   0x373B0,  msg_queue_post)
NSTUB(   0x371C4,  msg_queue_receive)

/** MPU communication **/
NSTUB(   0x36288, _mpu_send)                               /* dwSize < TXBD_DATA_SIZE */
NSTUB(    0x4040, _mpu_recv)                               /* registered in InitializeIntercom as CBR */

/** PTP **/
NSTUB(0xFF2745E0,  ptpPropSetUILock)
NSTUB(0xFF296B80,  ptp_register_handler)

/** Properties **/
NSTUB(0xFF129D54,  PROPAD_GetPropertyData)                  // check FF0CAAF0 later
NSTUB(0xFF128748, _prop_cleanup)
NSTUB(0xFF128100, _prop_request_change)
NSTUB(0xFF12867C,  prop_deliver)
NSTUB(0xFF127F10,  prop_register_slave)

/** Recursive locks **/
NSTUB(   0x374FC,  AcquireRecursiveLock)                    // AJ_KernelDry_KerRLock.c
NSTUB(    0xB5EC,  CreateRecursiveLock)
NSTUB(   0x37610,  ReleaseRecursiveLock)                    // AJ_KernelDry_KerRLock.c_p2

/** ResLock **/
NSTUB(0xFF2B6238,  CreateResLockEntry)
NSTUB(0xFF2B66F8,  LockEngineResources)
NSTUB(0xFF2B689C,  UnLockEngineResources)

/** Semaphores **/
NSTUB(    0x72F8,  create_named_semaphore)
NSTUB(    0x7544,  give_semaphore)
NSTUB(    0x7458,  take_semaphore)

/** Task info **/
NSTUB(    0x259C,  get_current_task)
NSTUB(0xFFAABFD8 - RAM_OFFSET,  get_obj_attr)               // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xFFAE833C - RAM_OFFSET,  get_task_info_by_id)            // AJ_task_trampoline_related_p10
NSTUB(   0x6669C,  task_max)
NSTUB(   0x652AC,  current_task)
NSTUB(     0x64C,  current_interrupt)                       // in interrupt handler (0x18), where MEM(C0201004) is stored
 
/** Task sleep **/
NSTUB(    0x7934,  msleep)

/** Tasks **/
NSTUB(    0x79E8,  task_create)
NSTUB(   0x652C0,  task_dispatch_hook)                      // task_trampoline -> last call -> last non-empty BL -> one indirect call here
NSTUB(    0xD070,  task_trampoline)

/** Misc **/
NSTUB(0xFFAD5048 - RAM_OFFSET,  vsnprintf)
NSTUB(0xFF13A87C,  LoadCalendarFromRTC)
NSTUB(0xFFAA4070,  GetTaskName)
