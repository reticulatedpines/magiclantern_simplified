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
#include <string.h>

#include "ime_base.h"

static CONFIG_INT("ime.base.method", ime_base_method, 0);

static int32_t ime_base_method_count = 0;
static char ime_base_test_text[100];
static t_ime_handler ime_base_methods[IME_MAX_METHODS];


/* this function has to be public so that other modules can register file types for viewing this file */
uint32_t ime_base_register(t_ime_handler *handler)
{
    /* locking without semaphores as this function may get called even before our own init routine */
    uint32_t old_int = cli();
    
    if(ime_base_method_count < IME_MAX_METHODS)
    {
        ime_base_methods[ime_base_method_count] = *handler;
        ime_base_method_count++;
    }
    
    sei(old_int);
    return 0;
}

void *ime_base_start (char *caption, char *text, int32_t max_length, int32_t codepage, int32_t charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int32_t x, int32_t y, int32_t w, int32_t h )
{
    if(ime_base_method < ime_base_method_count)
    {
        return ime_base_methods[ime_base_method].start(caption, text, max_length, codepage, charset, update_cbr, done_cbr, x, y, w, h);
    }
    
    strncpy(text, "No IME available", max_length);
    return NULL;
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

static IME_UPDATE_FUNC(ime_base_test_update)
{
    return IME_OK;
}

static IME_DONE_FUNC(ime_base_test_done)
{
    for(int32_t loops = 0; loops < 50; loops++)
    {
        bmp_printf(FONT_MED, 30, 120, "ime_base: done: <%s>, %d", text, status);
        msleep(100);
    }
    return IME_OK;
}

static MENU_SELECT_FUNC(ime_base_test_any)
{
    strncpy(ime_base_test_text, "This is a test", sizeof(ime_base_test_text));
    ime_base_start("Any charset", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, IME_CHARSET_ANY, &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_alpha)
{
    strncpy(ime_base_test_text, "Test", sizeof(ime_base_test_text));
    ime_base_start("Alpha", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, IME_CHARSET_ALPHA, &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_num)
{
    strncpy(ime_base_test_text, "16384", sizeof(ime_base_test_text));
    ime_base_start("Numeric", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, IME_CHARSET_NUMERIC, &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_alnum)
{
    strncpy(ime_base_test_text, "Test1", sizeof(ime_base_test_text));
    ime_base_start("Alphanumeric", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, (IME_CHARSET_ALPHA | IME_CHARSET_NUMERIC), &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_hex)
{
    strncpy(ime_base_test_text, "0xDEADBEEF", sizeof(ime_base_test_text));
    ime_base_start("Hexadecimal", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, (IME_CHARSET_HEX), &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_punct)
{
    strncpy(ime_base_test_text, ".,", sizeof(ime_base_test_text));
    ime_base_start("Punctuation", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, IME_CHARSET_PUNCTUATION, &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_math)
{
    strncpy(ime_base_test_text, "1+2*3", sizeof(ime_base_test_text));
    ime_base_start("Math", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, IME_CHARSET_MATH, &ime_base_test_update, &ime_base_test_done, 0, 0, 0, 0);
}
static MENU_SELECT_FUNC(ime_base_test_file)
{
    strncpy(ime_base_test_text, "FILE.DAT", sizeof(ime_base_test_text));
    ime_base_start("Filename", ime_base_test_text, sizeof(ime_base_test_text), IME_UTF8, IME_CHARSET_FILENAME, NULL, &ime_base_test_done, 0, 0, 0, 0);
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
                .name = "Test: All chars",
                .select = &ime_base_test_any,
            },
            {
                .name = "Test: Alpha",
                .select = &ime_base_test_alpha,
            },
            {
                .name = "Test: Numeric",
                .select = &ime_base_test_num,
            },
            {
                .name = "Test: Hexadecimal",
                .select = &ime_base_test_hex,
            },
            {
                .name = "Test: Alphanumeric",
                .select = &ime_base_test_alnum,
            },
            {
                .name = "Test: Punctuation",
                .select = &ime_base_test_punct,
            },
            {
                .name = "Test: Math",
                .select = &ime_base_test_math,
            },
            {
                .name = "Test: File",
                .select = &ime_base_test_file,
            },
            MENU_EOL,
        },
    }
};

static unsigned int ime_base_init()
{
    strcpy(ime_base_test_text, "test");
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

MODULE_CONFIGS_START()
    MODULE_CONFIG(ime_base_method)
MODULE_CONFIGS_END()
#endif
