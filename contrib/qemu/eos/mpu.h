#ifndef EOS_MPU_H
#define EOS_MPU_H

#include "eos.h"

#define ARG0 0x0100
#define ARG1 0x0101
#define ARG2 0x0102
#define ARG3 0x0103
#define MPU_SHUTDOWN 0xFFFF

void mpu_spells_init(EOSState *s);
void mpu_handle_sio3_interrupt(EOSState *s);
void mpu_handle_mreq_interrupt(EOSState *s);
unsigned int eos_handle_mpu(unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_sio3 ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );
unsigned int eos_handle_mreq ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value );

void mpu_send_keypress(EOSState *s, int keycode);

enum button_codes
{
    MPU_EVENT_DISABLED,
    BGMT_INFO, BGMT_MENU, BGMT_PLAY, BGMT_UNPRESS_INFO,
    BGMT_PRESS_SET, BGMT_UNPRESS_SET, BGMT_JOY_CENTER,
    BGMT_PRESS_DOWN, BGMT_PRESS_DOWN_LEFT, BGMT_PRESS_DOWN_RIGHT, 
    BGMT_PRESS_LEFT, BGMT_PRESS_RIGHT, 
    BGMT_PRESS_UP, BGMT_PRESS_UP_LEFT, BGMT_PRESS_UP_RIGHT,
    BGMT_UNPRESS_DOWN, BGMT_UNPRESS_LEFT, BGMT_UNPRESS_RIGHT, BGMT_UNPRESS_UP,
    BGMT_UNPRESS_UDLR,
    BGMT_SILENT_UP, BGMT_SILENT_LEFT, BGMT_SILENT_RIGHT, BGMT_SILENT_DOWN,
    BGMT_SILENT_UNPRESS,
    BGMT_PRESS_ZOOM_IN, BGMT_PRESS_ZOOM_OUT,
    BGMT_UNPRESS_ZOOM_IN, BGMT_UNPRESS_ZOOM_OUT,
    BGMT_PRESS_MAGNIFY_BUTTON, BGMT_UNPRESS_MAGNIFY_BUTTON,
    BGMT_Q, BGMT_UNPRESS_Q, BGMT_LV, BGMT_PRESS_AV, BGMT_UNPRESS_AV,
    BGMT_JUMP, BGMT_PRESS_DIRECT_PRINT, BGMT_UNPRESS_DIRECT_PRINT, BGMT_FUNC,
    BGMT_TRASH, BGMT_UNLOCK, BGMT_PICSTYLE, BGMT_LIGHT, 
    BGMT_AFPAT_UNPRESS, BGMT_PRESS_DISP, BGMT_UNPRESS_DISP,
    BGMT_PRESS_RAW_JPEG, BGMT_UNPRESS_RAW_JPEG,
    BGMT_WHEEL_DOWN, BGMT_WHEEL_LEFT, BGMT_WHEEL_RIGHT, BGMT_WHEEL_UP,
    BGMT_PRESS_HALFSHUTTER, BGMT_UNPRESS_HALFSHUTTER,
    BGMT_PRESS_FULLSHUTTER, BGMT_UNPRESS_FULLSHUTTER,
    GMT_GUICMD_OPEN_SLOT_COVER, GMT_GUICMD_START_AS_CHECK,
    GMT_GUICMD_OPEN_BATT_COVER,
    GMT_GUICMD_PRESS_BUTTON_SOMETHING,
    GMT_LOCAL_DIALOG_REFRESH_LV,

    /* other events (MPU messages triggered on key presses) */
    MPU_SEND_SHUTDOWN_REQUEST,
    MPU_SEND_ABORT_REQUEST,
    MPU_NEXT_SHOOTING_MODE,
    MPU_PREV_SHOOTING_MODE,
    MPU_ENTER_MOVIE_MODE,

    /* finished */
    BGMT_END_OF_LIST
};

#endif

