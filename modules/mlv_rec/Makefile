
# define the module name - make sure name is max 8 characters
MODULE_NAME=mlv_rec
MODULE_OBJS=mlv_rec.o mlv.o

# include modules environment
include ../Makefile.modules

MLV_CFLAGS = -I$(SRC_DIR) -D MLV_USE_LZMA -Wpadded -mno-ms-bitfields -D _7ZIP_ST -D MLV2DNG
MLV_LFLAGS = 
MLV_LIBS = -lm
MLV_LIBS_MINGW = -lm


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
endif

# LZMA library flags
LZMA_DIR=lzma/
LZMA_INC=-I$(LZMA_DIR)
LZMA_LIB=$(LZMA_DIR)lib7z.a
LZMA_LIB_MINGW=$(LZMA_DIR)lib7z.w32.a
# linux version doesnt support multi threading?
LZMA_OBJS=$(LZMA_DIR)7zAlloc.host.o $(LZMA_DIR)7zBuf.host.o $(LZMA_DIR)7zBuf2.host.o $(LZMA_DIR)7zCrc.host.o $(LZMA_DIR)7zCrcOpt.host.o $(LZMA_DIR)7zDec.host.o $(LZMA_DIR)7zFile.host.o $(LZMA_DIR)7zIn.host.o $(LZMA_DIR)7zStream.host.o $(LZMA_DIR)Alloc.host.o $(LZMA_DIR)Bcj2.host.o $(LZMA_DIR)Bra.host.o $(LZMA_DIR)Bra86.host.o $(LZMA_DIR)BraIA64.host.o $(LZMA_DIR)CpuArch.host.o $(LZMA_DIR)Delta.host.o $(LZMA_DIR)LzFind.host.o $(LZMA_DIR)Lzma2Dec.host.o $(LZMA_DIR)Lzma2Enc.host.o $(LZMA_DIR)Lzma86Dec.host.o $(LZMA_DIR)Lzma86Enc.host.o $(LZMA_DIR)LzmaDec.host.o $(LZMA_DIR)LzmaEnc.host.o $(LZMA_DIR)LzmaLib.host.o $(LZMA_DIR)Ppmd7.host.o $(LZMA_DIR)Ppmd7Dec.host.o $(LZMA_DIR)Ppmd7Enc.host.o $(LZMA_DIR)Sha256.host.o $(LZMA_DIR)Xz.host.o $(LZMA_DIR)XzCrc64.host.o
LZMA_OBJS_MINGW=$(LZMA_DIR)Threads.w32.o $(LZMA_DIR)LzFindMt.w32.o $(LZMA_DIR)MtCoder.w32.o $(LZMA_DIR)7zAlloc.w32.o $(LZMA_DIR)7zBuf.w32.o $(LZMA_DIR)7zBuf2.w32.o $(LZMA_DIR)7zCrc.w32.o $(LZMA_DIR)7zCrcOpt.w32.o $(LZMA_DIR)7zDec.w32.o $(LZMA_DIR)7zFile.w32.o $(LZMA_DIR)7zIn.w32.o $(LZMA_DIR)7zStream.w32.o $(LZMA_DIR)Alloc.w32.o $(LZMA_DIR)Bcj2.w32.o $(LZMA_DIR)Bra.w32.o $(LZMA_DIR)Bra86.w32.o $(LZMA_DIR)BraIA64.w32.o $(LZMA_DIR)CpuArch.w32.o $(LZMA_DIR)Delta.w32.o $(LZMA_DIR)LzFind.w32.o $(LZMA_DIR)Lzma2Dec.w32.o $(LZMA_DIR)Lzma2Enc.w32.o $(LZMA_DIR)Lzma86Dec.w32.o $(LZMA_DIR)Lzma86Enc.w32.o $(LZMA_DIR)LzmaDec.w32.o $(LZMA_DIR)LzmaEnc.w32.o $(LZMA_DIR)LzmaLib.w32.o $(LZMA_DIR)Ppmd7.w32.o $(LZMA_DIR)Ppmd7Dec.w32.o $(LZMA_DIR)Ppmd7Enc.w32.o $(LZMA_DIR)Sha256.w32.o $(LZMA_DIR)Xz.w32.o $(LZMA_DIR)XzCrc64.w32.o


MLV_CFLAGS += $(LZMA_INC)
MLV_LFLAGS += 
MLV_LIBS += $(LZMA_LIB)
MLV_LIBS_MINGW += $(LZMA_LIB_MINGW)

MLV_DUMP_OBJS=mlv_dump.host.o $(SRC_DIR)/chdk-dng.host.o ../lv_rec/raw2dng.host.o $(LZMA_LIB) 
MLV_DUMP_OBJS_MINGW=mlv_dump.w32.o $(SRC_DIR)/chdk-dng.w32.o ../lv_rec/raw2dng.w32.o $(LZMA_LIB_MINGW) 


clean::
	$(call rm_files, mlv_dump mlv_dump.exe $(LZMA_OBJS) $(LZMA_LIB) $(LZMA_OBJS_MINGW) $(LZMA_LIB_MINGW) )

#
# rules for host and win32 objects
#
%.host.o: %.c
	$(call build,HOST_CC,$(HOST_CC) $(HOST_CFLAGS) $(HOST_LFS_FLAGS) $(MLV_CFLAGS) -o $@ -c $<)

%.w32.o: %.c
	$(call build,MINGW_GCC,$(MINGW_GCC) $(MINGW_CFLAGS) $(MINGW_LFS_FLAGS) $(MLV_CFLAGS) -o $@ -c $<)

#
# create static LZMA library
#
$(LZMA_LIB): $(LZMA_OBJS)
	$(call build,HOST_AR,$(HOST_AR) -q $@ $(LZMA_OBJS) )

$(LZMA_LIB_MINGW): $(LZMA_OBJS_MINGW)
	$(call build,MINGW_AR,$(MINGW_AR) -q $@ $(LZMA_OBJS_MINGW) )
    
#
# mlv_dump rules
#
mlv_dump: $(MLV_DUMP_OBJS)
	$(call build,HOST_CC,$(HOST_CC) $(HOST_LFLAGS) $(MLV_LFLAGS) $(MLV_DUMP_OBJS) -o $@ $(HOST_LIBS) $(MLV_LIBS) )

mlv_dump.exe: $(MLV_DUMP_OBJS_MINGW)
	$(call build,MINGW_GCC,$(MINGW_GCC) $(MINGW_LFLAGS) $(MLV_LFLAGS) $(MLV_DUMP_OBJS_MINGW) -o $@ $(MINGW_LIBS) $(MLV_LIBS_MINGW) )

