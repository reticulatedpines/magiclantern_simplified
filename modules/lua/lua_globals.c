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

/***
 Turns the LED on
 @function led_on
 */
static int luaCB_led_on(lua_State * L)
{
    info_led_on();
    return 0;
}

/***
 Turns the LED off
 @function led_off
 */
static int luaCB_led_off(lua_State * L)
{
    info_led_off();
    return 0;
}

/***
 Blinks the LED
 @tparam[opt=1] integer times Number of times to blink.
 @tparam[opt=50] integer delay_on How long the LED is on (in ms)
 @tparam[opt=50] integer delay_off How long the LED is off (in ms)
 @function led_blink
 */
static int luaCB_led_blink(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(times, 1, 1);
    LUA_PARAM_INT_OPTIONAL(delay_on, 1, 50);
    LUA_PARAM_INT_OPTIONAL(delay_off, 1, 50);
    info_led_blink(times, delay_on, delay_off);
    return 0;
}

static const luaL_Reg globallib[] =
{
    { "msleep", luaCB_msleep },
    { "beep", luaCB_beep },
    { "shoot", luaCB_shoot },
    { "led_on", luaCB_led_on },
    { "led_off", luaCB_led_off },
    { "led_blink", luaCB_led_blink },
    { NULL, NULL }
};

int luaopen_globals(lua_State * L)
{
    lua_getglobal(L, "_G");
    luaL_setfuncs(L, globallib, 0);
    return 1;
}
