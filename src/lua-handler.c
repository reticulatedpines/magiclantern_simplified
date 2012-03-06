#include "lua-handler.h"
#include "lauxlib.h"

static lua_State* L;

static void lua_script_display(void* priv, int x, int y, int selected) {
	bmp_printf(FONT_LARGE, x, y, "Run lua script");
}

static int lua_printf( lua_State* L ) {
	int x, y;
	size_t l;
	x = luaL_checknumber( L, 1 );
	y = luaL_checknumber( L, 2 );
	const char *s = luaL_checklstring(L, 3, &l);
	bmp_printf(FONT_LARGE, x, y, "%s", s);
	return 0;
};

	static void
lua_script_run( void * priv, int delta )
{
	int ret;
	// asserts
	console_printf("Opening Lua state\n");
	L = luaL_newstate();
	if ( L ) {
		console_printf("Initializing functions\n");
		luaL_openlibs( L );
		lua_pushcfunction( L, lua_printf);
		lua_setglobal( L, "bmpprint");
		console_printf("Running script\n");
		if ((ret = luaL_loadstring( L,"bmpprint(100,100,'HELLO!');" ))!=0) {
			if (ret == LUA_ERRSYNTAX) {
				size_t lmsg;
				const char *msg = lua_tolstring(L, -1, &lmsg);
				if (msg) {
					console_printf("Syntax error %d (%s)\n",lmsg,msg);
				} else {
					console_printf("Syntax error\n");
				}
			} else {
				console_printf("Load error: %d\n", ret);
			}
		} else if ((ret = lua_pcall( L, 0, LUA_MULTRET, 0))!=0) {
			if (ret == LUA_ERRRUN) {
				size_t lmsg;
				const char *msg = lua_tolstring(L, -1, &lmsg);
				if (msg) {
					console_printf("Runtime error %d (%s)\n",lmsg,msg);
				} else {
					console_printf("Runtime error\n");
				}
			} else {
				console_printf("Call error: %d\n", ret);
			}
		} else {
			console_printf("Script has run succesfully\n", ret);
		}
		lua_close( L );
	}
	return;
}

static struct menu_entry lua_menus[] = {
	{
		.name       = "Start LUA script",
		.priv       = NULL,
		.display    = lua_script_display,
		.select     = lua_script_run,
		.help       = "Start a predefined LUA script",
		.essential  = FOR_PHOTO | FOR_MOVIE
	}
};

	static void
lua_init( void *unused )
{
	menu_add("Tweaks", lua_menus, COUNT(lua_menus) );
	return;
}

INIT_FUNC( __FILE__, lua_init );
