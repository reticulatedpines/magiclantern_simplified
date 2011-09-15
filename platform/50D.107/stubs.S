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

/** These must be found first for any progress to be made */
NSTUB( 0xFF810894, cstart )
NSTUB( 0xFFC099CC, bzero32 ) // called by cstart()
NSTUB( 0xFF811DBC, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF81724C, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF863AF8, DryosDebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1934, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0xFDDC, additional_version )
NSTUB( 0xFF9ABF80, GUI_GetFirmVersion )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF864874, call )

/** Find the shell register commands */
NSTUB( 0xFF98F8F8, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF863A78, dmstart )
NSTUB( 0xFF863AB8, dmstop )
NSTUB( 0xFF86438C, dumpf )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF98E170, vsnprintf )

NSTUB( 0xFF866DF0, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF866EA4, task_create )
NSTUB( 0xFF9823C0, FIO_Open)
NSTUB( 0xFF982474, FIO_CreateFile )
NSTUB( 0xFF98251C, FIO_CloseFile )
NSTUB( 0xFF982B10, FIO_GetFileSize )
NSTUB( 0xFF98295C, FIO_WriteFile )
NSTUB( 0xFF9827AC, FIO_ReadFile )
NSTUB( 0xFF982690, FIO_RemoveFile )  // not found by finsig. look for "SETUP_WriteFile: RemoveFile ERROR(%s)"
	
NSTUB( 0xFF866A04, give_semaphore )
NSTUB( 0xFF86691C, take_semaphore )
NSTUB( 0xFF863E44, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF859774, prop_register_slave ) // called by "GUI_RegisterPropertySlave"
NSTUB( 0xFF8598AC, prop_request_change )
NSTUB( 0xFF8596EC, prop_deliver ) 
NSTUB( 0xFF8599C8, prop_get_value )
	
NSTUB( 0xFF85ED14, dispcheck )
NSTUB( 0xFF85F0FC, SetBitmapVramAddress ) // where to find bmp_vram_info. not found by finsig
NSTUB( 0x24124, bmp_vram_info )
NSTUB( 0x361B8, vram_info )
NSTUB( 0xFFA1D554, vram_get_number )      // GetVramNumber. has vram_info in r2
NSTUB( 0xFF983B24, vram_get_pos_and_size )

NSTUB( 0xFF818CDC, malloc ) // maybe it's better to use Allocate/Free?
NSTUB( 0xFF818DB0, free )        // to be checked
NSTUB( 0xFF867B3C, AllocateMemory ) // called by alloc_dma_memory
NSTUB( 0xFF867D00, FreeMemory ) 

NSTUB( 0x2C48, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF8751A4, strcpy )
NSTUB( 0xFF863334, LoadCalendarFromRTC )
NSTUB( 0xFF86F978, task_trampoline )

NSTUB( 0xFF859848, prop_cleanup )

NSTUB( 0xFF854124, _audio_ic_write )
NSTUB( 0xFF854A70, _audio_ic_read )
NSTUB( 0xFF855760, sounddev_task )        // look for "SoundDevice_CODEC.c"
NSTUB( 0xFF855B50, sounddev_active_in )  // look for "SoundDevActiveIn"
NSTUB( 0xFFC48500, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
NSTUB( 0x2348, sounddev )                    // in sounddev_task

NSTUB( 0xFF8667BC, create_named_semaphore )
NSTUB( 0xFFA45F20, gui_task_create )
NSTUB( 0xFFA46040, gui_task_destroy )
NSTUB( 0xFFA45DD0, ctrlman_dispatch_event )

NSTUB( 0xFFC098D8, memcpy )
NSTUB( 0xFF88B7AC, prop_request_icu_auto_poweroff )  // finsig was wrong = 0xff88b74c
NSTUB( 0xFF1DE6F0, alloc_dma_memory )
NSTUB( 0xFF990FD8, free_dma_memory )

NSTUB( 0xFFB624EC, cfReadBlk )
NSTUB( 0x1D59C, cf_device )                // in cfReadBlk
NSTUB( FFB6206C, cfWriteBlk )
NSTUB( 0xFFB6E888, sdReadBlk )
NSTUB( 0x1D5E4, sd_device )                // in sdReadBlk
NSTUB( 0xFFB6E0C8, sdWriteBlk)

NSTUB( 0xFF81D9C0, hotplug_task )
NSTUB( 0x1AF0, hotplug_struct )
NSTUB( 0x1B28, hotplug_usb_buf )          // after "USB Disconnect". r1 for prop_deliver 

NSTUB( 0xFF863760, bootdisk_enable )
NSTUB( 0xFF86376C, bootdisk_disable )

NSTUB( 0xFF812E44, cli_save )
NSTUB( 0xFF812E58, sei_restore )
NSTUB( 0xFF9ED814, ptp_register_handler )
NSTUB( 0xFF90E6F0, gui_lock )             // PtpPropSetUILock
NSTUB( 0xFF8652B4, oneshot_timer )
NSTUB( 0xFF8225D8, gui_main_task )       //  queue at offset 0x34 of gui_main_struct. (LDR     R0, [R5,#0x34])
 NSTUB( 0xFF2B4D30, gui_task_destroy )
NSTUB( 0xFF822F88, gui_init_end )     // EndGuiInit
NSTUB( 0x3960, gui_timer_struct )     
NSTUB( 0x1C50, gui_main_struct )      //  in gui_main_task. use r5
NSTUB( 0xFF86651C, msg_queue_receive )       
NSTUB( 0xFF8898B4, gui_local_post )    // GUI_Local_Post
NSTUB( 0xFF889340, gui_change_mode )   // GUI_ChangeMode_Post
NSTUB( 0xFF889C90, gui_other_post )   // GUI_Other_Post

NSTUB( 0xff9787f8, mvrFixQScale )
NSTUB( 0xff978614, mvrSetDefQScale )
//NSTUB( 0xFF84D874, mvrSetBitRate )
NSTUB( 0xff978818, mvrSetPrintMovieLog )
NSTUB( 0xff978658, mvrSetFullHDOptSize )
//NSTUB( 0xFF1AA638, mvrSetHDOptSize )
NSTUB( 0xff978794, mvrSetVGAOptSize )
//NSTUB( 0xFF1AA6C8, mvrSetGopOptSizeFULLHD )
//NSTUB( 0xFF1AA74C, mvrSetGopOptSizeHD )
//NSTUB( 0xFF1AA7C4, mvrSetGopOptSizeVGA )
NSTUB(0xff9785c0, mvrSetDeblockingFilter)

//NSTUB( 0xFF983400, FIO_FindFirstEx ) // these are present, but difficult to find
//NSTUB( 0xFF98362C, FIO_FindNextEx ) 

NSTUB( 0x3787c, LV_EX_X)
NSTUB( 0x37880, LV_EX_Y)

//NSTUB( 0xFF020450, GUI_ChangeMode )
//NSTUB( 0xFF020350, GUI_Control)

NSTUB( 0xFFA712C0, AJ_guess_LED_ON )       // used by FIO_OpenFile and FIO_CreateFile
NSTUB( 0xFFA712E0, AJ_guess_LED_OFF )      // used by FIO_OpenFile and FIO_CreateFile

//NSTUB( 0xFF1A1CA4, SleepLiveViewDevice )
//NSTUB( 0xFF1A16D0, ReadyLiveViewDevice )
NSTUB( 0xFF8765D0, DispSensorStart )

NSTUB( 0xFF887FF4, gui_massive_event_loop )   // GUI_Control_Post
//NSTUB( 0xFF020760, GUI_CONTROL )

NSTUB(0xffabb76c, ChangeColorPalette)

//NSTUB( 0xFF2CFA54, MirrorDisplay )
//NSTUB( 0xFF2CFAB4, NormalDisplay )
NSTUB(0xffa6f6c4, RedrawDisplay) // like in 500D and 5D2, not sure if it can be called in the same way

NSTUB( 0xFF885200, strlen )
NSTUB(0xff8851a8, strcmp)

 //NSTUB( 0xFF44f8f8, AJ_memset_R0_addr_R1_Len_R2_byteVal ) // bzero32+4

//NSTUB( 0x67bc, mvr_config )

NSTUB( 0xFF9BD9E8, GuiEdLedOn )
NSTUB( 0xFF9BDA04, GuiEdLedOff )
NSTUB( 0xFF9BDA28, GuiEdLedBlink )

NSTUB(0xff830e60, main_ctrl_task):
//NSTUB( 0xFF0364A0, LVCAF_LensDriveStart )

NSTUB(0xffa95594, ChangeHDMIOutputSizeToVGA) // not called
NSTUB(0xffa96614, ChangeHDMIOutputSizeToFULLHD) // not called

NSTUB(0xff876700, msg_queue_post) // not sure

 // NSTUB(0xFF03B8EC, PD_GetBatteryPower) not safe to call


NSTUB(0xff87aef8, CreateRecursiveLock)
NSTUB(0xff99acb4, AcquireRecursiveLock)
NSTUB(0xff99adc8, ReleaseRecursiveLock) // not sure, these are not called anywhere
