#include <stdio.h>
#include "lua.h"

static int
lua_bmp_puts(
	lua_State * L
)
{
	size_t len;
	const char * msg = luaL_checklstring( L, 1, &len );
	printf( "bmp_puts: '%s' (len %d)\n", msg, len );
	return 0;
}

static int traceback (lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}



int main(void)
{
	lua_State * lua = lua_open();
	lua_register( lua, "bmp_puts", lua_bmp_puts );

	static const char buf[] = "bmp_puts('hello')\n";

	lua_settop( lua, 0 );
	lua_pushstring( lua, buf );
	luaL_loadbuffer( lua, lua_tostring(lua,1), lua_strlen(lua,1), "=(command line)" );
	int narg = 0;
	int base = lua_gettop(lua) - narg;
	printf( "base=%d\n", base );
  //lua_pushcfunction(lua, traceback);  /* push traceback function */
  //lua_insert(lua, base);  /* put it under chunk and args */
	int rc = lua_pcall( lua, narg, 0, base );
  //lua_remove(lua, base);  /* remove traceback function */

	const char * msg = lua_tostring( lua, -1 );

	printf( "rc=%d '%s'\n", rc, msg ? msg : "" );
	return 0;
}
