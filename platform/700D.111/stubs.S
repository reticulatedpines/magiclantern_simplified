/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the 700D.
 *
 *  This is a straight copy from the 5d3, EOS M, 650D stubs need found/verified still.
 *  Indented line = incorrect.
 *  Initial stubs identified by Nanomad, most of the other stubs were matched by hand 
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
	.extern name; \
	name = addr

#define XSTUB(addr,name) \
	.global STUB_##name; \
	.extern STUB_##name; \
	STUB_##name = addr

NSTUB( ROMBASEADDR, firmware_entry )

/** experimental EDMAC research **/
NSTUB(0x8B7C, CreateMemorySuite)
NSTUB(0x8974, DeleteMemorySuite)

/** EDMAC routines */
NSTUB(0x11480, SetEDmac)
NSTUB(0x11798, StartEDmac)
NSTUB(0x1192C, PopEDmac) //Xref: PackMem_PopEDmacForMemorySuite
NSTUB(0x118E4, AbortEDmac)
NSTUB(0x1152C, ConnectWriteEDmac)
NSTUB(0x115F0, ConnectReadEDmac)


/** keep the old name until all are refactored **/
NSTUB(0x11984, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0x11984, RegisterEDmacCompleteCBR)
NSTUB(0x11A6C, RegisterEDmacAbortCBR)
NSTUB(0x11B00, RegisterEDmacPopCBR)
NSTUB(0x119C0, UnregisterEDmacCompleteCBR)
NSTUB(0x11AA8, UnregisterEDmacAbortCBR)
NSTUB(0x11B3C, UnregisterEDmacPopCBR)

/** PackMem **/
NSTUB(0xFF5F3354, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5F33F0, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5F3068, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5F3284, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF5F317C, PackMem_StartEDmac)

#define RAM_OFFSET (FFA5E120-0x1900) // Nanomad: some functions are copied to RAM at around ff0c0098; they have to be called from RAM...

	NSTUB(0xFF0C10E4, cli_save) 
	NSTUB(0xFF0C10F8, sei_restore) 
NSTUB(0xFF0C1C20, cstart )

NSTUB(0x138A8, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0x76F8, AllocateMemory)
NSTUB(0xFF4B6970, CreateDialogBox)
NSTUB(0x13810, CreateRecursiveLock)
NSTUB(0xFF4B5938, DeleteDialogBox)
NSTUB(0x395C, DryosDebugMsg)

	//FIO Functions
NSTUB(0xFF344AE8, FIO_CreateFile) 
NSTUB(0xFF344DDC, FIO_WriteFile) 
NSTUB(0xFF344E8C, FIO_CloseFile) 
NSTUB(0xFF345E84, FIO_CleanupAfterFindNext_maybe) // proper name: FindClose
NSTUB(0xFF3456E8, FIO_CreateDirectory)
NSTUB(0xFF345CB0, FIO_FindFirstEx)
NSTUB(0xFF345DA4, FIO_FindNextEx)
NSTUB(0xFF345008, FIO_GetFileSize)
NSTUB(0xFF344A2C, FIO_Open)
NSTUB(0xFF344C3C, FIO_ReadFile)
NSTUB(0xFF344B94, FIO_RemoveFile)
NSTUB(0xFF345520, FIO_RenameFile)

NSTUB(0x7A94, FreeMemory)
NSTUB(0xFF0D6FEC, GUI_ChangeMode)
NSTUB(0xFF0D6E30, GUI_Control)
NSTUB(0xFF1BFE58, GUI_GetFirmVersion)
NSTUB(0xFF131454, LoadCalendarFromRTC) // Not so sure....
NSTUB(0xFF4D70E8, RedrawDisplay)
NSTUB(0x139BC, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF1292B0 , SetBitmapVramAddress)
NSTUB(0xFF1AB548, SetGUIRequestMode)

    //NSTUB(0xff136f5c, _audio_ic_read)
    //NSTUB(0xff136ea4, _audio_ic_write) // str:Reg_0x_02X_Data_04x
	//NSTUB(   0x26C34, additional_version) // not working

NSTUB(0x16E8C, alloc_dma_memory)
NSTUB(   0x522CC, bmp_vram_info ) // look in SetBitmapVramAddress - as in 60D
NSTUB(0x1C45C, bzero32)
NSTUB(0xFF138A34 , call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF745CB4, cfReadBlk)
    //	NSTUB(   0x20890, cf_device) idk...
NSTUB(0x2EB8, create_init_task)
NSTUB(0x6178, create_named_semaphore)
NSTUB(0xFF4B46F4, ctrlman_dispatch_event)
NSTUB(0xFF4B76AC, dialog_redraw)
NSTUB(0xFF523C90, dialog_set_property_str)
NSTUB(   0x241C8, dm_names)
NSTUB(0x3CC8, dm_set_store_level) // Not so sure
NSTUB(0x3C60, dm_set_print_level) // Not so sure
NSTUB(0xFF32AC9C, dispcheck)

NSTUB(0xFF0C8340, free) // see "can't malloc"
	NSTUB(0xFFA63518 - RAM_OFFSET, free_dma_memory)
	NSTUB(0xFF62A628, fsuDecodePartitionTable) // AJ_fsuGetPart_related
	NSTUB(0xFFA52A1C - RAM_OFFSET, give_semaphore)
	NSTUB(0xFF0D7340, gui_init_end)
	NSTUB(   0x236A8, gui_main_struct)
	NSTUB(0xFF0D68F8, gui_main_task)
	//NSTUB(0xFF4B07EC, gui_task_create)
	//NSTUB(0xFF4B0918, gui_task_destroy)
	NSTUB(   0x3D7A0, gui_task_list)
NSTUB(0xFF0C5408, init_task)
NSTUB(0xFF0C82E8, malloc)
	NSTUB(0xFFA5FDB4 - RAM_OFFSET, msg_queue_post)
	NSTUB(0xFFA5FBC8 - RAM_OFFSET, msg_queue_receive)
	NSTUB(0xFFA52E0C - RAM_OFFSET, msleep)
	NSTUB(0xFF11EF80, _prop_cleanup)
	NSTUB(0xFF11EEB4, prop_deliver)
	NSTUB(0xFF11E704, prop_register_slave)
	NSTUB(0xFF11E8F8, _prop_request_change)
	NSTUB(0xFF4543AC, ptp_register_handler)
NSTUB(0xFF3409C4, register_func)
	NSTUB(0xFF742AC8, sdReadBlk)
	NSTUB(   0x48B6C, sd_device)
NSTUB(0x62D8 - RAM_OFFSET, take_semaphore)
	NSTUB(0xFFA52EC0 - RAM_OFFSET, task_create)
	NSTUB(   0x232C4, task_dispatch_hook )
	NSTUB(0xFFA581D8 - RAM_OFFSET, task_trampoline)
	NSTUB(0xFFA6153C - RAM_OFFSET, vsnprintf) // not sure it gets called from RAM
	NSTUB(0xFF3F6AA4, vram_get_number)
	NSTUB(   0x6E4A4, vram_info) // in vram_get_number
	NSTUB(0xFF6DF964, LiveViewShutterApp_handler)

	NSTUB(0xFF527C14, LiveViewApp_handler)
	NSTUB(0xFF528ABC, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
	NSTUB(0xFF529DC0, LiveViewApp_handler_end) // start of next function

	NSTUB(0xFF6F916C, PlayMain_handler)
	NSTUB(0xFF54209C, PlayMovieGuideApp_handler)
	NSTUB(0xFF68FA50, ErrCardForLVApp_handler)
	//NSTUB(0xFF511384, ErrCardApp_handler) // Unused
	NSTUB(0xFF512250, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
	NSTUB(0xFF5257A0, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

	NSTUB(0xFF2BFF18, _engio_write)
	NSTUB(0xFF2BFCA4, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
	NSTUB(0xFF2BFC08, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

	    //NSTUB(0xff9b1be0 - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
	NSTUB(0xFF52E4E4, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

	    //NSTUB(0xff987200 - RAM_OFFSET, set_fps_maybe)

NSTUB(0x252C, get_current_task)

	NSTUB(0x505EC, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

		//NSTUB(0xff34db4c, GuiSetAutoBrightness) // guess: refreshes brightness in auto mode?

		//NSTUB(0xFF195A3C, Gui_SetSoundRecord) danger

	NSTUB(0xFFA53AF0 - RAM_OFFSET, GetMemoryInformation)
	    //NSTUB(0xFF143C18, copy_malloc_info_maybe)

	NSTUB(0xFFA5FAB8 - RAM_OFFSET, msg_queue_create)

	NSTUB(0xFF1001BC, PD_RemoteRelease)

	// for task information
	NSTUB(0x247A4, task_max) // Not so sure....
NSTUB(0x1992C, is_taskid_valid) // AJ_task_trampoline_related_p10
	NSTUB(0xFFA5ACE4 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos
	NSTUB(0xFF1337F0, PowerAudioOutput) // not sure, looks different
	NSTUB(0xFF108918, StartASIFDMADAC)  //~ warning: looks different
	NSTUB(0xFF108760, StartASIFDMAADC)  //~ warning: looks different
	NSTUB(0xFF1089F0, StopASIFDMADAC)   //~ warning: looks different
	NSTUB(0xFF108E94, SetNextASIFADCBuffer)
	NSTUB(0xFF108F48, SetNextASIFDACBuffer)
	NSTUB(0xFF132C20, SetSamplingRate )
	NSTUB(0xFF1325B8, SetAudioVolumeOut)

	NSTUB(0xFF19CD0C, ENCODE_StartEncodeJpeg)

	NSTUB(0xFF13BA28, AllocateMemoryResource) // m_pfAllocMemoryCBR
	NSTUB(0xFF13BA74, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
	NSTUB(0xFF13BB8C, FreeMemoryResource) // m_pfFreeMemoryCBR

NSTUB(0x8F28, GetFirstChunkFromSuite)
NSTUB(0x8900, GetMemoryAddressOfMemoryChunk)
NSTUB(0x88A0, GetRemainOfMemoryChunk)
//NSTUB(0x88D0, GetSizeOfMemoryChunk)
//NSTUB(0x8C08, GetSizeOfMemorySuite)
//NSTUB(0x8CE8, GetNumberOfChunks)
NSTUB(0x96C4, GetNextMemoryChunk)

	NSTUB(0xFFA56AB8 - RAM_OFFSET, dma_memcpy) // AJ_HP_Copy_n_DMA_channel_n_BLTDMA

	NSTUB(0xFF1B54F8, GUI_SetRollingPitchingLevelStatus)

	NSTUB(0xFF2BEEDC, mvrSetPrintMovieLog)
	NSTUB(0xFF2BE8EC, mvrSetQscale)
	NSTUB(   0x31424, mvr_config)
	NSTUB(0xFF2BEEB8, mvrFixQScale)
	NSTUB(0xFF2BE9B0, mvrSetDefQScale)

	NSTUB(0xFF11F0DC, prop_get_value)
	NSTUB(0xFF2A4840, ptpPropSetUILock)
	NSTUB(0xFF330408, touch_cbr_canon)

	NSTUB(0xFF5A55FC, lvae_setdispgain)
	NSTUB(0xFF66F328, GetCFnData)
	NSTUB(0xFF66F540, SetCFnData)

	NSTUB(0xFF4D4BE4, NormalDisplay)
	NSTUB(0xFF4D4BB4, ReverseDisplay)
	NSTUB(0xFF4D4B84, MirrorDisplay)

NSTUB(0x16C44, TryPostEvent)
NSTUB(0x16CA0, TryPostEvent_end)

NSTUB(0x16564, TryPostStageEvent)
NSTUB(0x165C0, TryPostStageEvent_end)
