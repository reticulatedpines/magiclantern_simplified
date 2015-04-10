
#include <dryos.h>
#include <string.h>
#include <console.h>
#include "lua_common.h"

static int luaCB_console_show(lua_State * L)
{
    console_show();
    return 0;
}

static int luaCB_console_hide(lua_State * L)
{
    console_hide();
    return 0;
}

static int luaCB_console_write(lua_State * L)
{
    if(lua_isstring(L, 1)) console_puts(lua_tostring(L, 1));
    return 0;
}

static int luaCB_console_index(lua_State * L) { lua_rawget(L, 1); return 1; }
static int luaCB_console_newindex(lua_State * L) { lua_rawset(L, 1); return 0; }

static const luaL_Reg consolelib[] =
{
    { "show", luaCB_console_show },
    { "hide", luaCB_console_hide },
    { "write", luaCB_console_write },
    { NULL, NULL }
};

LUA_LIB(console)