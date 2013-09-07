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
#include "menuhelp.h"

extern int menu_help_active;
static int current_page = 1;
static int help_pages = 100; // dummy value, will be updated on the fly

void 
draw_beta_warning()
{
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    bfnt_puts("Magic Lantern", 242, 53, COLOR_WHITE, COLOR_BLACK);

    bmp_printf(FONT_MED | FONT_ALIGN_CENTER, 360, 150, "This is a development snapshot for testing purposes.");

    bmp_printf(FONT_MED | FONT_ALIGN_CENTER, 360, 200, "Please report all bugs at www.magiclantern.fm.");

    bmp_printf(FONT_MED | FONT_ALIGN_CENTER, 360, 250, "Be careful using it for production work.      ");

    bmp_printf(FONT_MED | FONT_ALIGN_CENTER, 360, 300, "Enjoy!");

    big_bmp_printf(FONT_MED,  10,  410,
        "Magic Lantern version : %s\n"
        "Mercurial changeset   : %s\n"
        "Built on %s by %s.",
        build_version,
        build_id,
        build_date,
        build_user);
}

void 
draw_404_page()
{
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    bfnt_puts("404 Undocumented Feature", 10, 20, COLOR_WHITE, COLOR_BLACK);
    
    bmp_printf(FONT_MED, 10, 100, "This feature is probably not yet documented.");
    bmp_printf(FONT_MED, 10, 120, "After all, we are programmers, not tech writers.");

    bmp_printf(FONT_MED, 10, 180, "But... you can simply try it and see what it does.");

    bmp_printf(FONT_MED, 10, 240, "Then, write a short paragraph to describe it,");
    bmp_printf(FONT_MED, 10, 260, "and we will include it in the user guide.");

    bmp_printf(FONT_MED, 10, 320, "Thanks!");

}

void 
draw_help_not_installed_page()
{
    bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

    bfnt_puts("Help files not found", 10, 20, COLOR_WHITE, COLOR_BLACK);
    
    bmp_printf(FONT_MED, 10, 150, "Magic Lantern help files could not be found.              ");

    bmp_printf(FONT_MED, 10, 200, "Make sure all ML files are installed to your card.        ");

    bmp_printf(FONT_MED, 10, 250, "See http://wiki.magiclantern.fm/install for instructions. ");
}

void menu_help_show_page(int page)
{
    menu_help_active = 1;
    
#ifndef CONFIG_RELEASE_BUILD
    if (page == 1) { draw_beta_warning(); return; } // display this instead of the main About page
#endif

    if (page == 0) { draw_404_page(); return; } // help page not found
    if (page == -1) { draw_help_not_installed_page(); return; } // help page not found
    
    char path[100];
    struct bmp_file_t * doc = (void*) -1;

#ifdef CONFIG_HELP_CACHE
    char rpath[30];
    snprintf(rpath, sizeof(rpath), CARD_DRIVE "ML/doc/page-%03d.vrm", page);
    if (load_vram(rpath)==-1)
#endif
    {
        snprintf(path, sizeof(path), CARD_DRIVE "ML/doc/page-%03d.bmh", page);
        doc = bmp_load(path, 1);
        if (!doc)
        {
            snprintf(path, sizeof(path), CARD_DRIVE "ML/doc/page-%03d.bmp", page);
            doc = bmp_load(path, 1);
        }

        if (doc)
        {
            bmp_draw_scaled_ex(doc, 0, 0, 720, 480, 0);
            #ifdef CONFIG_HELP_CACHE
            extern int _bmp_draw_should_stop;
            if (!_bmp_draw_should_stop) save_vram(rpath);
            #endif
            FreeMemory(doc);
        }
        else
        {
            clrscr();
            bmp_printf(FONT_MED, 0, 0, "Could not load help page %s.", path);
        }
    }
}

void menu_help_redraw()
{
    BMP_LOCK( menu_help_show_page(current_page); );
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

void str_make_lowercase(char* s)
{
    while (*s) { *s = tolower(*s); s++; }
}

void menu_help_go_to_label(void* label, int delta)
{
    int page = 0; // if help page won't be found, will show 404
    if (is_menu_selected("Help")) page = 1; // don't show the 404 page in Help menu :P
    
    int size = 0;
    char* buf = (void*)read_entire_file(CARD_DRIVE "ML/doc/menuidx.dat", &size);
    if (!buf || !size) page = -1; // show "help not found" warning
    
    // trim spaces
    char label_adj[100];
    snprintf(label_adj, sizeof(label_adj), "%s", label);
    while (label_adj[strlen(label_adj)-1] == ' ')
    {
        label_adj[strlen(label_adj)-1] = '\0';
    }
    str_make_lowercase(label_adj);

    int prev = -1;
    for (int i = 0; i < size; i++)
    {
        if (buf[i] == '\n')
        {
            buf[i] = 0;
            char* line_buf = &buf[prev+1];

            char* name = line_buf+4;
            str_make_lowercase(name);
            int pagenum = atoi(line_buf);

            if(streq(name, label_adj))
                page = pagenum;
            help_pages = MAX(help_pages, pagenum);

            prev = i;
        }
    }

    free_dma_memory(buf);
    
    current_page = page;
    menu_help_active = 1;
}
