/***
 Key functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module key
 */

#include <dryos.h>
#include <string.h>

#include "lua_common.h"

extern int last_keypress;
int module_send_keypress(int module_key);

/***
 Send a keypress.
 @tparam int key the key to press
 @function press
 */
static int luaCB_key_press(lua_State * L)
{
    LUA_PARAM_INT(key, 1);
    module_send_keypress(key);
    return 0;
}

/***
 Wait for a key to be pressed.
 @tparam[opt] int key
 @tparam[opt] int timeout
 @treturn int the key that was pressed
 @function wait
 */
static int luaCB_key_wait(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(key, 1, 0);
    LUA_PARAM_INT_OPTIONAL(timeout, 1, 0);
    timeout *= 10;
    last_keypress = 0;
    int time = 0;
    lua_give_semaphore(L, NULL);
    //TODO: probably better to use a semaphore
    while((key && last_keypress != key) || (!key && !last_keypress))
    {
        msleep(100);
        if(timeout && time++ > timeout)
        {
            lua_take_semaphore(L, 0, NULL);
            lua_pushinteger(L, 0);
            return 1;
        }
    }
    lua_take_semaphore(L, 0, NULL);
    lua_pushinteger(L, last_keypress);
    return 1;
}

static int luaCB_key_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// The last key that was pressed.
    // @tfield int last
    if(!strcmp(key, "last")) lua_pushinteger(L, last_keypress);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_key_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "last"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static const char * lua_key_fields[] =
{
    "last",
    NULL
};

const luaL_Reg keylib[] =
{
    {"press", luaCB_key_press},
    {"wait", luaCB_key_wait},
    {NULL, NULL}
};

LUA_LIB(key)
