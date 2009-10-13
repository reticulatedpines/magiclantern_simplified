/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware.
 *
 * \todo Sort this file?
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

/** These must be found first for any progress to be made */
NSTUB( 0xFF810894, cstart )
NSTUB( 0xFFC22F0C, bzero32 )
NSTUB( 0xFF8173A0, create_init_task )
NSTUB( 0xFF811DBC, init_task )

/** How to find this one again? */
NSTUB( 0x11F98, additional_version )

/** AllocateMemory() calls this one */
NSTUB( 0xFF86ABD0, _malloc )

/** These are necessary for starting with debugging */
/** look for the string "dumpf", then find register_function as well */
NSTUB( 0xFF81B628, dmsetup )
NSTUB( 0xFF86725C, dumpf )
NSTUB( 0xFF866948, dmstart )
NSTUB( 0xFF866988, dmstop )

/** This stores pointers to all of the debugging subsystem names */
NSTUB( 0x292c, dm_names )

/** This sets the level of messages that will be stored */
NSTUB( 0xFF866D14, dm_set_store_level )

/** Find this one to be able to identify named functions */
NSTUB( 0xFF999CFC, register_function )

/** Look for error conditions that then call this function */
NSTUB( 0xFF8669C8, DebugMsg )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF99853C, vsnprintf )
NSTUB( 0xFF833A24, write_debug_file )

/** This one is hard to find.  Start around 0xff869c90, looking
 * for functions that take one argument and are called by
 * thousands of other functions.
 */
NSTUB( 0xFF869C90, msleep )

/** These are lower prority to locate */
NSTUB( 0xFF8781F4, strcpy )
NSTUB( 0xFFC22E18, memcpy )
NSTUB( 0xFF99C7B4, index )
NSTUB( 0xFF878250, strlen )

/** File I/O using the FIO_ routines */
NSTUB( 0xFF81BDC0, open )
NSTUB( 0xFF81BE70, close )
NSTUB( 0xFF98C2A0, FIO_Open )
NSTUB( 0xFF98C354, FIO_CreateFile )
NSTUB( 0xFF98C68C, FIO_ReadFile )
NSTUB( 0xFF98C83C, FIO_WriteFile )
NSTUB( 0xFF98C3FC, FIO_CloseFile )
NSTUB( 0xFF98C9F0, FIO_GetFileSize )
NSTUB( 0xFF98D40C, FIO_FindNextEx )
NSTUB( 0xFF98D2E0, FIO_FindFirstEx )
NSTUB( 0xFF98C80C, FIO_Seek )
NSTUB( 0xFF94B3C4, FR_SyncReadFileToBuffer )
NSTUB( 0xFF94AFB8, FR_SyncReadFile )

/** Look for a matched pair in gui_init_end */
NSTUB( 0xFF8697EC, take_semaphore )
NSTUB( 0xFF8698D4, give_semaphore )

/** There are two task creation calls.  We want the
 * one that takes five arguments with the name as the
 * first one.
 */
NSTUB( 0xFF869D48, task_create )

/** Verify that 0x1934 is the task dispatch hook by
 * looking for the dispatch call.  Follow task_create
 * to find it.
 */
NSTUB( 0x1934, task_dispatch_hook )

/** The task trampoline is after create_task. */
NSTUB( 0xFF8729C8, task_trampoline )

/** Look for the string 'DisablePowerSave' */
NSTUB( 0xFF86D4DC, powersave_disable )

/** Look for the string 'emAutoPowerOff' */
NSTUB( 0xFF8A3DB8, emAutoPowerOff )
NSTUB( 0xFF88F23C, prop_request_icu_auto_poweroff )

/** I don't remember how I found this one */
NSTUB( 0xFF9FB5E4, ptp_register_handler )

/** These data structures are required for the gui_main_task
 * override.  Once gui_main_task has been located, walk
 * through it to find all of the other functions and structures.
 * @{
 */
NSTUB( 0x1EE0, mvr_struct )
NSTUB( 0x72AC, mvr_config )
NSTUB( 0x1C4C, gui_main_struct )
NSTUB( 0x3668, gui_timer_struct )
NSTUB( 0xFF823698, gui_main_task )
NSTUB( 0xFF823B5C, gui_control )
NSTUB( 0xFF824048, gui_init_end )
NSTUB( 0xFF8693EC, msg_queue_receive )
NSTUB( 0xFF88B730, gui_massive_event_loop )
NSTUB( 0xFF88D148, gui_local_post )
NSTUB( 0xFF994CE8, gui_timer_something )
NSTUB( 0xFF88CCA4, gui_change_mode )
NSTUB( 0xFF88D534, gui_other_post )
NSTUB( 0xFF88D658, gui_post_10000085 )
NSTUB( 0xFF88DD94, gui_init_event )
NSTUB( 0xFF88D810, gui_change_shoot_type_post )
NSTUB( 0xFF88D8A8, gui_change_lcd_state_post )
/** @} */

/** Look for the call in CreateDialog */
NSTUB( 0xFFA5446C, gui_task_create )
NSTUB( 0xFFA5458C, gui_task_destroy )
NSTUB( 0x147CC, gui_task_list )
NSTUB( 0xFFA55654, dialog_create )
NSTUB( 0xFFA570D4, dialog_delete )
NSTUB( 0xFFA54844, dialog_post_event )
NSTUB( 0xFFA55F58, dialog_draw )
NSTUB( 0xFFA6DC84, dialog_set_active )
NSTUB( 0xFFA6DC74, dialog_set_inactive )
NSTUB( 0xFFA6DC04, dialog_screen_backup )
NSTUB( 0xFFA55B38, dialog_label_item )
NSTUB( 0xFFA549C0, dialog_redraw_maybe ) // maybe
NSTUB( 0xFFA56DE8, dialog_set_enabled )
NSTUB( 0xFFA56E3C, dialog_make_visible ) // maybe
NSTUB( 0xFFA55288, dialog_handler )

/** Look for "GUI_SetLvMode" */
NSTUB( 0xFF9AF4A4, gui_set_lv_mode )
NSTUB( 0xFF88A8E0, gui_set_video_display_type )
NSTUB( 0xFF88ACE4, gui_set_request_mode )
NSTUB( 0xFF823CE4, gui_notify_event )

NSTUB( 0xFFA5431C, ctrlman_dispatch_event )

/** This can be found in dispcheck, but keep in mind that it
 * references bmp_vram_info[1], so subtract 0xC from its value
 */
NSTUB( 0x240C0, bmp_vram_info )
NSTUB( 0xFF86020C, dispcheck )

/** Search for the string GetVramNumber */
NSTUB( 0xFFA2B6EC, vram_get_number )

/** First struct referenced in vram_get_number */
NSTUB( 0x13EA0, image_play_struct )
/** Second struct referenced in vram_get_number */
NSTUB( 0x333B0, vram_info )

/** Once this is found, the rest of the audio should be easy to locate */
NSTUB( 0xFF972348, audio_level_task )

/** These are all grouped together */
NSTUB( 0xFF856C34, audio_set_filter_off )
NSTUB( 0xFF856450, audio_set_windcut )
NSTUB( 0xFF857AE4, audio_set_sampling_param )
NSTUB( 0xFF857D0C, audio_set_volume_in )
NSTUB( 0xFF856AB4, audio_set_alc_on )
NSTUB( 0xFF856B10, audio_set_alc_off )
NSTUB( 0xFF97233C, audio_interval_unlock )
NSTUB( 0xFF857004, sounddev_active_in )
NSTUB( 0xFF857134, sounddev_active_out )
NSTUB( 0xFF854FC4, audio_start_asif_observer )
NSTUB( 0xFF856D8C, _audio_ic_read )
NSTUB( 0xFF855CC8, _audio_ic_write )
NSTUB( 0xFF856FC0, audio_ic_sweep_message_queue )

NSTUB( 0xFF868184, oneshot_timer )
NSTUB( 0xFF8704D8, timeout_schedule )


/** Look for it in functions that are from SoundDevice/ */
NSTUB( 0xFF856E5C, sounddev_task )
NSTUB( 0x208C, sounddev )

/** Look for it in functions that are from AudioIC/ */
NSTUB( 0x1F58, audio_ic )
NSTUB( 0x7320, audio_in )

/** Find the audio_level_to_db function; it references this table */
NSTUB( 0xFFC61C28, audio_thresholds )

/** Look for the error message CreateBinarySemaphore */
NSTUB( 0xFF86DDFC, create_named_semaphore )

/** Hot plug task detects when video, hdmi and usb are plugged in. */
NSTUB( 0xFF81DB08, hotplug_task )
NSTUB( 0x1AA4, hotplug_struct )
NSTUB( 0x1AD8, hotplug_usb_buf )
NSTUB( 0x2580, hdmi_config )

/** Call registered function by name? */
NSTUB( 0xFF867744, call )

/** These are useful functions that control the camera */
NSTUB( 0xFF8896F0, FA_StartLiveView )
NSTUB( 0xFF8897E8, FA_StopLiveView )
NSTUB( 0xFF888E84, take_photo ) // called "Release"

/** Part of the lens information */
NSTUB( 0x2F3E0, current_lens_name )

/** Property slave handler */
NSTUB( 0xFF85AC64, prop_register_slave )
NSTUB( 0xFF85AD38, prop_cleanup )
NSTUB( 0xFF85ABDC, prop_deliver )
NSTUB( 0xFF85AD9C, prop_request_change )
NSTUB( 0xFF85AEB8, prop_get_value )

/** ErrBatteryApp prevents chipped batteries from working */
NSTUB( 0xFF88F0E0, GUI_SetErrBattery )
NSTUB( 0xFFA86614, StopErrBatteryApp )
NSTUB( 0x14B9C, err_battery_ptr )


/** Look for strings like %Y:%m:%d; they are calls to strftime */
NSTUB( 0xFF86A310, strftime )
NSTUB( 0xFF86513C, LoadCalendarFromRTC )
