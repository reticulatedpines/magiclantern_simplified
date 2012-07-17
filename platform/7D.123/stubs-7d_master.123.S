/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.2.3 master firmware for the 7d.
 *
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

NSTUB( ROMBASEADDR, firmware_entry )           // 0xff810000

/** These must be found first for any progress to be made */
NSTUB( 0xFF810FB0, cstart )
NSTUB( 0xFF956C48, bzero32 ) // called by cstart()
NSTUB( 0xFF812B4C, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF815274, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF834154, DebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x1934, task_dispatch_hook )                 

/** Find the additional version string in GUI_GetFirmVersion */
// NSTUB( 0x170CC, additional_version )
// NSTUB( 0xFF21048C, GUI_GetFirmVersion )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF834ED4, call )

/** Find the shell register commands */
NSTUB( 0xFF8DA1DC, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF8340D4, dmstart )
NSTUB( 0xFF834114, dmstop )
NSTUB( 0xFF817ADC, dumpf )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF8D8B54, vsnprintf )

NSTUB( 0xFF838248, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF8382FC, task_create )
NSTUB( 0xFF826908, FIO_Open)                 // FIO_OpenFile
NSTUB( 0xFF82660C, FIO_CreateFile )
NSTUB( 0xFF8264AC, FIO_CloseFile )
NSTUB( 0xFF8263A0, FIO_GetFileSize )
NSTUB( 0xFF826560, FIO_WriteFile )
NSTUB( 0xFF82685C, FIO_ReadFile )
// NSTUB( 0xFF1CDF90, FIO_RemoveFile )
	
NSTUB( 0xFF8D5A38, give_semaphore )
NSTUB( 0xFF8D5950, take_semaphore )
NSTUB( 0xFF8344A0, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF82FC14, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF82F5AC, prop_request_change )
NSTUB( 0xFF82FB8C, prop_deliver ) 
NSTUB( 0xFF82FDC8, prop_get_value )
	
// NSTUB( 0xFF068250, dispcheck )

NSTUB( 0xFF833DBC, bootdisk_enable )
NSTUB( 0xFF833DC8, bootdisk_disable )


 NSTUB( 0xFF068660, SetBitmapVramAddress ) // where to find bmp_vram_info
 NSTUB( 0x294BC, bmp_vram_info )
 NSTUB( 0x7DCE4, vram_info )
 NSTUB( 0xFF289270, vram_get_number )
 NSTUB( 0xFF1CF6A4, vram_get_pos_and_size )

 NSTUB( 0xFF01620C, malloc )
 NSTUB( 0xFF016268, free )
 NSTUB( 0xFF074F88, AllocateMemory ) 
 NSTUB( 0xFF075318, FreeMemory ) 
	
 NSTUB( 0x2AEC, dm_names )                // in DebugMsg, before the 1st loop target
 NSTUB( 0xFF1E0EC8, strcpy )
 NSTUB( 0xFF06DE8C, LoadCalendarFromRTC )
NSTUB( 0xFF8417A0, task_trampoline )

 NSTUB( 0xFF061A98, prop_cleanup )

 NSTUB( 0xFF06C4B0, _audio_ic_write )
 NSTUB( 0xFF06C2E0, _audio_ic_read )
 NSTUB( 0xFF05346C, sounddev_task )      // look for "_SoundDeviceTask"
 NSTUB( 0xFF05DC68, sounddev_active_in )
 NSTUB( 0xFF4C466C, audio_thresholds )        // not found
 NSTUB( 0x1F10, sounddev )                    // in sounddev_task

 NSTUB( 0xFF073A28, create_named_semaphore )
 NSTUB( 0xFF2B2080, gui_task_create )
 NSTUB( 0xFF2B21A0, gui_task_destroy )
 NSTUB( 0xFF2B1F30, ctrlman_dispatch_event )

 NSTUB( 0xFF498298, memcpy )
 NSTUB( 0xFF0A2910, prop_request_icu_auto_poweroff )
 NSTUB( 0xFF1DF650, alloc_dma_memory )
 NSTUB( 0xFF1DF684, free_dma_memory )

 NSTUB( 0xFF3EA8D8, cfReadBlk )
 NSTUB( 0x22650, cf_device )                // in cfReadBlk
 NSTUB( 0xFF3F019C, sdReadBlk )
 NSTUB( 0x22680, sd_device )                // in sdReadBlk

// NSTUB( 0xFF01A734, hotplug_task )        // FF01E7C0 hotplug_task, but do not handle USB
// NSTUB( 0x1A6C, hotplug_struct )
// NSTUB( 0x1AA0, hotplug_usb_buf )           


 NSTUB( 0xFF0104BC, cli_save )
 NSTUB( 0xFF0104D0, sei_restore )
 NSTUB( 0xFF2518F8, ptp_register_handler )
 NSTUB( 0xFF14AEE8, gui_lock )             // PtpPropSetUILock
 NSTUB( 0xFF0710D0, oneshot_timer )
// NSTUB( 0xFF2B4D30, gui_task_destroy )
 NSTUB( 0xFF020BB4, gui_main_task )       //  uses a 8 functions table compared to 5d2 204
 NSTUB( 0xFF0216F0, gui_init_end )     // EndGuiInit
 NSTUB( 0x39CC, gui_timer_struct )     // in GUI_Control_Post
 NSTUB( 0x1BE8, gui_main_struct )      //  in gui_main_task
 NSTUB( 0xFF1D7870, msg_queue_receive )
 NSTUB( 0xFF0A0404, gui_local_post )     // GUI_Local_Post
 NSTUB( 0xFF09FE08, gui_change_mode )   // GUI_ChangeMode_Post
 NSTUB( 0xFF0A07E4, gui_other_post )     // GUI_Other_Post
 
 NSTUB( 0xFF49C92C, gui_main_task_functbl )  // 8 functions. queue field at offset 0x38

 NSTUB( 0xFF1B3070, mvrFixQScale )
 NSTUB( 0xFF1B2B4C, mvrSetDefQScale )


#if 0   // not checked yet in 1.0.9
 NSTUB( 0xFF0959A4, gui_post_10000085 ) // should be called post 100000a5 no ?
 NSTUB( 0xFF096208, gui_init_event )
 NSTUB( 0xFF095A30, gui_change_shoot_type_post )
 NSTUB( 0xFF095AC8, gui_change_lcd_state_post )
 NSTUB( 0xFF093108, gui_massive_event_loop )   // GUI_Control_Post
NSTUB( 0xFF07396C, gui_timer_something )
#endif
