/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.4 firmware for the 1100d / Rebel T3.
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
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )       //0xff010000

/** These must be found first for any progress to be made */
NSTUB( 0xFF011028, cstart )
NSTUB( 0xFF53098C, bzero32 ) // called by cstart()
NSTUB( 0xFF0197E0, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017F88, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF06C91C, DebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x193C, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x833C, additional_version )
NSTUB( 0xFF223220, GUI_GetFirmVersion )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF06D7E4, call )

/** Find the shell register commands */
NSTUB( 0xff1eda18, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF06C89C, dmstart )
NSTUB( 0xFF06C8DC, dmstop )
NSTUB( 0xFF06D2BC, dumpf )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF1EBFE8, vsnprintf )

NSTUB( 0xFF06FA48, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF06FAFC, task_create )
NSTUB( 0xFF1D91C8, FIO_Open)
NSTUB( 0xFF1D9284, FIO_CreateFile )
NSTUB( 0xFF1D9628, FIO_CloseFile )
NSTUB( 0xFF1D97A4, FIO_GetFileSize )
NSTUB( 0xFF1D9578, FIO_WriteFile )
NSTUB( 0xFF1D93D8, FIO_ReadFile )
NSTUB( 0xff1d9330, FIO_RemoveFile )
NSTUB( 0xFF1DA424, FIO_FindFirstEx )
NSTUB( 0xFF1DA518, FIO_FindNextEx )
NSTUB( 0xFF1D9E64, FIO_CreateDirectory)

NSTUB( 0xFF06F65C, give_semaphore )
NSTUB( 0xFF06F574, take_semaphore )
NSTUB( 0xFF06CC7C, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF05BACC, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF05B464, prop_request_change )
NSTUB( 0xFF05BA44, prop_deliver ) 
NSTUB( 0xFF05BBEC, _prop_cleanup )
NSTUB( 0xFF05BC84, prop_get_value )

NSTUB( 0xFF1DC114, dispcheck )
NSTUB( 0xFF062AFC, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x1D86C, bmp_vram_info )
NSTUB( 0x76268, vram_info )             // in vram_get_number
NSTUB( 0xFF2E016C, vram_get_number )
NSTUB( 0xFF1C7EA8, vram_get_pos_and_size )

NSTUB( 0xFF018D3C, malloc )
NSTUB( 0xFF018D94, free )
NSTUB( 0xff070974, AllocateMemory )
NSTUB( 0xff070d04, FreeMemory )

NSTUB( 0x28E4, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF201F10, strcpy )
NSTUB( 0xFF0694EC, LoadCalendarFromRTC )
NSTUB( 0xFF07C978, task_trampoline )
                           
NSTUB( 0xFF069B40, _audio_ic_write )
NSTUB( 0xFF0699FC, _audio_ic_read )
NSTUB( 0xFF057578, sounddev_task )
NSTUB( 0xFF057A64, sounddev_active_in )
NSTUB( 0xFF4C466C, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
// ROM:FF1AC348 check_audio_lvl_maybe         550d 108
// ROM:FF1BA638 check_audio_lvl_maybe         1100d 104, where to find audio_thresholds
NSTUB( 0x1E58, sounddev )                    // in sounddev_task

NSTUB( 0xFF06F414, create_named_semaphore )
NSTUB( 0xFF31B5F8, gui_task_create )
NSTUB( 0xFF31B718, gui_task_destroy )
NSTUB( 0xFF31B4A8, ctrlman_dispatch_event )

NSTUB( 0xFF530898, memcpy )
NSTUB( 0xFF22384C, prop_request_icu_auto_poweroff )
NSTUB( 0xFF1EFE10, alloc_dma_memory )
NSTUB( 0xFF1EFE44, free_dma_memory )

NSTUB( 0xFF4C2CE0, cfReadBlk )
NSTUB( 0x1553C, cf_device )                // in cfReadBlk
NSTUB( 0xFF4C8A24, sdReadBlk )
NSTUB( 0x15560, sd_device )                // in sdReadBlk

NSTUB( 0xFF01B2D4, hotplug_task )
NSTUB( 0x1A7C, hotplug_struct )
NSTUB( 0x1A8C, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xFF06C544, bootdisk_enable )
NSTUB( 0xFF06C550, bootdisk_disable )

NSTUB( 0xFF010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xFF2A35DC, ptp_register_handler )
NSTUB( 0xFF139918, gui_lock )             // PtpPropSetUILock
NSTUB( 0xFF06E268, oneshot_timer )
// NSTUB( 0xFF31B718, gui_task_destroy )   // already defined above
NSTUB( 0xFF020A48, gui_main_task )       //  uses a 8 functions table 
// func_table FF536110
NSTUB( 0xFF021454, gui_init_end )     // EndGuiInit
NSTUB( 0x3AC8, gui_timer_struct )     // in GUI_Control_Post
NSTUB( 0x1BF4, gui_main_struct )      //  in gui_main_task
// queue_handle at offset 0x30, counter at offset 0xC
NSTUB( 0xFF1E8864, msg_queue_receive )
NSTUB( 0xFF0A7B98, gui_local_post )    // GUI_Local_Post
NSTUB( 0xFF0A7428, gui_change_mode )   // GUI_ChangeMode_Post
NSTUB( 0xFF0A8130, gui_other_post )   // GUI_Other_Post
NSTUB( 0xFF020E04, GUI_Control )

NSTUB(0xAF6C, gui_task_list)

NSTUB( 0x5B4C, mvr_config )
NSTUB(0xff1b954c, mvrFixQScale)
//NSTUB(mvrSetBitRate, 0xff04e7e8)
NSTUB(0xff1b9028, mvrSetDefQScale)
	/* finsig
NSTUB(mvrSetFullHDOptSize, 0xff1b9060)
NSTUB(mvrSetGopOptSizeFULLHD, 0xff1b9250)
NSTUB(mvrSetGopOptSizeHD, 0xff1b92d4)
NSTUB(mvrSetGopOptSizeVGA, 0xff1b934c)
NSTUB(mvrSetHDOptSize, 0xff1b91c0)
NSTUB(mvrSetPrintMovieLog, 0xff1b956c)
NSTUB(mvrSetQscale, 0xff1b8f58)
NSTUB(mvrSetVGAOptSize, 0xff1b9208)
*/

//NSTUB( 0xFF347830, AJ_guess_LED_ON )   // led for SD/CF read and write ops
//NSTUB( 0xFF347800, AJ_guess_LED_OFF )
NSTUB(0xFF073A84, CreateRecursiveLock)
NSTUB( 0xFF337C34, MirrorDisplay )
NSTUB( 0xFF337C94, NormalDisplay )

#if 0   // not checked yet in 1.0.9
NSTUB( 0xFF0959A4, gui_post_10000085 ) // should be called post 100000a5 no ?
NSTUB( 0xFF096208, gui_init_event )
NSTUB( 0xFF095A30, gui_change_shoot_type_post )
NSTUB( 0xFF095AC8, gui_change_lcd_state_post )
 NSTUB( 0xFF093108, gui_massive_event_loop )   // GUI_Control_Post
NSTUB( 0xFF1D2760, gui_timer_something )
#endif

// From 550D possible-code-matches.txt
NSTUB(0xFF1E8B9C, AcquireRecursiveLock) // (AJ_KernelDry_KerRLock.c) (mean=7, stdev=9.8, num=17)
NSTUB(0xFF1E8CB0, ReleaseRecursiveLock) // (AJ_KernelDry_KerRLock.c_p2) (mean=6.2, stdev=9.5, num=16)
NSTUB(0xFF1DA5F8, FIO_CleanupAfterFindNext_maybe) //  (AJ__switch_0x1A50_n_calls_fstOpenDir) (mean=25, stdev=10, num=11)
NSTUB(0xFF337C64, ReverseDisplay) //  (Reverse) (mean=0.89, stdev=0, num=1)
NSTUB(0xFF31D3F4, dialog_redraw) // (dialog_draw) (mean=19, stdev=3.9, num=68)

NSTUB(0xff1dc714, MuteOn_0)
NSTUB(0xff1dc75c, MuteOff_0)

NSTUB(0xFF139A08, RemoteRelease)
NSTUB(0xFF13ACCC, PtpPropButtonSW1)
NSTUB(0xFF13AD70, PtpPropButtonSW2)

NSTUB(0xFF070718, GetMemoryInformation) // called from AllocateMemory

NSTUB(0xFF510C24, GetCFnData)
NSTUB(0xFF510E50, SetCFnData)

NSTUB(0xff31cae0, CreateDialogBox)
NSTUB(0xff31c278, DeleteDialogBox)
NSTUB(0xff2277f0, dialog_set_property_str)

NSTUB(0xff416874, fsuDecodePartitionTable)
NSTUB(0xFF300BA0, LiveViewApp_handler)
NSTUB(0xff3765fc, LiveViewApp_handler_BL_JudgeBottomInfoDispTimerState)
NSTUB(0xff377c4c, LiveViewApp_handler_end) // start of next function
NSTUB(0xff3714ac, HideUnaviFeedBack_maybe) // look for StartUnaviFeedBackTimer

NSTUB(0x736b4, lv_path_struct) // first arg of SelectPathDriveMode

NSTUB(0xff0175fc, get_current_task)
