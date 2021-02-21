/***
 Global Functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module global
 */

#include <dryos.h>
#include <compiler.h>
#include <string.h>
#include <shoot.h>
#include <lens.h>
#include <beep.h>
#include "lua_common.h"

/***
 Beep
 @tparam[opt=1] int times number of times to beep
 @tparam[opt] int duration beep duration (ms)
 @tparam[opt] int frequency beep frequency (Hz)
 @function beep
 */
static int luaCB_beep(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(times, 1, 1);
    LUA_PARAM_INT_OPTIONAL(duration, 2, 0);
    LUA_PARAM_INT_OPTIONAL(frequency, 3, 440);
    
    if (duration && frequency)
    {
        if (times == 1)
        {
            /* only one beep, no need to block */
            beep_custom(duration, frequency, 0);
        }
        else
        {
            /* repeated beeps - easier to implement a blocking routine */
            for (int i = 0; i < times; i++)
            {
                beep_custom(duration, frequency, 1);
                msleep(duration/2);
            }
        }
    }
    else
    {
        beep_times(times);
    }
    
    return 0;
}

/***
 Pauses for `s` seconds (floating point) and allows other tasks to run.

 This will block other tasks/events from this script, but will allow
 other scripts, ML tasks or Canon tasks.

 Timer resolution: 10ms.
 
 @tparam float s number of seconds to sleep.
 @function sleep
 */
static int luaCB_sleep(lua_State * L)
{
    LUA_PARAM_NUMBER(s, 1);
    msleep((int) roundf(s * 1000.0));
    return 0;
}

/***
 Pauses for `ms` milliseconds and allows other tasks to run.

 Consider using `sleep` for large delays, e.g. `sleep(2)` instead of `msleep(2000)` for readability (there's no functional difference between the two).

 @tparam int ms number of milliseconds to sleep.
 @function msleep
 */
static int luaCB_msleep(lua_State * L)
{
    LUA_PARAM_INT(ms, 1);
    msleep(ms);
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
 @tparam[opt=1] int times Number of times to blink.
 @tparam[opt=50] int delay_on How long the LED is on (in ms)
 @tparam[opt=50] int delay_off How long the LED is off (in ms)
 @function led_blink
 */
static int luaCB_led_blink(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(times, 1, 1);
    LUA_PARAM_INT_OPTIONAL(delay_on, 2, 50);
    LUA_PARAM_INT_OPTIONAL(delay_off, 3, 50);
    info_led_blink(times, delay_on, delay_off);
    return 0;
}

static const luaL_Reg globallib[] =
{
    { "sleep", luaCB_sleep },
    { "msleep", luaCB_msleep },
    { "beep", luaCB_beep },
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
