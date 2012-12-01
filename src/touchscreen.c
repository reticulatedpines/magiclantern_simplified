/** \file
 *
 *  Magic Lantern Touch Screen Driver
 *
 *  Extends touch screen capability to Magic Lantern.
 *
 *  - Coutts
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

#include "dryos.h"
#include "bmp.h"
#include "touchscreen.h"
#include "cache_hacks.h"

struct touch
{
    int x;
    int y;
    int num_touch_points;
    int touch_id;
};

static struct touch touch;
static int touch_happening = 0;
static int multi_touch_happening = 0;
static int lock = 0;
static int xold = 0;
static int xnew = 0;




static int record_gesture()
{
    //~ do msleep on separate task so we can still report the latest touch values.
    lock = 1;
    
    return xnew;
}

static int check_horizontal_swipe()
{
    if (ABS(xnew - xold) > 80)
    {
        if (xnew < xold)
            NotifyBox(500, "Swiped right!");
        if (xnew > xold)
            NotifyBox(500, "Swiped left!");
    }
}


//~ This replaces touch_cbr_canon with our function. We don't need to worry
//~ about calling canon's CBR as it only prints a DebugMsg.
static int my_touch_event_cbr(int x, int y, int num_touch_points, int touch_id)
{
    touch.x = x;
    touch.y = y;
    touch.num_touch_points = num_touch_points;
    touch.touch_id = touch_id;
    
    if (touch.num_touch_points)
    {
        xold = touch.x;
        //~ bmp_printf(FONT_MED, 0, 300, "oldx: %d", xold);
        xnew = record_gesture();
        //~ bmp_printf(FONT_MED, 0, 320, "newx: %d", xnew);
        check_horizontal_swipe();
    }
}


//~ Replace pointer to canon's CBR function when touch events happen. This is
//~ the safest method I know of at the moment. Much better than relying on a
//~ task and reading values from TOUCH_XY_RAW1/2.
static void touch_init(void* unused)
{
    *(int*)HIJACK_TOUCH_CBR_PTR = &my_touch_event_cbr;
}

static int touch_gesture_task()
{
    
    TASK_LOOP
    {
        if (lock)
        {
            msleep(20);
            xnew = touch.x;
            //~ bmp_printf(FONT_MED, 0, 100, "xnew from task: %d", xnew);
            lock = 0;
        }
        else
        {
            msleep(50);
        }
    }
}

TASK_CREATE("touch_gesture_task", touch_gesture_task, 0, 0x1f, 0x1000);

INIT_FUNC(__FILE__, touch_init);
