/** \file
 * Lens focus and zoom related things
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
#include "lens.h"
#include "property.h"
#include "bmp.h"
#include "config.h"
#include "menu.h"
#include "math.h"

#ifdef CONFIG_5D2
#define CONFIG_FULLFRAME
#endif

void update_stuff();

void bv_update_lensinfo();
void bv_auto_update();

CONFIG_INT("shutter.display.degrees", shutter_display_degrees, 0);

CONFIG_INT("movie.log", movie_log, 0);
#ifndef CONFIG_FULLFRAME
#define SENSORCROPFACTOR 16
CONFIG_INT("crop.info", crop_info, 0);
#else
#define SENSORCROPFACTOR 10
#define crop_info 0
#endif

static struct semaphore * lens_sem;
static struct semaphore * focus_done_sem;
//~ static struct semaphore * job_sem;


struct lens_info lens_info = {
    .name        = "NO LENS NAME"
};


/** Compute the depth of field for the current lens parameters.
 *
 * This relies heavily on:
 *     http://en.wikipedia.org/wiki/Circle_of_confusion
 * The CoC value given there is 0.019 mm, but we need to scale things
 */
static void
calc_dof(
    struct lens_info * const info
)
{
    const uint32_t        coc = 19; // 1/1000 mm
    const uint32_t        fd = info->focus_dist * 10; // into mm
    const uint32_t        fl = info->focal_len; // already in mm

    // If we have no aperture value then we can't compute any of this
    // Not all lenses report the focus distance
    if( fl == 0 || info->aperture == 0 )
    {
        info->dof_near        = 0;
        info->dof_far        = 0;
        info->hyperfocal    = 0;
        return;
    }

    const uint32_t        fl2 = fl * fl;

    // The aperture is scaled by 10 and the CoC by 1000,
    // so scale the focal len, too.  This results in a mm measurement
    const unsigned H = ((1000 * fl2) / (info->aperture  * coc)) * 10;
    info->hyperfocal = H;

    // If we do not have the focus distance, then we can not compute
    // near and far parameters
    if( fd == 0 )
    {
        info->dof_near        = 0;
        info->dof_far        = 0;
        return;
    }

    // fd is in mm, H is in mm, but the product of H * fd can
    // exceed 2^32, so we scale it back down before processing
    info->dof_near = ((H * (fd/10)) / ( H + fd )) * 10; // in mm
    if( fd > H )
        info->dof_far = 1000 * 1000; // infinity
    else
        info->dof_far = ((H * (fd/10)) / ( H - fd )) * 10; // in mm
}

/*
const char *
lens_format_dist(
    unsigned        mm
)
{
    static char dist[ 32 ];

    if( mm > 100000 ) // 100 m
        snprintf( dist, sizeof(dist),
            "%d.%1dm",
            mm / 1000,
            (mm % 1000) / 100
        );
    else
    if( mm > 10000 ) // 10 m
        snprintf( dist, sizeof(dist),
            "%2d.%02dm",
            mm / 1000,
            (mm % 1000) / 10
        );
    else
    if( mm >  1000 ) // 1 m
        snprintf( dist, sizeof(dist),
            "%1d.%03dm",
            mm / 1000,
            (mm % 1000)
        );
    else
        snprintf( dist, sizeof(dist),
            "%dcm",
            mm / 10
        );

    return dist;
}*/

const char * lens_format_dist( unsigned mm)
{
   static char dist[ 32 ];

   if( mm > 100000 ) // 100 m
   {
      snprintf( dist, sizeof(dist), "Inf.");
   }
   else if( mm > 10000 ) // 10 m
   {
      snprintf( dist, sizeof(dist), "%2d.%1dm", mm / 1000,  (mm % 1000) / 1000);
   }
   else    if( mm >  1000 ) // 1 m 
   {
      snprintf( dist, sizeof(dist), "%1d.%1dm", mm / 1000, (mm % 1000)/100 );
   }
   else
   {
      snprintf( dist, sizeof(dist),"%2dcm", mm / 10 );
   }

   return (dist);
} /* end of aj_lens_format_dist() */

void
update_lens_display()
{
    draw_ml_topbar();
    
    extern int menu_upside_down; // don't use double buffer in this mode
    int double_buffering = !menu_upside_down && !is_canon_bottom_bar_dirty();
    draw_ml_bottombar(double_buffering, 1);
}

int should_draw_bottom_bar()
{
    if (gui_menu_shown()) return 1;
    if (!get_global_draw()) return 0;
    //~ if (EXT_MONITOR_CONNECTED) return 1;
    if (canon_gui_front_buffer_disabled()) return 1;
    if (is_canon_bottom_bar_dirty())
    {
        crop_set_dirty(5);
        return 0;
    }
    if (lv_disp_mode == 0) return 1;
    return 0;
}

int raw2shutter_ms(int raw_shutter)
{
    if (!raw_shutter) return 0;
    return (int) roundf(powf(2.0, (136.0 - raw_shutter)/8.0));
}
int shutter_ms_to_raw(int shutter_ms)
{
    if (shutter_ms == 0) return 160;
    return (int) roundf(136 - log2f(shutter_ms) * 8);
}
int raw2iso(int raw_iso)
{
    int iso = (int) roundf(100.0 * powf(2.0, (raw_iso - 72.0)/8.0));
    if (iso >= 100 && iso <= 6400)
        iso = values_iso[raw2index_iso(raw_iso)];
    else if (iso > 100000)
        iso = ((iso+500)/1000) * 1000;
    else if (iso > 10000)
        iso = ((iso+50)/100) * 100;
    else if (iso >= 60)
        iso = ((iso+5)/10) * 10;
    else if (iso >= 15)
        iso = ((iso+2)/5) * 5;
    else
        iso = iso&~1;
    return iso;
}

void shave_color_bar(int x0, int y0, int w, int h, int shaved_color);

void draw_ml_bottombar(int double_buffering, int clear)
{
    //~ beep();
    if (!should_draw_bottom_bar()) return;
    
    struct lens_info *    info = &lens_info;

    //~ int bg = TOPBAR_BGCOLOR;
    int bg = COLOR_BLACK;
    if (is_movie_mode() || gui_menu_shown()) bg = COLOR_BLACK;
    //~ unsigned font    = FONT(FONT_MED, COLOR_WHITE, bg);
    //~ unsigned font_err    = FONT( FONT_MED, COLOR_RED, bg);
    //~ unsigned Font    = FONT(FONT_LARGE, COLOR_WHITE, bg);
    //~ unsigned height    = fontspec_height( font );
    
    unsigned bottom = 480;
    int screen_layout = get_screen_layout();
    
    if (screen_layout == SCREENLAYOUT_3_2_or_4_3) bottom = os.y_max;
    else if (screen_layout == SCREENLAYOUT_16_9) bottom = os.y_max - os.off_169;
    else if (screen_layout == SCREENLAYOUT_16_10) bottom = os.y_max - os.off_1610;
        else if (screen_layout == SCREENLAYOUT_UNDER_3_2) bottom = MIN(os.y_max + 54, vram_bm.height);
        else if (screen_layout == SCREENLAYOUT_UNDER_16_9) bottom = MIN(os.y_max - os.off_169 + 54, vram_bm.height);

    if (gui_menu_shown())
        bottom = 480 + (hdmi_code == 5 ? 40 : 0); // force it at the bottom of menu

    //~ bottom -= 10;

    //~ if (screen_layout == SCREENLAYOUT_16_9)
        //~ bg = bmp_getpixel(os.x0 + 123, bottom-36);
    //unsigned x = 420;
    //~ unsigned y = 480 - height - 10;
    //~ if (ext_monitor_hdmi) y += recording ? -100 : 200;

    unsigned int x_origin = MAX(os.x0 + os.x_ex/2 - 360 + 50, 0);
    unsigned int y_origin = bottom - 30;
    unsigned text_font = SHADOW_FONT(FONT(FONT_LARGE, COLOR_WHITE, bg));

    int ytop = bottom - 35;
    
    // start drawing to mirror buffer to avoid flicker
    if (double_buffering)
    {
        //~ bmp_mirror_copy(0);
        memcpy(bmp_vram_idle() + BM(0,ytop), bmp_vram_real() + BM(0,ytop), 35 * BMPPITCH);
        bmp_draw_to_idle(1);
    }

    if (clear)
    {
        uint8_t* B = bmp_vram();
        uint8_t* M = get_bvram_mirror();
        for (int y = bottom-35; y < bottom; y++)
        {
            for (int x = 0; x < vram_bm.width; x++)
            {
                uint8_t p = B[BM(x,y)];
                uint8_t m = M[BM(x,y)];
                if (m & 0x80) B[BM(x,y)] = m & ~0x80; // from cropmark
                else B[BM(x,y)] = 0;
            }
        }
    }

        // MODE
        
            bmp_printf( FONT(text_font, canon_gui_front_buffer_disabled() ? COLOR_YELLOW : COLOR_WHITE, FONT_BG(text_font)), x_origin - 50, y_origin,
                "%s",
                is_movie_mode() ? "Mv" : 
                shooting_mode == SHOOTMODE_P ? "P " :
                shooting_mode == SHOOTMODE_M ? "M " :
                shooting_mode == SHOOTMODE_TV ? "Tv" :
                shooting_mode == SHOOTMODE_AV ? "Av" :
                shooting_mode == SHOOTMODE_CA ? "CA" :
                shooting_mode == SHOOTMODE_ADEP ? "AD" :
                shooting_mode == SHOOTMODE_AUTO ? "[]" :
                shooting_mode == SHOOTMODE_LANDSCAPE ? "LD" :
                shooting_mode == SHOOTMODE_PORTRAIT ? ":)" :
                shooting_mode == SHOOTMODE_NOFLASH ? "NF" :
                shooting_mode == SHOOTMODE_MACRO ? "MC" :
                shooting_mode == SHOOTMODE_SPORTS ? "SP" :
                shooting_mode == SHOOTMODE_NIGHT ? "NI" :
                "?"
            );

      /*******************
      * FOCAL & APERTURE *
      *******************/
      
      if (info->aperture && info->name[0])
      {
          text_font = FONT(SHADOW_FONT(FONT_LARGE),COLOR_WHITE,bg);
          unsigned med_font = FONT(SHADOW_FONT(FONT_MED),COLOR_WHITE,bg);

          static char focal[32];
          snprintf(focal, sizeof(focal), "%d",
                   crop_info ? (info->focal_len * SENSORCROPFACTOR + 5) / 10 : info->focal_len);

          //~ int IS_font = FONT(text_font, lens_info.IS ? COLOR_YELLOW : COLOR_WHITE, bg );
          int IS_font_med = FONT(med_font,
                    lens_info.IS == 0 ? COLOR_WHITE    :  // IS off
                    lens_info.IS == 4 ? COLOR_CYAN     :  // IS active, but not engaged
                    lens_info.IS == 8 ? COLOR_BLACK    :  // IS disabled on sigma lenses?
                    lens_info.IS == 0xC ? COLOR_YELLOW :  // IS starting?
                    lens_info.IS == 0xE ? COLOR_ORANGE :  // IS active and kicking
                    COLOR_RED,                            // unknown
                    bg
                );
          bmp_printf( text_font, x_origin - 5, y_origin, focal );

        if (info->aperture < 100)
        {
              bmp_printf( text_font, 
                          x_origin + 74 + font_med.width + font_large.width - 7, 
                          y_origin, 
                          ".");
              bmp_printf( text_font, 
                          x_origin + 74 + font_med.width  , 
                          y_origin, 
                          "%d", info->aperture / 10);
              bmp_printf( text_font, 
                          x_origin + 74 + font_med.width + font_large.width * 2 - 14, 
                          y_origin, 
                          "%d", info->aperture % 10);
        }
        else
              bmp_printf( text_font, 
                          x_origin + 74 + font_med.width  , 
                          y_origin, 
                          "%d    ", info->aperture / 10) ;

          bmp_printf( med_font, 
                      x_origin + font_large.width * strlen(focal) - 3 - 5, 
                      bottom - font_med.height, 
                      crop_info ? "eq" : "mm");

          if (lens_info.IS)
          bmp_printf( IS_font_med, 
                      x_origin + font_large.width * strlen(focal) - 3 - 5 + 1, 
                      y_origin - 3, 
                      "IS");

          bmp_printf( med_font, 
                      x_origin + 74 + 2  , 
                      y_origin - 2, 
                      "f") ;
      }
  
      /*******************
      *  SHUTTER         *
      *******************/


      int shutter_x10 = raw2shutter_ms(info->raw_shutter) / 100;
      int shutter_reciprocal = info->raw_shutter ? (int) roundf(4000.0 / powf(2.0, (152 - info->raw_shutter)/8.0)) : 0;

      // in movie mode we can get the exact value from Canon timers
      if (is_movie_mode()) shutter_reciprocal = (get_current_shutter_reciprocal_x1000()+500)/1000;
      
      if (shutter_reciprocal > 100) shutter_reciprocal = 10 * ((shutter_reciprocal+5) / 10);
      if (shutter_reciprocal > 1000) shutter_reciprocal = 100 * ((shutter_reciprocal+50) / 100);
      static char shutter[32];
      if (info->raw_shutter == 0) snprintf(shutter, sizeof(shutter), "    ");
      else if (shutter_x10 >= 350) snprintf(shutter, sizeof(shutter), "BULB");
      else if (shutter_x10 <= 3) snprintf(shutter, sizeof(shutter), "%d  ", shutter_reciprocal);
      else if (shutter_x10 % 10 && shutter_x10 < 30) snprintf(shutter, sizeof(shutter), "%d.%d ", shutter_x10 / 10, shutter_x10 % 10);
      else snprintf(shutter, sizeof(shutter), "%d  ", (shutter_x10+5) / 10);

      int fgs = COLOR_CYAN; // blue (neutral)
      int shutter_degrees = -1;
      if (is_movie_mode()) // check 180 degree rule
      {
           shutter_degrees = 360 * video_mode_fps / shutter_reciprocal;
           if (ABS(shutter_degrees - 180) < 10)
              fgs = FONT(FONT_LARGE,COLOR_GREEN1,bg);
           else if (shutter_degrees > 190)
              fgs = FONT(FONT_LARGE,COLOR_RED,bg);
           else if (shutter_degrees < 45)
              fgs = FONT(FONT_LARGE,COLOR_RED,bg);
      }
      else if (info->aperture) // rule of thumb: shutter speed should be roughly equal to focal length
      {
           int focal_35mm = (info->focal_len * SENSORCROPFACTOR + 5) / 10;
           if (lens_info.IS) focal_35mm /= 4; // assume 2-stop effectiveness for IS
           if (shutter_reciprocal > focal_35mm * 15/10) 
              fgs = FONT(FONT_LARGE,COLOR_GREEN1,bg); // very good
           else if (shutter_reciprocal < focal_35mm / 2) 
              fgs = FONT(FONT_LARGE,COLOR_RED,bg); // you should have really steady hands
           else if (shutter_reciprocal < focal_35mm) 
              fgs = FONT(FONT_LARGE,COLOR_YELLOW,bg); // OK, but be careful
      }

    text_font = SHADOW_FONT(FONT(FONT_LARGE,fgs,bg));
    if (is_movie_mode() && shutter_display_degrees)
    {
        snprintf(shutter, sizeof(shutter), "%d  ", shutter_degrees);
        bmp_printf( text_font, 
                    x_origin + 143 + font_med.width*2  , 
                    y_origin, 
                    shutter);

        text_font = FONT(SHADOW_FONT(FONT_MED),fgs,bg);

        bmp_printf( text_font, 
                    x_origin + 143 + font_med.width*2 + (strlen(shutter) - 2) * font_large.width, 
                    y_origin, 
                    "o");
    }
    else if (is_movie_mode() && is_hard_shutter_override_active())
    {
        int d = get_shutter_override_degrees_x10();
        int q = d/10;
        int r = d%10;
        int cr = r + '0';
        snprintf(shutter, sizeof(shutter), "%d%s%s  ", q, r ? "." : "", r ? (char*)&cr : "");
        bmp_printf( FONT(text_font,COLOR_ORANGE,bg), 
                    x_origin + 143 + font_med.width*2  , 
                    y_origin, 
                    shutter);

        text_font = FONT(SHADOW_FONT(FONT_MED),COLOR_ORANGE,bg);

        bmp_printf( text_font, 
                    x_origin + 143 + font_med.width*2 + (strlen(shutter) - 2) * font_large.width, 
                    y_origin, 
                    "o");
    }
    else
    {
        bmp_printf( text_font, 
                x_origin + 143 + font_med.width*2  , 
                y_origin, 
                shutter);

        text_font = FONT(SHADOW_FONT(FONT_MED),fgs,bg);

        bmp_printf( text_font, 
                x_origin + 143 + 1  , 
                y_origin - 2, 
                shutter_x10 > 3 ? "  " : "1/");
    }

      /*******************
      *  ISO             *
      *******************/

      // good iso = 160 320 640 1250  - according to bloom video  
      //  http://www.youtube.com/watch?v=TNNqUm_nSXk&NR=1

      text_font = FONT(
      SHADOW_FONT(FONT_LARGE), 
      is_native_iso(lens_info.iso) ? COLOR_YELLOW :
      is_lowgain_iso(lens_info.iso) ? COLOR_GREEN2 : COLOR_RED,
      bg);
      
      extern int hdrv_enabled;

        if (hdrv_enabled)
        {
            int iso_low, iso_high;
            hdr_get_iso_range(&iso_low, &iso_high);
            iso_low = raw2iso(iso_low);
            iso_high = raw2iso(iso_high);
            bmp_printf( FONT(FONT_MED, COLOR_WHITE, bg),
                      x_origin + 245  , 
                      y_origin + 5, 
                      "%4d/%d", iso_low, iso_high) ;
        }
        else if (info->iso)
        {
            int digital_w_dispgain = info->iso_digital_ev;
            int digital_wo_dispgain = digital_w_dispgain;
            
            if (LVAE_DISP_GAIN && !CONTROL_BV && is_movie_mode())
                // display gain gets recorded, consider it as ISO digital gain
                digital_w_dispgain = digital_w_dispgain + (gain_to_ev_x8(LVAE_DISP_GAIN) - 80);

            text_font = FONT(
                SHADOW_FONT(FONT_LARGE),
                    digital_w_dispgain > 0 || digital_wo_dispgain > 0 ? COLOR_RED :
                    digital_w_dispgain < 0 ? COLOR_GREEN2 :
                    COLOR_YELLOW,
                bg
            );
            char msg[10];
            int iso = raw2iso(info->iso_equiv_raw);
            snprintf(msg, sizeof(msg), "%d   ", iso >= 10000 ? iso/100 : iso);
            bmp_printf( text_font, 
                      x_origin + 250  , 
                      y_origin, 
                      msg);
            if (digital_w_dispgain != digital_wo_dispgain || CONTROL_BV)
                bmp_printf( FONT(SHADOW_FONT(FONT_MED), FONT_FG(text_font), bg), 
                          x_origin + 250 + font_large.width * (strlen(msg)-3) - 2, 
                          bottom - font_med.height, 
                          CONTROL_BV ? "ov" : "eq");
            if (iso >= 10000)
                bmp_printf( FONT(SHADOW_FONT(FONT_MED), FONT_FG(text_font), bg), 
                          x_origin + 250 + font_large.width * (strlen(msg)-3) - 2, 
                          y_origin - 2, 
                          "00");
        }
        else if (info->iso_auto)
            bmp_printf( text_font, 
                      x_origin + 250  , 
                      y_origin, 
                      "A%d   ", info->iso_auto);
        else
            bmp_printf( text_font, 
                      x_origin + 250  , 
                      y_origin, 
                      "Auto ");

      if (ISO_ADJUSTMENT_ACTIVE) goto end;
      
        // kelvins
      text_font = FONT(
      SHADOW_FONT(FONT_LARGE), 
      0x13, // orange
      bg);

        if( info->wb_mode == WB_KELVIN )
            bmp_printf( text_font, x_origin + 360 + (info->kelvin >= 10000 ? 0 : font_large.width), y_origin,
                info->kelvin >= 10000 ? "%5dK " : "%4dK ",
                info->kelvin
            );
        else
            bmp_printf( text_font, x_origin + 360, y_origin,
                "%s ",
                (lens_info.wb_mode == 0 ? "AutoWB" : 
                (lens_info.wb_mode == 1 ? " Sunny" :
                (lens_info.wb_mode == 2 ? "Cloudy" : 
                (lens_info.wb_mode == 3 ? "Tungst" : 
                (lens_info.wb_mode == 4 ? "Fluor." : 
                (lens_info.wb_mode == 5 ? " Flash" : 
                (lens_info.wb_mode == 6 ? "Custom" : 
                (lens_info.wb_mode == 8 ? " Shade" :
                 "unk"))))))))
            );
        
        int gm = lens_info.wbs_gm;
        int ba = lens_info.wbs_ba;
        if (gm) 
            bmp_printf(
                FONT(ba ? FONT_MED : FONT_LARGE, COLOR_WHITE, gm > 0 ? COLOR_GREEN2 : 14 /* magenta */),
                x_origin + 360 + font_large.width * 6, y_origin + (ba ? -3 : 0), 
                "%d", ABS(gm)
            );

        if (ba) 
            bmp_printf(
                FONT(gm ? FONT_MED : FONT_LARGE, COLOR_WHITE, ba > 0 ? 12 : COLOR_BLUE), 
                x_origin + 360 + font_large.width * 6, y_origin + (gm ? 14 : 0), 
                "%d", ABS(ba));


      /*******************
      *  Focus distance  *
      *******************/

      text_font = FONT(SHADOW_FONT(FONT_LARGE), is_manual_focus() ? COLOR_CYAN : COLOR_WHITE, bg );   // WHITE

      if(lens_info.focus_dist)
          bmp_printf( text_font, 
                  x_origin + 505  , 
                  y_origin, 
                  lens_format_dist( lens_info.focus_dist * 10 )
                );
        else
          bmp_printf( text_font, 
                  x_origin + 515  , 
                  y_origin, 
                  is_manual_focus() ? "MF" : "AF"
                );

#ifdef CONFIG_5D2
    //~ extern int lightsensor_value;
    //~ extern int lightsensor_triggered;
    //~ text_font = FONT(SHADOW_FONT(FONT_MED), COLOR_CYAN, bg );
    //~ maru(x_origin + 600, y_origin+3, lightsensor_triggered ? COLOR_RED : 50);
    display_lcd_remote_icon(x_origin + 640, y_origin+10);
    //~ bmp_printf(text_font, x_origin + 630, y_origin + 10, "%d", lightsensor_value);
#else

      text_font = FONT(SHADOW_FONT(FONT_LARGE), COLOR_CYAN, bg ); 

      bmp_printf( text_font, 
                  x_origin + 610 + font_large.width * 2 - 8, 
                  y_origin, 
                  ".");
      bmp_printf( text_font, 
                  x_origin + 610 - font_large.width, 
                  y_origin, 
                  " %s%d", 
                    AE_VALUE < 0 ? "-" : AE_VALUE > 0 ? "+" : " ",
                    ABS(AE_VALUE) / 8
                  );
      bmp_printf( text_font, 
                  x_origin + 610 + font_large.width * 3 - 16, 
                  y_origin, 
                  "%d",
                    mod(ABS(AE_VALUE) * 10 / 8, 10)
                  );
#endif

        // battery indicator
        int xr = x_origin + 612 - font_large.width - 4;

    #if defined(CONFIG_60D) || defined(CONFIG_5D2)
        int bat = GetBatteryLevel();
    #else
        int bat = battery_level_bars == 0 ? 5 : battery_level_bars == 1 ? 30 : 100;
    #endif

        int col = battery_level_bars == 0 ? COLOR_RED :
                  battery_level_bars == 1 ? COLOR_YELLOW : 
                #if defined(CONFIG_60D) || defined(CONFIG_5D2)
                  bat <= 70 ? COLOR_WHITE : 
                #endif
                  COLOR_GREEN1;
        
        bat = bat * 22 / 100;
        bmp_draw_rect(COLOR_BLACK, xr+1, y_origin-4, 9, 4);
        bmp_draw_rect(COLOR_BLACK, xr-3, y_origin-1, 17, 31);
        //~ bmp_draw_rect(COLOR_BLACK, xr, y_origin+2, 11, 29);
        bmp_draw_rect(COLOR_BLACK, xr+1, y_origin + 24 - bat, 9, bat+2);
        bmp_fill(col, xr+4, y_origin-3, 8, 3);
        bmp_draw_rect(col, xr-2, y_origin, 15, 29);
        bmp_draw_rect(col, xr-1, y_origin + 1, 13, 27);
        bmp_fill(col, xr+4, y_origin + 26 - bat, 8, bat);


    //~ if (hdmi_code == 2) shave_color_bar(40,370,640,16,bg);
    //~ if (hdmi_code == 5) shave_color_bar(75,480,810,22,bg);
    
    // these have a black bar at the bottom => no problems
    //~ #if !defined(CONFIG_500D) && !defined(CONFIG_50D)
    //~ int y169 = os.y_max - os.off_169;

    //~ if (!gui_menu_shown() && (screen_layout == SCREENLAYOUT_16_9 || screen_layout == SCREENLAYOUT_16_10 || hdmi_code == 2 || ext_monitor_rca))
        //~ shave_color_bar(os.x0, ytop, os.x_ex, y169 - ytop + 1, bg);
    //~ #endif

    // mark the BV mode somehow
    if(CONTROL_BV)
    {
        bmp_draw_rect(COLOR_RED, x_origin + 70, y_origin - 4, 284, 35);
        bmp_draw_rect(COLOR_RED, x_origin + 71, y_origin - 3, 284-2, 35-2);
    }


end:

    if (double_buffering)
    {
        // done drawing, copy image to main BMP buffer
        bmp_draw_to_idle(0);
        //~ bmp_mirror_copy(1);
        memcpy(bmp_vram_real() + BM(0,ytop), bmp_vram_idle() + BM(0,ytop), 35 * BMPPITCH);
        bzero32(bmp_vram_idle() + BM(0,ytop), 35 * BMPPITCH);
    }

    // this is not really part of the bottom bar, but it's close to it :)
    /*
    if (LVAE_DISP_GAIN)
    {
        text_font = FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK ); 
        int gain_ev = gain_to_ev(LVAE_DISP_GAIN) - 10;
        bmp_printf( text_font, 
                  x_origin + 590, 
                  y_origin - font_large.height, 
                  "%s%dEV", 
                  gain_ev > 0 ? "+" : "-",
                  ABS(gain_ev));
    }*/
}
/*
void shave_color_bar(int x0, int y0, int w, int h, int shaved_color)
{
    // shave the bottom bar a bit :)
    int i,j;
    int new_color = bmp_getpixel_real(os.x0 + 123, y0-5);
    for (i = y0; i < y0 + h; i++)
    {
        //~ int new_color = 0;
        for (j = x0; j < x0+w; j++)
            if (bmp_getpixel(j,i) == shaved_color)
                bmp_putpixel(j,i,new_color);
        //~ bmp_putpixel(x0+5,i,COLOR_RED);
    }
}*/

void draw_ml_topbar()
{
    if (!get_global_draw()) return;
    
    //~ int bg = TOPBAR_BGCOLOR;
    //~ if (gui_menu_shown()) bg = COLOR_BLACK;
    unsigned font    = FONT(SHADOW_FONT(FONT_MED), COLOR_WHITE, COLOR_BLACK);
    //~ unsigned font_err    = FONT( f, COLOR_RED, bg);
    //~ unsigned Font    = FONT(FONT_LARGE, COLOR_WHITE, bg);
    //~ unsigned height    = fontspec_height( font );
    
    unsigned x = 80;
    unsigned y = 0;

    int screen_layout = get_screen_layout();

    if (screen_layout >= 3 && !should_draw_bottom_bar())
        return; // top bar drawn at bottom, may interfere with canon info


    if (gui_menu_shown())
    {
        x = MAX(os.x0 + os.x_ex/2 - 360, 0);
        //~ y = MAX(os.y0 + os.y_ex/2 - 240, os.y0);
        y = (hdmi_code == 5 ? 50 : 10); // force it at the top of menu
    }
    else
    {
        x = MAX(os.x0 + os.x_ex/2 - 360, 0);
        if (screen_layout == SCREENLAYOUT_3_2_or_4_3) y = os.y0; // just above the 16:9 frame
        else if (screen_layout == SCREENLAYOUT_16_9) y = os.y0 + os.off_169; // meters just below 16:9 border
        else if (screen_layout == SCREENLAYOUT_16_10) y = os.y0 + os.off_1610; // meters just below 16:9 border
        else if (screen_layout == SCREENLAYOUT_UNDER_3_2) y = MIN(os.y_max, vram_bm.height - 54);
        else if (screen_layout == SCREENLAYOUT_UNDER_16_9) y = MIN(os.y_max - os.off_169, vram_bm.height - 54);
    }
    
    extern int time_indic_x, time_indic_y; // for bitrate indicators
    time_indic_x = os.x_max - 160;
    time_indic_y = y;
    
    if (time_indic_y > vram_bm.height - 30) time_indic_y = vram_bm.height - 30;

    if (audio_meters_are_drawn() && !get_halfshutter_pressed()) return;

    struct tm now;
    LoadCalendarFromRTC( &now );
    bmp_printf(font, x, y, "%02d:%02d", now.tm_hour, now.tm_min);

    x += 80;

    bmp_printf( font, x, y,
        "DISP%d", get_disp_mode()
    );

    x += 70;

    int raw = pic_quality & 0x60000;
    int rawsize = pic_quality & 0xF;
    int jpegtype = pic_quality >> 24;
    int jpegsize = (pic_quality >> 8) & 0xFF;
    bmp_printf( font, x, y, "%s%s%s%s",
        rawsize == 1 ? "M" : rawsize == 2 ? "S" : "",
        raw ? "RAW" : "",
        jpegtype == 4 ? "" : (raw ? "+" : "JPG-"),
        jpegtype == 4 ? "" : (
            jpegsize == 0 ? (jpegtype == 3 ? "L" : "l") : 
            jpegsize == 1 ? (jpegtype == 3 ? "M" : "m") : 
            jpegsize == 2 ? (jpegtype == 3 ? "S" : "s") :
            jpegsize == 0x0e ? (jpegtype == 3 ? "S1" : "s1") :
            jpegsize == 0x0f ? (jpegtype == 3 ? "S2" : "s2") :
            jpegsize == 0x10 ? (jpegtype == 3 ? "S3" : "s3") :
            "err"
        )
    );

    x += 80;
    int alo = get_alo();
    bmp_printf( font, x, y,
        get_htp() ? "HTP" :
        alo == ALO_LOW ? "alo" :
        alo == ALO_STD ? "Alo" :
        alo == ALO_HIGH ? "ALO" : "   "
    );

    x += 45;
    bmp_printf( font, x, y, (char*)get_picstyle_shortname(lens_info.raw_picstyle));

    x += 70;
    #if defined(CONFIG_60D) || defined(CONFIG_5D2)
        bmp_printf( font, x, y,"T=%d BAT=%d", efic_temp, GetBatteryLevel());
    #else
        bmp_printf( font, x, y,"T=%d", efic_temp);
    #endif

    //~ display_clock();
    free_space_show();

    x += 160;
    bmp_printf( font, x, y,
        is_movie_mode() ? "MVI-%04d" : "[%d]",
        is_movie_mode() ? file_number_also : avail_shot
    );
}

volatile int lv_focus_done = 1;
volatile int lv_focus_error = 0;

PROP_HANDLER( PROP_LV_FOCUS_DONE )
{
    lv_focus_done = 1;
    if (buf[0] & 0x1000) 
    {
        NotifyBox(1000, "Focus: soft limit reached");
        lv_focus_error = 1;
    }
    return prop_cleanup( token, property );
}

void
lens_focus_wait( void )
{
    for (int i = 0; i < 100; i++)
    {
        msleep(10);
        if (lv_focus_done) return;
        if (!lv) return;
        if (is_manual_focus()) return;
    }
    NotifyBox(1000, "Focus error :(");
    lv_focus_error = 1;
    //~ NotifyBox(1000, "Press PLAY twice or reboot");
}

// this is compatible with all cameras so far, but allows only 3 speeds
int
lens_focus(
    int num_steps, 
    int stepsize, 
    int wait,
    int extra_delay
)
{
    if (!lv) return 0;
    if (is_manual_focus()) return 0;
    if (lens_info.job_state) return 0;

    if (num_steps < 0)
    {
        num_steps = -num_steps;
        stepsize = -stepsize;
    }

    stepsize = COERCE(stepsize, -3, 3);
    int focus_cmd = stepsize;
    if (stepsize < 0) focus_cmd = 0x8000 - stepsize;
    
    for (int i = 0; i < num_steps; i++)
    {
        lv_focus_done = 0;
        info_led_on();
        if (lv && !mirror_down && DISPLAY_IS_ON && lens_info.job_state == 0)
            prop_request_change(PROP_LV_LENS_DRIVE_REMOTE, &focus_cmd, 4);
        if (wait)
        {
            lens_focus_wait(); // this will sleep at least 10ms
            if (extra_delay > 10) msleep(extra_delay - 10); 
        }
        else
        {
            msleep(extra_delay);
        }
        info_led_off();
    }

    if (get_zoom_overlay_trigger_by_focus_ring()) zoom_overlay_set_countdown(300);
    //~ if (get_global_draw()) BMP_LOCK( draw_ml_bottombar(); )
    idle_wakeup_reset_counters(-10);
    lens_display_set_dirty();
    
    if (lv_focus_error) { msleep(200); lv_focus_error = 0; return 0; }
    return 1;
}

void lens_wait_readytotakepic(int wait)
{
    info_led_on();
    int i;
    for (i = 0; i < wait * 10; i++)
    {
        if (lens_info.job_state <= 0xA && burst_count > 0 && !is_movie_mode()) break;
        if (lens_info.job_state == 0 && burst_count > 0 && is_movie_mode()) break;
        msleep(20);
    }
    info_led_off();
}

int mirror_locked = 0;
void mlu_lock_mirror_if_needed() // called by lens_take_picture
{
    //~ NotifyBox(1000, "MLU locking");
    if (get_mlu() && !lv)
    {
        if (!mirror_locked)
        {
            mirror_locked = 1;
            #if defined(CONFIG_5D2) || defined(CONFIG_50D)
            SW1(1,100);
            SW2(1,100);
            SW2(0,100);
            SW1(0,100);
            #else
            call("Release");
            #endif
            msleep(100);
        }
    }
    //~ NotifyBox(1000, "MLU locked");
}

volatile int af_button_assignment = -1;
// to preview AF patterns
void assign_af_button_to_halfshutter()
{
    if (is_manual_focus()) return;
    take_semaphore(lens_sem, 0);
    while (lens_info.job_state >= 0xa) msleep(20);
    if (af_button_assignment == -1) af_button_assignment = cfn_get_af_button_assignment();
    if (af_button_assignment != AF_BTN_HALFSHUTTER) cfn_set_af_button(AF_BTN_HALFSHUTTER);
    else af_button_assignment = -1;
    give_semaphore(lens_sem);
}

// to prevent AF
void assign_af_button_to_star_button()
{
    if (is_manual_focus()) return;
    take_semaphore(lens_sem, 0);
    while (lens_info.job_state >= 0xa) msleep(20);
    if (af_button_assignment == -1) af_button_assignment = cfn_get_af_button_assignment();
    if (af_button_assignment != AF_BTN_STAR) cfn_set_af_button(AF_BTN_STAR);
    else af_button_assignment = -1;
    give_semaphore(lens_sem);
}

void restore_af_button_assignment()
{
    if (is_manual_focus()) return;
    if (af_button_assignment == -1) return;
    take_semaphore(lens_sem, 0);
    while (lens_info.job_state >= 0xa) msleep(20);
    cfn_set_af_button(af_button_assignment);
    af_button_assignment = -1;
    give_semaphore(lens_sem);
}


int
lens_take_picture(
    int            wait, 
    int allow_af
)
{
    if (!allow_af) assign_af_button_to_star_button();
    take_semaphore(lens_sem, 0);
    lens_wait_readytotakepic(64);
    
    mlu_lock_mirror_if_needed();

    #if defined(CONFIG_5D2) || defined(CONFIG_50D)
    if (get_mlu() && !lv)
    {
        SW1(1,100);
        SW2(1,100);
        SW2(0,100);
        SW1(0,100);
    }
    else call("Release");
    #else
    call("Release");
    #endif
    
    if( !wait )
    {
        give_semaphore(lens_sem);
        if (!allow_af) restore_af_button_assignment();
        return 0;
    }
    else
    {
        msleep(200);
        lens_wait_readytotakepic(wait);
        give_semaphore(lens_sem);
        if (!allow_af) restore_af_button_assignment();
        return lens_info.job_state;
    }
}

static FILE * mvr_logfile = INVALID_PTR;

/** Write the current lens info into the logfile */
static void
mvr_update_logfile(
    struct lens_info *    info,
    int            force
)
{
    if( mvr_logfile == INVALID_PTR )
        return;

    static unsigned last_iso;
    static unsigned last_shutter;
    static unsigned last_aperture;
    static unsigned last_focal_len;
    static unsigned last_focus_dist;

    // Check if nothing changed and not forced.  Do not write.
    if( !force
    &&  last_iso        == info->iso
    &&  last_shutter    == info->shutter
    &&  last_aperture    == info->aperture
    &&  last_focal_len    == info->focal_len
    &&  last_focus_dist    == info->focus_dist
    )
        return;

    // Record the last settings so that we know if anything changes
    last_iso    = info->iso;
    last_shutter    = info->shutter;
    last_aperture    = info->aperture;
    last_focal_len    = info->focal_len;
    last_focus_dist    = info->focus_dist;

    struct tm now;
    LoadCalendarFromRTC( &now );

    my_fprintf(
        mvr_logfile,
        "%02d:%02d:%02d,%d,%d,%d.%d,%d,%d\n",
        now.tm_hour,
        now.tm_min,
        now.tm_sec,
        info->iso,
        info->shutter,
        info->aperture / 10,
        info->aperture % 10,
        info->focal_len,
        info->focus_dist
    );
}

/** Create a logfile for each movie.
 * Record a logfile with the lens info for each movie.
 */
static void
mvr_create_logfile(
    unsigned        event
)
{
    DebugMsg( DM_MAGIC, 3, "%s: event %d", __func__, event );
    if (!movie_log) return;

    if( event == 0 )
    {
        // Movie stopped
        if( mvr_logfile != INVALID_PTR )
            FIO_CloseFile( mvr_logfile );
        mvr_logfile = INVALID_PTR;
        return;
    }

    if( event != 2 )
        return;

    // Movie starting
    char name[100];
    snprintf(name, sizeof(name), "%s/MVI_%04d.LOG", get_dcim_dir(), file_number);

    FIO_RemoveFile(name);
    mvr_logfile = FIO_CreateFile( name );
    if( mvr_logfile == INVALID_PTR )
    {
        bmp_printf( FONT_MED, 0, 40,
            "Unable to create movie log! fd=%x\n%s",
            (unsigned) mvr_logfile,
            name
        );

        return;
    }

    struct tm now;
    LoadCalendarFromRTC( &now );

    my_fprintf( mvr_logfile,
        "Start          : %4d/%02d/%02d %02d:%02d:%02d\n",
        now.tm_year + 1900,
        now.tm_mon + 1,
        now.tm_mday,
        now.tm_hour,
        now.tm_min,
        now.tm_sec
    );

    my_fprintf( mvr_logfile, "Lens name      : %s\n", lens_info.name );

    my_fprintf(
        mvr_logfile,
        "ISO            : %d\n"
        "Shutter        : 1/%ds\n"
        "Aperture       : f/%d.%d\n"
        "Focal length   : %d mm\n"
        "Focus distance : %d mm\n",
        lens_info.iso,
        lens_info.shutter,
        lens_info.aperture / 10, lens_info.aperture % 10,
        lens_info.focal_len,
        lens_info.focus_dist
    );

    my_fprintf(mvr_logfile, 
        "White Balance  : %d%s, %s %d, %s %d\n",
        lens_info.wb_mode == WB_KELVIN ? lens_info.kelvin : lens_info.wb_mode,
        lens_info.wb_mode == WB_KELVIN ? "K" : 
        lens_info.wb_mode == 0 ? " - Auto" : 
        lens_info.wb_mode == 1 ? " - Sunny" :
        lens_info.wb_mode == 2 ? " - Cloudy" : 
        lens_info.wb_mode == 3 ? " - Tungsten" : 
        lens_info.wb_mode == 4 ? " - Fluorescent" : 
        lens_info.wb_mode == 5 ? " - Flash" : 
        lens_info.wb_mode == 6 ? " - Custom" : 
        lens_info.wb_mode == 8 ? " - Shade" : " - unknown",
        lens_info.wbs_gm > 0 ? "Green" : "Magenta", ABS(lens_info.wbs_gm), 
        lens_info.wbs_ba > 0 ? "Amber" : "Blue", ABS(lens_info.wbs_ba)
        );

    
    my_fprintf( mvr_logfile, 
        "Picture Style  : %s (%d,%d,%d,%d)\n", 
        get_picstyle_name(lens_info.raw_picstyle), 
        lens_get_sharpness(),
        lens_get_contrast(),
        ABS(lens_get_saturation()) < 10 ? lens_get_saturation() : 0,
        ABS(lens_get_color_tone()) < 10 ? lens_get_color_tone() : 0
        );

    fps_mvr_log(mvr_logfile);
    hdr_mvr_log(mvr_logfile);

    my_fprintf( mvr_logfile, "\n\nCSV data:\n%s\n",
        "Frame,ISO,Shutter,Aperture,Focal_Len,Focus_Dist"
    );

    // Force the initial values to be written
    mvr_update_logfile( &lens_info, 1 );
}



static inline uint16_t
bswap16(
    uint16_t        val
)
{
    return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}

PROP_HANDLER( PROP_MVR_REC_START )
{
    mvr_rec_start_shoot(buf[0]);
    mvr_create_logfile( *(unsigned*) buf );
    return prop_cleanup( token, property );
}


PROP_HANDLER( PROP_LENS_NAME )
{
    if( len > sizeof(lens_info.name) )
        len = sizeof(lens_info.name);
    memcpy( (char*)lens_info.name, buf, len );
    return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_LENS)
{
    uint8_t* info = (uint8_t *) buf;
    lens_info.raw_aperture_min = info[1];
    lens_info.raw_aperture_max = info[2];

    if (lens_info.raw_aperture < lens_info.raw_aperture_min || lens_info.raw_aperture > lens_info.raw_aperture_max)
        lensinfo_set_aperture(0); // value no longer valid?
    
    bv_update_lensinfo();
    return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_LV_LENS_STABILIZE)
{
    //~ NotifyBox(2000, "%x ", buf[0]);
    lens_info.IS = (buf[0] & 0x000F0000) >> 16; // not sure, but lower word seems to be AF/MF status
    return prop_cleanup( token, property );
}

// it may be slow; if you need faster speed, replace this with a binary search or something better
#define RAWVAL_FUNC(param) \
int raw2index_##param(int raw) \
{ \
    int i; \
    for (i = 0; i < COUNT(codes_##param); i++) \
        if(codes_##param[i] >= raw) return i; \
    return 0; \
}\
\
int val2raw_##param(int val) \
{ \
    unsigned i; \
    for (i = 0; i < COUNT(codes_##param); i++) \
        if(values_##param[i] >= val) return codes_##param[i]; \
    return -1; \
}

RAWVAL_FUNC(iso)
RAWVAL_FUNC(shutter)
RAWVAL_FUNC(aperture)

#define RAW2VALUE(param,rawvalue) values_##param[raw2index_##param(rawvalue)]
#define VALUE2RAW(param,value) val2raw_##param(value)

void lensinfo_set_iso(int raw)
{
    lens_info.raw_iso = raw;
    lens_info.iso = RAW2VALUE(iso, raw);
    update_stuff();
}

void lensinfo_set_shutter(int raw)
{
    lens_info.raw_shutter = raw;
    lens_info.shutter = RAW2VALUE(shutter, raw);
    update_stuff();
}

void lensinfo_set_aperture(int raw)
{
    if (lens_info.raw_aperture_min && lens_info.raw_aperture_max)
    {
        raw = COERCE(raw, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
        lens_info.raw_aperture = raw;
        lens_info.aperture = RAW2VALUE(aperture, raw);
    }
    else
    {
        lens_info.aperture = lens_info.raw_aperture = 0;
    }
    //~ BMP_LOCK( lens_info.aperture = (int)roundf(10.0 * sqrtf(powf(2.0, (raw-8.0)/8.0))); )
    update_stuff();
}

extern int bv_auto;
int bv_auto_needed_by_iso = 0;
int bv_auto_needed_by_shutter = 0;
int bv_auto_needed_by_aperture = 0;

PROP_HANDLER( PROP_ISO )
{
    if (!CONTROL_BV) lensinfo_set_iso(buf[0]);
    #ifndef CONFIG_500D
    else if (buf[0] && !gui_menu_shown() && ISO_ADJUSTMENT_ACTIVE)
    {
        bv_set_rawiso(buf[0]);
        bv_auto_needed_by_iso = 0;
    }
    #endif
    bv_auto_update();
    lens_display_set_dirty();
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_ISO_AUTO )
{
    uint32_t raw = *(uint32_t *) buf;

    #ifdef CONFIG_5D2
    if (expsim==2) raw = *(uint8_t*)(MEM(0x1D78) + 0x5C);
    #endif

    lens_info.raw_iso_auto = raw;
    lens_info.iso_auto = RAW2VALUE(iso, raw);

    update_stuff();
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_BV )
{
    uint32_t raw_iso = ((uint8_t*)buf)[1];

    #ifdef CONFIG_5D2
    if (expsim==2) raw_iso = *(uint8_t*)(MEM(0x1D78) + 0x5C);
    #endif

    if (raw_iso)
    {
        lens_info.raw_iso_auto = raw_iso;
        lens_info.iso_auto = RAW2VALUE(iso, raw_iso);
        update_stuff();
    }
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_SHUTTER )
{
    if (!CONTROL_BV) lensinfo_set_shutter(buf[0]);
    #ifndef CONFIG_500D
    else if (buf[0] && !gui_menu_shown()
        && ABS(buf[0] - lens_info.raw_shutter) > 4 // some cameras may attempt to round shutter value to 1/2 or 1/3 stops
        )
    {
        bv_set_rawshutter(buf[0]);
        bv_auto_needed_by_shutter = 0;
    }
    #endif
    bv_auto_update();
    lens_display_set_dirty();
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_APERTURE2 )
{
    if (!CONTROL_BV) lensinfo_set_aperture(buf[0]);
    #ifndef CONFIG_500D
    else if (buf[0] && !gui_menu_shown())
    {
        bv_set_rawaperture(buf[0]);
        bv_auto_needed_by_aperture = 0;
    }
    #endif
    bv_auto_update();
    lens_display_set_dirty();
    return prop_cleanup( token, property );
}

#if !defined(CONFIG_5D2) && !defined(CONFIG_50D)
PROP_HANDLER( PROP_APERTURE ) // for Tv mode
{
    if (!CONTROL_BV) lensinfo_set_aperture(buf[0]);
    lens_display_set_dirty();
    return prop_cleanup( token, property );
}
PROP_HANDLER( PROP_SHUTTER_ALSO ) // for Av mode
{
    if (!CONTROL_BV) lensinfo_set_shutter(buf[0]);
    lens_display_set_dirty();
    return prop_cleanup( token, property );
}
#endif

PROP_HANDLER( PROP_AE )
{
    const uint32_t value = *(uint32_t *) buf;
    lens_info.ae = (int8_t)value;
    update_stuff();
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_WB_MODE_LV )
{
    const uint32_t value = *(uint32_t *) buf;
    lens_info.wb_mode = value;
    return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_WBS_GM)
{
    const int8_t value = *(int8_t *) buf;
    lens_info.wbs_gm = value;
    return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_WBS_BA)
{
    const int8_t value = *(int8_t *) buf;
    lens_info.wbs_ba = value;
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_WB_KELVIN_LV )
{
    const uint32_t value = *(uint32_t *) buf;
    lens_info.kelvin = value;
    return prop_cleanup( token, property );
}

uint16_t custom_wb_gains[26];
PROP_HANDLER(PROP_CUSTOM_WB)
{
    memcpy(custom_wb_gains, buf, sizeof(custom_wb_gains));
    const uint16_t * gains = (uint16_t *) buf;
    lens_info.WBGain_R = gains[16];
    lens_info.WBGain_G = gains[18];
    lens_info.WBGain_B = gains[19];
    return prop_cleanup( token, property );
}

void lens_set_custom_wb_gains(int gain_R, int gain_G, int gain_B)
{
    // normalize: green gain should be always 1
    //~ gain_G = COERCE(gain_G, 4, 32000);
    //~ gain_R = COERCE(gain_R * 1024 / gain_G, 128, 32000);
    //~ gain_B = COERCE(gain_B * 1024 / gain_G, 128, 32000);
    //~ gain_G = 1024;

    gain_G = COERCE(gain_G, 128, 8192);
    gain_R = COERCE(gain_R, 128, 8192);
    gain_B = COERCE(gain_B, 128, 8192);

    // round off a bit to get nice values in menu
    gain_R = ((gain_R + 8) / 16) * 16;
    gain_B = ((gain_B + 8) / 16) * 16;
    
    custom_wb_gains[16] = gain_R;
    custom_wb_gains[18] = gain_G;
    custom_wb_gains[19] = gain_B;
    prop_request_change(PROP_CUSTOM_WB, custom_wb_gains, sizeof(custom_wb_gains));

    int mode = WB_CUSTOM;
    prop_request_change(PROP_WB_MODE_LV, &mode, 4);
    prop_request_change(PROP_WB_MODE_PH, &mode, 4);
}

#define LENS_GET(param) \
int lens_get_##param() \
{ \
    return lens_info.param; \
} 

LENS_GET(iso)
LENS_GET(shutter)
LENS_GET(aperture)
LENS_GET(ae)
LENS_GET(kelvin)
LENS_GET(wbs_gm)
LENS_GET(wbs_ba)

#define LENS_SET(param) \
void lens_set_##param(int value) \
{ \
    int raw = VALUE2RAW(param,value); \
    if (raw >= 0) lens_set_raw##param(raw); \
}

LENS_SET(iso)
LENS_SET(shutter)
LENS_SET(aperture)

void
lens_set_kelvin(int k)
{
    k = COERCE(k, KELVIN_MIN, KELVIN_MAX);
    int mode = WB_KELVIN;

    if (k > 10000 || k < 2500) // workaround for 60D; out-of-range values are ignored in photo mode
    {
        int lim = k > 10000 ? 10000 : 2500;
        prop_request_change(PROP_WB_KELVIN_PH, &lim, 4);
        msleep(10);
    }

    prop_request_change(PROP_WB_MODE_LV, &mode, 4);
    prop_request_change(PROP_WB_KELVIN_LV, &k, 4);
    prop_request_change(PROP_WB_MODE_PH, &mode, 4);
    prop_request_change(PROP_WB_KELVIN_PH, &k, 4);
    msleep(10);
}

void
lens_set_kelvin_value_only(int k)
{
    k = COERCE(k, KELVIN_MIN, KELVIN_MAX);

    if (k > 10000 || k < 2500) // workaround for 60D; out-of-range values are ignored in photo mode
    {
        int lim = k > 10000 ? 10000 : 2500;
        prop_request_change(PROP_WB_KELVIN_PH, &lim, 4);
        msleep(10);
    }

    prop_request_change(PROP_WB_KELVIN_LV, &k, 4);
    prop_request_change(PROP_WB_KELVIN_PH, &k, 4);
    msleep(10);
}

void split_iso(int raw_iso, int* analog_iso, int* digital_gain)
{
    if (!raw_iso) { *analog_iso = 0; *digital_gain = 0; return; }
    int rounded = ((raw_iso+3)/8) * 8;
    *analog_iso = COERCE(rounded, 72, 112); // analog ISO range: 100-3200
    *digital_gain = raw_iso - *analog_iso;
}

void iso_components_update()
{
    split_iso(lens_info.raw_iso, &lens_info.iso_analog_raw, &lens_info.iso_digital_ev);

    lens_info.iso_equiv_raw = lens_info.raw_iso;

    if (lens_info.iso_equiv_raw && LVAE_DISP_GAIN && !CONTROL_BV && is_movie_mode())
    {
        // display gain gets recorded, consider it as ISO digital gain
        lens_info.iso_equiv_raw = lens_info.iso_equiv_raw + (gain_to_ev_x8(LVAE_DISP_GAIN) - 80);
    }

    extern int highlight_recover;
    extern int default_shad_gain;
    if (highlight_recover)
    {
        int G = (gain_to_ev_x8(get_new_shad_gain()) - gain_to_ev_x8(default_shad_gain));
        lens_info.iso_equiv_raw = lens_info.iso_equiv_raw + G;
        lens_info.iso_digital_ev = G;
    }
}

void update_stuff()
{
    calc_dof( &lens_info );
    //~ if (gui_menu_shown()) lens_display_set_dirty();
    if (movie_log) mvr_update_logfile( &lens_info, 0 ); // do not force it
    iso_components_update();
}

PROP_HANDLER( PROP_LV_LENS )
{
    const struct prop_lv_lens * const lv_lens = (void*) buf;
    lens_info.focal_len    = bswap16( lv_lens->focal_len );
    lens_info.focus_dist    = bswap16( lv_lens->focus_dist );

    uint32_t lrswap = SWAP_ENDIAN(lv_lens->lens_rotation);
    uint32_t lsswap = SWAP_ENDIAN(lv_lens->lens_step);

    lens_info.lens_rotation = *((float*)&lrswap);
    lens_info.lens_step = *((float*)&lsswap);
    
    static unsigned old_focus_dist = 0;
    if (lv && old_focus_dist && lens_info.focus_dist != old_focus_dist)
    {
        if (get_zoom_overlay_trigger_by_focus_ring()) zoom_overlay_set_countdown(300);
        idle_wakeup_reset_counters(-11);
        lens_display_set_dirty();
    }
    old_focus_dist = lens_info.focus_dist;

    update_stuff();
    
    return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_LAST_JOB_STATE )
{
    const uint32_t state = *(uint32_t*) buf;
    lens_info.job_state = state;
    DEBUG("job state: %d", state);
    mirror_locked = 0;
    return prop_cleanup( token, property );
}

PROP_HANDLER(PROP_HALF_SHUTTER)
{
    update_stuff();
    lens_display_set_dirty();
    //~ bv_auto_update();
    return prop_cleanup( token, property );
}

static void 
movielog_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Movie Logging : %s",
        movie_log ? "ON" : "OFF"
    );
}
static struct menu_entry lens_menus[] = {
#ifndef CONFIG_50D
    {
        .name = "Movie logging",
        .priv = &movie_log,
        .select = menu_binary_toggle,
        .display = movielog_display,
        .help = "Save metadata for each movie, e.g. MVI_1234.LOG",
        .essential = 1,
    },
#endif
};

#ifndef CONFIG_FULLFRAME
static void cropinfo_display( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Crop Factor Display : %s",
        crop_info ? "ON,35mm eq." : "OFF"
    );
    menu_draw_icon(x, y, MNI_BOOL_LV(crop_info));
}
static struct menu_entry tweak_menus[] = {
    {
        .name = "Crop Factor Display",
        .priv = &crop_info,
        .select = menu_binary_toggle,
        .display = cropinfo_display,
        .help = "Display the 35mm equiv. focal length including crop factor."
    }
};
#endif

static void
lens_init( void* unused )
{
    lens_sem = create_named_semaphore( "lens_info", 1 );
    focus_done_sem = create_named_semaphore( "focus_sem", 1 );
    //~ job_sem = create_named_semaphore( "job", 1 ); // seems to cause lockups
    menu_add("Movie", lens_menus, COUNT(lens_menus));
#ifndef CONFIG_FULLFRAME
    menu_add("Tweaks", tweak_menus, COUNT(tweak_menus));
#endif

    lens_info.lens_rotation = 0.1;
    lens_info.lens_step = 1.0;
}

INIT_FUNC( "lens", lens_init );


// picture style, contrast...
// -------------------------------------------

PROP_HANDLER(PROP_PICTURE_STYLE)
{
    const uint32_t raw = *(uint32_t *) buf;
    lens_info.raw_picstyle = raw;
    lens_info.picstyle = get_prop_picstyle_index(raw);
    return prop_cleanup( token, property );
}

extern struct prop_picstyle_settings picstyle_settings[];

// get contrast/saturation/etc from the current picture style

#define LENS_GET_FROM_PICSTYLE(param) \
int \
lens_get_##param() \
{ \
    int i = lens_info.picstyle; \
    if (!i) return -10; \
    return picstyle_settings[i].param; \
} \

#define LENS_GET_FROM_OTHER_PICSTYLE(param) \
int \
lens_get_from_other_picstyle_##param(int picstyle_index) \
{ \
    return picstyle_settings[picstyle_index].param; \
} \

// set contrast/saturation/etc in the current picture style (change is permanent!)
#define LENS_SET_IN_PICSTYLE(param,lo,hi) \
void \
lens_set_##param(int value) \
{ \
    if (value < lo || value > hi) return; \
    int i = lens_info.picstyle; \
    if (!i) return; \
    picstyle_settings[i].param = value; \
    prop_request_change(PROP_PICSTYLE_SETTINGS(i), &picstyle_settings[i], 24); \
} \

LENS_GET_FROM_PICSTYLE(contrast)
LENS_GET_FROM_PICSTYLE(sharpness)
LENS_GET_FROM_PICSTYLE(saturation)
LENS_GET_FROM_PICSTYLE(color_tone)

LENS_GET_FROM_OTHER_PICSTYLE(contrast)
LENS_GET_FROM_OTHER_PICSTYLE(sharpness)
LENS_GET_FROM_OTHER_PICSTYLE(saturation)
LENS_GET_FROM_OTHER_PICSTYLE(color_tone)

LENS_SET_IN_PICSTYLE(contrast, -4, 4)
LENS_SET_IN_PICSTYLE(sharpness, 0, 7)
LENS_SET_IN_PICSTYLE(saturation, -4, 4)
LENS_SET_IN_PICSTYLE(color_tone, -4, 4)


void SW1(int v, int wait)
{
    //~ int unused;
    //~ ptpPropButtonSW1(v, 0, &unused);
    prop_request_change(PROP_REMOTE_SW1, &v, 2);
    if (wait) msleep(wait);
}

void SW2(int v, int wait)
{
    //~ int unused;
    //~ ptpPropButtonSW2(v, 0, &unused);
    prop_request_change(PROP_REMOTE_SW2, &v, 2);
    if (wait) msleep(wait);
}

/** exposure primitives (the "clean" way, via properties) */

bool prop_set_rawaperture(unsigned aperture)
{
    lens_wait_readytotakepic(64);
    aperture = COERCE(aperture, 8, 200);
    prop_request_change( PROP_APERTURE, &aperture, 4 );
    msleep(100);
    return (unsigned int) get_prop(PROP_APERTURE2) == aperture;
}

bool prop_set_rawshutter(unsigned shutter, int coerce)
{
    lens_wait_readytotakepic(64);
    if (coerce) shutter = COERCE(shutter, 16, 160); // 30s ... 1/8000
    prop_request_change( PROP_SHUTTER, &shutter, 4 );
    msleep(20);
    return (unsigned int) get_prop(PROP_SHUTTER) == shutter;
}

bool prop_set_rawiso(unsigned iso)
{
    lens_wait_readytotakepic(64);
    if (iso) iso = COERCE(iso, get_htp() ? 80 : 72, 136); // ISO 100-25600
    prop_request_change( PROP_ISO, &iso, 4 );
    msleep(20);
    return (unsigned int) get_prop(PROP_ISO) == iso;
}

/** Exposure primitives (the "dirty" way, via BV control, bypasses protections) */

void bv_update_lensinfo()
{
    if (CONTROL_BV) // sync lens info and camera properties with overriden values
    {
        lensinfo_set_iso(CONTROL_BV_ISO);
        lensinfo_set_shutter(CONTROL_BV_TV);
        lensinfo_set_aperture(CONTROL_BV_AV);
    }
}

void bv_update_props()
{
    if (CONTROL_BV) // sync lens info and camera properties with overriden values
    {
        prop_set_rawiso(CONTROL_BV_ISO);
        prop_set_rawshutter(CONTROL_BV_TV, 1);
        prop_set_rawaperture(CONTROL_BV_AV);
    }
}

extern int bv_iso;
extern int bv_tv;
extern int bv_av;

bool bv_set_rawshutter(unsigned shutter)
{
    CONTROL_BV_TV = bv_tv = shutter; bv_update_lensinfo();
    //~ NotifyBox(2000, "%d > %d?", raw2shutter_ms(shutter), 1000/video_mode_fps); msleep(400);
    if (is_movie_mode() && raw2shutter_ms(shutter+1) > 1000/video_mode_fps) return 0;
    return shutter != 0;
}
bool bv_set_rawiso(unsigned iso) 
{ 
    if (iso >= 72 && iso <= 128) // 100-12800
    {
        CONTROL_BV_ISO = bv_iso = iso; bv_update_lensinfo();  
        return 1;
    }
    else
    {
        return 0;
    }
}
bool bv_set_rawaperture(unsigned aperture) 
{ 
    if (aperture >= lens_info.raw_aperture_min && aperture <= lens_info.raw_aperture_max) 
    { 
        CONTROL_BV_AV = bv_av = aperture; bv_update_lensinfo(); 
        return 1; 
    }
    else
    {
        return 0;
    }
}

int bv_auto_should_enable()
{
    if (!bv_auto) return 0;
    if (!lv) return 0;

    extern int bulb_ramp_calibration_running; 
    if (bulb_ramp_calibration_running) 
        return 0; // temporarily disable BV mode to make sure display gain will work
    
    // cameras without manual exposure control
    #if defined(CONFIG_50D) || defined(CONFIG_500D) || defined(CONFIG_1100D)
    if (is_movie_mode()) return 1;
    else return 0;
    #endif

    if (LVAE_DISP_GAIN) // compatibility problem, disable it
        return 0;

    // extra ISO values in movie mode
    if (is_movie_mode() && (bv_auto_needed_by_iso || bv_auto_needed_by_shutter || bv_auto_needed_by_aperture)) 
        return 1;
    
    // temporarily cancel it in photo mode
    //~ if (!is_movie_mode() && get_halfshutter_pressed())
        //~ return 0;
    
    // underexposure bug with manual lenses in M mode
    #if defined(CONFIG_60D)
    if (shooting_mode == SHOOTMODE_M && 
        !lens_info.name[0] && 
        lens_info.raw_iso != 0 && 
        lens_info.raw_shutter >= 93 // otherwise the image will be dark, better turn off ExpSim
    )
        return 1;
    #endif

    return 0;
}

void bv_auto_update_do()
{
    if (!bv_auto) return;
    take_semaphore(lens_sem, 0);
    if (bv_auto_should_enable()) bv_enable();
    else bv_disable();
    lens_display_set_dirty();
    give_semaphore(lens_sem);
}
void bv_auto_update()
{
    if (!bv_auto) return;
    
    extern int ml_started;
    if (!ml_started) return;
    
    fake_simple_button(MLEV_BV_AUTO_UPDATE);
}

/** Camera control functions */
bool lens_set_rawaperture( int aperture)
{
    bv_auto_needed_by_aperture = !prop_set_rawaperture(aperture); // first try to set via property
    bv_auto_update(); // auto flip between "BV" or "normal"
    if (bv_auto_should_enable() || CONTROL_BV) return bv_set_rawaperture(aperture);
    return !bv_auto_needed_by_aperture;
}

bool lens_set_rawiso( int iso )
{
    bv_auto_needed_by_iso = !prop_set_rawiso(iso); // first try to set via property
    bv_auto_update(); // auto flip between "BV" or "normal"
    if (bv_auto_should_enable() || CONTROL_BV) return bv_set_rawiso(iso);
    return !bv_auto_needed_by_iso;
}

bool lens_set_rawshutter( int shutter )
{
    bv_auto_needed_by_shutter = !prop_set_rawshutter(shutter,1); // first try to set via property
    bv_auto_update(); // auto flip between "BV" or "normal"
    if (bv_auto_should_enable() || CONTROL_BV) return bv_set_rawshutter(shutter);
    return !bv_auto_needed_by_shutter;
}


bool lens_set_ae( int ae )
{
    prop_request_change( PROP_AE, &ae, 4 );
    msleep(10);
    return get_prop(PROP_AE) == ae;
}

void lens_set_drivemode( int dm )
{
    lens_wait_readytotakepic(64);
    prop_request_change( PROP_DRIVE, &dm, 4 );
    msleep(10);
}

void lens_set_wbs_gm(int value)
{
    value = COERCE(value, -9, 9);
    prop_request_change(PROP_WBS_GM, &value, 4);
}

void lens_set_wbs_ba(int value)
{
    value = COERCE(value, -9, 9);
    prop_request_change(PROP_WBS_BA, &value, 4);
}

// Functions to change camera settings during bracketing
// They will check the operation and retry if necessary
// Used for HDR bracketing
bool hdr_set_rawiso(int iso)
{
    for (int i = 0; i < 10; i++)
        if (prop_set_rawiso(iso))
            return 1;
    return 0;
}

bool hdr_set_rawshutter(int shutter)
{
    for (int i = 0; i < 10; i++)
    {
        if (prop_set_rawshutter(shutter, 0))
            return 1;
    }
    return 0;
}

bool hdr_set_ae(int ae)
{
    for (int i = 0; i < 10; i++)
        if (lens_set_ae(ae))
            return 1;
    return 0;
}
