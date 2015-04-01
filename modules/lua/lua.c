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
#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

#define MAX_PATH_LEN 0x80
#define SCRIPTS_DIR "ML/SCRIPTS"

struct script_entry
{
    struct script_entry * next;
    lua_State * L;
    struct menu_entry * menu_entry;
};
static struct script_entry * scripts = NULL;
static struct script_entry * running_script = NULL;
static int lua_running = 0;
static int lua_loaded = 0;

#define LUA_PARAM_INT(name, index)\
if(index > lua_gettop(L) || !lua_isinteger(L, index))\
{\
    lua_pushliteral(L, "Invalid or missing parameter: " #name);\
    lua_error(L);\
}\
int name = lua_tointeger(L, index)

#define LUA_PARAM_INT_OPTIONAL(name, index, default) int name = (index <= lua_gettop(L) && lua_isinteger(L, index)) ? lua_tointeger(L, index) : default

#define LUA_PARAM_STRING(name, index)\
if(index > lua_gettop(L) || !lua_isstring(L, index))\
{\
   lua_pushliteral(L, "Invalid or missing parameter: " #name);\
   lua_error(L);\
}\
const char * name = lua_tostring(L, index);

#define LUA_PARAM_STRING_OPTIONAL(name, index, default) const char * name = (index <= lua_gettop(L) && lua_isstring(L, index)) ? lua_tostring(L, index) : default

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

static int camera_shoot(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(wait, 1, 64);
    LUA_PARAM_INT_OPTIONAL(should_af, 2, 1);
    int result = lens_take_picture(wait, should_af);
    lua_pushinteger(L, result);
    return 1;
}

static const luaL_Reg cameralib[] =
{
    { "shoot", camera_shoot },
    { NULL, NULL }
};

static int global_beep(lua_State * L)
{
    LUA_PARAM_INT_OPTIONAL(times, 1, 1);
    beep_times(times);
    return 0;
}

static int global_call(lua_State * L)
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

static int global_msleep(lua_State * L)
{
    LUA_PARAM_INT(amount, 1);
    msleep(amount);
    return 0;
}

static const luaL_Reg globallib[] =
{
    { "msleep", global_msleep },
    { "beep", global_beep },
    { "call", global_call },
    { "shoot", camera_shoot },
    { NULL, NULL }
};

static void lua_run_task(int unused)
{
    if(running_script && running_script->L)
    {
        lua_State * L = running_script->L;
        lua_getglobal(L, "main");
        if(!lua_isfunction(L, -1))
        {
            console_printf("script error: no main function\n");
        }
        else
        {
            console_printf("running script...\n");
            if(lua_pcall(L, 0, LUA_MULTRET, 0))
            {
                console_printf("script failed:\n %s\n", lua_tostring(L, -1));
            }
            else
            {
                console_printf("script finished\n");
            }
        }
    }
    lua_running = 0;
}

static MENU_SELECT_FUNC(run_script)
{
    if (!lua_running)
    {
        lua_running = 1;
        running_script = priv;
        task_create("lua_task", 0x1c, 0x4000, lua_run_task, (void*) 0);
    }
}

static MENU_UPDATE_FUNC(script_update)
{
    MENU_SET_VALUE("");
}

static int add_script(const char * filename)
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    lua_newtable(L);
    luaL_setfuncs(L, cameralib, 0);
    lua_setglobal(L, "camera");
    lua_getglobal(L, "_G");
    luaL_setfuncs(L, globallib, 0);
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", filename);
    console_printf("loading script: %s\n", filename, lua_tostring(L, -1));
    if(luaL_dofile(L, full_path))
    {
        console_printf("load script '%s' failed:\n %s\n", filename, lua_tostring(L, -1));
    }
    else
    {
        struct script_entry * script_entry = malloc(sizeof(struct script_entry));
        if(script_entry)
        {
            script_entry->menu_entry = malloc(sizeof(struct menu_entry));
            if(!script_entry->menu_entry)
            {
                free(script_entry);
                lua_close(L);
                return 0;
            }
            script_entry->next = scripts;
            scripts = script_entry;
            script_entry->L = L;
            memset(script_entry->menu_entry, 0, sizeof(struct menu_entry));
            lua_getglobal(L, "script_name");
            script_entry->menu_entry->name = lua_isstring(L, -1) ? lua_tostring(L, -1) : "no name";
            lua_getglobal(L, "script_help");
            script_entry->menu_entry->help = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
            
            lua_getglobal(L, "script_parameters");
            if(lua_istable(L, -1))
            {
                script_entry->menu_entry->select = menu_open_submenu;
                int params_count = luaL_len(L, -1);
                script_entry->menu_entry->children = malloc(sizeof(struct menu_entry) * (2 + params_count));
                memset(script_entry->menu_entry->children, 0, sizeof(struct menu_entry) * (2 + params_count));
                script_entry->menu_entry->children[0].name = "Run";
                script_entry->menu_entry->children[0].help = script_entry->menu_entry->help;
                int param_index = 1;
                for (lua_pushnil(L); lua_next(L, -1) != 0 && param_index <= params_count; lua_pop(L, 1))
                {
                    script_entry->menu_entry->children[param_index].name = lua_tostring(L, -2);
                    script_entry->menu_entry->children[param_index].unit = UNIT_DEC;
                }
                script_entry->menu_entry->children[params_count + 1].priv = MENU_EOL_PRIV;
            }
            else
            {
                script_entry->menu_entry->select = run_script;
            }
            script_entry->menu_entry->update = script_update;
            script_entry->menu_entry->priv = script_entry;
            script_entry->menu_entry->icon_type = IT_ACTION;
            menu_add("LUA", script_entry->menu_entry, 1);
            return 1;
        }
    }
    return 0;
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
    task_create("lua_load_task", 0x1c, 0x4000, lua_load_task, (void*) 0);
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


