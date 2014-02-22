/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 2.0.3 firmware for the 7D.
 *
 * \todo Sort this file?  Generate it from the IDA map?
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

NSTUB( ROMBASEADDR, firmware_entry )

NSTUB(0x193C, pre_isr_hook)
NSTUB(0x1940, post_isr_hook)

NSTUB(0x67C, next_task_ctx)
NSTUB(0x680, current_task_ctx)

/** EDMAC routines **/
NSTUB(0xFF1F5360, SetEDmac)
NSTUB(0xFF1F5660, StartEDmac)
NSTUB(0xFF1F5864, PopEDmac)
//NSTUB([idk], AbortEDmac)
NSTUB(0xFF1F5418, ConnectWriteEDmac)
NSTUB(0xFF1F54DC, ConnectReadEDmac)

/** register/unregister CBR names **/
NSTUB(0xFF1F5850, RegisterEDmacCompleteCBR)
NSTUB(0xFF1F5878, RegisterEDmacAbortCBR)
NSTUB(0xFF1F5980, RegisterEDmacPopCBR)
NSTUB(0xFF1F5864, UnregisterEDmacCompleteCBR)
NSTUB(0xFF1F58B8, UnregisterEDmacAbortCBR)
NSTUB(0xFF1F5994, UnregisterEDmacPopCBR)

NSTUB(0xFF3D26EC, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF3D27CC, PackMem_RegisterEDmacAbortCBRForMemorySuite)
NSTUB(0xFF3D275C, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF3D2644, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF3D24A0, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF3D2598, PackMem_StartEDmac)

NSTUB(0xFF07AAFC, MasterExecute)

NSTUB(0xFF07AED4, RegisterRPCHandler)
NSTUB(0xFF07AF84, RequestRPC)
NSTUB(0xFF07A954, QuickOutIPCTransfer)
NSTUB(0xFF07A704, BulkOutIPCTransfer)
NSTUB(0xFF07A7D4, BulkInIPCTransfer)

NSTUB(0xFF07E4A0, GetFirstChunkFromSuite)
NSTUB(0xFF07EC38, GetNextMemoryChunk)
NSTUB(0xFF07DE88, GetRemainOfMemoryChunk)
NSTUB(0xFF07DEE8, GetMemoryAddressOfMemoryChunk)

NSTUB(0xFF06494C, SoundDevStopIn)
NSTUB(0xFF064A1C, SoundDevStopOut)

NSTUB(0xFF064304, SoundDevShutDownIn)
NSTUB(0xFF064034, SoundDevLock)
NSTUB(0xFF0650BC, SetWindCutMode)
NSTUB(0xFF064D80, SamplingParamInRate)
NSTUB(0xFF064FF8, SetVolumeInVolume)
NSTUB(0xFF0640EC, SoundDevActiveIn)

NSTUB(0xFF202814, LightMeasure_n_Callback_r0)


NSTUB(0xFF3759F0, GUI_GetCFnForTab4)
NSTUB(0xFF375998, GUI_SetCFnForTab4)
NSTUB(0xFF11A8E0, GetJpegBufForLV)
NSTUB(0xFF11A8EC, GetJpegSizeForLV)

NSTUB(0xFF0705E4, set_brightness_registers)
NSTUB(0xFF205FA8, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xff07cd24, _AllocateMemory)
NSTUB(0xff2ffd3c, CreateDialogBox)
NSTUB(0xff07fe0c, CreateRecursiveLock)
NSTUB(0xff2ff7c4, DeleteDialogBox)
NSTUB(0xff0776ac, DryosDebugMsg)
NSTUB(0xff1fd554, FIO_FindClose) // AJ__switch_0x1A50_n_calls_fstOpenDir
NSTUB(0xff1fc684, FIO_CloseFile)
NSTUB(0xff1fccf4, _FIO_CreateDirectory) // not sure. g3gg0: confirmed by caller 'ctgDirCreate'
NSTUB(0xff1fc2e0, _FIO_CreateFile)
NSTUB(0xff1fd2b8, _FIO_FindFirstEx)
NSTUB(0xff1fd42c, FIO_FindNextEx)
NSTUB(0xff1fc800, _FIO_GetFileSize)
NSTUB(0xff1fc224, _FIO_Open)
NSTUB(0xff1fc434, FIO_ReadFile)
NSTUB(0xff1fc38c, _FIO_RemoveFile)
NSTUB(0xff1fc5d4, FIO_WriteFile)
NSTUB(0xFF1FC5B4, FIO_SeekFile)
NSTUB(0xff07d0b4, _FreeMemory)
NSTUB(0xff022004, GUI_ChangeMode)
NSTUB(0xff021f04, GUI_Control)
NSTUB(0xff23f414, GUI_GetFirmVersion)
NSTUB(0xff4c621c, GetCFnData)
NSTUB(0xff242f38, GuiEdLedBlink)
NSTUB(0xff242f14, GuiEdLedOff)
NSTUB(0xff242ef0, GuiEdLedOn)
NSTUB(0xff075210, LoadCalendarFromRTC)
NSTUB(0xFF072290, MuteOff_0) // not sure
NSTUB(0xff06d6a8, MuteOn_0)
NSTUB(0xff317ee8, RedrawDisplay)
NSTUB(0xff2060bc, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xff15fd74, RemoteRelease)
NSTUB(0xff06f2fc, SetBitmapVramAddress)
NSTUB(0xff4c6424, SetCFnData)
NSTUB(0xff0a8738, SetGUIRequestMode)
NSTUB(0xff072f7c, _audio_ic_read)
NSTUB(0xff0730c8, _audio_ic_write)
NSTUB(0xFF57B014, audio_thresholds)
NSTUB(   0x16fb8, additional_version)
NSTUB(0xff20da50, _alloc_dma_memory)
NSTUB(   0x29a60, bmp_vram_info )
NSTUB(0xff50cdec, bzero32)
NSTUB(0xff07842c, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xff44ebb0, cfReadBlk)                      // might be good (dumps=1, score=6.1)  g3gg0: confirmed by debugmsg 'cfReadBlk'
NSTUB(   0x22938, cf_device)
NSTUB(0xff0104bc, cli_save)
NSTUB(0xff01559c, create_init_task)
NSTUB(0xff07b7c4, create_named_semaphore)
NSTUB(0xff010fb0, cstart)
NSTUB(0xff2fe9f4, ctrlman_dispatch_event)
NSTUB(0xff300640, dialog_redraw)
NSTUB(0xff361bdc, dialog_set_property_str)
NSTUB(    0x2b18, dm_names)
NSTUB(0xff077990, dm_set_store_level)
NSTUB(0xff01634c, _free)
NSTUB(0xff20da84, _free_dma_memory)
NSTUB(0xff3d7798, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xff07ba0c, give_semaphore)
NSTUB(0xff0ab97c, gui_change_lcd_state_post)
NSTUB(0xff0aabc4, gui_change_mode)
NSTUB(0xff0ab8e4, gui_change_shoot_type_post)
NSTUB(0xff022680, gui_init_end)
NSTUB(0xFF0AC000, gui_init_event)
NSTUB(0xff0ab1c0, gui_local_post)
NSTUB(    0x1bf4, gui_main_struct)
NSTUB(0xff021b44, gui_main_task)
NSTUB(0xff0a9384, gui_massive_event_loop)
NSTUB(0xff0ab5dc, gui_other_post)
NSTUB(0xff0ab858, gui_post_100000a1)
NSTUB(0xff2feb44, gui_task_create)
NSTUB(0xff2fec64, gui_task_destroy)
NSTUB(   0x1A904, gui_task_list)
NSTUB(0xff07b708, gui_timer_something)
NSTUB(    0x3624, gui_timer_struct)
NSTUB(0xff018734, init_task)
NSTUB(0xff0162f0, _malloc)
NSTUB(0xff205e5c, msg_queue_post)
NSTUB(0xff205c70, msg_queue_receive)
NSTUB(0xff205e9c, msg_queue_count)
NSTUB(0xff07bdf8, msleep)
NSTUB(0xff1e0e04, mvrFixQScale)
NSTUB(0xff1e0870, mvrSetDeblockingFilter)
NSTUB(0xff1e08e0, mvrSetDefQScale)
NSTUB(0xff1e0e24, mvrSetPrintMovieLog)
NSTUB(    0xA39C, mvr_config)
NSTUB(0xFF080224, dma_memcpy)
NSTUB(0xff078e6c, oneshot_timer)
NSTUB(0xff068734, _prop_cleanup)
NSTUB(0xff06858c, prop_deliver)
NSTUB(0xff0687c8, prop_get_value)

/** corectly named canon property reading functions **/
NSTUB(0xFF0687C8, PROP_GetMulticastProperty)
NSTUB(0xFF06934C, PROPAD_GetPropertyData)

NSTUB(0xff068614, prop_register_slave)
NSTUB(0xff0686a4, prop_unregister_slave)
NSTUB(0xff067fac, _prop_request_change)
NSTUB(0xff2885cc, ptp_register_handler)
NSTUB(0xff20ae48, register_func)
NSTUB(0xFF454540, sdReadBlk)            // g3gg0: confirmed by DebugMsg - but makes no sense on 7D
    //NSTUB(   0x208D8, sd_device)
NSTUB(0xff0104d0, sei_restore)
NSTUB(    0x1f54, sounddev)
NSTUB(0xFF063C70, sounddev_task)
NSTUB(0xFF0640EC, sounddev_active_in)
NSTUB(0xff07b924, take_semaphore)
NSTUB(0xff07beac, task_create)
NSTUB(    0x1934, task_dispatch_hook )
NSTUB(0xff084ca4, task_trampoline)
NSTUB(0xff2cb1e0, vram_get_number)
NSTUB(   0x7e9e8, vram_info)
NSTUB(0xff209670, vsnprintf)
NSTUB(0xFF347918, LiveViewApp_handler)
NSTUB(0xFF3552DC, PlayMain_handler)
NSTUB(0xFF42B700, PlayMovieGuideApp_handler)
NSTUB(0xFF35AC90, OlcAFFrameApp_handler)
NSTUB(0xff430250, ErrCardForLVApp_handler)
NSTUB(0xFF3674A4, LiveViewWbApp_handler)
NSTUB(0xFF236E9C, GUI_SetRollingPitchingLevelStatus)
NSTUB(0xff336d60, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

NSTUB(0xff1f6b20, _engio_write)
NSTUB(0xff1f664c, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xff1f675c, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xFF35DF58, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

NSTUB(0x29A9C, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xff07cac8, GetMemoryInformation)

NSTUB(0xff07fc28, msg_queue_create)

NSTUB(0xFF03F4A8, PD_RemoteRelease)
NSTUB( 0xff16004c, PtpDps_remote_release_SW1_SW2_worker ) // called from: ptpRemoteRelease Occupy

    // for task information
NSTUB(0x2B24, task_max)
NSTUB(0xFF087940, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xff08779c, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos
NSTUB(0xff014c10, get_current_task)

NSTUB(0xff080e78, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff080ec4, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff080fdc, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xFF07CA74, GetSizeOfMaxRegion)

NSTUB(0xff07365c, PowerAudioOutput)
NSTUB(0xff061c44, StartASIFDMADAC)
NSTUB(0xff061d20, StopASIFDMADAC)
NSTUB(0xFF061A88, StartASIFDMAADC)
NSTUB(0xFF061C08, StopASIFDMAADC) // Called by SoundDevStopIn

NSTUB(0xFF073F54, SetFilterRec)

NSTUB(0xFF0621C4, SetNextASIFADCBuffer) // called by SetNextUINT8ASIFADCBuffer and SetNextINT16ASIFADCBuffer
NSTUB(0xFF06227C, SetNextASIFDACBuffer)
NSTUB(0xff0736f4, SetSamplingRate )
NSTUB(0xFF073944, SetAudioVolumeOut)
NSTUB(0xFF074410, PowerMicAmp)
NSTUB(0xFF07443C, UnpowerMicAmp)


NSTUB(0xFF06EDD0, AsyncEnableImagePhysicalScreenParameter)
NSTUB(0xff06e8b0, EnableImagePhysicalScreenParameter)

NSTUB(0xFF41E17C, StartPlayProtectGuideApp)
NSTUB(0xff41dde0, StopPlayProtectGuideApp)

/* ResLock */
NSTUB(0xFF1F9694, CreateResLockEntry)
NSTUB(0xFF1F9BDC, LockEngineResources)
NSTUB(0xFF1F9D18, UnLockEngineResources)
