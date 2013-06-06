
# define the module name - make sure name is max 8 characters
MODULE_NAME=lv_rec
MODULE_OBJS=lv_rec.o

# include modules environment
include ../Makefile.modules

all:: raw2dng


# RAW to DNG converter for PC
raw2dng: FORCE
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

MINGW=~/mingw-w32/bin/i686-w64-mingw32-gcc

raw2dng.exe: FORCE
	$(call build,MINGW,$(MINGW) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW,$(MINGW) -c raw2dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,MINGW,$(MINGW) raw2dng.o chdk-dng.o -o raw2dng.exe -lm -m32)

clean::
	$(call rm_files, raw2dng)
