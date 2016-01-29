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
#include <bmp.h>
#include <imath.h>

#include "lua_common.h"

static int luaCB_shutter_index(lua_State * L);
static int luaCB_shutter_newindex(lua_State * L);
static int luaCB_shutter_tostring(lua_State * L);

static int luaCB_aperture_index(lua_State * L);
static int luaCB_aperture_newindex(lua_State * L);
static int luaCB_aperture_tostring(lua_State * L);

static int luaCB_iso_index(lua_State * L);
static int luaCB_iso_newindex(lua_State * L);
static int luaCB_iso_tostring(lua_State * L);

static int luaCB_ec_index(lua_State * L);
static int luaCB_ec_newindex(lua_State * L);
static int luaCB_ec_tostring(lua_State * L);

static int luaCB_fec_index(lua_State * L);
static int luaCB_fec_newindex(lua_State * L);
static int luaCB_fec_tostring(lua_State * L);

static int luaCB_camera_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Gets a @{shutter} object that represents the camera's current shutter speed
    // @tfield shutter shutter
    if(!strcmp(key, "shutter"))
    {
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_shutter_tostring);
        lua_setfield(L, -2, "tostring");
        lua_pushcfunction(L, luaCB_shutter_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_shutter_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
    /// Gets an @{aperture} object that represents the lens' aperture
    // @tfield aperture aperture
    else if(!strcmp(key, "aperture"))
    {
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_aperture_tostring);
        lua_setfield(L, -2, "tostring");
        lua_pushcfunction(L, luaCB_aperture_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_aperture_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
    /// Gets an @{iso} object that represents camera's ISO
    // @tfield iso iso
    else if(!strcmp(key, "iso"))
    {
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_iso_tostring);
        lua_setfield(L, -2, "tostring");
        lua_pushcfunction(L, luaCB_iso_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_iso_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
    /// Gets an @{ec} object that represents exposure compensation
    // @tfield ec ec
    else if(!strcmp(key, "ec"))
    {
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_ec_tostring);
        lua_setfield(L, -2, "tostring");
        lua_pushcfunction(L, luaCB_ec_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_ec_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
    /// Gets an @{ec} object that represents flash exposure compensation
    // @tfield ec flash_ec
    else if(!strcmp(key, "flash_ec"))
    {
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_fec_tostring);
        lua_setfield(L, -2, "tostring");
        lua_pushcfunction(L, luaCB_fec_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_fec_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -2);
    }
    /// Get/Set kelvin white balance
    // @tfield int kelvin
    else if(!strcmp(key, "kelvin")) lua_pushinteger(L, lens_info.kelvin);
    /// Get the current camera mode, possible values defined in @{constants.MODE}
    // @tfield int mode
    else if(!strcmp(key, "mode")) lua_pushinteger(L, shooting_mode);
    /// Get the current auto focus mode
    // @tfield int af_mode readonly
    else if(!strcmp(key, "af_mode")) lua_pushinteger(L, af_mode);
    /// Get the current metering mode
    // @tfield int metering_mode readonly
    else if(!strcmp(key, "metering_mode")) lua_pushinteger(L, metering_mode);
    /// Get the current drive mode
    // @tfield int drive_mode readonly
    else if(!strcmp(key, "drive_mode")) lua_pushinteger(L, drive_mode);
    /// Get the model name of the camera
    // @tfield string model readonly
    else if(!strcmp(key, "model")) lua_pushstring(L, camera_model);
    /// Get the shortened model name of the camera (e.g. 5D3)
    // @tfield string model_short readonly
    else if(!strcmp(key, "model_short")) lua_pushstring(L, __camera_model_short);
    /// Get the Canon firmware version string
    // @tfield string firmware readonly
    else if(!strcmp(key, "firmware")) lua_pushstring(L, firmware_version);
    /// Get the temperature from the efic chip in raw units
    // @tfield int temperature readonly
    else if(!strcmp(key, "temperature")) lua_pushinteger(L, efic_temp);
    /// Get the current Canon GUI state of the camera (PLAY, QR, LV, etc)
    // @tfield int state readonly
    else if(!strcmp(key, "state")) lua_pushinteger(L, gui_state);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_camera_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "ec"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_ae(EC2RAW(value));
    }
    else if(!strcmp(key, "flash_ec"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_flash_ae(EC2RAW(value));
    }
    else if(!strcmp(key, "kelvin"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_kelvin(value);
    }
    else if(!strcmp(key, "shutter") || !strcmp(key, "aperture") || !strcmp(key, "iso") || !strcmp(key, "ec"))
    {
        return luaL_error(L, "'%s' field is readonly\nyou can set %s with it's various fields\nex:camera.shutter.value = 1/10", key, key);
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
 @tparam[opt=64] int wait how long to wait for camera to be ready to take a picture
 @tparam[opt=true] bool should_af whether or not to use auto focus
 @function shoot
 */
static int luaCB_camera_shoot(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(wait, 1, 64);
    LUA_PARAM_BOOL_OPTIONAL(should_af, 2, 1);
    int result = lens_take_picture(wait, should_af);
    lua_pushinteger(L, result);
    return 1;
}

/***
 Take a picture in bulb mode
 @tparam int duration bulb duration in seconds
 @function bulb
 */
static int luaCB_camera_bulb(lua_State * L)
{
    LUA_PARAM_INT(duration, 1);
    bulb_take_pic(duration);
    return 0;
}

/***
 Restart the camera
 @function reboot
 */
static int luaCB_camera_reboot(lua_State * L)
{
    int reboot = 0;
    prop_request_change(PROP_REBOOT, &reboot, 4);
    return 0;

    /* shutdown is probably done in a similar way, but I had no success with nearby properties */
}

/// Represents the camera's shutter speed setting
//@type shutter

static int luaCB_shutter_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set shutter speed in Canon raw units
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_shutter);
    /// Get/Set shutter speed in apex units x10
    // @tfield int apex
    else if(!strcmp(key,"apex")) lua_pushinteger(L, RAW2TV(lens_info.raw_shutter));
    /// Get/Set shutter speed in apex units (floating point)
    // @tfield number apexf
    else if(!strcmp(key,"apexf")) lua_pushnumber(L, (RAW2TV(lens_info.raw_shutter)/10.0));
    /// Get/Set shutter speed in milliseconds
    // @tfield int ms
    else if(!strcmp(key,"ms")) lua_pushinteger(L, raw2shutter_ms(lens_info.raw_shutter));
    /// Get/Set shutter speed in seconds (floating point)
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, raw2shutterf(lens_info.raw_shutter));
    else lua_rawget(L, 1);
    return 1;
}

/// Pretty prints the current shutter speed as a string
//@treturn string
//@function tostring
static int luaCB_shutter_tostring(lua_State * L)
{
    lua_pushstring(L, lens_format_shutter(lens_info.raw_shutter));
    return 1;
}

static int luaCB_shutter_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    if(!lens_info.raw_shutter) return luaL_error(L, "Shutter speed is automatic - cannot adjust manually.");
    
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawshutter(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawshutter(TV2RAW(value));
    }
    else if(!strcmp(key, "apexf"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_rawshutter(TV2RAW((int)roundf(value * 10)));
    }
    else if(!strcmp(key, "ms"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawshutter(shutter_ms_to_raw(value));
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_rawshutter(shutterf_to_raw(value));
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/// Represents the lens's aperture
//@type aperture

static int luaCB_aperture_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set aperture in Canon raw units
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_aperture);
    /// Get/Set aperture in apex units x10
    // @tfield int apex
    else if(!strcmp(key,"apex")) lua_pushinteger(L, RAW2AV(lens_info.raw_aperture));
    /// Get/Set aperture in apex units (floating point)
    // @tfield number apexf
    else if(!strcmp(key,"apexf")) lua_pushnumber(L, (RAW2AV(lens_info.raw_aperture) / 10.0));
    /// Get/Set aperture as f-number (floating point)
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, lens_info.aperture / 10.0);
    else lua_rawget(L, 1);
    return 1;
}

/// Pretty prints the current aperture as a string
//@treturn string
//@function tostring
static int luaCB_aperture_tostring(lua_State * L)
{
    int a = lens_info.aperture;
    lua_pushfstring(L, SYM_F_SLASH"%d.%d", a / 10, a % 10);
    return 1;
}

static int luaCB_aperture_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    if (!lens_info.aperture)
    {
        return luaL_error(L, lens_info.name[0] ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture.");
    }
    
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawaperture(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawaperture(AV2RAW(value));
    }
    else if(!strcmp(key, "apexf"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_rawaperture(AV2RAW((int)roundf(value * 10)));
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_rawaperture((int)roundf((log2i(value) * 16) + 8));
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/// Represents the camera ISO
//@type iso

static int luaCB_iso_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set ISO in Canon raw units
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_iso);
    /// Get/Set ISO in apex units x10
    // @tfield int apex
    else if(!strcmp(key,"apex")) lua_pushinteger(L, RAW2SV(lens_info.raw_iso));
    /// Get/Set ISO in apex units (floating point)
    // @tfield number apexf
    else if(!strcmp(key,"apexf")) lua_pushnumber(L, (RAW2SV(lens_info.raw_iso) / 10.0));
    /// Get/Set ISO
    // @tfield int value
    else if(!strcmp(key,"value")) lua_pushinteger(L, raw2iso(lens_info.raw_iso));
    else lua_rawget(L, 1);
    return 1;
}

/// Pretty prints the current ISO as a string
//@treturn string
//@function tostring
static int luaCB_iso_tostring(lua_State * L)
{
    if(lens_info.raw_iso)
    {
        lua_pushfstring(L, SYM_ISO"%d", raw2iso(lens_info.raw_iso));
    }
    else
    {
        lua_pushstring(L, SYM_ISO"Auto");
    }
    return 1;
}

static int luaCB_iso_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawiso(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_rawiso(SV2RAW(value));
    }
    else if(!strcmp(key, "apexf"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_rawiso(SV2RAW((int)roundf(value * 10)));
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_INT(value, 3);
        int i = 0;
        for(i = 0; i < COUNT(values_iso); i++)
            if(values_iso[i] <= value) break;
        lens_set_rawiso(codes_iso[i]);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/// Represents the exposure compensation
//@type ec

static int luaCB_ec_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set exposure compensation in Canon raw units
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.ae);
    /// Get/Set exposure compensation in apex units x10
    // @tfield int apex
    else if(!strcmp(key,"apex")) lua_pushinteger(L, RAW2EC(lens_info.ae));
    /// Get/Set exposure compensation in EVs (floating point)
    // @tfield number apexf
    else if(!strcmp(key,"apexf")) lua_pushnumber(L, (RAW2EC(lens_info.ae) / 10.0));
    /// Get/Set exposure compensation in EVs
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, RAW2EC(lens_info.ae) / 10.0);
    else lua_rawget(L, 1);
    return 1;
    
}

/// Pretty prints the exposure compensation as a string
//@treturn string
//@function tostring
static int luaCB_ec_tostring(lua_State * L)
{
    lua_pushfstring(L, "%d.%d", RAW2EC(lens_info.ae) / 10, RAW2EC(lens_info.ae) % 10);
    return 1;
}

static int luaCB_ec_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_ae(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_ae(EC2RAW(value));
    }
    else if(!strcmp(key, "apexf") || !strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_ae(EC2RAW((int)roundf(value * 10)));
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static int luaCB_fec_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.flash_ae);
    else if(!strcmp(key,"apex")) lua_pushinteger(L, RAW2EC(lens_info.flash_ae));
    else if(!strcmp(key,"apexf")) lua_pushnumber(L, (RAW2EC(lens_info.flash_ae) / 10.0));
    else if(!strcmp(key,"value")) lua_pushnumber(L, RAW2EC(lens_info.flash_ae) / 10.0);
    else lua_rawget(L, 1);
    return 1;
    
}

static int luaCB_fec_tostring(lua_State * L)
{
    lua_pushfstring(L, "%d.%d", RAW2EC(lens_info.flash_ae) / 10, RAW2EC(lens_info.flash_ae) % 10);
    return 1;
}

static int luaCB_fec_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_flash_ae(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_flash_ae(EC2RAW(value));
    }
    else if(!strcmp(key, "apexf") || !strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        lens_set_flash_ae(EC2RAW((int)roundf(value * 10)));
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static const luaL_Reg cameralib[] =
{
    { "shoot", luaCB_camera_shoot },
    { "bulb", luaCB_camera_bulb },
    { "reboot", luaCB_camera_reboot },
    { NULL, NULL }
};

LUA_LIB(camera)
