/** \file
 * Menu structures and functions.
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
	void			(*display)(
		void *			priv,
		int			x,
		int			y,
		int			selected
	);
};

extern struct menu_entry main_menu;


extern void
menu_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
);


extern void
menu_select(
	struct menu_entry *	entry
);


extern void
menu_add(
	struct menu_entry *	menu,
	struct menu_entry *	new_entry,
	int			count
);


#endif
