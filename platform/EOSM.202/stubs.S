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

#define RAM_OFFSET 0xFFA69590 // some functions are copied to RAM at around ff0c009c; they have to be called from RAM...

/** Startup **/
NSTUB(0xFF0C1C10, cstart)
NSTUB(0xFFAA0468 - RAM_OFFSET, bzero32)
NSTUB(0xFFA6C448 - RAM_OFFSET, create_init_task) //2EB8
NSTUB(0xFF0C5484, init_task)
	//NSTUB(   0x468b8, additional_version)

/** Misc **/
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF132118, LoadCalendarFromRTC)
NSTUB(0xFFA89B5C - RAM_OFFSET, vsnprintf) //205CC

/** File I/O **/
NSTUB(0xFF346550, FIO_CreateFile)
NSTUB(0xFF346494, FIO_Open)
NSTUB(0xFF3468F4, FIO_CloseFile)
NSTUB(0xFF346824, FIO_SeekFile)
NSTUB(0xFF3466A4, FIO_ReadFile)
NSTUB(0xFF346844, FIO_WriteFile)
NSTUB(0xFF346A70, FIO_GetFileSize)
NSTUB(0xFF3465FC, FIO_RemoveFile)
NSTUB(0xFF347150, FIO_CreateDirectory)
NSTUB(0xFF347718, FIO_FindFirstEx)
NSTUB(0xFF34780C, FIO_FindNextEx)
NSTUB(0xFF3478EC, FIO_FindClose) // proper name: FindClose
NSTUB(0xFF346F88, FIO_RenameFile)

/** Bitmap **/
NSTUB(   0x6D2B4, bmp_vram_info ) // look in SetBitmapVramAddress - as in 60D
NSTUB(	 0x6BAB4, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800

/** Debug messages **/
NSTUB(   0x3EC7C, dm_names)
NSTUB(0xFFA6D664 - RAM_OFFSET, DryosDebugMsg)
NSTUB(0xFFA6D9D0 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFFA6D968 - RAM_OFFSET, dm_set_print_level)

/** Task sleep **/
NSTUB(0xFFA70524 - RAM_OFFSET, msleep) //6F94

/** Semaphores **/
NSTUB(0xFFA6FEE8 - RAM_OFFSET, create_named_semaphore)
NSTUB(0xFFA70048 - RAM_OFFSET, take_semaphore)
NSTUB(0xFFA70134 - RAM_OFFSET, give_semaphore)


/** Recursive locks **/
NSTUB(0xFFA73E6C - RAM_OFFSET, CreateRecursiveLock)
NSTUB(0xFFA87CB8 - RAM_OFFSET, AcquireRecursiveLock) // AJ_KernelDry_KerRLock.c
NSTUB(0xFFA87D64 - RAM_OFFSET, ReleaseRecursiveLock) // AJ_KernelDry_KerRLock.c_p2

/** Message queues **/
NSTUB(0xFFA73C84 - RAM_OFFSET, msg_queue_create)
NSTUB(0xFFA883D4 - RAM_OFFSET, msg_queue_post) //1EE44
NSTUB(0xFFA881E8 - RAM_OFFSET, msg_queue_receive) //1EC58
NSTUB(0xFFA88414 - RAM_OFFSET, msg_queue_count) // 0x1EE84 Above "!!!!!!! QUE Overflo"


/** Tasks **/
NSTUB(0xFFA705D8 - RAM_OFFSET, task_create)
NSTUB(   0x3DD60, task_dispatch_hook )
NSTUB(0xFFA758F0 - RAM_OFFSET, task_trampoline)

/** Task info **/
NSTUB(0x3F14C, task_max)
NSTUB(0xFFA6BABC - RAM_OFFSET, get_current_task)
NSTUB(0xFFA96530 - RAM_OFFSET, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xFFA783FC - RAM_OFFSET, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

/** Interrupts **/
NSTUB(0xFF0C10E4, cli_save)
NSTUB(0xFF0C10F8, sei_restore)

/** Eventprocs (call by name) **/
NSTUB(0xFF137C1C, call)

/** GUI **/
NSTUB(0xFF0D7674, GUI_Control)
NSTUB(0xFF1A805C, SetGUIRequestMode)
NSTUB(0xFF0D72A0, gui_main_task)
NSTUB(   0x3E154, gui_main_struct)
NSTUB(0xFF0D7D18, gui_init_end)
	//NSTUB(0xff1a4fc8, gui_massive_event_loop)
NSTUB(   0x58DA0, gui_task_list)


/** Dialog API **/
NSTUB(0xFF4C3BC0, CreateDialogBox)
NSTUB(0xFF4C2B88, DeleteDialogBox)
NSTUB(0xFF4C48FC, dialog_redraw)
NSTUB(0xFF52F748, dialog_set_property_str)
NSTUB(0xFF4E4338, RedrawDisplay)

	/** Dialog handlers **/
	//NSTUB(0xff658aa8, LiveViewLevelApp_handler)
	//NSTUB(0xFF623240, LiveViewWbApp_handler)
NSTUB(0xFF53A558, LiveViewApp_handler)
NSTUB(0xFF3BC078, PlayMain_handler)
NSTUB(0xFF5540E4, PlayMovieGuideApp_handler)
NSTUB(0xFF6A2E00, ErrCardForLVApp_handler)
NSTUB(0xFF520960, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)
NSTUB(0xFF53800C, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer
NSTUB(0xFF541BD4, ShootOlcApp_handler) // AJ_DIALOG.HANDLER_DlgShootOlcInfo.c
NSTUB(0xFF6FBD44, LiveViewShutterApp_handler)


/** Properties **/
NSTUB(0xFF11F910, prop_register_slave)
NSTUB(0xFF12018C, _prop_cleanup)
NSTUB(0xFF1200C0, prop_deliver)
NSTUB(0xFF11FB04, _prop_request_change)
NSTUB(0xFF1202E8, prop_get_value)


/** Memory allocation **/
NSTUB(0xFF0C8364, _malloc)
NSTUB(0xFF0C83BC, _free) // see "can't malloc"
NSTUB(0xFFA71468 - RAM_OFFSET, _AllocateMemory)
NSTUB(0xFFA71804 - RAM_OFFSET, _FreeMemory)
NSTUB(0xFFA8BB04 - RAM_OFFSET, _alloc_dma_memory)
NSTUB(0xFFA8BB38 - RAM_OFFSET, _free_dma_memory)
NSTUB(0xFFA71208 - RAM_OFFSET, GetMemoryInformation)

/** ExMem **/
NSTUB(0xFF13B7C0, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xFF13B80C, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xFF13B924, FreeMemoryResource) // m_pfFreeMemoryCBR

NSTUB(0xFFA72C98 - RAM_OFFSET, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xFFA73434 - RAM_OFFSET, GetNextMemoryChunk)
NSTUB(0xFFA72670 - RAM_OFFSET, GetMemoryAddressOfMemoryChunk)
NSTUB(0x7C24, GetSizeOfMaxRegion)

/** Engio **/
NSTUB(0xFF2C19AC, _engio_write)
NSTUB(0xFF2C1730, shamem_read) // AJ_0x8FB0_engio_struct_n_R0_manipulation_to_get_ptr
NSTUB(0xFF2C1694, _EngDrvOut) // AJ_EngDrvOut_1xVar_to_ShadowStruct

/** Edmac **/
NSTUB(0x935C, CreateMemorySuite)
NSTUB(0x9154, DeleteMemorySuite)

NSTUB(0x1C5FC, SetEDmac)
NSTUB(0x1C914, StartEDmac)
NSTUB(0x1CAA8, PopEDmac) //From popedmacformemorysuite
NSTUB(0x1CA60, AbortEDmac) 
NSTUB(0x1C6A8, ConnectWriteEDmac)
NSTUB(0x1C76C, ConnectReadEDmac)

/** register/unregister CBR names **/
NSTUB(0x1CB00, RegisterEDmacCompleteCBR) 
NSTUB(0x1CBE8, RegisterEDmacAbortCBR)
NSTUB(0x1CC7C, RegisterEDmacPopCBR) 
NSTUB(0x1CB3C, UnregisterEDmacCompleteCBR)
NSTUB(0x1CC24, UnregisterEDmacAbortCBR) 
NSTUB(0x1CCB8 , UnregisterEDmacPopCBR)  

NSTUB(0xFF5FEE38, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5FF1C0, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5FF124, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5FF25C, PackMem_RegisterEDmacAbortCBRForMemorySuite)

NSTUB(0xFF5FEF4C, PackMem_StartEDmac)
NSTUB(0xFF5FF054, PackMem_PopEDmacForMemorySuite)

/** PTP **/
NSTUB(0xFF45FBDC, ptp_register_handler)

/** Audio **/
//	NSTUB(0xFF130C94, _audio_ic_read) // Good Candidate i2c read
//	NSTUB(0xFF339B00, _audio_ic_read) // Good Candidate i2c read
	NSTUB(0xFF339ADC, _audio_ic_read) // Good Candidate i2c read
	NSTUB(0xFF339490, _audio_ic_write) // str:i2c(int *array)

NSTUB(0xFF339490, i2c_write)
#NSTUB(0xFF3463F8, i2c_read)
NSTUB(0xFF346230, i2c_read)
NSTUB(0xFF133DC0, send_audio)

NSTUB(0xFF7FBBDC, audio_thresholds)
NSTUB(	0x3E46C, sounddev )
NSTUB(0xFF10CD40, sounddev_task )

	NSTUB(0xFF10D118, SoundDevActiveIn)
	NSTUB(0xFF10D144, SoundDevActiveOut)
	NSTUB(0xFF10D370, SoundDevShutDownIn)


/** ASIF **/
NSTUB(0xFF1350F0, PowerAudioOutput) //Look for actrlCpowerControl Case 1 Sub
NSTUB(0xFF10AA48, StartASIFDMADAC)  //~ Needs Patches Or
NSTUB(0xFF10ACC8, StopASIFDMADAC)   //~ NormalStopAsif
NSTUB(0xFF10B378, SetNextASIFDACBuffer) // Int16 Regular
NSTUB(0xFF134504, SetSamplingRate )
NSTUB(0xFF1330A0, SetAudioVolumeOut)

NSTUB(0xFF10A674, StartASIFDMAADC)  //~ To Regular
NSTUB(0xFF10A920, StopASIFDMAADC)   //~ Regular
NSTUB(0xFF10B190, SetNextASIFADCBuffer) // Int16
NSTUB(0xFF134CE4, PowerMicAmp)



//NSTUB(0xFF2BF578, StartASIFDMADAC)  //~ Direct
//NSTUB(0xFF111D74, StartASIFDMADAC)  //~ SePlayStart
	
	
	

//NSTUB(0xFF10AC98, StopASIFDMADAC)   //~ NormalStopAsif
//NSTUB(0xFF2BF754, StopASIFDMADAC)   //~ ACTRL Direct
//NSTUB(0xFF4743D0, StopASIFDMADAC)   //~ AsifStopCBR
//NSTUB(0xFF133CA0, StopASIFDMADAC)   //~ StopPlayWavData
//NSTUB(0xFF111EE4, StopASIFDMADAC)   //~ TouchSoundSePlayStop
//NSTUB(0xFF111D10, StopASIFDMADAC)   //~ SeTerminate
	//NSTUB(0xFF339938, StopASIFDMADAC)   //~ Call Play Stop

//NSTUB(0xFF2BFAEC, SetNextASIFDACBuffer) // Int16 Direct Control



/** Making the card bootable (How?) **/
	// NSTUB(0x0, cfReadBlk)
	// NSTUB(0x0, cf_device) idk...
//NSTUB(0xFF642910, sdReadBlk)
NSTUB(   0x606DC, sd_device)
NSTUB(0xFF4F8348, fsuDecodePartitionTable) // AJ_fsuGetPart_related


/* Electronic level - roll/pitch info */
	//NSTUB(0xFF1B4454, GUI_SetRollingPitchingLevelStatus)

/* H264 Encoder  */
NSTUB(0xFF2C0968, mvrSetPrintMovieLog)
NSTUB(0xFF2C0378, mvrSetQscale)
NSTUB(   0x4C124, mvr_config)
NSTUB(0xFF2C0944, mvrFixQScale)
NSTUB(0xFF2C043C, mvrSetDefQScale)

/* Touchscreen  */
NSTUB(0xFF334A98, touch_cbr_canon)

/* Debugging  */

NSTUB(0x2232C, TryPostEvent)
NSTUB(0x223B0 , TryPostEvent_end)

NSTUB(0x21C4C, TryPostStageEvent)
NSTUB(0x21CD0, TryPostStageEvent_end)

	NSTUB(0x3E1C8, pre_isr_hook)
	NSTUB(0x3E1CC, post_isr_hook)


/* Misc  */
NSTUB(0xFFA741D0 - RAM_OFFSET, dma_memcpy)
NSTUB(0xFF6818B4, GetCFnData)
NSTUB(0xFF681ACC, SetCFnData)

/* ResLock */
NSTUB(0xFF2C5848, CreateResLockEntry)
NSTUB(0xFF2C5D10, LockEngineResources)
NSTUB(0xFF2C5EB4, UnLockEngineResources)


/* Unused Some Are OldFW  */
//NSTUB(0xFF0D783C, GUI_ChangeMode)
//NSTUB(0xFF1BD13C, GUI_GetFirmVersion)
//NSTUB(0xFF32D348, MuteOff_0)
//NSTUB(0xFF32D300, MuteOn_0)
//NSTUB(0xFF1299C4, SetBitmapVramAddress)
//NSTUB(0xFF4C135C, ctrlman_dispatch_event)
//NSTUB(0xFF3420F4, register_func)
//NSTUB(   0x8D330, vram_info)
		//NSTUB(0xff987200 - RAM_OFFSET, set_fps_maybe)
//NSTUB(0xFF1B4F70, Gui_SetSoundRecord) // Look for GUI_setsoundrec - Just changes prop
//NSTUB(0xFF143C18, copy_malloc_info_maybe)
//NSTUB(0xFF101E04, PD_RemoteRelease)
//NSTUB(0xFF19991C, ENCODE_StartEncodeJpeg)
//NSTUB(0xFF2A3378, ptpPropSetUILock)
//NSTUB(0xFF5B6688, lvae_setdispgain)
//NSTUB(0xFF0E5808, MVR_NotifyLvFrameTiming)
//NSTUB(0xFFA71D5C - RAM_OFFSET, GetRemainOfMemoryChunk)
//NSTUB(0xFFA71D8C - RAM_OFFSET, GetSizeOfMemoryChunk)
//NSTUB(0xFFA720C4 - RAM_OFFSET, GetSizeOfMemorySuite)
//NSTUB(0xFFA721A4 - RAM_OFFSET, GetNumberOfChunks)

//FIND
//NSTUB(0x35700, MREQ_ISR)
//NSTUB(0x35784, SIO3_ISR)
//NSTUB(0x8FE4, register_interrupt)
//Unused
