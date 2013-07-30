/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.1 firmware for the 650D.
 *
 *  This is a straight copy from the 5d3, EOS M stubs need found/verified still.
 *  Indented line = incorrect.
 *  Initial stubs identified by Indy, most of the other stubs were matched by hand
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

NSTUB(0xFF376650, lv_change_raw)

/** experimental EDMAC research **/
NSTUB(0x00008B7C, CreateMemorySuite)
NSTUB(0x00008974, DeleteMemorySuite)

/** EDMAC routines */
NSTUB(0x00011480, SetEDmac)
NSTUB(0x00011798, StartEDmac)
NSTUB(0x0001192C, PopEDmac) //Xref: PackMem_PopEDmacForMemorySuite
NSTUB(0x000118E4, AbortEDmac)
NSTUB(0x0001152C, ConnectWriteEDmac)
NSTUB(0x000115F0, ConnectReadEDmac)


/** keep the old name until all are refactored **/
NSTUB(0x00011984, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0x00011984, RegisterEDmacCompleteCBR)
NSTUB(0x00011A6C, RegisterEDmacAbortCBR)
NSTUB(0x00011B00, RegisterEDmacPopCBR)
NSTUB(0x000119C0, UnregisterEDmacCompleteCBR)
NSTUB(0x00011AA8, UnregisterEDmacAbortCBR)
NSTUB(0x00011B3C, UnregisterEDmacPopCBR)

/** PackMem **/
NSTUB(0xFF5EDF58, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5EDFF4, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5EDC6C, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5EDE88, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF5EDD80, PackMem_StartEDmac)

#define RAM_OFFSET (0xFFA4DF58-0x1900) // Indy: some functions are copied to RAM at around ff0c0098; they have to be called from RAM...

NSTUB(0xFF0C10E4, cli_save) // from Indy
NSTUB(0xFF0C10F8, sei_restore) // from Indy
NSTUB(0xFF0C1C10, cstart ) // from Indy
//NSTUB(0xFF137B18, bootdisk_enable) // from Indy
//NSTUB(0xFF137B24, bootdisk_disable) // from Indy

NSTUB(0xFFA5FF00 - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFA53D50 - RAM_OFFSET, AllocateMemory)
NSTUB(0xFF4B2918, CreateDialogBox)
NSTUB(0xFFA56754 - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFF4B18E0, DeleteDialogBox)
NSTUB(0xFFA4FFB4 - RAM_OFFSET, DryosDebugMsg)

//FIO Functions
NSTUB(0xFF341CE4, FIO_CreateFile) // from Indy
NSTUB(0xFF341FD8, FIO_WriteFile) // from Indy
NSTUB(0xFF342088, FIO_CloseFile) // from Indy
NSTUB(0xFF343080, FIO_CleanupAfterFindNext_maybe) // proper name: FindClose
NSTUB(0xFF3428E4, FIO_CreateDirectory)
NSTUB(0xFF342EAC, FIO_FindFirstEx)
NSTUB(0xFF342FA0, FIO_FindNextEx)
NSTUB(0xFF342204, FIO_GetFileSize)
NSTUB(0xFF341C28, FIO_Open)
NSTUB(0xFF341E38, FIO_ReadFile)
NSTUB(0xFF341D90, FIO_RemoveFile)
NSTUB(0xFF34271C, FIO_RenameFile)
NSTUB(0xFF341FB8, FIO_SeekFile)

NSTUB(0xFFA540EC - RAM_OFFSET, FreeMemory)
NSTUB(0xFF0D6E88, GUI_ChangeMode)
NSTUB(0xFF0D6CCC, GUI_Control)
NSTUB(0xFF1BDDC4, GUI_GetFirmVersion)
NSTUB(0xFF1308C4, LoadCalendarFromRTC) // Not so sure....
NSTUB(0xFF32861C, MuteOff_0)
NSTUB(0xFF3285D4, MuteOn_0)
NSTUB(0xFF4D3090, RedrawDisplay)
NSTUB(0xFFA60014 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xFF128734, SetBitmapVramAddress)
NSTUB(0xFF1A953C, SetGUIRequestMode)

    //NSTUB(0xff136f5c, _audio_ic_read)
    //NSTUB(0xff136ea4, _audio_ic_write) // str:Reg_0x_02X_Data_04x
//NSTUB(   0x26C34, additional_version) // not working

NSTUB(0xFFA634E4 - RAM_OFFSET, alloc_dma_memory)
NSTUB(   0x51DEC, bmp_vram_info ) // look in SetBitmapVramAddress - as in 60D
NSTUB(0xFFA68AB4 - RAM_OFFSET, bzero32)
NSTUB(0xFF137E84, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF73C608, cfReadBlk)
        //	NSTUB(   0x20890, cf_device) idk...
NSTUB(0xFFA4F510 - RAM_OFFSET, create_init_task)
NSTUB(0xFFA527D0 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFF4B069C, ctrlman_dispatch_event)
NSTUB(0xFF4B3654, dialog_redraw)
NSTUB(0xFF51FCE0, dialog_set_property_str)
NSTUB(   0x241BC, dm_names)
NSTUB(0xFFA50320 - RAM_OFFSET, dm_set_store_level) // Not so sure
NSTUB(0xFFA502B8 - RAM_OFFSET, dm_set_print_level) // Not so sure
NSTUB(0xFF327FCC, dispcheck)

NSTUB(0xFF0C8364, free) // see "can't malloc"
NSTUB(0xFFA63518 - RAM_OFFSET, free_dma_memory)
NSTUB(0xFF62A628, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xFFA52A1C - RAM_OFFSET, give_semaphore)
NSTUB(0xFF0D7340, gui_init_end)
NSTUB(   0x236A8, gui_main_struct)
NSTUB(0xFF0D68F8, gui_main_task)
//NSTUB(0xFF4B07EC, gui_task_create)
//NSTUB(0xFF4B0918, gui_task_destroy)
NSTUB(   0x3D7A0, gui_task_list)
NSTUB(0xFF0C542C, init_task)
NSTUB(0xFF0C830C, malloc)
NSTUB(0xFFA5FDB4 - RAM_OFFSET, msg_queue_post)
NSTUB(0xFFA5FBC8 - RAM_OFFSET, msg_queue_receive)
NSTUB(0xFFA52E0C - RAM_OFFSET, msleep)
NSTUB(0xFF11EF80, _prop_cleanup)
NSTUB(0xFF11EEB4, prop_deliver)
NSTUB(0xFF11E704, prop_register_slave)
NSTUB(0xFF11E8F8, _prop_request_change)
NSTUB(0xFF4543AC, ptp_register_handler)
NSTUB(0xFF33DBC0, register_func)
NSTUB(0xFF742AC8, sdReadBlk)
NSTUB(   0x48B6C, sd_device)
NSTUB(0xFFA52930 - RAM_OFFSET, take_semaphore)
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

NSTUB(0xFF3B5D4C, PlayMain_handler)
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
NSTUB(0xFFA65F84 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
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
