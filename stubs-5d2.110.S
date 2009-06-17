/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.1.0 firmware.
 *
 * \todo Sort this file?
 */
.text

#define NSTUB(addr,name) \
	.global name; \
	name = addr

NSTUB( ROMBASEADDR, firmware_entry )
NSTUB( RELOCADDR+0xc, reloc_entry )

/** Camera interface? */
NSTUB( 0xC0220000, camera_engine )

/** These must be found first for any progress to be made */
NSTUB( 0xFF810894, cstart )
NSTUB( 0xFFC22F0C, bzero32 )
NSTUB( 0xFF8173A0, create_init_task )
NSTUB( 0xFF811DBC, init_task )

/** How to find this one again? */
NSTUB( 0x11F98, additional_version )

/** These are necessary for starting with debugging */
/** look for the string "dumpf", then find register_function as well */
NSTUB( 0xFF86725C, dumpf )
NSTUB( 0xFF866948, dmstart )
NSTUB( 0xFF866988, dmstop )

/** This stores pointers to all of the debugging subsystem names */
NSTUB( 0x292c, dm_names )

/** Find this one to be able to identify named functions */
NSTUB( 0xFF999CFC, register_function )

/** Look for error conditions that then call this function */
NSTUB( 0xFF8669C8, DebugMsg )

/** Look in DebugMsg, near the bottom.  It calls vsnprintf()
 * with a buffer size of 0x4C.
 */
NSTUB( 0xFF99853C, vsnprintf )

/** This one is hard to find.  Start around 0xff869c90, looking
 * for functions that take one argument and are called by
 * thousands of other functions.
 */
NSTUB( 0xFF869C90, msleep )

/** These are lower prority to locate */
NSTUB( 0xFF8781F4, strcpy )
NSTUB( 0xFFC22E18, memcpy )

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


/** These data structures are required for the gui_main_task
 * override.  Once gui_main_task has been located, walk
 * through it to find all of the other functions and structures.
 * @{
 */
NSTUB( 0x1C4C, gui_main_struct )
NSTUB( 0x3668, gui_timer_struct )
NSTUB( 0xFF823698, gui_main_task )
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
NSTUB( 0xFFA55654, dialog_create )

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
NSTUB( 0xFF856D8C, audio_ic_read )
NSTUB( 0xFF855CC8, audio_ic_write )

NSTUB( 0xFF868184, oneshot_timer )


/** Look for it in functions that are from SoundDevice/ */
NSTUB( 0xFF856E5C, sounddev_task )
NSTUB( 0x208C, sounddev )

/** Look for it in functions that are from AudioIC/ */
NSTUB( 0x1F58, audioic )
NSTUB( 0x7320, audio_in )

/** Find the audio_level_to_db function; it references this table */
NSTUB( 0xFFC61C28, audio_thresholds )

/** Look for the error message CreateBinarySemaphore */
NSTUB( 0xFF86DDFC, create_named_semaphore )

/** Hot plug task detects when video, hdmi and usb are plugged in. */
NSTUB( 0xFF81DB08, hotplug_task )
NSTUB( 0x1AA4, hotplug_struct )

/** Call registered function by name? */
NSTUB( 0xFF867744, call )

/** These are useful functions that control the camera */
NSTUB( 0xFF8896F0, FA_StartLiveView )
NSTUB( 0xFF8897E8, FA_StopLiveView )

#if 0
NSTUB( 0xFF810674, DryosPanic )
NSTUB( 0xFF8167F0, get_current_task )
NSTUB( 0xFF869C94, msleep )
NSTUB( 0xFF815CC0, sched_yield )
NSTUB( 0xFF811DBC, init_task )
NSTUB( 0xFF8173A0, create_init_task )
NSTUB( 0xFFC22054, task_save_state )
NSTUB( 0xFF8676EC, RegisterEventProcedure_im1 )
NSTUB( 0xFF8676F4, UnregisterEventProcedure )
NSTUB( 0xFF9F2D48, EP_SetMovieManualExposureMode )
NSTUB( 0xFF9F4E8C, EP_SetLVAEDebugPort )
NSTUB( 0xFF9F4E64, EP_SetDebugLogMode )
NSTUB( 0xFF86DFEC, new_task_struct )
NSTUB( 0xFF86DD10, create_task )
NSTUB( 0xFF8729CC, task_trampoline )
NSTUB( 0xFF86DE00, create_named_semaphore )
NSTUB( 0xFF8697F0, take_semaphore )
NSTUB( 0xFF8698D8, give_semaphore )
NSTUB( 0xFF992924, EdLedOn )
NSTUB( 0xFF992950, EdLedOff )
NSTUB( 0xFF86694C, dmstart )
NSTUB( 0xFF86698C, dmstop )
NSTUB( 0xFF86701C, dumpentire )
NSTUB( 0xFF867260, dumpf )
NSTUB( 0xFF8704DC, oneshot_timer )
NSTUB( 0xFF856AB8, audio_set_alc_on )
NSTUB( 0xFF856B14, audio_set_alc_off )
NSTUB( 0xFF856C38, audio_set_filter_off )
NSTUB( 0xFF856454, audio_set_windcut )
NSTUB( 0xFF857AE8, audio_set_sampling_param )
NSTUB( 0xFF857D10, audio_set_volume_in )
NSTUB( 0xFF854FC8, audio_start_asif_observer )
NSTUB( 0xFF9721C0, audio_level_task )
NSTUB( 0xFF9721B4, audio_interval_unlock )
NSTUB( 0xFF856E60, sound_dev_task )
NSTUB( 0xFF857F30, sound_dev_start_observer )
NSTUB( 0xFF857F98, sound_dev_stop_observer )
NSTUB( 0xFF81BDC0, open )
NSTUB( 0xFF81BE70, close )
NSTUB( 0xFF98C1CC, FIO_CreateFile )
NSTUB( 0xFF98C6B4, FIO_WriteFile )
NSTUB( 0xFF98CD6C, FIO_CloseFile )
NSTUB( 0xFF98C274, FIO_CloseSync )
NSTUB( 0xFF833A18, write_debug_file )
NSTUB( 0xFFC21E14, bzero32 )
NSTUB( 0xFFA5499C, dialog_create )
NSTUB( 0xFFA54BFC, dialog_window_prepare )
NSTUB( 0xFFA56184, dialog_window_draw )
NSTUB( 0xFFA552A0, dialog_draw )
NSTUB( 0xFFA55FBC, dialog_set_origin_type )
NSTUB( 0xFFA54C04, dialog_resize )
NSTUB( 0xFFA55BCC, dialog_window_resize )
NSTUB( 0xFFA54CA4, dialog_move )
NSTUB( 0xFFA55E8C, dialog_move_item )
NSTUB( 0xFFA54E80, dialog_label_item )
NSTUB( 0xFF99A2E0, state_object_create )
NSTUB( 0xFF99A1F0, state_object_dispatch )
NSTUB( 0xFFA6656C, vram_get_lock )
NSTUB( 0xFFA66748, vram_instance )
NSTUB( 0xFFAE05FC, vram_image_pos_and_size )
NSTUB( 0xFF85F9A0, vram_schedule_callback )
NSTUB( 0xFF860210, dispcheck )
NSTUB( 0xFF8669CC, DebugMsg )
NSTUB( 0xFFAB88AC, color_palette_push )
NSTUB( 0xFF9AF5D8, copy_lvram_info )
NSTUB( 0xFF88A8D8, gui_get_display_type )
NSTUB( 0xFFA537B4, gui_task_create )
NSTUB( 0xFF88B734, gui_massive_event_loop )


NSTUB( 0xFFA6CF4C, winsys_whole_screen_backup )
NSTUB( 0xFFA6CFCC, winsys_set_flag_0x34 )
NSTUB( 0xFFA6CFBC, winsys_clr_flag_0x34 )
NSTUB( 0xFFA6CF28, winsys_set_flag_0x30 )
NSTUB( 0xFFA6D070, winsys_take_semaphore )


NSTUB( 0xFFBA0898, main_tab_header_dialog )
NSTUB( 0xFFBA0820, StopMnMainTabHeaderApp )
NSTUB( 0xFFB9CA70, StartMnMainRec1App )
NSTUB( 0xFFB9D384, StartMnMainRec2App )
NSTUB( 0xFFB9B800, StartMnMainPlay1App )
NSTUB( 0xFFB9BF9C, StartMnMainPlay2App )
NSTUB( 0xFFB9DE74, StartMnMainSetup1App )
NSTUB( 0xFFB9E754, StartMnMainSetup2App )
NSTUB( 0xFFB9F048, StartMnMainSetup3App )
NSTUB( 0xFFB985BC, StartMnMainCustomFuncApp )
NSTUB( 0xFFB9AA54, StartMnMainMyMenuApp )

NSTUB( 0xFFBDDB50, StartFactoryMenuApp )
NSTUB( 0xFF9BAC48, StartMnStudioSetupmenuApp )

NSTUB( 0xFF86674C, dm_event_dispatch )
NSTUB( 0xFF866D18, dm_set_store_level )

NSTUB( 0xFF8781F8, strcpy )
NSTUB( 0xFFB7F380, memcpy )

/* Structures in the BSS or rwdata segment */
NSTUB( 0x1D74, liveview )
NSTUB( 0x2580, hdmi_config )
NSTUB( 0x2924, dm_state_ptr )
NSTUB( 0x3548, gui_struct )
NSTUB( 0x426C, lv_struct )
NSTUB( 0x7324, audio_info )
NSTUB( 0x8348, dm_state_object )
NSTUB( 0x14900, winsys_struct )
NSTUB( 0x1F848, main_tab_dialog_id )
NSTUB( 0x2f33c, lvram_info )
NSTUB( 0x333B0, vram_info )
#endif
