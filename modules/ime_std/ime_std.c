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

extern int32_t menu_redraw_blocked;

/* appearance options */
static int32_t ime_char_x = 30;
static int32_t ime_char_y = 200;
static int32_t ime_char_w = 650;
static int32_t ime_char_h = 280;
static int32_t ime_str_x = 30;
static int32_t ime_str_y = 100;
static int32_t ime_str_w = 650;
static int32_t ime_caption_x = 27;
static int32_t ime_caption_y = 20;

static int32_t ime_text_fg = COLOR_WHITE;
static int32_t ime_text_bg = COLOR_BLACK;
static int32_t ime_color_bg = COLOR_GRAY(10);

static uint32_t ime_font_title = 0;
static uint32_t ime_font_box = 0;
static uint32_t ime_font_txtfield = 0;
static uint32_t ime_font_caret = 0;

/* thats the charset that is used for various use cases */
#define IME_VAR_CHARSET           3
#define IME_VAR_CHARSET_DEF       ime_charset_punctuation
#define IME_VAR_CHARSET_DEF_TYPE  IME_CHARSET_PUNCTUATION

/* these are the OK buttons etc */
#define IME_FUNC_CHARSET          7

/* should that be located in files? */
static char ime_charset_alpha_upper[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
static char ime_charset_alpha_lower[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
static char ime_charset_hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'x'};
static char ime_charset_numeric[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static char ime_charset_punctuation[] = {'.', ',', ':', ';', '!', '?', '`', '\'', '\"'};
static char ime_charset_math[] = { '+', '-', '*', '/', '^' };
static char ime_charset_file[] = { '.', ' ', '-', '_' };
static char ime_charset_mail[] = { '@', '.', '-', '_', '!', '#', '$', '%', '&', '\'', '*', '+', '-', '/', '=', '?', '^', '_', '`', '{', '|', '}' };
static char ime_charset_func[] = { CHAR_DEL, CHAR_OK, CHAR_CANCEL };

/* number of characters in specific sets */
static uint32_t ime_charset_types[] = { IME_CHARSET_ALPHA | IME_CHARSET_FILENAME, IME_CHARSET_ALPHA, IME_CHARSET_NUMERIC | IME_CHARSET_FILENAME, IME_CHARSET_PUNCTUATION, IME_CHARSET_MATH, IME_CHARSET_MAIL, IME_CHARSET_HEX, IME_CHARSET_ANY };
static char *ime_charsets[] = { ime_charset_alpha_upper, ime_charset_alpha_lower, ime_charset_numeric, ime_charset_punctuation, ime_charset_math, ime_charset_mail, ime_charset_hex, ime_charset_func };
static uint32_t ime_charcounts[] = { sizeof(ime_charset_alpha_upper), sizeof(ime_charset_alpha_lower), sizeof(ime_charset_numeric), sizeof(ime_charset_punctuation), sizeof(ime_charset_math), sizeof(ime_charset_mail), sizeof(ime_charset_hex), sizeof(ime_charset_func) };


typedef struct
{
    uint32_t active;
    char *string;
    uint32_t max_length;
    uint32_t selection;
    char *caption;
    t_ime_update_cbr update_cbr;
    t_ime_done_cbr done_cbr;
    uint32_t caret_pos;
    uint32_t caret_shown;
    uint32_t valid;
    
    /* infos about current selected charset */
    uint32_t charset_type;
    char *charset;
    uint32_t charset_charcount;
    uint32_t charsetnum;
    uint32_t returncode;
} ime_ctx_t;

static ime_ctx_t *ime_current_ctx = NULL;


static void ime_update(ime_ctx_t *ctx)
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

static void ime_draw_charset(ime_ctx_t *ctx, uint32_t charset, uint32_t font, uint32_t charnum, uint32_t selected, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* total border width */
    uint32_t border = 2;
    uint32_t visible_chars = (w - x - border) / fontspec_font(font)->width;
    uint32_t first_char = (uint32_t)(charnum - visible_chars / 2);
    uint32_t last_char = (uint32_t)(charnum + visible_chars / 2);
    
    /* display only a window with the nearest characters around selection */
    if(last_char >= ime_charcounts[charset] - 1)
    {
        last_char = ime_charcounts[charset] - 1;
        first_char = MAX(0, last_char - visible_chars);
    }
    
    /* happens when first_char is less than zero */
    if(first_char > last_char)
    {
        first_char = 0;
        last_char = MIN(visible_chars, ime_charcounts[charset] - 1);
    }
    
    if((uint32_t)h > fontspec_font(font)->height)
    {
        /* only the selected charset line gets a black bar */
        if(selected == 1)
        {
            bmp_fill(COLOR_BLACK, x, y, fontspec_font(font)->width * visible_chars + border, fontspec_font(font)->height + border);
        }
     
        int32_t x_pos = x + border / 2;
        
        for(uint32_t pos = 0; pos <= (last_char - first_char); pos++)
        {
            char buf[16];
            unsigned char selected_char = ime_charsets[charset][first_char + pos];
            
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
            x_pos += strlen((const char *)buf) * fontspec_font(font)->width;
        }
    }
}

static void ime_draw(ime_ctx_t *ctx)
{
    int32_t color_border = COLOR_ORANGE;
    int32_t color_bg = COLOR_BLACK;
    
    /* if text isnt valid, print red */
    if(!ctx->valid)
    {
        color_border = COLOR_RED;
        color_bg = COLOR_DARK_RED;
    }
    
    BMP_LOCK
    (
        bmp_draw_to_idle(1);
        
        /* uniform background */
        bmp_fill(ime_color_bg, 0, 0, 720, 480);
        
        /* some nice borders */
        for(int32_t width = 0; width < 5; width++)
        {
            draw_line(0+width, 0+width, 720-width, 0+width, COLOR_GRAY(20));
            draw_line(0+width, 0+width, 0+width, 480-width, COLOR_GRAY(20));
            draw_line(720-width, 0+width, 720-width, 480-width, COLOR_GRAY(2));
            draw_line(0+width, 480-width, 720-width, 480-width, COLOR_GRAY(2));
        }
        
        /* print title text */
        if(ctx->caption)
        {
            bmp_printf(ime_font_title, ime_caption_x, ime_caption_y, "%s", ctx->caption);
            draw_line(ime_caption_x, ime_caption_y + 40, ime_str_w + ime_str_x, ime_caption_y + 40, COLOR_ORANGE);
            draw_line(ime_caption_x + 5, ime_caption_y + 40 + 3, ime_str_w + ime_str_x + 5, ime_caption_y + 40 + 3, COLOR_ORANGE);
        }
        
        /* draw a dark background for the text line */
        bmp_fill(color_bg, ime_str_x, ime_str_y, ime_str_w, fontspec_height(ime_font_txtfield) + 6);

        /* orange rectangle around that dark text box background */
        bmp_draw_rect(color_border, ime_str_x, ime_str_y, ime_str_w, fontspec_height(ime_font_txtfield) + 6);
        
        /* now the text and right after the caret */
        bmp_printf(ime_font_txtfield, ime_str_x + 3, ime_str_y + 3, "%s", ctx->string);
        char *tmp_str = strdup(ctx->string);
        tmp_str[ctx->caret_pos] = '\000';
        
        if(ctx->caret_shown)
        {
            bmp_printf(ime_font_caret, ime_str_x + 3 + bmp_string_width(ime_font_txtfield, tmp_str), ime_str_y + 3, "_");
        }
        free(tmp_str);

        /* orange rectangle around that dark characters box background */
        bmp_fill(COLOR_GRAY(20), ime_char_x - 1, ime_char_y - 1, ime_char_w + 2, fontspec_height(ime_font_txtfield) * COUNT(ime_charcounts) + 4);
        bmp_draw_rect(COLOR_ORANGE, ime_char_x - 1, ime_char_y - 1, ime_char_w + 2, fontspec_height(ime_font_txtfield) * COUNT(ime_charcounts) + 4);
        
        /* print charsets that are selected */
        for(uint32_t set = 0; set < COUNT(ime_charcounts); set++)
        {
            int32_t offset_y = fontspec_height(ime_font_txtfield) * set;
            if(ime_charset_types[set] & ctx->charset_type)
            {
                ime_draw_charset(ctx, set, ime_font_txtfield, ctx->selection, set == ctx->charsetnum, ime_char_x + 2, ime_char_y + offset_y, ime_char_w - 1, ime_char_h - offset_y);
            }
            else
            {
                ime_draw_charset(ctx, set, ime_font_txtfield, ctx->selection, 2, ime_char_x + 2, ime_char_y + offset_y, ime_char_w - 1, ime_char_h - offset_y);
            }
        }
        
        /* show buffer */
        bmp_draw_to_idle(0);
        bmp_idle_copy(1,0);
    )
}

static int32_t ime_select_charset(ime_ctx_t *ctx, int32_t charset)
{
    /* only select charset if it matches the charset type pattern specified by dialog creator */
    if(charset < COUNT(ime_charcounts) && (ime_charset_types[charset] & ctx->charset_type))
    {
        ctx->charset = ime_charsets[charset];
        ctx->charset_charcount = ime_charcounts[charset];
        
        /* make sure that the currently selected character position is available */
        if(ctx->selection >= ctx->charset_charcount)
        {
            ctx->selection = ctx->charset_charcount - 1;
        }
        
        ctx->charsetnum = charset;
        return 1;
    }
    
    return 0;
}

static unsigned int ime_keypress_cbr(unsigned int key)
{
    ime_ctx_t *ctx = ime_current_ctx;
    
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
            
        case MODULE_KEY_Q:
            ime_select_charset(ctx, IME_FUNC_CHARSET);
            ctx->selection = 1;
            break;
            
        case MODULE_KEY_WHEEL_UP:
            if(ctx->caret_pos > 0)
            {
                ctx->caret_pos--;
            }
            break;
            
        case MODULE_KEY_WHEEL_DOWN:
            if(ctx->caret_pos < strlen((const char *)ctx->string))
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
            if(ctx->charsetnum < COUNT(ime_charcounts))
            {
                int32_t set = ctx->charsetnum + 1;
                
                while(set < COUNT(ime_charcounts))
                {
                    /* check the next possible charset */
                    if(ime_select_charset(ctx, set))
                    {
                        break;
                    }
                    
                    set++;
                }
            }
            break;
            
        case MODULE_KEY_PRESS_UP:
            if(ctx->charsetnum > 0)
            {
                int32_t set = ctx->charsetnum - 1;
                
                while(set >= 0)
                {
                    /* check the next possible charset */
                    if(ime_select_charset(ctx, set))
                    {
                        break;
                    }
                    set--;
                }
            }
            break;
            
        case MODULE_KEY_TRASH:
            /* directly delete character */
            strncpy((char*)&ctx->string[ctx->caret_pos], (char*)&ctx->string[ctx->caret_pos+1], ctx->max_length - ctx->caret_pos - 1);
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
                    strncpy((char*)&ctx->string[ctx->caret_pos], (char*)&ctx->string[ctx->caret_pos+1], ctx->max_length - ctx->caret_pos - 1);
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
                    if(ctx->caret_pos < ctx->max_length - 1)
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
    
    ime_update(ctx);
    
    ime_draw(ctx);
    return 0;
}

static void ime_config()
{
}


IME_UPDATE_FUNC(ime_update_cbr_file)
{
    if(strlen((const char *)text) > 12)
    {
        return IME_ERR_UNKNOWN;
    }
    
    int32_t dots = 0;
    for(uint32_t pos = 0; pos < strlen((const char *)text); pos++)
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
    if(strlen((const char *)dot_pos + 1) > 3)
    {
        return IME_ERR_UNKNOWN;
    }
    
    if((int32_t)dot_pos - (int32_t)text > 8)
    {
        return IME_ERR_UNKNOWN;
    }
    
    return IME_OK;
}


static void ime_input(uint32_t parm)
{
    ime_ctx_t *ctx = (ime_ctx_t *)parm;
    
    /* select appropriate punctuation for filenames */
    if(ctx->charset_type == IME_CHARSET_FILENAME)
    {
        ime_charsets[IME_VAR_CHARSET] = ime_charset_file;
        ime_charset_types[IME_VAR_CHARSET] = IME_CHARSET_FILENAME;
        ime_charcounts[IME_VAR_CHARSET] = sizeof(ime_charset_file);
        if(!ctx->update_cbr)
        {
            ctx->update_cbr = &ime_update_cbr_file;
        }
    }
    else
    {
        ime_charsets[IME_VAR_CHARSET] = IME_VAR_CHARSET_DEF;
        ime_charset_types[IME_VAR_CHARSET] = IME_VAR_CHARSET_DEF_TYPE;
        ime_charcounts[IME_VAR_CHARSET] = sizeof(IME_VAR_CHARSET_DEF);
    }
    
    /* stop menu painting */
    menu_redraw_blocked = 1;
    
    /* start text input */    
    ime_current_ctx = ctx;
    
    /* redraw periodically */
    uint32_t caret_ctr = 0;
    while(ctx->active)
    {
        caret_ctr++;
        if(caret_ctr > 1)
        {
            ctx->caret_shown = !ctx->caret_shown;
        }
        
        ime_update(ctx);
        ime_draw(ctx);
        msleep(250);
    }
    
    if(ctx->done_cbr)
    {
        ctx->done_cbr(ctx, ctx->returncode, ctx->string);
    }
    ime_current_ctx = NULL;
    
    /* re-enable menu painting */
    menu_redraw_blocked = 0;
    
    if(ctx->caption)
    {
        free(ctx->caption);
    }
    free(ctx);
}

static void *ime_start(char *caption, char *text, int32_t max_length, int32_t codepage, int32_t charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int32_t x, int32_t y, int32_t w, int32_t h)
{
    ime_ctx_t *ctx = malloc(sizeof(ime_ctx_t));
    
    /* set parameters */
    ctx->string = text;
    ctx->caption = NULL;
    if(caption)
    {
        ctx->caption = strdup(caption);
    }
    ctx->caret_pos = strlen((const char *)text);
    ctx->max_length = max_length;
    ctx->charset_type = charset;

    /* store callback routines */
    ctx->update_cbr = update_cbr;
    ctx->done_cbr = done_cbr;
    
    ctx->selection = 0;
    ctx->active = 1;
    ctx->valid = 1;
    ctx->returncode = IME_CANCEL;

    /* fill remaining space with zeros just to make sure.t */
    for(int32_t pos = strlen((const char *)ctx->string); pos < max_length; pos++)
    {
        ctx->string[pos] = '\000';
    }
    
    /* select first charset */
    ctx->charsetnum = 0;    

    while(ctx->charsetnum < COUNT(ime_charcounts))
    {
        /* check the next possible charset */
        if(ime_select_charset(ctx, ctx->charsetnum))
        {
            break;
        }
        
        ctx->charsetnum++;
    }
    
    /* no valid charset found */
    if(ctx->charsetnum >= COUNT(ime_charcounts))
    {
        return NULL;
    }
    
    /* start input system - ToDo: add a lock to make sure only one thread starts the IME */
    task_create("ime_std", 0x1c, 0x1000, ime_input, ctx);
    
    return ctx;
}

static t_ime_handler ime_descriptor = 
{
    .name = "ime_std",
    .description = "Standard input method",
    .start = &ime_start,
    .configure = &ime_config,
};

static unsigned int ime_init()
{
    ime_font_title = FONT(FONT_LARGE, ime_text_fg, ime_color_bg);
    ime_font_box = FONT(FONT_LARGE, COLOR_ORANGE, ime_color_bg);
    ime_font_txtfield = FONT(FONT_LARGE, ime_text_fg, ime_text_bg);
    ime_font_caret = SHADOW_FONT(FONT(FONT_LARGE, COLOR_BLACK, COLOR_ORANGE));

    ime_base_register(&ime_descriptor);
    return 0;
}

static unsigned int ime_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(ime_init)
    MODULE_DEINIT(ime_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_KEYPRESS, ime_keypress_cbr, 0)
MODULE_CBRS_END()


