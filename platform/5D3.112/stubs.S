/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.9 firmware for the 550d.
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

.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )

#define RAM_OFFSET 0xFF9DEEE8 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...

NSTUB(0xff9f38a0 - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xff9e8888 - RAM_OFFSET, AllocateMemory)
NSTUB(0xff44c850, CreateDialogBox)
NSTUB(0xFF9EB28C - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xff44b820, DeleteDialogBox)
NSTUB(0xFF9E4A78 - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFF30ACF0, FIO_CleanupAfterFindNext_maybe) // proper name: FindClose
NSTUB(0xff309cf8, FIO_CloseFile)
NSTUB(0xff30a554, FIO_CreateDirectory)
NSTUB(0xff309954, FIO_CreateFile)
NSTUB(0xff30ab1c, FIO_FindFirstEx)
NSTUB(0xff30ac10, FIO_FindNextEx)
NSTUB(0xff309e74, FIO_GetFileSize)
NSTUB(0xff309898, FIO_Open)
NSTUB(0xff309aa8, FIO_ReadFile)
NSTUB(0xff309a00, FIO_RemoveFile)
NSTUB(0xff309c48, FIO_WriteFile)
NSTUB(0xff9e8c24 - RAM_OFFSET, FreeMemory)
NSTUB(0xFF0DEE48, GUI_ChangeMode)
NSTUB(0xFF0DEC8C, GUI_Control)
NSTUB(0xff19d7a8, GUI_GetFirmVersion)
NSTUB(0xFF195804, GUI_SetLvMode)
NSTUB(0xff60ccc8, GetCFnData)
NSTUB(0xff133f50, GuiEdLedBlink)
NSTUB(0xff3401bc, GuiEdLedOff)
NSTUB(0xFF343300, GuiEdLedOn)
NSTUB(0xff135de8, LoadCalendarFromRTC)
NSTUB(0xff133020, MuteOff_0)
NSTUB(0xff12d19c, MuteOn_0)
NSTUB(0xFF46CFC8, RedrawDisplay)
NSTUB(0xff9f39b4 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2
NSTUB(0xff12e110, SetBitmapVramAddress)
//	NSTUB(0xFFB5E994, SetCFnData) // no strings
NSTUB(0xff188c48, SetGUIRequestMode)
NSTUB(0xff136f1c, _audio_ic_read)
NSTUB(0xff136e64, _audio_ic_write) // str:Reg_0x_02X_Data_04x
NSTUB(   0x468b8, additional_version)
NSTUB(0xff9f6e84 - RAM_OFFSET, alloc_dma_memory)
//	NSTUB(0xffcb4ae4, audio_thresholds) - not there?
NSTUB(   0x42d7c, bmp_vram_info )                 // look in SetBitmapVramAddress - as in 60D
NSTUB(0xFF9FB330 - RAM_OFFSET, bzero32)
NSTUB(0xff13b64c, call)
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF6AB2C4, cfReadBlk)
//	NSTUB(   0x20890, cf_device) idk...
NSTUB(0xff0c10e0, cli_save)
NSTUB(0xFF9E1DA0 - RAM_OFFSET, create_init_task)
NSTUB(0xff9e7308 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFF0C1C0C, cstart)
NSTUB(0xff44a5f4, ctrlman_dispatch_event)
NSTUB(0xFF44D57C, dialog_redraw)
NSTUB(0xff352868, dialog_set_property_str)
NSTUB(0xff2f3874, dispcheck)
NSTUB(   0x24b60, dm_names)
NSTUB(0xff9e4de0 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xff9e4d78 - RAM_OFFSET, dm_set_print_level)
NSTUB(0xff9e49f8 - RAM_OFFSET, dmstart)
NSTUB(0xff9e4a38 - RAM_OFFSET, dmstop)
NSTUB(0xff9e5420 - RAM_OFFSET, dumpf)
NSTUB(0xFF0C8144, free) // see "can't malloc"
NSTUB(0xff9f6eb8 - RAM_OFFSET, free_dma_memory)
NSTUB(0xff5bb268, fsuDecodePartitionTable) // AJ_fsuGetPart_related
NSTUB(0xff9e7554 - RAM_OFFSET, give_semaphore)
NSTUB(0xff0df2e4, gui_init_end)
NSTUB(   0x24118, gui_main_struct)
NSTUB(0xFF0DE8BC, gui_main_task)
	NSTUB(0xFF8901A4, gui_massive_event_loop)         // no match
NSTUB(0xff44a744, gui_task_create)
NSTUB(0xFF44A870, gui_task_destroy)
NSTUB(   0x32244, gui_task_list)
NSTUB(0xff0c520c, init_task)
NSTUB(0xff0c80ec, malloc)
NSTUB(0x1c364, memcpy)
NSTUB(0x1486C, msg_queue_post)
NSTUB(0xff9f3568 - RAM_OFFSET, msg_queue_receive)
NSTUB(    0x8a5c, msleep)
NSTUB(0xFF28BB9C, mvrFixQScale)
NSTUB(0xFF0EA9B0, mvrSetBitRate)
NSTUB(0xFF28B62C, mvrSetDeblockingFilter)
NSTUB(0xff28b694, mvrSetDefQScale)
NSTUB(0xFF28BBC0, mvrSetPrintMovieLog)
NSTUB(   0x299f0, mvr_config)
NSTUB(0xff123c88, _prop_cleanup)
NSTUB(0xff123bbc, prop_deliver)
NSTUB(0xff123de4, prop_get_value)
NSTUB(0xff12340c, prop_register_slave)
NSTUB(0xff123600, _prop_request_change)
NSTUB(0xff19e014, prop_request_icu_auto_poweroff)
NSTUB(0xff3f4ee0, ptp_register_handler)
NSTUB(0xff13b5f0, register_func)
NSTUB(0xff6b164c, sdReadBlk)
NSTUB(   0x399D4, sd_device)
NSTUB(0xff0c10f4, sei_restore)
//NSTUB(    0x2360, sounddev) - no sound for now
//NSTUB(0xff85b0e4, sounddev_task)
//NSTUB(0xff85b454, sounddev_active_in)
NSTUB(0xff144880, strcmp)
NSTUB(0xff31b8f0, strcpy)
NSTUB(0xff0c8008, strlen)
NSTUB(0xff9e7468 - RAM_OFFSET, take_semaphore)
NSTUB(0x8b10, task_create)
NSTUB(   0x23d00, task_dispatch_hook )
NSTUB(0xff9ecd10 - RAM_OFFSET, task_trampoline)
//NSTUB(0xFFA41604, vram_get_number) // not found
NSTUB(0xFF2F2884, vram_get_pos_and_size)
//NSTUB(   0x57090, vram_info) // not sure, but not used
NSTUB(0xff9f4edc - RAM_OFFSET, vsnprintf)
NSTUB(0xff658a48, LiveViewApp_handler)
NSTUB(0xff4dfb20, PlayMain_handler)
//NSTUB(0xFF4B88F4, OlcAFFrameApp_handler)
NSTUB(0xFF49C0B0, ErrCardForLVApp_handler) // not sure
NSTUB(0xFF49CBF4, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

NSTUB(0xff28cbfc, _engio_write)
NSTUB(0xff28c988, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xff28c8ec, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

	//NSTUB(0xff9b1be0 - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
NSTUB(0xff4b68bc, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

	//NSTUB(0xff987200 - RAM_OFFSET, set_fps_maybe)

NSTUB(0xff9e1414 - RAM_OFFSET, get_current_task)

NSTUB(0x4157c, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

//NSTUB(0xff34db4c, GuiSetAutoBrightness) // guess: refreshes brightness in auto mode?

//NSTUB(0xFF195A3C, Gui_SetSoundRecord) danger

NSTUB(0xFF9E8628 - RAM_OFFSET, GetMemoryInformation)
NSTUB(0xFF1449FC, copy_malloc_info_maybe)

NSTUB(0xff9eb0a4 - RAM_OFFSET, msg_queue_create)

NSTUB(0xff1064c8, PD_RemoteRelease)

// for task information
NSTUB(0x25114, task_max)
NSTUB(0xFF9F8800 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xff9ef81c - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xFF1378CC, PowerAudioOutput)
NSTUB(0xff10ec58, StartASIFDMADAC)
NSTUB(0xff10eaa0, StartASIFDMAADC)
NSTUB(0xff10ed30, StopASIFDMADAC)
NSTUB(0xff10f1d4, SetNextASIFADCBuffer)
NSTUB(0xFF10F288, SetNextASIFDACBuffer)
NSTUB(0xff137964, SetSamplingRate )
NSTUB(0xff137064, SetAudioVolumeOut)

