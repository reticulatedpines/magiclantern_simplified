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
    { "shoot", luaCB_shoot },
    { NULL, NULL }
};

int luaopen_globals(lua_State * L)
{
    lua_getglobal(L, "_G");
    luaL_setfuncs(L, globallib, 0);
    return 1;
}
