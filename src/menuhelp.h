#ifndef _menuhelp_h_
#define _menuhelp_h_

void menu_help_go_to_page(int page);
void menu_help_go_to_label(void* label, int delta);
void menu_help_section_print(
        void *                  priv,
        int                     x,
        int                     y,
        int                     selected
);

#endif // _menuhelp_h_
