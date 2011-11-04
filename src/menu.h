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


#define MENU_FONT	FONT(FONT_LARGE,COLOR_WHITE,COLOR_BLACK)
#define MENU_FONT_SEL	get_menu_font_sel()

int get_menu_font_sel();

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
	void			(*select_auto)(
		void *			priv
	);
	void			(*display)(
		void *			priv,
		int			x,
		int			y,
		int			selected
	);
	int essential; // set to 1 to show it in simple mode
	const char * help;
	const char * name; // for now it's used only for context help; will be used for display too.
};


struct menu
{
	struct menu *		next;
	struct menu *		prev;
	const char *		name;
	struct menu_entry *	children;
	int			selected;
	int icon;
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

extern void menu_ternary_toggle(void* priv);
extern void menu_ternary_toggle_reverse(void* priv);
extern void menu_quaternary_toggle(void* priv);
extern void menu_quaternary_toggle_reverse(void* priv);
extern void menu_quinternary_toggle(void* priv);
extern void menu_quinternary_toggle_reverse(void* priv);


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

void menu_draw_icon(int x, int y, int type, int arg);

#define MNI_NONE -1
#define MNI_OFF 0
#define MNI_ON 1
#define MNI_AUTO 2
#define MNI_WARNING 3
#define MNI_PERCENT 4
#define MNI_ACTION 5
#define MNI_BOOL(x) ((x) ? MNI_ON : MNI_OFF)
#define MNI_BOOL_AUTO(x) ((x) == 1 ? MNI_ON : (x) == 0 ? MNI_OFF : MNI_AUTO)
#define MNI_BOOL_GDR(x) ((x) ? ( get_global_draw() ? MNI_ON : MNI_WARNING ) : MNI_OFF), "GlobalDraw is OFF"
#define MNI_BOOL_GDR_EXPSIM(x) ((x) ? ( get_global_draw() && expsim ? MNI_ON : MNI_WARNING ) : MNI_OFF), !get_global_draw() ? "GlobalDraw is OFF" : "ExpSim is OFF"
#define MNI_BOOL_LV(x) ((x) ? ( lv ? MNI_ON : MNI_WARNING ) : MNI_OFF), "This option works only in LiveView" 
#endif
