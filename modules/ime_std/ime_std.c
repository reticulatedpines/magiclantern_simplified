/**
 * 
 */

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <string.h>

#include "../ime_base/ime_base.h"

#define CHAR_OK     0x01
#define CHAR_CANCEL 0x02
#define CHAR_DEL    0x08

extern int menu_redraw_blocked;

/* appearance options */
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
unsigned int ime_std_font = FONT_LARGE;

/* thats the charset that is used for various use cases */
#define IME_STD_VAR_CHARSET           3
#define IME_STD_VAR_CHARSET_DEF       ime_std_charset_punctuation
#define IME_STD_VAR_CHARSET_DEF_TYPE  IME_CHARSET_PUNCTUATION

/* should that be located in files? */
unsigned char ime_std_charset_alpha_upper[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
unsigned char ime_std_charset_alpha_lower[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
unsigned char ime_std_charset_numeric[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
unsigned char ime_std_charset_punctuation[] = {'.', ',', ':', ';', '!', '?', '`', '\'', '\"'};
unsigned char ime_std_charset_math[] = { '+', '-', '*', '/', '^' };
unsigned char ime_std_charset_file[] = { '.', ' ', '-', '_' };
unsigned char ime_std_charset_mail[] = { '@', '.', '-', '_', '!', '#', '$', '%', '&', '\'', '*', '+', '-', '/', '=', '?', '^', '_', '`', '{', '|', '}' };
unsigned char ime_std_charset_func[] = { CHAR_DEL, CHAR_OK, CHAR_CANCEL };

/* number of characters in specific sets */
unsigned int ime_std_charset_types[] = { IME_CHARSET_ALPHA | IME_CHARSET_FILENAME, IME_CHARSET_ALPHA, IME_CHARSET_NUMERIC | IME_CHARSET_FILENAME, IME_CHARSET_PUNCTUATION, IME_CHARSET_MATH, IME_CHARSET_MAIL, IME_CHARSET_ANY };
unsigned char *ime_std_charsets[] = { ime_std_charset_alpha_upper, ime_std_charset_alpha_lower, ime_std_charset_numeric, ime_std_charset_punctuation, ime_std_charset_math, ime_std_charset_mail, ime_std_charset_func };
int ime_std_charcounts[] = { sizeof(ime_std_charset_alpha_upper), sizeof(ime_std_charset_alpha_lower), sizeof(ime_std_charset_numeric), sizeof(ime_std_charset_punctuation), sizeof(ime_std_charset_math), sizeof(ime_std_charset_mail), sizeof(ime_std_charset_func) };
unsigned char *ime_std_charsetnames[] = { (unsigned char *)"upper", (unsigned char *)"lower", (unsigned char *)"num", (unsigned char *)"punct", (unsigned char *)"math", (unsigned char *)"var" };


typedef struct
{
    unsigned int active;
    unsigned char *string;
    unsigned int max_length;
    unsigned int selection;
    unsigned char *caption;
    t_ime_update_cbr update_cbr;
    t_ime_done_cbr done_cbr;
    unsigned int caret_pos;
    unsigned int valid;
    
    /* infos about current selected charset */
    unsigned int charset_type;
    unsigned char *charset;
    unsigned int charset_charcount;
    unsigned int charsetnum;
    unsigned int returncode;
} ime_std_ctx_t;

ime_std_ctx_t *ime_std_current_ctx = NULL;


static void ime_std_update(ime_std_ctx_t *ctx)
{
    /* check update function return code */
    if(ctx->update_cbr)
    {
        ctx->valid = (ctx->update_cbr(ctx, ctx->string, ctx->caret_pos, 0) == IME_OK);
    }
    else
    {
        ctx->valid = 1;
    }
}

static void ime_std_draw_charset(ime_std_ctx_t *ctx, unsigned int charset, unsigned int font, int charnum, int selected, int x, int y, int w, int h)
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

static void ime_std_draw(ime_std_ctx_t *ctx)
{
    int color_bg = COLOR_GRAY(10);
    
    /* if text isnt valid, print red */
    if(ctx->valid)
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
        if(ctx->caption)
        {
            bfnt_printf(ime_std_caption_x, ime_std_caption_y, COLOR_WHITE, color_bg, "%s", ctx->caption);
            draw_line(ime_std_caption_x, ime_std_caption_y + 40, ime_std_str_w + ime_std_str_x, ime_std_caption_y + 40, COLOR_ORANGE);
            draw_line(ime_std_caption_x + 5, ime_std_caption_y + 40 + 3, ime_std_str_w + ime_std_str_x + 5, ime_std_caption_y + 40 + 3, COLOR_ORANGE);
        }
        
        /* draw a dark background for the text line */
        bmp_fill(COLOR_BLACK, ime_std_str_x, ime_std_str_y, ime_std_str_w, font_large.height + 6);

        /* orange rectangle around that dark text box background */
        bmp_draw_rect(COLOR_ORANGE, ime_std_str_x, ime_std_str_y, ime_std_str_w, font_large.height + 6);
        
        /* now the text and right after the caret */
        bmp_printf(FONT(FONT_LARGE,ime_std_text_fg, ime_std_text_bg), ime_std_str_x + 3, ime_std_str_y + 3, "%s", ctx->string);
        bmp_printf(SHADOW_FONT(FONT(FONT_LARGE,COLOR_BLACK, COLOR_ORANGE)), ime_std_str_x + 3 + font_large.width * ctx->caret_pos, ime_std_str_y + 3, "_");

        /* orange rectangle around that dark characters box background */
        bmp_fill(COLOR_GRAY(20), ime_std_char_x - 1, ime_std_char_y - 1, ime_std_char_w + 2, fontspec_font(ime_std_font)->height * COUNT(ime_std_charcounts) + 4);
        bmp_draw_rect(COLOR_ORANGE, ime_std_char_x - 1, ime_std_char_y - 1, ime_std_char_w + 2, fontspec_font(ime_std_font)->height * COUNT(ime_std_charcounts) + 4);
        
        /* print charsets that are selected */
        for(unsigned int set = 0; set < COUNT(ime_std_charcounts); set++)
        {
            int offset_y = fontspec_font(ime_std_font)->height * set;
            if(ime_std_charset_types[set] & ctx->charset_type)
            {
                ime_std_draw_charset(ctx, set, ime_std_font, ctx->selection, set == ctx->charsetnum, ime_std_char_x + 2, ime_std_char_y + offset_y, ime_std_char_w - 1, ime_std_char_h - offset_y);
            }
            else
            {
                ime_std_draw_charset(ctx, set, ime_std_font, ctx->selection, 2, ime_std_char_x + 2, ime_std_char_y + offset_y, ime_std_char_w - 1, ime_std_char_h - offset_y);
            }
        }
        
        bmp_draw_to_idle(0);
        bmp_idle_copy(1,0);
    )
}

static int ime_std_select_charset(ime_std_ctx_t *ctx, int charset)
{
    /* only select charset if it matches the charset type patter specified by dialog creator */
    if(charset < COUNT(ime_std_charcounts) && (ime_std_charset_types[charset] & ctx->charset_type))
    {
        ctx->charset = ime_std_charsets[charset];
        ctx->charset_charcount = ime_std_charcounts[charset];
        
        /* make sure that the currently selected character position is available */
        if(ctx->selection >= ctx->charset_charcount)
        {
            ctx->selection = ctx->charset_charcount - 1;
        }
        
        return 1;
    }
    
    return 0;
}

static unsigned int ime_std_keypress_cbr(unsigned int key)
{
    ime_std_ctx_t *ctx = ime_std_current_ctx;
    
    if (!ctx || !ctx->active)
        return 1;
    
    switch (key)
    {
        case MODULE_KEY_PRESS_HALFSHUTTER:
        case MODULE_KEY_UNPRESS_HALFSHUTTER:
        case MODULE_KEY_PRESS_FULLSHUTTER:
        case MODULE_KEY_UNPRESS_FULLSHUTTER:
            /* cancel was pressed, set return code and return */
            ctx->returncode = IME_CANCEL;
            ctx->active = 0;
            break;
            
        case MODULE_KEY_WHEEL_UP:
            if(ctx->caret_pos > 0)
            {
                ctx->caret_pos--;
            }
            break;
            
        case MODULE_KEY_WHEEL_DOWN:
            if(ctx->caret_pos < strlen((const char*)ctx->string))
            {
                ctx->caret_pos++;
            }
            break;
            
        case MODULE_KEY_WHEEL_LEFT:
        case MODULE_KEY_PRESS_LEFT:
            if(ctx->selection > 0)
            {
                ctx->selection--;
            }
            break;
            
        case MODULE_KEY_WHEEL_RIGHT:
        case MODULE_KEY_PRESS_RIGHT:
            if(ctx->selection < ctx->charset_charcount - 1)
            {
                ctx->selection++;
            }
            break;
            
        case MODULE_KEY_PRESS_DOWN:
            if(ctx->charsetnum < COUNT(ime_std_charcounts))
            {
                int set = ctx->charsetnum + 1;
                
                while(set < COUNT(ime_std_charcounts))
                {
                    /* check the next possible charset */
                    if(ime_std_select_charset(ctx, set))
                    {
                        ctx->charsetnum = set;
                        break;
                    }
                    
                    set++;
                }
            }
            break;
            
        case MODULE_KEY_PRESS_UP:
            if(ctx->charsetnum > 0)
            {
                int set = ctx->charsetnum - 1;
                
                while(set >= 0)
                {
                    /* check the next possible charset */
                    if(ime_std_select_charset(ctx, set))
                    {
                        ctx->charsetnum = set;
                        break;
                    }
                    set--;
                }
            }
            break;
            
        case MODULE_KEY_JOY_CENTER:
        case MODULE_KEY_PRESS_SET:
            if(ctx->selection < ctx->charset_charcount)
            {
                unsigned char selected_char = ctx->charset[ctx->selection];
                
                if(selected_char == CHAR_DEL)
                {
                    /* backspace/del was pressed */
                    //ctx->caret_pos--;
                    strncpy((char*)&ctx->string[ctx->caret_pos], (char*)&ctx->string[ctx->caret_pos+1], ctx->max_length - ctx->caret_pos);
                }
                else if(selected_char == CHAR_OK)
                {
                    /* ok was pressed, set return code and return */
                    ctx->returncode = IME_OK;
                    ctx->active = 0;
                }
                else if(selected_char == CHAR_CANCEL)
                {
                    /* cancel was pressed, set return code and return */
                    ctx->returncode = IME_CANCEL;
                    ctx->active = 0;
                }
                else
                {
                    if(ctx->caret_pos < ctx->max_length)
                    {
                        ctx->string[ctx->caret_pos] = selected_char;
                        ctx->caret_pos++;
                    }
                }
            }
            break;
            
        default:
            return 0;
    }
    
    ime_std_update(ctx);
    
    ime_std_draw(ctx);
    return 0;
}

static void ime_std_config()
{
}


IME_UPDATE_FUNC(ime_std_update_cbr_file)
{
    if(strlen((char*)text) > 12)
    {
        return IME_ERR_UNKNOWN;
    }
    
    int dots = 0;
    for(int pos = 0; pos < strlen((char*)text); pos++)
    {
        if(text[pos] == '.')
        {
            dots++;
        }
    }
    
    if(dots != 1)
    {
        return IME_ERR_UNKNOWN;
    }

    char *dot_pos = strchr((char*)text, '.');
    if(strlen(dot_pos + 1) > 3)
    {
        return IME_ERR_UNKNOWN;
    }
    
    if((int)dot_pos - (int)text > 8)
    {
        return IME_ERR_UNKNOWN;
    }
    
    return IME_OK;
}


static void ime_std_input(unsigned int parm)
{
    ime_std_ctx_t *ctx = (ime_std_ctx_t *)parm;
    
    /* select appropriate punctuation for filenames */
    if(ctx->charset_type & IME_CHARSET_FILENAME)
    {
        ime_std_charsets[IME_STD_VAR_CHARSET] = ime_std_charset_file;
        ime_std_charset_types[IME_STD_VAR_CHARSET] = IME_CHARSET_FILENAME;
        ime_std_charcounts[IME_STD_VAR_CHARSET] = sizeof(ime_std_charset_file);
        if(!ctx->update_cbr)
        {
            ctx->update_cbr = &ime_std_update_cbr_file;
        }
    }
    else
    {
        ime_std_charsets[IME_STD_VAR_CHARSET] = IME_STD_VAR_CHARSET_DEF;
        ime_std_charset_types[IME_STD_VAR_CHARSET] = IME_STD_VAR_CHARSET_DEF_TYPE;
        ime_std_charcounts[IME_STD_VAR_CHARSET] = sizeof(IME_STD_VAR_CHARSET_DEF);
    }
        
    /* stop menu painting */
    menu_redraw_blocked = 1;
    
    /* start text input */    
    ime_std_current_ctx = ctx;
    
    /* redraw periodically */
    while(ctx->active)
    {
        ime_std_update(ctx);
        ime_std_draw(ctx);
        msleep(250);
    }
    
    ctx->done_cbr(ctx, ctx->returncode, ctx->string);
    ime_std_current_ctx = NULL;
    
    /* re-enable menu painting */
    menu_redraw_blocked = 0;
    
    free(ctx);
}

static void *ime_std_start(unsigned char *caption, unsigned char *text, int max_length, int codepage, int charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int x, int y, int w, int h)
{
    ime_std_ctx_t *ctx = malloc(sizeof(ime_std_ctx_t));
    
    /* set parameters */
    ctx->string = text;
    ctx->caption = caption;
    ctx->caret_pos = strlen((char*)text);
    ctx->max_length = max_length;
    ctx->charset_type = charset;

    /* store callback routines */
    ctx->update_cbr = update_cbr;
    ctx->done_cbr = done_cbr;
    
    ctx->selection = 0;
    ctx->active = 1;
    ctx->valid = 1;
    ctx->returncode = IME_CANCEL;

    /* fill remaining space with zeros just to make sure. trailing zero is placed behind text */
    for(int pos = strlen((char*)ctx->string); pos <= max_length; pos++)
    {
        ctx->string[pos] = '\000';
    }
    
    /* select first charset */
    ctx->charsetnum = 0;    

    while(ctx->charsetnum < COUNT(ime_std_charcounts))
    {
        /* check the next possible charset */
        if(ime_std_select_charset(ctx, ctx->charsetnum))
        {
            break;
        }
        
        ctx->charsetnum++;
    }
    
    /* no valid charset found */
    if(ctx->charsetnum >= COUNT(ime_std_charcounts))
    {
        return NULL;
    }
    
    /* start input system - ToDo: add a lock to make sure only one thread starts the IME */
    ime_std_current_ctx = ctx;
    task_create("ime_std_input", 0x1c, 0x1000, ime_std_input, ctx);
    
    return ctx;
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


