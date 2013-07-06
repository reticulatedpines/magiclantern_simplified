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

#define CHAR_OK     0x01
#define CHAR_CANCEL 0x02
#define CHAR_DEL    0x08

extern int menu_redraw_blocked;

unsigned char *ime_std_string = NULL;
unsigned char *ime_std_caption = NULL;

int ime_std_char_x = 30;
int ime_std_char_y = 200;
int ime_std_char_w = 650;
int ime_std_char_h = 280;
int ime_std_str_x = 30;
int ime_std_str_y = 100;
int ime_std_str_w = 650;
int ime_std_caption_x = 27;
int ime_std_caption_y = 20;

int ime_std_text_fg = COLOR_WHITE;
int ime_std_text_bg = COLOR_BLACK;    


/* currently selected character */
unsigned int ime_std_selection = 0;
/* which string position to change */
unsigned int ime_std_caret_pos = 0;

unsigned int ime_std_max_length = 0;
unsigned int ime_std_active = 0;
unsigned int ime_std_valid = 0;
unsigned int ime_std_current_sel = 0;
unsigned int ime_std_charset_type = 0;


/* should that be located in files? */
unsigned char ime_std_charset_alpha_upper[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
unsigned char ime_std_charset_alpha_lower[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
unsigned char ime_std_charset_numeric[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
unsigned char ime_std_charset_punctuation[] = {'.', ',', ':', ';', '!', '?', '`', '\'', '\"'};
unsigned char ime_std_charset_math[] = { '+', '-', '*', '/', '^' };
unsigned char ime_std_charset_mail[] = { '@', '.', '-', '_', '!', '#', '$', '%', '&', '\'', '*', '+', '-', '/', '=', '?', '^', '_', '`', '{', '|', '}' };
unsigned char ime_std_charset_func[] = { CHAR_DEL, CHAR_OK, CHAR_CANCEL };

/* number of characters in current set, currently set 0 */
unsigned int ime_std_charsetnum = 0;
unsigned int ime_std_charcount = sizeof(ime_std_charset_alpha_upper);
unsigned char *ime_std_charset = ime_std_charset_alpha_upper;

/* number of characters in specific sets */
unsigned int ime_std_charset_types[] = { IME_CHARSET_ALPHA | IME_CHARSET_FILENAME, IME_CHARSET_ALPHA, IME_CHARSET_NUMERIC, IME_CHARSET_PUNCTUATION, IME_CHARSET_MATH, IME_CHARSET_MAIL, IME_CHARSET_ANY };
unsigned char *ime_std_charsets[] = { ime_std_charset_alpha_upper, ime_std_charset_alpha_lower, ime_std_charset_numeric, ime_std_charset_punctuation, ime_std_charset_math, ime_std_charset_mail, ime_std_charset_func };
int ime_std_charcounts[] = { sizeof(ime_std_charset_alpha_upper), sizeof(ime_std_charset_alpha_lower), sizeof(ime_std_charset_numeric), sizeof(ime_std_charset_punctuation), sizeof(ime_std_charset_math), sizeof(ime_std_charset_mail), sizeof(ime_std_charset_func) };
unsigned char *ime_std_charsetnames[] = { (unsigned char *)"upper", (unsigned char *)"lower", (unsigned char *)"num", (unsigned char *)"punct", (unsigned char *)"math", (unsigned char *)"mail" };

unsigned int ime_std_font = FONT_LARGE;
  
unsigned int ime_std_ret = IME_ERR_UNKNOWN;
t_ime_update_cbr ime_std_update_cbr = NULL;
t_ime_done_cbr ime_std_done_cbr = NULL;

static void ime_std_draw_charset(unsigned int charset, unsigned int font, int charnum, int selected, int x, int y, int w, int h)
{
    /* total border width */
    int border = 2;
    int visible_chars = (w - x - border) / fontspec_font(font)->width;
    int first_char = (int)(charnum - visible_chars / 2);
    int last_char = (int)(charnum + visible_chars / 2);
    
    /* display only a window with the nearest characters around selection */
    if(last_char >= ime_std_charcounts[charset] - 1)
    {
        last_char = ime_std_charcounts[charset] - 1;
        first_char = MAX(0, last_char - visible_chars);
    }
    
    if(first_char <= 0)
    {
        first_char = 0;
        last_char = MIN(visible_chars, ime_std_charcounts[charset] - 1);
    }
    
    if((unsigned int)h > fontspec_font(font)->height)
    {
        /* only the selected charset line gets a black bar */
        if(selected == 1)
        {
            bmp_fill(COLOR_BLACK, x, y, fontspec_font(font)->width * visible_chars + border, fontspec_font(font)->height + border);
        }
     
        int x_pos = x + border / 2;
        
        for(int pos = 0; pos <= (last_char - first_char); pos++)
        {
            char buf[16];
            unsigned char selected_char = ime_std_charsets[charset][first_char + pos];
            
            if(selected_char == CHAR_OK)
            {
                strcpy(buf, " OK ");
            }
            else if(selected_char == CHAR_CANCEL)
            {
                strcpy(buf, " Cancel ");
            }
            else if(selected_char == CHAR_DEL)
            {
                strcpy(buf, " Del ");
            }
            else
            {
                buf[0] = selected_char;
                buf[1] = '\000';
            }
            
            /* print character or string */
            if((selected == 1) && (first_char + pos == charnum))
            {
                bmp_printf(FONT(font,COLOR_BLACK, COLOR_ORANGE), x_pos, y + border/2, "%s", buf);
            }
            else if(selected == 2)
            {
                bmp_printf(SHADOW_FONT(FONT(font,COLOR_GRAY(20), COLOR_BLACK)), x_pos, y + border/2, "%s", buf);
            }
            else
            {
                bmp_printf(SHADOW_FONT(font), x_pos, y + border/2, "%s", buf);
            }
            
            /* advance to next position */
            x_pos += strlen(buf) * fontspec_font(font)->width;
        }
    }
}

static void ime_std_draw()
{
    int color_bg = COLOR_GRAY(10);
    
    /* if text isnt valid, print red */
    if(ime_std_valid)
    {
        ime_std_text_fg = COLOR_WHITE;
    }
    else
    {
        ime_std_text_fg = COLOR_RED;
    }
    
    BMP_LOCK
    (
        bmp_draw_to_idle(1);
            
        /* uniform background */
        bmp_fill(color_bg, 0, 0, 720, 480);
        
        /* some nice borders */
        for(int width = 0; width < 5; width++)
        {
            draw_line(0+width, 0+width, 720-width, 0+width, COLOR_GRAY(20));
            draw_line(0+width, 0+width, 0+width, 480-width, COLOR_GRAY(20));
            draw_line(720-width, 0+width, 720-width, 480-width, COLOR_GRAY(2));
            draw_line(0+width, 480-width, 720-width, 480-width, COLOR_GRAY(2));
        }
        
        /* print title text */
        if(ime_std_caption)
        {
            bfnt_printf(ime_std_caption_x, ime_std_caption_y, COLOR_WHITE, color_bg, "%s", ime_std_caption);
            draw_line(ime_std_caption_x, ime_std_caption_y + 40, ime_std_str_w + ime_std_str_x, ime_std_caption_y + 40, COLOR_ORANGE);
            draw_line(ime_std_caption_x + 5, ime_std_caption_y + 40 + 3, ime_std_str_w + ime_std_str_x + 5, ime_std_caption_y + 40 + 3, COLOR_ORANGE);
        }
        
        /* draw a dark background for the text line */
        bmp_fill(COLOR_BLACK, ime_std_str_x, ime_std_str_y, ime_std_str_w, font_large.height + 6);

        /* orange rectangle around that dark text box background */
        bmp_draw_rect(COLOR_ORANGE, ime_std_str_x, ime_std_str_y, ime_std_str_w, font_large.height + 6);
        
        /* now the text and right after the caret */
        bmp_printf(FONT(FONT_LARGE,ime_std_text_fg, ime_std_text_bg), ime_std_str_x + 3, ime_std_str_y + 3, "%s", ime_std_string);
        bmp_printf(SHADOW_FONT(FONT(FONT_LARGE,COLOR_BLACK, COLOR_ORANGE)), ime_std_str_x + 3 + font_large.width * ime_std_caret_pos, ime_std_str_y + 3, "_");

        /* orange rectangle around that dark characters box background */
        bmp_fill(COLOR_GRAY(20), ime_std_char_x - 1, ime_std_char_y - 1, ime_std_char_w + 2, fontspec_font(ime_std_font)->height * COUNT(ime_std_charcounts) + 4);
        bmp_draw_rect(COLOR_ORANGE, ime_std_char_x - 1, ime_std_char_y - 1, ime_std_char_w + 2, fontspec_font(ime_std_font)->height * COUNT(ime_std_charcounts) + 4);
        
        /* print charsets that are selected */
        for(int set = 0; set < COUNT(ime_std_charcounts); set++)
        {
            int offset_y = fontspec_font(ime_std_font)->height * set;
            if(ime_std_charset_types[set] & ime_std_charset_type)
            {
                ime_std_draw_charset(set, ime_std_font, ime_std_selection, set == ime_std_charsetnum, ime_std_char_x + 2, ime_std_char_y + offset_y, ime_std_char_w - 1, ime_std_char_h - offset_y);
            }
            else
            {
                ime_std_draw_charset(set, ime_std_font, ime_std_selection, 2, ime_std_char_x + 2, ime_std_char_y + offset_y, ime_std_char_w - 1, ime_std_char_h - offset_y);
            }
        }
        
        bmp_draw_to_idle(0);
        bmp_idle_copy(1,0);
    )
}

static int ime_std_select_charset(int charset)
{
    if(charset < COUNT(ime_std_charcounts) && (ime_std_charset_types[charset] & ime_std_charset_type))
    {
        ime_std_charset = ime_std_charsets[charset];
        ime_std_charcount = ime_std_charcounts[charset];
        
        /* make sure that the currently selected character position is available */
        if(ime_std_selection >= ime_std_charcount)
        {
            ime_std_selection = ime_std_charcount - 1;
        }
        
        return 1;
    }
    
    return 0;
}

static unsigned int ime_std_keypress_cbr(unsigned int key)
{
    if (!ime_std_active)
        return 1;
    
    switch (key)
    {
        case MODULE_KEY_WHEEL_UP:
            if(ime_std_caret_pos > 0)
            {
                ime_std_caret_pos--;
            }
            break;
            
        case MODULE_KEY_WHEEL_DOWN:
            if(ime_std_caret_pos < strlen(ime_std_string))
            {
                ime_std_caret_pos++;
            }
            break;
            
        case MODULE_KEY_WHEEL_LEFT:
        case MODULE_KEY_PRESS_LEFT:
            if(ime_std_selection > 0)
            {
                ime_std_selection--;
            }
            break;
            
        case MODULE_KEY_WHEEL_RIGHT:
        case MODULE_KEY_PRESS_RIGHT:
            if(ime_std_selection < ime_std_charcount - 1)
            {
                ime_std_selection++;
            }
            break;
            
        case MODULE_KEY_PRESS_DOWN:
            if(ime_std_charsetnum < COUNT(ime_std_charcounts))
            {
                int set = ime_std_charsetnum + 1;
                
                while(set < COUNT(ime_std_charcounts))
                {
                    /* check the next possible charset */
                    if(ime_std_select_charset(set))
                    {
                        ime_std_charsetnum = set;
                        break;
                    }
                    
                    set++;
                }
            }
            break;
            
        case MODULE_KEY_PRESS_UP:
            if(ime_std_charsetnum > 0)
            {
                int set = ime_std_charsetnum - 1;
                
                while(set >= 0)
                {
                    /* check the next possible charset */
                    if(ime_std_select_charset(set))
                    {
                        ime_std_charsetnum = set;
                        break;
                    }
                    set--;
                }
            }
            break;
            
        case MODULE_KEY_JOY_CENTER:
        case MODULE_KEY_PRESS_SET:
            if(ime_std_selection < ime_std_charcount)
            {
                unsigned char selected_char = ime_std_charset[ime_std_selection];
                
                if(selected_char == CHAR_DEL)
                {
                    /* backspace/del was pressed */
                    //ime_std_caret_pos--;
                    strncpy(&ime_std_string[ime_std_caret_pos], &ime_std_string[ime_std_caret_pos+1], ime_std_max_length - ime_std_caret_pos);
                }
                else if(selected_char == CHAR_OK)
                {
                    /* ok was pressed, set return code and return */
                    ime_std_ret = IME_OK;
                    ime_std_active = 0;
                }
                else if(selected_char == CHAR_CANCEL)
                {
                    /* cancel was pressed, set return code and return */
                    ime_std_ret = IME_CANCEL;
                    ime_std_active = 0;
                }
                else
                {
                    if(ime_std_caret_pos < ime_std_max_length)
                    {
                        ime_std_string[ime_std_caret_pos] = selected_char;
                        ime_std_caret_pos++;
                    }
                }
            }
            break;
            
        default:
            return 0;
    }
    
    /* check update function return code */
    if(ime_std_update_cbr)
    {
        ime_std_valid = (ime_std_update_cbr(ime_std_string, ime_std_caret_pos, 0) == IME_OK);
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

static void ime_std_input(unsigned int unused)
{
    /* stop menu painting */
    menu_redraw_blocked = 1;
    
    /* redraw periodically */
    while(ime_std_active)
    {
        ime_std_draw();
        msleep(250);
    }
    
    /* re-enable menu painting */
    menu_redraw_blocked = 0;
    ime_std_done_cbr(ime_std_ret, ime_std_string);
}

static unsigned int ime_std_start(char *caption, char *text, int max_length, int codepage, int charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int x, int y, int w, int h)
{
    ime_std_string = text;
    ime_std_caption = caption;
    ime_std_caret_pos = strlen(text);
    ime_std_update_cbr = update_cbr;
    ime_std_done_cbr = done_cbr;
    ime_std_active = 1;
    ime_std_valid = 1;
    ime_std_max_length = max_length;
    ime_std_charset_type = charset;
    
    for(int pos = strlen(ime_std_string); pos < max_length; pos++)
    {
        ime_std_string[pos] = '\000';
    }
    
    /* select first charset */
    ime_std_charsetnum = 0;    

    while(ime_std_charsetnum < COUNT(ime_std_charcounts))
    {
        /* check the next possible charset */
        if(ime_std_select_charset(ime_std_charsetnum))
        {
            break;
        }
        
        ime_std_charsetnum++;
    }
    
    /* no valid charset found */
    if(ime_std_charsetnum >= COUNT(ime_std_charcounts))
    {
        return IME_ERR_UNAVAIL;
    }
    
    /* start input system */
    task_create("ime_std_input", 0x1c, 0x1000, ime_std_input, (void*)0);
    
    return IME_OK;
}


static t_ime_handler ime_std_descriptor = 
{
    .name = "ime_std",
    .description = "Standard input method",
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
    MODULE_STRING("Description", "IME Standard module")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Author", "g3gg0")
MODULE_STRINGS_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_KEYPRESS, ime_std_keypress_cbr, 0)
MODULE_CBRS_END()


