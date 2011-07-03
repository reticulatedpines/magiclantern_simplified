/** \file
 * Magic Lantern menu help
 */
/*
 * Copyright (C) 2011 Alex Dumitrache <broscutamaker@gmail.com>
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
#include "version.h"
#include "bmp.h"
#include "gui.h"
#include "config.h"
#include "property.h"
#include "lens.h"
#include "font.h"
#include "menu.h"

extern int menu_help_active;
int current_page = 1;
extern int help_pages;

void menu_help_show_page(int page)
{
	menu_help_active = 1;
	char path[100];
	snprintf(path, sizeof(path), "B:/doc/cam/page-%03d.bmp", page);
	struct bmp_file_t * doc = (void*) -1;
	doc = bmp_load(path, 1);
	if (doc)
	{
		bmp_draw_scaled_ex(doc, 0, 0, 720, 480, 0, 0);
		FreeMemory(doc);
	}
	else
	{
		bmp_printf(FONT_MED, 0, 0, "Could not load help page %s\nPlease unzip 'doc' directory on your SD card.", path);
	}
}

void menu_help_redraw()
{
	menu_help_show_page(current_page);
}

void menu_help_next_page()
{
	current_page = mod(current_page, help_pages) + 1;
	menu_help_active = 1;
}

void menu_help_prev_page()
{
	current_page = mod(current_page - 2, help_pages) + 1;
	menu_help_active = 1;
}

void menu_help_go_to_page(int page)
{
	current_page = page;
	menu_help_active = 1;
}

void
menu_help_section_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
) // hack: we don't have direct access to menu entry struct
{
	extern struct menu_entry help_menus[];
	extern int help_menus_num;
	int i;
	for (i = 0; i < help_menus_num; i++)
	{
		if (help_menus[i].priv == priv)
		{
			bmp_printf(
				selected ? MENU_FONT_SEL : MENU_FONT,
				x, y,
				"%s",
				(const char*) help_menus[i].name
			);
			menu_draw_icon(x, y, MNI_ACTION, 0);
			return;
		}
	}
}


void menu_help_go_to_label(char* label)
{
	int page = 1;
	
	// hack: use config file routines to parse menu index file
	extern int config_file_size, config_file_pos;
	extern char* config_file_buf;
	config_file_buf = read_entire_file("B:/doc/cam/menuidx.dat", &config_file_size);
	config_file_pos = 0;

	char line_buf[ 100 ];

	while( read_line(line_buf, sizeof(line_buf) ) >= 0 )
	{
		char* name = line_buf+4;
		//~ bmp_printf(FONT_MED, 0, 0, "'%s' '%s'"  , name, label);
		//~ msleep(200);
		if(!strcmp(name, label))
		{
			//~ bmp_printf(FONT_MED, 0, 0, "'%s' :) "  , name);
			//~ msleep(1000);
			page = atoi(line_buf);
			break;
		}
	}
	free_dma_memory(config_file_buf);
	config_file_buf = 0;
	
	current_page = page;
	menu_help_active = 1;
}
