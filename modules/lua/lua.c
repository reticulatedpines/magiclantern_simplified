/*
 * Copyright (C) 2014 David Milligan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <module.h>
#include <dryos.h>
#include <string.h>
#include <math.h>
#include <raw.h>
#include <lens.h>
#include <property.h>
#include <console.h>
#include <fps.h>
#include <beep.h>
#include <fio-ml.h>
#include <bmp.h>
#include <shoot.h>
#include <zebra.h>
#include <focus.h>
#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

#define MAX_PATH_LEN 0x80
#define SCRIPTS_DIR "ML/SCRIPTS"
#define EC2RAW(ec) ec * 8 / 10

struct script_entry
{
    int menu_value;
    struct script_entry * next;
    lua_State * L;
    struct menu_entry * menu_entry;
    int submenu_index;
};
static struct script_entry * scripts = NULL;
static struct script_entry * running_script = NULL;
static int lua_running = 0;
static int lua_loaded = 0;
static int lua_run_arg_count = 0;
static int script_entry_count = 0;

#define LUA_PARAM_INT(name, index)\
if(index > lua_gettop(L) || !lua_isinteger(L, index))\
{\
    lua_pushliteral(L, "Invalid or missing parameter: " #name);\
    lua_error(L);\
}\
int name = lua_tointeger(L, index)

#define LUA_PARAM_INT_OPTIONAL(name, index, default) int name = (index <= lua_gettop(L) && lua_isinteger(L, index)) ? lua_tointeger(L, index) : default

#define LUA_PARAM_NUMBER(name, index)\
if(index > lua_gettop(L) || !lua_isnumber(L, index))\
{\
    lua_pushliteral(L, "Invalid or missing parameter: " #name);\
    lua_error(L);\
}\
float name = lua_tonumber(L, index)

#define LUA_PARAM_NUMBER_OPTIONAL(name, index, default) float name = (index <= lua_gettop(L) && lua_isnumber(L, index)) ? lua_tonumber(L, index) : default

#define LUA_PARAM_STRING(name, index)\
if(index > lua_gettop(L) || !lua_isstring(L, index))\
{\
   lua_pushliteral(L, "Invalid or missing parameter: " #name);\
   lua_error(L);\
}\
const char * name = lua_tostring(L, index)

#define LUA_PARAM_STRING_OPTIONAL(name, index, default) const char * name = (index <= lua_gettop(L) && lua_isstring(L, index)) ? lua_tostring(L, index) : default

#define LUA_FIELD_STRING(field, default) lua_getfield(L, -1, field) == LUA_TSTRING ? lua_tostring(L, -1) : default; lua_pop(L, 1)
#define LUA_FIELD_INT(field, default) lua_getfield(L, -1, field) == LUA_TNUMBER ? lua_tointeger(L, -1) : default; lua_pop(L, 1)
/**
 * Determines if a string ends in some string
 */
static int string_ends_with(const char *source, const char *ending)
{
    if(source == NULL || ending == NULL) return 0;
    if(strlen(source) <= 0) return 0;
    if(strlen(source) < strlen(ending)) return 0;
    return !strcmp(source + strlen(source) - strlen(ending), ending);
}

static int luaCB_console_show(lua_State * L)
{
    console_show();
    return 0;
}

static int luaCB_console_hide(lua_State * L)
{
    console_hide();
    return 0;
}

static int luaCB_console_write(lua_State * L)
{
    if(lua_isstring(L, 1)) console_puts(lua_tostring(L, 1));
    return 0;
}

static int luaCB_console_index(lua_State * L) { lua_rawget(L, 1); return 1; }
static int luaCB_console_newindex(lua_State * L) { lua_rawset(L, 1); return 0; }

static const luaL_Reg consolelib[] =
{
    { "show", luaCB_console_show },
    { "hide", luaCB_console_hide },
    { "write", luaCB_console_write },
    { NULL, NULL }
};

static int luaCB_beep(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(times, 1, 1);
    beep_times(times);
    return 0;
}

static int luaCB_call(lua_State * L)
{
    LUA_PARAM_STRING(function_name, 1);
    int result = 0;
    int argc = lua_gettop(L);
    
    if(argc <= 1)
    {
        result = call(function_name);
    }
    else if(lua_isinteger(L, 2))
    {
        int arg = lua_tointeger(L, 2);
        result = call(function_name, arg);
    }
    else if(lua_isnumber(L, 2))
    {
        float arg = lua_tonumber(L, 2);
        result = call(function_name, arg);
    }
    else if(lua_isstring(L, 2))
    {
        const char * arg = lua_tostring(L, 2);
        result = call(function_name, arg);
    }
    
    lua_pushinteger(L, result);
    return 1;
}

static int luaCB_shoot(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(wait, 1, 64);
    LUA_PARAM_INT_OPTIONAL(should_af, 2, 1);
    int result = lens_take_picture(wait, should_af);
    lua_pushinteger(L, result);
    return 1;
}

static int luaCB_msleep(lua_State * L)
{
    LUA_PARAM_INT(amount, 1);
    msleep(amount);
    return 0;
}

static const luaL_Reg globallib[] =
{
    { "msleep", luaCB_msleep },
    { "beep", luaCB_beep },
    { "call", luaCB_call },
    { "shoot", luaCB_shoot },
    { NULL, NULL }
};

static int luaCB_camera_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "shutter")) lua_pushinteger(L, RAW2TV(lens_info.raw_shutter));
    else if(!strcmp(key, "aperture")) lua_pushinteger(L, RAW2AV(lens_info.raw_aperture));
    else if(!strcmp(key, "iso")) lua_pushinteger(L, RAW2SV(lens_info.raw_iso));
    else if(!strcmp(key, "ec")) lua_pushinteger(L, RAW2EC(lens_info.ae));
    else if(!strcmp(key, "flash_ec")) lua_pushinteger(L, RAW2EC(lens_info.flash_ae));
    else if(!strcmp(key, "mode")) lua_pushinteger(L, shooting_mode);
    else if(!strcmp(key, "af_mode")) lua_pushinteger(L, metering_mode);
    else if(!strcmp(key, "metering_mode")) lua_pushinteger(L, metering_mode);
    else if(!strcmp(key, "drive_mode")) lua_pushinteger(L, drive_mode);
    else if(!strcmp(key, "model")) lua_pushstring(L, camera_model);
    else if(!strcmp(key, "firmware")) lua_pushstring(L, firmware_version);
    else if(!strcmp(key, "temperature")) lua_pushinteger(L, efic_temp);
    else if(!strcmp(key, "state")) lua_pushinteger(L, gui_state);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_camera_newindex(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
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
        lua_pushstring(L, "property is readonly!"); lua_error(L);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static const luaL_Reg cameralib[] =
{
    { "shoot", luaCB_shoot },
    { NULL, NULL }
};

static int luaCB_lv_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "enabled")) lua_pushboolean(L, lv);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_lv_newindex(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "enabled"))
    {
        LUA_PARAM_INT(value, 3);
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

static int luaCB_lens_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
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
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "name") || !strcmp(key, "focal_length") || !strcmp(key, "focal_distance") || !strcmp(key, "hyperfocal") || !strcmp(key, "dof_near") || !strcmp(key, "dof_far") || !strcmp(key, "af"))
    {
        lua_pushstring(L, "property is readonly!"); lua_error(L);
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


static int luaCB_movie_start(lua_State* L)
{
    if (shooting_type != 3 && shooting_mode != SHOOTMODE_MOVIE)
    {
        lua_pushstring(L, "Not in movie mode");
        lua_error(L);
    }
    else if (RECORDING)
    {
        
        lua_pushstring(L, "Already recording");
        lua_error(L);
    }
    else
    {
        movie_start();
    }
    return 0;
}

static int luaCB_movie_stop(lua_State* L)
{
    if (shooting_type != 3 && shooting_mode != SHOOTMODE_MOVIE)
    {
        lua_pushstring(L, "Not in movie mode");
        lua_error(L);
    }
    else if (!RECORDING)
    {
        lua_pushstring(L, "Not recording");
        lua_error(L);
    }
    else
    {
        movie_end();
    }
    return 0;
}

static int luaCB_movie_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "recording")) lua_pushboolean(L, RECORDING);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_movie_newindex(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "recording"))
    {
        LUA_PARAM_INT(value, 3);
        if(value) luaCB_movie_start(L);
        else luaCB_movie_stop(L);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

const luaL_Reg movielib[] =
{
    {"start", luaCB_movie_start},
    {"stop", luaCB_movie_stop},
    {NULL, NULL}
};

#define LOAD_LUA_LIB(name) lua_newtable(L);luaL_setfuncs(L, name##lib, 0);lua_pushvalue(L,-1);lua_setglobal(L, #name);lua_newtable(L);lua_pushcfunction(L, luaCB_##name##_index);lua_setfield(L, -2, "__index");lua_pushcfunction(L, luaCB_##name##_newindex);lua_setfield(L, -2, "__newindex");lua_setmetatable(L, -2);lua_pop(L,1)
#define LUA_CONSTANT(name, value) lua_pushinteger(L, value); lua_setfield(L, -2, #name)

static lua_State * load_lua_state()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    
    LOAD_LUA_LIB(console);
    LOAD_LUA_LIB(camera);
    LOAD_LUA_LIB(lv);
    LOAD_LUA_LIB(lens);
    LOAD_LUA_LIB(movie);
    
    //constants
    lua_newtable(L);
    LUA_CONSTANT(P, SHOOTMODE_P);
    LUA_CONSTANT(TV, SHOOTMODE_TV);
    LUA_CONSTANT(AV, SHOOTMODE_AV);
    LUA_CONSTANT(M, SHOOTMODE_M);
    LUA_CONSTANT(BULB, SHOOTMODE_BULB);
    LUA_CONSTANT(ADEP, SHOOTMODE_ADEP);
    LUA_CONSTANT(C, SHOOTMODE_C);
    LUA_CONSTANT(C2, SHOOTMODE_C2);
    LUA_CONSTANT(C3, SHOOTMODE_C3);
    LUA_CONSTANT(CA, SHOOTMODE_CA);
    LUA_CONSTANT(AUTO, SHOOTMODE_AUTO);
    LUA_CONSTANT(NOFLASH, SHOOTMODE_NOFLASH);
    LUA_CONSTANT(PORTRAIT, SHOOTMODE_PORTRAIT);
    LUA_CONSTANT(LANDSCAPE, SHOOTMODE_LANDSCAPE);
    LUA_CONSTANT(MACRO, SHOOTMODE_MACRO);
    LUA_CONSTANT(SPORTS, SHOOTMODE_SPORTS);
    LUA_CONSTANT(NIGHT, SHOOTMODE_NIGHT);
    LUA_CONSTANT(MOVIE, SHOOTMODE_MOVIE);
    lua_setglobal(L, "MODE");
    
    lua_newtable(L);
    LUA_CONSTANT(AUTO, IT_AUTO);
    LUA_CONSTANT(BOOL, IT_BOOL);
    LUA_CONSTANT(DICE, IT_DICE);
    LUA_CONSTANT(PERCENT, IT_PERCENT);
    LUA_CONSTANT(ALWAYS_ON, IT_ALWAYS_ON);
    LUA_CONSTANT(ACTION, IT_ACTION);
    LUA_CONSTANT(BOOL_NEG, IT_BOOL_NEG);
    LUA_CONSTANT(DISABLE_SOME_FEATURE, IT_DISABLE_SOME_FEATURE);
    LUA_CONSTANT(SUBMENU, IT_SUBMENU);
    LUA_CONSTANT(DICE_OFF, IT_DICE_OFF);
    LUA_CONSTANT(PERCENT_OFF, IT_PERCENT_OFF);
    LUA_CONSTANT(PERCENT_LOG, IT_PERCENT_LOG);
    LUA_CONSTANT(PERCENT_LOG_OFF, IT_PERCENT_LOG_OFF);
    lua_setglobal(L, "ICON_TYPE");
    
    lua_newtable(L);
    LUA_CONSTANT(EV, UNIT_1_8_EV);
    LUA_CONSTANT(x10, UNIT_x10);
    LUA_CONSTANT(PERCENT, UNIT_PERCENT);
    LUA_CONSTANT(PERCENT_x10, UNIT_PERCENT_x10);
    LUA_CONSTANT(ISO, UNIT_ISO);
    LUA_CONSTANT(HEX, UNIT_HEX);
    LUA_CONSTANT(DEC, UNIT_DEC);
    LUA_CONSTANT(TIME, UNIT_TIME);
    lua_setglobal(L, "UNIT");
    
    lua_newtable(L);
    LUA_CONSTANT(GLOBAL_DRAW, DEP_GLOBAL_DRAW);
    LUA_CONSTANT(LIVEVIEW, DEP_LIVEVIEW);
    LUA_CONSTANT(NOT_LIVEVIEW, DEP_NOT_LIVEVIEW);
    LUA_CONSTANT(MOVIE_MODE, DEP_MOVIE_MODE);
    LUA_CONSTANT(PHOTO_MODE, DEP_PHOTO_MODE);
    LUA_CONSTANT(AUTOFOCUS, DEP_AUTOFOCUS);
    LUA_CONSTANT(MANUAL_FOCUS, DEP_MANUAL_FOCUS);
    LUA_CONSTANT(CFN_AF_HALFSHUTTER, DEP_CFN_AF_HALFSHUTTER);
    LUA_CONSTANT(CFN_AF_BACK_BUTTON, DEP_CFN_AF_BACK_BUTTON);
    LUA_CONSTANT(EXPSIM, DEP_EXPSIM);
    LUA_CONSTANT(NOT_EXPSIM, DEP_NOT_EXPSIM);
    LUA_CONSTANT(CHIPPED_LENS, DEP_CHIPPED_LENS);
    LUA_CONSTANT(M_MODE, DEP_M_MODE);
    LUA_CONSTANT(MANUAL_ISO, DEP_MANUAL_ISO);
    LUA_CONSTANT(SOUND_RECORDING, DEP_SOUND_RECORDING);
    LUA_CONSTANT(NOT_SOUND_RECORDING, DEP_NOT_SOUND_RECORDING);
    lua_setglobal(L, "DEPENDS_ON");
    
    lua_getglobal(L, "_G");
    luaL_setfuncs(L, globallib, 0);
    return L;
}

static void update_lua_menu_values(lua_State * L)
{
    struct script_entry * current;
    for(current = scripts; current; current = current->next)
    {
        //only update values for entries related to the current lua state, and that don't already have a select function
        if(current->L == L && current->menu_entry && current->menu_entry->select == NULL)
        {
            if(lua_getglobal(L, "menu") == LUA_TTABLE)
            {
                if(current->submenu_index)
                {
                    if(lua_getfield(L, -1, "submenu") != LUA_TTABLE)
                    {
                        console_printf("script error: could not find submenu\n");
                        lua_pop(L, 2);
                        continue;
                    }
                    if(lua_geti(L, -1, current->submenu_index) != LUA_TTABLE)
                    {
                        console_printf("script error: could not find submenu '%d'\n", current->submenu_index);
                        lua_pop(L, 3);
                        continue;
                    }
                }
                
                if(current->menu_entry->choices)
                {
                    lua_pushstring(L, current->menu_entry->choices[COERCE(current->menu_value, 0, current->menu_entry->max)]);
                }
                else
                {
                    lua_pushinteger(L, current->menu_value);
                }
                lua_setfield(L, -2, "value");
                
                if(current->submenu_index)
                {
                    lua_pop(L, 2);
                }
            }
            lua_pop(L, 1);
        }
    }
}

static void lua_run_task(int unused)
{
    lua_running = 1;
    if(running_script && running_script->L)
    {
        lua_State * L = running_script->L;
        console_printf("running script...\n");
        if(lua_pcall(L, lua_run_arg_count, 0, 0))
        {
            console_printf("script failed:\n %s\n", lua_tostring(L, -1));
        }
        else
        {
            console_printf("script finished\n");
        }
        if(running_script->submenu_index) lua_pop(L, 1);
    }
    lua_running = 0;
}

static MENU_SELECT_FUNC(run_script)
{
    if (!lua_running)
    {
        lua_running = 1;
        lua_run_arg_count = 0;
        running_script = priv;
        lua_State * L = running_script->L;
        lua_getglobal(L, "main");
        if(!lua_isfunction(L, -1))
        {
            console_printf("script error: no main function\n");
            lua_running = 0;
        }
        else
        {
            task_create("lua_task", 0x1c, 0x4000, lua_run_task, (void*) 0);
        }
    }
}

static MENU_SELECT_FUNC(script_menu_select)
{
    struct script_entry * script_entry = priv;
    if(script_entry)
    {
        lua_State * L = script_entry->L;
        if(L)
        {
            update_lua_menu_values(L);
            lua_getglobal(L, "menu");
            if(lua_istable(L, -1))
            {
                if(script_entry->submenu_index)
                {
                    if(lua_getfield(L, -1, "submenu") != LUA_TTABLE)
                    {
                        console_printf("script error: could not find submenu\n");
                        lua_pop(L, 2);
                        return;
                    }
                    if(lua_geti(L, -1, script_entry->submenu_index) != LUA_TTABLE)
                    {
                        console_printf("script error: could not find submenu '%d'\n", script_entry->submenu_index);
                        lua_pop(L, 3);
                        return;
                    }
                }
                
                if(lua_getfield(L, -1, "select") == LUA_TFUNCTION)
                {
                    if (!lua_running)
                    {
                        lua_run_arg_count = 1;
                        lua_pushinteger(L, delta);
                        running_script = script_entry;
                        lua_running = 1;
                        task_create("lua_task", 0x1c, 0x8000, lua_run_task, (void*) 0);
                    }
                    else
                    {
                        console_printf("script error: another script is currently running\n");
                        lua_pop(L, 1);
                        if(script_entry->submenu_index) lua_pop(L, 2);
                    }
                }
                else
                {
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "value");
                    int current_value = lua_isinteger(L, -1) ? lua_tointeger(L, -1) : 0;
                    lua_pop(L, 1);
                    
                    current_value += delta;
                    if(current_value > script_entry->menu_entry->max) current_value = script_entry->menu_entry->min;
                    else if(current_value < script_entry->menu_entry->min) current_value = script_entry->menu_entry->max;
                    lua_pushinteger(L, current_value);
                    lua_setfield(L, -2, "value");
                    
                    script_entry->menu_value = current_value;
                    
                    lua_pop(L, 1);
                    if(script_entry->submenu_index) lua_pop(L, 2);
                }
            }
        }
    }
}

static MENU_UPDATE_FUNC(script_menu_update)
{
    struct script_entry * script_entry = entry->priv;
    if(script_entry)
    {
        lua_State * L = script_entry->L;
        if(L)
        {
            update_lua_menu_values(L);
            lua_getglobal(L, "menu");
            if(lua_istable(L, -1))
            {
                if(script_entry->submenu_index)
                {
                    if(lua_getfield(L, -1, "submenu") != LUA_TTABLE)
                    {
                        console_printf("script error: could not find submenu\n");
                        MENU_SET_VALUE("");
                        lua_pop(L, 2);
                        return;
                    }
                    if(lua_geti(L, -1, script_entry->submenu_index) != LUA_TTABLE)
                    {
                        console_printf("script error: could not find submenu '%d'\n", script_entry->submenu_index);
                        MENU_SET_VALUE("");
                        lua_pop(L, 3);
                        return;
                    }
                }
                
                if(lua_getfield(L, -1, "update") == LUA_TFUNCTION)
                {
                    if(!lua_pcall(L, 0, 1, 0))
                    {
                        MENU_SET_VALUE("%s", lua_tostring(L, -1));
                    }
                }
                else if(lua_isstring(L, -1))
                {
                    MENU_SET_VALUE("%s", lua_tostring(L, -1));
                }
                lua_pop(L, 1);
                
                if(lua_getfield(L, -1, "info") == LUA_TFUNCTION)
                {
                    if(!lua_pcall(L, 0, 1, 0))
                    {
                        if(lua_isstring(L, -1))
                        {
                            MENU_SET_WARNING(MENU_WARN_INFO, "%s", lua_tostring(L, -1));
                        }
                    }
                }
                lua_pop(L, 1);
                
                if(lua_getfield(L, -1, "warning") == LUA_TFUNCTION)
                {
                    if(!lua_pcall(L, 0, 1, 0))
                    {
                        if(lua_isstring(L, -1))
                        {
                            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "%s", lua_tostring(L, -1));
                        }
                    }
                }
                lua_pop(L, 1);
                
                if(script_entry->submenu_index) lua_pop(L, 2);
            }
            lua_pop(L, 1);
        }
    }
    else
    {
        MENU_SET_VALUE("");
    }
}

static struct script_entry * create_script_entry(lua_State * L, struct menu_entry * existing_menu_entry)
{
    struct script_entry * script_entry = malloc(sizeof(struct script_entry));
    
    if(script_entry)
    {
        if(existing_menu_entry)
        {
            script_entry->menu_entry = existing_menu_entry;
        }
        else
        {
            script_entry->menu_entry = malloc(sizeof(struct menu_entry));
            if(!script_entry->menu_entry)
            {
                free(script_entry);
                lua_close(L);
                return NULL;
            }
        }
        script_entry_count++;
        script_entry->next = scripts;
        scripts = script_entry;
        script_entry->L = L;
        memset(script_entry->menu_entry, 0, sizeof(struct menu_entry));
        script_entry->menu_entry->priv = script_entry;
        script_entry->submenu_index = 0;
        return script_entry;
    }
    return NULL;
}

static int lua_hasfield(lua_State * L, int idx, const char * name, int expected_type)
{
    int result = lua_getfield(L, -1, name) == expected_type;
    lua_pop(L, 1);
    return result;
}

static void load_menu_entry(lua_State * L, struct menu_entry * menu_entry, const char * default_name)
{
    menu_entry->name = LUA_FIELD_STRING("name", default_name);
    menu_entry->help = LUA_FIELD_STRING("help", "");
    menu_entry->help2 = LUA_FIELD_STRING("help2", "");
    menu_entry->depends_on = LUA_FIELD_INT("depends_on", 0);
    menu_entry->icon_type = LUA_FIELD_INT("icon_type", 0);
    menu_entry->unit = LUA_FIELD_INT("unit", 0);
    menu_entry->min = LUA_FIELD_INT("min", 0);
    menu_entry->max = LUA_FIELD_INT("max", 0);
    menu_entry->works_best_in = LUA_FIELD_INT("works_best_in", 0);
    if(lua_getfield(L, -1, "choices") == LUA_TTABLE)
    {
        int choices_count = luaL_len(L, -1);
        menu_entry->choices = malloc(sizeof(char*) * choices_count);
        if(menu_entry->choices)
        {
            int choice_index = 0;
            for (choice_index = 0; choice_index < choices_count; choice_index++)
            {
                if(lua_geti(L, -1, choice_index + 1) == LUA_TSTRING) //lua arrays are 1 based
                {
                    menu_entry->choices[choice_index] = lua_tostring(L, -1);
                }
                else
                {
                    console_printf("invalid choice[%d]\n", choice_index);
                    menu_entry->choices[choice_index] = NULL;
                    choices_count = choice_index;
                }
                lua_pop(L, 1);
            }
            menu_entry->min = 0;
            menu_entry->max = choices_count - 1;
        }
    }
    lua_pop(L, 1);
    menu_entry->select = lua_hasfield(L, -1, "select", LUA_TFUNCTION) ? script_menu_select : NULL;
    menu_entry->update =
        lua_hasfield(L, -1, "update", LUA_TFUNCTION) ||
        lua_hasfield(L, -1, "warning", LUA_TFUNCTION) ||
        lua_hasfield(L, -1, "info", LUA_TFUNCTION) ? script_menu_update : NULL;
}

static void add_script(const char * filename)
{
    lua_State* L = load_lua_state();
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", filename);
    console_printf("loading script: %s\n", filename);
    if(luaL_dofile(L, full_path))
    {
        console_printf("load script '%s' failed:\n %s\n", filename, lua_tostring(L, -1));
    }
    else
    {
        lua_getglobal(L, "menu");
        if(lua_istable(L, -1))
        {
            //script that defines it's own menu structure
            struct script_entry * script_entry = create_script_entry(L, NULL);
            if(script_entry)
            {
                const char * parent = LUA_FIELD_STRING("parent", "LUA");
                load_menu_entry(L, script_entry->menu_entry, filename);
                
                //submenu
                if(lua_getfield(L, -1, "submenu") == LUA_TTABLE)
                {
                    int submenu_count = luaL_len(L, -1);
                    if(submenu_count > 0)
                    {
                        int submenu_index = 0;
                        script_entry->menu_value = 1;
                        script_entry->menu_entry->icon_type = IT_SUBMENU;
                        script_entry->menu_entry->select = menu_open_submenu;
                        script_entry->menu_entry->children = malloc(sizeof(struct menu_entry) * (1 + submenu_count));
                        memset(script_entry->menu_entry->children, 0, sizeof(struct menu_entry) * (1 + submenu_count));
                        for (submenu_index = 0; submenu_index < submenu_count; submenu_index++)
                        {
                            if(lua_geti(L, -1, submenu_index + 1) == LUA_TTABLE) //lua arrays are 1 based
                            {
                                struct script_entry * submenu_entry = create_script_entry(L, &(script_entry->menu_entry->children[submenu_index]));
                                if(submenu_entry)
                                {
                                    load_menu_entry(L, submenu_entry->menu_entry, "unknown");
                                    submenu_entry->menu_value = LUA_FIELD_INT("value", 0);
                                    submenu_entry->submenu_index = submenu_index + 1;
                                }
                            }
                            else
                            {
                                console_printf("invalid submenu[%d]\n", submenu_index);
                            }
                            lua_pop(L, 1);
                        }
                        script_entry->menu_entry->children[submenu_index].priv = MENU_EOL_PRIV;
                    }
                }
                lua_pop(L, 1);
                
                menu_add(parent, script_entry->menu_entry, 1);
            }
        }
        else
        {
            //simple script w/o menu
            struct script_entry * script_entry = create_script_entry(L, NULL);
            if(script_entry)
            {
                lua_getglobal(L, "script_name");
                script_entry->menu_entry->name = lua_isstring(L, -1) ? lua_tostring(L, -1) : filename;
                lua_getglobal(L, "script_help");
                script_entry->menu_entry->help = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
                script_entry->menu_entry->select = run_script;
                script_entry->menu_entry->update = script_menu_update;
                script_entry->menu_entry->icon_type = IT_ACTION;
                menu_add("LUA", script_entry->menu_entry, 1);
            }
        }
        
        console_printf("loading finished: %s\n", filename);
    }
}

static void lua_load_task(int unused)
{
    struct fio_file file;
    struct fio_dirent * dirent = 0;
    
    dirent = FIO_FindFirstEx(SCRIPTS_DIR, &file);
    if(!IS_ERROR(dirent))
    {
        do
        {
            if (!(file.mode & ATTR_DIRECTORY) && string_ends_with(file.name, ".LUA"))
            {
                add_script(file.name);
            }
        }
        while(FIO_FindNextEx(dirent, &file) == 0);
    }
    lua_running = 0;
    lua_loaded = 1;
}

static unsigned int lua_init()
{
    lua_running = 1;
    task_create("lua_load_task", 0x1c, 0x8000, lua_load_task, (void*) 0);
    return 0;
}

static unsigned int lua_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(lua_init)
    MODULE_DEINIT(lua_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBRS_END()


