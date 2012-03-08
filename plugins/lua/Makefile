PLUGIN_DIR=$(shell pwd)
PLUGIN_BASE_DIR=$(PLUGIN_DIR)/..
PLUGIN_NAME=lua
PLUGIN_OBJS=lua-ml-compat.o \
	setjmp.o \
	lapi.o \
	lauxlib.o \
	lbaselib.o \
	lbitlib.o \
	lcode.o \
	lcorolib.o \
	lctype.o \
	ldblib.o \
	ldebug.o \
	ldo.o \
	ldump.o \
	lfunc.o \
	lgc.o \
	linit.o \
	llex.o \
	lmem.o \
	loadlib.o \
	lobject.o \
	lopcodes.o \
	lparser.o \
	lstate.o \
	lstring.o \
	lstrlib.o \
	ltable.o \
	ltablib.o \
	ltm.o \
	lundump.o \
	lvm.o \
	lzio.o \
	lua-handler.o \
  #	liolib.o \
  # lmathlib.o \
  # loslib.o \

include $(PLUGIN_BASE_DIR)/Makefile.top

-include $(PLUGIN_DIR)/.*.d

