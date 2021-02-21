/***
 Intervalometer operations and properties.
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module interval
 */

#include <dryos.h>
#include <string.h>
#include <shoot.h>

#include "lua_common.h"

static int luaCB_interval_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set the interval time (in seconds).
    // @tfield int time
    if(!strcmp(key, "time")) lua_pushinteger(L, get_interval_time());
    /// Get the current number of pictures that have been taken.
    // @tfield int count readonly
    else if(!strcmp(key, "count")) lua_pushinteger(L, get_interval_count());
    /// Get whether or not the intervalometer is currently running.
    // @tfield bool running readonly
    else if(!strcmp(key, "running")) lua_pushboolean(L, is_intervalometer_running());
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_interval_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "time"))
    {
        LUA_PARAM_INT(value, 3);
        set_interval_time(value);
    }
    else if(!strcmp(key, "running") || !strcmp(key, "count"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/***
 Stop the intervalometer
 @function stop
 */
static int luaCB_interval_stop(lua_State * L)
{
    intervalometer_stop();
    return 0;
}

static const char * lua_interval_fields[] =
{
    "time",
    "count",
    "running",
    NULL
};

static const luaL_Reg intervallib[] =
{
    { "stop", luaCB_interval_stop },
    { NULL, NULL }
};

LUA_LIB(interval)
