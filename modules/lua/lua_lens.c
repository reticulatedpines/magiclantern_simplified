/***
 Lens functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module lens
 */

#include <dryos.h>
#include <string.h>
#include <lens.h>
#include <focus.h>
#include <module.h>
#include "lua_common.h"

static int luaCB_lens_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get the name of the lens (reported by the lens).
    // @tfield string name readonly
    if(!strcmp(key, "name")) lua_pushstring(L, lens_info.name);
    /// Get the focal length of the lens (in mm). Only updated in LiveView.
    // @tfield int focal_length readonly
    else if(!strcmp(key, "focal_length")) lua_pushinteger(L, lens_info.focal_len);
    /// Get the current focus distance (in mm). Only updated in LiveView.
    // @tfield int focus_distance readonly
    else if(!strcmp(key, "focus_distance")) lua_pushinteger(L, lens_info.focus_dist * 10);
    /// Get the hyperfocal distance of the lens (in mm). Only updated in LiveView.
    ///
    /// Computed from focal length, focus distance and aperture, see Focus -> DOF Settings menu for options.
    // @tfield int hyperfocal readonly
    else if(!strcmp(key, "hyperfocal")) lua_pushinteger(L, lens_info.hyperfocal);
    /// Get the distance to the DOF near (in mm). Only updated in LiveView.
    ///
    /// Computed from focal length, focus distance and aperture, see Focus -> DOF Settings menu for options.
    // @tfield int dof_near readonly
    else if(!strcmp(key, "dof_near")) lua_pushinteger(L, lens_info.dof_near);
    /// Get the distance to the DOF far (in mm). Only updated in LiveView.
    ///
    /// Computed from focal length, focus distance and aperture, see Focus -> DOF Settings menu for options.
    // @tfield int dof_far readonly
    else if(!strcmp(key, "dof_far")) lua_pushinteger(L, lens_info.dof_far);
    /// Get whether or not auto focus is enabled.
    // @tfield bool af readonly
    else if(!strcmp(key, "af")) lua_pushboolean(L, !is_manual_focus());
    /// Get the current auto focus mode (may be model-specific, see PROP\_AF\_MODE in property.h).
    // @tfield int af_mode readonly
    else if(!strcmp(key, "af_mode")) lua_pushinteger(L, af_mode);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_lens_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "name") || !strcmp(key, "focal_length") || !strcmp(key, "focus_distance") || !strcmp(key, "hyperfocal") || !strcmp(key, "dof_near") || !strcmp(key, "dof_far") || !strcmp(key, "af"))
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
 Moves the focus motor a specified number of steps.
 
 Only works in LiveView.
 @tparam int num_steps How many steps to move the focus motor (signed).
 @tparam[opt=2] int step_size Allowed values: 1, 2 or 3.
 
 Step 1 may give finer movements, but may get stuck or may be very slow on some lenses.
 @tparam[opt=true] bool wait Wait until each focus command finishes, before queueing others.
 
 wait=false may give smoother movements, but may no longer return accurate status for each command,
 and is known to **crash** on some cameras. The exact behavior may be camera- or lens-dependent.
 
 __Do not disable it without a good reason!__
 
 @tparam[opt] int delay Delay between focus commands (ms)
 
 With wait=true, the delay is after each focus command is executed (as reported by Canon firmware).
 
 With wait=false, the delay is after each focus command is started (without waiting for it to finish).
 
 (_default_ 0 if wait=true, 30ms if wait=false)
 
 
 @treturn bool whether the operation was successful or not.
 @function focus
 */
static int luaCB_lens_focus(lua_State * L)
{
    LUA_PARAM_INT(num_steps, 1);
    LUA_PARAM_INT_OPTIONAL(step_size, 2, 2);
    LUA_PARAM_BOOL_OPTIONAL(wait, 3, true);
    LUA_PARAM_INT_OPTIONAL(delay, 4, wait ? 0 : 30);

    if (!lv) return luaL_error(L, "lens.focus() only works in LiveView.");
    if (is_manual_focus()) return luaL_error(L, "lens.focus() requires autofocus enabled.");

    lua_pushboolean(L, lens_focus(num_steps, step_size, wait, delay));

    return 1;
}

static int wait_focus_status(int timeout, int value)
{
    int t0 = get_ms_clock_value();

    while (get_ms_clock_value() - t0 < timeout)
    {
        msleep(10);

        if (lv_focus_status == value)
        {
            return 1;
        }
    }
    return 0;
}

/***
 Performs autofocus, similar to half-shutter press.
 
 Works in both LiveView and in plain photo mode.
 @treturn bool whether the operation was successful or not.
 @function autofocus
 */
static int luaCB_lens_autofocus(lua_State * L)
{
    int focus_command_sent = 0;

    if (is_manual_focus())
    {
        goto error;
    }

    lens_setup_af(AF_ENABLE);
    module_send_keypress(MODULE_KEY_PRESS_HALFSHUTTER);
    focus_command_sent = 1;

    if (!lv)
    {
        for (int i = 0; i < 20; i++)
        {
            msleep(100);
        
            /* FIXME: this may fail on recent models where trap focus is not working */
            if (get_focus_confirmation())
            {
                goto success;
            }
        }

        goto error;
    }

    /* 3 = focusing, 1 = idle */
    if (wait_focus_status(1000, 3))
    {
        if (wait_focus_status(5000, 1))
        {
            goto success;
        }
        else
        {
            /* timeout */
            printf("Focus status: %d\n", lv_focus_status);
            goto error;
        }
    }

error:
    lua_pushboolean(L, false);
    goto cleanup;

success:
    lua_pushboolean(L, true);
    goto cleanup;

cleanup:
    if (focus_command_sent)
    {
        module_send_keypress(MODULE_KEY_UNPRESS_HALFSHUTTER);
        lens_cleanup_af();
    }
    return 1;
}

static const char * lua_lens_fields[] =
{
    "name",
    "focal_length",
    "focus_distance",
    "hyperfocal",
    "dof_near",
    "dof_far",
    "af",
    "af_mode",
    NULL
};

static const luaL_Reg lenslib[] =
{
    { "focus",      luaCB_lens_focus },
    { "autofocus",  luaCB_lens_autofocus },
    { NULL, NULL }
};

LUA_LIB(lens)
