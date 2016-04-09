
# this is a very simple makefile for modules
TOP_DIR=../..

#lua stuff
LUA_SRC=lua

CORE_O= $(LUA_SRC)/lapi.o $(LUA_SRC)/lcode.o $(LUA_SRC)/lctype.o $(LUA_SRC)/ldebug.o $(LUA_SRC)/ldo.o $(LUA_SRC)/ldump.o $(LUA_SRC)/lfunc.o $(LUA_SRC)/lgc.o $(LUA_SRC)/llex.o $(LUA_SRC)/lmem.o $(LUA_SRC)/lobject.o $(LUA_SRC)/lopcodes.o $(LUA_SRC)/lparser.o $(LUA_SRC)/lstate.o $(LUA_SRC)/lstring.o $(LUA_SRC)/ltable.o $(LUA_SRC)/ltm.o $(LUA_SRC)/lundump.o $(LUA_SRC)/lvm.o $(LUA_SRC)/lzio.o
LIB_O= $(LUA_SRC)/lauxlib.o $(LUA_SRC)/lbaselib.o $(LUA_SRC)/lbitlib.o $(LUA_SRC)/lcorolib.o $(LUA_SRC)/ldblib.o $(LUA_SRC)/liolib.o $(LUA_SRC)/lmathlib.o $(LUA_SRC)/lstrlib.o $(LUA_SRC)/ltablib.o $(LUA_SRC)/lutf8lib.o $(LUA_SRC)/loadlib.o $(LUA_SRC)/linit.o
LUA_LIB_O= lua_globals.o lua_console.o lua_camera.o lua_lv.o lua_lens.o lua_movie.o lua_display.o lua_key.o lua_menu.o lua_dryos.o lua_interval.o lua_battery.o lua_task.o lua_property.o lua_constants.o

# define the module name - make sure name is max 8 characters
MODULE_NAME=lua
MODULE_OBJS=$(LUA_SRC)/ml-lua-shim.o $(CORE_O) $(LIB_O) $(LUA_LIB_O) lua.o
MODULE_CFLAGS += -DLUA_32BITS -DLUA_COMPAT_FLOATSTRING

# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

# strip all Lua symbols, so we don't export internal Lua routines to other modules
# (we have nothing interesting to export yet)
# fixme: how to implement this in a cleaner way?
lua.sym: lua.mo
	$(call build,STRIP,$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$8;}" | sort > localsyms)
	$(call build,STRIP,$(OBJCOPY) lua.mo lua.mo --localize-symbols localsyms)
	$(call build,EXPORTS,$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$2 \" \" \$$8;}" | sort > $@ && cat $@)

# also copy the scripts when running "make install" from the module directory
install::
	$(CP) $(SCRIPT_DIR)/*.lua $(INSTALL_SCRIPTS_DIR)/
	$(CP) $(SCRIPT_LIB_DIR)/*.lua $(INSTALL_SCRIPTS_LIB_DIR)/

# run a syntax check after compiling
all:: syntax_check.log

syntax_check.log: $(SCRIPT_DIR)/*.lua $(SCRIPT_LIB_DIR)/*.lua
	$(call build,LUAC,(for f in $^; do luac -p $$f || true; done) 2> syntax_check.log && cat syntax_check.log)
