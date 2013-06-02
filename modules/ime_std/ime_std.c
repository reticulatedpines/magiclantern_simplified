/**
 * 
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "../ime_base/ime_base.h"

unsigned char *ime_std_string = NULL;

unsigned int ime_std_active = 0;
unsigned int ime_std_valid = 0;
/* currently selected character */
unsigned int ime_std_selection = 0;
/* which string position to change */
unsigned int ime_std_caret_pos = 0;
unsigned int ime_std_current_sel = 0;

/* not sure yet how to deal with UTF-8 exactly, just use this as fixed set ;) */
unsigned char ime_std_charset[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
  
t_ime_update_cbr ime_std_update_cbr = NULL;

static void ime_std_draw()
{
    char selected[10];
    
    if(ime_std_selection < sizeof(ime_std_charset))
    {
        selected[0] = ime_std_charset[ime_std_selection];
        selected[1] = '\000';
    }
    else if(ime_std_selection < sizeof(ime_std_charset) + 1)
    {
        /* backspace */
        sprintf(selected, "<-");
    }
    else
    {
        sprintf(selected, "OK");
    }
    
    bmp_printf(FONT_MED, 30, 50, "ime_std: sel: <- %s -> pos: %d valid: %d  ", selected, ime_std_caret_pos, ime_std_valid);
    bmp_printf(FONT_MED, 30, 70, "ime_std: str: <%s >             ", ime_std_string);
    bmp_printf(SHADOW_FONT(FONT_MED), 30 + font_med.width * (15+ime_std_caret_pos), 70, "_");
}


static unsigned int ime_std_keypress_cbr(unsigned int key)
{
    if (!ime_std_active)
        return 1;
    
    bmp_printf(FONT_MED, 30, 30, "ime_std: key %d      ", key);
    switch (key)
    {
        case MODULE_KEY_PRESS_LEFT:
            if(ime_std_caret_pos > 0)
            {
                ime_std_caret_pos--;
            }
            break;
            
        case MODULE_KEY_PRESS_RIGHT:
            if(ime_std_caret_pos < strlen(ime_std_string) - 1)
            {
                ime_std_caret_pos++;
            }
            break;
            
        case MODULE_KEY_WHEEL_LEFT:
            if(ime_std_selection > 0)
            {
                ime_std_selection--;
            }
            break;
            
        case MODULE_KEY_WHEEL_RIGHT:
            if(ime_std_selection < sizeof(ime_std_charset) + 1)
            {
                ime_std_selection++;
            }
            break;
            
        case MODULE_KEY_JOY_CENTER:
        case MODULE_KEY_PRESS_SET:
            if(ime_std_selection < sizeof(ime_std_charset))
            {
                ime_std_string[ime_std_caret_pos] = ime_std_charset[ime_std_selection];
                ime_std_caret_pos++;
                ime_std_string[ime_std_caret_pos] = '\000';
            }
            else if(ime_std_selection < sizeof(ime_std_charset) + 1)
            {
                /* backspace */
                ime_std_caret_pos--;
                ime_std_string[ime_std_caret_pos] = '\000';
            }
            else
            {
                ime_std_active = 0;
            }
            break;
            
        default:
            return 0;
    }
    
    if(ime_std_update_cbr)
    {
        ime_std_valid = ime_std_update_cbr(ime_std_string, ime_std_caret_pos, 0);
    }
    else
    {
        ime_std_valid = 1;
    }
    
    ime_std_draw();
    return 0;
}

static void ime_std_config()
{
}

static unsigned int ime_std_start(char *text, int max_length, int codepage, int charset, t_ime_update_cbr update, int x, int y, int w, int h)
{
    ime_std_string = text;
    ime_std_caret_pos = strlen(text);
    ime_std_update_cbr = update;
    ime_std_active = 1;
    
    while(ime_std_active)
    {
        ime_std_draw();
        msleep(100);
    }
    
    
    return IME_OK;
}


static t_ime_handler ime_std_descriptor = 
{
    .name = "ime_std",
    .description = "Dummy input method",
    .start = &ime_std_start,
    .configure = &ime_std_config,
};

static unsigned int ime_std_init()
{
    ime_base_register(&ime_std_descriptor);
    return 0;
}

static unsigned int ime_std_deinit()
{
    return 0;
}



MODULE_INFO_START()
    MODULE_INIT(ime_std_init)
    MODULE_DEINIT(ime_std_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "IME dummy module")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "g3gg0")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_KEYPRESS, ime_std_keypress_cbr, 0)
MODULE_CBRS_END()


