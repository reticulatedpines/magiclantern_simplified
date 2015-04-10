
#include <dryos.h>
#include <string.h>
#include <shoot.h>
#include <property.h>
#include <zebra.h>

#include "lua_common.h"

static int luaCB_lv_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "enabled")) lua_pushboolean(L, lv);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_lv_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "enabled"))
    {
        LUA_PARAM_BOOL(value, 3);
        if(value && !lv && !LV_PAUSED) force_liveview();
        else if(lv) close_liveview();
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static int luaCB_lv_start(lua_State * L)
{
    force_liveview();
    return 0;
}

static int luaCB_lv_pause(lua_State * L)
{
    PauseLiveView();
    return 0;
}

static int luaCB_lv_resume(lua_State * L)
{
    ResumeLiveView();
    return 0;
}

static int luaCB_lv_stop(lua_State * L)
{
    close_liveview();
    return 0;
}

static const luaL_Reg lvlib[] =
{
    { "start", luaCB_lv_start },
    { "pause", luaCB_lv_pause },
    { "resume", luaCB_lv_resume },
    { "stop", luaCB_lv_stop },
    { NULL, NULL }
};

LUA_LIB(lv)