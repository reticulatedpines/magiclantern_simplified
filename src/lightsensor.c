/** \file
 * LCD RemoteShot for 5D (with ambient light se)
 * 
 * (C) 2012 Alex Dumitrache, broscutamaker@gmail.com
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


int lcd_sensor_shortcuts = 0;
int get_lcd_sensor_shortcuts() { return lcd_sensor_shortcuts; }

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
        v == 1 ? "Near" : v == 2 ? (get_mlu() ? "Away/MLU" : "Away") : v == 3 ? "Wave" : "OFF"
    );
    if (v) display_lcd_remote_icon(x-25, y+5);
    menu_draw_icon(x, y, v ? MNI_NONE : MNI_OFF, 0);
}

extern int remote_shot_flag; // from shoot.c
int wave_count = 0;
int wave_count_countdown = 0;
int lcd_ff_dir = 1;
void sensor_status_trigger(int on)
{
    lens_display_set_dirty();
    if (lcd_release_running && on) info_led_on();

    static int prev = 0;
    int off = !on;
    if (on == prev) // false alarm
        goto end;
    prev = on;

    if (lcd_release_running && off) info_led_off();
    
    if (remote_shot_flag) goto end;

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
        if (gui_menu_shown()) goto end;
        if (lcd_release_running == 1 && off) goto end;
        if (lcd_release_running == 2 && on )
        {
            if (get_mlu()) schedule_mlu_lock();
            goto end;
        }
        if (lcd_release_running == 3) { wave_count++; wave_count_countdown = 50; }
        if (lcd_release_running == 3 && wave_count < 5) goto end;

        if (lcd_release_running == 3 && RECORDING) schedule_movie_end(); // wave mode is allowed to stop movies
        else if (RECORDING && is_rack_focus_enabled())
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

    //~ idle_wakeup_reset_counters(-20);

    end:
    return;
}

void lcd_release_step() // to be called from shoot_task
{
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

int lightsensor_raw_value = 0;
//~ int lightsensor_value = 0;

//~ int is_lightsensor_triggered() { return lightsensor_triggered; }

void LightMeasureCBR(int priv, int light)
{
    lightsensor_raw_value = light;
    //~ bmp_printf(FONT_LARGE, 0, 0, "%d ", light);
}

void light_sensor_task(void* unused)
{
    TASK_LOOP
    {
        // only poll the light sensor when needed (seems to be a bit CPU-intensive)
        int sensor_needed = lcd_release_running || (is_follow_focus_active() && get_follow_focus_mode()==1) || is_menu_active("Debug");
        if (!sensor_needed) { msleep(500); continue; }
        
        msleep(50);
        LightMeasure_n_Callback_r0(LightMeasureCBR, 0);

        int ev_x100 = gain_to_ev_scaled(lightsensor_raw_value, 100);

        static int ev_avg_x100 = 0;
        if (ev_avg_x100 == 0) ev_avg_x100 = ev_x100;
        ev_avg_x100 = (ev_avg_x100 * 63 + ev_x100) / 64;

        static int avg_prev0 = 1000;
        static int avg_prev1 = 1000;
        static int avg_prev2 = 1000;
        static int avg_prev3 = 1000;

        int lightsensor_delta_from_average = ev_x100 - avg_prev3;

        avg_prev3 = avg_prev2;
        avg_prev2 = avg_prev1;
        avg_prev1 = avg_prev0;
        avg_prev0 = ev_avg_x100;

        // trigger at -2 EV under average
        // maintain until -1 EV
        if (lightsensor_delta_from_average < -200) display_sensor = 1;
        else if (lightsensor_delta_from_average > -100) display_sensor = 0;
        
        if (k < 50) k++; // ignore the first cycles: wait for the signal to become stable
        else sensor_status_trigger(display_sensor);
    }
}

TASK_CREATE( "light_sensor_task", light_sensor_task, 0, 0x1c, 0x1000 );
