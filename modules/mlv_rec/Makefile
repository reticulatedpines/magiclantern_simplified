
# define the module name - make sure name is max 8 characters
MODULE_NAME=raw_rec
MODULE_OBJS=raw_rec.o mlv.o

# include modules environment
include ../Makefile.modules

all:: raw2dng

mlv_dump: mlv_dump.c
	$(call build,GCC,gcc -c mlv_dump.c -o mlv_dump)

# RAW to DNG converter for PC
raw2dng: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c ../lv_rec/raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

# debug tool
dng2raw: dng2raw.c
	$(call build,GCC,gcc dng2raw.c -m32 -O2 -Wall -I$(SRC_DIR)) -o dng2raw

MINGW=~/mingw-w32/bin/i686-w64-mingw32-gcc

raw2dng.exe: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,MINGW,$(MINGW) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW,$(MINGW) -c ../lv_rec/raw2dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,MINGW,$(MINGW) raw2dng.o chdk-dng.o -o raw2dng.exe -lm -m32)

clean::
	$(call rm_files, raw2dng, mlv_dump)
