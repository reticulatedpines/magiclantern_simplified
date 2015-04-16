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

static int luaCB_card_index(lua_State * L);
static int luaCB_card_newindex(lua_State * L);

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
    /// Get the card ML started from
    // @tfield card ml_card
    else if(!strcmp(key, "ml_card"))
    {
        lua_newtable(L);
        lua_pushlightuserdata(L, get_ml_card());
        lua_setfield(L, -2, "_card_ptr");
        lua_pushcfunction(L, luaCB_card_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_card_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
    /// Get the shooting card
    // @tfield card shooting_card
    else if(!strcmp(key, "shooting_card"))
    {
        lua_newtable(L);
        lua_pushlightuserdata(L, get_shooting_card());
        lua_setfield(L, -2, "_card_ptr");
        lua_pushcfunction(L, luaCB_card_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_card_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
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

/// Represents a card (storage media)
// @type card

static int luaCB_card_index(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(lua_getfield(L, 1, "_card_ptr") == LUA_TLIGHTUSERDATA)
    {
        struct card_info * card = lua_touserdata(L, -1);
        /// The cluster size
        // @tfield integer cluster_size
        if(!strcmp(key, "cluster_size")) lua_pushinteger(L, card->cluster_size);
        /// The drive letter
        // @tfield string drive_letter
        else if(!strcmp(key, "drive_letter")) lua_pushstring(L, card->drive_letter);
        /// The current Canon file number
        // @tfield integer file_number
        else if(!strcmp(key, "file_number")) lua_pushinteger(L, card->file_number);
        /// The current Canon folder number
        // @tfield integer folder_number
        else if(!strcmp(key, "folder_number")) lua_pushinteger(L, card->folder_number);
        /// The current free space (in MiB)
        // @tfield integer free_space
        else if(!strcmp(key, "free_space")) lua_pushinteger(L, get_free_space_32k(card) * 1024 / 32);
        /// The type of card
        // @tfield string type
        else if(!strcmp(key, "type")) lua_pushstring(L, card->type);
    }
    else
    {
        return luaL_error(L, "could not get lightuserdata for card");
    }
    return 1;
}
static int luaCB_card_newindex(lua_State * L)
{
    return luaL_error(L, "'card' type is readonly");
}

const luaL_Reg dryoslib[] =
{
    {"call", luaCB_dryos_call},
    {NULL, NULL}
};

LUA_LIB(dryos)
