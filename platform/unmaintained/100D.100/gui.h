/*
 * Just a copy of the 650D stuff. Indented = WRONG
 */

#ifndef _cameraspecific_gui_h_
#define _cameraspecific_gui_h_

/*
 * TODO: EVERYTHING BELOW HERE IS UNVERIFIED OR PLAIN WRONG
 */

    // touch events
    #define BGMT_TOUCH_1_FINGER 0x6b
    #define BGMT_UNTOUCH_1_FINGER 0x6c
    #define BGMT_TOUCH_2_FINGER 0x72
    #define BGMT_UNTOUCH_2_FINGER 0x73
#if 0

        // used for knowing when canon's lv overlays are showing
        #define GUI_LV_OVERLAYS_HIDDEN -7
        #define GUI_LV_OVERLAYS_VISIBLE 0x37


        // button codes as received by gui_main_task
        // need to print those on screen
#endif
        #define BGMT_WHEEL_UP 0
        #define BGMT_WHEEL_DOWN 1
    #define BGMT_WHEEL_LEFT 2
    #define BGMT_WHEEL_RIGHT 3
    #define BGMT_PRESS_SET 4
    #define BGMT_UNPRESS_SET 5
    #define BGMT_MENU 6
    #define BGMT_INFO 7
    #define BGMT_PLAY 0xB
#define BGMT_TRASH 0xD
#if 0

    #define BGMT_REC 0x1E
#endif
    #define BGMT_LV 0x1E
    #define BGMT_Q 0x1D
#if 0
        //~ #define BGMT_Q_ALT 0x67

        //~ #define BGMT_FUNC 0x12
        //~ #define BGMT_PICSTYLE 0x13
        //~ #define BGMT_JOY_CENTER (lv ? 0x1e : 0x3b)

#endif
    #define BGMT_PRESS_UP 0x28          //~ unpress = 0x2b
    #define BGMT_UNPRESS_UP 0x29
        //~ #define BGMT_PRESS_UP_RIGHT 0x17
        //~ #define BGMT_PRESS_UP_LEFT 0x18
    #define BGMT_PRESS_LEFT 0x26       //~ unpress = 0x27
    #define BGMT_UNPRESS_LEFT 0x27
    #define BGMT_PRESS_RIGHT 0x24      //~ unpress = 0x29
    #define BGMT_UNPRESS_RIGHT 0x25
        //~ #define BGMT_PRESS_DOWN_RIGHT 0x1B
        //~ #define BGMT_PRESS_DOWN_LEFT 0x1C
    #define BGMT_PRESS_DOWN 0x2a       //~ unpress = 0x2d
    #define BGMT_UNPRESS_DOWN 0x2b

    #define BGMT_PRESS_HALFSHUTTER 0x4e
#if 0
    #define BGMT_UNPRESS_HALFSHUTTER 0x4f

        #define BGMT_PRESS_FULLSHUTTER 0xdeadbeef

    #define GMT_GUICMD_PRESS_BUTTON_SOMETHING 0x52 // unhandled buttons?

#endif
    #define GMT_OLC_INFO_CHANGED 0x67 // backtrace copyOlcDataToStorage call in gui_massive_event_loop
#if 0

    // needed for correct shutdown from powersave modes
    #define GMT_GUICMD_START_AS_CHECK 95
    #define GMT_GUICMD_OPEN_SLOT_COVER 91
    #define GMT_GUICMD_LOCK_OFF 89
        
        //~ not implemented yet
#endif
        #define GMT_LOCAL_DIALOG_REFRESH_LV 0x36 // event type = 2, gui code = 0x100000C6 in EOS-M
#if 0
        #define BGMT_FLASH_MOVIE (0)
        #define BGMT_PRESS_FLASH_MOVIE (0)
        #define BGMT_UNPRESS_FLASH_MOVIE (0)
        #define FLASH_BTN_MOVIE_MODE (get_disp_pressed() && lv)

    #define BGMT_PRESS_ZOOM_OUT 0x10
    #define BGMT_UNPRESS_ZOOM_OUT 0x11
#endif
    #define BGMT_PRESS_ZOOM_IN 0xe
#if 0
    #define BGMT_UNPRESS_ZOOM_IN 0xf

    /* can't be PLAY */
    //~ #define BTN_ZEBRAS_FOR_PLAYBACK BGMT_PLAY // what button to use for zebras in Play mode
#endif
#endif
