
# this is a very simple makefile for modules
TOP_DIR=../..

#lua stuff
LUA_SRC=lua

CORE_O= $(LUA_SRC)/lapi.o $(LUA_SRC)/lcode.o $(LUA_SRC)/lctype.o $(LUA_SRC)/ldebug.o $(LUA_SRC)/ldo.o $(LUA_SRC)/ldump.o $(LUA_SRC)/lfunc.o $(LUA_SRC)/lgc.o $(LUA_SRC)/llex.o $(LUA_SRC)/lmem.o $(LUA_SRC)/lobject.o $(LUA_SRC)/lopcodes.o $(LUA_SRC)/lparser.o $(LUA_SRC)/lstate.o $(LUA_SRC)/lstring.o $(LUA_SRC)/ltable.o $(LUA_SRC)/ltm.o $(LUA_SRC)/lundump.o $(LUA_SRC)/lvm.o $(LUA_SRC)/lzio.o
LIB_O= $(LUA_SRC)/lauxlib.o $(LUA_SRC)/lbaselib.o $(LUA_SRC)/lbitlib.o $(LUA_SRC)/lcorolib.o $(LUA_SRC)/ldblib.o $(LUA_SRC)/liolib.o $(LUA_SRC)/lmathlib.o $(LUA_SRC)/lstrlib.o $(LUA_SRC)/ltablib.o $(LUA_SRC)/lutf8lib.o $(LUA_SRC)/loadlib.o $(LUA_SRC)/linit.o
LUA_LIB_O= lua_globals.o lua_console.o lua_camera.o lua_lv.o lua_lens.o lua_movie.o lua_display.o lua_key.o lua_menu.o lua_dryos.o lua_interval.o lua_battery.o lua_task.o lua_property.o lua_constants.o
UMM_O= umm_malloc/umm_malloc.o

# define the module name - make sure name is max 8 characters
MODULE_NAME=lua
MODULE_OBJS=$(LUA_SRC)/ml-lua-shim.o $(CORE_O) $(LIB_O) $(LUA_LIB_O) lua.o dietlibc.a $(UMM_O)
MODULE_CFLAGS += -DLUA_32BITS -DLUA_COMPAT_FLOATSTRING -Idietlibc/include/

# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

# strip all Lua symbols, so we don't export internal Lua routines to other modules
# (we have nothing interesting to export yet)
# fixme: how to implement this in a cleaner way?
lua.sym: lua.mo
	$(call build,STRIP,$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$8;}" | sort > localsyms)
	$(call build,STRIP,$(OBJCOPY) lua.mo lua.mo --localize-symbols localsyms)
	$(call rm_files, localsyms)
	$(call build,EXPORTS,$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$2 \" \" \$$8;}" | sort > $@ && cat $@)

# Extract the stdio files that we need
DIET_OBJ = \
	fopen.o \
	freopen.o \
	fclose.o \
	fread.o \
	fwrite.o \
	fseek.o \
	ftell.o \
	fprintf.o \
	fflush.o \
	feof.o \
	fputs.o \
	fgets.o \
	fgetc_unlocked.o \
	fputc_unlocked.o \
	ungetc.o \
	fdglue.o \
	fdglue2.o \
	setvbuf.o \
	vfprintf.o \
	__v_printf.o \
	__ltostr.o \
	printf.o \
	vprintf.o \
	stdin.o \
	stdout.o \
	stderr.o \
	errno.o \
	errno_location.o \
	strerror.o \
	ferror.o \
	clearerr.o \
	errlist.o \
	errlistu.o \
	strtol.o \
	strtof.o \
	strstr.o \
	strspn.o \
	strpbrk.o \
	memchr.o \
	localeconv.o \

$(DIET_OBJ): $(DIETLIBC_A)
	$(call build,AR,$(AR) x $? $(DIET_OBJ))

dietlibc.a: $(DIET_OBJ)
	$(call build,AR,$(AR) cr $@ $^)

# also copy the scripts when running "make install" from the module directory
install_user_data::
	$(INSTALL_ALL_SCRIPTS)

# run "make install extra=1" to copy scripts from the extra dir
# into the main scripts dir on the card, so they autostart
ifeq ($(extra),1)
install_user_data::
	$(CP) $(SCRIPT_DIR)/extra/*.lua $(INSTALL_SCRIPTS_DIR)
endif

# run a syntax check after compiling
all:: syntax_check.log

syntax_check.log: $(shell find $(SCRIPT_DIR) -type f -name *.lua)
	$(call build,LUAC,(for f in $^; do luac -p $$f || true; done) 2> syntax_check.log && cat syntax_check.log)
