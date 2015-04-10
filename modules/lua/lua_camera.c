
#include <dryos.h>
#include <string.h>
#include <lens.h>
#include <shoot.h>

#include "lua_common.h"

static int luaCB_camera_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "shutter")) lua_pushinteger(L, RAW2TV(lens_info.raw_shutter));
    else if(!strcmp(key, "aperture")) lua_pushinteger(L, RAW2AV(lens_info.raw_aperture));
    else if(!strcmp(key, "iso")) lua_pushinteger(L, RAW2SV(lens_info.raw_iso));
    else if(!strcmp(key, "ec")) lua_pushinteger(L, RAW2EC(lens_info.ae));
    else if(!strcmp(key, "flash_ec")) lua_pushinteger(L, RAW2EC(lens_info.flash_ae));
    else if(!strcmp(key, "mode")) lua_pushinteger(L, shooting_mode);
    else if(!strcmp(key, "af_mode")) lua_pushinteger(L, af_mode);
    else if(!strcmp(key, "metering_mode")) lua_pushinteger(L, metering_mode);
    else if(!strcmp(key, "drive_mode")) lua_pushinteger(L, drive_mode);
    else if(!strcmp(key, "model")) lua_pushstring(L, camera_model);
    else if(!strcmp(key, "model_short")) lua_pushstring(L, __camera_model_short);
    else if(!strcmp(key, "firmware")) lua_pushstring(L, firmware_version);
    else if(!strcmp(key, "temperature")) lua_pushinteger(L, efic_temp);
    else if(!strcmp(key, "state")) lua_pushinteger(L, gui_state);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_camera_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "shutter"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawshutter(TV2RAW(value));
    }
    else if(!strcmp(key, "aperture"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawaperture(AV2RAW(value));
    }
    else if(!strcmp(key, "iso"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_rawiso(SV2RAW(value));
    }
    else if(!strcmp(key, "ec"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_ae(EC2RAW(value));
    }
    else if(!strcmp(key, "flash_ec"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_flash_ae(EC2RAW(value));
    }
    else if(!strcmp(key, "model") || !strcmp(key, "firmware") || !strcmp(key, "mode") || !strcmp(key, "af_mode") || !strcmp(key, "metering_mode") || !strcmp(key, "drive_mode") || !strcmp(key, "temperature") || !strcmp(key, "state"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static int luaCB_camera_shoot(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(wait, 1, 64);
    LUA_PARAM_INT_OPTIONAL(should_af, 2, 1);
    int result = lens_take_picture(wait, should_af);
    lua_pushinteger(L, result);
    return 1;
}

static int luaCB_camera_bulb(lua_State * L)
{
    LUA_PARAM_INT(duration, 1);
    bulb_take_pic(duration);
    return 0;
}

static const luaL_Reg cameralib[] =
{
    { "shoot", luaCB_camera_shoot },
    { "bulb", luaCB_camera_bulb },
    { NULL, NULL }
};

LUA_LIB(camera)
