# makefile for building Lua for Magic Lantern

liblua-y	+= \
	lapi.o \
	lcode.o \
	ldebug.o \
	ldo.o \
	ldump.o \
	lfunc.o \
	lgc.o \
	llex.o \
	lmem.o \
	lobject.o \
	lopcodes.o \
	lparser.o \
	lstate.o \
	lstring.o \
	ltable.o \
	ltm.o  \
	lundump.o \
	lvm.o \
	lzio.o \

liblua-y	+= \
	lauxlib.o \
	lbaselib.o \
	ldblib.o \
	liolib.o \
	lmathlib.o \
	loslib.o \
	ltablib.o \
	lstrlib.o \
	loadlib.o \
	linit.o \


$(LUA_PATH)/liblua.a: \
	$(foreach o,$(liblua-y),$(LUA_PATH)/$o)
	@-$(RM) $@
	$(call build,AR,$(AR) rcu $@ $^)
	$(call build,RANLIB,$(RANLIB) $@)


-include $(LUA_PATH)/.*.d
