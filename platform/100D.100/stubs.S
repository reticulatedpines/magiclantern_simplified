/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware for the 100D.
 *
 *  I've copied the skeleton from the 5D3. This should probably be common
 *  practice for future ports as the 5D3 is updated most often.
 *
 *
 *  INDENTED = NEED TO FIND
 *
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

#define RAM_OFFSET g //~ Subtract 0x1900 from sub_foo at ROM:FF0C0098.

/** Startup **/
NSTUB(0xFF0C1BD0, cstart)
NSTUB(0xFFAF5BB8 - RAM_OFFSET, bzero32)
NSTUB(0xFFA9A738 - RAM_OFFSET, create_init_task)
NSTUB(0xFF0C56C0, init_task)
NSTUB(   0x687BC, additional_version)   //~ just after string: "***** PROP_ARTIST size = 0[%d]"

/** Misc **/
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF13AB60, LoadCalendarFromRTC)
NSTUB(0xFFAD03A0 - RAM_OFFSET, vsnprintf)

/** File I/O **/
NSTUB(0xFF33E3AC, _FIO_CreateFile)
NSTUB(0xFF33E2F0, _FIO_Open) // or OpenFile?
NSTUB(0xFF33E750,  FIO_CloseFile)
NSTUB(0xFF33E680,  FIO_SeekFile)
NSTUB(0xFF33E500,  FIO_ReadFile)
NSTUB(0xFF33E6A0,  FIO_WriteFile)
NSTUB(0xFF33E8CC, _FIO_GetFileSize)
NSTUB(0xFF33E458, _FIO_RemoveFile)
NSTUB(0xFF33EFAC, _FIO_CreateDirectory)
NSTUB(0xFF33F328, _FIO_FindFirstEx)
NSTUB(0xFF33F668,  FIO_FindNextEx)
NSTUB(0xFF33F748,  FIO_CleanupAfterFindNext_maybe) // proper name: FindClose

/** Bitmap **/
NSTUB(   0x8ED9C, bmp_vram_info )                 // look in SetBitmapVramAddress - as in 60D
NSTUB(   0x8D59C, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

/** Debug messages **/
NSTUB(   0x66108, dm_names) //~ first 'LDR R4, =foo' in any debug function with string "./DbgMgr/DbgMgr.c"
NSTUB(0xFFA9C284 - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFFA9C5F0 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFFA9C588 - RAM_OFFSET, dm_set_print_level)

/** Task sleep **/
NSTUB(0xFFA9F144 - RAM_OFFSET, msleep)

/** Semaphores **/
NSTUB(0xFFA9EB08 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFFA9EC68 - RAM_OFFSET, take_semaphore)
NSTUB(0xFFA9ED54 - RAM_OFFSET, give_semaphore)

/** Recursive locks **/
NSTUB(0xFFAA2DFC - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFFACED0C - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFACEE20 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2

/** Message queues **/
NSTUB(0xFFAA2C14 - RAM_OFFSET, msg_queue_create)
NSTUB(0xFFACEBC0 - RAM_OFFSET, msg_queue_post)
NSTUB(0xFFACE9D4 - RAM_OFFSET, msg_queue_receive)

/** Tasks **/
NSTUB(0xFFA9F1F8 - RAM_OFFSET, task_create)
NSTUB(   0x6519C, task_dispatch_hook )  //~ just above cstart
NSTUB(0xFFAA4880 - RAM_OFFSET, task_trampoline)

/** Task info **/
NSTUB(   0x6676C, task_max)
NSTUB(0xFFA99DAC - RAM_OFFSET, get_current_task)
NSTUB(0xFFAE3694 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFFAA7330 - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

/** Interrupts **/
NSTUB(0xFF0C10B0, cli_save)
NSTUB(0xFF0C10C4, sei_restore)

/** Eventprocs (call by name) **/
NSTUB(0xFF143278, call)

/** GUI **/
NSTUB(0xFF0D6348, GUI_Control)
NSTUB(0xFF196BE8, SetGUIRequestMode)
NSTUB(0xFF0D5F74, gui_main_task)
NSTUB(   0x6557C, gui_main_struct)  //~ in gui_main_task
NSTUB(0xFF0D69EC, gui_init_end)
NSTUB(0xFF1B2C88, gui_massive_event_loop)
NSTUB(   0x761C0, gui_task_list)    //~ by string "DispatchingStackPointer != SIZE_OF_DISPATCHING_STACK -2"

/** Dialog API **/
NSTUB(0xFF4BE61C, CreateDialogBox)
NSTUB(0xFF4BD5B4, DeleteDialogBox)
NSTUB(0xFF4BE8D8, dialog_redraw)
NSTUB(0xFF5345C4, dialog_set_property_str)
NSTUB(0xFF4E1F80, RedrawDisplay)

/** Dialog handlers **/
NSTUB(0xFF6B4DD8, LiveViewLevelApp_handler)
    NSTUB(0xFF623240, LiveViewWbApp_handler)    //~ absent in 100d?!
NSTUB(0xFF541510, LiveViewApp_handler)
NSTUB(0xFF3AE880, PlayMain_handler)
NSTUB(0xFF55F944, PlayMovieGuideApp_handler)
NSTUB(0xFF525714, ErrCardForLVApp_handler) // not sure
NSTUB(0xFF5267D8, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF53B5A4, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer
NSTUB(0xFF54E650, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c

/** Properties **/
NSTUB(0xFF127F14, prop_register_slave)
NSTUB(0xFF12874C, _prop_cleanup)
NSTUB(0xFF128680, prop_deliver)
NSTUB(0xFF128104, _prop_request_change)
NSTUB(0xFF12886C, _prop_get_value) // unsafe

/** Memory allocation **/
NSTUB(0xFF0C8FC8, malloc)
NSTUB(0xFF0C9020, free) // see "can't malloc"
NSTUB(0xFFAA0088 - RAM_OFFSET, AllocateMemory)
NSTUB(0xFFAA0424 - RAM_OFFSET, FreeMemory)
NSTUB(0xFFAD2348 - RAM_OFFSET, alloc_dma_memory)
NSTUB(0xFFAD237C - RAM_OFFSET, free_dma_memory)
NSTUB(0xFFA9FE28 - RAM_OFFSET, GetMemoryInformation)

/** ExMem **/
NSTUB(0xFF146E18, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF146E64, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF146F7C, FreeMemoryResource) // m_pfFreeMemoryCBR

NSTUB(0xFFAA16B8 - RAM_OFFSET, GetFirstChunkFromSuite)
NSTUB(0xFFAA23BC - RAM_OFFSET, GetNextMemoryChunk)
NSTUB(0xFFAA1334 - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)

/** Engio **/
NSTUB(0xFF2B2254, _engio_write)
NSTUB(0xFF2B1FD8, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF2B1F3C, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

/** PTP **/
NSTUB(0xFF296974, ptp_register_handler)

/** Audio **/
    NSTUB(0xff136f5c, _audio_ic_read)   //~ needs work
    NSTUB(0xff136ea4, _audio_ic_write)

/** ASIF **/
//~ idk, audio stuff changed like usual. should be mostly right.
NSTUB(0xFF2AE580, PowerAudioOutput)
NSTUB(0xFF1127AC, StartASIFDMADAC)
NSTUB(0xFF1123D8, StartASIFDMAADC)
NSTUB(0xFF112A2C, StopASIFDMADAC)
NSTUB(0xFF112E70, SetNextASIFADCBuffer)
NSTUB(0xFF113058, SetNextASIFDACBuffer)
NSTUB(0xFF1187B0, SetSamplingRate )
    NSTUB(0xff1370a4, SetAudioVolumeOut)

/** experimental EDMAC research **/
NSTUB(0xFFAA1D4C - RAM_OFFSET, CreateMemorySuite)
NSTUB(0xFFAA199C - RAM_OFFSET, DeleteMemorySuite)


/** EDMAC routines */
NSTUB(0xFFACC1AC - RAM_OFFSET, SetEDmac)
NSTUB(0xFFACC558 - RAM_OFFSET, StartEDmac)
//NSTUB(0x12B38, PopEDmac)
NSTUB(0xFFACC8C4 - RAM_OFFSET, PopEDmac)
NSTUB(0xFFACC87C - RAM_OFFSET, AbortEDmac)
NSTUB(0xFFACC30C - RAM_OFFSET, ConnectWriteEDmac)
NSTUB(0xFFACC3D0 - RAM_OFFSET, ConnectReadEDmac)

/** keep the old name until all are refcatored **/
NSTUB(0xFFACC91C - RAM_OFFSET, EDMAC_RegisterCompleteCBR)

/** register/unregister CBR names **/
NSTUB(0xFFACC91C - RAM_OFFSET, RegisterEDmacCompleteCBR)
NSTUB(0xFFACC9B0 - RAM_OFFSET, RegisterEDmacAbortCBR)
NSTUB(0xFFACCA44 - RAM_OFFSET, RegisterEDmacPopCBR)
NSTUB(0xFFACC958 - RAM_OFFSET, UnregisterEDmacCompleteCBR)
NSTUB(0xFFACC9EC - RAM_OFFSET, UnregisterEDmacAbortCBR)
NSTUB(0xFFACCA80 - RAM_OFFSET, UnregisterEDmacPopCBR)


NSTUB(0xFF60842C, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF6084C8, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF608140, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF60835C, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF608254, PackMem_StartEDmac)



/** EVERYTHING BELOW IS UNTOUCHED FROM 5D3 **/

/** Making the card bootable (not working on 5D3) **/
// NSTUB(0xff6ab324, cfReadBlk)
// NSTUB(   0x20890, cf_device) idk...
// NSTUB(0xff6b16ac, sdReadBlk)
// NSTUB(   0x399D4, sd_device)
// NSTUB(0xff5bb2c8, fsuDecodePartitionTable) // AJ_fsuGetPart_related

/** Unused **/
// NSTUB(0x23D08, pre_isr_hook)
// NSTUB(0x23D0C, post_isr_hook)
// NSTUB(0xff181314, ENCODE_StartEncodeJpeg)
// NSTUB(0xFF9EB650 - RAM_OFFSET, dma_memcpy)
// NSTUB(0xFF195084, GUI_SetRollingPitchingLevelStatus) // has side effects
// NSTUB(0xff1064c8, PD_RemoteRelease)
// NSTUB(0xff144a3c, copy_malloc_info_maybe)
// NSTUB(0xff9b1be0 - RAM_OFFSET, LightMeasure_n_Callback_r0) - looks a bit different, disabling for now
// NSTUB(0xFF0DEE48, GUI_ChangeMode)
// NSTUB(0xff19d7e8, GUI_GetFirmVersion)
// NSTUB(0xff44a668, ctrlman_dispatch_event)
// NSTUB(0xff2f3f8c, MuteOff_0)
// NSTUB(0xff2f3f44, MuteOn_0)
// NSTUB(0xff12e110, SetBitmapVramAddress)
// NSTUB(0xff44a7b8, gui_task_create)
// NSTUB(0xFF44A870, gui_task_destroy)
// NSTUB(0xff4b09e4, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
// NSTUB(0xff4b1cc4, LiveViewApp_handler_end) // start of next function
// NSTUB(0xff13b630, register_func)
// NSTUB(0xFF9E9AC0 - RAM_OFFSET, GetSizeOfMemoryChunk)
// NSTUB(0xFF9E9A90 - RAM_OFFSET, GetRemainOfMemoryChunk)
// NSTUB(0xFF9E9ED8 - RAM_OFFSET, GetNumberOfChunks)
// NSTUB(0xFF9E9DF8 - RAM_OFFSET, GetSizeOfMemorySuite)
