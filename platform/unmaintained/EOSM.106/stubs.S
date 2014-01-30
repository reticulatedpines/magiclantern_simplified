/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.6 firmware for the EOS M.
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

#define RAM_OFFSET 0xFFA68D58 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...

NSTUB(0xFFA87C1C - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFA70BB4 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFF4C35D8, CreateDialogBox)
NSTUB(0xFFA735B8 - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFF4C25A0, DeleteDialogBox)
NSTUB(0xFFA6CDB0 - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFF347460, FIO_FindClose) // proper name: FindClose
NSTUB(0xFF346468, FIO_CloseFile)
NSTUB(0xFF346398, FIO_SeekFile) //All take forever
//NSTUB(0xFF3462C8, FIO_SeekFile) //SeekSkipFile - Seems to work too.
NSTUB(0xFF346CC4, FIO_CreateDirectory)
NSTUB(0xFF3460C4, FIO_CreateFile)
NSTUB(0xFF34728C, FIO_FindFirstEx)
NSTUB(0xFF347380, FIO_FindNextEx)
NSTUB(0xFF3465E4, FIO_GetFileSize)
NSTUB(0xFF346008, FIO_Open)
NSTUB(0xFF346218, FIO_ReadFile)
NSTUB(0xFF346170, FIO_RemoveFile)
NSTUB(0xFF3463B8, FIO_WriteFile)
NSTUB(0xFF346AFC, FIO_RenameFile)
NSTUB(0xFFA70F50 - RAM_OFFSET, _FreeMemory)
NSTUB(0xFF0D783C, GUI_ChangeMode)
NSTUB(0xFF0D7680, GUI_Control)
NSTUB(0xFF1BD13C, GUI_GetFirmVersion)
NSTUB(0xFF1320E8, LoadCalendarFromRTC)
NSTUB(0xFF32D348, MuteOff_0)
NSTUB(0xFF32D300, MuteOn_0)
NSTUB(0xFF4E3D50, RedrawDisplay)
NSTUB(0xFFA87D30 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF1299C4, SetBitmapVramAddress)
NSTUB(0xFF1A7BD0, SetGUIRequestMode)


/*---------------*/
	NSTUB(0xFF345F6C, _audio_ic_read) // Good Candidate i2c read
	NSTUB(0xff136ea4, _audio_ic_write) // str:Reg_0x_02X_Data_04x
/*--------------*/


NSTUB(0xFFA8B200 - RAM_OFFSET, _alloc_dma_memory)
NSTUB(   0x6D6F4, bmp_vram_info )                 // look in SetBitmapVramAddress - as in 60D
NSTUB(0xFFAA0090 - RAM_OFFSET, bzero32)
NSTUB(0xFF137AD4, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF0C10E4, cli_save)
NSTUB(0xFFA6BC10 - RAM_OFFSET, create_init_task)
NSTUB(0xFFA6F634 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFF0C1C10, cstart)
NSTUB(0xFF4C135C, ctrlman_dispatch_event)
NSTUB(0xFF4C4314, dialog_redraw)
NSTUB(0xFF52F148, dialog_set_property_str)
NSTUB(   0x3F0DC, dm_names)
NSTUB(0xFFA6D11C - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFFA6D0B4 - RAM_OFFSET, dm_set_print_level)
NSTUB(0xFF0C83C0, _free) // see "can't malloc"
NSTUB(0xFFA8B234 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFF4F7D60, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xFFA6F880 - RAM_OFFSET, give_semaphore)
NSTUB(0xFF0D7D24, gui_init_end)
NSTUB(   0x3E5B4, gui_main_struct)
NSTUB(0xFF0D72AC, gui_main_task)
NSTUB(   0x591F0, gui_task_list)
NSTUB(0xFF0C5488, init_task)
NSTUB(0xFF0C8368, _malloc)
NSTUB(0xFFA87AD0 - RAM_OFFSET, msg_queue_post)
NSTUB(0xFFA87B10 - RAM_OFFSET, msg_queue_count)
NSTUB(0xFFA878E4 - RAM_OFFSET, msg_queue_receive)
NSTUB(0xFFA6FC70 - RAM_OFFSET, msleep)
NSTUB(0xFF12015C, _prop_cleanup)
NSTUB(0xFF120090, prop_deliver)
NSTUB(0xFF11F8E0, prop_register_slave)
NSTUB(0xFF11FAD4, _prop_request_change)
NSTUB(0xFF45F61C, ptp_register_handler)
NSTUB(0xFF3420F4, register_func)
NSTUB(0xFF6421C0, sdReadBlk)
NSTUB(   0x60B2C, sd_device)
NSTUB(0xFF0C10F8, sei_restore)
NSTUB(0xFFA6F794 - RAM_OFFSET, take_semaphore)
NSTUB(0xFFA6FD24 - RAM_OFFSET, task_create)
NSTUB(   0x3E1C0, task_dispatch_hook )
NSTUB(0xFFA7503C - RAM_OFFSET, task_trampoline)
NSTUB(0xFFA89258 - RAM_OFFSET, vsnprintf)
NSTUB(   0x8D330, vram_info)
NSTUB(0xFF6FB53C, LiveViewShutterApp_handler)

NSTUB(0xFF539DC4, LiveViewApp_handler)

NSTUB(0xFF3BBAFC, PlayMain_handler)
NSTUB(0xFF553994, PlayMovieGuideApp_handler)
NSTUB(0xFF6A2650, ErrCardForLVApp_handler)
NSTUB(0xFF520378, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF53786C, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

NSTUB(0xFF2C1524, _engio_write)
NSTUB(0xFF2C12A8, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF2C120C, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

NSTUB(0xFF541484, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

		//NSTUB(0xff987200 - RAM_OFFSET, set_fps_maybe)

NSTUB(0xFFA6B284 - RAM_OFFSET, get_current_task)

NSTUB(0x6BEF4, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFF1B4F70, Gui_SetSoundRecord) // Look for GUI_setsoundrec - Just changes prop

NSTUB(0xFFA70954 - RAM_OFFSET, GetMemoryInformation)
NSTUB(0xFF143C18, copy_malloc_info_maybe)

NSTUB(0xFFA733D0 - RAM_OFFSET, msg_queue_create)

NSTUB(0xFF101E04, PD_RemoteRelease)

	// for task information
NSTUB(0x3F5AC, task_max)
NSTUB(0xFFA96158 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFFA77B48 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xFF135034, PowerAudioOutput) //Look for actrlCpowerControl Case 1 Sub
NSTUB(0xFF10AA18, StartASIFDMADAC)  //~ Needs Patches Or
//NSTUB(0xFF2BF578, StartASIFDMADAC)  //~ Direct
//NSTUB(0xFF111D74, StartASIFDMADAC)  //~ SePlayStart
NSTUB(0xFF134C28 , PowerMicAmp)

NSTUB(0xFF10A644, StartASIFDMAADC)  //~ To Use ACTRL

//NSTUB(0xFF10AC98, StopASIFDMADAC)   //~ NormalStopAsif
//NSTUB(0xFF2BF754, StopASIFDMADAC)   //~ ACTRL Direct
//NSTUB(0xFF4743D0, StopASIFDMADAC)   //~ AsifStopCBR
//NSTUB(0xFF133CA0, StopASIFDMADAC)   //~ StopPlayWavData
//NSTUB(0xFF111EE4, StopASIFDMADAC)   //~ TouchSoundSePlayStop
//NSTUB(0xFF111D10, StopASIFDMADAC)   //~ SeTerminate
NSTUB(0xFF339938, StopASIFDMADAC)   //~ Call Play Stop

NSTUB(0xFF10A8F0, StopASIFDMAADC)   //~ warning: looks different
NSTUB(0xFF10B160, SetNextASIFADCBuffer) // Int16
//NSTUB(0xFF2BFAEC, SetNextASIFDACBuffer) // Int16 Direct Control
NSTUB(0xFF10B348, SetNextASIFDACBuffer) // Int16 Regular
NSTUB(0xFF134458, SetSamplingRate )
NSTUB(0xFF133070, SetAudioVolumeOut)

NSTUB(0xFF7FB3B8, audio_thresholds)
NSTUB(0x3E8CC, sounddev )
NSTUB(0xFF10CD10, sounddev_task )

NSTUB(0xFF10D118, SoundDevActiveIn)
NSTUB(0xFF10D144, SoundDevActiveOut)
NSTUB(0xFF10D370, SoundDevShutDownIn)



NSTUB(0xFF19991C, ENCODE_StartEncodeJpeg)

NSTUB(0xFF13B678, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF13B6C4, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF13B7DC, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xFFA723E4 - RAM_OFFSET, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xFFA71DBC - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)

NSTUB(0xFFA72B80 - RAM_OFFSET, GetNextMemoryChunk)


NSTUB(0xFFA7391C - RAM_OFFSET, dma_memcpy)

NSTUB(0xFF1B4454, GUI_SetRollingPitchingLevelStatus)


NSTUB(0xFF2C04E0, mvrSetPrintMovieLog)
NSTUB(0xFF2BFEF0, mvrSetQscale)
NSTUB(   0x4C584, mvr_config)
NSTUB(0xFF2C04BC, mvrFixQScale)
NSTUB(0xFF2BFFB4, mvrSetDefQScale)

NSTUB(0xFF1202B8, prop_get_value)
NSTUB(0xFF2A3378, ptpPropSetUILock)
NSTUB(0xFF334610, touch_cbr_canon)

/** corectly named canon property reading functions **/
NSTUB(0xFF1202B8, PROP_GetMulticastProperty )
NSTUB(0xFF121744, PROPAD_GetPropertyData )

NSTUB(0xFF5B6688, lvae_setdispgain)

NSTUB(0xFF0E5808, MVR_NotifyLvFrameTiming)


// Edmac
// Ram Offset:FFCC34E4
//M RAM_OFFSET 0xFFA68D58
NSTUB(0x92E0, CreateMemorySuite) //FFCCEF34
NSTUB(0x90D8, DeleteMemorySuite) // FFCCED2C

NSTUB(0x1C530, SetEDmac) //FFA85288
NSTUB(0x1C848, StartEDmac) //FFA855A0
NSTUB(0x1C9DC, PopEDmac) //From popedmacformemorysuite
NSTUB(0x1C994, AbortEDmac) //FFCF7BBC
NSTUB(0x1C5DC, ConnectWriteEDmac) //FFA85334
NSTUB(0x1C6A0, ConnectReadEDmac) //FFA853F8

/** register/unregister CBR names **/
NSTUB(0x1CA34, RegisterEDmacCompleteCBR) //FFCF7C5C
NSTUB(0x1CB1C, RegisterEDmacAbortCBR) //FFCF7CF0
NSTUB(0x1CBB0, RegisterEDmacPopCBR) //FFCF7D84
NSTUB(0x1CA70, UnregisterEDmacCompleteCBR)
NSTUB(0x1CB58, UnregisterEDmacAbortCBR) //FFCF7D2C
NSTUB(0x1CBEC , UnregisterEDmacPopCBR) //FFCF7DC0

NSTUB(0xFF5FE6E8, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5FEA70, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5FE9D4, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5FEB0C, PackMem_RegisterEDmacAbortCBRForMemorySuite)

NSTUB(0xFF5FE7FC, PackMem_StartEDmac)
NSTUB(0xFF5FE904, PackMem_PopEDmacForMemorySuite)

NSTUB(0xFF681104, GetCFnData)
NSTUB(0xFF68131C, SetCFnData)

NSTUB(0x22260, TryPostEvent)
NSTUB(0x222E4 , TryPostEvent_end)

NSTUB(0x21B80, TryPostStageEvent)
NSTUB(0x21C04, TryPostStageEvent_end)

//FIND
//NSTUB(0x35700, MREQ_ISR)
//NSTUB(0x35784, SIO3_ISR)
//NSTUB(0x8FE4, register_interrupt)
//Unused
//NSTUB(0xFFA71D5C - RAM_OFFSET, GetRemainOfMemoryChunk)
//NSTUB(0xFFA71D8C - RAM_OFFSET, GetSizeOfMemoryChunk)
//NSTUB(0xFFA720C4 - RAM_OFFSET, GetSizeOfMemorySuite)
//NSTUB(0xFFA721A4 - RAM_OFFSET, GetNumberOfChunks)
