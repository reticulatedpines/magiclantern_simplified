/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.2 firmware for the 6D.
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

.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )

#define RAM_OFFSET 0xFFCC34D4 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...


/** indented == need to fix **/

NSTUB(0xFFCFA03C - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFCCDAA0 - RAM_OFFSET, AllocateMemory)
NSTUB(0xFF4B4BA4, CreateDialogBox)
NSTUB(0xFFCD04A4 - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFF4B3B40, DeleteDialogBox)
NSTUB(0xFFCC9C9C - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFF333974, FIO_CleanupAfterFindNext_maybe) // proper name: FindClose
NSTUB(0xFF33297C, FIO_CloseFile)
NSTUB(0xFF3331D8, FIO_CreateDirectory)
NSTUB(0xFF3325D8, FIO_CreateFile)
NSTUB(0xFF3337A0, FIO_FindFirstEx)
NSTUB(0xFF333894, FIO_FindNextEx)
NSTUB(0xFF332AF8, FIO_GetFileSize)
NSTUB(0xFF33251C, FIO_Open)
NSTUB(0xFF33272C, FIO_ReadFile)
NSTUB(0xFF332684, FIO_RemoveFile)
NSTUB(0xFF3328CC, FIO_WriteFile)
NSTUB(0xFFCCDE3C - RAM_OFFSET, FreeMemory)
NSTUB(0xFF0DFB2C, GUI_ChangeMode)
NSTUB(0xFF0DF970, GUI_Control)
NSTUB(0xFF1A3600, GUI_GetFirmVersion)
NSTUB(0xFF140B9C, LoadCalendarFromRTC)
NSTUB(0xFF31E2D8, MuteOff_0)
NSTUB(0xFF31E290, MuteOn_0)
NSTUB(0xFF4D8358, RedrawDisplay)
NSTUB(0xFFCFA150 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF13A12C, SetBitmapVramAddress)
NSTUB(0xFF18DFD4, SetGUIRequestMode)
NSTUB(0x752e0, sounddev )  // Maybe 0x752e0 or 0x753f4
NSTUB(0xFF11E24C, sounddev_task )
NSTUB(0xFF143104, _audio_ic_write)
NSTUB(0xFF1424E0, _audio_ic_read)
NSTUB(0xFF14280C, TestSetAudioHeadPhone)
NSTUB(0xFF14229C, SerialCommand_Send)
//NSTUB(0xFF1430BC, SetAudioVolumeOut) // From initialization function.
NSTUB(0xFF11EA28, SetAudioVolumeOut) //str:SetVolumeOut_Volume 1 Parameter
NSTUB(0xFF9D0308, audio_thresholds)
//NSTUB(   0x77DC8, additional_version)


NSTUB(0xFFCFD620 - RAM_OFFSET, alloc_dma_memory)
NSTUB(   0x9BC78, bmp_vram_info )                 // look in SetBitmapVramAddress - as in 60D
NSTUB(0xFFD2F2E8 - RAM_OFFSET, bzero32)
NSTUB(0xFF146830, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF78C38C, cfReadBlk)
	//	    NSTUB(   0x20890, cf_device) idk...
NSTUB(0xFF0C1088, cli_save)
NSTUB(0xFFCC663C - RAM_OFFSET, create_init_task)
NSTUB(0xFFCCC520 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFF0C1BA8, cstart)
NSTUB(0xFF4B2850, ctrlman_dispatch_event)
NSTUB(0xFF4B4E60, dialog_redraw)
NSTUB(0xFF51DA30, dialog_set_property_str)
NSTUB(   0x7599C, dm_names)
NSTUB(0xFFCCA008 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFFCC9FA0 - RAM_OFFSET, dm_set_print_level)
NSTUB(0xFF0C8DA8, free) // see "can't malloc"
NSTUB(0xFFCFD654 - RAM_OFFSET, free_dma_memory)
NSTUB(0xFF654C30, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xFFCCC76C - RAM_OFFSET, give_semaphore)
NSTUB(0xFF0DFFE4, gui_init_end)
NSTUB(   0x74F10, gui_main_struct)
NSTUB(0xFF0DF59C, gui_main_task)
NSTUB(   0x8295C, gui_task_list)
NSTUB(0xFF0C5438, init_task)
NSTUB(0xFF0C8D50, malloc)
NSTUB(0xFFCF9EF0 - RAM_OFFSET, msg_queue_post)
NSTUB(0xFFCF9D04 - RAM_OFFSET, msg_queue_receive)
NSTUB(0xFFCCCB5C - RAM_OFFSET, msleep)
NSTUB(0xFF1303BC, _prop_cleanup)
NSTUB(0xFF1302F0, prop_deliver)
NSTUB(0xFF1304DC, prop_get_value) // unsafe?
NSTUB(0xFF12FB84, prop_register_slave)
NSTUB(0xFF12FD74, _prop_request_change)
NSTUB(0xFF292A10, ptp_register_handler)
NSTUB(0xFF14AD3C, register_func)
NSTUB(0xFF79284C, sdReadBlk)
NSTUB(   0x8D678, sd_device)
NSTUB(0xFF0C109C, sei_restore)
NSTUB(0xFFCCC680 - RAM_OFFSET, take_semaphore)
NSTUB(0xFFCCCC10 - RAM_OFFSET, task_create)
NSTUB(   0x74ACC, task_dispatch_hook )
NSTUB(0xFFCD1F34 - RAM_OFFSET, task_trampoline)
NSTUB(0xFFCFB678 - RAM_OFFSET, vsnprintf)
NSTUB(   0xB99DC, vram_info)
NSTUB(0xFF713760, LiveViewShutterApp_handler)

//~ Doesn't take over like wifi app. level dialog non lv - ff716658
//NSTUB(0xFF6BE3DC, LiveViewLevelApp_handler ) //str:DlgLiveViewLevele.c_TERMINATE_WINSYS_DELETE_DI
//NSTUB(0xFF6BE3DC, LiveViewLevelApp_handler ) //str:DlgLiveViewLevele.c_TERMINATE_WINSYS_DELETE_DI
NSTUB(0xFF716658, DeadViewLevelApp_handler ) //
NSTUB(0xFF71DB74, LiveViewWifiApp_handler); //str:CONNECT_VIDEO_HDMI_TERMINATE_WINSYS_DELETE_DIA_v3)

NSTUB(0xFF52C7FC, LiveViewApp_handler)
NSTUB(0xFF52C3C8, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
//    NSTUB(0xff4b1cc4, LiveViewApp_handler_end) //ends at FF52ED60 next NSTUB(0xFF52ED64, str:StartDummyDialog_CreateDummyDialogBox)

NSTUB(0xFF3A195C, PlayMain_handler)
NSTUB(0xFF558BAC, PlayMovieGuideApp_handler)
NSTUB(0xFF6BEBA4, ErrCardForLVApp_handler) // not sure
NSTUB(0xFF50C514, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF52A268, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

NSTUB(0xFF2AE134, _engio_write)
NSTUB(0xFF2ADEB8, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF2ADE1C, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

		//    NSTUB(0xff9b1be0 - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
NSTUB(0xFF539B94, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

		//    NSTUB(0xff987200 - RAM_OFFSET, set_fps_maybe)

NSTUB(0xFFCC5A70 - RAM_OFFSET, get_current_task)

NSTUB(0x9A478, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

NSTUB(0xFF19AB44, GuiSetAutoBrightness)

NSTUB(0xFF19B434, Gui_SetSoundRecord)

NSTUB(0xFFCCD840 - RAM_OFFSET, GetMemoryInformation)
NSTUB(0xFF1529A0, copy_malloc_info_maybe)

NSTUB(0xFFCD02BC - RAM_OFFSET, msg_queue_create)

NSTUB(0xFF10ACAC, PD_RemoteRelease) //~ string: "COM_RemoteRelease:%lx, %lx"

	// for task information
NSTUB(0x75F08, task_max)
NSTUB(0xFFD0EF90 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFFCD4DC8 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xFF2ABD44, asifAIntDmaDac)
NSTUB(0xFF2AC304, asifAStopDmaDac)
NSTUB(0xFF2AC3B8, asifAStopDmaAdc)
NSTUB(0xFF2AC668, asifAFuncIgnore_Give_FuncSync)
NSTUB(0xFF2AC6A8, asifAIgnore_ASIF)

//NSTUB(0xFF2AA934, PowerAudioOutput)
NSTUB(0xFF142D54, PowerAudioOutput) //sub_FF142D54 Same Actions
NSTUB(0xFF11C9A4, StartASIFDMADAC)  //~ warning: looks different
NSTUB(0xFF11C5D0, StartASIFDMAADC)  //~ warning: looks different
NSTUB(0xFF11CC24, StopASIFDMADAC)   //~ warning: looks different
NSTUB(0xFF11D068, SetNextASIFADCBuffer)
NSTUB(0xFF11D250, SetNextASIFDACBuffer)
NSTUB(0xFF142DD0, SetSamplingRate) //AudioIC.c_SetRate_hHPTimer Identical
//NSTUB(0xFF2AB888, SetAudioVolumeOut) In Play Functions with PowerAudioOutput
NSTUB(0xFF11E654, sounddev_active_in)
NSTUB(0xFF11E680, sounddev_active_out)


NSTUB(0xFFCE3178 - RAM_OFFSET, ENCODE_StartEncodeJpeg)  //~ called from RAM in 6D, wasn't in EOS M.

NSTUB(0xFF14A410, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF14A45C, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF14A574, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xFFCCF2D0 - RAM_OFFSET, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xFFCCECA8 - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)

//NSTUB(0xFFCD0808 - RAM_OFFSET, AJ_HP_Copy_n_DMA_channel_n_BLTDMA)

NSTUB(0xFFCD0808 - RAM_OFFSET, dma_memcpy) // Slower?

//NSTUB(0xFFCD0998 - RAM_OFFSET, dma_memcpy) // V1: FFCD0998 V2:FFCD0ABC Made Weird Pics


NSTUB(0xFF19A918, GUI_SetRollingPitchingLevelStatus)

//NSTUB(0xFFCD0ABC, dma_memcpy) // V1: FFCD0998 V2:FFCD0ABC
NSTUB(0xFF24A044 - RAM_OFFSET, GetJpegBufForLV)
NSTUB(0xFF24A050 - RAM_OFFSET, GetJpegSizeForLV)


NSTUB(0xFF2AD0F0, mvrSetPrintMovieLog)
NSTUB(0xFF2ACB00, mvrSetQscale)
NSTUB(   0x7AE8C, mvr_config)
NSTUB(0xFF2AD0CC, mvrFixQScale)
NSTUB(0xFF2ACBC4, mvrSetDefQScale)

