
#include <dryos.h>
#include <string.h>
#include <lens.h>
#include <focus.h>

#include "lua_common.h"

static int luaCB_lens_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "name")) lua_pushstring(L, lens_info.name);
    else if(!strcmp(key, "focal_length")) lua_pushinteger(L, lens_info.focal_len);
    else if(!strcmp(key, "focal_distance")) lua_pushinteger(L, lens_info.focus_dist);
    else if(!strcmp(key, "hyperfocal")) lua_pushinteger(L, lens_info.hyperfocal);
    else if(!strcmp(key, "dof_near")) lua_pushinteger(L, lens_info.dof_near);
    else if(!strcmp(key, "dof_far")) lua_pushinteger(L, lens_info.dof_far);
    else if(!strcmp(key, "af")) lua_pushboolean(L, !is_manual_focus());
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_lens_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "name") || !strcmp(key, "focal_length") || !strcmp(key, "focal_distance") || !strcmp(key, "hyperfocal") || !strcmp(key, "dof_near") || !strcmp(key, "dof_far") || !strcmp(key, "af"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static int luaCB_lens_focus(lua_State * L)
{
    LUA_PARAM_INT(num_steps, 1);
    LUA_PARAM_INT_OPTIONAL(step_size, 1, 1);
    LUA_PARAM_INT_OPTIONAL(wait, 1, 0);
    LUA_PARAM_INT_OPTIONAL(extra_delay, 1, 0);
    lua_pushboolean(L, lens_focus(num_steps, step_size, wait, extra_delay));
    return 1;
}

static const luaL_Reg lenslib[] =
{
    { "focus", luaCB_lens_focus },
    { NULL, NULL }
};

LUA_LIB(lens)