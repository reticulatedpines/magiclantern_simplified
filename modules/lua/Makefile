
# this is a very simple makefile for modules
TOP_DIR=../..

#lua stuff
LUA_SRC=lua

CORE_O= $(LUA_SRC)/lapi.o $(LUA_SRC)/lcode.o $(LUA_SRC)/lctype.o $(LUA_SRC)/ldebug.o $(LUA_SRC)/ldo.o $(LUA_SRC)/ldump.o $(LUA_SRC)/lfunc.o $(LUA_SRC)/lgc.o $(LUA_SRC)/llex.o $(LUA_SRC)/lmem.o $(LUA_SRC)/lobject.o $(LUA_SRC)/lopcodes.o $(LUA_SRC)/lparser.o $(LUA_SRC)/lstate.o $(LUA_SRC)/lstring.o $(LUA_SRC)/ltable.o $(LUA_SRC)/ltm.o $(LUA_SRC)/lundump.o $(LUA_SRC)/lvm.o $(LUA_SRC)/lzio.o
LIB_O= $(LUA_SRC)/lauxlib.o $(LUA_SRC)/lbaselib.o $(LUA_SRC)/lbitlib.o $(LUA_SRC)/lcorolib.o $(LUA_SRC)/ldblib.o $(LUA_SRC)/liolib.o $(LUA_SRC)/lmathlib.o $(LUA_SRC)/lstrlib.o $(LUA_SRC)/ltablib.o $(LUA_SRC)/lutf8lib.o $(LUA_SRC)/loadlib.o $(LUA_SRC)/linit.o

# define the module name - make sure name is max 8 characters
MODULE_NAME=lua
MODULE_OBJS=$(LUA_SRC)/ml-lua-shim.o $(CORE_O) $(LIB_O) lua.o
MODULE_CFLAGS += -DLUA_32BITS

# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

