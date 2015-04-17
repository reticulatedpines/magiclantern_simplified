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
 @tparam[opt] integer mode
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
 @tparam integer x
 @tparam integer y
 @tparam[opt] integer font @{constants.FONT}
 @tparam[opt] integer fg foreground @{constants.COLOR}
 @tparam[opt] integer bg background @{constants.COLOR}
 @function print
 */
static int luaCB_display_print(lua_State * L)
{
    LUA_PARAM_STRING(str, 1);
    LUA_PARAM_INT(x, 2);
    LUA_PARAM_INT(y, 3);
    LUA_PARAM_INT_OPTIONAL(font, 4, (int)FONT_MED);
    LUA_PARAM_INT_OPTIONAL(fg, 5, COLOR_WHITE);
    LUA_PARAM_INT_OPTIONAL(bg, 6, COLOR_BLACK);
    bmp_printf(FONT(font, fg, bg), x, y, "%s", str);
    return 0;
}

/***
 Set a pixel to a color 
 (omit color param to read the current color)
 @tparam integer x
 @tparam integer y
 @tparam[opt] integer color @{constants.COLOR}
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
 @tparam integer x1
 @tparam integer y1
 @tparam integer x2
 @tparam integer y2
 @tparam integer color @{constants.COLOR}
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
 @tparam integer x
 @tparam integer y
 @tparam integer w
 @tparam integer h
 @tparam integer stroke outline @{constants.COLOR}
 @tparam[opt] integer fill fill @{constants.COLOR}
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
 @tparam integer x center x
 @tparam integer y center y
 @tparam integer r radius
 @tparam integer stroke outline @{constants.COLOR}
 @tparam[opt] integer fill fill @{constants.COLOR}
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
    {NULL, NULL}
};

LUA_LIB(display);
