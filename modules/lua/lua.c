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
#include <screenshot.h>
#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

#define MAX_PATH_LEN 0x80
#define SCRIPTS_DIR "ML/SCRIPTS"
#define EC2RAW(ec) ec * 8 / 10
#define CBR_RET_KEYPRESS_NOTHANDLED 1
#define CBR_RET_KEYPRESS_HANDLED 0
//#define CONFIG_VSYNC_EVENTS

struct script_menu_entry
{
    int menu_value;
    lua_State * L;
    struct menu_entry * menu_entry;
    int select_ref;
    int update_ref;
    int warning_ref;
    int info_ref;
    int submenu_ref;
};
struct script_event_entry
{
    struct script_event_entry * next;
    lua_State * L;
};
static lua_State * running_script = NULL;
static int lua_running = 0;
static int lua_loaded = 0;
static int lua_run_arg_count = 0;

#define LUA_PARAM_INT(name, index)\
if(index > lua_gettop(L) || !lua_isinteger(L, index))\
{\
    lua_pushliteral(L, "Invalid or missing parameter: " #name);\
    lua_error(L);\
}\
int name = lua_tointeger(L, index)

#define LUA_PARAM_INT_OPTIONAL(name, index, default) int name = (index <= lua_gettop(L) && lua_isinteger(L, index)) ? lua_tointeger(L, index) : default

#define LUA_PARAM_BOOL(name, index)\
if(index > lua_gettop(L) || !lua_isboolean(L, index))\
{\
    lua_pushliteral(L, "Invalid or missing parameter: " #name);\
    lua_error(L);\
}\
int name = lua_toboolean(L, index)

#define LUA_PARAM_BOOL_OPTIONAL(name, index, default) int name = (index <= lua_gettop(L) && lua_isboolean(L, index)) ? lua_toboolean(L, index) : default

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

static int luaCB_camera_bulb(lua_State * L)
{
    LUA_PARAM_INT(duration, 1);
    bulb_take_pic(duration);
    return 0;
}

static int luaCB_camera_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
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
    { "bulb", luaCB_camera_bulb },
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
        LUA_PARAM_BOOL(value, 3);
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

static int luaCB_display_on(lua_State * L)
{
    display_on();
    return 0;
}

static int luaCB_display_off(lua_State * L)
{
    display_off();
    return 0;
}

static int luaCB_display_screenshot(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(filename, 1, SCREENSHOT_FILENAME_AUTO);
    LUA_PARAM_INT_OPTIONAL(mode, 2, SCREENSHOT_BMP | SCREENSHOT_YUV);
    take_screenshot((char*)filename, mode);
    return 0;
}

static int luaCB_display_clear(lua_State * L)
{
    clrscr();
    return 0;
}

static int luaCB_display_print(lua_State * L)
{
    LUA_PARAM_STRING(str, 1);
    LUA_PARAM_INT(x, 2);
    LUA_PARAM_INT(y, 3);
    LUA_PARAM_INT_OPTIONAL(font, 4, (int)FONT_MED);
    LUA_PARAM_INT_OPTIONAL(fg, 5, COLOR_WHITE);
    LUA_PARAM_INT_OPTIONAL(bg, 6, COLOR_BLACK);
    bmp_printf(FONT(font, fg, bg), x, y, "%s", str);
    return 0;
}

static int luaCB_display_pixel(lua_State * L)
{
    LUA_PARAM_INT(x, 1);
    LUA_PARAM_INT(y, 2);
    LUA_PARAM_INT(color, 3);
    bmp_putpixel(x, y, (uint8_t)color);
    return 0;
}

static int luaCB_display_line(lua_State * L)
{
    LUA_PARAM_INT(x1, 1);
    LUA_PARAM_INT(y1, 2);
    LUA_PARAM_INT(x2, 3);
    LUA_PARAM_INT(y2, 4);
    LUA_PARAM_INT(color, 5);
    draw_line(x1, y1, x2, y2, (uint8_t)color);
    return 0;
}

static int luaCB_display_rect(lua_State * L)
{
    LUA_PARAM_INT(x, 1);
    LUA_PARAM_INT(y, 2);
    LUA_PARAM_INT(w, 3);
    LUA_PARAM_INT(h, 4);
    LUA_PARAM_INT(stroke, 5);
    LUA_PARAM_INT_OPTIONAL(fill, 6, -1);
    if(fill >= 0) bmp_fill((uint8_t)fill, x, y, w, h);
    if(stroke >= 0) bmp_draw_rect((uint8_t)stroke, x, y, w, h);
    return 0;
}

static int luaCB_display_circle(lua_State * L)
{
    LUA_PARAM_INT(x, 1);
    LUA_PARAM_INT(y, 2);
    LUA_PARAM_INT(r, 3);
    LUA_PARAM_INT(stroke, 4);
    LUA_PARAM_INT_OPTIONAL(fill, 5, -1);
    if(fill >= 0) fill_circle(x, y, r, (uint8_t)fill);
    if(stroke >= 0) draw_circle(x, y, r, (uint8_t)stroke);
    return 0;
}

static int luaCB_display_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "idle")) lua_pushboolean(L, display_idle());
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_display_newindex(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "idle"))
    {
        lua_pushstring(L, "property is readonly!"); lua_error(L);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

const luaL_Reg displaylib[] =
{
    {"on", luaCB_display_on},
    {"off", luaCB_display_off},
    {"screenshot", luaCB_display_screenshot},
    {"clear", luaCB_display_clear},
    {"print", luaCB_display_print},
    {"pixel", luaCB_display_pixel},
    {"line", luaCB_display_line},
    {"rect", luaCB_display_rect},
    {"circle", luaCB_display_circle},
    {NULL, NULL}
};

static int last_keypress = 0;

static int luaCB_key_press(lua_State * L)
{
    LUA_PARAM_INT(key, 1);
    module_send_keypress(key);
    return 0;
}

static int luaCB_key_wait(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(key, 1, 0);
    LUA_PARAM_INT_OPTIONAL(timeout, 1, 0);
    timeout *= 10;
    last_keypress = 0;
    int time = 0;
    //TODO: probably better to use a semaphore
    while((key && last_keypress != key) || (!key && !last_keypress))
    {
        msleep(100);
        if(timeout && time++ > timeout)
        {
            lua_pushinteger(L, 0);
            return 1;
        }
    }
    lua_pushinteger(L, last_keypress);
    return 1;
}

static int luaCB_key_index(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "last")) lua_pushinteger(L, last_keypress);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_key_newindex(lua_State * L)
{
    LUA_PARAM_STRING(key, 2);
    if(!strcmp(key, "last"))
    {
        lua_pushstring(L, "property is readonly!"); lua_error(L);
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

const luaL_Reg keylib[] =
{
    {"press", luaCB_key_press},
    {"wait", luaCB_key_wait},
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
    LOAD_LUA_LIB(display);
    LOAD_LUA_LIB(key);
    
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
    
    lua_newtable(L);
    LUA_CONSTANT(MONO_12, FONT_MONO_12);
    LUA_CONSTANT(MONO_20, FONT_MONO_20);
    LUA_CONSTANT(SANS_23, FONT_SANS_23);
    LUA_CONSTANT(SANS_28, FONT_SANS_28);
    LUA_CONSTANT(SANS_32, FONT_SANS_32);
    LUA_CONSTANT(CANON, FONT_CANON);
    LUA_CONSTANT(SMALL, FONT_SMALL);
    LUA_CONSTANT(MED, FONT_MED);
    LUA_CONSTANT(MED_LARGE, FONT_MED_LARGE);
    lua_setglobal(L, "FONT");
    
    lua_newtable(L);
    LUA_CONSTANT(TRANSPARENT, COLOR_EMPTY);
    LUA_CONSTANT(WHITE, COLOR_WHITE);
    LUA_CONSTANT(BLACK, COLOR_BLACK);
    LUA_CONSTANT(TRANSPARENT_BLACK, COLOR_TRANSPARENT_BLACK);
    LUA_CONSTANT(CYAN, COLOR_CYAN);
    LUA_CONSTANT(GREEN1, COLOR_GREEN1);
    LUA_CONSTANT(GREEN2, COLOR_GREEN2);
    LUA_CONSTANT(RED, COLOR_RED);
    LUA_CONSTANT(LIGHT_BLUE, COLOR_LIGHT_BLUE);
    LUA_CONSTANT(BLUE, COLOR_BLUE);
    LUA_CONSTANT(DARK_RED, COLOR_DARK_RED);
    LUA_CONSTANT(MAGENTA, COLOR_MAGENTA);
    LUA_CONSTANT(YELLOW, COLOR_YELLOW);
    LUA_CONSTANT(ORANGE, COLOR_ORANGE);
    LUA_CONSTANT(ALMOST_BLACK, COLOR_ALMOST_BLACK);
    LUA_CONSTANT(ALMOST_WHITE, COLOR_ALMOST_WHITE);
    LUA_CONSTANT(DARK_GREEN1_MOD, COLOR_DARK_GREEN1_MOD);
    LUA_CONSTANT(DARK_GREEN2_MOD, COLOR_DARK_GREEN2_MOD);
    LUA_CONSTANT(DARK_ORANGE_MOD, COLOR_DARK_ORANGE_MOD);
    LUA_CONSTANT(DARK_CYAN1_MOD, COLOR_DARK_CYAN1_MOD);
    LUA_CONSTANT(DARK_CYAN2_MOD, COLOR_DARK_CYAN2_MOD);
    lua_setglobal(L, "COLOR");
    
    lua_newtable(L);
    LUA_CONSTANT(HALFSHUTTER, MODULE_KEY_PRESS_HALFSHUTTER);
    LUA_CONSTANT(UNPRESS_HALFSHUTTER, MODULE_KEY_UNPRESS_HALFSHUTTER);
    LUA_CONSTANT(FULLSHUTTER, MODULE_KEY_PRESS_FULLSHUTTER);
    LUA_CONSTANT(UNPRESS_FULLSHUTTER, MODULE_KEY_UNPRESS_FULLSHUTTER);
    LUA_CONSTANT(WHEEL_UP, MODULE_KEY_WHEEL_UP);
    LUA_CONSTANT(WHEEL_DOWN, MODULE_KEY_WHEEL_DOWN);
    LUA_CONSTANT(WHEEL_LEFT, MODULE_KEY_WHEEL_LEFT);
    LUA_CONSTANT(WHEEL_RIGHT, MODULE_KEY_WHEEL_RIGHT);
    LUA_CONSTANT(SET, MODULE_KEY_PRESS_SET);
    LUA_CONSTANT(UNPRESS_SET, MODULE_KEY_UNPRESS_SET);
    LUA_CONSTANT(JOY_CENTER, MODULE_KEY_JOY_CENTER);
    LUA_CONSTANT(UP, MODULE_KEY_PRESS_UP);
    LUA_CONSTANT(UP_RIGHT, MODULE_KEY_PRESS_UP_RIGHT);
    LUA_CONSTANT(UP_LEFT, MODULE_KEY_PRESS_UP_LEFT);
    LUA_CONSTANT(RIGHT, MODULE_KEY_PRESS_RIGHT);
    LUA_CONSTANT(LEFT, MODULE_KEY_PRESS_LEFT);
    LUA_CONSTANT(DOWN_RIGHT, MODULE_KEY_PRESS_DOWN_RIGHT);
    LUA_CONSTANT(DOWN_LEFT, MODULE_KEY_PRESS_DOWN_LEFT);
    LUA_CONSTANT(DOWN, MODULE_KEY_PRESS_DOWN);
    LUA_CONSTANT(UNPRESS_UDLR, MODULE_KEY_UNPRESS_UDLR);
    LUA_CONSTANT(ZOOMIN, MODULE_KEY_PRESS_ZOOMIN);
    LUA_CONSTANT(MENU, MODULE_KEY_MENU);
    LUA_CONSTANT(INFO, MODULE_KEY_INFO);
    LUA_CONSTANT(PLAY, MODULE_KEY_PLAY);
    LUA_CONSTANT(TRASH, MODULE_KEY_TRASH);
    LUA_CONSTANT(RATE, MODULE_KEY_RATE);
    LUA_CONSTANT(REC, MODULE_KEY_REC);
    LUA_CONSTANT(LV, MODULE_KEY_LV);
    LUA_CONSTANT(Q, MODULE_KEY_Q);
    LUA_CONSTANT(PICSTYLE, MODULE_KEY_PICSTYLE);
    LUA_CONSTANT(FLASH_MOVIE, MODULE_KEY_PRESS_FLASH_MOVIE);
    LUA_CONSTANT(UNPRESS_FLASH_MOVIE, MODULE_KEY_UNPRESS_FLASH_MOVIE);
    LUA_CONSTANT(DP, MODULE_KEY_PRESS_DP);
    LUA_CONSTANT(UNPRESS_DP, MODULE_KEY_UNPRESS_DP);
    LUA_CONSTANT(TOUCH_1_FINGER, MODULE_KEY_TOUCH_1_FINGER);
    LUA_CONSTANT(UNTOUCH_1_FINGER, MODULE_KEY_UNTOUCH_1_FINGER);
    LUA_CONSTANT(TOUCH_2_FINGER, MODULE_KEY_TOUCH_2_FINGER);
    LUA_CONSTANT(UNTOUCH_2_FINGER, MODULE_KEY_UNTOUCH_2_FINGER);
    lua_setglobal(L, "KEY");
    
    lua_getglobal(L, "_G");
    luaL_setfuncs(L, globallib, 0);
    return L;
}

static unsigned int lua_do_cbr(unsigned int ctx, struct script_event_entry * event_entries, const char * event_name, int sucess, int failure)
{
    //no events registered by lua scripts
    if(!event_entries) return sucess;
    
    //something is currently running
    if(lua_running)
    {
        console_printf("lua cbr error: another script is currently running\n");
        return sucess;
    }
    lua_running = 1;
    struct script_event_entry * current;
    for(current = event_entries; current; current = current->next)
    {
        lua_State * L = current->L;
        if(lua_getglobal(L, "events") == LUA_TTABLE)
        {
            if(lua_getfield(L, -1, event_name) == LUA_TFUNCTION)
            {
                lua_pushinteger(L, ctx);
                if(lua_pcall(L, 1, 1, 0))
                {
                    console_printf("lua cbr error:\n %s\n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                    lua_running = 0;
                    return CBR_RET_ERROR;
                }
                else
                {
                    if(lua_isboolean(L, -1) && !lua_toboolean(L, -1))
                    {
                        lua_pop(L, 1);
                        lua_running = 0;
                        return failure;
                    }
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_running = 0;
    return sucess;
}

#define LUA_CBR_FUNC(name)\
static struct script_event_entry * name##_cbr_scripts = NULL;\
static unsigned int lua_##name##_cbr(unsigned int ctx) {\
    return lua_do_cbr(ctx, name##_cbr_scripts, #name, CBR_RET_CONTINUE, CBR_RET_STOP);\
}\

LUA_CBR_FUNC(pre_shoot)
LUA_CBR_FUNC(post_shoot)
LUA_CBR_FUNC(shoot_task)
LUA_CBR_FUNC(seconds_clock)
LUA_CBR_FUNC(custom_picture_taking)
LUA_CBR_FUNC(intervalometer)

#ifdef CONFIG_VSYNC_EVENTS
LUA_CBR_FUNC(vsync)
LUA_CBR_FUNC(display_filter)
LUA_CBR_FUNC(vsync_setparam)
#endif

static struct script_event_entry * keypress_cbr_scripts = NULL;
static unsigned int lua_keypress_cbr(unsigned int ctx)
{
    last_keypress = ctx;
    //keypress cbr interprets things backwards from other CBRs
    return lua_do_cbr(ctx, keypress_cbr_scripts, "keypress", CBR_RET_KEYPRESS_NOTHANDLED, CBR_RET_KEYPRESS_HANDLED);
}

static void lua_run_task(int unused)
{
    lua_running = 1;
    if(running_script)
    {
        lua_State * L = running_script;
        console_printf("running script...\n");
        if(lua_pcall(L, lua_run_arg_count, 0, 0))
        {
            console_printf("script failed:\n %s\n", lua_tostring(L, -1));
        }
        else
        {
            console_printf("script finished\n");
        }
    }
    lua_running = 0;
}

static MENU_SELECT_FUNC(script_menu_select)
{
    if(lua_running)
    {
        console_printf("script error: another script is currently running\n");
        return;
    }
    
    struct script_menu_entry * script_entry = priv;
    if(script_entry && script_entry->L && script_entry->select_ref != LUA_NOREF)
    {
        lua_State * L = script_entry->L;
        if(lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->select_ref) == LUA_TFUNCTION)
        {
            lua_run_arg_count = 1;
            lua_pushinteger(L, delta);
            running_script = L;
            lua_running = 1;
            task_create("lua_task", 0x1c, 0x8000, lua_run_task, (void*) 0);
        }
        else
        {
            lua_pushstring(L, "error: select was not a function"); lua_error(L);
        }
    }
}

static MENU_UPDATE_FUNC(script_menu_update)
{
    if(lua_running)
    {
        console_printf("script error: another script is currently running\n");
        return;
    }
    
    struct script_menu_entry * script_entry = entry->priv;
    if(script_entry && script_entry->L)
    {
        lua_State * L = script_entry->L;
        if(script_entry->update_ref != LUA_NOREF)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->update_ref) == LUA_TFUNCTION)
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
            lua_pop(L,1);
        }
        if(script_entry->info_ref != LUA_NOREF)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->info_ref) == LUA_TFUNCTION)
            {
                if(!lua_pcall(L, 0, 1, 0))
                {
                    MENU_SET_WARNING(MENU_WARN_INFO, "%s", lua_tostring(L, -1));
                }
            }
            else if(lua_isstring(L, -1))
            {
                MENU_SET_VALUE("%s", lua_tostring(L, -1));
            }
            lua_pop(L,1);
        }
        if(script_entry->warning_ref != LUA_NOREF)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->warning_ref) == LUA_TFUNCTION)
            {
                if(!lua_pcall(L, 0, 1, 0))
                {
                    MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "%s", lua_tostring(L, -1));
                }
            }
            lua_pop(L,1);
        }
    }
}

static int lua_hasfield(lua_State * L, int idx, const char * name, int expected_type)
{
    int result = lua_getfield(L, -1, name) == expected_type;
    lua_pop(L, 1);
    return result;
}

static int get_index_for_choices(struct menu_entry * menu_entry, const char * value)
{
    int i;
    for(i = 0; i < menu_entry->max; i++)
    {
        if(!strcmp(menu_entry->choices[i], value))
        {
            return i;
        }
    }
    return 0;
}

static int luaCB_menu_index(lua_State * L)
{
    if(!lua_isuserdata(L, 1)) return luaL_argerror(L, 1, NULL);
    struct script_menu_entry * script_entry = lua_touserdata(L, 1);
    if(!script_entry || !script_entry->menu_entry) return luaL_argerror(L, 1, "internal error: userdata was NULL");
    
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "value"))
    {
        if(script_entry->menu_entry->choices)
        {
            lua_pushstring(L, script_entry->menu_entry->choices[script_entry->menu_value]);
        }
        else
        {
            lua_pushinteger(L, script_entry->menu_value);
        }
    }
    else if(!strcmp(key, "name")) lua_pushstring(L, script_entry->menu_entry->name);
    else if(!strcmp(key, "help")) lua_pushstring(L, script_entry->menu_entry->help);
    else if(!strcmp(key, "help2")) lua_pushstring(L, script_entry->menu_entry->help2);
    else if(!strcmp(key, "advanced")) lua_pushinteger(L, script_entry->menu_entry->advanced);
    else if(!strcmp(key, "depends_on")) lua_pushinteger(L, script_entry->menu_entry->depends_on);
    else if(!strcmp(key, "edit_mode")) lua_pushinteger(L, script_entry->menu_entry->edit_mode);
    else if(!strcmp(key, "hidden")) lua_pushinteger(L, script_entry->menu_entry->hidden);
    else if(!strcmp(key, "icon_type")) lua_pushinteger(L, script_entry->menu_entry->icon_type);
    else if(!strcmp(key, "jhidden")) lua_pushinteger(L, script_entry->menu_entry->jhidden);
    else if(!strcmp(key, "max")) lua_pushinteger(L, script_entry->menu_entry->max);
    else if(!strcmp(key, "min")) lua_pushinteger(L, script_entry->menu_entry->min);
    else if(!strcmp(key, "selected")) lua_pushinteger(L, script_entry->menu_entry->selected);
    else if(!strcmp(key, "shidden")) lua_pushinteger(L, script_entry->menu_entry->shidden);
    else if(!strcmp(key, "starred")) lua_pushinteger(L, script_entry->menu_entry->starred);
    else if(!strcmp(key, "submenu_height")) lua_pushinteger(L, script_entry->menu_entry->submenu_height);
    else if(!strcmp(key, "submenu_width")) lua_pushinteger(L, script_entry->menu_entry->submenu_width);
    else if(!strcmp(key, "unit")) lua_pushinteger(L, script_entry->menu_entry->unit);
    else if(!strcmp(key, "works_best_in")) lua_pushinteger(L, script_entry->menu_entry->works_best_in);
    else if(!strcmp(key, "select")) lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->select_ref);
    else if(!strcmp(key, "update")) lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->update_ref);
    else if(!strcmp(key, "info")) lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->info_ref);
    else if(!strcmp(key, "warning")) lua_rawgeti(L, LUA_REGISTRYINDEX, script_entry->warning_ref);
    else
    {
        //retrieve the key from the metatable
        if(lua_getmetatable(L, 1))
        {
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
        }
        else
        {
            lua_pushnil(L);
        }
    }
    return 1;
}

static int luaCB_menu_newindex(lua_State * L)
{
    if(!lua_isuserdata(L, 1)) return luaL_argerror(L, 1, NULL);
    struct script_menu_entry * script_entry = lua_touserdata(L, 1);
    if(!script_entry || !script_entry->menu_entry) return luaL_argerror(L, 1, "internal error: userdata was NULL");
    
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "value"))
    {
        if(script_entry->menu_entry->choices)
        {
            LUA_PARAM_STRING(value, 3);
            script_entry->menu_value = get_index_for_choices(script_entry->menu_entry, value);
        }
        else
        {
            LUA_PARAM_INT(value, 3);
            script_entry->menu_value = value;
        }
    }
    else if(!strcmp(key, "name")) { LUA_PARAM_STRING(value, 3); script_entry->menu_entry->name = value; }
    else if(!strcmp(key, "help")) { LUA_PARAM_STRING(value, 3); script_entry->menu_entry->help = value; }
    else if(!strcmp(key, "help2")) { LUA_PARAM_STRING(value, 3); script_entry->menu_entry->help2 = value; }
    else if(!strcmp(key, "advanced")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->advanced = value; }
    else if(!strcmp(key, "depends_on")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->depends_on = value; }
    else if(!strcmp(key, "edit_mode")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->edit_mode = value; }
    else if(!strcmp(key, "hidden")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->hidden = value; }
    else if(!strcmp(key, "icon_type")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->icon_type = value; }
    else if(!strcmp(key, "jhidden")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->jhidden = value; }
    else if(!strcmp(key, "max")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->max = value; }
    else if(!strcmp(key, "min")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->min = value; }
    else if(!strcmp(key, "selected")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->selected = value; }
    else if(!strcmp(key, "shidden")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->shidden = value; }
    else if(!strcmp(key, "starred")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->starred = value; }
    else if(!strcmp(key, "submenu_height")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->submenu_height = value; }
    else if(!strcmp(key, "submenu_width")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->submenu_width = value; }
    else if(!strcmp(key, "unit")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->unit = value; }
    else if(!strcmp(key, "works_best_in")) { LUA_PARAM_INT(value, 3); script_entry->menu_entry->works_best_in = value; }
    else if(!strcmp(key, "select"))
    {
        if(script_entry->select_ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, script_entry->select_ref);
        if(!lua_isfunction(L, 3))
        {
            script_entry->select_ref = LUA_NOREF;
            script_entry->menu_entry->select = NULL;
        }
        else
        {
            script_entry->select_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            script_entry->menu_entry->select = script_menu_select;
        }
    }
    else if(!strcmp(key, "update"))
    {
        if(script_entry->update_ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, script_entry->update_ref);
        if(!lua_isfunction(L, 3)) script_entry->update_ref = LUA_NOREF;
        else
        {
            script_entry->update_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            script_entry->menu_entry->update = script_menu_update;
        }
    }
    else if(!strcmp(key, "info"))
    {
        if(script_entry->info_ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, script_entry->info_ref);
        if(!lua_isfunction(L, 3)) script_entry->info_ref = LUA_NOREF;
        else
        {
            script_entry->info_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            script_entry->menu_entry->update = script_menu_update;
        }
    }
    else if(!strcmp(key, "warning"))
    {
        if(script_entry->warning_ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, script_entry->warning_ref);
        if(!lua_isfunction(L, 3)) script_entry->warning_ref = LUA_NOREF;
        else
        {
            script_entry->warning_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            script_entry->menu_entry->update = script_menu_update;
        }
    }
    else
    {
        //set the key in the metatable
        if(lua_getmetatable(L, 1))
        {
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
        }
        else
        {
            lua_pushnil(L);
        }
    }
    return 0;
}

static int get_function_ref(lua_State * L, const char * name)
{
    if(lua_getfield(L, -1, name) == LUA_TFUNCTION)
    {
        return luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        lua_pop(L,1);
        return LUA_NOREF;
    }
}

static void load_menu_entry(lua_State * L, struct script_menu_entry * script_entry, struct menu_entry * menu_entry, const char * default_name)
{
    if(!menu_entry)
    {
        menu_entry = malloc(sizeof(struct menu_entry));
        if(!menu_entry)
        {
            lua_pushstring(L, "malloc error creating menu_entry");
            lua_error(L);
            return;
        }
    }
    memset(menu_entry, 0, sizeof(struct menu_entry));
    script_entry->L = L;
    script_entry->menu_entry = menu_entry;
    menu_entry->priv = script_entry;
    menu_entry->name = LUA_FIELD_STRING("name", default_name);
    menu_entry->help = LUA_FIELD_STRING("help", "");
    menu_entry->help2 = LUA_FIELD_STRING("help2", "");
    menu_entry->depends_on = LUA_FIELD_INT("depends_on", 0);
    menu_entry->icon_type = LUA_FIELD_INT("icon_type", 0);
    menu_entry->unit = LUA_FIELD_INT("unit", 0);
    menu_entry->min = LUA_FIELD_INT("min", 0);
    menu_entry->max = LUA_FIELD_INT("max", 0);
    menu_entry->works_best_in = LUA_FIELD_INT("works_best_in", 0);
    menu_entry->submenu_width = LUA_FIELD_INT("submenu_width", 0);
    menu_entry->submenu_height = LUA_FIELD_INT("submenu_height", 0);
    menu_entry->hidden = LUA_FIELD_INT("hidden", 0);
    menu_entry->jhidden = LUA_FIELD_INT("jhidden", 0);
    menu_entry->shidden = LUA_FIELD_INT("shidden", 0);
    menu_entry->starred = LUA_FIELD_INT("starred", 0);
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
    
    if((script_entry->select_ref = get_function_ref(L, "select")) != LUA_NOREF) menu_entry->select = script_menu_select;
    if((script_entry->update_ref = get_function_ref(L, "update")) != LUA_NOREF) menu_entry->update = script_menu_update;
    if((script_entry->warning_ref = get_function_ref(L, "warning")) != LUA_NOREF) menu_entry->update = script_menu_update;
    if((script_entry->info_ref = get_function_ref(L, "info")) != LUA_NOREF) menu_entry->update = script_menu_update;
    
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
            
            if(lua_getmetatable(L, -3))
            {
                //create a new submenu table
                lua_newtable(L);
                lua_setfield(L, -2, "submenu");
                lua_pop(L, 1);
            }
            else
            {
                console_printf("warning: could not create metatable submenu");
            }
            
            for (submenu_index = 0; submenu_index < submenu_count; submenu_index++)
            {
                if(lua_geti(L, -1, submenu_index + 1) == LUA_TTABLE) //lua arrays are 1 based
                {
                    struct script_menu_entry * new_entry = lua_newuserdata(L, sizeof(struct script_menu_entry));
                    
                    //add a metatable to the userdata object for value lookups and to store submenu
                    lua_newtable(L);
                    lua_pushcfunction(L, luaCB_menu_index);
                    lua_setfield(L, -2, "__index");
                    lua_pushcfunction(L, luaCB_menu_newindex);
                    lua_setfield(L, -2, "__newindex");
                    lua_setmetatable(L, -2);
                    
                    lua_pushvalue(L, -2);
                    load_menu_entry(L, new_entry, &(script_entry->menu_entry->children[submenu_index]), "unknown");
                    lua_pop(L, 1);
                    
                    //add the new userdata object to the submenu table of the parent metatable, using the menu name as a key
                    if(lua_getmetatable(L, -5))
                    {
                        if(lua_getfield(L, -1, "submenu") == LUA_TTABLE)
                        {
                            lua_pushvalue(L, -3);
                            lua_setfield(L, -2, script_entry->menu_entry->children[submenu_index].name);
                        }
                        else
                        {
                            console_printf("warning: could not get metatable submenu");
                        }
                        lua_pop(L, 2);
                    }
                    else
                    {
                        console_printf("warning: could not get parent metatable");
                    }
                    
                    lua_pop(L, 1);//userdata
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
    
    //load default 'value' so our index metamethod works
    if(menu_entry->choices)
    {
        const char * str_value = LUA_FIELD_STRING("value", "");
        script_entry->menu_value = get_index_for_choices(menu_entry, str_value);
    }
    else
    {
        script_entry->menu_value = LUA_FIELD_INT("value", 0);
    }
     
}

static int luaCB_menu_new(lua_State * L)
{
    if(!lua_istable(L, 1))
    {
        lua_pushstring(L, "Invalid or missing parameter to menu.new()");
        lua_error(L);
    }
    struct script_menu_entry * new_entry = lua_newuserdata(L, sizeof(struct script_menu_entry));
    //add a metatable to the userdata object for value lookups and to store submenu
    lua_newtable(L);
    lua_pushcfunction(L, luaCB_menu_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_menu_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    
    lua_pushvalue(L, 1);
    const char * parent = LUA_FIELD_STRING("parent", "LUA");
    load_menu_entry(L, new_entry, NULL, "unknown");
    menu_add(parent, new_entry->menu_entry, 1);
    lua_pop(L, 1);
    
    return 1; //return the userdata object
}

const luaL_Reg menulib[] =
{
    {"new", luaCB_menu_new},
    {NULL, NULL}
};

#define SCRIPT_CBR(event) if(lua_getfield(L, -1, #event) == LUA_TFUNCTION) add_event_script_entry(&(event##_cbr_scripts), L); lua_pop(L, 1)

static void add_event_script_entry(struct script_event_entry ** root, lua_State * L)
{
    struct script_event_entry * new_entry = malloc(sizeof(struct script_event_entry));
    if(new_entry)
    {
        new_entry->next = *root;
        new_entry->L = L;
        *root = new_entry;
    }
}

static void add_script(const char * filename)
{
    lua_State* L = load_lua_state();
    
    //load menu table
    lua_newtable(L); luaL_setfuncs(L, menulib, 0); lua_setglobal(L, "menu");
    
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", filename);
    console_printf("loading script: %s\n", filename);
    if(luaL_dofile(L, full_path))
    {
        console_printf("load script '%s' failed:\n %s\n", filename, lua_tostring(L, -1));
    }
    else
    {
        if(lua_getglobal(L, "events") == LUA_TTABLE)
        {
            SCRIPT_CBR(pre_shoot);
            SCRIPT_CBR(post_shoot);
            SCRIPT_CBR(shoot_task);
            SCRIPT_CBR(seconds_clock);
            SCRIPT_CBR(keypress);
            SCRIPT_CBR(custom_picture_taking);
            SCRIPT_CBR(intervalometer);
#ifdef CONFIG_VSYNC_EVENTS
            SCRIPT_CBR(display_filter);
            SCRIPT_CBR(vsync);
            SCRIPT_CBR(vsync_setparam);
#endif
        }
        lua_pop(L, 1);
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
    MODULE_CBR(CBR_PRE_SHOOT, lua_pre_shoot_cbr, 0)
    MODULE_CBR(CBR_POST_SHOOT, lua_post_shoot_cbr, 0)
    MODULE_CBR(CBR_SHOOT_TASK, lua_shoot_task_cbr, 0)
    MODULE_CBR(CBR_SECONDS_CLOCK, lua_seconds_clock_cbr, 0)
    MODULE_CBR(CBR_KEYPRESS, lua_keypress_cbr, 0)
    MODULE_CBR(CBR_CUSTOM_PICTURE_TAKING, lua_custom_picture_taking_cbr, 0)
    MODULE_CBR(CBR_INTERVALOMETER, lua_intervalometer_cbr, 0)

#ifdef CONFIG_VSYNC_EVENTS
    MODULE_CBR(CBR_VSYNC, lua_vsync_cbr, 0)
    MODULE_CBR(CBR_DISPLAY_FILTER, lua_display_filter_cbr, 0)
    MODULE_CBR(CBR_VSYNC_SETPARAM, lua_vsync_setparam_cbr, 0)
#endif

MODULE_CBRS_END()


