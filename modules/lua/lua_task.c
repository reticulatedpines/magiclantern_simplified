/***
 Task functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module task
 */

#include <dryos.h>
#include <fileprefix.h>
#include <string.h>
#include <powersave.h>

#include "lua_common.h"

struct lua_task_func
{
    lua_State * L;
    int function_ref;
    int disable_psave;
};

static int luaCB_card_index(lua_State * L);
static int luaCB_card_newindex(lua_State * L);

static void lua_run_task(struct lua_task_func * lua_task_func)
{
    if(lua_task_func)
    {
        lua_State * L = lua_task_func->L;
        struct semaphore * sem = NULL;
        if (lua_take_semaphore(L, 0, &sem) == 0)
        {
            ASSERT(sem);
            if (lua_get_cant_yield(L) == -1)
            {
                /* main task was unloaded? continuing would be use after free */
                fprintf(stderr, "[%s] will not start new tasks.\n", lua_get_script_filename(L));
                goto skip;
            }
            
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, lua_task_func->function_ref) == LUA_TFUNCTION)
            {
                /* script created a task;
                 * it can't be unloaded while this task is running */
                lua_set_cant_unload(L, 1, LUA_TASK_UNLOAD_MASK);

                if (lua_task_func->disable_psave)
                {
                    powersave_prohibit();
                }

                printf("[%s] task starting.\n", lua_get_script_filename(L));

                if(docall(L, 0, 0))
                {
                    fprintf(stderr, "[%s] task error:\n%s\n", lua_get_script_filename(L), lua_tostring(L, -1));
                    lua_save_last_error(L);
                }
                luaL_unref(L, LUA_REGISTRYINDEX, lua_task_func->function_ref);

                printf("[%s] task exiting.\n", lua_get_script_filename(L));

                if (lua_task_func->disable_psave)
                {
                    powersave_permit();
                }

                /* If all tasks started by the script are finished
                 * _before_ the main task ends, the script can be unloaded.
                 * Note: lua_set_cant_unload keeps a counter of tasks
                 * (number of calls LUA_TASK_UNLOAD_MASK)
                 */
                lua_set_cant_unload(L, 0, LUA_TASK_UNLOAD_MASK);
            }
            else
            {
                /* should be covered in luaCB_task_create */
                ASSERT(0);
            }

        skip:
            give_semaphore(sem);
        }
        else
        {
            printf("[%s] semaphore error: run task\n", lua_get_script_filename(L));
        }
        free(lua_task_func);
    }
}

/***
 Creates a new task. It will begin executing when you return or call task.yield().
 
 Tasks spawned from Lua can optionally run with powersaving disabled
 (so the camera won't turn off while running them) if you enable `disable_powesave`
 (for example, `task.create(my_func, nil, nil, true)`).
 
 @tparam function f the function to run
 @tparam[opt] int priority DryOS task priority (0x1F = lowest priority; lower values = higher priority; default 0x1C)
 @tparam[opt] int stack_size Memory reserved for this task to be used as stack (default 0x10000)
 @tparam[opt=false] bool disable_powersave Set to `true` to prevent the camera from turning off while running this task.
 @function create
 */
static int luaCB_task_create(lua_State * L)
{
    if(!lua_isfunction(L, 1)) return luaL_argerror(L, 1, "function expected");
    LUA_PARAM_INT_OPTIONAL(priority, 2, 0x1c);
    LUA_PARAM_INT_OPTIONAL(stack_size, 3, 0x10000);
    LUA_PARAM_BOOL_OPTIONAL(disable_psave, 4, 0);

    struct lua_task_func * func = malloc(sizeof(struct lua_task_func));
    if(!func) return luaL_error(L, "malloc error\n");

    func->L = L;
    func->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    func->disable_psave = disable_psave;
    uint32_t ret = (uint32_t) task_create("lua_script_task", priority, stack_size, lua_run_task, func);
    if (ret & 1) return luaL_error(L, "task not started\n");
    return 0;
}

/***
 Yields execution of this script to other tasks and event handlers
 (also from the same script).
 
 FIXME: once a task executed yield(), other tasks or event handlers
 from the same script, that might interrupt the former, **must not** execute yield().
 Otherwise, an error will be thrown to prevent memory corruption, camera lockup or worse.

 The above limitation is a very dirty hack - a proper fix should be implemented
 by somebody familiar with multitasking in Lua.
 Help is more than welcome, as this topic is
 [not exactly our cup of tea](http://www.magiclantern.fm/forum/index.php?topic=14828.msg179227#msg179227).

 TODO: replace with msleep?
 @tparam int duration how long to sleep for in ms.
 @function yield
 */
static int luaCB_task_yield(lua_State * L)
{
    LUA_PARAM_INT(duration, 1);

    /* hack: figure out whether we might be interrupting
     * somebody else who called task.yield() */
    if (lua_get_cant_yield(L))
    {
        return luaL_error(L, "FIXME: cannot use task.yield() from two tasks");
    }

    /* let's hope the stack is not going to be modified by other tasks */
    int t1 = lua_gettop(L);         /* current stack top */
    int v1 = lua_tointeger(L, t1);  /* duration (integer) */

    lua_set_cant_yield(L, 1);
    lua_give_semaphore(L, NULL);
    msleep(duration);
    lua_take_semaphore(L, 0, NULL);
    lua_set_cant_yield(L, 0);

    /* check the stack */
    int t2 = lua_gettop(L);
    int v2 = lua_tointeger(L, t2);
    ASSERT(t1 == t2);
    ASSERT(v1 == v2);
    return 0;
}

static int luaCB_task_index(lua_State * L)
{
    lua_rawget(L, 1);
    return 1;
}

static int luaCB_task_newindex(lua_State * L)
{
    lua_rawset(L, 1);
    return 0;
}

static const char * lua_task_fields[] =
{
    NULL
};

const luaL_Reg tasklib[] =
{
    {"create", luaCB_task_create},
    {"yield", luaCB_task_yield},
    {NULL, NULL}
};

LUA_LIB(task)
