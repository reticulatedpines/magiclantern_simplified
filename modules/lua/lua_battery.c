/***
 Battery properties
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module battery
 */

#include <dryos.h>
#include <string.h>
#include <battery.h>

#include "lua_common.h"

static int luaCB_battery_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get battery level in percentage (0-100)
    // @tfield integer level
    if(!strcmp(key, "level")) lua_pushinteger(L, GetBatteryLevel());
    /// Get battery ID, as registered in Canon menu
    // @tfield integer id
    else if(!strcmp(key, "id")) lua_pushinteger(L, GetBatteryHist());
    /// Get how many "green dots" (3 for a new battery, less for a used battery)
    // @tfield integer performance
    else if(!strcmp(key, "performance")) lua_pushinteger(L, GetBatteryPerformance());
    /// Get estimated time remaining
    // @tfield integer time
    else if(!strcmp(key, "time")) lua_pushinteger(L, GetBatteryTimeRemaining());
    /// Get estimated battery drain rate
    // @tfield integer drain_rate
    else if(!strcmp(key, "drain_rate")) lua_pushinteger(L, GetBatteryDrainRate());
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_battery_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "level") || !strcmp(key, "id") || !strcmp(key, "performance") || !strcmp(key, "time") || !strcmp(key, "drain_rate"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static const luaL_Reg batterylib[] =
{
    { NULL, NULL }
};

LUA_LIB(battery)
