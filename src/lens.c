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

#ifdef CONFIG_1100D
#include "disable-this-module.h"
#endif

void update_stuff();
void draw_ml_topbar();
void draw_ml_bottombar();

CONFIG_INT("shutter.display.degrees", shutter_display_degrees, 0);

CONFIG_INT("movie.log", movie_log, 0);
#ifndef CONFIG_FULLFRAME
#define SENSORCROPFACTOR 1.6
CONFIG_INT("crop.info", crop_info, 0);
#endif

static struct semaphore * lens_sem;
static struct semaphore * focus_done_sem;
//~ static struct semaphore * job_sem;


struct lens_info lens_info = {
	.name		= "NO LENS NAME"
};


/** Compute the depth of field for the current lens parameters.
 *
 * This relies heavily on:
 * 	http://en.wikipedia.org/wiki/Circle_of_confusion
 * The CoC value given there is 0.019 mm, but we need to scale things
 */
static void
calc_dof(
	struct lens_info * const info
)
{
	const uint32_t		coc = 19; // 1/1000 mm
	const uint32_t		fd = info->focus_dist * 10; // into mm
	const uint32_t		fl = info->focal_len; // already in mm

	// If we have no aperture value then we can't compute any of this
	// Not all lenses report the focus distance
	if( fl == 0 || info->aperture == 0 )
	{
		info->dof_near		= 0;
		info->dof_far		= 0;
		info->hyperfocal	= 0;
		return;
	}

	const uint32_t		fl2 = fl * fl;

	// The aperture is scaled by 10 and the CoC by 1000,
	// so scale the focal len, too.  This results in a mm measurement
	const unsigned H = ((1000 * fl2) / (info->aperture  * coc)) * 10;
	info->hyperfocal = H;

	// If we do not have the focus distance, then we can not compute
	// near and far parameters
	if( fd == 0 )
	{
		info->dof_near		= 0;
		info->dof_far		= 0;
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


const char *
lens_format_dist(
	unsigned		mm
)
{
	static char dist[ 32 ];

	if( mm > 100000 ) // 100 m
		snprintf( dist, sizeof(dist),
			"%3d.%1dm",
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
			"%4dcm",
			mm / 10
		);

	return dist;
}

/********************************************************************
*                                                                   *
*  aj_lens_format_dist() -    Private version of ML lens.c routine  *                                      
*                                                                   *
********************************************************************/

char *aj_lens_format_dist( unsigned mm)
{
   static char dist[ 32 ];

   if( mm > 100000 ) // 100 m
   {
      snprintf( dist, sizeof(dist), "Infty");
   }
   else if( mm > 10000 ) // 10 m
   {
      snprintf( dist, sizeof(dist), "%2d.%01dm", mm / 1000,  (mm % 1000) / 100);
   }
   else	if( mm >  1000 ) // 1 m 
   {
      snprintf( dist, sizeof(dist), "%1d.%02dm", mm / 1000, (mm % 1000)/10 );
   }
   else
   {
      snprintf( dist, sizeof(dist),"%3dcm", mm / 10 );
   }

   return (dist);
} /* end of aj_lens_format_dist() */

void
update_lens_display()
{
	draw_ml_topbar();

#ifdef CONFIG_500D
	if (lv_disp_mode == 0 && !LV_BOTTOM_BAR_DISPLAYED || EXT_MONITOR_CONNECTED)
#else
	if (lv_disp_mode == 0 || flicker_being_killed() || EXT_MONITOR_CONNECTED)
#endif
	{
		if (!get_halfshutter_pressed())
			draw_ml_bottombar();
	}
}

int raw2shutter_ms(int raw_shutter)
{
	if (!raw_shutter) return 0;
    return (int) roundf(powf(2.0, (152.0 - raw_shutter)/8.0) / 4.0);
}
int shutter_ms_to_raw(int shutter_ms)
{
	if (shutter_ms == 0) return 160;
	return (int) roundf(152 - log2f(shutter_ms * 4) * 8);
}

void shave_color_bar(int x0, int y0, int w, int h, int shaved_color);

void draw_ml_bottombar()
{
#ifdef CONFIG_500D
	if (lv_disp_mode != 0 && !LV_BOTTOM_BAR_DISPLAYED)
		return;
#endif
		
	if (lv_disp_mode == 0 && LV_BOTTOM_BAR_DISPLAYED)
	{
		fake_simple_button(MLEV_HIDE_CANON_BOTTOM_BAR);
		return;
	}
	
	struct lens_info *	info = &lens_info;

	int bg = TOPBAR_BGCOLOR;
	if (is_movie_mode()) bg = COLOR_BLACK;
	//~ unsigned font	= FONT(FONT_MED, COLOR_WHITE, bg);
	//~ unsigned font_err	= FONT( FONT_MED, COLOR_RED, bg);
	//~ unsigned Font	= FONT(FONT_LARGE, COLOR_WHITE, bg);
	//~ unsigned height	= fontspec_height( font );
	unsigned text_font = FONT(FONT_LARGE, COLOR_WHITE, bg);
	
	unsigned bottom = 480;
	if (ext_monitor_rca) bottom = 420;
	if (hdmi_code == 2) bottom = 405;
	if (hdmi_code == 5) bottom = 525;
	unsigned x = 420;
	//~ unsigned y = 480 - height - 10;
	//~ if (ext_monitor_hdmi) y += recording ? -100 : 200;
	
     unsigned int x_origin = 50;
     unsigned int y_origin = bottom - 30;

	if (hdmi_code == 5) x_origin = 150;

		// MODE
		
			bmp_printf( text_font, x_origin - 50, y_origin,
				"%s         ",
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
      if (info->aperture)
      {
		  text_font = FONT(FONT_LARGE,COLOR_WHITE,bg);
		  unsigned med_font = FONT(FONT_MED,COLOR_WHITE,bg);

		  char focal[32];
		  snprintf(focal, sizeof(focal), "%d",
				   crop_info ? (int)roundf((double)info->focal_len * SENSORCROPFACTOR) : info->focal_len);

		  bmp_printf( text_font, x_origin, y_origin, focal );

		if (info->aperture < 100)
		{
			  bmp_printf( text_font, 
						  x_origin + 74 + font_med.width + font_large.width - 4, 
						  y_origin, 
						  ".");
			  bmp_printf( text_font, 
						  x_origin + 74 + font_med.width  , 
						  y_origin, 
						  "%d", info->aperture / 10);
			  bmp_printf( text_font, 
						  x_origin + 74 + font_med.width + font_large.width * 2 - 8, 
						  y_origin, 
						  "%d", info->aperture % 10);
		}
		else
			  bmp_printf( text_font, 
						  x_origin + 74 + font_med.width  , 
						  y_origin, 
						  "%d    ", info->aperture / 10) ;

		  bmp_printf( med_font, 
					  x_origin + font_large.width * strlen(focal) - 3, 
					  bottom - font_med.height + 1, 
					  crop_info ? "eq" : "mm");

		  bmp_printf( med_font, 
					  x_origin + 74 + 2  , 
					  y_origin - 3, 
					  "f") ;
      }
  
      /*******************
      *  SHUTTER         *
      *******************/

      int shutter_x10 = raw2shutter_ms(info->raw_shutter) / 100;
      int shutter_reciprocal = (int) roundf(4000.0 / powf(2.0, (152 - info->raw_shutter)/8.0));
      if (shutter_reciprocal > 100) shutter_reciprocal = 10 * (int)roundf(shutter_reciprocal / 10.0);
      if (shutter_reciprocal > 1000) shutter_reciprocal = 100 * (int)roundf(shutter_reciprocal / 100.0);
      char shutter[32];
      if (info->raw_shutter == 0) snprintf(shutter, sizeof(shutter), "    ");
      else if (shutter_x10 >= 350) snprintf(shutter, sizeof(shutter), "BULB");
      else if (shutter_x10 <= 3) snprintf(shutter, sizeof(shutter), "%d  ", shutter_reciprocal);
      else if (shutter_x10 % 10 && shutter_x10 < 30) snprintf(shutter, sizeof(shutter), "%d.%d ", shutter_x10 / 10, shutter_x10 % 10);
      else snprintf(shutter, sizeof(shutter), "%d  ", (int)roundf(shutter_x10 / 10.0));

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
           int focal_35mm = (int)roundf((double)info->focal_len * SENSORCROPFACTOR);
           if (shutter_reciprocal > focal_35mm * 15/10) 
              fgs = FONT(FONT_LARGE,COLOR_GREEN1,bg); // very good
           else if (shutter_reciprocal < focal_35mm / 2) 
              fgs = FONT(FONT_LARGE,COLOR_RED,bg); // you should have really steady hands
           else if (shutter_reciprocal < focal_35mm) 
              fgs = FONT(FONT_LARGE,COLOR_YELLOW,bg); // OK, but be careful
      }

	text_font = FONT(FONT_LARGE,fgs,bg);
	if (is_movie_mode() && shutter_display_degrees)
	{
		snprintf(shutter, sizeof(shutter), "%d  ", shutter_degrees);
		bmp_printf( text_font, 
					x_origin + 143 + font_med.width*2  , 
					y_origin, 
					shutter);

		text_font = FONT(FONT_MED,fgs,bg);

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

		text_font = FONT(FONT_MED,fgs,bg);

		bmp_printf( text_font, 
				x_origin + 143 + 1  , 
				y_origin - 3, 
				shutter_x10 > 3 ? "  " : "1/");
	}

      /*******************
      *  ISO             *
      *******************/

      // good iso = 160 320 640 1250  - according to bloom video  
      //  http://www.youtube.com/watch?v=TNNqUm_nSXk&NR=1

      text_font = FONT(
      FONT_LARGE, 
      is_native_iso(lens_info.iso) ? COLOR_YELLOW :
      is_lowgain_iso(lens_info.iso) ? COLOR_GREEN2 : COLOR_RED,
      bg);

		if (info->iso)
			bmp_printf( text_font, 
					  x_origin + 250  , 
					  y_origin, 
					  "%d   ", info->iso) ;
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

      if (ISO_ADJUSTMENT_ACTIVE) return;
      
		// kelvins
      text_font = FONT(
      FONT_LARGE, 
      0x13, // orange
      bg);

		if( info->wb_mode == WB_KELVIN )
			bmp_printf( text_font, x_origin + 350, y_origin,
				"%5dK ",
				info->kelvin
			);
		else
			bmp_printf( text_font, x_origin + 350, y_origin,
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
		
		x += font_large.width * 6;
		int gm = lens_info.wbs_gm;
		int ba = lens_info.wbs_ba;
		if (gm) 
			bmp_printf(
				FONT(ba ? FONT_MED : FONT_LARGE, COLOR_WHITE, gm > 0 ? COLOR_GREEN2 : 14 /* magenta */),
				x, y_origin + (ba ? -3 : 0), 
				"%d", ABS(gm)
			);

		if (ba) 
			bmp_printf(
				FONT(gm ? FONT_MED : FONT_LARGE, COLOR_WHITE, ba > 0 ? 12 : COLOR_BLUE), 
				x, y_origin + (gm ? 14 : 0), 
				"%d", ABS(ba));


      /*******************
      *  Focus distance  *
      *******************/

      text_font = FONT(FONT_LARGE, COLOR_WHITE, bg );   // WHITE

      if(lens_info.focus_dist)
          bmp_printf( text_font, 
                  x_origin + 495  , 
                  y_origin, 
                  aj_lens_format_dist( lens_info.focus_dist * 10 )
                );

		//~ y += height;
/*		x = 500;
		bmp_printf( font, x+12, y,
			"%s",
			info->focus_dist == 0xFFFF
				? "Infnty"
				: lens_format_dist( info->focus_dist * 10 )
		);*/
		
/*
		x += 50;

		bmp_printf( font, x, y,
#ifndef CONFIG_FULLFRAME
			crop_info ? "%deq/%d.%d  " : "%d f/%d.%d  ",
			crop_info ? (int)roundf((double)info->focal_len * SENSORCROPFACTOR) : info->focal_len,
#else
			"%d f/%d.%d  ",
			info->focal_len,
#endif
			info->aperture / 10,
			info->aperture % 10
		);

		x += 120;
		if( info->shutter )
			bmp_printf( font, x, y,
				"1/%d  ",
				info->shutter
			);
		else
			bmp_printf( font_err, x, y,
				"1/0x%02x",
				info->raw_shutter
			);

		x += 80;
		if( info->iso )
			bmp_printf( font, x, y,
				"ISO%5d",
				info->iso
			);
		else
			bmp_printf( font, x, y,
				"ISO Auto"
			);

		x += 110;
		if( info->wb_mode == WB_KELVIN )
			bmp_printf( font, x, y,
				"%5dK",
				info->kelvin
			);
		else
			bmp_printf( font, x, y,
				"%s",
				(lens_info.wb_mode == 0 ? "AutoWB" : 
				(lens_info.wb_mode == 1 ? "Sunny " :
				(lens_info.wb_mode == 2 ? "Cloudy" : 
				(lens_info.wb_mode == 3 ? "Tungst" : 
				(lens_info.wb_mode == 4 ? "CFL   " : 
				(lens_info.wb_mode == 5 ? "Flash " : 
				(lens_info.wb_mode == 6 ? "Custom" : 
				(lens_info.wb_mode == 8 ? "Shade " :
				 "unk"))))))))
			);
		x += font_med.width * 7;

		int gm = lens_info.wbs_gm;
		if (gm) bmp_printf(font, x, y, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
		else bmp_printf(font, x, y, "  ");

		x += font_med.width * 2;
		int ba = lens_info.wbs_ba;
		if (ba) bmp_printf(font, x, y, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
		else bmp_printf(font, x, y, "  ");

		bmp_printf( text_font, x_origin + 590, y_origin,
			"%s%d.%d",
			AE_VALUE < 0 ? "-" : AE_VALUE > 0 ? "+" : " ",
			ABS(AE_VALUE) / 8,
			mod(ABS(AE_VALUE) * 10 / 8, 10)
		);
*/

	  text_font = FONT(FONT_LARGE, COLOR_CYAN, bg ); 

	  bmp_printf( text_font, 
				  x_origin + 600 + font_large.width * 2 - 4, 
				  y_origin, 
				  ".");
	  bmp_printf( text_font, 
				  x_origin + 600, 
				  y_origin, 
				  "%s%d", 
					AE_VALUE < 0 ? "-" : AE_VALUE > 0 ? "+" : " ",
					ABS(AE_VALUE) / 8
				  );
	  bmp_printf( text_font, 
				  x_origin + 600 + font_large.width * 3 - 8, 
				  y_origin, 
				  "%d",
					mod(ABS(AE_VALUE) * 10 / 8, 10)
				  );

	if (hdmi_code == 2) shave_color_bar(40,370,640,16,bg);
	if (hdmi_code == 5) shave_color_bar(75,480,810,22,bg);

	extern int display_gain;
	if (display_gain)
	{
		text_font = FONT(FONT_LARGE, COLOR_WHITE, COLOR_BLACK ); 
		int gain_ev = gain_to_ev(display_gain) - 10;
		bmp_printf( text_font, 
				  x_origin + 590, 
				  y_origin - font_large.height, 
				  "+%dEV", 
				  gain_ev);
	}

}

void shave_color_bar(int x0, int y0, int w, int h, int shaved_color)
{
	// shave the bottom bar a bit :)
	int i,j;
	for (i = y0; i < y0 + h; i++)
	{
		int new_color = bmp_getpixel(x0+1,i);
		for (j = x0; j < x0+w; j++)
			if (bmp_getpixel(j,i) == shaved_color)
				bmp_putpixel(j,i,new_color);
		//~ bmp_putpixel(x0+5,i,COLOR_RED);
	}
}

void draw_ml_topbar()
{
	int bg = TOPBAR_BGCOLOR;
	unsigned f = audio_meters_are_drawn() && !get_halfshutter_pressed() ? FONT_SMALL : FONT_MED;
	unsigned font	= FONT(f, COLOR_WHITE, bg);
	//~ unsigned font_err	= FONT( f, COLOR_RED, bg);
	//~ unsigned Font	= FONT(FONT_LARGE, COLOR_WHITE, bg);
	//~ unsigned height	= fontspec_height( font );
	
	unsigned x = 80;
	unsigned y = 0;
	if (ext_monitor_rca) y = 10;
	if (hdmi_code == 2) y = 15;

	if (hdmi_code == 5)
	{
		if (gui_menu_shown())
		{
			x = 120;
			y = 45;
		}
		else
		{
			x = 100;
			y = 30;
			if (f == FONT_SMALL) return;
		}
	}

	bmp_printf( font, x, y,
		"DISP %d", get_disp_mode()
	);

	x += 80;

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

	x += 60;
	bmp_printf( font, x, y, (char*)get_picstyle_shortname(lens_info.raw_picstyle));

	x += 80;
	#ifdef CONFIG_60D
		bmp_printf( font, x, y,"T=%d BAT=%d", efic_temp, GetBatteryLevel());
	#else
		bmp_printf( font, x, y,"T=%d", efic_temp);
	#endif

	display_clock();
	free_space_show();

	x = 550;
	bmp_printf( font, x, y,
		"[%d]  ",
		avail_shot
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
	NotifyBox(1000, "Focus error :("); msleep(1000);
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
		card_led_on();
		if (lv && !mirror_down && tft_status == 0 && lens_info.job_state == 0)
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
		card_led_off();
	}

	if (get_zoom_overlay_mode()==2) zoom_overlay_set_countdown(300);
	if (get_global_draw()) draw_ml_bottombar();
	idle_wakeup_reset_counters(-10);
	
	if (lv_focus_error) { lv_focus_error = 0; return 0; }
	return 1;
}

void lens_wait_readytotakepic(int wait)
{
	extern int burst_count;
	int i;
	for (i = 0; i < wait * 10; i++)
	{
		if (lens_info.job_state <= 0xA && burst_count > 0) break;
		msleep(100);
	}
}

int mirror_locked = 0;
void mlu_lock_mirror_if_needed() // called by lens_take_picture
{
	if (get_mlu() && !lv)
	{
		if (!mirror_locked)
		{
			mirror_locked = 1;
			call("Release");
			msleep(1000);
		}
	}
}

int af_button_assignment = -1;
// to preview AF patterns
void assign_af_button_to_halfshutter()
{
	take_semaphore(lens_sem, 0);
	msleep(10);
	lens_wait_readytotakepic(64);
	if (af_button_assignment == -1) af_button_assignment = cfn_get_af_button_assignment();
	cfn_set_af_button(AF_BTN_HALFSHUTTER);
	msleep(10);
	give_semaphore(lens_sem);
}

// to prevent AF
void assign_af_button_to_star_button()
{
	take_semaphore(lens_sem, 0);
	msleep(10);
	lens_wait_readytotakepic(64);
	if (af_button_assignment == -1) af_button_assignment = cfn_get_af_button_assignment();
	cfn_set_af_button(AF_BTN_STAR);
	msleep(10);
	give_semaphore(lens_sem);
}

void restore_af_button_assignment()
{
	take_semaphore(lens_sem, 0);
	msleep(10);
	lens_wait_readytotakepic(64);
	if (af_button_assignment == -1) return;
	cfn_set_af_button(af_button_assignment);
	msleep(10);
	give_semaphore(lens_sem);
}


int
lens_take_picture(
	int			wait, 
	int allow_af
)
{
	if (!allow_af) assign_af_button_to_star_button();
	take_semaphore(lens_sem, 0);
	lens_wait_readytotakepic(64);
	
	mlu_lock_mirror_if_needed();

	call( "Release", 0 );
	
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
	struct lens_info *	info,
	int			force
)
{
	if( mvr_logfile == INVALID_PTR )
		return;

	static unsigned last_iso;
	static unsigned last_shutter;
	static unsigned last_aperture;
	static unsigned last_focal_len;
	static unsigned last_focus_dist;
	static unsigned last_wb_mode;
	static unsigned last_kelvin;
	static int last_wbs_gm;
	static int last_wbs_ba;
	static unsigned last_picstyle;
	static int last_contrast;
	static int last_saturation;
	static int last_sharpness;
	static int last_color_tone;

	// Check if nothing changed and not forced.  Do not write.
	if( !force
	&&  last_iso		== info->iso
	&&  last_shutter	== info->shutter
	&&  last_aperture	== info->aperture
	&&  last_focal_len	== info->focal_len
	&&  last_focus_dist	== info->focus_dist
	&&  last_wb_mode	== info->wb_mode
	&&  last_kelvin		== info->kelvin
	&&  last_wbs_gm		== info->wbs_gm
	&&  last_wbs_ba		== info->wbs_ba
	&&  last_picstyle	== info->picstyle
	&&  last_contrast	== lens_get_contrast()
	&&  last_saturation	== lens_get_saturation()
	&&  last_sharpness	== lens_get_sharpness()
	&&  last_color_tone	== lens_get_color_tone()
	)
		return;

	// Record the last settings so that we know if anything changes
	last_iso	= info->iso;
	last_shutter	= info->shutter;
	last_aperture	= info->aperture;
	last_focal_len	= info->focal_len;
	last_focus_dist	= info->focus_dist;
	last_wb_mode = info->wb_mode;
	last_kelvin = info->kelvin;
	last_wbs_gm = info->wbs_gm; 
	last_wbs_ba = info->wbs_ba;
	last_picstyle = info->picstyle;
	last_contrast = lens_get_contrast(); 
	last_saturation = lens_get_saturation();
	last_sharpness = lens_get_sharpness();
	last_color_tone = lens_get_color_tone();

	struct tm now;
	LoadCalendarFromRTC( &now );

	my_fprintf(
		mvr_logfile,
		"%02d:%02d:%02d,%d,%d,%d.%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d\n",
		now.tm_hour,
		now.tm_min,
		now.tm_sec,
		info->iso,
		info->shutter,
		info->aperture / 10,
		info->aperture % 10,
		info->focal_len,
		info->focus_dist,
		info->wb_mode, 
		info->wb_mode == WB_KELVIN ? info->kelvin : 0,
		info->wbs_gm, 
		info->wbs_ba,
		get_picstyle_name(info->raw_picstyle),
		lens_get_contrast(),
		lens_get_saturation(), 
		lens_get_sharpness(), 
		lens_get_color_tone()
	);
}

/** Create a logfile for each movie.
 * Record a logfile with the lens info for each movie.
 */
static void
mvr_create_logfile(
	unsigned		event
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
	snprintf(name, sizeof(name), CARD_DRIVE "DCIM/%03dCANON/MVI_%04d.LOG", folder_number, file_number);

	FIO_RemoveFile(name);
	mvr_logfile = FIO_CreateFile( name );
	if( mvr_logfile == INVALID_PTR )
	{
		bmp_printf( FONT_LARGE, 0, 40,
			"Unable to create movie log! fd=%x",
			(unsigned) mvr_logfile
		);

		return;
	}

	struct tm now;
	LoadCalendarFromRTC( &now );

	my_fprintf( mvr_logfile,
		"Start: %4d/%02d/%02d %02d:%02d:%02d\n",
		now.tm_year + 1900,
		now.tm_mon + 1,
		now.tm_mday,
		now.tm_hour,
		now.tm_min,
		now.tm_sec
	);

	my_fprintf( mvr_logfile, "Lens: %s\n", lens_info.name );

	my_fprintf( mvr_logfile, "%s\n",
		"Frame,ISO,Shutter,Aperture,Focal_Len,Focus_Dist,WB_Mode,Kelvin,WBShift_GM,WBShift_BA,PicStyle,Contrast,Saturation,Sharpness,ColorTone"
	);

	// Force the initial values to be written
	mvr_update_logfile( &lens_info, 1 );
}



static inline uint16_t
bswap16(
	uint16_t		val
)
{
	return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}

PROP_HANDLER( PROP_MVR_REC_START )
{
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

PROP_HANDLER( PROP_ISO )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_iso = raw;
	lens_info.iso = RAW2VALUE(iso, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_ISO_AUTO )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_iso_auto = raw;
	lens_info.iso_auto = RAW2VALUE(iso, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_SHUTTER_ALSO )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_shutter = raw;
	lens_info.shutter = RAW2VALUE(shutter, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

PROP_HANDLER( PROP_APERTURE2 )
{
	const uint32_t raw = *(uint32_t *) buf;
	lens_info.raw_aperture = raw;
	lens_info.aperture = RAW2VALUE(aperture, raw);
	update_stuff();
	return prop_cleanup( token, property );
}

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

void update_stuff()
{
	calc_dof( &lens_info );
	lens_display_set_dirty();
	if (movie_log) mvr_update_logfile( &lens_info, 0 ); // do not force it
}

PROP_HANDLER( PROP_LV_LENS )
{
	const struct prop_lv_lens * const lv_lens = (void*) buf;
	lens_info.focal_len	= bswap16( lv_lens->focal_len );
	lens_info.focus_dist	= bswap16( lv_lens->focus_dist );

	uint32_t lrswap = SWAP_ENDIAN(lv_lens->lens_rotation);
	uint32_t lsswap = SWAP_ENDIAN(lv_lens->lens_step);

	lens_info.lens_rotation = *((float*)&lrswap);
	lens_info.lens_step = *((float*)&lsswap);
	
	static unsigned old_focus_dist = 0;
	if (lv && old_focus_dist && lens_info.focus_dist != old_focus_dist)
	{
		if (get_zoom_overlay_mode()==2) zoom_overlay_set_countdown(300);
		idle_wakeup_reset_counters(-11);
		menu_set_dirty(); // force a redraw
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
		.help = "Save metadata for each movie, e.g. MVI_1234.LOG"
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
