# define the module name - make sure name is max 8 characters
MODULE_NAME = mlv_rec
MODULE_OBJS = $(BUILD_DIR)/mlv_rec.o $(BUILD_DIR)/mlv.o
TOP_DIR = ../../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

MLV_CFLAGS = -I$(SRC_DIR) -D MLV_USE_LZMA -D MLV_USE_LJ92 -Wpadded -mno-ms-bitfields -D _7ZIP_ST
MLV_LFLAGS = 
MLV_LIBS = -lm
MLV_LIBS_MINGW = -lm

# detect kernel version, if it contains Microsoft, it is WSL
ifneq (,$(findstring Microsoft,$(shell uname -r)))
    # and if that runs on a x64 machine, then enforce a 64 bit build
    ifneq (,$(findstring x86_64,$(shell uname -p)))
        USE_X64 = yes
    endif
endif

# to allow x64 binaries. doesn't work on OSX, seems to SIGSEGV for some reason.
ifndef USE_X64
    MLV_CFLAGS += -m32
    MLV_LFLAGS += -m32
endif

# just comment out to disable LUA
#LUA_VER=5.2

ifdef LUA_VER
    MLV_CFLAGS += $(shell pkg-config --cflags lua$(LUA_VER)) -D USE_LUA
    MLV_LFLAGS += 
    MLV_LIBS += $(shell pkg-config --libs lua$(LUA_VER))
    MLV_LIBS_MINGW += $(shell pkg-config --libs lua$(LUA_VER))
endif


# add target architecture for OSX
ifdef FOR_OSX
    MLV_CFLAGS += -arch i386 -mmacosx-version-min=10.6
    MLV_LFLAGS += -arch i386 -mmacosx-version-min=10.6
endif

# LZMA library flags
LZMA_DIR = lzma
LZMA_INC = -I$(LZMA_DIR) -I$(BUILD_DIR)/$(LZMA_DIR)
LZMA_LIB = $(BUILD_DIR)/$(LZMA_DIR)/lib7z.a
LZMA_LIB_MINGW = $(LZMA_DIR)/lib7z.w32.a
# linux version doesnt support multi threading?
LZMA_OBJS_00 = 7zAlloc.host.o \
               7zBuf.host.o \
               7zBuf2.host.o \
               7zCrc.host.o \
               7zCrcOpt.host.o \
               7zDec.host.o \
               7zFile.host.o \
               7zIn.host.o \
               7zStream.host.o \
               Alloc.host.o \
               Bcj2.host.o \
               Bra.host.o \
               Bra86.host.o \
               BraIA64.host.o \
               CpuArch.host.o \
               Delta.host.o \
               LzFind.host.o \
               Lzma2Dec.host.o \
               Lzma2Enc.host.o \
               Lzma86Dec.host.o \
               Lzma86Enc.host.o \
               LzmaDec.host.o \
               LzmaEnc.host.o \
               LzmaLib.host.o \
               Ppmd7.host.o \
               Ppmd7Dec.host.o \
               Ppmd7Enc.host.o \
               Sha256.host.o \
               Xz.host.o \
               XzCrc64.host.o
LZMA_OBJS = $(addprefix $(BUILD_DIR)/$(LZMA_DIR)/,$(LZMA_OBJS_00))

LZMA_OBJS_MINGW_00 = Threads.w32.o \
                     LzFindMt.w32.o \
                     MtCoder.w32.o \
                     7zAlloc.w32.o \
                     7zBuf.w32.o \
                     7zBuf2.w32.o \
                     7zCrc.w32.o \
                     7zCrcOpt.w32.o \
                     7zDec.w32.o \
                     7zFile.w32.o \
                     7zIn.w32.o \
                     7zStream.w32.o \
                     Alloc.w32.o \
                     Bcj2.w32.o \
                     Bra.w32.o \
                     Bra86.w32.o \
                     BraIA64.w32.o \
                     CpuArch.w32.o \
                     Delta.w32.o \
                     LzFind.w32.o \
                     Lzma2Dec.w32.o \
                     Lzma2Enc.w32.o \
                     Lzma86Dec.w32.o \
                     Lzma86Enc.w32.o \
                     LzmaDec.w32.o \
                     LzmaEnc.w32.o \
                     LzmaLib.w32.o \
                     Ppmd7.w32.o \
                     Ppmd7Dec.w32.o \
                     Ppmd7Enc.w32.o \
                     Sha256.w32.o \
                     Xz.w32.o \
                     XzCrc64.w32.o
LZMA_OBJS_MINGW = $(addprefix $(BUILD_DIR)/$(LZMA_DIR)/,$(LZMA_OBJS_MINGW_00))

DNG_OBJS = $(BUILD_DIR)/dng/dng.host.o
DNG_OBJS_MINGW = $(BUILD_DIR)/dng/dng.w32.o

RAW_PROC_DIR = raw_proc
RAW_PROC_OBJS_00 = stripes.host.o pixel_proc.host.o patternnoise.host.o histogram.host.o
RAW_PROC_OBJS = $(addprefix $(BUILD_DIR)/$(RAW_PROC_DIR)/,$(RAW_PROC_OBJS_00))

RAW_PROC_OBJS_MINGW_00 = stripes.w32.o pixel_proc.w32.o patternnoise.w32.o histogram.w32.o
RAW_PROC_OBJS_MINGW = $(addprefix $(BUILD_DIR)/$(RAW_PROC_DIR)/,$(RAW_PROC_OBJS_MINGW_00))

MLV_CFLAGS += $(LZMA_INC)
MLV_LFLAGS += 
MLV_LIBS += $(LZMA_LIB)
MLV_LIBS_MINGW += $(LZMA_LIB_MINGW)

MLV_DUMP_OBJS = $(BUILD_DIR)/mlv_dump.host.o $(BUILD_DIR)/lj92.host.o \
                $(DNG_OBJS) $(RAW_PROC_OBJS) $(LZMA_LIB)
MLV_DUMP_OBJS_MINGW = $(BUILD_DIR)/mlv_dump.w32.o $(BUILD_DIR)/lj92.w32.o \
                      $(DNG_OBJS_MINGW) $(RAW_PROC_OBJS_MINGW) $(LZMA_LIB_MINGW)


#clean::
#	$(call rm_files, mlv_dump mlv_dump.exe $(LZMA_OBJS) $(LZMA_LIB) $(LZMA_OBJS_MINGW) $(LZMA_LIB_MINGW) $(MLV_DUMP_OBJS) $(MLV_DUMP_OBJS_MINGW) )

#
# rules for host and win32 objects
#
$(BUILD_DIR)/%.host.o: %.c
	@mkdir -p $(BUILD_DIR)/dng
	@mkdir -p $(BUILD_DIR)/lzma
	@mkdir -p $(BUILD_DIR)/$(RAW_PROC_DIR)
	@echo $(TEST_PROC_PREFIX)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_LFS_FLAGS) $(MLV_CFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.w32.o: %.c
	@mkdir -p $(BUILD_DIR)/dng
	@mkdir -p $(BUILD_DIR)/lzma
	@mkdir -p $(BUILD_DIR)/$(RAW_PROC_DIR)
	$(MINGW_GCC) $(MINGW_CFLAGS) $(MINGW_LFS_FLAGS) $(MLV_CFLAGS) -o $@ -c $<

#
# create static LZMA library
#
$(LZMA_LIB): $(LZMA_OBJS)
	@mkdir -p $(BUILD_DIR)/lzma
	$(HOST_AR) -q $@ $(LZMA_OBJS)

$(LZMA_LIB_MINGW): $(LZMA_OBJS_MINGW)
	@mkdir -p $(BUILD_DIR)/lzma
	$(MINGW_AR) -q $@ $(LZMA_OBJS_MINGW)
    
#
# mlv_dump rules
#
mlv_dump.c: $(BUILD_DIR)/module_strings.h

$(BUILD_DIR)/mlv_dump: $(MLV_DUMP_OBJS)
	@mkdir -p $(BUILD_DIR)/dng
	@mkdir -p $(BUILD_DIR)/$(RAW_PROC_DIR)
	$(HOST_CC) $(HOST_LFLAGS) $(MLV_LFLAGS) $(MLV_DUMP_OBJS) -o $@ $(HOST_LIBS) $(MLV_LIBS)

$(BUILD_DIR)/mlv_dump.exe: $(BUILD_DIR)/module_strings.h $(MLV_DUMP_OBJS_MINGW)
	@mkdir -p $(BUILD_DIR)/dng
	@mkdir -p $(BUILD_DIR)/$(RAW_PROC_DIR)
	$(MINGW_GCC) $(MINGW_LFLAGS) $(MLV_LFLAGS) $(MLV_DUMP_OBJS_MINGW) -o $@ $(MINGW_LIBS) $(MLV_LIBS_MINGW)
