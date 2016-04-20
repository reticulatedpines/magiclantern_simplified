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

#define NOT_AVAILABLE "function not available on this camera"
extern WEAK_FUNC(ret_0) int GetBatteryLevel();
extern WEAK_FUNC(ret_0) int GetBatteryHist();
extern WEAK_FUNC(ret_0) int GetBatteryPerformance();
extern WEAK_FUNC(ret_0) int GetBatteryTimeRemaining();
extern WEAK_FUNC(ret_0) int GetBatteryDrainRate();

static int luaCB_battery_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get battery level in percentage (0-100)
    // @tfield int level
    if(!strcmp(key, "level"))
    {
        if((void*)&GetBatteryLevel == (void*)&ret_0) return luaL_error(L, NOT_AVAILABLE);
        lua_pushinteger(L, GetBatteryLevel());
    }
    /// Get battery ID, as registered in Canon menu
    // @tfield int id
    else if(!strcmp(key, "id"))
    {
        if((void*)&GetBatteryHist == (void*)&ret_0) return luaL_error(L, NOT_AVAILABLE);
        lua_pushinteger(L, GetBatteryHist());
    }
    /// Get how many "green dots" (3 for a new battery, less for a used battery)
    // @tfield int performance
    else if(!strcmp(key, "performance"))
    {
        if((void*)&GetBatteryPerformance == (void*)&ret_0) return luaL_error(L, NOT_AVAILABLE);
        lua_pushinteger(L, GetBatteryPerformance());
    }
    /// Get estimated time remaining
    // @tfield int time
    else if(!strcmp(key, "time"))
    {
        if((void*)&GetBatteryTimeRemaining == (void*)&ret_0) return luaL_error(L, NOT_AVAILABLE);
        lua_pushinteger(L, GetBatteryTimeRemaining());
    }
    /// Get estimated battery drain rate
    // @tfield int drain_rate
    else if(!strcmp(key, "drain_rate"))
    {
        if((void*)&GetBatteryDrainRate == (void*)&ret_0) return luaL_error(L, NOT_AVAILABLE);
        lua_pushinteger(L, GetBatteryDrainRate());
    }
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

static const char * lua_battery_fields[] =
{
    "level",
    "id",
    "performance",
    "time",
    "drain_rate",
    NULL
};

static const luaL_Reg batterylib[] =
{
    { NULL, NULL }
};

LUA_LIB(battery)
