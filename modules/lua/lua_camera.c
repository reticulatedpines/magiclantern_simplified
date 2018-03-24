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
#include <math.h>
#include <zebra.h>

#include "lua_common.h"

static int luaCB_shutter_index(lua_State * L);
static int luaCB_shutter_newindex(lua_State * L);
static int luaCB_shutter_tostring(lua_State * L);

static int luaCB_aperture_index(lua_State * L);
static int luaCB_aperture_newindex(lua_State * L);
static int luaCB_aperture_tostring(lua_State * L);

static int luaCB_min_aperture_index(lua_State * L);
static int luaCB_min_aperture_tostring(lua_State * L);
static int luaCB_max_aperture_index(lua_State * L);
static int luaCB_max_aperture_tostring(lua_State * L);

static int luaCB_iso_index(lua_State * L);
static int luaCB_iso_newindex(lua_State * L);
static int luaCB_iso_tostring(lua_State * L);

static int luaCB_ec_index(lua_State * L);
static int luaCB_ec_newindex(lua_State * L);
static int luaCB_ec_tostring(lua_State * L);

static int luaCB_fec_index(lua_State * L);
static int luaCB_fec_newindex(lua_State * L);
static int luaCB_fec_tostring(lua_State * L);

static int luaCB_gui_index(lua_State * L);
static int luaCB_gui_newindex(lua_State * L);

static const char * lua_camera_shutter_fields[] =
{
    "raw",
    "apex",
    "ms",
    "value",
    NULL
};

static const char * lua_camera_aperture_fields[] =
{
    "raw",
    "apex",
    "value",
    "min",
    "max",
    NULL
};

static const char * lua_camera_aperture_minmax_fields[] =
{
    "raw",
    "apex",
    "value",
    NULL
};

static const char * lua_camera_iso_fields[] =
{
    "raw",
    "apex",
    "value",
    NULL
};

static const char * lua_camera_ec_fields[] =
{
    "raw",
    "value",
    NULL
};

static const char * lua_camera_gui_fields[] =
{
    "menu",
    "play",
    "play_photo",
    "play_movie",
    "qr",
    "idle",
    NULL
};

static int luaCB_camera_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Gets a @{shutter} object that represents the camera's current shutter speed.
    // @tfield shutter shutter
    if(!strcmp(key, "shutter"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_shutter_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_shutter_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_shutter_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_shutter_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    /// Gets an @{aperture} object that represents the lens' aperture.
    // @tfield aperture aperture
    else if(!strcmp(key, "aperture"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_aperture_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_aperture_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_aperture_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_aperture_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    /// Gets an @{iso} object that represents camera's ISO.
    // @tfield iso iso
    else if(!strcmp(key, "iso"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_iso_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_iso_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_iso_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_iso_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    /// Gets an @{ec} object that represents exposure compensation.
    // @tfield ec ec
    else if(!strcmp(key, "ec"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_ec_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_ec_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_ec_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_ec_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    /// Get/set whether the flash is enabled.
    ///
    /// Possible values: true, false, "auto", "unknown".
    ///
    /// Flash can only be changed from script in simple (non-automatic) modes.
    ///
    /// Not tested with external flashes; it may work or it may not.
    // @tfield ?bool|string flash
    else if(!strcmp(key, "flash"))
    {
        if (strobo_firing == 0) {
            lua_pushboolean(L, 1); /* reversed */
        } else if (strobo_firing == 1) {
            lua_pushboolean(L, 0);
        } else if (strobo_firing == 2) {
            lua_pushstring(L, "auto");
        } else {
            lua_pushstring(L, "unknown");
        }
    }
    /// Gets an @{ec} object that represents flash exposure compensation.
    // @tfield ec flash_ec
    else if(!strcmp(key, "flash_ec"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_fec_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_fec_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_fec_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_ec_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    /// Get/Set kelvin white balance.
    // @tfield int kelvin
    else if(!strcmp(key, "kelvin")) lua_pushinteger(L, lens_info.kelvin);
    /// Get the current camera mode, possible values defined in @{constants.MODE}.
    ///
    /// Note: for cameras without a dedicated video mode, it will return MODE.MOVIE
    /// whenever your camera is configured to record videos (usually from Canon menu).
    // @tfield int mode
    else if(!strcmp(key, "mode"))
    {
        lua_pushinteger(L, is_movie_mode() ? SHOOTMODE_MOVIE : shooting_mode);
    }
    /// Get the current metering mode (see PROP\_METERING\_MODE in property.h).
    ///
    /// TODO: add constants and setter.
    // @tfield int metering_mode readonly
    else if(!strcmp(key, "metering_mode")) lua_pushinteger(L, metering_mode);
    /// Get the current drive mode (see PROP\_DRIVE in property.h).
    ///
    /// TODO: add constants and setter.
    // @tfield int drive_mode readonly
    else if(!strcmp(key, "drive_mode")) lua_pushinteger(L, drive_mode);
    /// Get the model name of the camera (e.g.&nbsp;"Canon EOS 60D").
    // @tfield string model readonly
    else if(!strcmp(key, "model")) lua_pushstring(L, camera_model);
    /// Get the shortened model name of the camera (e.g.&nbsp;"5D3").
    // @tfield string model_short readonly
    else if(!strcmp(key, "model_short")) lua_pushstring(L, __camera_model_short);
    /// Get the Canon firmware version string (e.g.&nbsp;"1.2.3").
    // @tfield string firmware readonly
    else if(!strcmp(key, "firmware")) lua_pushstring(L, firmware_version);
    /// Get the temperature from the EFIC chip in raw units.
    ///
    /// TODO: Celsius.
    // @tfield int temperature readonly
    else if(!strcmp(key, "temperature")) lua_pushinteger(L, efic_temp);
    /// Gets a @{gui} object that controls Canon GUI state (PLAY, MENU, QR, various dialogs)
    // @tfield gui gui
    else if(!strcmp(key, "gui"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_gui_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_gui_newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_gui_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_camera_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    int status = 1;
    
    if(!strcmp(key, "shutter"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawshutter(shutterf_to_raw(value));
    }
    else if(!strcmp(key, "aperture"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawaperture((int)roundf((log2f(value) * 16) + 8));
    }
    else if(!strcmp(key, "iso"))
    {
        LUA_PARAM_INT(value, 3);
        int raw = value ? (int)roundf(log2f(value/3.125) * 8) + 32 : 0;
        status = hdr_set_rawiso(raw);
    }
    else if(!strcmp(key, "ec"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = lens_set_ae(APEX1000_EC2RAW((int)roundf(value * 1000)));
    }
    else if(!strcmp(key, "flash"))
    {
        if (strobo_firing == 0 || strobo_firing == 1)
        {
            LUA_PARAM_BOOL(value, 3);
            set_flash_firing(value ? 0 : 1);
        }
        else
        {
            return luaL_error(L, "set 'camera.flash' failed");
        }
    }
    else if(!strcmp(key, "flash_ec"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = lens_set_flash_ae(APEX1000_EC2RAW((int)roundf(value * 1000)));
    }
    else if(!strcmp(key, "kelvin"))
    {
        LUA_PARAM_INT(value, 3);
        lens_set_kelvin(value);
    }
    else if(!strcmp(key, "model") || !strcmp(key, "firmware") || !strcmp(key, "mode") || !strcmp(key, "metering_mode") || !strcmp(key, "drive_mode") || !strcmp(key, "temperature") || !strcmp(key, "state"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else
    {
        lua_rawset(L, 1);
    }
    if(!status) return luaL_error(L, "set '%s' failed", key);
    return 0;
}

/***
 Take a picture.
 @tparam[opt=false] bool should_af whether or not to use autofocus.
 @function shoot
 */
static int luaCB_camera_shoot(lua_State * L)
{
    LUA_PARAM_BOOL_OPTIONAL(should_af, 1, 0);
    int result = take_a_pic(should_af);
    lua_pushinteger(L, result);
    return 1;
}

/***
 Wait until the camera is idle (not processing pictures).
 
 Normally, camera.shoot() returns as soon as the next picture can be taken,
 while image compression and saving happen in background.
 @function wait
 */
static int luaCB_camera_wait(lua_State * L)
{
    while (lens_info.job_state) {
        msleep(20);
    }
    return 0;
}

/***
 Take N pictures in burst mode.
 
 Note: your camera must be already in some continuous drive mode,
 otherwise the speed will be slow.
 
 @tparam int num_pictures how many pictures to take.
 @function burst
 */
static int luaCB_camera_burst(lua_State * L)
{
    LUA_PARAM_INT(num_pictures, 1);
    take_fast_pictures(num_pictures);
    return 0;
}

/***
 Take a picture in bulb mode.
 @tparam float duration bulb duration in seconds.
 @function bulb
 */
static int luaCB_camera_bulb(lua_State * L)
{
    LUA_PARAM_NUMBER(duration, 1);
    bulb_take_pic(duration * 1000);
    return 0;
}

/***
 Restart the camera.
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
    /// Get/Set shutter speed in Canon raw units.
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_shutter);
    /// Get/Set shutter speed in APEX units (floating point).
    // @tfield number apex
    else if(!strcmp(key,"apex")) lua_pushnumber(L, (APEX1000_RAW2TV(lens_info.raw_shutter)/1000.0));
    /// Get/Set shutter speed in milliseconds.
    // @tfield int ms
    else if(!strcmp(key,"ms")) lua_pushinteger(L, raw2shutter_ms(lens_info.raw_shutter));
    /// Get/Set shutter speed in seconds (floating point).
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, raw2shutterf(lens_info.raw_shutter));
    else lua_rawget(L, 1);
    return 1;
}

/// Pretty prints the current shutter speed as a string.
//@treturn string
//@function __tostring
static int luaCB_shutter_tostring(lua_State * L)
{
    int old = cli();
    lua_pushstring(L, lens_format_shutter(lens_info.raw_shutter));
    sei(old);
    return 1;
}

static int luaCB_shutter_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    /* this breaks copy2m */
    //~ if(!lens_info.raw_shutter) return luaL_error(L, "Shutter speed is automatic - cannot adjust manually.");
    
    int status = 1;
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        status = hdr_set_rawshutter(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawshutter(APEX1000_TV2RAW((int)roundf(value * 1000)));
    }
    else if(!strcmp(key, "ms"))
    {
        LUA_PARAM_INT(value, 3);
        status = hdr_set_rawshutter(shutter_ms_to_raw(value));
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawshutter(shutterf_to_raw(value));
    }
    else
    {
        lua_rawset(L, 1);
    }
    if(!status) return luaL_error(L, "set 'camera.shutter.%s' failed", key);
    return 0;
}

/// Represents the lens's aperture.
//@type aperture

static int luaCB_aperture_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set aperture in Canon raw units.
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_aperture);
    /// Get/Set aperture in APEX units (floating point).
    // @tfield number apex
    else if(!strcmp(key,"apex")) lua_pushnumber(L, (APEX1000_RAW2AV(lens_info.raw_aperture) / 1000.0));
    /// Get/Set aperture as f-number (floating point).
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, lens_info.aperture / 10.0);
    /// Get minimum (wide open) aperture value (aperture object).
    // @tfield aperture min readonly
    else if(!strcmp(key,"min"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_min_aperture_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_min_aperture_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_aperture_minmax_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    /// Get maximum (closed) aperture value (aperture object).
    // @tfield aperture max readonly
    else if(!strcmp(key,"max"))
    {
        lua_newtable(L);
        lua_newtable(L);
        lua_pushcfunction(L, luaCB_max_aperture_tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luaCB_max_aperture_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luaCB_pairs);
        lua_setfield(L, -2, "__pairs");
        lua_pushlightuserdata(L, lua_camera_aperture_minmax_fields);
        lua_setfield(L, -2, "fields");
        lua_setmetatable(L, -2);
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_min_aperture_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    // Get minimum (wide open) aperture in Canon raw units
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_aperture_min);
    // Get minimum (wide open) aperture in APEX units (floating point)
    // @tfield number apex
    else if(!strcmp(key,"apex")) lua_pushnumber(L, (APEX1000_RAW2AV(lens_info.raw_aperture_min) / 1000.0));
    // Get minimum (wide open) aperture as f-number (floating point)
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, RAW2VALUE(aperture, lens_info.raw_aperture_min) / 10.0);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_max_aperture_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    // Get maximum (closed) aperture in Canon raw units
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_aperture_max);
    // Get maximum (closed) aperture in APEX units (floating point)
    // @tfield number apex
    else if(!strcmp(key,"apex")) lua_pushnumber(L, (APEX1000_RAW2AV(lens_info.raw_aperture_max) / 1000.0));
    // Get maximum (closed) aperture as f-number (floating point)
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, RAW2VALUE(aperture, lens_info.raw_aperture_max) / 10.0);
    else lua_rawget(L, 1);
    return 1;
}

/// Pretty prints the current aperture as a string.
//@treturn string
//@function __tostring
static int luaCB_aperture_tostring(lua_State * L)
{
    int old = cli();
    lua_pushstring(L, lens_format_aperture(lens_info.raw_aperture));
    sei(old);
    return 1;
}

static int luaCB_min_aperture_tostring(lua_State * L)
{
    int old = cli();
    lua_pushstring(L, lens_format_aperture(lens_info.raw_aperture_min));
    sei(old);
    return 1;
}

static int luaCB_max_aperture_tostring(lua_State * L)
{
    int old = cli();
    lua_pushstring(L, lens_format_aperture(lens_info.raw_aperture_max));
    sei(old);
    return 1;
}

static int luaCB_aperture_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    if (!lens_info.aperture)
    {
        /* is it needed? */
        //~ return luaL_error(L, lens_info.name[0] ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture.");
    }
    int status = 1;
    
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        status = hdr_set_rawaperture(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawaperture(APEX1000_AV2RAW((int)roundf(value * 1000)));
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawaperture((int)roundf((log2f(value) * 16) + 8));
    }
    else
    {
        lua_rawset(L, 1);
    }
    if(!status) return luaL_error(L, "set 'camera.aperture.%s' failed", key);
    return 0;
}

/// Represents the camera ISO.
//@type iso

static int luaCB_iso_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set ISO in Canon raw units.
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.raw_iso);
    /// Get/Set ISO in APEX units (floating point).
    // @tfield number apex
    else if(!strcmp(key,"apex")) lua_pushnumber(L, (APEX1000_RAW2SV(lens_info.raw_iso) / 1000.0));
    /// Get/Set ISO (e.g.&nbsp;100, 200).
    // @tfield int value
    else if(!strcmp(key,"value")) lua_pushinteger(L, raw2iso(lens_info.raw_iso));
    else lua_rawget(L, 1);
    return 1;
}

/// Pretty prints the current ISO as a string.
//@treturn string
//@function __tostring
static int luaCB_iso_tostring(lua_State * L)
{
    int old = cli();
    lua_pushstring(L, lens_format_iso(lens_info.raw_iso));
    sei(old);
    return 1;
}

static int luaCB_iso_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    int status = 1;
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        status = hdr_set_rawiso(value);
    }
    else if(!strcmp(key, "apex"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = hdr_set_rawiso(APEX1000_SV2RAW((int)roundf(value * 1000)));
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_INT(value, 3);
        int raw = value ? (int)roundf(log2f(value/3.125) * 8) + 32 : 0;
        status = hdr_set_rawiso(raw);
    }
    else
    {
        lua_rawset(L, 1);
    }
    if(!status) return luaL_error(L, "set 'camera.iso.%s' failed", key);
    return 0;
}

/// Represents the exposure compensation.
//@type ec

static int luaCB_ec_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set exposure compensation in Canon raw units.
    // @tfield int raw
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.ae);
    /// Get/Set exposure compensation in EV or APEX (both are identical here).
    // @tfield number value
    else if(!strcmp(key,"value")) lua_pushnumber(L, APEX1000_RAW2EC(lens_info.ae) / 1000.0);
    else lua_rawget(L, 1);
    return 1;
    
}

/// Pretty prints the exposure compensation as a string.
//@treturn string
//@function __tostring
static int luaCB_ec_tostring(lua_State * L)
{
    int ec = APEX10_RAW2EC(lens_info.ae);
    lua_pushfstring(L, "%s%d.%d", FMT_FIXEDPOINT1(ec));
    return 1;
}

static int luaCB_ec_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    int status = 1;
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        status = lens_set_ae(value);
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = lens_set_ae(APEX1000_EC2RAW((int)roundf(value * 1000)));
    }
    else
    {
        lua_rawset(L, 1);
    }
    if(!status) return luaL_error(L, "set 'camera.ec.%s' failed", key);
    return 0;
}

static int luaCB_fec_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key,"raw")) lua_pushinteger(L, lens_info.flash_ae);
    else if(!strcmp(key,"value")) lua_pushnumber(L, APEX1000_RAW2EC(lens_info.flash_ae) / 1000.0);
    else lua_rawget(L, 1);
    return 1;
    
}

static int luaCB_fec_tostring(lua_State * L)
{
    int fec = APEX10_RAW2EC(lens_info.flash_ae);
    lua_pushfstring(L, "%s%d.%d", FMT_FIXEDPOINT1(fec));
    return 1;
}

static int luaCB_fec_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    int status = 1;
    if(!strcmp(key, "raw"))
    {
        LUA_PARAM_INT(value, 3);
        status = lens_set_flash_ae(value);
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_NUMBER(value, 3);
        status = lens_set_flash_ae(APEX1000_EC2RAW((int)roundf(value * 1000)));
    }
    else
    {
        lua_rawset(L, 1);
    }
    if(!status) return luaL_error(L, "set 'camera.fec.%s' failed", key);
    return 0;
}

/// Canon GUI controls (PLAY, MENU, QR and so on) 
//@type gui

static int luaCB_gui_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get/Set whether Canon menu is active or not.
    ///
    /// Can also be used to enter Canon menu: camera.gui.menu = true;
    // @tfield bool menu
    if(!strcmp(key,"menu")) lua_pushboolean(L, is_menu_mode());
    /// Get/Set whether the camera is in PLAY mode (image or video review).
    ///
    /// Can also be used to enter PLAY mode: camera.gui.menu = true;
    // @tfield bool play
    else if(!strcmp(key,"play")) lua_pushboolean(L, is_play_mode());
    /// Get whether the camera is in PLAY mode with a still photo selected.
    // @tfield bool play_photo
    else if(!strcmp(key,"play_photo")) lua_pushboolean(L, is_pure_play_photo_mode());
    /// Get whether the camera is in PLAY mode with a H.264 movie selected.
    // @tfield bool play_movie
    else if(!strcmp(key,"play_movie")) lua_pushboolean(L, is_pure_play_movie_mode());
    /// Get whether the camera is in QR (QuickReview) mode.
    ///
    /// QuickReview is the image review mode used right after taking a picture.
    ///
    /// It is not the same as the regular PLAY mode, even though they are similar at first sight (both are used for reviewing an image).
    /// Important difference: in QR mode, ML has access to raw image data from the last captured picture; this data is not available in PLAY mode.
    ///
    // @tfield bool qr
    else if(!strcmp(key,"qr")) lua_pushboolean(L, !is_play_mode() && is_play_or_qr_mode());
    /// Get whether the camera is "idle" (in standby, with no other dialogs active)
    // @tfield bool idle
    else if(!strcmp(key,"idle")) lua_pushboolean(L, display_idle());
    /// Get/Set current GUI mode from Canon (model-dependent).
    /// 
    /// On most models, 0 is "idle" (not exactly identical to what ML considers idle), 1 is PLAY, 2 is MENU.
    // @tfield int mode
    else if(!strcmp(key,"mode")) lua_pushinteger(L, get_gui_mode());
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_gui_tostring(lua_State * L)
{
    char status[50] = "";
    if (is_menu_mode())                 { STR_APPEND(status, "MENU"); }
    else if (is_play_mode())            { STR_APPEND(status, "PLAY"); }
    else if (is_pure_play_photo_mode()) { STR_APPEND(status, "PLAY,PH"); }
    else if (is_pure_play_movie_mode()) { STR_APPEND(status, "PLAY,MV"); }
    else if (is_play_mode())            { STR_APPEND(status, "PLAY"); }
    else
    {
        if (display_idle())             { STR_APPEND(status, "IDLE,"); }
        else                            { STR_APPEND(status, "BUSY,"); }

        if (is_movie_mode())            { STR_APPEND(status, "MV"); }
        else if (lv)                    { STR_APPEND(status, "LV"); }
        else                            { STR_APPEND(status, "PH"); }
    }
    lua_pushfstring(L, status);
    return 0;
}

static int luaCB_gui_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    int status = 1;
    if(!strcmp(key, "mode"))
    {
        LUA_PARAM_INT(value, 3);
        SetGUIRequestMode(value);
        msleep(500);
        status = (get_gui_mode() == value);
    }
    else if(!strcmp(key, "play"))
    {
        LUA_PARAM_BOOL(value, 3);
        if (value) enter_play_mode();
        else exit_play_qr_mode();
        status = (is_play_mode() == value);
    }
    else if(!strcmp(key, "menu"))
    {
        LUA_PARAM_BOOL(value, 3);
        if (value) enter_menu_mode();
        else exit_menu_mode();
        status = (is_menu_mode() == value);
    }
    if(!status) return luaL_error(L, "set 'camera.gui.%s' failed", key);
    return 0;
}

static const char * lua_camera_fields[] =
{
    "shutter",
    "aperture",
    "iso",
    "ec",
    "flash",
    "flash_ec",
    "kelvin",
    "mode",
    "metering_mode",
    "drive_mode",
    "model",
    "model_short",
    "firmware",
    "temperature",
    "gui",
    NULL
};

static const luaL_Reg cameralib[] =
{
    { "shoot", luaCB_camera_shoot },
    { "burst", luaCB_camera_burst },
    { "bulb", luaCB_camera_bulb },
    { "wait", luaCB_camera_wait },
    { "reboot", luaCB_camera_reboot },
    { NULL, NULL }
};

LUA_LIB(camera)
