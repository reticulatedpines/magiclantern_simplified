/***
 Basic camera operations and properties
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module camera
 */

#include <dryos.h>
#include <string.h>
#include <lens.h>
#include <shoot.h>

#include "lua_common.h"

static int luaCB_camera_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Shutter shutter speed in apex units x10
    // @tfield integer shutter
    if(!strcmp(key, "shutter")) lua_pushinteger(L, RAW2TV(lens_info.raw_shutter));
    /// Lens aperture in apex units x10
    // @tfield integer aperture
    else if(!strcmp(key, "aperture")) lua_pushinteger(L, RAW2AV(lens_info.raw_aperture));
    /// ISO in apex units x10
    // @tfield integer iso
    else if(!strcmp(key, "iso")) lua_pushinteger(L, RAW2SV(lens_info.raw_iso));
    /// Exposure compensation in apex units x10
    // @tfield integer ec
    else if(!strcmp(key, "ec")) lua_pushinteger(L, RAW2EC(lens_info.ae));
    /// Flash exposure compensation in apex units x10
    // @tfield integer flash_ec
    else if(!strcmp(key, "flash_ec")) lua_pushinteger(L, RAW2EC(lens_info.flash_ae));
    /// The current camera mode, possible values defined in @{constants.MODE}
    // @tfield integer mode
    else if(!strcmp(key, "mode")) lua_pushinteger(L, shooting_mode);
    /// The current auto focus mode
    // @tfield integer af_mode readonly
    else if(!strcmp(key, "af_mode")) lua_pushinteger(L, af_mode);
    /// The current metering mode
    // @tfield integer metering_mode readonly
    else if(!strcmp(key, "metering_mode")) lua_pushinteger(L, metering_mode);
    /// The current drive mode
    // @tfield integer drive_mode readonly
    else if(!strcmp(key, "drive_mode")) lua_pushinteger(L, drive_mode);
    /// The model name of the camera
    // @tfield string model readonly
    else if(!strcmp(key, "model")) lua_pushstring(L, camera_model);
    /// The shortened model name of the camera (e.g. 5D3)
    // @tfield string model_short readonly
    else if(!strcmp(key, "model_short")) lua_pushstring(L, __camera_model_short);
    /// The Canon firmware version string
    // @tfield string firmware readonly
    else if(!strcmp(key, "firmware")) lua_pushstring(L, firmware_version);
    /// The temperature from the efic chip in raw units
    // @tfield integer temperature readonly
    else if(!strcmp(key, "temperature")) lua_pushinteger(L, efic_temp);
    /// The current Canon GUI state of the camera (PLAY, QR, LV, etc)
    // @tfield integer state readonly
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

/***
 Take a picture
 @tparam[opt=64] integer wait how long to wait for camera to be ready to take a picture
 @tparam[opt=true] boolean should_af whether ot not to us auto focus
 @function shoot
 */
static int luaCB_camera_shoot(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(wait, 1, 64);
    LUA_PARAM_INT_OPTIONAL(should_af, 2, 1);
    int result = lens_take_picture(wait, should_af);
    lua_pushinteger(L, result);
    return 1;
}

/***
 Take a picture in bulb mode
 @tparam integer duration bulb duration in seconds
 @function bulb
 */
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
