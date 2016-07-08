
# define the module name - make sure name is max 8 characters
MODULE_NAME=raw_rec
MODULE_OBJS=raw_rec.o

# include modules environment
include ../Makefile.modules

all:: raw2dng

# RAW to DNG converter for PC
raw2dng: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c ../lv_rec/raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

# debug tool
dng2raw: dng2raw.c
	$(call build,GCC,gcc dng2raw.c -m32 -O2 -Wall -I$(SRC_DIR)) -o dng2raw

raw2dng.exe: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,MINGW,$(MINGW_GCC) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW,$(MINGW_GCC) -c ../lv_rec/raw2dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,MINGW,$(MINGW_GCC) raw2dng.o chdk-dng.o -o raw2dng.exe -lm -m32)

dng2raw.exe: dng2raw.c
	$(call build,MINGW,$(MINGW_GCC) dng2raw.c -m32 -O2 -Wall -I$(SRC_DIR)) -o dng2raw.exe

clean::
	$(call rm_files, raw2dng raw2dng.exe dng2raw dng2raw.exe)
