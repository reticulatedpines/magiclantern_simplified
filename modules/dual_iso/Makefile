
# this is a very simple makefile for modules
TOP_DIR=../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=dual_iso
MODULE_OBJS=dual_iso.o

all:: cr2hdr

# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

cr2hdr: $(SRC_DIR)/chdk-dng.c cr2hdr.c dcraw-bridge.c
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -ggdb -I$(SRC_DIR))
	$(call build,GCC,gcc -c dcraw-bridge.c -m32 -mno-ms-bitfields -O2 -Wall -ggdb -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc -c cr2hdr.c -m32 -mno-ms-bitfields -O2 -Wall -ggdb -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc dcraw-bridge.o cr2hdr.o chdk-dng.o -o cr2hdr -lm -m32 -ggdb)

MINGW=~/mingw-w32/bin/i686-w64-mingw32-gcc

cr2hdr.exe: $(SRC_DIR)/chdk-dng.c cr2hdr.c dcraw-bridge.c
	$(call build,MINGW,$(MINGW) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW,$(MINGW) -c dcraw-bridge.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,MINGW,$(MINGW) -c cr2hdr.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,MINGW,$(MINGW) cr2hdr.o chdk-dng.o -o cr2hdr.exe -lm -m32)

clean::
	$(call rm_files, cr2hdr)
