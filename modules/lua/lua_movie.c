/***
 Movie functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module movie
 */

#include <dryos.h>
#include <string.h>
#include <fps.h>
#include <shoot.h>
#include <property.h>

#include "lua_common.h"

/***
 Start recording a movie.
 @function start
 */
static int luaCB_movie_start(lua_State* L)
{
    if (!is_movie_mode())   return luaL_error(L, "Not in movie mode.");
    if (!lv)                return luaL_error(L, "Not in LiveView.");
    if (gui_menu_shown())   return luaL_error(L, "Please close ML menu.");
    if (RECORDING)          return luaL_error(L, "Already recording.");

    movie_start();
    return 0;
}

/***
 Stop recording a movie.
 @function stop
 */
static int luaCB_movie_stop(lua_State* L)
{
    if (!is_movie_mode())   return luaL_error(L, "Not in movie mode.");
    if (!lv)                return luaL_error(L, "Not in LiveView.");
    if (gui_menu_shown())   return luaL_error(L, "Please close ML menu.");
    if (!RECORDING)         return luaL_error(L, "Not recording.");

    movie_end();
    return 0;
}

static int luaCB_movie_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get whether or not the camera is recording a movie.
    // @tfield bool recording readonly
    if(!strcmp(key, "recording")) lua_pushboolean(L, RECORDING);
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_movie_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
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

static const char * lua_movie_fields[] =
{
    "recording",
    NULL
};

const luaL_Reg movielib[] =
{
    {"start", luaCB_movie_start},
    {"stop", luaCB_movie_stop},
    {NULL, NULL}
};

LUA_LIB(movie)
