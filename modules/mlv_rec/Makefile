
# define the module name - make sure name is max 8 characters
MODULE_NAME=raw_rec
MODULE_OBJS=raw_rec.o mlv.o

# include modules environment
include ../Makefile.modules


MINGW=i686-w64-mingw32
MINGW_GCC=$(MINGW)-gcc
MINGW_AR=$(MINGW)-ar

# large file support
# get them with $(shell getconf LFS_CFLAGS)
LFS_FLAGS=-DLARGEFILES -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 
MLV_CFLAGS=-DMLV_USE_LZMA -Wpadded -mno-ms-bitfields -std=c99 -m32 -O2 -Wall $(LFS_FLAGS) -I$(SRC_DIR)

# LZMA library flags
LZMA_DIR=lzma/
LZMA_CFLAGS=-c -O2 -Wall -c
LZMA_INC=-I$(LZMA_DIR)
LZMA_LIB=$(LZMA_DIR)Util/7z/lib7z.a
LZMA_LIB_MINGW=$(LZMA_DIR)Util/7z/lib7z.w32.a
# linux version doesnt support multi threading?
LZMA_OBJS=$(LZMA_DIR)7zAlloc.host.o $(LZMA_DIR)7zBuf.host.o $(LZMA_DIR)7zBuf2.host.o $(LZMA_DIR)7zCrc.host.o $(LZMA_DIR)7zCrcOpt.host.o $(LZMA_DIR)7zDec.host.o $(LZMA_DIR)7zFile.host.o $(LZMA_DIR)7zIn.host.o $(LZMA_DIR)7zStream.host.o $(LZMA_DIR)Alloc.host.o $(LZMA_DIR)Bcj2.host.o $(LZMA_DIR)Bra.host.o $(LZMA_DIR)Bra86.host.o $(LZMA_DIR)BraIA64.host.o $(LZMA_DIR)CpuArch.host.o $(LZMA_DIR)Delta.host.o $(LZMA_DIR)LzFind.host.o $(LZMA_DIR)Lzma2Dec.host.o $(LZMA_DIR)Lzma2Enc.host.o $(LZMA_DIR)Lzma86Dec.host.o $(LZMA_DIR)Lzma86Enc.host.o $(LZMA_DIR)LzmaDec.host.o $(LZMA_DIR)LzmaEnc.host.o $(LZMA_DIR)LzmaLib.host.o $(LZMA_DIR)Ppmd7.host.o $(LZMA_DIR)Ppmd7Dec.host.o $(LZMA_DIR)Ppmd7Enc.host.o $(LZMA_DIR)Sha256.host.o $(LZMA_DIR)Xz.host.o $(LZMA_DIR)XzCrc64.host.o
LZMA_OBJS_MINGW=$(LZMA_DIR)Threads.w32.o $(LZMA_DIR)LzFindMt.w32.o $(LZMA_DIR)MtCoder.w32.o $(LZMA_DIR)7zAlloc.w32.o $(LZMA_DIR)7zBuf.w32.o $(LZMA_DIR)7zBuf2.w32.o $(LZMA_DIR)7zCrc.w32.o $(LZMA_DIR)7zCrcOpt.w32.o $(LZMA_DIR)7zDec.w32.o $(LZMA_DIR)7zFile.w32.o $(LZMA_DIR)7zIn.w32.o $(LZMA_DIR)7zStream.w32.o $(LZMA_DIR)Alloc.w32.o $(LZMA_DIR)Bcj2.w32.o $(LZMA_DIR)Bra.w32.o $(LZMA_DIR)Bra86.w32.o $(LZMA_DIR)BraIA64.w32.o $(LZMA_DIR)CpuArch.w32.o $(LZMA_DIR)Delta.w32.o $(LZMA_DIR)LzFind.w32.o $(LZMA_DIR)Lzma2Dec.w32.o $(LZMA_DIR)Lzma2Enc.w32.o $(LZMA_DIR)Lzma86Dec.w32.o $(LZMA_DIR)Lzma86Enc.w32.o $(LZMA_DIR)LzmaDec.w32.o $(LZMA_DIR)LzmaEnc.w32.o $(LZMA_DIR)LzmaLib.w32.o $(LZMA_DIR)Ppmd7.w32.o $(LZMA_DIR)Ppmd7Dec.w32.o $(LZMA_DIR)Ppmd7Enc.w32.o $(LZMA_DIR)Sha256.w32.o $(LZMA_DIR)Xz.w32.o $(LZMA_DIR)XzCrc64.w32.o



all:: raw2dng mlv_dump

clean::
	$(call rm_files, raw2dng mlv_dump $(LZMA_OBJS) $(LZMA_LIB) $(LZMA_OBJS_MINGW) $(LZMA_LIB_MINGW) )

# RAW to DNG converter for PC
raw2dng: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c ../lv_rec/raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

raw2dng.exe: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,MINGW-GCC,$(MINGW_GCC) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW-GCC,$(MINGW_GCC) -c ../lv_rec/raw2dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,MINGW-GCC,$(MINGW_GCC) raw2dng.o chdk-dng.o -o raw2dng.exe -lm -m32)

# debug tool
dng2raw: dng2raw.c
	$(call build,GCC,gcc dng2raw.c -m32 -O2 -Wall -I$(SRC_DIR)) -o dng2raw

#
# rules for host and win32 objects
#
%.host.o: %.c
	gcc $(LZMA_CFLAGS) -D_7ZIP_ST  -mno-ms-bitfields -o $@ $<

%.w32.o: %.c
	$(MINGW_GCC) $(LZMA_CFLAGS) -D_7ZIP_MT  -mno-ms-bitfields -o $@ $<

#
# create static LZMA library
#
$(LZMA_LIB): $(LZMA_OBJS)
	ar -q $@ $(LZMA_OBJS)

$(LZMA_LIB_MINGW): $(LZMA_OBJS_MINGW)
	$(MINGW_AR) -q $@ $(LZMA_OBJS_MINGW)
    
#
# mlv_dump rules
#
mlv_dump: $(LZMA_LIB) mlv_dump.c
	$(call build,GCC,gcc -o $@ mlv_dump.c $(LZMA_LIB) $(LZMA_INC) $(MLV_CFLAGS))

mlv_dump.exe: $(LZMA_LIB_MINGW) mlv_dump.c
	$(call build,MINGW-GCC,$(MINGW_GCC) -o $@ mlv_dump.c $(LZMA_LIB_MINGW) $(LZMA_INC) $(MLV_CFLAGS))
