/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.1 firmware for the 700D.
 *
 *  Indented line = incorrect.
 *  Initial stubs identified by Nanomad, most of the other stubs were matched by hand
 *  Stubs sorted, checked and some added by TheUnkn0wn
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

#define RAM_OFFSET (0xFFA5E120-0x1900) // Nanomad: some functions are copied to RAM at around ff0c0098; they have to be called from RAM...

/** Startup **/
NSTUB(0xFF0C1C20, cstart)
NSTUB(   0x1C45C, bzero32)
NSTUB(    0x2EB8, create_init_task)
NSTUB(0xFF0C5408, init_task)
NSTUB(   0x26CE8, additional_version)

/** Misc **/
NSTUB(0xC0220000, camera_engine)
NSTUB(0xFF13153C, LoadCalendarFromRTC)
NSTUB(0xFFA71704 - RAM_OFFSET, vsnprintf)
NSTUB(0xFF3409C4, register_func)
NSTUB(0xFF100A44, PD_RemoteRelease)
NSTUB(0xFF19E87C, ENCODE_StartEncodeJpeg)

/** File I/O **/
NSTUB(0xFF344A2C, _FIO_Open)
NSTUB(0xFF344AE8, _FIO_CreateFile)
NSTUB(0xFF344B94, _FIO_RemoveFile)
NSTUB(0xFF344C3C, FIO_ReadFile)
NSTUB(0xFF344DBC, FIO_SeekFile)
NSTUB(0xFF344DDC, FIO_WriteFile)
NSTUB(0xFF344E8C, FIO_CloseFile)
NSTUB(0xFF345008, _FIO_GetFileSize)
NSTUB(0xFF345520, FIO_RenameFile)
NSTUB(0xFF3456E8, _FIO_CreateDirectory)
NSTUB(0xFF345CB0, _FIO_FindFirstEx)
NSTUB(0xFF345DA4, FIO_FindNextEx)
NSTUB(0xFF345E84, FIO_FindClose) //FindClose

/** Bitmap **/
NSTUB(   0x522CC, bmp_vram_info)
NSTUB(   0x50ACC, LCD_Palette)
NSTUB(0xFF1292B0, SetBitmapVramAddress)

/** Debug messages **/
NSTUB(   0x241C8, dm_names)
NSTUB(0xFFA604E8 - RAM_OFFSET, dm_set_store_level)
NSTUB(0xFFA60480 - RAM_OFFSET, dm_set_print_level)
NSTUB(    0x395C, DryosDebugMsg)

/** Task sleep **/
NSTUB(    0x67B4, msleep)

/** Semaphores **/
NSTUB(    0x6178, create_named_semaphore)
NSTUB(    0x62D8, take_semaphore)
NSTUB(    0x63C4, give_semaphore)

/** Recursive locks **/
NSTUB(   0x13810, CreateRecursiveLock)
NSTUB(   0x138A8, AcquireRecursiveLock)
NSTUB(   0x139BC, ReleaseRecursiveLock)

/** Message queues **/
NSTUB(   0x13460, msg_queue_create)
NSTUB(   0x1375C, msg_queue_post)
NSTUB(   0x13570, msg_queue_receive)
NSTUB(   0x1379C, msg_queue_count)

/** Tasks **/
NSTUB(    0x6868, task_create)
NSTUB(   0x232C4, task_dispatch_hook)
NSTUB(    0xBB80, task_trampoline)

/** Task info **/
NSTUB(   0x247C8, task_max)
NSTUB(    0x252C, get_current_task)
NSTUB(   0x1992C, is_taskid_valid)
NSTUB(    0xE68C, get_obj_attr)

/** Interrupts **/
NSTUB(0xFF0C10F4, cli_save)
NSTUB(0xFF0C1108, sei_restore)

/** Eventprocs (call by name) **/
NSTUB(0xFF138A34, call)

/** GUI **/
NSTUB(0xFF0D6E30, GUI_Control)
NSTUB(0xFF1AB548, SetGUIRequestMode)
NSTUB(0xFF0D6A5C, gui_main_task)
NSTUB(   0x236B0, gui_main_struct)
NSTUB(0xFF0D74A4, gui_init_end)
NSTUB(0xFF1C6ED0, gui_massive_event_loop)
NSTUB(   0x3D8A0, gui_task_list)
NSTUB(0xFF1B7580, GUI_SetRollingPitchingLevelStatus)
NSTUB(0xFF0D6FEC, GUI_ChangeMode)
NSTUB(0xFF1BFE58, GUI_GetFirmVersion)
//NSTUB(0xFF4B4844, gui_task_create)
//NSTUB(0xFF4B4970, gui_task_destroy)

/** Dialog API **/
NSTUB(0xFF4B6970, CreateDialogBox)
NSTUB(0xFF4B5938, DeleteDialogBox)
NSTUB(0xFF4B76AC, dialog_redraw)
NSTUB(0xFF523C90, dialog_set_property_str)
NSTUB(0xFF4D70E8, RedrawDisplay)

/** Dialog handlers **/
NSTUB(0xFF6E64B8, LiveViewShutterApp_handler)
NSTUB(0xFF52BE30, LiveViewApp_handler)
NSTUB(0xFF52CD54, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF52E1DC, LiveViewApp_handler_end)
NSTUB(0xFF3B98C4, PlayMain_handler)
NSTUB(0xFF546CDC, PlayMovieGuideApp_handler)
NSTUB(0xFF695354, ErrCardForLVApp_handler)
NSTUB(0xFF529930, HideUnaviFeedBack_maybe)
NSTUB(0xFF532B98, ShootOlcApp_handler)
NSTUB(0xFF5160D4, ErrForCamera_handler)

/** Properties **/
NSTUB(0xFF11F1C8, prop_register_slave)
NSTUB(0xFF11FA44, _prop_cleanup)
NSTUB(0xFF11F978, prop_deliver)
NSTUB(0xFF11F3BC, _prop_request_change)
NSTUB(0xFF11FBA0, prop_get_value)

/** Memory allocation **/
NSTUB(0xFF0C82E8, _malloc)
NSTUB(0xFF0C8340, _free) // see "can't malloc"
NSTUB(    0x76F8, _AllocateMemory)
NSTUB(    0x7A94, _FreeMemory)
NSTUB(   0x16E8C, _alloc_dma_memory)
NSTUB(   0x16EC0, _free_dma_memory)
NSTUB(    0x7498, GetMemoryInformation)
NSTUB(    0x7444, GetSizeOfMaxRegion)

/** ExMem **/
NSTUB(0xFF13C5D8, AllocateMemoryResource)
NSTUB(0xFF13C624, AllocateContinuousMemoryResource)
NSTUB(0xFF13C73C, FreeMemoryResource)
NSTUB(    0x8F28, GetFirstChunkFromSuite)
NSTUB(    0x96C4, GetNextMemoryChunk)
NSTUB(    0x8900, GetMemoryAddressOfMemoryChunk)
NSTUB(    0x88A0, GetRemainOfMemoryChunk)
//NSTUB(    0x88D0, GetSizeOfMemoryChunk)
//NSTUB(    0x8C08, GetSizeOfMemorySuite)
//NSTUB(    0x8CE8, GetNumberOfChunks)

/** VRAM **/
NSTUB(   0x6EDD8, vram_info)
NSTUB(0xFF3FA61C, vram_get_number)

/** Engio **/
NSTUB(0xFF2C2960, _engio_write)
NSTUB(0xFF2C26E4, shamem_read)
NSTUB(0xFF2C2648, _EngDrvOut)

/** PTP **/
NSTUB(0xFF4583C8, ptp_register_handler)
NSTUB(0xFF2A7108, ptpPropSetUILock)

/** Audio **/
NSTUB(0xFF337EB8, _audio_ic_read)
NSTUB(0xFF337870, _audio_ic_write)

NSTUB(0xFF10B254, sounddev_task )
NSTUB(   0x239F4, sounddev )
NSTUB(0xFF10B73C, sounddev_active_in)

NSTUB(0xFF10B73C, SoundDevActiveIn)
NSTUB(0xFF10B998, SoundDevActiveOut)
NSTUB(0xFF10B958, SoundDevShutDownIn)

NSTUB(0xFF7FCA00, audio_thresholds)

/** ASIF **/
NSTUB(0xFF13435C, PowerAudioOutput) 
NSTUB(0xFF1091A0, StartASIFDMADAC)  
NSTUB(0xFF108FE8, StartASIFDMAADC)  
NSTUB(0xFF109278, StopASIFDMADAC)   
NSTUB(0xFF10971C, SetNextASIFADCBuffer)
NSTUB(0xFF1097D0, SetNextASIFDACBuffer)
NSTUB(0xFF1337AC, SetSamplingRate )
NSTUB(0xFF1324C4, SetAudioVolumeOut) //keep volume

/** experimental EDMAC research **/
NSTUB(    0x8B7C, CreateMemorySuite)
NSTUB(    0x8974, DeleteMemorySuite)

/** EDMAC routines */
NSTUB(   0x11480, SetEDmac)
NSTUB(   0x11798, StartEDmac)
NSTUB(   0x1192C, PopEDmac)
NSTUB(   0x118E4, AbortEDmac)
NSTUB(   0x1152C, ConnectWriteEDmac)
NSTUB(   0x115F0, ConnectReadEDmac)

/** register/unregister CBR names **/
NSTUB(   0x11984, RegisterEDmacCompleteCBR)
NSTUB(   0x11A6C, RegisterEDmacAbortCBR)
NSTUB(   0x11B00, RegisterEDmacPopCBR)
NSTUB(   0x119C0, UnregisterEDmacCompleteCBR)
NSTUB(   0x11AA8, UnregisterEDmacAbortCBR)
NSTUB(   0x11B3C, UnregisterEDmacPopCBR)

/** PackMem **/
NSTUB(0xFF5F3354, PackMem_RegisterEDmacCompleteCBRForMemorySuite)
NSTUB(0xFF5F33F0, PackMem_RegisterEDmacPopCBRForMemorySuite)
NSTUB(0xFF5F3068, PackMem_SetEDmacForMemorySuite)
NSTUB(0xFF5F3284, PackMem_PopEDmacForMemorySuite)
NSTUB(0xFF5F317C, PackMem_StartEDmac)

/** ResLock */
NSTUB(0xFF2C67FC, CreateResLockEntry)
NSTUB(0xFF2C6CC4, LockEngineResources)
NSTUB(0xFF2C6E68, UnLockEngineResources)

/** DMA **/
NSTUB(0xA460, dma_memcpy)

/** MVR **/
NSTUB(0xFF2C191C, mvrSetPrintMovieLog)
NSTUB(0xFF2C132C, mvrSetQscale)
NSTUB(   0x314F4, mvr_config)
NSTUB(0xFF2C18F8, mvrFixQScale)
NSTUB(0xFF2C13F0, mvrSetDefQScale)

/** Storage **/
NSTUB(0xFF745CB4, cfReadBlk)
NSTUB(0xFF74C200, sdReadBlk)
NSTUB(0xFF74BF1C, sdWriteBlk)
NSTUB(   0x48FFC, sd_device)
NSTUB(0xFF62FA24, fsuDecodePartitionTable)

/** Bootdisk **/
NSTUB(0xFF1386C8, bootdisk_enable)
NSTUB(0xFF1386D4, bootdisk_disable)

/** Display **/
NSTUB(0xFF4D8C3C, NormalDisplay)
NSTUB(0xFF4D8C0C, ReverseDisplay)
NSTUB(0xFF4D8BDC, MirrorDisplay)
NSTUB(0xFF3330D8, touch_cbr_canon)
NSTUB(0xFF5AAB5C, lvae_setdispgain)
NSTUB(0xFF32AC9C, dispcheck)

/** Custom function numbers **/
NSTUB(0xFF674A08, GetCFnData)
NSTUB(0xFF674C20, SetCFnData)

/** Events **/
NSTUB(0xFF4B46F4, ctrlman_dispatch_event)
NSTUB(0x16C44, TryPostEvent)
NSTUB(0x16CA0, TryPostEvent_end)
NSTUB(0x16564, TryPostStageEvent)
NSTUB(0x165C0, TryPostStageEvent_end)

/** LCD Sensor **/
NSTUB(0xFF145E84, DispSensorStart)
