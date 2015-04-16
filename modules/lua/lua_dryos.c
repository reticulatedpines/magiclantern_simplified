/***
 DryOS functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module dryos
 */

#include <dryos.h>
#include <fileprefix.h>
#include <string.h>

#include "lua_common.h"

/***
 Calls an eventproc (a function from the camera firmware which can be called by name).
 See Eventprocs. Dangerous.
 @tparam string function the name of the function to call
 @param[opt] arg argument to pass to the call
 @function call
 */
static int luaCB_dryos_call(lua_State * L)
{
    LUA_PARAM_STRING(function_name, 1);
    int result = 0;
    int argc = lua_gettop(L);
    
    if(argc <= 1)
    {
        result = call(function_name);
    }
    else if(lua_isinteger(L, 2))
    {
        int arg = lua_tointeger(L, 2);
        result = call(function_name, arg);
    }
    else if(lua_isnumber(L, 2))
    {
        float arg = lua_tonumber(L, 2);
        result = call(function_name, arg);
    }
    else if(lua_isstring(L, 2))
    {
        const char * arg = lua_tostring(L, 2);
        result = call(function_name, arg);
    }
    
    lua_pushinteger(L, result);
    return 1;
}

static void setfield (lua_State *L, const char *key, int value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void setboolfield (lua_State *L, const char *key, int value) {
    if (value < 0)  /* undefined? */
        return;  /* does not set field */
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

static int luaCB_dryos_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get the value of the seconds clock
    // @tfield integer clock
    if(!strcmp(key, "clock")) lua_pushinteger(L, get_seconds_clock());
    /// Get the value of the milliseconds clock
    // @tfield integer ms_clock
    else if(!strcmp(key, "ms_clock")) lua_pushinteger(L, get_ms_clock_value());
    /// Get/Set the image filename prefix
    // @tfield string prefix
    else if(!strcmp(key, "prefix")) lua_pushstring(L, get_file_prefix());
    /// Get the path to the DCIM directory
    // @tfield string dcim_dir
    else if(!strcmp(key, "dcim_dir")) lua_pushstring(L, get_dcim_dir());
    /// Gets a table representing the current date/time
    // @tfield table date
    else if(!strcmp(key, "date"))
    {
        struct tm tm;
        LoadCalendarFromRTC(&tm);
        lua_newtable(L);
        setfield(L, "sec", tm.tm_sec);
        setfield(L, "min", tm.tm_min);
        setfield(L, "hour", tm.tm_hour);
        setfield(L, "day", tm.tm_mday);
        setfield(L, "month", tm.tm_mon+1);
        setfield(L, "year", tm.tm_year+1900);
        setfield(L, "wday", tm.tm_wday+1);
        setfield(L, "yday", tm.tm_yday+1);
        setboolfield(L, "isdst", tm.tm_isdst);
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_dryos_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "clock") || !strcmp(key, "ms_clock") || !strcmp(key, "date") || !strcmp(key, "ml_card") || !strcmp(key, "dcim_dir"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else if(!strcmp(key, "prefix"))
    {
        LUA_PARAM_STRING(new_prefix, 3);
        file_prefix_set(new_prefix);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

const luaL_Reg dryoslib[] =
{
    {"call", luaCB_dryos_call},
    {NULL, NULL}
};

LUA_LIB(dryos)
