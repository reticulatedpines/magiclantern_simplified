/** \file
 * LCD RemoteShot & related. Only for cameras with this sensor.
 * 
 * (C) 2010 Alex Dumitrache, broscutamaker@gmail.com
 */
/*
 * Magic Lantern is Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
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
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "lens.h"
#include "gui.h"

#ifdef CONFIG_500D
CONFIG_INT("lcdsensor.shortcuts", lcd_sensor_shortcuts, 1);
#else
CONFIG_INT("lcdsensor.shortcuts", lcd_sensor_shortcuts, 0);
#endif
int get_lcd_sensor_shortcuts() { return lcd_sensor_shortcuts==1 || (lcd_sensor_shortcuts==2 && is_movie_mode()); }

CONFIG_INT( "lcd.release", lcd_release_running, 0);

void display_lcd_remote_icon(int x0, int y0);

void 
lcd_release_display( void * priv, int x, int y, int selected )
{
    int v = (*(int*)priv);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LCDsensor Remote: %s",
        v == 1 ? "Near" : v == 2 ? "Away" : v == 3 ? "Wave" : "OFF"
    );
    if (v) display_lcd_remote_icon(x-25, y+5);
    menu_draw_icon(x, y, v ? MNI_NONE : MNI_OFF, 0);
}

extern int remote_shot_flag; // from shoot.c
int wave_count = 0;
int wave_count_countdown = 0;
int lcd_ff_dir = 1;
PROP_HANDLER(PROP_DISPSENSOR_CTRL)
{
    if (!DISPLAY_SENSOR_POWERED) return; // false alarm
    if (lens_info.job_state) return; // false alarm
    if (get_halfshutter_pressed()) return; // user taking a normal picture
    
    static int prev = 0;
    int on = !buf[0];
    int off = !on;
    if (on == prev) // false alarm
        return;
    prev = on;
    
    if (remote_shot_flag) return;

    if (lv && lens_info.job_state == 0 && lcd_release_running == 0 && is_follow_focus_active() && get_follow_focus_mode()==1) // FF on LCD sensor
    {
        if (on)
        {
            lens_focus_start( lcd_ff_dir * get_follow_focus_dir_h() );
        }
        else
        {
            lens_focus_stop();
            lcd_ff_dir = -lcd_ff_dir;
        }
    }
    else
    if (lcd_release_running && gui_state == GUISTATE_IDLE && !is_intervalometer_running())
    {
        if (gui_menu_shown()) return;
        if (lcd_release_running == 1 && off) return;
        if (lcd_release_running == 2 && on) return;
        if (lcd_release_running == 3) { wave_count++; wave_count_countdown = 50; }
        if (lcd_release_running == 3 && wave_count < 5) return;

        if (lcd_release_running == 3 && recording) schedule_movie_end(); // wave mode is allowed to stop movies
        else if (recording && is_rack_focus_enabled())
        {
            rack_focus_start_now(0);
        }
        else
        {
            schedule_remote_shot();
        }
        wave_count = 0;
    }
    else wave_count = 0;

    idle_wakeup_reset_counters(-20);
}

void lcd_release_step() // to be called from shoot_task
{
    extern int lcd_sensor_wakeup;
    int lcd_sensor_needeed_in_liveview = (lcd_release_running || lcd_sensor_shortcuts || lcd_sensor_wakeup || get_follow_focus_mode()==1);
    int lcd_sensor_needeed_in_photomode = (lcd_release_running);
    int lcd_sensor_needed = (lcd_sensor_needeed_in_liveview && lv) || (lcd_sensor_needeed_in_photomode && !lv && !DISPLAY_IS_ON && display_idle());
    int lcd_sensor_start_preconditions = !DISPLAY_SENSOR_POWERED && lens_info.job_state == 0;
    if (lcd_sensor_needed && lcd_sensor_start_preconditions) // force sensor on
    {
        int timestamp = get_seconds_clock();
        static int last_try = 0;
        if (timestamp != last_try)
            fake_simple_button(MLEV_LCD_SENSOR_START); // only try once per second
        last_try = timestamp;
    }

    if (wave_count_countdown)
    {
        wave_count_countdown--;
        if (!wave_count_countdown) wave_count = 0;
    }
}

void display_lcd_remote_icon(int x0, int y0)
{
    int cl_on = COLOR_RED;
    int cl_off = lv ? COLOR_WHITE : COLOR_FG_NONLV;
    int cl = display_sensor ? cl_on : cl_off;
    // int bg = lv ? 0 : bmp_getpixel(x0 - 20, 1);

    if (gui_menu_shown()) cl = COLOR_WHITE;

    if (lcd_release_running == 1)
    {
        draw_circle(x0, 10+y0, 8, cl);
        draw_circle(x0, 10+y0, 7, cl);
        draw_line(x0-5,10-5+y0,x0+5,10+5+y0,cl);
        draw_line(x0-5,10+5+y0,x0+5,10-5+y0,cl);
    }
    else if (lcd_release_running == 2)
    {
        draw_circle(x0, 10+y0, 8, cl);
        draw_circle(x0, 10+y0, 7, cl);
        draw_circle(x0, 10+y0, 1, cl);
        draw_circle(x0, 10+y0, 2, cl);
    }
    else if (lcd_release_running == 3)
    {
        int yup = y0;
        int ydn = 10+y0;
        int step = 5;
        int k;
        for (k = 0; k < 2; k++)
        {
            draw_line(x0 - 2*step, ydn, x0 - 1*step, yup, wave_count > 0 ? cl_on : cl_off);
            draw_line(x0 - 1*step, yup, x0 - 0*step, ydn, wave_count > 1 ? cl_on : cl_off);
            draw_line(x0 - 0*step, ydn, x0 + 1*step, yup, wave_count > 2 ? cl_on : cl_off);
            draw_line(x0 + 1*step, yup, x0 + 2*step, ydn, wave_count > 3 ? cl_on : cl_off);
            draw_line(x0 + 2*step, ydn, x0 + 3*step, yup, wave_count > 4 ? cl_on : cl_off);
            x0++;
        }
    }
    else if (lcd_release_running == 0 && is_follow_focus_active() && get_follow_focus_mode()==1 && lv)
    {
        bmp_printf(FONT_MED, x0-10, y0, "FF%s", get_follow_focus_dir_h() * lcd_ff_dir > 0 ? "-" : "+");
        bmp_printf(FONT_LARGE, 650, 50, "FF%s", get_follow_focus_dir_h() * lcd_ff_dir > 0 ? "-" : "+");
    }
    
    //~ if (gui_menu_shown()) return;
    
    //~ static unsigned int prev_lr = 0;
    //~ if (prev_lr != lcd_release_running) bmp_fill(bg, x0 - 20, y0, 40, 20);
    //~ prev_lr = lcd_release_running;
}


// sensor shortcuts
//**********************************************************************

void
lcd_sensor_shortcuts_print(
    void *          priv,
    int         x,
    int         y,
    int         selected
)
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "LCD Sensor Shortcuts: %s", 
        lcd_sensor_shortcuts == 1 ? "ON" : 
        lcd_sensor_shortcuts == 2 ? "Movie" : "OFF"
    );
}
