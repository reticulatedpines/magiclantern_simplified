/***
 Functions for writing data to the console
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module console
 */

#include <dryos.h>
#include <string.h>
#include <console.h>
#include "lua_common.h"

/***
 Show the console
 @function show
 */
static int luaCB_console_show(lua_State * L)
{
    console_show();
    return 0;
}

/***
 Hide the console
 @function hide
 */
static int luaCB_console_hide(lua_State * L)
{
    console_hide();
    return 0;
}

/***
 Clear the console contents
 @function clear
 */
static int luaCB_console_clear(lua_State * L)
{
    console_clear();
    return 0;
}

/***
 Write some text to the console
 @tparam string text the text to write
 @function write
 */
static int luaCB_console_write(lua_State * L)
{
    if(lua_isstring(L, 1)) printf("%s",lua_tostring(L, 1));
    return 0;
}

static int luaCB_console_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Whether or not the console is displayed.
    // @tfield bool visible
    if(!strcmp(key, "visible"))
    {
        extern int console_visible;
        lua_pushboolean(L, console_visible);
    }
    else lua_rawget(L, 1);
    return 1;
}
static int luaCB_console_newindex(lua_State * L) { lua_rawset(L, 1); return 0; }


static const char * lua_console_fields[] =
{
    NULL
};

static const luaL_Reg consolelib[] =
{
    { "show", luaCB_console_show },
    { "hide", luaCB_console_hide },
    { "clear", luaCB_console_clear },
    { "write", luaCB_console_write },
    { NULL, NULL }
};

LUA_LIB(console)
