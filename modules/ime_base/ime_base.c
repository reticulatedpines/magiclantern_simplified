/**
 * 
 */

#ifndef _ime_base_c_
#define _ime_base_c_

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "ime_base.h"

unsigned int ime_base_method = 0;
unsigned int ime_base_method_count = 0;


static char text_buffer[100];

t_ime_handler ime_base_methods[IME_MAX_METHODS];

/* this function has to be public so that other modules can register file types for viewing this file */
unsigned int ime_base_register(t_ime_handler *handler)
{
    /* locking without semaphores as this function may get called even before our own init routine */
    unsigned int old_int = cli();
    
    if(ime_base_method_count < IME_MAX_METHODS)
    {
        ime_base_methods[ime_base_method_count] = *handler;
        ime_base_method_count++;
    }
    
    sei(old_int);
    return 0;
}

unsigned int ime_base_start (char *caption, char *text, int max_length, int codepage, int charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int x, int y, int w, int h )
{
    if(ime_base_method < ime_base_method_count)
    {
        unsigned int ret = 0;
        
        ret = ime_base_methods[ime_base_method].start(caption, text, max_length, codepage, charset, update_cbr, done_cbr, x, y, w, h);
        return ret;
    }
    
    strncpy(text, "No IME available", max_length);
    return IME_ERR_UNAVAIL;
}

static MENU_UPDATE_FUNC(ime_base_method_update)
{
    if(ime_base_method < ime_base_method_count)
    {
        MENU_SET_VALUE(ime_base_methods[ime_base_method].name);
    }
    else
    {
        MENU_SET_VALUE("None");
    }
}

static MENU_SELECT_FUNC(ime_base_method_select)
{
    if (delta < 0 && ime_base_method > 0)
    {
        ime_base_method--;
    }
    if (delta > 0 && ime_base_method < ime_base_method_count - 1)
    {
        ime_base_method++;
    }
}

IME_UPDATE_FUNC(ime_base_test_update)
{
    //bmp_printf(FONT_MED, 30, 90, "ime_base: CBR: <%s>, %d, %d", text, caret_pos, selection_length);
    return IME_OK;
}

IME_DONE_FUNC(ime_base_test_done)
{
    for(int loops = 0; loops < 50; loops++)
    {
        bmp_printf(FONT_MED, 30, 120, "ime_base: done: <%s>, %d", text, status);
        msleep(100);
    }
    return IME_OK;
}

static MENU_SELECT_FUNC(ime_base_test)
{
    strcpy(text_buffer, "test");
    
    ime_base_start("Enter something:", text_buffer, sizeof(text_buffer), IME_UTF8, IME_CHARSET_ANY, &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}

static MENU_SELECT_FUNC(ime_base_config)
{
    if(ime_base_method < ime_base_method_count && ime_base_methods[ime_base_method].configure != NULL)
    {
        return ime_base_methods[ime_base_method].configure();
    }
}

static struct menu_entry ime_base_menu[] =
{
    {
        .name = "IME System",
        .submenu_width = 710,
        .help = "Input Method Editor System",
        .children =  (struct menu_entry[]) {
            {
                .name = "Method",
                .priv = &ime_base_method,
                .select = &ime_base_method_select,
                .update = &ime_base_method_update,
            },
            {
                .name = "Configure method",
                .select = &ime_base_config,
            },
            {
                .name = "Test method",
                .select = &ime_base_test,
            },
            MENU_EOL,
        },
    }
};

static unsigned int ime_base_init()
{
    menu_add("IME", ime_base_menu, COUNT(ime_base_menu));
    return 0;
}

static unsigned int ime_base_deinit()
{
    return 0;
}



MODULE_INFO_START()
    MODULE_INIT(ime_base_init)
    MODULE_DEINIT(ime_base_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "IME Base System")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "g3gg0")
MODULE_STRINGS_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

#endif
