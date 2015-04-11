/***
 Global Functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module global
 */

#include <dryos.h>
#include <string.h>
#include <shoot.h>
#include <lens.h>
#include <beep.h>
#include "lua_common.h"

/***
 Beep
 @tparam[opt=1] integer times number of times to beep
 @function beep
 */
static int luaCB_beep(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(times, 1, 1);
    beep_times(times);
    return 0;
}

/***
 Calls an eventproc (a function from the camera firmware which can be called by name).
 See Eventprocs. Dangerous.
 @tparam string function the name of the function to call
 @param[opt] arg argument to pass to the call
 @function call
 */
static int luaCB_call(lua_State * L)
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

/***
 Take a picture
 @tparam[opt=64] integer wait how long to wait for camera to be ready to take a picture
 @tparam[opt=true] boolean should_af whether or not to use auto focus
 @function shoot
 */
static int luaCB_shoot(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(wait, 1, 64);
    LUA_PARAM_INT_OPTIONAL(should_af, 2, 1);
    int result = lens_take_picture(wait, should_af);
    lua_pushinteger(L, result);
    return 1;
}

/***
 Pauses for ms miliseconds and allows other tasks to run.
 @tparam integer amount number of milliseconds to sleep
 @function msleep
 */
static int luaCB_msleep(lua_State * L)
{
    LUA_PARAM_INT(amount, 1);
    msleep(amount);
    return 0;
}

static const luaL_Reg globallib[] =
{
    { "msleep", luaCB_msleep },
    { "beep", luaCB_beep },
    { "call", luaCB_call },
    { "shoot", luaCB_shoot },
    { NULL, NULL }
};

int luaopen_globals(lua_State * L)
{
    lua_getglobal(L, "_G");
    luaL_setfuncs(L, globallib, 0);
    return 1;
}
