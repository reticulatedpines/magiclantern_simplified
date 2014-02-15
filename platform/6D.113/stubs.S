/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.3 firmware for the 6D.
 *
 *  This is a straight copy from the 5d3, EOS M stubs need found/verified still.
 *
 * \todo Sort this file?  Generate it from the IDA map?
 */
/*
 * Copyright (C) 2012 Magic Lantern Team
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

#define RAM_OFFSET 0xFFCC34E4 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...


/** indented == need to fix **/

NSTUB(0xFFCFA04C - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFCCDAB0 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFF4B4BB8, CreateDialogBox)
NSTUB(0xFFCD04B4 - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFF4B3B54, DeleteDialogBox)
NSTUB(0xFFCC9CAC - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFF333988, FIO_FindClose) // proper name: FindClose
NSTUB(0xFF332990, FIO_CloseFile)
NSTUB(0xFF3328C0, FIO_SeekFile)
NSTUB(0xFF3331EC, FIO_CreateDirectory)
NSTUB(0xFF3325EC, FIO_CreateFile)
NSTUB(0xFF3337B4, FIO_FindFirstEx)
NSTUB(0xFF3338A8, FIO_FindNextEx)
NSTUB(0xFF332B0C, FIO_GetFileSize)
NSTUB(0xFF332530, FIO_Open)
NSTUB(0xFF332740, FIO_ReadFile)
NSTUB(0xFF332698, FIO_RemoveFile)
NSTUB(0xFF3328E0, FIO_WriteFile)
NSTUB(0xFF333024, FIO_RenameFile)
NSTUB(0xFFCCDE4C - RAM_OFFSET, _FreeMemory)
NSTUB(0xFF0DFB2C, GUI_ChangeMode)
NSTUB(0xFF0DF970, GUI_Control)
NSTUB(0xFF1A3614, GUI_GetFirmVersion)
NSTUB(0xFF140C00, LoadCalendarFromRTC)
NSTUB(0xFF31E2EC, MuteOff_0)
NSTUB(0xFF31E2A4, MuteOn_0)
NSTUB(0xFF4D836C, RedrawDisplay)
NSTUB(0xFFCFA160 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF13A12C, SetBitmapVramAddress)
NSTUB(0xFF18DFE8, SetGUIRequestMode)
NSTUB(0x752e0, sounddev )  // Maybe 0x752e0 or 0x753f4
NSTUB(0xFF11E24C, sounddev_task )
NSTUB(0xFF1422B0, _audio_ic_write) //SerialCommanSend(int *array)
NSTUB(0xFF1424F4, _audio_ic_read) // Correct
NSTUB(0xFF142820, TestSetAudioHeadPhone)
NSTUB(0xFF1422B0, SerialCommand_Send)
NSTUB(0xFF11EA28, SetAudioVolumeOut) //str:SetVolumeOut_Volume 1 Parameter
NSTUB(0xFF9D031C, audio_thresholds)
NSTUB(0xFF143C40 , SelectMic)


NSTUB(0xFFCFD630 - RAM_OFFSET, _alloc_dma_memory)
NSTUB(   0x9BC78, bmp_vram_info )                 // look in SetBitmapVramAddress - as in 60D
NSTUB(0xFFD2F2F8 - RAM_OFFSET, bzero32)
NSTUB(0xFF146844, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF78C3A0, cfReadBlk)
NSTUB(0xFF0C1088, cli_save)
NSTUB(0xFFCC664C - RAM_OFFSET, create_init_task)
NSTUB(0xFFCCC530 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFF0C1BA8, cstart)
NSTUB(0xFF4B2864, ctrlman_dispatch_event)
NSTUB(0xFF4B4E74, dialog_redraw)
NSTUB(0xFF51DA44, dialog_set_property_str)
NSTUB(   0x7599C, dm_names)
NSTUB(0xFFCCA018 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFFCC9FB0 - RAM_OFFSET, dm_set_print_level)
NSTUB(0xFF0C8DA8, _free) // see "can't malloc"
NSTUB(0xFFCFD664 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF654C44, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xFFCCC77C - RAM_OFFSET, give_semaphore)
NSTUB(0xFF0DFFE4, gui_init_end)
NSTUB(   0x74F10, gui_main_struct)
NSTUB(0xFF0DF59C, gui_main_task)
NSTUB(   0x8295C, gui_task_list)
NSTUB(0xFF0C5438, init_task)
NSTUB(0xFF0C8D50, _malloc)
NSTUB(0xFFCF9F00 - RAM_OFFSET, msg_queue_post)
NSTUB(0xFFCF9D14 - RAM_OFFSET, msg_queue_receive)
NSTUB(0xFFCF9F40 - RAM_OFFSET, msg_queue_count) // 0x36A5C Above "!!!!!!! QUE Overflo"
NSTUB(0xFFCCCB6C - RAM_OFFSET, msleep)
NSTUB(0xFF1303BC, _prop_cleanup)
NSTUB(0xFF1302F0, prop_deliver)
NSTUB(0xFF1304DC, prop_get_value) // unsafe?
NSTUB(0xFF12FB84, prop_register_slave)
NSTUB(0xFF12FD74, _prop_request_change)
NSTUB(0xFF292A24, ptp_register_handler)
NSTUB(0xFF14AD50, register_func)
NSTUB(0xFF792860, sdReadBlk)
NSTUB(   0x8D678, sd_device)
NSTUB(0xFF0C109C, sei_restore)
NSTUB(0xFFCCC690 - RAM_OFFSET, take_semaphore)
NSTUB(0xFFCCCC20 - RAM_OFFSET, task_create)
NSTUB(   0x74ACC, task_dispatch_hook )
NSTUB(0xFFCD1F44 - RAM_OFFSET, task_trampoline)
NSTUB(0xFFCFB688 - RAM_OFFSET, vsnprintf)
NSTUB(   0xB99DC, vram_info)
NSTUB(0xFF713774, LiveViewShutterApp_handler)

//~ Doesn't take over like wifi app. level dialog non lv - FF71666C
//NSTUB(0xFF6BE3DC, LiveViewLevelApp_handler ) //str:DlgLiveViewLevele.c_TERMINATE_WINSYS_DELETE_DI
//NSTUB(0xFF6BE3DC, LiveViewLevelApp_handler ) //str:DlgLiveViewLevele.c_TERMINATE_WINSYS_DELETE_DI
NSTUB(0xFF71666C, DeadViewLevelApp_handler ) //
NSTUB(0xFF71DB88, LiveViewWifiApp_handler); //str:CONNECT_VIDEO_HDMI_TERMINATE_WINSYS_DELETE_DIA_v3)

NSTUB(0xFF52C810, LiveViewApp_handler)
NSTUB(0xFF52C3DC, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)

NSTUB(0xFF3A1970, PlayMain_handler)
NSTUB(0xFF558BC0, PlayMovieGuideApp_handler)
NSTUB(0xFF55D170, PlayQMenu_handler)
NSTUB(0xFF6BEBB8, ErrCardForLVApp_handler) // not sure
NSTUB(0xFF50C528, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF52A27C, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

NSTUB(0xFF2AE148, _engio_write)
NSTUB(0xFF2ADECC, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF2ADE30, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xFF539BA8, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
NSTUB(0xFF52A83C, SetDisplayDlgLV)


NSTUB(0xFFCC5A80 - RAM_OFFSET, get_current_task)

NSTUB(0x9A478, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFF19AB58, GuiSetAutoBrightness)

NSTUB(0xFF19B448, Gui_SetSoundRecord)

NSTUB(0xFFCCD850 - RAM_OFFSET, GetMemoryInformation)
NSTUB(0xFF1529B4, copy_malloc_info_maybe)

NSTUB(0xFFCD02CC - RAM_OFFSET, msg_queue_create)

NSTUB(0xFF10ACAC, PD_RemoteRelease) //~ string: "COM_RemoteRelease:%lx, %lx"

	// for task information
NSTUB(0x75F08, task_max)
NSTUB(0xFFD0EFA0 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFFCD4DD8 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xFF2ABD58, asifAIntDmaDac)
NSTUB(0xFF2AC318, asifAStopDmaDac)
NSTUB(0xFF2AC3CC, asifAStopDmaAdc)
NSTUB(0xFF2AC67C, asifAFuncIgnore_Give_FuncSync)
NSTUB(0xFF2AC6BC, asifAIgnore_ASIF)


NSTUB(0xFF142D68, PowerAudioOutput) //sub_FF142D54 Same Actions
NSTUB(0xFF11C9A4, StartASIFDMADAC)  //~ warning: looks different
NSTUB(0xFF11C5D0, StartASIFDMAADC)  //~ warning: looks different
NSTUB(0xFF11CC24, StopASIFDMADAC)   //~ warning: looks different
NSTUB(0xFF11C87C, StopASIFDMAADC)   //~ warning: looks different
NSTUB(0xFF11D0EC, SetNextASIFADCBuffer)
//NSTUB(0xFF2AC6FC, SetNextASIFDACBuffer) //From ACTRL
NSTUB(0xFF11D2D4, SetNextASIFDACBuffer) // From Event Generating Functions
NSTUB(0xFF143D0C, PowerMicAmp)

NSTUB(0xFF143638, SetFilterRec)
NSTUB(0xFF142A74, UnpowerSpeakerForWav)
NSTUB(0xFF142DE4, SetSamplingRate) //AudioIC.c_SetRate_hHPTimer Identical
NSTUB(0xFF142B70, SetAudioVolumeOut) //Keep Volume (%d->%d)
NSTUB(0xFF11E9F4, SetAudioVolumeIn) // SetVolumeIn(alc, L, R)
NSTUB(0xFF11E654, SoundDevActiveIn)
NSTUB(0xFF11E680, SoundDevActiveOut)
NSTUB(0xFF11C87C, SoundDevShutDownIn)
NSTUB(0xFF11E6E0, SoundDevStartIn)
NSTUB(0xFF11E740, SoundDevStopIn)


NSTUB(0xFFCE3188 - RAM_OFFSET, ENCODE_StartEncodeJpeg)  //~ called from RAM in 6D, wasn't in EOS M.
NSTUB(0xFFCD0818 - RAM_OFFSET, dma_memcpy) // Slower?


/** ExMem **/
NSTUB(0xFF14A424, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF14A470, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF0F2EC4, AllocateMemoryResourceForSingleChunk)
NSTUB(0xFF14A588, FreeMemoryResource) // m_pfFreeMemoryCBR

NSTUB(0xFFCCECB8 - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)
NSTUB(0xFFCCF2E0 - RAM_OFFSET, GetFirstChunkFromSuite)
NSTUB(0xFFCCFA7C - RAM_OFFSET, GetNextMemoryChunk)
NSTUB(0x0A318, GetSizeOfMaxRegion)

/* Electronic level - roll/pitch info */
NSTUB(0xFF270968, GUI_SetRollingPitchingLevelStatus); // RequestRollingPitching - no side effects :)
NSTUB(0xFF2AD104, mvrSetPrintMovieLog)
NSTUB(   0x7AE8C, mvr_config)

NSTUB(0xFF1F2550, IvaParamMode)

NSTUB(0xFFCCEF34 - RAM_OFFSET, CreateMemorySuite)
NSTUB(0xFFCCED2C - RAM_OFFSET, DeleteMemorySuite)

/** experimental EDMAC research **/
/** EDMAC routines */
NSTUB(0xFFCF74EC - RAM_OFFSET, SetEDmac)
NSTUB(0xFFCF7898 - RAM_OFFSET, StartEDmac)
NSTUB(0xFFCF7C04 - RAM_OFFSET, PopEDmac)
NSTUB(0xFFCF7BBC - RAM_OFFSET, AbortEDmac)
NSTUB(0xFFCF764C - RAM_OFFSET, ConnectWriteEDmac)
NSTUB(0xFFCF7710 - RAM_OFFSET, ConnectReadEDmac)

/** register/unregister CBR names **/
NSTUB(0xFFCF7C5C - RAM_OFFSET, RegisterEDmacCompleteCBR)
NSTUB(0xFFCF7CF0 - RAM_OFFSET, RegisterEDmacAbortCBR)
NSTUB(0xFFCF7D84 - RAM_OFFSET, RegisterEDmacPopCBR)
NSTUB(0xFFCF7C98 - RAM_OFFSET, UnregisterEDmacCompleteCBR)
NSTUB(0xFFCF7D2C - RAM_OFFSET, UnregisterEDmacAbortCBR)
NSTUB(0xFFCF7DC0 - RAM_OFFSET, UnregisterEDmacPopCBR)

NSTUB(0xFF612A68, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF612B04, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF61277C, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF612998, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF612890, PackMem_StartEDmac)
NSTUB(0xFF612BA0, PackMem_RegisterEDmacAbortCBRForMemorySuite)


/* TP LOG */
NSTUB(0xFFCFD3E8 - RAM_OFFSET, TryPostEvent)
NSTUB(0xFFCFD46C - RAM_OFFSET, TryPostEvent_end)

NSTUB(0xFFCFCD08 - RAM_OFFSET, TryPostStageEvent)
NSTUB(0xFFCFCD8C - RAM_OFFSET, TryPostStageEvent_end)

NSTUB(0xFFCF8BE4 - RAM_OFFSET, MREQ_ISR)
NSTUB(0xFFCF8C68 - RAM_OFFSET, SIO3_ISR)
NSTUB(0xFFCCC4C8 - RAM_OFFSET, register_interrupt)
/* ISR */

NSTUB(0x74AD0, pre_isr_hook)
NSTUB(0x74AD4, post_isr_hook)


/* ResLock */
NSTUB(0xFF2B20A4, CreateResLockEntry)
NSTUB(0xFF2B2564, LockEngineResources)
NSTUB(0xFF2B2708, UnLockEngineResources)

/** corectly named canon property reading functions **/
NSTUB(0xFF1304DC, PROP_GetMulticastProperty)
NSTUB(0xFF1319CC, PROPAD_GetPropertyData) //str:SearchFromProperty

/** CFN **/
NSTUB(0xFF69CD64, GetCFnData)
NSTUB(0xFF69CE4C, SetCFnData)
/** Unused **/
//NSTUB(0xFFCCEC88 - RAM_OFFSET, GetSizeOfMemoryChunk)
//NSTUB(0xFFCCEC58 - RAM_OFFSET, GetRemainOfMemoryChunk)
//NSTUB(0xFFCCF0A0 - RAM_OFFSET, GetNumberOfChunks)
//NSTUB(0xFFCCEFC0 - RAM_OFFSET, GetSizeOfMemorySuite)
