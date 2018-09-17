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
extern int waiting_for_keypress;
int module_send_keypress(int module_key);

/***
 Send a keypress.
 @tparam constants.KEY key the key to press.
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
 
 FIXME: while waiting for a key to be pressed,
 a task.yield() is performed, with identical limitations.
 
 @tparam[opt] constants.KEY key
 @tparam[opt] int timeout (milliseconds; 0 = wait forever)
 @treturn constants.KEY the key that was pressed.
 @function wait
 */
static int luaCB_key_wait(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(key, 1, 0);
    LUA_PARAM_INT_OPTIONAL(timeout, 2, 0);
    uint32_t pressed_key = 0;

    if (lua_get_cant_yield(L))
    {
        return luaL_error(L, "FIXME: cannot use task.yield() or key.wait() from two tasks");
    }

    /* clear "keypress buffer" and block the key(s) we are waiting for */
    lua_msg_queue_receive(L, &pressed_key, 10);
    waiting_for_keypress = key;

    /* let's hope the stack is not going to be modified by other tasks */
    int t1 = lua_gettop(L);         /* current stack top */
    int v1 = lua_tointeger(L, t1);  /* duration (integer) */

    /* let other script tasks run */
    lua_give_semaphore(L, NULL);

    /* wait for key to be pressed, or for timeout */
    int err = lua_msg_queue_receive(L, &pressed_key, timeout);

    /* other script tasks no longer allowed */
    lua_take_semaphore(L, 0, NULL);

    /* check the stack */
    int t2 = lua_gettop(L);
    int v2 = lua_tointeger(L, t2);
    ASSERT(t1 == t2);
    ASSERT(v1 == v2);

    if (err)
    {
        lua_pushinteger(L, 0);
        waiting_for_keypress = -1;
        return 1;
    }

    /* waiting_for_keypress was already disabled when the pressed key was placed in the queue */
    lua_pushinteger(L, pressed_key);
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
