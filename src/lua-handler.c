#include "lua-handler.h"

static lua_State* L;

static void lua_script_display(void* priv, int x, int y, int selected) {
    bmp_printf(FONT_LARGE, x, y, "Run lua script");
}

static void
lua_script_run( void * priv, int delta )
{
    L = luaL_newstate();
    luaL_openlibs( L );
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
