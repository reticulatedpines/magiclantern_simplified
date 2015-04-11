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
#include <beep.h>
#include <fio-ml.h>
#include <shoot.h>
#include <zebra.h>
#include <focus.h>
#include "lua_common.h"

struct script_event_entry
{
    struct script_event_entry * next;
    int function_ref;
    lua_State * L;
};
static int lua_loaded = 0;

int lua_running = 0;
int last_keypress = 0;

/*
 Determines if a string ends in some string
 */
static int string_ends_with(const char *source, const char *ending)
{
    if(source == NULL || ending == NULL) return 0;
    if(strlen(source) <= 0) return 0;
    if(strlen(source) < strlen(ending)) return 0;
    return !strcmp(source + strlen(source) - strlen(ending), ending);
}

/*** 
 Event Handlers.
 
 Scripts can repsond to events by setting the functions in the 'event' table.
 Event handler functions can take one integer parameter, and must return a boolean
 that specifies whether or not the backend should continue executing event handlers
 for this particular event.
 
 Event handlers will not run if there's already a script or another event handler 
 actively executing at the same time.
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module event
 */

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
        if(current->function_ref != LUA_NOREF)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, current->function_ref) == LUA_TFUNCTION)
            {
                lua_pushinteger(L, ctx);
                if(docall(L, 1, 1))
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
            lua_pop(L,1);
        }
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

#define SCRIPT_CBR_SET(event) \
if(!strcmp(key, #event))\
{\
    if(event##_cbr_scripts->function_ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, event##_cbr_scripts->function_ref);\
    event##_cbr_scripts->function_ref = lua_isfunction(L, 3) ? luaL_ref(L, LUA_REGISTRYINDEX) : LUA_NOREF;\
    return 0;\
}\

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

static int luaCB_event_index(lua_State * L)
{
    //LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    lua_rawget(L, 1);
    return 1;
}

static int luaCB_event_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    /// Called before a picture is taken
    // @param arg
    // @return whether or not to continue executing CBRs for this event
    // @function pre_shoot
    SCRIPT_CBR_SET(pre_shoot);
    /// Called after a picture is taken
    // @param arg
    // @return whether or not to continue executing CBRs for this event
    // @function post_shoot
    SCRIPT_CBR_SET(post_shoot);
    /// Called periodicaly from shoot_task
    // @param arg
    // @return whether or not to continue executing CBRs for this event
    // @function shoot_task
    SCRIPT_CBR_SET(shoot_task);
    /// Called each second
    // @param arg
    // @return whether or not to continue executing CBRs for this event
    // @function seconds_clock
    SCRIPT_CBR_SET(seconds_clock);
    /// Called when a key is pressed
    // @param key the key that was pressed, see @{constants.KEY}
    // @return whether or not to continue executing CBRs for this event
    // @function keypress
    SCRIPT_CBR_SET(keypress);
    /// Special types of picture taking (e.g. silent pics); so intervalometer and other photo taking routines should use that instead of regular pics
    // @param arg
    // @return whether or not to continue executing CBRs for this event
    // @function custom_picture_taking
    SCRIPT_CBR_SET(custom_picture_taking);
    /// called after a picture is taken with the intervalometer
    // @param arg
    // @return whether or not to continue executing CBRs for this event
    // @function intervalometer
    SCRIPT_CBR_SET(intervalometer);
#ifdef CONFIG_VSYNC_EVENTS
    SCRIPT_CBR_SET(display_filter);
    SCRIPT_CBR_SET(vsync);
    SCRIPT_CBR_SET(vsync_setparam);
#endif
    
    lua_rawset(L, 1);
    return 0;
}

static const luaL_Reg eventlib[] =
{
    { NULL, NULL }
};

LUA_LIB(event)

static lua_State * load_lua_state()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    
    luaL_requiref(L, "globals", luaopen_globals, 0);
    luaL_requiref(L, "console", luaopen_console, 1);
    luaL_requiref(L, "camera", luaopen_camera, 1);
    luaL_requiref(L, "lv", luaopen_lv, 1);
    luaL_requiref(L, "lens", luaopen_lens, 1);
    luaL_requiref(L, "movie", luaopen_movie, 1);
    luaL_requiref(L, "display", luaopen_display, 1);
    luaL_requiref(L, "key", luaopen_key, 1);
    luaL_requiref(L, "menu", luaopen_menu, 1);
    luaL_requiref(L, "event", luaopen_event, 1);
    
    luaL_requiref(L, "MODE", luaopen_MODE, 1);
    luaL_requiref(L, "ICON_TYPE", luaopen_ICON_TYPE, 1);
    luaL_requiref(L, "UNIT", luaopen_UNIT, 1);
    luaL_requiref(L, "DEPENDS_ON", luaopen_DEPENDS_ON, 1);
    luaL_requiref(L, "FONT", luaopen_FONT, 1);
    luaL_requiref(L, "COLOR", luaopen_COLOR, 1);
    luaL_requiref(L, "KEY", luaopen_KEY, 1);
    
    return L;
}

static void add_script(const char * filename)
{
    lua_State* L = load_lua_state();
    
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", filename);
    console_printf("loading script: %s\n", filename);
    if(luaL_loadfile(L, full_path) || docall(L, 0, LUA_MULTRET))
    {
        console_printf("load script '%s' failed:\n %s\n", filename, lua_tostring(L, -1));
    }
    else
    {
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


