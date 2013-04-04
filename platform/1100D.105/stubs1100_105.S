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

/** These must be found first for any progress to be made */
NSTUB( 0xFF011028, cstart )
NSTUB( 0xFF530984, bzero32 ) // called by cstart()
NSTUB( 0xFF0197D8, init_task ) // passed as arg to create_init_task, look for dmSetup
NSTUB( 0xFF017F80, create_init_task ) 

/** Look for the normal printf strings */
NSTUB( 0xFF06C914, DebugMsg )

/** Task dispatch hook hasn't moved in several versions */
NSTUB( 0x193C, task_dispatch_hook )

/** Find the additional version string in GUI_GetFirmVersion */
NSTUB( 0x833C, additional_version )
NSTUB( 0xFF223218, GUI_GetFirmVersion )

// Everything below here is not needed for the first boot

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

NSTUB( 0xFF06D7DC, call )

/** Find the shell register commands */
NSTUB( 0xFF1EDA10, register_func )

/** Debugging commands are all registered by name.  Search for the strings */
NSTUB( 0xFF06C894, dmstart )
NSTUB( 0xFF06C8D4, dmstop )
NSTUB( 0xFF06D2B4, dumpf )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF1EBFE0, vsnprintf )

NSTUB( 0xFF06FA40, msleep ) // called from EV_GETDONE_CACHE param 0x64
NSTUB( 0xFF06FAF4, task_create )
NSTUB( 0xFF1D91C0, FIO_Open)
NSTUB( 0xFF1D927C, FIO_CreateFile )
NSTUB( 0xFF1D9620, FIO_CloseFile )
NSTUB( 0xFF1D979C, FIO_GetFileSize )
NSTUB( 0xFF1D9570, FIO_WriteFile )
NSTUB( 0xFF1D93D0, FIO_ReadFile )
NSTUB( 0xFF1D9328, FIO_RemoveFile )
NSTUB( 0xFF1DA41C, FIO_FindFirstEx )
NSTUB( 0xFF1DA510, FIO_FindNextEx )
NSTUB( 0xFF1D9E5C, FIO_CreateDirectory)

NSTUB( 0xFF06F654, give_semaphore )
NSTUB( 0xFF06F56C, take_semaphore )
NSTUB( 0xFF06CC74, dm_set_store_level ) // called by "dmstore"
NSTUB( 0xFF05BAC4, prop_register_slave ) // called by"GUI_RegisterPropertySlave"
NSTUB( 0xFF05B45C, prop_request_change )
NSTUB( 0xFF05BA3C, prop_deliver ) 
NSTUB( 0xFF05BBE4, _prop_cleanup )
NSTUB( 0xFF05BC7C, prop_get_value )

NSTUB( 0xFF1DC10C, dispcheck )
NSTUB( 0xFF062AF4, SetBitmapVramAddress ) // where to find bmp_vram_info
NSTUB( 0x1D86C, bmp_vram_info )
NSTUB( 0x76268, vram_info )             // in vram_get_number
NSTUB( 0xFF2E0164, vram_get_number )
NSTUB( 0xFF1DAB90, vram_get_pos_and_size )

NSTUB( 0xFF018D34, malloc )
NSTUB( 0xFF018D8C, free )
NSTUB( 0xFF07096C, AllocateMemory )
NSTUB( 0xFF070CFC, FreeMemory )

NSTUB( 0x28E4, dm_names )                // in DebugMsg, before the 1st loop target
NSTUB( 0xFF201F08, strcpy )
NSTUB( 0xFF0694E4, LoadCalendarFromRTC )
NSTUB( 0xFF07C970, task_trampoline )
                           
NSTUB( 0xFF069B38, _audio_ic_write )
NSTUB( 0xFF0699F4, _audio_ic_read )
NSTUB( 0xFF057570, sounddev_task )
NSTUB( 0xFF057A5C, sounddev_active_in )
NSTUB( 0xFF5A67E4, audio_thresholds )        // after ptr to "ALVFactorySemSignature"
// ROM:FF1AC348 check_audio_lvl_maybe         550d 108
// ROM:FF1BA638 check_audio_lvl_maybe         1100d 104, where to find audio_thresholds
// FF1BA630                                   1100 105
NSTUB( 0x1E58, sounddev )                    // in sounddev_task

NSTUB( 0xFF06F40C, create_named_semaphore )
NSTUB( 0xFF31B5F0, gui_task_create )
NSTUB( 0xFF31B710, gui_task_destroy )
NSTUB( 0xFF31B4A0, ctrlman_dispatch_event )

NSTUB( 0xFF530890, memcpy )
NSTUB( 0xFF223844, prop_request_icu_auto_poweroff )
NSTUB( 0xFF1EFE08, alloc_dma_memory )
NSTUB( 0xFF1EFE3C, free_dma_memory )

//NSTUB( 0xFF4C2CE0, cfReadBlk )
//NSTUB( 0x1553C, cf_device )                // in cfReadBlk
NSTUB( 0xFF4C8A1C, sdReadBlk )
NSTUB( 0x15560, sd_device )                // in sdReadBlk

NSTUB( 0xFF01B2CC, hotplug_task )   // not found by finsig in 105
NSTUB( 0x1A7C, hotplug_struct )
NSTUB( 0x1A8C, hotplug_usb_buf )          // after "USB Disconnect" 

NSTUB( 0xFF06C53C, bootdisk_enable )    // EnableBootDisk
NSTUB( 0xFF06C548, bootdisk_disable )

NSTUB( 0xFF010500, cli_save )
NSTUB( 0xFF010514, sei_restore )
NSTUB( 0xFF2A35D4, ptp_register_handler )
NSTUB( 0xFF139910, gui_lock )             // PtpPropSetUILock
NSTUB( 0xFF06E260, oneshot_timer )
// NSTUB( 0xFF31B718, gui_task_destroy )   // already defined above
NSTUB( 0xFF020A40, gui_main_task )       //  uses a 8 functions table 
// func_table FF536108
NSTUB( 0xFF0A8BEC, gui_init_event )
NSTUB( 0xFF02144C, gui_init_end )     // EndGuiInit
// FF0A557C GUI_Control_Post
//NSTUB( 0x3AC8, gui_timer_struct )     // in GUI_Control_Post
NSTUB( 0x1BF4, gui_main_struct )      //  in gui_main_task
// queue_handle at offset 0x30, counter at offset 0xC
NSTUB( 0xFF1E885C, msg_queue_receive )

