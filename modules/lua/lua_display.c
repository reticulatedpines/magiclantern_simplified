/***
 Display and bmp drawing functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module display
 */

#include <dryos.h>
#include <string.h>
#include <bmp.h>
#include <zebra.h>
#include <screenshot.h>
#include <notify_box.h>

#include "lua_common.h"


/***
 Turn the display on
 @function on
 */
static int luaCB_display_on(lua_State * L)
{
    display_on();
    return 0;
}

/***
 Turn the display off
 @function off
 */
static int luaCB_display_off(lua_State * L)
{
    display_off();
    return 0;
}

/***
 Take a screenshot
 @tparam[opt] string filename
 @tparam[opt] int mode
 @function screenshot
 */
static int luaCB_display_screenshot(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(filename, 1, SCREENSHOT_FILENAME_AUTO);
    LUA_PARAM_INT_OPTIONAL(mode, 2, SCREENSHOT_BMP | SCREENSHOT_YUV);
    take_screenshot((char*)filename, mode);
    return 0;
}

/***
 Clear the screen
 @function clear
 */
static int luaCB_display_clear(lua_State * L)
{
    clrscr();
    return 0;
}

/***
 Print text to the screen.
 @tparam string text
 @tparam int x
 @tparam int y
 @tparam[opt] font font @{constants.FONT}
 @tparam[opt] int fg foreground @{constants.COLOR}
 @tparam[opt] int bg background @{constants.COLOR}
 @tparam[opt] int max_width
 @treturn string any remaining characters that wouldn't fit on the screen
 @function print
 */
static int luaCB_display_print(lua_State * L)
{
    LUA_PARAM_STRING(str, 1);
    LUA_PARAM_INT(x, 2);
    LUA_PARAM_INT(y, 3);
    uint32_t font = FONT_MED;
    if(lua_istable(L, 4))
    {
        if(lua_getfield(L, 4, "_spec") == LUA_TNUMBER)
        {
            font = lua_tointeger(L, -1);
        }
        lua_pop(L,1);
    }
    LUA_PARAM_INT_OPTIONAL(fg, 5, COLOR_WHITE);
    LUA_PARAM_INT_OPTIONAL(bg, 6, COLOR_BLACK);
    LUA_PARAM_INT_OPTIONAL(max_width, 7, 720 - x);
    int actual_width = bmp_string_width(font, str);
    uint32_t max_chars = (uint32_t)bmp_strlen_clipped(font, str, max_width);
    if(actual_width > max_width && max_chars < strlen(str))
    {
        lua_pushstring(L, str + max_chars);
        char * temp = malloc(sizeof(char) * max_chars + 1);
        strncpy(temp,str,max_chars);
        temp[max_chars] = 0;
        bmp_printf(FONT(font, fg, bg), x, y, "%s", temp);
        free(temp);
        return 1;
    }
    else
    {
        bmp_printf(FONT(font, fg, bg), x, y, "%s", str);
        return 0;
    }
}

/***
 Set a pixel to a color 
 (omit color param to read the current color)
 @tparam int x
 @tparam int y
 @tparam[opt] int color @{constants.COLOR}
 @return color @{constants.COLOR}
 @function pixel
 */
static int luaCB_display_pixel(lua_State * L)
{
    LUA_PARAM_INT(x, 1);
    LUA_PARAM_INT(y, 2);
    LUA_PARAM_INT_OPTIONAL(color, 3, -1);
    if(color == -1)
    {
        lua_pushinteger(L, bmp_getpixel(x, y));
    }
    else
    {
        bmp_putpixel(x, y, (uint8_t)color);
        lua_pushinteger(L, color);
    }
    return 1;
}

/***
 Draw a line
 @tparam int x1
 @tparam int y1
 @tparam int x2
 @tparam int y2
 @tparam int color @{constants.COLOR}
 @function line
 */
static int luaCB_display_line(lua_State * L)
{
    LUA_PARAM_INT(x1, 1);
    LUA_PARAM_INT(y1, 2);
    LUA_PARAM_INT(x2, 3);
    LUA_PARAM_INT(y2, 4);
    LUA_PARAM_INT(color, 5);
    draw_line(x1, y1, x2, y2, (uint8_t)color);
    return 0;
}

/***
 Draw a rect
 @tparam int x
 @tparam int y
 @tparam int w
 @tparam int h
 @tparam int stroke outline @{constants.COLOR}
 @tparam[opt] int fill fill @{constants.COLOR}
 @function rect
 */
static int luaCB_display_rect(lua_State * L)
{
    LUA_PARAM_INT(x, 1);
    LUA_PARAM_INT(y, 2);
    LUA_PARAM_INT(w, 3);
    LUA_PARAM_INT(h, 4);
    LUA_PARAM_INT(stroke, 5);
    LUA_PARAM_INT_OPTIONAL(fill, 6, -1);
    if(fill >= 0) bmp_fill((uint8_t)fill, x, y, w, h);
    if(stroke >= 0) bmp_draw_rect((uint8_t)stroke, x, y, w, h);
    return 0;
}

/***
 Draw a circle
 @tparam int x center x
 @tparam int y center y
 @tparam int r radius
 @tparam int stroke outline @{constants.COLOR}
 @tparam[opt] int fill fill @{constants.COLOR}
 @function circle
 */
static int luaCB_display_circle(lua_State * L)
{
    LUA_PARAM_INT(x, 1);
    LUA_PARAM_INT(y, 2);
    LUA_PARAM_INT(r, 3);
    LUA_PARAM_INT(stroke, 4);
    LUA_PARAM_INT_OPTIONAL(fill, 5, -1);
    if(fill >= 0) fill_circle(x, y, r, (uint8_t)fill);
    if(stroke >= 0) draw_circle(x, y, r, (uint8_t)stroke);
    return 0;
}

/***
 Draw to the idle buffer (for double buffered drawing)
 @function draw_start
 */
static int luaCB_display_draw_start(lua_State * L)
{
    AcquireRecursiveLock(bmp_lock, 0);
    bmp_idle_copy(0,0);
    bmp_draw_to_idle(1);
    return 0;
}

/***
 Copy the idle buffer to the main buffer (for double buffered drawing)
 @function draw_end
 */
static int luaCB_display_draw_end(lua_State * L)
{
    bmp_draw_to_idle(0);
    bmp_idle_copy(1,0);
    ReleaseRecursiveLock(bmp_lock);
    return 0;
}

/***
 Prints a message on the screen for a period of time
 @tparam string text
 @tparam[opt=1000] int timeout in ms
 @function circle
 */
static int luaCB_display_notify_box(lua_State * L)
{
    LUA_PARAM_STRING(text, 1);
    LUA_PARAM_INT_OPTIONAL(timeout, 2, 1000);
    NotifyBox(timeout, "%s", text);
    return 0;
}

static int luaCB_display_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "idle")) lua_pushboolean(L, display_idle());
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_display_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "idle"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

const luaL_Reg displaylib[] =
{
    {"on", luaCB_display_on},
    {"off", luaCB_display_off},
    {"screenshot", luaCB_display_screenshot},
    {"clear", luaCB_display_clear},
    {"print", luaCB_display_print},
    {"pixel", luaCB_display_pixel},
    {"line", luaCB_display_line},
    {"rect", luaCB_display_rect},
    {"circle", luaCB_display_circle},
    {"draw_start", luaCB_display_draw_start},
    {"draw_end", luaCB_display_draw_end},
    {"notify_box", luaCB_display_notify_box},
    {NULL, NULL}
};

LUA_LIB(display);
