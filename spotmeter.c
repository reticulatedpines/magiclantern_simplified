/** \file
 * Measure the intensity of the center few pixels and
 * display a numeric value at the bottom of the screen.
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
#include "tasks.h"
#include "menu.h"
#include "config.h"

CONFIG_INT( "spotmeter.size",		spotmeter_size,	5 );
CONFIG_INT( "spotmeter.draw",		spotmeter_draw, 0 );


static void
spotmeter_menu_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	int * draw_ptr = priv;

	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"Spotmeter:  %s",
		*draw_ptr ? "ON " : "OFF"
	);
}


static void
spotmeter_clear_display( void * priv )
{
	gui_stop_menu();
	bmp_fill( 0x0, 0, 0, 1080, 480 );
}


static struct menu_entry spotmeter_menus[] = {
	{
		.priv			= "Clear screen",
		.select			= spotmeter_clear_display,
		.display		= menu_print,
	},
	{
		.priv			= &spotmeter_draw,
		.select			= menu_binary_toggle,
		.display		= spotmeter_menu_display,
	},
};


static void
spotmeter_task( void * priv )
{
	menu_add( "Video", spotmeter_menus, COUNT(spotmeter_menus) );

	msleep( 1000 );
	while(1)
	{
		// Draw a few pixels to indicate the center
		if( !spotmeter_draw )
		{
			msleep( 1000 );
			continue;
		}

		msleep( 100 );

		struct vram_info *	vram = get_yuv422_vram();

		if( !vram->vram )
			continue;

		const unsigned		width = vram->width;
		const unsigned		pitch = vram->pitch;
		const unsigned		height = vram->height;
		const unsigned		dx = spotmeter_size;
		unsigned		sum = 0;
		unsigned		x, y;

		bmp_fill(
			0xA,
			width/2 - dx,
			height/2 - dx,
			2*dx + 1,
			4
		);

		bmp_fill(
			0xA,
			width/2 - dx,
			height/2 + dx,
			2*dx + 1,
			4
		);

		// Sum the values around the center
		for( y = height/2 - dx ; y <= height/2 + dx ; y++ )
		{
			for( x = width/2 - dx ; x <= width/2 + dx ; x++ )
				sum += (vram->vram[ x + y * pitch ]) & 0xFF00;
		}

		sum /= (2 * dx + 1) * (2 * dx + 1);

		// Scale to 100%
		const unsigned		scaled = (100 * sum) / 65536;
		bmp_printf(
			FONT_MED,
			300,
			400,
			"%3d%%",
			scaled
		);
	}
}

TASK_CREATE( __FILE__, spotmeter_task, 0, 0x1f, 0x1000 );
			
