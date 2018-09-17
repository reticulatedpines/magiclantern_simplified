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

#ifndef _lua_common_h
#define _lua_common_h

#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

#define MAX_PATH_LEN 0x80
#define SCRIPTS_DIR "ML/SCRIPTS"
#define CBR_RET_KEYPRESS_NOTHANDLED 1
#define CBR_RET_KEYPRESS_HANDLED 0
//#define CONFIG_VSYNC_EVENTS

#define LUA_MENU_UNLOAD_MASK   0
#define LUA_TASK_UNLOAD_MASK   1
#define LUA_PROP_UNLOAD_MASK   2
#define LUA_LVINFO_UNLOAD_MASK 3
//this needs to always be last b/c each event takes it's own mask starting at this one
#define LUA_EVENT_UNLOAD_MASK  4

#define LUA_PARAM_INT(name, index)\
if(index > lua_gettop(L) || !(lua_isinteger(L, index) || lua_isnumber(L,index))) return luaL_argerror(L, index, "expected integer for param '" #name "'");\
int name = lua_isinteger(L, index) ? lua_tointeger(L, index) : (int)lua_tonumber(L, index)

#define LUA_PARAM_INT_OPTIONAL(name, index, default) int name = (index <= lua_gettop(L) && (lua_isinteger(L, index) || lua_isnumber(L,index))) ? (lua_isinteger(L, index) ? lua_tointeger(L, index) : (int)lua_tonumber(L, index)) : default

#define LUA_PARAM_BOOL(name, index)\
if(index > lua_gettop(L) || !lua_isboolean(L, index)) return luaL_argerror(L, index, "expected boolean for param '" #name "'");\
int name = lua_toboolean(L, index)

#define LUA_PARAM_BOOL_OPTIONAL(name, index, default) int name = (index <= lua_gettop(L) && lua_isboolean(L, index)) ? lua_toboolean(L, index) : default

#define LUA_PARAM_NUMBER(name, index)\
if(index > lua_gettop(L) || !(lua_isinteger(L, index) || lua_isnumber(L,index))) return luaL_argerror(L, index, "expected number for param '" #name "'");\
float name = lua_tonumber(L, index)

#define LUA_PARAM_NUMBER_OPTIONAL(name, index, default) float name = (index <= lua_gettop(L) && (lua_isinteger(L, index) || lua_isnumber(L,index))) ? lua_tonumber(L, index) : default

#define LUA_PARAM_STRING(name, index)\
if(index > lua_gettop(L) || !lua_isstring(L, index)) return luaL_argerror(L, index, "expected string for param '" #name "'");\
const char * name = lua_tostring(L, index)

#define LUA_PARAM_STRING_OPTIONAL(name, index, default) const char * name = (index <= lua_gettop(L) && lua_isstring(L, index)) ? lua_tostring(L, index) : default

#define LUA_FIELD_STRING(field, default) lua_getfield(L, -1, field) == LUA_TSTRING ? copy_string(lua_tostring(L, -1)) : copy_string(default); lua_pop(L, 1)
#define LUA_FIELD_INT(field, default) lua_getfield(L, -1, field) == LUA_TNUMBER ? lua_tointeger(L, -1) : default; lua_pop(L, 1)
#define LUA_FIELD_BOOL(field, default) lua_getfield(L, -1, field) == LUA_TBOOLEAN ? lua_toboolean(L, -1) : default; lua_pop(L, 1)

#define LUA_CONSTANT(name, value) lua_pushinteger(L, value); lua_setfield(L, -2, #name)

#define LUA_LIB(name)\
int luaopen_##name(lua_State * L) {\
    lua_newtable(L);\
    luaL_setfuncs(L, name##lib, 0);\
    lua_newtable(L);\
    lua_pushcfunction(L, luaCB_##name##_index);\
    lua_setfield(L, -2, "__index");\
    lua_pushcfunction(L, luaCB_##name##_newindex);\
    lua_setfield(L, -2, "__newindex");\
    lua_pushcfunction(L, luaCB_pairs);\
    lua_setfield(L, -2, "__pairs");\
    lua_pushlightuserdata(L, lua_##name##_fields);\
    lua_setfield(L, -2, "fields");\
    lua_setmetatable(L, -2);\
    return 1;\
}

struct script_menu_entry
{
    int menu_value;
    lua_State * L;
    struct menu_entry * menu_entry;
    int self_ref;
    int select_ref;
    int update_ref;
    int warning_ref;
    int info_ref;
    int rinfo_ref;
    int submenu_ref;
};

char * copy_string(const char * str);
int docall(lua_State *L, int narg, int nres);

int lua_take_semaphore(lua_State * L, int timeout, struct semaphore ** assoc_semaphore);
int lua_give_semaphore(lua_State * L, struct semaphore ** assoc_semaphore);
int lua_msg_queue_receive(lua_State * L, uint32_t * msg, int timeout);

void lua_set_cant_unload(lua_State * L, int cant_unload, int mask);
void lua_save_last_error(lua_State * L);
void lua_set_last_menu(lua_State * L, const char * parent_menu, const char * menu_entry);

void lua_set_cant_yield(lua_State * L, int cant_yield);
int  lua_get_cant_yield(lua_State * L);

const char * lua_get_script_filename(lua_State * L);

void lua_stack_dump(lua_State *L);

int luaCB_next(lua_State * L);
int luaCB_pairs(lua_State * L);

int luaopen_globals(lua_State * L);
int luaopen_console(lua_State * L);
int luaopen_camera(lua_State * L);
int luaopen_lv(lua_State * L);
int luaopen_lens(lua_State * L);
int luaopen_movie(lua_State * L);
int luaopen_display(lua_State * L);
int luaopen_key(lua_State * L);
int luaopen_menu(lua_State * L);
int luaopen_dryos(lua_State * L);
int luaopen_interval(lua_State * L);
int luaopen_battery(lua_State * L);
int luaopen_task(lua_State * L);
int luaopen_property(lua_State *L);
int luaopen_constants(lua_State *L);

int luaopen_MODE(lua_State * L);
int luaopen_ICON_TYPE(lua_State * L);
int luaopen_UNIT(lua_State * L);
int luaopen_DEPENDS_ON(lua_State * L);
int luaopen_FONT(lua_State * L);
int luaopen_COLOR(lua_State * L);
int luaopen_KEY(lua_State * L);

#endif
