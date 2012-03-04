ML_LIBLUA= \
	lua-ml-compat.o \
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
  #	liolib.o \
  # lmathlib.o \
  # loslib.o \

$(LUA_PATH)/liblua.a: $(foreach o,$(ML_LIBLUA),$(LUA_PATH)/$o)
	@-$(RM) $@
	$(call build,AR,$(AR) rcu $@ $^)
	$(call build,RANLIB,$(RANLIB) $@)

-include $(LUA_PATH)/.*.d
