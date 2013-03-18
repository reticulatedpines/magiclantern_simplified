/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.5 firmware for the 1100d / Rebel T3.
 *
 * \todo Sort this file?  Generate it from the IDA map?
 */
/*
 * Copyright (C) 2011 Magic Lantern Team
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

NSTUB( ROMBASEADDR, firmware_entry )       //0xff010000

NSTUB(0xFF1E8B94, AcquireRecursiveLock)           // might be good (dumps=1, score=14)
NSTUB(0xFF07096C, _AllocateMemory)                 // might be good (dumps=1, score=1.7e+02)
NSTUB(0xff0707fc, AllocateMemory_do) // malloc struct at 0x2B48
NSTUB(0xFF31CAD8, CreateDialogBox)                // might be good (dumps=1, score=74)
NSTUB(0xFF073A7C, CreateRecursiveLock)            // might be good (dumps=1, score=5.6)
NSTUB(0xFF06C914, DryosDebugMsg)                       // might be good (dumps=1, score=1.1e+03)
NSTUB(0xFF06C914, DebugMsg)                       // might be good (dumps=1, score=1.1e+03)
NSTUB(0xFF31C270, DeleteDialogBox)                // might be good (dumps=1, score=47)
NSTUB(0xFF1D48C8, _EngDrvOut)                      // might be good (dumps=1, score=61)
NSTUB(0xFF1DA5F0, FIO_CleanupAfterFindNext_maybe) // might be good (dumps=1, score=69)
NSTUB(0xFF1D9620, FIO_CloseFile)                  // might be good (dumps=1, score=1e+02)
NSTUB(0xFF1D9E5C, FIO_CreateDirectory)            // might be good (dumps=1, score=13)
NSTUB(0xFF1D927C, FIO_CreateFile)                 // might be good (dumps=1, score=50)
NSTUB(0xFF1DA41C, FIO_FindFirstEx)                // might be good (dumps=1, score=50)
NSTUB(0xFF1DA510, FIO_FindNextEx)                 // might be good (dumps=1, score=50)
NSTUB(0xFF1D979C, FIO_GetFileSize)                // might be good (dumps=1, score=92)
NSTUB(0xFF1D91C0, FIO_Open)                       // might be good (dumps=1, score=92)
NSTUB(0xFF1D93D0, FIO_ReadFile)                   // might be good (dumps=1, score=87)
NSTUB(0xFF1D9328, FIO_RemoveFile)                 // might be good (dumps=1, score=53)
NSTUB(0xFF1D9570, FIO_WriteFile)                  // might be good (dumps=1, score=65)
NSTUB(0xFF1D9C94, FIO_RenameFile)
NSTUB(0xFF070CFC, FreeMemory)                     // might be good (dumps=1, score=1.6e+02)
NSTUB(0xFF020DFC, GUI_Control)                    // might be good (dumps=1, score=26)
NSTUB(0xFF223218, GUI_GetFirmVersion)             // might be good (dumps=1, score=11)
NSTUB(0xFF510C1C, GetCFnData)                     // might be good (dumps=1, score=19)
NSTUB(0xFF070710, GetMemoryInformation)           // might be good (dumps=1, score=6.4)
NSTUB(0xFF3714A4, HideUnaviFeedBack_maybe)        // might be good (dumps=1, score=1.2)
NSTUB(0xFF3712C8, HideBottomInfoDisp_maybe) // look for StartBottomInfoDispTimer
NSTUB(0x1C06C, LCD_Palette) // in InitializeBitmapDisplayDevice, right after 0xc0f14800
NSTUB(0xff2279C0, ChangeColorPalette )
NSTUB(0xff375f6c, LiveViewApp_handler)
NSTUB(0xff3765f4, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xFF377C44, LiveViewApp_handler_end)        // might be good (dumps=1, score=9.3)
NSTUB(0xFF0694E4, LoadCalendarFromRTC)            // might be good (dumps=1, score=30)
NSTUB(0xFF337C2C, MirrorDisplay)                  // might be good (dumps=1, score=3.4)
NSTUB(0xFF1DC754, MuteOff_0)                      // might be good (dumps=1, score=0.47)
NSTUB(0xFF1DC70C, MuteOn_0)                       // might be good (dumps=1, score=0.46)
NSTUB(0xFF337C8C, NormalDisplay)                  // might be good (dumps=1, score=3.4)
NSTUB(0xFF13ACC4, PtpPropButtonSW1)               // might be good (dumps=1, score=3.2)
NSTUB(0xFF13AD68, PtpPropButtonSW2)               // might be good (dumps=1, score=3.2)
NSTUB(0xFF1E8CA8, ReleaseRecursiveLock)           // might be good (dumps=1, score=14)
NSTUB(0xFF139A00, RemoteRelease)                  // might be good (dumps=1, score=3.8)
NSTUB(0xFF337C5C, ReverseDisplay)                 // might be good (dumps=1, score=3.4)
NSTUB(0xFF062AF4, SetBitmapVramAddress)           // might be good (dumps=1, score=8.2)
NSTUB(0xFF510E48, SetCFnData)                     // might be good (dumps=1, score=19)
NSTUB(0xFF0699F4, _audio_ic_read)                 // might be good (dumps=1, score=4.6)
NSTUB(0xFF069B38, _audio_ic_write)                // might be good (dumps=1, score=24)
NSTUB(0xFF05BBE4, _prop_cleanup)                  // might be good (dumps=1, score=42)
NSTUB( 0x833C, additional_version )
NSTUB(0xFF1EFE08, alloc_dma_memory)               // might be good (dumps=1, score=91)
NSTUB(0xff5a67e4, audio_thresholds)
NSTUB(   0x1D86C, bmp_vram_info)                  // no match
NSTUB(0xFF06C548, bootdisk_disable)               // might be good (dumps=1, score=1.3)
NSTUB(0xFF06C53C, bootdisk_enable)                // might be good (dumps=1, score=1.3)
NSTUB(0xFF530984, bzero32)                        // might be good (dumps=1, score=1.5e+02)
NSTUB(0xFF06D7DC, call)                           // might be good (dumps=1, score=1.3e+02)
NSTUB( 0xC0220000, camera_engine )
NSTUB(0xFF4C2CD8, cfReadBlk)                      // might be good (dumps=1, score=9.1)
NSTUB( 0x1553C, cf_device )                // in cfReadBlk
NSTUB(0xFF010500, cli_save)                       // might be good (dumps=1, score=0.32)
NSTUB(0xFF017F80, create_init_task)               // might be good (dumps=1, score=3.1)
NSTUB(0xFF06F40C, create_named_semaphore)         // might be good (dumps=1, score=81)
NSTUB(0xFF011028, cstart)                         // might be good (dumps=1, score=1.6)
NSTUB(0xFF31B4A0, ctrlman_dispatch_event)         // might be good (dumps=1, score=10)
NSTUB(0xFF31D3EC, dialog_redraw)                  // might be good (dumps=1, score=78)
NSTUB(0xFF2277E8, dialog_set_property_str)        // might be good (dumps=1, score=54)
NSTUB(0xFF1DC10C, dispcheck)                      // might be good (dumps=1, score=6)
NSTUB( 0x28E4, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB(0xFF06CC74, dm_set_store_level)             // might be good (dumps=1, score=15)
NSTUB(0xFF06C894, dmstart)                        // might be good (dumps=1, score=7.9)
NSTUB(0xFF06C8D4, dmstop)                         // might be good (dumps=1, score=7.9)
NSTUB(0xFF06D2B4, dumpf)                          // might be good (dumps=1, score=3.9)
NSTUB(0xFF1D4C8C, _engio_write)                    // might be good (dumps=1, score=77)
NSTUB(0xFF018D8C, free)                           // might be good (dumps=1, score=14)
NSTUB(0xFF1EFE3C, free_dma_memory)                // might be good (dumps=1, score=80)
NSTUB(0xFF41686C, fsuDecodePartitionTable)        // might be good (dumps=1, score=2.1)
NSTUB(0xFF0175F4, get_current_task)               // might be good (dumps=1, score=11)
NSTUB(0xFF06F654, give_semaphore)                 // might be good (dumps=1, score=1e+02)
NSTUB(0xFF0A7420, gui_change_mode)                // might be good (dumps=1, score=10)
NSTUB(0xFF02144C, gui_init_end)                   // might be good (dumps=1, score=4.2)
NSTUB(0xFF0A7B90, gui_local_post)                 // might be good (dumps=1, score=3.2)
NSTUB(0xFF139910, gui_lock)                       // might be good (dumps=1, score=23)
NSTUB( 0x1BF4, gui_main_struct )      //  in gui_main_task
NSTUB(0xFF020A40, gui_main_task)                  // might be good (dumps=1, score=5.5)
NSTUB(0xFF0A8128, gui_other_post)                 // might be good (dumps=1, score=0.99)
NSTUB(0xFF31B5F0, gui_task_create)                // might be good (dumps=1, score=9.4)
NSTUB(0xFF31B710, gui_task_destroy)               // might be good (dumps=1, score=18)
NSTUB(0xAF6C, gui_task_list)
NSTUB( 0x3AC8, gui_timer_struct )     // in GUI_Control_Post
NSTUB( 0x1A7C, hotplug_struct )
NSTUB(0xFF01B2CC, hotplug_task)                   // might be good (dumps=1, score=11)
NSTUB( 0x1A8C, hotplug_usb_buf )          // after "USB Disconnect" 
NSTUB(0xFF0197D8, init_task)                      // might be good (dumps=1, score=10)
NSTUB(0x736b4, lv_path_struct) // first arg of SelectPathDriveMode
NSTUB(0xFF018D34, malloc)                         // might be good (dumps=1, score=11)
NSTUB(0xFF073EB0, dma_memcpy)                     // Look for [HPC ERROR] Nothing DMA ch now!
NSTUB(0xFF1E885C, msg_queue_receive)              // might be good (dumps=1, score=24)
NSTUB(0xff1e8a48, msg_queue_post)
NSTUB(0xFF06FA40, msleep)                         // might be good (dumps=1, score=1.1e+02)
NSTUB(0xFF1B9544, mvrFixQScale)                   // might be good (dumps=1, score=4.8)
NSTUB(0xFF1B9020, mvrSetDefQScale)                // might be good (dumps=1, score=4.7)
NSTUB(0xFF1B9058, mvrSetFullHDOptSize)            // might be good (dumps=1, score=4.9)
NSTUB(0xFF1B9248, mvrSetGopOptSizeFULLHD)         // might be good (dumps=1, score=4.7)
NSTUB(0xFF1B92CC, mvrSetGopOptSizeHD)             // might be good (dumps=1, score=4.7)
NSTUB(0xFF1B9344, mvrSetGopOptSizeVGA)            // might be good (dumps=1, score=4.7)
NSTUB(0xFF1B91B8, mvrSetHDOptSize)                // might be good (dumps=1, score=4.9)
NSTUB(0xFF1B9564, mvrSetPrintMovieLog)            // might be good (dumps=1, score=4.8)
NSTUB(0xFF1B8F50, mvrSetQscale)                   // might be good (dumps=1, score=4.7)
NSTUB(0xFF1B9200, mvrSetVGAOptSize)               // might be good (dumps=1, score=4.9)
NSTUB( 0x5B4C, mvr_config )
NSTUB(0xFF06E260, oneshot_timer)                  // might be good (dumps=1, score=31)
NSTUB(0xFF05BA3C, prop_deliver)                   // might be good (dumps=1, score=44)
NSTUB(0xFF05BC7C, prop_get_value)                 // might be good (dumps=1, score=62)
NSTUB(0xFF05BAC4, prop_register_slave)            // might be good (dumps=1, score=39)
NSTUB(0xFF05B45C, _prop_request_change)            // might be good (dumps=1, score=1.7e+02)
NSTUB(0xFF223844, prop_request_icu_auto_poweroff) // might be good (dumps=1, score=68)
NSTUB(0xFF2A35D4, ptp_register_handler)           // might be good (dumps=1, score=38)
NSTUB(0xFF1EDA10, register_func)                  // Good. Found matches: ff1eda10/1.2e+02, ff1eda20/6.2
NSTUB(0xFF4C8A1C, sdReadBlk)                      // might be good (dumps=1, score=9.2)
NSTUB( 0x15560, sd_device )                // in sdReadBlk
NSTUB(0xFF010514, sei_restore)                    // might be good (dumps=1, score=0.22)
NSTUB(0xFF1D47B8, shamem_read)                    // might be good (dumps=1, score=10)
NSTUB( 0x1E58, sounddev )                    // in sounddev_task
NSTUB(0xff0579dc, sounddev_active_in)
NSTUB(0xFF057570, sounddev_task)                  // might be good (dumps=1, score=8.2)
NSTUB(0xFF06F56C, take_semaphore)                 // might be good (dumps=1, score=1.5e+02)
NSTUB(0xFF06FAF4, task_create)                    // might be good (dumps=1, score=23)
NSTUB( 0x193C, task_dispatch_hook )
NSTUB(0xFF07C970, task_trampoline)                // might be good (dumps=1, score=0.5)
NSTUB(0xFF2E0164, vram_get_number)                // might be good (dumps=1, score=12)
NSTUB(0xFF1DAB90, vram_get_pos_and_size)
NSTUB( 0x76268, vram_info )             // in vram_get_number
NSTUB(0xFF1EBFE0, vsnprintf)                      // might be good (dumps=1, score=9.5)

NSTUB(0xff383ac8, ShootOlcApp_handler)
NSTUB(0xFF45596C, ErrCardForLVApp_handler)
//NSTUB(0xff35abc4, ErrCardApp_handler) Unused
NSTUB(0xFF35B728, ErrForCamera_handler) // ERR70, ERR80 etc (DlgErrForCamera.c, AJ_DIALOG.HANDLER_DlgErrForCamera.c)

NSTUB(0xFF1E874C, msg_queue_create)

NSTUB(0xFF039518, PD_RemoteRelease)

// for task information
NSTUB(0x2e00, task_max)
NSTUB(0xFF34E544, is_taskid_valid) // AJ_task_trampoline_related_p10
NSTUB(0xff07f448, get_obj_attr) // AJ_checks_if_Process_id_created_by_Dryos

NSTUB(0xff0a3434, SetGUIRequestMode)

NSTUB(0xff074b1c, AllocateMemoryResource) // m_pfAllocMemoryCBR
NSTUB(0xff074b68, AllocateContinuousMemoryResource) // m_pfContAllocMemoryCBR
NSTUB(0xff074c80, FreeMemoryResource) // m_pfFreeMemoryCBR
NSTUB(0xff072114, GetFirstChunkFromSuite) // AJ_PackMemory_PackMem_p3
NSTUB(0xff071b5c, GetMemoryAddressOfMemoryChunk) // AJ_PackMemory_PackMem.c_IsChunkSignature

NSTUB(0xFF069E9C, PowerAudioOutput)
NSTUB(0xff055678, StartASIFDMADAC)
NSTUB(0xFF0554C4, StartASIFDMAADC)
NSTUB(0xff055c00, SetNextASIFADCBuffer)
NSTUB(0xff055cb8, SetNextASIFDACBuffer)
NSTUB(0xff05574c, StopASIFDMADAC)
NSTUB(0xff069f88, SetSamplingRate )
NSTUB(0xff069d88, SetAudioVolumeOut)
NSTUB(0xff055c00, SetNextASIFADCBuffer)
NSTUB(0xff055cb8, SetNextASIFDACBuffer)
NSTUB(0xff38f290, PlayMain_handler)
NSTUB(0xff48b9d4, PlayMovieGuideApp_handler)


NSTUB(0xFF1E8630, register_interrupt)
NSTUB(0xFF1CACFC, MREQ_ISR)
NSTUB(0xFF1CAD8C, SIO3_ISR)

NSTUB(0xFF10DC10, GetJpegBufForLV)  // Actually FA_GetJpegBufForLV check ROM FA_ routines to find it
NSTUB(0xFF10DC1C, GetJpegSizeForLV) // Actually FA_GetJpegSizeForLV (see above)
