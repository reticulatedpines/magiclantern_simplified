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
    /// Get the raw relative focus motor position, in steps.
    /// This counter is 0 at camera startup, its range depend on the lens,
    /// and is updated only when the focus motor moves. It will lose track
    /// of the lens position during manual focus, unless you use a focus-by-wire lens.
    /// Details: [www.magiclantern.fm/forum/index.php?topic=4997](http://www.magiclantern.fm/forum/index.php?topic=4997).
    // @tfield int focus_pos readonly
    else if(!strcmp(key, "focus_pos")) lua_pushinteger(L, lens_info.focus_pos);
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
    /// Get whether the lens is currently autofocusing.
    ///
    /// This does not include manual lens movements from lens.focus or ML follow focus - 
    /// only movements from Canon autofocus triggered by half-shutter / AF-ON / * button.
    /// It is updated several ms (sometimes hundreds of ms) after the half-shutter event.
    ///
    /// On cameras with continuous autofocus, the return value is unknown - please report.
    ///
    /// Known not to work on EOS M.
    // @tfield bool autofocusing readonly
    else if(!strcmp(key, "autofocusing")) lua_pushboolean(L, lv_focus_status == 3);
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
    if (is_continuous_af()) return luaL_error(L, "lens.focus() requires %s AF disabled.", is_movie_mode() ? "movie servo" : "continuous");

    lua_pushboolean(L, lens_focus(num_steps, step_size, wait, delay));

    return 1;
}

static int wait_focus_status(int timeout, int value1, int value2)
{
    int t0 = get_ms_clock();

    while (get_ms_clock() - t0 < timeout)
    {
        msleep(10);

        if (lv_focus_status == value1 || lv_focus_status == value2)
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

    /* these models won't AF with half-shutter in LiveView */
    int back_btn_af_lv = lv && (
        is_camera("5D2", "*") ||
        is_camera("50D", "*") ||
        is_camera("500D", "*")
    );

    if (back_btn_af_lv)
    {
        /* FIXME: this method fails on 60D, why? */
        int af_request = 1;
        prop_request_change(PROP_REMOTE_AFSTART_BUTTON, &af_request, 4);
    }
    else
    {
        lens_setup_af(AF_ENABLE);
        module_send_keypress(MODULE_KEY_PRESS_HALFSHUTTER);
    }

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

    /* 3 = focusing, 1 = idle (most models), 2 = idle (100D) */
    if (wait_focus_status(1000, 3, 3))
    {
        if (wait_focus_status(5000, 1, 2))
        {
            goto success;
        }
        else
        {
            /* timeout */
            printf("[%s] focus status: %d (expected 1 or 2)\n", lua_get_script_filename(L), lv_focus_status);
            goto error;
        }
    }
    printf("[%s] focus status: %d (expected 3)\n", lua_get_script_filename(L), lv_focus_status);

error:
    lua_pushboolean(L, false);
    goto cleanup;

success:
    lua_pushboolean(L, true);
    goto cleanup;

cleanup:
    if (focus_command_sent)
    {
        if (back_btn_af_lv)
        {
            int af_request = 0;
            prop_request_change(PROP_REMOTE_AFSTART_BUTTON, &af_request, 4);
        }
        else
        {
            module_send_keypress(MODULE_KEY_UNPRESS_HALFSHUTTER);
            lens_cleanup_af();
        }
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
