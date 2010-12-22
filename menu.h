/** \file
 * Menu structures and functions.
 *
 * When adding a menu item, the convention in the Magic Lantern menus
 * is that there are 12 characters for the label and up to 7 characters
 * for the value.  The box that is drawn is 540 pixels wide, enough
 * for 19 characters in FONT_LARGE.
 *
 * There is room for 8 entries in the menu.
 *
 * New menus must have a 5 character top level name.
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

#ifndef _menu_h_
#define _menu_h_


#define MENU_FONT	FONT(FONT_LARGE,COLOR_WHITE,COLOR_BG)
#define MENU_FONT_SEL	FONT(FONT_LARGE,COLOR_WHITE,COLOR_BLUE)

struct menu_entry
{
	struct menu_entry *	next;
	struct menu_entry *	prev;
	int			selected;
	void *			priv;
	void			(*select)(
		void *			priv
	);
	void			(*select_reverse)(
		void *			priv
	);
	void			(*display)(
		void *			priv,
		int			x,
		int			y,
		int			selected
	);
};


struct menu
{
	struct menu *		next;
	struct menu *		prev;
	const char *		name;
	struct menu_entry *	children;
	int			selected;
};


extern void
menu_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
);


extern void
menu_binary_toggle(
	void *			priv
);

// also close the menu after selecting an option
extern void menu_binary_toggle_and_close(void * priv);


extern void
menu_select(
	struct menu_entry *	entry
);


extern void
menu_add(
	const char *		name,
	struct menu_entry *	new_entry,
	int			count
);

extern void
menu_init( void );

// mod like in math... x mod n is from 0 to n-1
#define mod(x,m) ((((int)x) % ((int)m) + ((int)m)) % ((int)m))

#endif
