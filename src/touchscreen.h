/** \file
 * 
 *  Extends touch capability to Magic Lantern. Monitors touches
 *  and reports faked button presses to gui_main_task. This is used
 *  to interact with the ML menu via touch screen. Some gesture
 *  recognition implemented too. We can detect up to 2 fingers at once
 *  on the screen.
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


//~ canon's CBR function for touch events.
void touch_cbr_canon(int,int,int,int);
