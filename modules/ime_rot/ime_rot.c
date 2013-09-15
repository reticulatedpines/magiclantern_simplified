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

#include <cordic-16bit.h>

#include "../ime_base/ime_base.h"

#define CHAR_OK     0x01
#define CHAR_CANCEL 0x02
#define CHAR_DEL    0x08

#define PI 3.1415926f

extern int menu_redraw_blocked;

/* appearance options */
int ime_rot_wheel_x = 360;
int ime_rot_wheel_y = 420;
int ime_rot_wheel_w = 250;
int ime_rot_wheel_h = 200;
int ime_rot_str_x = 30;
int ime_rot_str_y = 100;
int ime_rot_str_w = 650;
int ime_rot_caption_x = 27;
int ime_rot_caption_y = 20;

int ime_rot_text_fg = COLOR_WHITE;
int ime_rot_text_bg = COLOR_BLACK;    
int ime_rot_color_bg = COLOR_GRAY(10);

unsigned int ime_rot_font_title = 0;
unsigned int ime_rot_font_wheel = 0;
unsigned int ime_rot_font_wheel_sel = 0;
unsigned int ime_rot_font_txtfield = 0;
unsigned int ime_rot_font_caret = 0;


/* thats the charset that is used for various use cases */
#define IME_ROT_VAR_CHARSET           3
#define IME_ROT_VAR_CHARSET_DEF       ime_rot_charset_punctuation
#define IME_ROT_VAR_CHARSET_DEF_TYPE  IME_CHARSET_PUNCTUATION

/* these are the OK buttons etc */
#define IME_ROT_FUNC_CHARSET          6

/* should that be located in files? */
unsigned char ime_rot_charset_alpha_upper[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
unsigned char ime_rot_charset_alpha_lower[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
unsigned char ime_rot_charset_numeric[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
unsigned char ime_rot_charset_punctuation[] = {'.', ',', ':', ';', '!', '?', '`', '\'', '\"'};
unsigned char ime_rot_charset_math[] = { '+', '-', '*', '/', '^' };
unsigned char ime_rot_charset_file[] = { '.', ' ', '-', '_' };
unsigned char ime_rot_charset_mail[] = { '@', '.', '-', '_', '!', '#', '$', '%', '&', '\'', '*', '+', '-', '/', '=', '?', '^', '_', '`', '{', '|', '}' };
unsigned char ime_rot_charset_func[] = { CHAR_DEL, CHAR_OK, CHAR_CANCEL };

/* number of characters in specific sets */
unsigned int ime_rot_charset_types[] = { IME_CHARSET_ALPHA | IME_CHARSET_FILENAME, IME_CHARSET_ALPHA, IME_CHARSET_NUMERIC | IME_CHARSET_FILENAME, IME_CHARSET_PUNCTUATION, IME_CHARSET_MATH, IME_CHARSET_MAIL, IME_CHARSET_ANY };
unsigned char *ime_rot_charsets[] = { ime_rot_charset_alpha_upper, ime_rot_charset_alpha_lower, ime_rot_charset_numeric, ime_rot_charset_punctuation, ime_rot_charset_math, ime_rot_charset_mail, ime_rot_charset_func };
int ime_rot_charcounts[] = { sizeof(ime_rot_charset_alpha_upper), sizeof(ime_rot_charset_alpha_lower), sizeof(ime_rot_charset_numeric), sizeof(ime_rot_charset_punctuation), sizeof(ime_rot_charset_math), sizeof(ime_rot_charset_mail), sizeof(ime_rot_charset_func) };
unsigned char *ime_rot_charsetnames[] = { (unsigned char *)"upper", (unsigned char *)"lower", (unsigned char *)"num", (unsigned char *)"punct", (unsigned char *)"math", (unsigned char *)"var" };


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
} ime_rot_ctx_t;

ime_rot_ctx_t *ime_rot_current_ctx = NULL;


static void ime_rot_update(ime_rot_ctx_t *ctx)
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

static void ime_rot_draw_wheel(ime_rot_ctx_t *ctx)
{
    int char_pos = ctx->selection;
    int visible_chars = MIN(ctx->charset_charcount, 15);

    int first_char = (int)(char_pos - visible_chars / 2);
    int last_char = (int)(char_pos + visible_chars / 2);
    
    /* display only a window with the nearest characters around selection */
    if(last_char >= ime_rot_charcounts[ctx->charsetnum] - 1)
    {
        last_char = ime_rot_charcounts[ctx->charsetnum] - 1;
        first_char = MAX(0, last_char - visible_chars + 1);
    }
    
    if(first_char <= 0)
    {
        first_char = 0;
        last_char = MIN(visible_chars - 1, ime_rot_charcounts[ctx->charsetnum] - 1);
    }
    
    int pos = 0;
    
    for(int char_pos = first_char; char_pos <= last_char; char_pos++)
    {
        int sine = 0;
        int cosine = 0;
        
        cordic((-PI/2 + PI * (float)pos / (float)(visible_chars-1)) * MUL, &sine, &cosine, CORDIC_NTAB);
        pos++;
        
        
        char buf[16];
        unsigned char selected_char = ime_rot_charsets[ctx->charsetnum][char_pos];
        
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
        
        /* measure string width */
        int width = bmp_string_width(ime_rot_font_wheel, buf);
            
        int x = ime_rot_wheel_x + ime_rot_wheel_w * sine / MUL - width/2;
        int y = ime_rot_wheel_y - ime_rot_wheel_h * cosine / MUL - 25;
        
        int line_x = ime_rot_wheel_x + (ime_rot_wheel_w - 40) * sine / MUL;
        int line_y = ime_rot_wheel_y - (ime_rot_wheel_h - 40) * cosine / MUL;
        
        if(ctx->selection == char_pos)
        {
            /* black box around character */
            bmp_fill(COLOR_BLACK, x - 3, y - 3, width + 6, 50);
            
            /* some kind of arrow towards the character */
            for(int bold = -2; bold <= 2; bold++)
            {
                draw_line(ime_rot_wheel_x, ime_rot_wheel_y, line_x, line_y, COLOR_ORANGE);
                draw_line(ime_rot_wheel_x+bold, ime_rot_wheel_y, line_x, line_y, COLOR_ORANGE);
                draw_line(ime_rot_wheel_x+bold, ime_rot_wheel_y+bold, line_x, line_y, COLOR_ORANGE);
                draw_line(ime_rot_wheel_x, ime_rot_wheel_y+bold, line_x, line_y, COLOR_ORANGE);
            }
            
            /* print character */
            bmp_printf(ime_rot_font_wheel_sel, x, y, buf);
        }
        else
        {
            draw_line(ime_rot_wheel_x, ime_rot_wheel_y, line_x, line_y, ime_rot_color_bg);
            bmp_printf(ime_rot_font_wheel, x, y, buf);
        }
    }
}

static void ime_rot_draw(ime_rot_ctx_t *ctx)
{
    int color_fg = ime_rot_text_fg;
    
    /* if text isnt valid, print red */
    if(!ctx->valid)
    {
        color_fg = COLOR_RED;
    }
    
    BMP_LOCK
    (
        bmp_draw_to_idle(1);
            
        /* uniform background */
        bmp_fill(ime_rot_color_bg, 0, 0, 720, 480);
        
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
            bmp_printf(ime_rot_font_title, ime_rot_caption_x, ime_rot_caption_y, "%s", ctx->caption);
            draw_line(ime_rot_caption_x, ime_rot_caption_y + 40, ime_rot_str_w + ime_rot_str_x, ime_rot_caption_y + 40, COLOR_ORANGE);
            draw_line(ime_rot_caption_x + 5, ime_rot_caption_y + 40 + 3, ime_rot_str_w + ime_rot_str_x + 5, ime_rot_caption_y + 40 + 3, COLOR_ORANGE);
        }
        
        /* draw a dark background for the text line */
        bmp_fill(COLOR_BLACK, ime_rot_str_x, ime_rot_str_y, ime_rot_str_w, fontspec_height(ime_rot_font_txtfield) + 6);

        /* orange rectangle around that dark text box background */
        bmp_draw_rect(COLOR_ORANGE, ime_rot_str_x, ime_rot_str_y, ime_rot_str_w, fontspec_height(ime_rot_font_txtfield) + 6);
        
        /* now the text and right after the caret */
        bmp_printf(ime_rot_font_txtfield, ime_rot_str_x + 3, ime_rot_str_y + 3, "%s", ctx->string);
        char *tmp_str = malloc(strlen(ctx->string) + 1);
        strcpy(tmp_str, ctx->string);
        tmp_str[ctx->caret_pos] = '\000';
        bmp_printf(ime_rot_font_caret, ime_rot_str_x + 3 + bmp_string_width(ime_rot_font_txtfield, tmp_str), ime_rot_str_y + 3, "_");
        free(tmp_str);

        /* draw rotation wheel */
        ime_rot_draw_wheel(ctx);
        
        /* show buffer */
        bmp_draw_to_idle(0);
        bmp_idle_copy(1,0);
    )
}

static int ime_rot_select_charset(ime_rot_ctx_t *ctx, int charset)
{
    /* only select charset if it matches the charset type patter specified by dialog creator */
    if(charset < COUNT(ime_rot_charcounts) && (ime_rot_charset_types[charset] & ctx->charset_type))
    {
        ctx->charset = ime_rot_charsets[charset];
        ctx->charset_charcount = ime_rot_charcounts[charset];
        
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

static unsigned int ime_rot_keypress_cbr(unsigned int key)
{
    ime_rot_ctx_t *ctx = ime_rot_current_ctx;
    
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
            ime_rot_select_charset(ctx, IME_ROT_FUNC_CHARSET);
            ctx->selection = 1;
            break;
            
        case MODULE_KEY_PRESS_LEFT:
            if(ctx->caret_pos > 0)
            {
                ctx->caret_pos--;
            }
            break;
            
        case MODULE_KEY_PRESS_RIGHT:
            if(ctx->caret_pos < strlen((const char*)ctx->string))
            {
                ctx->caret_pos++;
            }
            break;
            
        case MODULE_KEY_WHEEL_UP:
        case MODULE_KEY_WHEEL_LEFT:
            if(ctx->selection > 0)
            {
                ctx->selection--;
            }
            break;
            
        case MODULE_KEY_WHEEL_DOWN:
        case MODULE_KEY_WHEEL_RIGHT:
            if(ctx->selection < ctx->charset_charcount - 1)
            {
                ctx->selection++;
            }
            break;
            
        case MODULE_KEY_PRESS_DOWN:
            if(ctx->charsetnum < COUNT(ime_rot_charcounts))
            {
                int set = ctx->charsetnum + 1;
                
                while(set < COUNT(ime_rot_charcounts))
                {
                    /* check the next possible charset */
                    if(ime_rot_select_charset(ctx, set))
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
                int set = ctx->charsetnum - 1;
                
                while(set >= 0)
                {
                    /* check the next possible charset */
                    if(ime_rot_select_charset(ctx, set))
                    {
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
    
    ime_rot_update(ctx);
    
    ime_rot_draw(ctx);
    return 0;
}

static void ime_rot_config()
{
}


IME_UPDATE_FUNC(ime_rot_update_cbr_file)
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


static void ime_rot_input(unsigned int parm)
{
    ime_rot_ctx_t *ctx = (ime_rot_ctx_t *)parm;
    
    /* select appropriate punctuation for filenames */
    if(ctx->charset_type & IME_CHARSET_FILENAME)
    {
        ime_rot_charsets[IME_ROT_VAR_CHARSET] = ime_rot_charset_file;
        ime_rot_charset_types[IME_ROT_VAR_CHARSET] = IME_CHARSET_FILENAME;
        ime_rot_charcounts[IME_ROT_VAR_CHARSET] = sizeof(ime_rot_charset_file);
        if(!ctx->update_cbr)
        {
            ctx->update_cbr = &ime_rot_update_cbr_file;
        }
    }
    else
    {
        ime_rot_charsets[IME_ROT_VAR_CHARSET] = IME_ROT_VAR_CHARSET_DEF;
        ime_rot_charset_types[IME_ROT_VAR_CHARSET] = IME_ROT_VAR_CHARSET_DEF_TYPE;
        ime_rot_charcounts[IME_ROT_VAR_CHARSET] = sizeof(IME_ROT_VAR_CHARSET_DEF);
    }
    
    /* stop menu painting */
    menu_redraw_blocked = 1;
    
    /* start text input */    
    ime_rot_current_ctx = ctx;
    
    /* redraw periodically */
    while(ctx->active)
    {
        ime_rot_update(ctx);
        ime_rot_draw(ctx);
        msleep(250);
    }
    
    ctx->done_cbr(ctx, ctx->returncode, ctx->string);
    ime_rot_current_ctx = NULL;
    
    /* re-enable menu painting */
    menu_redraw_blocked = 0;
    
    free(ctx);
}

static void *ime_rot_start(unsigned char *caption, unsigned char *text, int max_length, int codepage, int charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int x, int y, int w, int h)
{
    ime_rot_ctx_t *ctx = malloc(sizeof(ime_rot_ctx_t));
    
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

    while(ctx->charsetnum < COUNT(ime_rot_charcounts))
    {
        /* check the next possible charset */
        if(ime_rot_select_charset(ctx, ctx->charsetnum))
        {
            break;
        }
        
        ctx->charsetnum++;
    }
    
    /* no valid charset found */
    if(ctx->charsetnum >= COUNT(ime_rot_charcounts))
    {
        return NULL;
    }
    
    /* start input system - ToDo: add a lock to make sure only one thread starts the IME */
    task_create("ime_rot_input", 0x1c, 0x1000, ime_rot_input, ctx);
    
    return ctx;
}


static t_ime_handler ime_rot_descriptor = 
{
    .name = "ime_rot",
    .description = "Standard input method",
    .start = &ime_rot_start,
    .configure = &ime_rot_config,
};

static unsigned int ime_rot_init()
{
    ime_rot_font_title = FONT(FONT_LARGE, COLOR_WHITE, ime_rot_color_bg);
    ime_rot_font_wheel = FONT(FONT_LARGE, COLOR_ORANGE, ime_rot_color_bg);
    ime_rot_font_wheel_sel = FONT(FONT_LARGE, COLOR_ORANGE, COLOR_BLACK);
    ime_rot_font_txtfield = FONT(FONT_LARGE, ime_rot_text_fg, ime_rot_text_bg);
    ime_rot_font_caret = SHADOW_FONT(FONT(FONT_LARGE, COLOR_BLACK, COLOR_ORANGE));

    ime_base_register(&ime_rot_descriptor);
    return 0;
}

static unsigned int ime_rot_deinit()
{
    return 0;
}



MODULE_INFO_START()
    MODULE_INIT(ime_rot_init)
    MODULE_DEINIT(ime_rot_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_KEYPRESS, ime_rot_keypress_cbr, 0)
MODULE_CBRS_END()


