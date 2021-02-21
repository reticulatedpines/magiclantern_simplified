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
#include <powersave.h>

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

static int luaCB_bitmap_index(lua_State * L);
static int luaCB_bitmap_newindex(lua_State * L);

/***
 Loads a bitmap file so it can be drawn on the screen
 @tparam string filename
 @treturn bitmap
 @function load
 */
static int luaCB_display_load(lua_State * L)
{
    LUA_PARAM_STRING(filename, 1);
    struct bmp_file_t * bmp_file = bmp_load(filename, 1);
    if(!bmp_file) return luaL_error(L, "Error loading bitmap file");
    lua_newtable(L);
    lua_pushlightuserdata(L, bmp_file);
    lua_setfield(L, -2, "_ptr");
    lua_pushcfunction(L, luaCB_bitmap_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_bitmap_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    return 1;
}

/***
 Runs the given function and double-buffers all the drawing done by the function
 Then copies the buffer back to the actual screen buffer when the function returns
 Use this to avoid flickering when drawing
 @tparam func draw_func
 @function draw
 */
static int luaCB_display_draw(lua_State *L)
{
    if(!lua_isfunction(L, 1)) return luaL_argerror(L, 1, "expected function");
    
    //if not __display_draw_running
    lua_getglobal(L, "__display_draw_running");
    if(lua_isboolean(L, -1) && lua_toboolean(L, -1))
    {
        //we're already running inside a display.draw call, so just run the function
        lua_pushvalue(L, 1);
        lua_call(L,0,0);
    }
    else
    {
        //__display_draw_running = true
        lua_pushboolean(L, 1);lua_setglobal(L, "__display_draw_running");
        int status = 0;
        
        BMP_LOCK
        (
            bmp_idle_copy(0,0);
            bmp_draw_to_idle(1);
         
            lua_pushvalue(L, 1);
            status = docall(L, 0, 0);
        
            bmp_draw_to_idle(0);
            bmp_idle_copy(1,0);
        )
        
        //__display_draw_running = false
        lua_pushboolean(L, 0);lua_setglobal(L, "__display_draw_running");
        
        //re-throw the error
        if(status != LUA_OK) lua_error(L);
    }
    return 0;
}

/***
 Prints a message on the screen for a period of time
 @tparam string text
 @tparam[opt=1000] int timeout in ms
 @function notify_box
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
    /// The width of the display (720)
    //@tfield int width
    if(!strcmp(key, "width")) lua_pushinteger(L, 720);
    /// The height of the display (480)
    //@tfield int height
    else if(!strcmp(key, "height")) lua_pushinteger(L, 480);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_display_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if (!strcmp(key, "height") || !strcmp(key, "width"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/// Represents a bitmap file that has been loaded
// @type bitmap

/***
 Draws this bitmap image to the screen
 @tparam int x
 @tparam int y
 @tparam[opt] int w
 @tparam[opt] int h
 @function draw
 */
static int luaCB_bitmap_draw(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    if(lua_getfield(L, 1, "_ptr") == LUA_TLIGHTUSERDATA)
    {
        struct bmp_file_t * bmp_file = lua_touserdata(L, -1);
        LUA_PARAM_INT(x, 2);
        LUA_PARAM_INT(y, 3);
        LUA_PARAM_INT_OPTIONAL(w, 4, (int)bmp_file->width);
        LUA_PARAM_INT_OPTIONAL(h, 5, (int)bmp_file->height);
        bmp_draw_scaled_ex(bmp_file, x, y, w, h, 0);
    }
    else
    {
        return luaL_error(L, "Invalid pointer to bitmap file");
    }
    return 0;
}

static int luaCB_bitmap_index(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(lua_getfield(L, 1, "_ptr") == LUA_TLIGHTUSERDATA)
    {
        struct bmp_file_t * bmp_file = lua_touserdata(L, -1);
        /// Get the bits per pixel
        // @tfield int bits_per_pixel
        if(!strcmp(key, "bits_per_pixel")) lua_pushinteger(L, bmp_file->bits_per_pixel);
        /// Get the image width
        // @tfield int width
        else if(!strcmp(key, "width")) lua_pushinteger(L, bmp_file->width);
        /// Get the image height
        // @tfield int height
        else if(!strcmp(key, "height")) lua_pushinteger(L, bmp_file->height);
        /// Get the number of colors
        // @tfield int num_colors
        else if(!strcmp(key, "num_colors")) lua_pushinteger(L, bmp_file->num_colors);
        /// Get the image size
        // @tfield int size
        else if(!strcmp(key, "size")) lua_pushinteger(L, bmp_file->size);
        /// Get the image signature
        // @tfield int signature
        else if(!strcmp(key, "signature")) lua_pushinteger(L, bmp_file->signature);
        /// Get the image horizontal resolution
        // @tfield int hpix_per_meter
        else if(!strcmp(key, "hpix_per_meter")) lua_pushinteger(L, bmp_file->hpix_per_meter);
        /// Get the image vertical resolution
        // @tfield int vpix_per_meter
        else if(!strcmp(key, "vpix_per_meter")) lua_pushinteger(L, bmp_file->vpix_per_meter);
        else if(!strcmp(key, "draw")) lua_pushcfunction(L, luaCB_bitmap_draw);
        else return lua_rawget(L, 1);
    }
    else
    {
        return luaL_error(L, "could not get lightuserdata for card");
    }
    return 1;
}
static int luaCB_bitmap_newindex(lua_State * L)
{
    return luaL_error(L, "'bitmap' type is readonly");
}

static const char * lua_display_fields[] =
{
    "idle",
    "height",
    "width",
    NULL
};

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
    {"load", luaCB_display_load},
    {"draw", luaCB_display_draw},
    {"notify_box", luaCB_display_notify_box},
    {NULL, NULL}
};

LUA_LIB(display);
