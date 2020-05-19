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

#include "lua_common.h"

struct lua_task_func
{
    lua_State * L;
    int function_ref;
};

static int luaCB_card_index(lua_State * L);
static int luaCB_card_newindex(lua_State * L);

static void lua_run_task(struct lua_task_func * lua_task_func)
{
    if(lua_task_func)
    {
        lua_State * L = lua_task_func->L;
        struct semaphore * sem = NULL;
        if(!lua_take_semaphore(L, 0, &sem) && sem)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, lua_task_func->function_ref) == LUA_TFUNCTION)
            {
                if(docall(L, 0, 0))
                {
                    fprintf(stderr, "script failed:\n %s\n", lua_tostring(L, -1));
                }
                else
                {
                    printf("script finished\n");
                }
                luaL_unref(L, LUA_REGISTRYINDEX, lua_task_func->function_ref);
            }
            give_semaphore(sem);
        }
        else
        {
            printf("lua semaphore timeout: run task (%dms)\n", 0);
        }
        free(lua_task_func);
    }
}

/***
 Creates a new task. It will begin executing when you return or call task.yield()
 @tparam function f the function to run
 @tparam[opt] int priority
 @tparam[opt] int stack_size
 @function create
 */
static int luaCB_task_create(lua_State * L)
{
    if(!lua_isfunction(L, 1)) return luaL_argerror(L, 1, "function expected");
    LUA_PARAM_INT_OPTIONAL(priority, 2, 0x1c);
    LUA_PARAM_INT_OPTIONAL(stack_size, 3, 0x10000);
    
    struct lua_task_func * func = malloc(sizeof(struct lua_task_func));
    if(!func) return luaL_error(L, "malloc error\n");
    
    func->L = L;
    func->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    char task_name[32];
    static int lua_task_id = 0;
    snprintf(task_name,32,"lua_run_task[%d]",lua_task_id++);
    task_create(task_name, priority, stack_size, lua_run_task, func);
    return 0;
}

/***
 Yields execution of this script to other tasks and event handlers
 @tparam int duration how long to sleep for in ms
 @function yield
 */
static int luaCB_task_yield(lua_State * L)
{
    LUA_PARAM_INT(duration, 1);
    lua_give_semaphore(L, NULL);
    msleep(duration);
    lua_take_semaphore(L, 0, NULL);
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
