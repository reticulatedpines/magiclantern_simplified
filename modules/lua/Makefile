TOP_DIR=../..

#lua stuff
LUA_SRC=lua

CORE_O_00 = lapi.o lcode.o lctype.o ldebug.o \
            ldo.o ldump.o lfunc.o lgc.o \
            llex.o lmem.o lobject.o lopcodes.o \
            lparser.o lstate.o lstring.o ltable.o \
            ltm.o lundump.o lvm.o lzio.o
CORE_O = $(addprefix $(BUILD_DIR)/$(LUA_SRC)/, $(CORE_O_00))

LIB_O_00 = lauxlib.o lbaselib.o lbitlib.o \
           lcorolib.o ldblib.o liolib.o \
           lmathlib.o lstrlib.o ltablib.o \
           lutf8lib.o loadlib.o linit.o
LIB_O = $(addprefix $(BUILD_DIR)/$(LUA_SRC)/, $(LIB_O_00))

LUA_LIB_O_00 = lua_globals.o lua_console.o lua_camera.o \
               lua_lv.o lua_lens.o lua_movie.o lua_display.o \
               lua_key.o lua_menu.o lua_dryos.o lua_interval.o \
               lua_battery.o lua_task.o lua_property.o lua_constants.o
LUA_LIB_O = $(addprefix $(BUILD_DIR)/, $(LUA_LIB_O_00))

UMM_O = $(BUILD_DIR)/umm_malloc/umm_malloc.o

# define the module name - make sure name is max 8 characters
MODULE_NAME = lua
MODULE_OBJS = $(BUILD_DIR)/$(LUA_SRC)/ml-lua-shim.o \
              $(CORE_O) $(LIB_O) $(LUA_LIB_O) \
              $(BUILD_DIR)/lua.o \
              $(BUILD_DIR)/dietlibc.a \
              $(UMM_O)
MODULE_CFLAGS += -DLUA_32BITS -DLUA_COMPAT_FLOATSTRING -Idietlibc/include/ \
                 -Wno-undef

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

# mildly ugly hack, force existence of some build subdirs
# without expressing them as a prerequisite
$(shell mkdir -p $(BUILD_DIR)/umm_malloc)
$(shell mkdir -p $(BUILD_DIR)/lua)
$(shell mkdir -p $(BUILD_DIR)/dietlibc)

# strip all Lua symbols, so we don't export internal Lua routines to other modules
# (we have nothing interesting to export yet)
# fixme: how to implement this in a cleaner way?
$(BUILD_DIR)/lua.sym: $(BUILD_DIR)/lua.mo
	$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$8;}" | sort > localsyms
	$(OBJCOPY) $(BUILD_DIR)/lua.mo $(BUILD_DIR)/lua.mo --localize-symbols localsyms
	rm -f localsyms
	$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$2 \" \" \$$8;}" | sort > $@ && cat $@

# Extract the stdio files that we need
DIET_OBJ_00 = \
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
    localeconv.o

DIET_OBJ = $(addprefix $(BUILD_DIR)/dietlibc/, $(DIET_OBJ_00))

$(DIET_OBJ): $(DIETLIBC_A)
	$(AR) x $? $(DIET_OBJ) --output $(BUILD_DIR)/dietlibc

$(BUILD_DIR)/dietlibc.a: $(DIET_OBJ)
	$(AR) cr $@ $^
