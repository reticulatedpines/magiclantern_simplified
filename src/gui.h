#ifndef _big_gui_h_
#define _big_gui_h_

#define FAKE_BTN -123456
#define IS_FAKE(event) (event->arg == FAKE_BTN)

#define MLEV_HIJACK_FORMAT_DIALOG_BOX -1
#define MLEV_TURN_ON_DISPLAY -2
#define MLEV_TURN_OFF_DISPLAY -3
#define MLEV_HIDE_CANON_BOTTOM_BAR -4
#define MLEV_ChangeHDMIOutputSizeToVGA -5
#define MLEV_LCD_SENSOR_START -6

#ifdef CONFIG_550D
#include "../platform/550D.109/gui.h"
#endif

#ifdef CONFIG_60D
#include "../platform/60D.110/gui.h"
#endif

#ifdef CONFIG_600D
#include "../platform/600D.101/gui.h"
#endif

#ifdef CONFIG_50D
#include "../platform/50D.108/gui.h"
#endif

#ifdef CONFIG_1100D
#include "../platform/1100D.104/gui.h"
#endif


#endif
