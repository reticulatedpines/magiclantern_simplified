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

/* needed for drawing characters in circle */
#include <cordic-16bit.h>
#define PI 3.1415926f

#include "../ime_base/ime_base.h"

#define CHAR_OK     0x01
#define CHAR_CANCEL 0x02
#define CHAR_DEL    0x08

extern int32_t menu_redraw_blocked;

static int32_t ime_config_mode = 0;

/* appearance options */
static CONFIG_INT("ime.rot.wheel.x", ime_wheel_x, 360);
static CONFIG_INT("ime.rot.wheel.y", ime_wheel_y, 450);
static CONFIG_INT("ime.rot.wheel.w", ime_wheel_w, 300);
static CONFIG_INT("ime.rot.wheel.h", ime_wheel_h, 250);
static CONFIG_INT("ime.rot.str.x", ime_str_x, 30);
static CONFIG_INT("ime.rot.str.y", ime_str_y, 100);
static CONFIG_INT("ime.rot.str.w", ime_str_w, 650);
static CONFIG_INT("ime.rot.caption.x", ime_caption_x, 27);
static CONFIG_INT("ime.rot.caption.x", ime_caption_y, 20);

static CONFIG_INT("ime.rot.color.text_fg", ime_text_fg, COLOR_WHITE);
static CONFIG_INT("ime.rot.color.text_bg", ime_text_bg, COLOR_BLACK);
static CONFIG_INT("ime.rot.color.bg", ime_color_bg, COLOR_GRAY(10));

static uint32_t ime_font_title = 0;
static uint32_t ime_font_wheel = 0;
static uint32_t ime_font_wheel_sel = 0;
static uint32_t ime_font_wheel_inact = 0;
static uint32_t ime_font_txtfield = 0;
static uint32_t ime_font_caret = 0;

/* thats the charset that is used for various use cases */
#define IME_VAR_CHARSET           3
#define IME_VAR_CHARSET_DEF       ime_charset_punctuation
#define IME_VAR_CHARSET_DEF_TYPE  IME_CHARSET_PUNCTUATION

/* these are the OK buttons etc */
#define IME_FUNC_CHARSET          7

/* should that be located in files? */
static unsigned char ime_charset_alpha_upper[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
static unsigned char ime_charset_alpha_lower[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
static unsigned char ime_charset_numeric[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static unsigned char ime_charset_punctuation[] = {'.', ',', ':', ';', '!', '?', '`', '\'', '\"'};
static unsigned char ime_charset_hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
static unsigned char ime_charset_math[] = { '+', '-', '*', '/', '^' };
static unsigned char ime_charset_file[] = { '.', ' ', '-', '_' };
static unsigned char ime_charset_mail[] = { '@', '.', '-', '_', '!', '#', '$', '%', '&', '\'', '*', '+', '-', '/', '=', '?', '^', '_', '`', '{', '|', '}' };
static unsigned char ime_charset_func[] = { CHAR_DEL, CHAR_OK, CHAR_CANCEL };

/* number of characters in specific sets */
static unsigned int ime_charset_types[] = { IME_CHARSET_ALPHA | IME_CHARSET_FILENAME, IME_CHARSET_ALPHA, IME_CHARSET_NUMERIC | IME_CHARSET_FILENAME, IME_CHARSET_PUNCTUATION, IME_CHARSET_HEX, IME_CHARSET_MATH, IME_CHARSET_MAIL, IME_CHARSET_ANY };
static unsigned char *ime_charsets[] = { ime_charset_alpha_upper, ime_charset_alpha_lower, ime_charset_numeric, ime_charset_punctuation, ime_charset_hex, ime_charset_math, ime_charset_mail, ime_charset_func };
static int ime_charcounts[] = { sizeof(ime_charset_alpha_upper), sizeof(ime_charset_alpha_lower), sizeof(ime_charset_numeric), sizeof(ime_charset_punctuation), sizeof(ime_charset_hex), sizeof(ime_charset_math), sizeof(ime_charset_mail), sizeof(ime_charset_func) };
static unsigned char *ime_charsetnames[] = { (unsigned char *)"upper", (unsigned char *)"lower", (unsigned char *)"num", (unsigned char *)"punct", (unsigned char *)"hex", (unsigned char *)"math", (unsigned char *)"var" };


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


static void ime_draw_arc(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t steps)
{
    /* draw borders around current character set */
    uint32_t prev_x = 0;
    uint32_t prev_y = 0;
    
    /* draw circle using a few steps */
    for(uint32_t circle_step = 0; circle_step < steps; circle_step++)
    {
        int32_t sine = 0;
        int32_t cosine = 0;
        
        cordic((-PI/2 + PI * circle_step / steps) * MUL, (int *)&sine, (int *)&cosine, CORDIC_NTAB);
        
        /* two lines, above and below the letters */
        uint32_t circle_x = COERCE(x + (sine * (int32_t)w) / MUL, 0, 719);
        uint32_t circle_y = COERCE(y - (cosine * (int32_t)h) / MUL, 0, 479);
        
        /* first line isnt drawn, it would go from origin (0,0) */
        if(prev_x != 0 && prev_y != 0)
        {
            draw_line(prev_x, prev_y, circle_x, circle_y, color);
        }
        
        /* remember last position */
        prev_x = circle_x;
        prev_y = circle_y;
    }
}

static void ime_draw_wheel(uint32_t selection, uint32_t charsetnum, uint32_t color, uint32_t color_sel, int32_t radius_delta, int32_t arrow_width, int32_t active)
{
    if(charsetnum >= COUNT(ime_charcounts))
    {
        return;
    }
    
    uint32_t char_pos = selection;
    uint32_t visible_chars = MIN(ime_charcounts[charsetnum], 15);
    uint32_t first_char = (uint32_t)(char_pos - visible_chars / 2);
    uint32_t last_char = (uint32_t)(char_pos + visible_chars / 2);
    
    /* display only a window with the nearest characters around selection */
    if(last_char >= ime_charcounts[charsetnum] - 1)
    {
        last_char = ime_charcounts[charsetnum] - 1;
        first_char = MAX(0, last_char - visible_chars + 1);
    }
    
    /* happens when first_char is less than zero */
    if(first_char > last_char)
    {
        first_char = 0;
        last_char = MIN(visible_chars - 1, ime_charcounts[charsetnum] - 1);
    }
    
    ime_draw_arc(ime_wheel_x, ime_wheel_y, ime_wheel_w - 25, ime_wheel_h - 25, COLOR_ORANGE, 64);
    ime_draw_arc(ime_wheel_x, ime_wheel_y, ime_wheel_w - 22, ime_wheel_h - 22, COLOR_ORANGE, 64);
    ime_draw_arc(ime_wheel_x, ime_wheel_y, ime_wheel_w + 25, ime_wheel_h + 25, COLOR_ORANGE, 64);
    ime_draw_arc(ime_wheel_x, ime_wheel_y, ime_wheel_w + 27, ime_wheel_h + 27, COLOR_ORANGE, 64);
    
    uint32_t pos = 0;
    for(uint32_t char_pos = first_char; char_pos <= last_char; char_pos++)
    {
        int32_t sine = 0;
        int32_t cosine = 0;
        
        cordic((-PI/2 + PI * (float)pos / (float)(visible_chars-1)) * MUL, (int *)&sine, (int *)&cosine, CORDIC_NTAB);
        pos++;
        
        char buf[16];
        unsigned char selected_char = ime_charsets[charsetnum][char_pos];
        
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
        
        /* position as circle around center */
        int32_t x = ime_wheel_x + ((ime_wheel_w + radius_delta) * sine) / MUL;
        int32_t y = ime_wheel_y - ((ime_wheel_h + radius_delta) * cosine) / MUL;
        
        int32_t line_x = ime_wheel_x + (ime_wheel_w - 40) * sine / MUL;
        int32_t line_y = ime_wheel_y - (ime_wheel_h - 40) * cosine / MUL;
        
        /* measure string width */
        int32_t width = bmp_string_width(ime_font_wheel, buf);
        int32_t height = fontspec_height(ime_font_wheel);
        
        if((selection == char_pos) && active)
        {
            /* black box around character */
            bmp_fill(COLOR_BLACK, x - width/2 - 3, y - height/2 - 3, width + 6, height + 6);
            
            /* some kind of arrow towards the character */
            for(int32_t bold = -arrow_width; bold <= arrow_width; bold++)
            {
                draw_line(ime_wheel_x, ime_wheel_y, line_x, line_y, COLOR_ORANGE);
                draw_line(ime_wheel_x+bold, ime_wheel_y, line_x, line_y, COLOR_ORANGE);
                draw_line(ime_wheel_x+bold, ime_wheel_y+bold, line_x, line_y, COLOR_ORANGE);
                draw_line(ime_wheel_x, ime_wheel_y+bold, line_x, line_y, COLOR_ORANGE);
            }
            fill_circle(ime_wheel_x, ime_wheel_y, arrow_width * 2, COLOR_RED);
            
            /* print character */
            bmp_printf(color_sel, x - width/2, y - height/2, buf);
        }
        else
        {
            if(active)
            {
                uint32_t gray_distance = 5;
                uint32_t gray_dist = 10 + (100 - 10) * (gray_distance - MIN(gray_distance, ABS((int32_t)selection - (int32_t)char_pos))) / gray_distance;
                uint32_t gray_rot = 10 + (100 - 10) * cosine / MUL;
                uint32_t gray = MAX(gray_rot, gray_dist);
                
                bmp_printf(FONT(color,COLOR_GRAY(gray),FONT_BG(color)), x - width/2, y - height/2, buf);
            }
            else
            {
                bmp_printf(color, x - width/2, y - height/2, buf);
            }
        }
        
        /* draw dot on letter center */
        // fill_circle(x, y, 4, COLOR_RED);
    }
}

static void ime_draw_wheels(ime_ctx_t *ctx)
{
    /* preview previous charset */
    for(int32_t set = ctx->charsetnum - 1; set >= 0; set--)
    {
        /* is that the next valid one? */
        if(ime_charset_types[set] & ctx->charset_type)
        {
            ime_draw_wheel(ctx->selection, set, ime_font_wheel_inact, ime_font_wheel_inact, 40, 0, 0);
            break;
        }
    }
    /* preview next charset */
    for(int32_t set = ctx->charsetnum + 1; set < COUNT(ime_charset_types); set++)
    {
        /* is that the next valid one? */
        if(ime_charset_types[set] & ctx->charset_type)
        {
            ime_draw_wheel(ctx->selection, set, ime_font_wheel_inact, ime_font_wheel_inact, -40, 0, 0);
            break;
        }
    }
    
    /* draw the currently selected charset */
    ime_draw_wheel(ctx->selection, ctx->charsetnum, ime_font_wheel, ime_font_wheel_sel, 0, 6, 1);
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
        
        if(ime_config_mode)
        {
            bmp_printf(FONT(FONT_MONO_20,COLOR_RED,COLOR_BLACK), ime_caption_x, ime_caption_y,      "[CONFIG MODE]                   ");
            bmp_printf(FONT(FONT_MONO_20,COLOR_RED,COLOR_BLACK), ime_caption_x, ime_caption_y + 20, "  U/D/L/R:  Position characters ");
            bmp_printf(FONT(FONT_MONO_20,COLOR_RED,COLOR_BLACK), ime_caption_x, ime_caption_y + 40, "  Wheels:   Resize characters   ");
            bmp_printf(FONT(FONT_MONO_20,COLOR_RED,COLOR_BLACK), ime_caption_x, ime_caption_y + 60, "  SET:      Done                ");
            bmp_printf(FONT(FONT_MONO_20,COLOR_RED,COLOR_BLACK), ime_caption_x, ime_caption_y + 80, "                                ");
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

        /* draw rotation wheel */
        ime_draw_wheels(ctx);
        
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

static unsigned int ime_keypress_config(unsigned int key)
{
    switch (key)
    {
        case MODULE_KEY_PRESS_LEFT:
            ime_wheel_x--;
            break;
            
        case MODULE_KEY_PRESS_RIGHT:
            ime_wheel_x++;
            break;
            
        case MODULE_KEY_PRESS_UP:
            ime_wheel_y--;
            break;
            
        case MODULE_KEY_PRESS_DOWN:
            ime_wheel_y++;
            break;
            
        case MODULE_KEY_WHEEL_UP:
            ime_wheel_w--;
            break;
            
        case MODULE_KEY_WHEEL_DOWN:
            ime_wheel_w++;
            break;
            
        case MODULE_KEY_WHEEL_LEFT:
            ime_wheel_h--;
            break;
            
        case MODULE_KEY_WHEEL_RIGHT:
            ime_wheel_h++;
            break;
            
        case MODULE_KEY_JOY_CENTER:
        case MODULE_KEY_PRESS_SET:
            ime_config_mode = 0;
            break;
        
            
        default:
            return 0;
    }
    
    return 0;
}


static unsigned int ime_keypress_cbr(unsigned int key)
{
    ime_ctx_t *ctx = ime_current_ctx;
    
    if (!ctx || !ctx->active)
    {
        return 1;
    }

    if(ime_config_mode)
    {
        return ime_keypress_config(key);
    }

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
    static char buf[32];
    
    strcpy(buf, "Config mode");
    
    ime_config_mode = 1;
    ime_base_start("Config", buf, sizeof(buf), IME_UTF8, IME_CHARSET_ANY, NULL, NULL, 0, 0, 0, 0);
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
    task_create("ime_rot", 0x1c, 0x1000, ime_input, ctx);
    
    return ctx;
}

static t_ime_handler ime_descriptor = 
{
    .name = "ime_rot",
    .description = "Standard input method",
    .start = &ime_start,
    .configure = &ime_config,
};

static unsigned int ime_init()
{
    ime_font_title = FONT(FONT_LARGE, ime_text_fg, ime_color_bg);
    ime_font_wheel = FONT(FONT_LARGE, COLOR_ORANGE, ime_color_bg);
    ime_font_wheel_sel = FONT(FONT_LARGE, COLOR_ORANGE, COLOR_BLACK);
    ime_font_wheel_inact = FONT(FONT_LARGE, COLOR_GRAY(20), ime_color_bg);
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

MODULE_CONFIGS_START()
    MODULE_CONFIG(ime_wheel_x)
    MODULE_CONFIG(ime_wheel_y)
    MODULE_CONFIG(ime_wheel_w)
    MODULE_CONFIG(ime_wheel_h)
    MODULE_CONFIG(ime_str_x)
    MODULE_CONFIG(ime_str_y)
    MODULE_CONFIG(ime_str_w)
    MODULE_CONFIG(ime_caption_x)
    MODULE_CONFIG(ime_caption_y)
    MODULE_CONFIG(ime_text_fg)
    MODULE_CONFIG(ime_text_bg)
    MODULE_CONFIG(ime_color_bg)
MODULE_CONFIGS_END()
